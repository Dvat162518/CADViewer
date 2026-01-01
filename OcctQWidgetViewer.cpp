// Copyright (c) 2025 Open CASCADE
// CAD Viewer - Qt Widget Viewer Implementation

#ifdef _WIN32
#include <windows.h>
#endif

#include "OcctQWidgetViewer.h"
#include "OcctQtTools.h"
#include "OcctGlTools.h"

// ================================================================
// Qt Headers
// ================================================================
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>

// ================================================================
// OCCT Core Headers
// ================================================================
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <AIS_Triangulation.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>

#include <TopExp.hxx>          // âœ… for TopExp::Vertices()
#include <TopoDS_Vertex.hxx>   // âœ… for TopoDS_Vertex
#include <gp_Pnt.hxx>          // âœ… for gp_Pnt (used for XYZ printing)
#include <AIS_TextLabel.hxx>  // âœ… Add this
#include <Prs3d_Arrow.hxx>
#include <Geom_Axis2Placement.hxx>
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>


// ================================================================
// OCCT Graphics Headers
// ================================================================
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_NameOfColor.hxx>

// ================================================================
// OCCT Messaging Headers
// ================================================================
#include <Message.hxx>
#include <Standard_Version.hxx>

#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

// Selection headers
#include <AIS_Selection.hxx>
#include <SelectMgr_SelectionType.hxx>
#include <SelectMgr_FilterType.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Aspect_TypeOfLine.hxx>

// ================================================================
// OCCT File I/O Headers
// ================================================================
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <BRepTools.hxx>

// ================================================================
// OCCT Topology Headers
// ================================================================
#include <TopoDS_Shape.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

// ================================================================
// OCCT Geometry Headers
// ================================================================
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <BRepAdaptor_Curve.hxx>

// ================================================================
// OCCT Mesh Headers
// ================================================================
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <BRep_Tool.hxx>

// ================================================================
// OCCT Error Handling Headers
// ================================================================
#include <OSD.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Failure.hxx>

#include <AIS_Axis.hxx>
#include <Geom_Axis1Placement.hxx>
#include <QFileInfo>
#include <QtMath> // For M_PI


// ================================================================
// Platform-specific X11 Headers
// ================================================================
#if !defined(__APPLE__) && !defined(_WIN32) && defined(__has_include)
#if __has_include(<X11/Xlib.h>)
#include <X11/Xlib.h>
#define USE_XW_DISPLAY
#endif
#endif

#ifndef USE_XW_DISPLAY
typedef Aspect_DisplayConnection Xw_DisplayConnection;
#endif

// ================================================================
// Function : OcctQWidgetViewer
// ================================================================
OcctQWidgetViewer::OcctQWidgetViewer(QWidget* theParent)
    : QWidget(theParent)
{
    try {
        // Create display connection
        Handle(Aspect_DisplayConnection) aDisp = new Aspect_DisplayConnection();

        // Create OpenGL driver
        Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisp, false);

        // Create viewer
        myViewer = new V3d_Viewer(aDriver);
        myViewer->SetDefaultBackgroundColor(Quantity_NOC_BLACK);
        myViewer->SetDefaultLights();
        myViewer->SetLightOn();
        myViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

        // Create AIS context
        myContext = new AIS_InteractiveContext(myViewer);

        // Create view cube
        myViewCube = new AIS_ViewCube();
        myViewCube->SetViewAnimation(myViewAnimation);
        myViewCube->SetFixedAnimationLoop(false);
        myViewCube->SetAutoStartAnimation(true);
        myViewCube->TransformPersistence()->SetOffset2d(Graphic3d_Vec2i(100, 150));

        // Create view
        myView = myViewer->CreateView();
        myView->SetImmediateUpdate(false);

#ifndef __APPLE__
        myView->ChangeRenderingParams().NbMsaaSamples = 4;
#endif

        myView->ChangeRenderingParams().ToShowStats = true;
        myView->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)(
            Graphic3d_RenderingParams::PerfCounters_FrameRate |
            Graphic3d_RenderingParams::PerfCounters_Triangles);

        // Qt widget setup
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_AcceptTouchEvents);
        setMouseTracking(true);
        setBackgroundRole(QPalette::NoRole);
        setFocusPolicy(Qt::StrongFocus);
        setUpdatesEnabled(true);

        // Initialize OpenGL
        initializeGL();

        Message::SendInfo() << "OcctQWidgetViewer initialized successfully";

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "Exception during initialization: " << e.GetMessageString();
    } catch (const std::exception& e) {
        Message::SendFail() << "Exception during initialization: " << e.what();
    }
}

