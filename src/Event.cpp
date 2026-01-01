// Event.cpp
#include "Event.h"
#include "Core.h"
#include "OcctQtTools.h"
#include "Render.h"

#include <QEvent>
#include <QTouchEvent>
#include <QCloseEvent>

EventManager::EventManager(OcctQWidgetViewer* viewer)
    : m_viewer(viewer)
{
}

bool EventManager::event(QEvent* theEvent)
{
    if (m_viewer->myView.IsNull()) {
        return m_viewer->QWidget::event(theEvent);
    }

    if (theEvent->type() == QEvent::TouchBegin ||
        theEvent->type() == QEvent::TouchUpdate ||
        theEvent->type() == QEvent::TouchEnd) {
        theEvent->accept();
        m_viewer->myHasTouchInput = true;
        if (OcctQtTools::qtHandleTouchEvent(*m_viewer, m_viewer->myView,
                                            static_cast<QTouchEvent*>(theEvent))) {
            m_viewer->updateView();
        }
        return true;
    }

    return m_viewer->QWidget::event(theEvent);
}

void EventManager::closeEvent(QCloseEvent* theEvent)
{
    theEvent->accept();
}
