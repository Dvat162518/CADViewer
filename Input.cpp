// Input.cpp
#include "Input.h"
#include "Core.h"
#include "OcctQtTools.h"
#include "Render.h"
#include "Measurement.h"

#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QFileInfo>
#include <Message.hxx>

#include <Aspect_VKey.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewCube.hxx>

InputManager::InputManager(OcctQWidgetViewer* viewer)
    : m_viewer(viewer)
{
}

void InputManager::keyPressEvent(QKeyEvent* theEvent)
{
    if (m_viewer->myView.IsNull()) {
        return;
    }

    const Aspect_VKey aKey = OcctQtTools::qtKey2VKey(theEvent->key());

    switch (aKey) {
    case Aspect_VKey_Escape:
        QApplication::exit();
        return;

    case Aspect_VKey_F:
        m_viewer->myView->FitAll(0.01, false);
        m_viewer->update();
        theEvent->accept();
        return;

    default:
        break;
    }

    m_viewer->QWidget::keyPressEvent(theEvent);
}

void InputManager::mouseMoveEvent(QMouseEvent* theEvent)
{
    m_viewer->QWidget::mouseMoveEvent(theEvent);
    if (m_viewer->myView.IsNull()) return;

    if (m_viewer->myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem)
        return;

    theEvent->accept();

    const Standard_Integer aX = theEvent->pos().x();
    const Standard_Integer aY = theEvent->pos().y();

    try {
        // If mouse button is pressed, handle rotation/pan
        if (theEvent->buttons() != Qt::NoButton) {
            if (OcctQtTools::qtHandleMouseEvent(*m_viewer, m_viewer->myView, theEvent)) {
                m_viewer->updateView();
            }
        }
        // Otherwise just highlight (hover)
        else {
            m_viewer->myContext->MoveTo(aX, aY, m_viewer->myView, Standard_True);
        }

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Mouse move error: " << e.GetMessageString();
    }
}


void InputManager::mousePressEvent(QMouseEvent* theEvent)
{
    m_viewer->QWidget::mousePressEvent(theEvent);
    if (m_viewer->myView.IsNull()) return;

    if (m_viewer->myHasTouchInput && theEvent->source() == Qt::MouseEventSynthesizedBySystem)
        return;

    theEvent->accept();
    const Standard_Integer aX = theEvent->pos().x();
    const Standard_Integer aY = theEvent->pos().y();

    try {
        m_viewer->myContext->MoveTo(aX, aY, m_viewer->myView, Standard_False);

        // 1. Handle Navigation (Right/Middle click)
        if (theEvent->button() == Qt::RightButton || theEvent->button() == Qt::MiddleButton) {
            if (OcctQtTools::qtHandleMouseEvent(*m_viewer, m_viewer->myView, theEvent)) {
                m_viewer->updateView();
            }
            return;
        }

        // 2. Handle Left Click (Selection)
        if (theEvent->button() == Qt::LeftButton) {

            // CASE A: User clicked on a Shape or ViewCube
            if (m_viewer->myContext->HasDetected()) {

                // Check if ViewCube
                Handle(AIS_InteractiveObject) aDetected = m_viewer->myContext->DetectedInteractive();
                if (!aDetected.IsNull() && aDetected == m_viewer->myViewCube) {
                    m_viewer->myContext->SelectDetected(AIS_SelectionScheme_Replace);
                    m_viewer->myContext->UpdateCurrentViewer();
                    m_viewer->updateView();
                    return;
                }

                // Normal Shape Selection
                // âœ… CRITICAL: Using SelectDetected() correctly applies the highlight
                if (theEvent->modifiers() & Qt::ControlModifier) {
                    // CTRL HELD: Add/Remove (XOR)
                    m_viewer->myContext->SelectDetected(AIS_SelectionScheme_XOR);
                } else {
                    // NO CTRL: Replace selection
                    m_viewer->myContext->SelectDetected(AIS_SelectionScheme_Replace);
                }

                // âœ… FORCE UPDATE: Ensure visual feedback happens immediately
                m_viewer->myContext->UpdateCurrentViewer();

                // Calculate measurements AFTER selection is confirmed
                m_viewer->calculateMeasurements();
            }
            // CASE B: User clicked on Empty Space
            else {
                m_viewer->myContext->ClearSelected(Standard_False);
                m_viewer->clearLabels();
                m_viewer->myContext->UpdateCurrentViewer();

                // âœ… FIX: Don't send empty props! Populate file info.
                ModelProperties props;

                // 1. Fill File Metadata (Copy this logic or make a helper function)
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
                    props.filename = "None";
                    props.size = "-";
                    props.location = "-";
                }

                // 2. Zero out measurements
                props.type = "None"; // Explicitly say "None"
                props.originX = 0; props.originY = 0; props.originZ = 0;
                props.area = 0; props.volume = 0; props.length = 0;
                props.radius = 0; props.diameter = 0; props.angle = 0;

                emit m_viewer->measurementsUpdated(props, ""); // Send with empty point data
                if (OcctQtTools::qtHandleMouseEvent(*m_viewer, m_viewer->myView, theEvent)) {
                    m_viewer->updateView();
                }
            }

            // Final Qt widget repaint
            m_viewer->updateView();
        }

    } catch (const Standard_Failure& e) {
        Message::SendWarning() << "Click error: " << e.GetMessageString();
    }
}


void InputManager::mouseReleaseEvent(QMouseEvent* theEvent)
{
    m_viewer->QWidget::mouseReleaseEvent(theEvent);
    if (m_viewer->myView.IsNull()) {
        return;
    }

    theEvent->accept();
    if (OcctQtTools::qtHandleMouseEvent(*m_viewer, m_viewer->myView, theEvent)) {
        m_viewer->updateView();
    }
}

void InputManager::wheelEvent(QWheelEvent* theEvent)
{
    m_viewer->QWidget::wheelEvent(theEvent);
    if (m_viewer->myView.IsNull()) {
        return;
    }

    theEvent->accept();
    if (OcctQtTools::qtHandleWheelEvent(*m_viewer, m_viewer->myView, theEvent)) {
        m_viewer->updateView();
    }
}
