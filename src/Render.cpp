// Render.cpp
#include "Render.h"
#include "Core.h"
#include "OcctGlTools.h"
#include "Measurement.h"

#include <QPaintEvent>
#include <QDebug>
#include <QApplication>

// OCCT Graphics Headers
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <AIS_Axis.hxx>
#include <AIS_Trihedron.hxx>          // --- NEW ---
#include <Geom_Axis2Placement.hxx>    // --- NEW ---
#include <Geom_Axis1Placement.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_DatumAspect.hxx>      // --- NEW ---
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>
#include <Message.hxx>

RenderManager::RenderManager(OcctQWidgetViewer* viewer)
    : m_viewer(viewer)
{
}

void RenderManager::initializeGL()
{
    if (m_viewer->myView.IsNull()) return;

    // Safety: Ensure widget is actually created on the OS side
    if (!m_viewer->isVisible()) return;

    try {
        const QRect aRect = m_viewer->rect();
        const double aDevPixRatio = m_viewer->devicePixelRatioF();

        const Graphic3d_Vec2i aViewSize(Graphic3d_Vec2d(
            Round((aRect.right() - aRect.left()) * aDevPixRatio),
            Round((aRect.bottom() - aRect.top()) * aDevPixRatio)));

        const Aspect_Drawable aNativeWin = (Aspect_Drawable)m_viewer->winId();

        // Critical for Linux: If winId is 0, wait for next event
        if (!aNativeWin) return;

        Handle(OcctGlTools::OcctNeutralWindow) aWindow =
            Handle(OcctGlTools::OcctNeutralWindow)::DownCast(m_viewer->myView->Window());

        if (aWindow.IsNull()) {
            aWindow = new OcctGlTools::OcctNeutralWindow();
        }

        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        aWindow->SetDevicePixelRatio(aDevPixRatio);
        m_viewer->myView->SetWindow(aWindow);

        dumpGlInfo(true, true);

        // Only setup ViewCube if scene is empty (first run)
        if (m_viewer->myContext->NbCurrents() == 0 && m_viewer->myDisplayedShapes.isEmpty()) {
            m_viewer->myContext->Display(m_viewer->myViewCube, 0, 0, false);
            Message::SendInfo() << "Initial GL setup completed";
        }

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "GL initialization error: " << e.GetMessageString();
    }
}

void RenderManager::paintEvent(QPaintEvent*)
{
    if (m_viewer->myView.IsNull()) return;

    // Lazy Initialization
    if (m_viewer->myView->Window().IsNull()) {
        initializeGL();
        if (m_viewer->myView->Window().IsNull()) return;
    }

    try {
        const double aDevPixelRatioOld = m_viewer->myView->Window()->DevicePixelRatio();
        const double aDevPixelRatioNew = m_viewer->devicePixelRatioF();

        if (m_viewer->myView->Window()->NativeHandle() != (Aspect_Drawable)m_viewer->winId()) {
            initializeGL();
        } else if (aDevPixelRatioNew != aDevPixelRatioOld) {
            initializeGL();
        } else {
            Graphic3d_Vec2i aViewSizeOld;
            m_viewer->myView->Window()->Size(aViewSizeOld.x(), aViewSizeOld.y());

            const QRect aRect = m_viewer->rect();
            Graphic3d_Vec2i aViewSizeNew(Graphic3d_Vec2d(
                Round((aRect.right() - aRect.left()) * aDevPixelRatioNew),
                Round((aRect.bottom() - aRect.top()) * aDevPixelRatioNew)));

            if (aViewSizeNew != aViewSizeOld) {
                Handle(OcctGlTools::OcctNeutralWindow) aWindow =
                    Handle(OcctGlTools::OcctNeutralWindow)::DownCast(m_viewer->myView->Window());
                aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
                m_viewer->myView->MustBeResized();
                m_viewer->myView->Invalidate();
            }
        }

        Handle(V3d_View) aView = !m_viewer->myFocusView.IsNull() ? m_viewer->myFocusView : m_viewer->myView;
        aView->InvalidateImmediate();
        m_viewer->AIS_ViewController::FlushViewEvents(m_viewer->myContext, aView, true);

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "Paint error: " << e.GetMessageString();
    }
}

void RenderManager::resizeEvent(QResizeEvent*)
{
    if (!m_viewer->myView.IsNull()) {
        m_viewer->myView->MustBeResized();
    }
}