// ================================================================
// Function : ~OcctQWidgetViewer
// ================================================================
OcctQWidgetViewer::~OcctQWidgetViewer()
{
    try {
        // Release OCCT viewer
        if (!myContext.IsNull()) {
            myContext->RemoveAll(false);
            myContext.Nullify();
        }

        if (!myView.IsNull()) {
            myView->Remove();
            myView.Nullify();
        }

        if (!myViewer.IsNull()) {
            myViewer.Nullify();
        }

        myDisplayedShapes.clear();
        myLoadedShape.Nullify();

        Message::SendInfo() << "OcctQWidgetViewer destroyed";

    } catch (const Standard_Failure& e) {
        qWarning() << "Exception during cleanup:" << e.GetMessageString();
    }
}

// ================================================================
// Function : initializeGL
// ================================================================
void OcctQWidgetViewer::initializeGL()
{
    if (myView.IsNull()) {
        return;
    }

    try {
        const QRect aRect = rect();
        const double aDevPixRatio = devicePixelRatioF();

        const Graphic3d_Vec2i aViewSize(Graphic3d_Vec2d(
            Round((aRect.right() - aRect.left()) * aDevPixRatio),
            Round((aRect.bottom() - aRect.top()) * aDevPixRatio)));

        const Aspect_Drawable aNativeWin = (Aspect_Drawable)winId();
        Handle(OcctGlTools::OcctNeutralWindow) aWindow =
            Handle(OcctGlTools::OcctNeutralWindow)::DownCast(myView->Window());

        const bool isFirstInit = aWindow.IsNull();

        if (aWindow.IsNull()) {
            aWindow = new OcctGlTools::OcctNeutralWindow();
            aWindow->SetVirtual(true);
        }

        aWindow->SetNativeHandle(aNativeWin);
        aWindow->SetSize(aViewSize.x(), aViewSize.y());
        aWindow->SetDevicePixelRatio(aDevPixRatio);
        myView->SetWindow(aWindow);

        dumpGlInfo(true, true);

        if (isFirstInit) {
            // Display ViewCube with standard selection mode
            myContext->Display(myViewCube, 0, 0, false);

            // Display dummy box for testing
            TopoDS_Shape aBox = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
            Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
            myContext->Display(aShape, AIS_Shaded, 0, false);
            myDisplayedShapes.append(aShape);

            Message::SendInfo() << "Initial GL setup completed";
        }

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "GL initialization error: " << e.GetMessageString();
    }
}

// ================================================================
// Function : paintEvent
// ================================================================
void OcctQWidgetViewer::paintEvent(QPaintEvent*)
{
    if (myView.IsNull() || myView->Window().IsNull()) {
        return;
    }

    try {
        const double aDevPixelRatioOld = myView->Window()->DevicePixelRatio();
        const double aDevPixelRatioNew = devicePixelRatioF();

        if (myView->Window()->NativeHandle() != (Aspect_Drawable)winId()) {
            Message::SendWarning() << "Native window handle has changed";
            initializeGL();
        } else if (aDevPixelRatioNew != aDevPixelRatioOld) {
            initializeGL();
        } else {
            Graphic3d_Vec2i aViewSizeOld;
            myView->Window()->Size(aViewSizeOld.x(), aViewSizeOld.y());

            const QRect aRect = rect();
            Graphic3d_Vec2i aViewSizeNew(Graphic3d_Vec2d(
                Round((aRect.right() - aRect.left()) * aDevPixelRatioNew),
                Round((aRect.bottom() - aRect.top()) * aDevPixelRatioNew)));

            if (aViewSizeNew != aViewSizeOld) {
                Handle(OcctGlTools::OcctNeutralWindow) aWindow =
                    Handle(OcctGlTools::OcctNeutralWindow)::DownCast(myView->Window());
                aWindow->SetSize(aViewSizeNew.x(), aViewSizeNew.y());
                myView->MustBeResized();
                myView->Invalidate();
                dumpGlInfo(true, false);
            }
        }

        // Render
        Handle(V3d_View) aView = !myFocusView.IsNull() ? myFocusView : myView;
        aView->InvalidateImmediate();
        AIS_ViewController::FlushViewEvents(myContext, aView, true);

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "Paint error: " << e.GetMessageString();
    }
}

