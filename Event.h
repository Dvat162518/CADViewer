// Event.h
#ifndef _Event_HeaderFile
#define _Event_HeaderFile

class OcctQWidgetViewer;
class QEvent;
class QCloseEvent;

class EventManager
{
public:
    explicit EventManager(OcctQWidgetViewer* viewer);

    bool event(QEvent* theEvent);
    void closeEvent(QCloseEvent* theEvent);

private:
    OcctQWidgetViewer* m_viewer;
};

#endif // _Event_HeaderFile