void RenderManager::dumpGlInfo(bool theIsBasic, bool theToPrint)
{
    if (m_viewer->myView.IsNull()) return;
    try {
        TColStd_IndexedDataMapOfStringString aGlCapsDict;
        m_viewer->myView->DiagnosticInformation(aGlCapsDict,
                                                theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
        TCollection_AsciiString anInfo;
        for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict);
             aValueIter.More(); aValueIter.Next()) {
            if (!aValueIter.Value().IsEmpty()) {
                if (!anInfo.IsEmpty()) anInfo += "\n";
                anInfo += aValueIter.Key() + ": " + aValueIter.Value();
            }
        }
        if (theToPrint) Message::SendInfo(anInfo);
        m_viewer->myGlInfo = QString::fromUtf8(anInfo.ToCString());
    } catch (const Standard_Failure& e) {
        qWarning() << "GL info dump error:" << e.GetMessageString();
    }
}

void RenderManager::updateView()
{
    m_viewer->QWidget::update();
}

void RenderManager::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                     const Handle(V3d_View)& theView)
{
    m_viewer->AIS_ViewController::handleViewRedraw(theCtx, theView);
    if (m_viewer->myToAskNextFrame) {
        updateView();
    }
}

void RenderManager::displayOriginAxis()
{
    Handle(Geom_Axis1Placement) xAxis = new Geom_Axis1Placement(gp::OX());
    Handle(Geom_Axis1Placement) yAxis = new Geom_Axis1Placement(gp::OY());
    Handle(Geom_Axis1Placement) zAxis = new Geom_Axis1Placement(gp::OZ());

    Handle(AIS_Axis) aisX = new AIS_Axis(xAxis);
    Handle(AIS_Axis) aisY = new AIS_Axis(yAxis);
    Handle(AIS_Axis) aisZ = new AIS_Axis(zAxis);

    aisX->SetColor(Quantity_NOC_RED);
    aisY->SetColor(Quantity_NOC_GREEN);
    aisZ->SetColor(Quantity_NOC_BLUE);

    m_viewer->myContext->Display(aisX, 0, 0, Standard_False);
    m_viewer->myContext->Display(aisY, 0, 0, Standard_False);
    m_viewer->myContext->Display(aisZ, 0, 0, Standard_False);
}

// --- NEW: Display Trihedron at Model Origin ---
void RenderManager::displayModelOrigin(const gp_Pnt& thePnt)
{
    if (m_viewer->myContext.IsNull()) return;

    // 1. Remove old visual if exists
    if (!myModelOriginVis.IsNull()) {
        m_viewer->myContext->Remove(myModelOriginVis, Standard_False);
        myModelOriginVis.Nullify();
    }

    // 2. Create Coordinate System at Point
    // Z-axis defaults to (0,0,1), X-axis to (1,0,0) shifted to 'thePnt'
    Handle(Geom_Axis2Placement) aPlace = new Geom_Axis2Placement(thePnt, gp::DZ(), gp::DX());

    // 3. Create Trihedron
    Handle(AIS_Trihedron) aTrihedron = new AIS_Trihedron(aPlace);

    // 4. Style It
    aTrihedron->SetDatumDisplayMode(Prs3d_DM_WireFrame);
    aTrihedron->SetDrawArrows(true);

    // Adjust visual attributes (Thicker lines, larger arrows)
    const Handle(Prs3d_Drawer)& aDrawer = aTrihedron->Attributes();
    aDrawer->DatumAspect()->SetAttribute(Prs3d_DA_XAxisLength, 20.0);
    aDrawer->DatumAspect()->SetAttribute(Prs3d_DA_YAxisLength, 20.0);
    aDrawer->DatumAspect()->SetAttribute(Prs3d_DA_ZAxisLength, 20.0);

    // X=Red, Y=Green, Z=Blue is standard
    aDrawer->DatumAspect()->LineAspect(Prs3d_DP_XAxis)->SetColor(Quantity_NOC_RED);
    aDrawer->DatumAspect()->LineAspect(Prs3d_DP_YAxis)->SetColor(Quantity_NOC_GREEN);
    aDrawer->DatumAspect()->LineAspect(Prs3d_DP_ZAxis)->SetColor(Quantity_NOC_BLUE);

    // 5. Display
    m_viewer->myContext->Display(aTrihedron, 0, 0, Standard_False);
    myModelOriginVis = aTrihedron;
}
// ----------------------------------------------