// ================================================================
// Function : resizeEvent
// ================================================================
void OcctQWidgetViewer::resizeEvent(QResizeEvent*)
{
    if (!myView.IsNull()) {
        myView->MustBeResized();
    }
}

// ================================================================
// Function : dumpGlInfo
// ================================================================
void OcctQWidgetViewer::dumpGlInfo(bool theIsBasic, bool theToPrint)
{
    if (myView.IsNull()) {
        return;
    }

    try {
        TColStd_IndexedDataMapOfStringString aGlCapsDict;
        myView->DiagnosticInformation(aGlCapsDict,
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

        myGlInfo = QString::fromUtf8(anInfo.ToCString());

    } catch (const Standard_Failure& e) {
        qWarning() << "GL info dump error:" << e.GetMessageString();
    }
}

// ================================================================
// Function : updateView
// ================================================================
void OcctQWidgetViewer::updateView()
{
    QWidget::update();
}

// ================================================================
// Function : handleViewRedraw
// ================================================================
void OcctQWidgetViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                         const Handle(V3d_View)& theView)
{
    AIS_ViewController::handleViewRedraw(theCtx, theView);
    if (myToAskNextFrame) {
        updateView();
    }
}

// ================================================================
// Event Handlers
// ================================================================

bool OcctQWidgetViewer::event(QEvent* theEvent)
{
    if (myView.IsNull()) {
        return QWidget::event(theEvent);
    }

    if (theEvent->type() == QEvent::TouchBegin ||
        theEvent->type() == QEvent::TouchUpdate ||
        theEvent->type() == QEvent::TouchEnd) {
        theEvent->accept();
        myHasTouchInput = true;
        if (OcctQtTools::qtHandleTouchEvent(*this, myView,
                                            static_cast<QTouchEvent*>(theEvent))) {
            updateView();
        }
        return true;
    }

    return QWidget::event(theEvent);
}

void OcctQWidgetViewer::closeEvent(QCloseEvent* theEvent)
{
    theEvent->accept();
}

void OcctQWidgetViewer::keyPressEvent(QKeyEvent* theEvent)
{
    if (myView.IsNull()) {
        return;
    }

    const Aspect_VKey aKey = OcctQtTools::qtKey2VKey(theEvent->key());

    switch (aKey) {
    case Aspect_VKey_Escape:
        QApplication::exit();
        return;

    case Aspect_VKey_F:
        myView->FitAll(0.01, false);
        update();
        theEvent->accept();
        return;

    default:
        break;
    }

    QWidget::keyPressEvent(theEvent);
}

void OcctQWidgetViewer::mouseMoveEvent(QMouseEvent* theEvent)
{
    QWidget::mouseMoveEvent(theEvent);
    if (myView.IsNull()) return;

    if (myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem)
        return;

    theEvent->accept();

    const Standard_Integer aX = theEvent->pos().x();
    const Standard_Integer aY = theEvent->pos().y();

    try {
        // If mouse button is pressed, handle rotation/pan
        if (theEvent->buttons() != Qt::NoButton) {
            if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent)) {
                updateView();
            }
        }
        // Otherwise just highlight (hover)
        else {
            myContext->MoveTo(aX, aY, myView, Standard_True);
        }

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Mouse move error: " << e.GetMessageString();
    }
}


