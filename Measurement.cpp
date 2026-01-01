// Measurement.cpp
#include "Measurement.h"
#include "Core.h"

// OCCT Geometry Headers
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <gp_Pnt.hxx>
#include <AIS_TextLabel.hxx>
#include <AIS_Shape.hxx>
#include <QtMath>
#include <QFileInfo>

MeasurementManager::MeasurementManager(OcctQWidgetViewer* viewer)
    : m_viewer(viewer)
{
}

void MeasurementManager::extractMeshTopology()
{
    if (m_viewer->myLoadedShape.IsNull()) {
        Message::SendWarning() << "No shape loaded";
        return;
    }

    try {
        m_viewer->myFaceMap.Clear();
        m_viewer->myEdgeMap.Clear();

        TopExp_Explorer aFaceExplorer(m_viewer->myLoadedShape, TopAbs_FACE);
        while (aFaceExplorer.More()) {
            m_viewer->myFaceMap.Add(aFaceExplorer.Current());
            aFaceExplorer.Next();
        }

        TopExp_Explorer anEdgeExplorer(m_viewer->myLoadedShape, TopAbs_EDGE);
        while (anEdgeExplorer.More()) {
            m_viewer->myEdgeMap.Add(anEdgeExplorer.Current());
            anEdgeExplorer.Next();
        }

        Message::SendInfo() << "Extracted " << m_viewer->myFaceMap.Size() << " faces and "
                            << m_viewer->myEdgeMap.Size() << " edges";

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Error extracting topology: " << e.GetMessageString();
    }
}

void MeasurementManager::clearLabels()
{
    if (m_viewer->myContext.IsNull()) return;

    // Iterate over base class handles
    for (const Handle(AIS_InteractiveObject)& anObj : std::as_const(m_viewer->myPointLabels)) {
        m_viewer->myContext->Remove(anObj, Standard_False);
    }
    m_viewer->myPointLabels.clear();
}

