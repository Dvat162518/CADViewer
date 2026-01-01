// Render.cpp
#include "Render.h"
#include "Core.h"
#include "OcctGlTools.h"
#include "Measurement.h" // For extractMeshTopology

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
#include <Geom_Axis1Placement.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>

RenderManager::RenderManager(OcctQWidgetViewer* viewer)
    : m_viewer(viewer)
{
}

void RenderManager::initializeGL()
{
    if (m_viewer->myView.IsNull()) {
        return;
    }

    try {
        const QRect aRect = m_viewer->rect();
        const double aDevPixRatio = m_viewer->devicePixelRatioF();

        const Graphic3d_Vec2i aViewSize(Graphic3d_Vec2d(
            Round((aRect.right() - aRect.left()) * aDevPixRatio),
            Round((aRect.bottom() - aRect.top()) * aDevPixRatio)));

        const Aspect_Drawable aNativeWin = (Aspect_Drawable)m_viewer->winId();
        Handle(OcctGlTools::OcctNeutralWindow) aWindow =
            Handle(OcctGlTools::OcctNeutralWindow)::DownCast(m_viewer->myView->Window());

        const bool isFirstInit = aWindow.IsNull();

        if (aWindow.IsNull()) {
            aWindow = new OcctGlTools::OcctNeutralWindow();
            aWindow->SetVirtual(true);
        }

        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        aWindow->SetDevicePixelRatio(aDevPixRatio);
        m_viewer->myView->SetWindow(aWindow);

        dumpGlInfo(true, true);

        if (isFirstInit) {
            // Display ViewCube with standard selection mode
            m_viewer->myContext->Display(m_viewer->myViewCube, 0, 0, false);

            // Display dummy box for testing
            TopoDS_Shape aBox = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
            Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
            m_viewer->myContext->Display(aShape, AIS_Shaded, 0, false);
            m_viewer->myDisplayedShapes.append(aShape);

            Message::SendInfo() << "Initial GL setup completed";
        }

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "GL initialization error: " << e.GetMessageString();
    }
}

void RenderManager::paintEvent(QPaintEvent*)
{
    if (m_viewer->myView.IsNull() || m_viewer->myView->Window().IsNull()) {
        return;
    }

    try {
        const double aDevPixelRatioOld = m_viewer->myView->Window()->DevicePixelRatio();
        const double aDevPixelRatioNew = m_viewer->devicePixelRatioF();

        if (m_viewer->myView->Window()->NativeHandle() != (Aspect_Drawable)m_viewer->winId()) {
            Message::SendWarning() << "Native window handle has changed";
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
                dumpGlInfo(true, false);
            }
        }

        // Render
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
    if (m_viewer->myView.IsNull()) {
        return;
    }
    try {
        TColStd_IndexedDataMapOfStringString aGlCapsDict;
        m_viewer->myView->DiagnosticInformation(aGlCapsDict,
                                      theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);

        TCollection_AsciiString anInfo;
        for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict);
             aValueIter.More(); aValueIter.Next()) {
            if (!aValueIter.Value().IsEmpty()) {
                if (!anInfo.IsEmpty()) {
                    anInfo += "\n";
                }
                anInfo += aValueIter.Key() + ": " + aValueIter.Value();
            }
        }

        if (theToPrint) {
            Message::SendInfo(anInfo);
        }

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

        // Display with default mode 0
        m_viewer->myContext->Display(aShapeAIS, AIS_Shaded, 0, Standard_False);

        // Deactivate whole-object selection
        m_viewer->myContext->Deactivate(aShapeAIS, 0);

        // Activate ONLY face and edge modes (use numeric constants)
        m_viewer->myContext->Activate(aShapeAIS, 4, Standard_True);
        m_viewer->myContext->Activate(aShapeAIS, 2, Standard_True);

        // Increase selection tolerance
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
        } else {
            Message::SendWarning() << "Mesh generation may have issues";
        }

    } catch (const Standard_Failure& aException) {
        Message::SendWarning() << "Exception during mesh generation: "
                               << aException.GetMessageString();
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

        m_viewer->myDisplayedShapes.clear();
        m_viewer->myLoadedShape.Nullify();
        m_viewer->myCADDocument.Nullify(); // If you use XDE

        // 3. Ensure ViewCube stays
        if (!m_viewer->myViewCube.IsNull()) {
            if (!m_viewer->myContext->IsDisplayed(m_viewer->myViewCube)) {
                m_viewer->myContext->Display(m_viewer->myViewCube, 0, 0, Standard_False);
            }
        }

        // 4. Force update to clear buffers
        m_viewer->myContext->UpdateCurrentViewer();

        // 5. Reset internal maps
        m_viewer->myFaceMap.Clear();
        m_viewer->myEdgeMap.Clear();

        Message::SendInfo() << "Shapes cleared successfully";

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

    // Safety: Check if box is valid before math operations
    Bnd_Box aBox;
    BRepBndLib::Add(m_viewer->myLoadedShape, aBox);

    if (aBox.IsVoid()) {
        // Don't fit if box is empty - prevents division by zero
        return;
    }

    m_viewer->myView->FitAll(0.01, false);
}