void OcctQWidgetViewer::mousePressEvent(QMouseEvent* theEvent)
{
    QWidget::mousePressEvent(theEvent);
    if (myView.IsNull()) return;

    if (myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem)
        return;

    theEvent->accept();
    const Standard_Integer aX = theEvent->pos().x();
    const Standard_Integer aY = theEvent->pos().y();

    try {
        myContext->MoveTo(aX, aY, myView, Standard_False);

        // 1. Handle Navigation (Right/Middle click)
        if (theEvent->button() == Qt::RightButton || theEvent->button() == Qt::MiddleButton) {
            if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent)) {
                updateView();
            }
            return;
        }

        // 2. Handle Left Click (Selection)
        if (theEvent->button() == Qt::LeftButton) {

            // CASE A: User clicked on a Shape or ViewCube
            if (myContext->HasDetected()) {

                // Check if ViewCube
                Handle(AIS_InteractiveObject) aDetected = myContext->DetectedInteractive();
                if (!aDetected.IsNull() && aDetected == myViewCube) {
                    myContext->SelectDetected(AIS_SelectionScheme_Replace);
                    myContext->UpdateCurrentViewer();
                    updateView();
                    return;
                }

                // Normal Shape Selection
                // âœ… CRITICAL: Using SelectDetected() correctly applies the highlight
                if (theEvent->modifiers() & Qt::ControlModifier) {
                    // CTRL HELD: Add/Remove (XOR)
                    myContext->SelectDetected(AIS_SelectionScheme_XOR);
                } else {
                    // NO CTRL: Replace selection
                    myContext->SelectDetected(AIS_SelectionScheme_Replace);
                }

                // âœ… FORCE UPDATE: Ensure visual feedback happens immediately
                myContext->UpdateCurrentViewer();

                // Calculate measurements AFTER selection is confirmed
                calculateMeasurements();
            }
            // CASE B: User clicked on Empty Space
            else {
                myContext->ClearSelected(Standard_False);
                clearLabels();
                myContext->UpdateCurrentViewer();

                // âœ… FIX: Don't send empty props! Populate file info.
                ModelProperties props;

                // 1. Fill File Metadata (Copy this logic or make a helper function)
                if (!myCurrentFilePath.isEmpty()) {
                    QFileInfo fi(myCurrentFilePath);
                    props.filename = fi.fileName();
                    props.location = fi.absolutePath();

                    double sizeBytes = fi.size();
                    if (sizeBytes > 1024 * 1024) {
                        props.size = QString::number(sizeBytes / (1024.0 * 1024.0), 'f', 2) + " MB";
                    } else {
                        props.size = QString::number(sizeBytes / 1024.0, 'f', 2) + " KB";
                    }
                } else {
                    props.filename = "None";
                    props.size = "-";
                    props.location = "-";
                }

                // 2. Zero out measurements
                props.type = "None"; // Explicitly say "None"
                props.originX = 0; props.originY = 0; props.originZ = 0;
                props.area = 0; props.volume = 0; props.length = 0;
                props.radius = 0; props.diameter = 0; props.angle = 0;

                emit measurementsUpdated(props, ""); // Send with empty point data
                if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent)) {
                    updateView();
                }
            }

            // Final Qt widget repaint
            updateView();
        }

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Click error: " << e.GetMessageString();
    }
}


void OcctQWidgetViewer::mouseReleaseEvent(QMouseEvent* theEvent)
{
    QWidget::mouseReleaseEvent(theEvent);
    if (myView.IsNull()) {
        return;
    }

    theEvent->accept();
    if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent)) {
        updateView();
    }
}

void OcctQWidgetViewer::wheelEvent(QWheelEvent* theEvent)
{
    QWidget::wheelEvent(theEvent);
    if (myView.IsNull()) {
        return;
    }

    theEvent->accept();
    if (OcctQtTools::qtHandleWheelEvent(*this, myView, theEvent)) {
        updateView();
    }
}

// ================================================================
// CAD Import Methods
// ================================================================

QString OcctQWidgetViewer::getFileFormatFromExtension(const QString& theFilePath) const
{
    QString aExtension = theFilePath.right(5).toLower();

    if (aExtension.endsWith(".step") || aExtension.endsWith(".stp")) {
        return "STEP";
    } else if (aExtension.endsWith(".iges") || aExtension.endsWith(".igs")) {
        return "IGES";
    } else if (aExtension.endsWith(".brep")) {
        return "BREP";
    }

    return "UNKNOWN";
}

