// Copyright (c) 2025 Open CASCADE
// CAD Viewer - Qt Widget Viewer Implementation

#ifdef _WIN32
#include <windows.h>
#endif

#include "OcctQWidgetViewer.h"
#include "OcctQtTools.h"
#include "OcctGlTools.h"

#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QFileInfo>

// OCCT headers
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Message.hxx>
#include <Standard_Version.hxx>

// CAD Import
#include <STEPCAFControl_Reader.hxx>
#include <IGESCAFControl_Reader.hxx>
#include <BRepTools.hxx>
#include <TDocStd_Document.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <IFSelect_ReturnStatus.hxx>

// Mesh generation
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>

// Properties
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

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
        Handle(Aspect_DisplayConnection) aDisp = new Xw_DisplayConnection();
        
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

void OcctQWidgetViewer::mousePressEvent(QMouseEvent* theEvent)
{
    QWidget::mousePressEvent(theEvent);
    if (myView.IsNull()) {
        return;
    }
    
    if (myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }
    
    theEvent->accept();
    if (OcctQtTools::qtHandleMouseEvent(*this, myView, theEvent)) {
        updateView();
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

void OcctQWidgetViewer::mouseMoveEvent(QMouseEvent* theEvent)
{
    QWidget::mouseMoveEvent(theEvent);
    if (myView.IsNull()) {
        return;
    }
    
    if (myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem) {
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
    
    try {
        TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);
        
        myCADDocument = new TDocStd_Document("MDTV-XCAF");
        
        STEPCAFControl_Reader aReader;
        IFSelect_ReturnStatus aStatus = aReader.ReadFile(aPath.ToCString());
        
        if (aStatus != IFSelect_RetDone) {
            Message::SendFail() << "Failed to read STEP file. Error code: " << (int)aStatus;
            return false;
        }
        
        Standard_Boolean aTransferSuccess = 
            aReader.Transfer(myCADDocument, IFSelect_TransferCopy);
        
        if (!aTransferSuccess) {
            Message::SendFail() << "Failed to transfer STEP data to document";
            return false;
        }
        
        clearAllShapes();
        
        TDF_Label aRootLabel = myCADDocument->Main();
        Handle(XCAFDoc_ShapeTool) aShapeTool = 
            XCAFDoc_DocumentTool::ShapeTool(aRootLabel);
        
        TDF_LabelSequence aLabels;
        aShapeTool->GetComponents(aRootLabel, aLabels, Standard_False);
        
        Message::SendInfo() << "Found " << aLabels.Length() << " components in STEP file";
        
        if (aLabels.IsEmpty()) {
            aShapeTool->GetShapes(aRootLabel, aLabels);
        }
        
        for (Standard_Integer i = 1; i <= aLabels.Length(); ++i) {
            TopoDS_Shape aShape = aShapeTool->GetShape(aLabels(i));
            
            if (!aShape.IsNull()) {
                displayShape(aShape);
                
                if (myLoadedShape.IsNull()) {
                    myLoadedShape = aShape;
                }
            }
        }
        
        if (myLoadedShape.IsNull()) {
            Message::SendWarning() << "No valid shapes found in STEP file";
            return false;
        }
        
        Message::SendInfo() << "STEP file loaded successfully";
        return true;
        
    } catch (const Standard_Failure& aException) {
        Message::SendFail() << "Exception during STEP load: " << aException.GetMessageString();
        return false;
    }
}

bool OcctQWidgetViewer::loadIGESFile(const QString& theFilePath)
{
    Message::SendInfo() << "Loading IGES file: " << theFilePath.toStdString();
    
    try {
        TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);
        
        myCADDocument = new TDocStd_Document("MDTV-XCAF");
        
        IGESCAFControl_Reader aReader;
        IFSelect_ReturnStatus aStatus = aReader.ReadFile(aPath.ToCString());
        
        if (aStatus != IFSelect_RetDone) {
            Message::SendFail() << "Failed to read IGES file. Error code: " << (int)aStatus;
            return false;
        }
        
        Standard_Boolean aTransferSuccess = 
            aReader.Transfer(myCADDocument, IFSelect_TransferCopy);
        
        if (!aTransferSuccess) {
            Message::SendFail() << "Failed to transfer IGES data to document";
            return false;
        }
        
        clearAllShapes();
        
        TDF_Label aRootLabel = myCADDocument->Main();
        Handle(XCAFDoc_ShapeTool) aShapeTool = 
            XCAFDoc_DocumentTool::ShapeTool(aRootLabel);
        
        TDF_LabelSequence aLabels;
        aShapeTool->GetComponents(aRootLabel, aLabels, Standard_False);
        
        if (aLabels.IsEmpty()) {
            aShapeTool->GetShapes(aRootLabel, aLabels);
        }
        
        Message::SendInfo() << "Found " << aLabels.Length() << " components in IGES file";
        
        for (Standard_Integer i = 1; i <= aLabels.Length(); ++i) {
            TopoDS_Shape aShape = aShapeTool->GetShape(aLabels(i));
            
            if (!aShape.IsNull()) {
                displayShape(aShape);
                
                if (myLoadedShape.IsNull()) {
                    myLoadedShape = aShape;
                }
            }
        }
        
        if (myLoadedShape.IsNull()) {
            Message::SendWarning() << "No valid shapes found in IGES file";
            return false;
        }
        
        Message::SendInfo() << "IGES file loaded successfully";
        return true;
        
    } catch (const Standard_Failure& aException) {
        Message::SendFail() << "Exception during IGES load: " << aException.GetMessageString();
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

void OcctQWidgetViewer::displayShape(const TopoDS_Shape& theShape)
{
    if (theShape.IsNull()) {
        return;
    }
    
    try {
        Handle(AIS_Shape) aShapeAIS = new AIS_Shape(theShape);
        
        aShapeAIS->SetColor(Quantity_NOC_BLUE);
        aShapeAIS->SetMaterial(Graphic3d_NOM_PLASTIC);
        aShapeAIS->SetTransparency(0.0);
        
        myContext->Display(aShapeAIS, AIS_Shaded, 0, false);
        
        myDisplayedShapes.append(aShapeAIS);
        
        meshShape(theShape);
        
        Message::SendInfo() << "Shape displayed successfully";
        
    } catch (const Standard_Failure& aException) {
        Message::SendFail() << "Exception during shape display: " << aException.GetMessageString();
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
    myContext->RemoveAll(false);
    myDisplayedShapes.clear();
    myLoadedShape.Nullify();
    myCADDocument.Nullify();
}

void OcctQWidgetViewer::fitViewToModel()
{
    if (!myLoadedShape.IsNull() && !myView.IsNull()) {
        myView->FitAll(0.01, false);
    }
}
