// Core.cpp
#include "Core.h"
#include "CadModel.h"
#include "Render.h"
#include "Measurement.h"
#include "Input.h"
#include "Event.h"

// OCCT Core Headers
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewCube.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Message.hxx>
#include <Standard_Failure.hxx>
#include <QDebug>

OcctQWidgetViewer::OcctQWidgetViewer(QWidget* theParent)
    : QWidget(theParent)
{
    // Initialize Managers
    m_cadModel = new CadModelManager(this);
    m_render = new RenderManager(this);
    m_measurement = new MeasurementManager(this);
    m_input = new InputManager(this);
    m_event = new EventManager(this);

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

        // REMOVED: initializeGL(); to prevent BadWindow on Linux
        // Initialization happens in paintEvent

        Message::SendInfo() << "OcctQWidgetViewer initialized successfully";

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "Exception during initialization: " << e.GetMessageString();
    } catch (const std::exception& e) {
        Message::SendFail() << "Exception during initialization: " << e.what();
    }
}

OcctQWidgetViewer::~OcctQWidgetViewer()
{
    try {
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

        delete m_cadModel;
        delete m_render;
        delete m_measurement;
        delete m_input;
        delete m_event;

        Message::SendInfo() << "OcctQWidgetViewer destroyed";

    } catch (const Standard_Failure& e) {
        qWarning() << "Exception during cleanup:" << e.GetMessageString();
    }
}

// Delegated Methods
void OcctQWidgetViewer::initializeGL() { m_render->initializeGL(); }
void OcctQWidgetViewer::paintEvent(QPaintEvent* e) { m_render->paintEvent(e); }
void OcctQWidgetViewer::resizeEvent(QResizeEvent* e) { m_render->resizeEvent(e); }
void OcctQWidgetViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& ctx, const Handle(V3d_View)& v) { m_render->handleViewRedraw(ctx, v); }
void OcctQWidgetViewer::dumpGlInfo(bool b, bool p) { m_render->dumpGlInfo(b, p); }
void OcctQWidgetViewer::updateView() { m_render->updateView(); }
void OcctQWidgetViewer::displayShape(const TopoDS_Shape& s) { m_render->displayShape(s); }
void OcctQWidgetViewer::clearAllShapes() { m_render->clearAllShapes(); }
void OcctQWidgetViewer::fitViewToModel() { m_render->fitViewToModel(); }
void OcctQWidgetViewer::displayOriginAxis() { m_render->displayOriginAxis(); }
void OcctQWidgetViewer::meshShape(const TopoDS_Shape& s, double d) { m_render->meshShape(s, d); }
void OcctQWidgetViewer::setOriginTrihedronVisible(bool theVisible){ m_render->setOriginTrihedronVisible(theVisible); }


// --- NEW ---
void OcctQWidgetViewer::displayModelOrigin(const gp_Pnt& p) { m_render->displayModelOrigin(p); }
// -----------

bool OcctQWidgetViewer::loadCADModel(const QString& p) { return m_cadModel->loadCADModel(p); }
bool OcctQWidgetViewer::loadSTEPFile(const QString& p) { return m_cadModel->loadSTEPFile(p); }
bool OcctQWidgetViewer::loadIGESFile(const QString& p) { return m_cadModel->loadIGESFile(p); }
bool OcctQWidgetViewer::loadBREPFile(const QString& p) { return m_cadModel->loadBREPFile(p); }
QString OcctQWidgetViewer::getFileFormatFromExtension(const QString& p) const { return m_cadModel->getFileFormatFromExtension(p); }

void OcctQWidgetViewer::calculateMeasurements() { m_measurement->calculateMeasurements(); }
void OcctQWidgetViewer::clearLabels() { m_measurement->clearLabels(); }
QString OcctQWidgetViewer::getMeasurementString() const { return m_measurement->getMeasurementString(); }
MeasurementData OcctQWidgetViewer::getMeasurements() const { return m_measurement->getMeasurements(); }

void OcctQWidgetViewer::keyPressEvent(QKeyEvent* e) { m_input->keyPressEvent(e); }
void OcctQWidgetViewer::mousePressEvent(QMouseEvent* e) { m_input->mousePressEvent(e); }
void OcctQWidgetViewer::mouseReleaseEvent(QMouseEvent* e) { m_input->mouseReleaseEvent(e); }
void OcctQWidgetViewer::mouseMoveEvent(QMouseEvent* e) { m_input->mouseMoveEvent(e); }
void OcctQWidgetViewer::wheelEvent(QWheelEvent* e) { m_input->wheelEvent(e); }

bool OcctQWidgetViewer::event(QEvent* e) { return m_event->event(e); }
void OcctQWidgetViewer::closeEvent(QCloseEvent* e) { m_event->closeEvent(e); }