bool OcctQWidgetViewer::loadCADModel(const QString& theFilePath)
{
    if (theFilePath.isEmpty()) {
        Message::SendWarning() << "File path is empty";
        emit errorOccurred("File path is empty");
        return false;
    }

    if (!QFileInfo::exists(theFilePath)) {
        Message::SendWarning() << "File does not exist:" << theFilePath.toStdString();
        emit errorOccurred("File does not exist");
        return false;
    }

    QString aFormat = getFileFormatFromExtension(theFilePath);

    bool aSuccess = false;
    if (aFormat == "STEP") {
        aSuccess = loadSTEPFile(theFilePath);
    } else if (aFormat == "IGES") {
        aSuccess = loadIGESFile(theFilePath);
    } else if (aFormat == "BREP") {
        aSuccess = loadBREPFile(theFilePath);
    } else {
        Message::SendWarning() << "Unsupported file format: " << aFormat.toStdString();
        emit errorOccurred("Unsupported file format: " + aFormat);
        return false;
    }

    if (aSuccess) {
        myCurrentFilePath = theFilePath; // âœ… Store the path here!
        fitViewToModel();
        updateView();

        QFileInfo aFileInfo(theFilePath);
        emit modelLoaded(aFileInfo.fileName());
    } else {
        emit errorOccurred("Failed to load model");
    }

    return aSuccess;
}

bool OcctQWidgetViewer::loadSTEPFile(const QString& theFilePath)
{
    Message::SendInfo() << "Loading STEP file: " << theFilePath.toStdString();

    OSD::SetSignal(false);

    try {
        OCC_CATCH_SIGNALS

                TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);

        STEPControl_Reader aReader;

        IFSelect_ReturnStatus aStatus = aReader.ReadFile(aPath.ToCString());
        if (aStatus != IFSelect_RetDone) {
            Message::SendFail() << "Failed to read STEP file. Status: " << (int)aStatus;
            return false;
        }

        Message::SendInfo() << "STEP file read. Now transferring...";

        Standard_Boolean aTransferStatus = aReader.TransferRoots();
        if (!aTransferStatus) {
            Message::SendFail() << "Failed to transfer STEP data";
            return false;
        }

        Message::SendInfo() << "Transfer complete. Extracting shapes...";


        TopoDS_Shape aShape = aReader.OneShape();

        if (aShape.IsNull()) {
            Message::SendWarning() << "No valid shape in STEP file";
            return false;
        }

        clearAllShapes();

        displayShape(aShape);
        myLoadedShape = aShape;

        fitViewToModel();

        updateView(); // Qt update

        Message::SendInfo() << "STEP file loaded successfully";
        return true;

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "CRITICAL ERROR: " << e.GetMessageString();
        return false;
    }
}

bool OcctQWidgetViewer::loadIGESFile(const QString& theFilePath)
{
    Message::SendInfo() << "Loading IGES file: " << theFilePath.toStdString();

    OSD::SetSignal(false);

    try {
        OCC_CATCH_SIGNALS

                TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);

        IGESControl_Reader aReader;

        IFSelect_ReturnStatus aStatus = aReader.ReadFile(aPath.ToCString());
        if (aStatus != IFSelect_RetDone) {
            Message::SendFail() << "Failed to read IGES file. Status: " << (int)aStatus;
            return false;
        }

        Message::SendInfo() << "IGES file read. Now transferring...";

        Standard_Boolean aTransferStatus = aReader.TransferRoots();
        if (!aTransferStatus) {
            Message::SendFail() << "Failed to transfer IGES data";
            return false;
        }

        Message::SendInfo() << "Transfer complete. Extracting shapes...";

        clearAllShapes();

        TopoDS_Shape aShape = aReader.OneShape();

        if (aShape.IsNull()) {
            Message::SendWarning() << "No valid shape in IGES file";
            return false;
        }

        displayShape(aShape);
        myLoadedShape = aShape;

        Message::SendInfo() << "IGES file loaded successfully";
        return true;

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "CRITICAL ERROR: " << e.GetMessageString();
        return false;
    }
}