void RenderManager::displayShape(const TopoDS_Shape& theShape)
{
    if (theShape.IsNull()) return;

    try {
        clearAllShapes();
        m_viewer->myLoadedShape = theShape;

        m_viewer->myMesher = new BRepMesh_IncrementalMesh(theShape, 0.005);
        if (m_viewer->myMesher->IsDone()) {
            Message::SendInfo() << "High-quality mesh generated";
        }

        m_viewer->m_measurement->extractMeshTopology();

        Handle(AIS_Shape) aShapeAIS = new AIS_Shape(theShape);

        aShapeAIS->SetColor(Quantity_NOC_LIGHTGRAY);
        aShapeAIS->SetTransparency(0.25);
        aShapeAIS->SetMaterial(Graphic3d_NOM_PLASTIC);

        Handle(Prs3d_Drawer) aDrawer = aShapeAIS->Attributes();
        aDrawer->SetFaceBoundaryDraw(Standard_True);
        aDrawer->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.0));

        m_viewer->myContext->Display(aShapeAIS, AIS_Shaded, 0, Standard_False);
        m_viewer->myContext->Deactivate(aShapeAIS, 0);
        m_viewer->myContext->Activate(aShapeAIS, 4, Standard_True);
        m_viewer->myContext->Activate(aShapeAIS, 2, Standard_True);
        m_viewer->myContext->SetPixelTolerance(5);
        m_viewer->myDisplayedShapes.append(aShapeAIS);
        m_viewer->myContext->UpdateCurrentViewer();

        Message::SendInfo() << "Shape displayed - Face/Edge selection enabled";

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "Display error: " << e.GetMessageString();
    }
}

void RenderManager::meshShape(const TopoDS_Shape& theShape, double theDeflection)
{
    try {
        BRepMesh_IncrementalMesh aMesher(theShape, theDeflection);
        if (aMesher.IsDone()) {
            Message::SendInfo() << "Mesh generated with deflection: " << theDeflection;
        }
    } catch (const Standard_Failure& aException) {
        Message::SendWarning() << "Mesh Error: " << aException.GetMessageString();
    }
}

void RenderManager::clearAllShapes()
{
    if (m_viewer->myContext.IsNull()) return;

    try {
        // 1. Deselect everything first
        m_viewer->myContext->ClearSelected(Standard_False);

        // 2. Remove all displayed shapes safely
        for (const Handle(AIS_Shape)& aShape : m_viewer->myDisplayedShapes) {
            if (!aShape.IsNull() && m_viewer->myContext->IsDisplayed(aShape)) {
                m_viewer->myContext->Remove(aShape, Standard_False);
            }
        }

        // 3. Remove the Origin Visual
        if (!myModelOriginVis.IsNull()) {
            m_viewer->myContext->Remove(myModelOriginVis, Standard_False);
            myModelOriginVis.Nullify();
        }

        // 4. Reset Data Structures
        m_viewer->myDisplayedShapes.clear();
        m_viewer->myLoadedShape.Nullify();
        m_viewer->myCADDocument.Nullify();

        // --- FIX: CLEAR FILE PATH ---
        m_viewer->myCurrentFilePath.clear();
        // ----------------------------

        // 5. Ensure ViewCube stays
        if (!m_viewer->myViewCube.IsNull()) {
            if (!m_viewer->myContext->IsDisplayed(m_viewer->myViewCube)) {
                m_viewer->myContext->Display(m_viewer->myViewCube, 0, 0, Standard_False);
            }
        }

        // 6. Force update and reset maps
        m_viewer->myContext->UpdateCurrentViewer();
        m_viewer->myFaceMap.Clear();
        m_viewer->myEdgeMap.Clear();

        // --- FIX: RESET MEASUREMENTS TO EMPTY ---
        m_viewer->calculateMeasurements();
        // ----------------------------------------

        Message::SendInfo() << "Shapes and data cleared successfully";

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Error in clearAllShapes: " << e.GetMessageString();
    }
}

void RenderManager::fitViewToModel()
{
    if (m_viewer->myView.IsNull()) return;

    if (m_viewer->myLoadedShape.IsNull()) {
        m_viewer->myView->FitAll(0.01, false);
        return;
    }

    Bnd_Box aBox;
    BRepBndLib::Add(m_viewer->myLoadedShape, aBox);

    if (aBox.IsVoid()) return;

    m_viewer->myView->FitAll(0.01, false);
}