void MeasurementManager::calculateMeasurements()
{
    // 1. Clear old labels
    clearLabels();

    // 2. Initialize Properties Structure
    ModelProperties props;

    // --- ✅ A. POPULATE FILE METADATA ---
    if (!m_viewer->myCurrentFilePath.isEmpty()) {
        QFileInfo fi(m_viewer->myCurrentFilePath);
        props.filename = fi.fileName();
        props.location = fi.absolutePath();

        double sizeBytes = fi.size();
        if (sizeBytes > 1024 * 1024) {
            props.size = QString::number(sizeBytes / (1024.0 * 1024.0), 'f', 2) + " MB";
        } else {
            props.size = QString::number(sizeBytes / 1024.0, 'f', 2) + " KB";
        }
    } else {
        props.filename = "Unknown";
        props.size = "-";
        props.location = "-";
    }

    // Init other props
    props.type = "Unknown";
    props.originX = 0.0; props.originY = 0.0; props.originZ = 0.0;
    props.area = 0.0; props.volume = 0.0; props.length = 0.0;
    props.radius = 0.0; props.diameter = 0.0; props.angle = 0.0;

    // Metrics tracking
    double totalArea = 0.0;
    double totalLength = 0.0;
    double totalVolume = 0.0;
    double lastDiameter = 0.0;
    double lastAngle = 0.0;

    QString pointTableData = "";

    // 3. Collect Unique Edges & Calculate Metrics
    TopTools_IndexedMapOfShape aUniqueEdges;
    QStringList types;

    m_viewer->myContext->InitSelected();
    while (m_viewer->myContext->MoreSelected()) {
        TopoDS_Shape aShape = m_viewer->myContext->SelectedShape();
        if (!aShape.IsNull()) {
            TopAbs_ShapeEnum aType = aShape.ShapeType();

            if (aType == TopAbs_FACE) {
                if (!types.contains("FACE")) types << "FACE";
                TopoDS_Face aFace = TopoDS::Face(aShape);

                GProp_GProps aProps;
                BRepGProp::SurfaceProperties(aFace, aProps);
                totalArea += aProps.Mass();

                // Extract Edges from Face
                TopExp_Explorer anEdgeExplorer(aFace, TopAbs_EDGE);
                while (anEdgeExplorer.More()) {
                    aUniqueEdges.Add(anEdgeExplorer.Current());
                    anEdgeExplorer.Next();
                }
            }
            else if (aType == TopAbs_EDGE) {
                if (!types.contains("EDGE")) types << "EDGE";
                aUniqueEdges.Add(aShape);
            }
            else if (aType == TopAbs_SOLID) {
                if (!types.contains("SOLID")) types << "SOLID";
                TopoDS_Solid aSolid = TopoDS::Solid(aShape);
                GProp_GProps aProps;
                BRepGProp::VolumeProperties(aSolid, aProps);
                totalVolume += aProps.Mass();
            }
        }
        m_viewer->myContext->NextSelected();
    }
    props.type = types.join("+");

    // 4. Process Edges List
    QList<TopoDS_Edge> selectedEdges;
    for (int i = 1; i <= aUniqueEdges.Extent(); ++i) {
        TopoDS_Edge anEdge = TopoDS::Edge(aUniqueEdges.FindKey(i));
        selectedEdges.append(anEdge);

        // Length
        GProp_GProps aProps;
        BRepGProp::LinearProperties(anEdge, aProps);
        totalLength += aProps.Mass();

        // --- CHECK CURVE / RADIUS / ANGLE ---
        BRepAdaptor_Curve aCurve(anEdge);
        if (aCurve.GetType() == GeomAbs_Circle) {
            lastDiameter = 2.0 * aCurve.Circle().Radius();

            double startP = aCurve.FirstParameter();
            double endP = aCurve.LastParameter();
            double angleRad = qAbs(endP - startP); // Use qAbs to be safe

            lastAngle = qRadiansToDegrees(angleRad);
            if (lastAngle > 359.9) lastAngle = 360.0;
        }
    }

    // Fill Property Values
    props.area = totalArea;
    props.length = totalLength;
    props.volume = totalVolume;
    if (lastDiameter > 0) {
        props.diameter = lastDiameter;
        props.radius = lastDiameter / 2.0;
        props.angle = lastAngle;
    }

    // --- PATH VISUALIZATION VARIABLES ---
    QList<TopoDS_Edge> orderedEdges;

    // 5. SORT EDGES (Snake Path Logic)
    if (!selectedEdges.isEmpty()) {
        QList<TopoDS_Edge> pool = selectedEdges;
        orderedEdges.append(pool.takeFirst());

        gp_Pnt chainEnd;
        if (!pool.isEmpty()) {
            TopoDS_Vertex V1, V2;
            TopExp::Vertices(orderedEdges.first(), V1, V2);
            gp_Pnt P1 = BRep_Tool::Pnt(V1);
            gp_Pnt P2 = BRep_Tool::Pnt(V2);
            bool p2Connects = false;
            for (const TopoDS_Edge& nextEdge : pool) {
                TopoDS_Vertex nV1, nV2;
                TopExp::Vertices(nextEdge, nV1, nV2);
                gp_Pnt nP1 = BRep_Tool::Pnt(nV1);
                gp_Pnt nP2 = BRep_Tool::Pnt(nV2);
                if (P2.Distance(nP1) < 1e-4 || P2.Distance(nP2) < 1e-4) {
                    p2Connects = true; break;
                }
            }
            chainEnd = p2Connects ? P2 : P1;
        } else {
            TopoDS_Vertex V1, V2;
            TopExp::Vertices(orderedEdges.first(), V1, V2);
            chainEnd = BRep_Tool::Pnt(V2);
        }

        while (!pool.isEmpty()) {
            int bestIndex = -1;
            double bestDist = 1e9;
            for (int i = 0; i < pool.size(); ++i) {
                TopoDS_Vertex V1, V2;
                TopExp::Vertices(pool[i], V1, V2);
                gp_Pnt P1 = BRep_Tool::Pnt(V1);
                gp_Pnt P2 = BRep_Tool::Pnt(V2);
                double d1 = chainEnd.Distance(P1);
                double d2 = chainEnd.Distance(P2);
                if (d1 < bestDist) { bestDist = d1; bestIndex = i; }
                if (d2 < bestDist) { bestDist = d2; bestIndex = i; }
            }

            if (bestIndex != -1 && bestDist < 1.0) {
                orderedEdges.append(pool.takeAt(bestIndex));
                TopoDS_Vertex V1, V2;
                TopExp::Vertices(orderedEdges.last(), V1, V2);
                gp_Pnt P1 = BRep_Tool::Pnt(V1);
                gp_Pnt P2 = BRep_Tool::Pnt(V2);
                chainEnd = (chainEnd.Distance(P1) < chainEnd.Distance(P2)) ? P2 : P1;
            } else {
                orderedEdges.append(pool.takeFirst());
                TopoDS_Vertex V1, V2;
                TopExp::Vertices(orderedEdges.last(), V1, V2);
                chainEnd = BRep_Tool::Pnt(V2);
            }
        }
    }

    // 6. GENERATE POINTS
    int pointCounter = 1;
    gp_Pnt lastPos;
    bool isFirstEdge = true;
    gp_Pnt lastLabelPos;
    gp_Pnt firstLabelPos;

    auto drawLabel = [&](const gp_Pnt& p, int id) {
        gp_Pnt textPos = p;
        if (id == 1) firstLabelPos = p;
        bool isOverlapping = (id > 1 && p.Distance(lastLabelPos) < 0.1) ||
                             (id > 1 && p.Distance(firstLabelPos) < 0.1);
        Handle(AIS_TextLabel) aLabel = new AIS_TextLabel();
        if (isOverlapping) {
            gp_Vec offsetVec(0.0, 0.0, 0.5);
            textPos.Translate(offsetVec);
            TopoDS_Edge arrowLine = BRepBuilderAPI_MakeEdge(p, textPos);
            Handle(AIS_Shape) lineShape = new AIS_Shape(arrowLine);
            lineShape->SetColor(Quantity_NOC_YELLOW);
            m_viewer->myContext->Display(lineShape, 0, 0, Standard_False);
            m_viewer->myPointLabels.append(lineShape);
            aLabel->SetColor(Quantity_NOC_YELLOW);
        } else {
            textPos.SetZ(textPos.Z() + 0.05);
            aLabel->SetColor(Quantity_NOC_GREEN);
        }
        aLabel->SetText(TCollection_ExtendedString(QString("P%1").arg(id).toUtf8().constData()));
        aLabel->SetPosition(textPos);
        aLabel->SetHeight(14);
        aLabel->SetZLayer(Graphic3d_ZLayerId_Topmost);
        m_viewer->myContext->Display(aLabel, 0, 0, Standard_False);
        m_viewer->myPointLabels.append(aLabel);
        lastLabelPos = p;
    };

    // ✅ UPDATED: Accepts optional Radius/Angle info
    auto addPointData = [&](const gp_Pnt& p, const QString& radAngInfo = "-") {
        int id = pointCounter++;
        QString distStr = "-";
        if (id > 1) {
            double d = p.Distance(lastPos);
            distStr = QString::number(d, 'f', 2);
        }
        // ✅ Appending 6th column: Rad/Ang
        pointTableData += QString("P%1|%2|%3|%4|%5|%6\n")
                              .arg(id)
                              .arg(p.X(), 0, 'f', 2)
                              .arg(p.Y(), 0, 'f', 2)
                              .arg(p.Z(), 0, 'f', 2)
                              .arg(distStr)
                              .arg(radAngInfo);
        drawLabel(p, id);
        lastPos = p;
    };

    auto processEdge = [&](const TopoDS_Edge& edge, bool reverse) {
        BRepAdaptor_Curve adaptor(edge);

        // ✅ NEW: Detect Curve Data
        QString edgeInfo = "-";
        if (adaptor.GetType() == GeomAbs_Circle) {
            double r = adaptor.Circle().Radius();
            double angleRad = qAbs(adaptor.LastParameter() - adaptor.FirstParameter());
            double angleDeg = qRadiansToDegrees(angleRad);
            // Format: "R: 25.0 / A: 90.0°"
            edgeInfo = QString("R:%1 / A:%2°")
                           .arg(r, 0, 'f', 1)
                           .arg(angleDeg, 0, 'f', 1);
        }

        if (adaptor.GetType() != GeomAbs_Line) {
            GCPnts_QuasiUniformDeflection discretizer(adaptor, 0.1);
            if (discretizer.IsDone()) {
                int nPoints = discretizer.NbPoints();
                if (reverse) {
                    for (int i = nPoints; i >= 1; --i) addPointData(discretizer.Value(i), edgeInfo);
                } else {
                    for (int i = 1; i <= nPoints; ++i) addPointData(discretizer.Value(i), edgeInfo);
                }
                return;
            }
        }
        TopoDS_Vertex V1, V2;
        TopExp::Vertices(edge, V1, V2);
        gp_Pnt P1 = BRep_Tool::Pnt(V1);
        gp_Pnt P2 = BRep_Tool::Pnt(V2);
        if (reverse) { addPointData(P2, edgeInfo); addPointData(P1, edgeInfo); }
        else { addPointData(P1, edgeInfo); addPointData(P2, edgeInfo); }
    };

    for (const TopoDS_Edge& edge : std::as_const(orderedEdges)) {
        TopoDS_Vertex V1, V2;
        TopExp::Vertices(edge, V1, V2);
        if (V1.IsNull() || V2.IsNull()) continue;
        gp_Pnt P1 = BRep_Tool::Pnt(V1);
        gp_Pnt P2 = BRep_Tool::Pnt(V2);
        if (isFirstEdge) {
            bool flipFirst = false;
            if (orderedEdges.size() > 1) {
                TopoDS_Vertex nV1, nV2;
                TopExp::Vertices(orderedEdges[1], nV1, nV2);
                gp_Pnt nP1 = BRep_Tool::Pnt(nV1);
                gp_Pnt nP2 = BRep_Tool::Pnt(nV2);
                double d1 = std::min(P1.Distance(nP1), P1.Distance(nP2));
                double d2 = std::min(P2.Distance(nP1), P2.Distance(nP2));
                if (d1 < d2) flipFirst = true;
            }
            processEdge(edge, flipFirst);
            isFirstEdge = false;
        } else {
            if (P1.Distance(lastPos) < P2.Distance(lastPos)) processEdge(edge, false);
            else processEdge(edge, true);
        }
    }

    m_viewer->myContext->UpdateCurrentViewer();
    emit m_viewer->measurementsUpdated(props, pointTableData);
    
    // Update internal struct just in case
    m_data.type = props.type;
    m_data.area = props.area;
    m_data.length = props.length;
    m_data.volume = props.volume;
    m_data.diameter = props.diameter;
    m_data.perimeter = 0.0;
}

QString MeasurementManager::getMeasurementString() const
{
    if (m_data.type.isEmpty()) {
        return "No selection";
    }

    QString result = QString("Type: %1\n").arg(m_data.type);

    if (m_data.area > 0.0) {
        result += QString("Area: %1 mm²\n").arg(m_data.area, 0, 'f', 2);
    }
    if (m_data.volume > 0.0) {
        result += QString("Volume: %1 mm³\n").arg(m_data.volume, 0, 'f', 2);
    }
    if (m_data.length > 0.0) {
        result += QString("Length: %1 mm\n").arg(m_data.length, 0, 'f', 2);
    }
    if (m_data.perimeter > 0.0) {
        result += QString("Perimeter: %1 mm\n").arg(m_data.perimeter, 0, 'f', 2);
    }
    if (m_data.diameter > 0.0) {
        result += QString("Diameter: %1 mm\n").arg(m_data.diameter, 0, 'f', 2);
    }

    return result;
}