bool OcctQWidgetViewer::loadBREPFile(const QString& theFilePath)
{
    Message::SendInfo() << "Loading BREP file: " << theFilePath.toStdString();

    try {
        TopoDS_Shape aShape;
        TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);

        if (!BRepTools::Read(aShape, aPath.ToCString(), BRep_Builder())) {
            Message::SendFail() << "Failed to read BREP file";
            return false;
        }

        if (aShape.IsNull()) {
            Message::SendFail() << "BREP file contains invalid shape";
            return false;
        }

        clearAllShapes();
        displayShape(aShape);
        myLoadedShape = aShape;

        Message::SendInfo() << "BREP file loaded successfully";
        return true;

    } catch (const Standard_Failure& aException) {
        Message::SendFail() << "Exception during BREP load: " << aException.GetMessageString();
        return false;
    }
}
// 1. ADD ORIGIN VISUALIZATION (Call this in displayShape or loadFile)
void OcctQWidgetViewer::displayOriginAxis()
{
    // Create X, Y, Z axes at 0,0,0
    Handle(Geom_Axis1Placement) xAxis = new Geom_Axis1Placement(gp::OX());
    Handle(Geom_Axis1Placement) yAxis = new Geom_Axis1Placement(gp::OY());
    Handle(Geom_Axis1Placement) zAxis = new Geom_Axis1Placement(gp::OZ());

    Handle(AIS_Axis) aisX = new AIS_Axis(xAxis);
    Handle(AIS_Axis) aisY = new AIS_Axis(yAxis);
    Handle(AIS_Axis) aisZ = new AIS_Axis(zAxis);

    aisX->SetColor(Quantity_NOC_RED);
    aisY->SetColor(Quantity_NOC_GREEN);
    aisZ->SetColor(Quantity_NOC_BLUE);

    myContext->Display(aisX, 0, 0, Standard_False);
    myContext->Display(aisY, 0, 0, Standard_False);
    myContext->Display(aisZ, 0, 0, Standard_False);
}

void OcctQWidgetViewer::displayShape(const TopoDS_Shape& theShape)
{
    if (theShape.IsNull()) return;

    try {
        clearAllShapes();
        myLoadedShape = theShape;

        myMesher = new BRepMesh_IncrementalMesh(theShape, 0.005);
        if (myMesher->IsDone()) {
            Message::SendInfo() << "High-quality mesh generated";
        }

        extractMeshTopology();

        Handle(AIS_Shape) aShapeAIS = new AIS_Shape(theShape);

        aShapeAIS->SetColor(Quantity_NOC_LIGHTGRAY);
        aShapeAIS->SetTransparency(0.25);
        aShapeAIS->SetMaterial(Graphic3d_NOM_PLASTIC);

        Handle(Prs3d_Drawer) aDrawer = aShapeAIS->Attributes();
        aDrawer->SetFaceBoundaryDraw(Standard_True);
        aDrawer->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 1.0));

        // Display with default mode 0
        myContext->Display(aShapeAIS, AIS_Shaded, 0, Standard_False);

        // Deactivate whole-object selection
        myContext->Deactivate(aShapeAIS, 0);

        // Activate ONLY face and edge modes (use numeric constants)
        myContext->Activate(aShapeAIS, 4, Standard_True);
        myContext->Activate(aShapeAIS, 2, Standard_True);

        // Increase selection tolerance
        myContext->SetPixelTolerance(5);

        myDisplayedShapes.append(aShapeAIS);

        myContext->UpdateCurrentViewer();

        Message::SendInfo() << "Shape displayed - Face/Edge selection enabled";

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "Display error: " << e.GetMessageString();
    }
}


void OcctQWidgetViewer::meshShape(const TopoDS_Shape& theShape, double theDeflection)
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

void OcctQWidgetViewer::clearAllShapes()
{
    if (myContext.IsNull()) return;

    try {
        // 1. Deselect everything first
        myContext->ClearSelected(Standard_False);

        // 2. Remove all displayed shapes safely
        for (const Handle(AIS_Shape)& aShape : myDisplayedShapes) {
            if (!aShape.IsNull() && myContext->IsDisplayed(aShape)) {
                myContext->Remove(aShape, Standard_False);
            }
        }

        myDisplayedShapes.clear();
        myLoadedShape.Nullify();
        myCADDocument.Nullify(); // If you use XDE

        // 3. Ensure ViewCube stays
        if (!myViewCube.IsNull()) {
            if (!myContext->IsDisplayed(myViewCube)) {
                myContext->Display(myViewCube, 0, 0, Standard_False);
            }
        }

        // 4. Force update to clear buffers
        myContext->UpdateCurrentViewer();

        // 5. Reset internal maps
        myFaceMap.Clear();
        myEdgeMap.Clear();

        Message::SendInfo() << "Shapes cleared successfully";

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Error in clearAllShapes: " << e.GetMessageString();
    }
}


void OcctQWidgetViewer::fitViewToModel()
{
    if (myView.IsNull()) return;

    if (myLoadedShape.IsNull()) {
        myView->FitAll(0.01, false);
        return;
    }

    // Safety: Check if box is valid before math operations
    Bnd_Box aBox;
    BRepBndLib::Add(myLoadedShape, aBox);

    if (aBox.IsVoid()) {
        // Don't fit if box is empty - prevents division by zero
        return;
    }

    myView->FitAll(0.01, false);
}

void OcctQWidgetViewer::extractMeshTopology()
{
    if (myLoadedShape.IsNull()) {
        Message::SendWarning() << "No shape loaded";
        return;
    }

    try {
        myFaceMap.Clear();
        myEdgeMap.Clear();

        TopExp_Explorer aFaceExplorer(myLoadedShape, TopAbs_FACE);
        while (aFaceExplorer.More()) {
            myFaceMap.Add(aFaceExplorer.Current());
            aFaceExplorer.Next();
        }

        TopExp_Explorer anEdgeExplorer(myLoadedShape, TopAbs_EDGE);
        while (anEdgeExplorer.More()) {
            myEdgeMap.Add(anEdgeExplorer.Current());
            anEdgeExplorer.Next();
        }

        Message::SendInfo() << "Extracted " << myFaceMap.Size() << " faces and "
                            << myEdgeMap.Size() << " edges";

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Error extracting topology: " << e.GetMessageString();
    }
}

void OcctQWidgetViewer::calculateMeasurements()
{
    // 1. Clear old labels
    clearLabels();

    // 2. Initialize Properties Structure
    ModelProperties props;

    // --- ✅ A. POPULATE FILE METADATA ---
    if (!myCurrentFilePath.isEmpty()) {
        QFileInfo fi(myCurrentFilePath);
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

    myContext->InitSelected();
    while (myContext->MoreSelected()) {
        TopoDS_Shape aShape = myContext->SelectedShape();
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
        myContext->NextSelected();
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
            myContext->Display(lineShape, 0, 0, Standard_False);
            myPointLabels.append(lineShape);
            aLabel->SetColor(Quantity_NOC_YELLOW);
        } else {
            textPos.SetZ(textPos.Z() + 0.05);
            aLabel->SetColor(Quantity_NOC_GREEN);
        }
        aLabel->SetText(TCollection_ExtendedString(QString("P%1").arg(id).toUtf8().constData()));
        aLabel->SetPosition(textPos);
        aLabel->SetHeight(14);
        aLabel->SetZLayer(Graphic3d_ZLayerId_Topmost);
        myContext->Display(aLabel, 0, 0, Standard_False);
        myPointLabels.append(aLabel);
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

    myContext->UpdateCurrentViewer();
    emit measurementsUpdated(props, pointTableData);
}

void OcctQWidgetViewer::clearLabels()
{
    if (myContext.IsNull()) return;

    // Iterate over base class handles
    for (const Handle(AIS_InteractiveObject)& anObj : std::as_const(myPointLabels)) {
        myContext->Remove(anObj, Standard_False);
    }
    myPointLabels.clear();
}



QString OcctQWidgetViewer::getMeasurementString() const
{
    if (myMeasurements.type.isEmpty()) {
        return "No selection";
    }

    QString result = QString("Type: %1\n").arg(myMeasurements.type);

    if (myMeasurements.area > 0.0) {
        result += QString("Area: %1 mmÂ²\n").arg(myMeasurements.area, 0, 'f', 2);
    }
    if (myMeasurements.volume > 0.0) {
        result += QString("Volume: %1 mmÂ³\n").arg(myMeasurements.volume, 0, 'f', 2);
    }
    if (myMeasurements.length > 0.0) {
        result += QString("Length: %1 mm\n").arg(myMeasurements.length, 0, 'f', 2);
    }
    if (myMeasurements.perimeter > 0.0) {
        result += QString("Perimeter: %1 mm\n").arg(myMeasurements.perimeter, 0, 'f', 2);
    }
    if (myMeasurements.diameter > 0.0) {
        result += QString("Diameter: %1 mm\n").arg(myMeasurements.diameter, 0, 'f', 2);
    }

    return result;
}
