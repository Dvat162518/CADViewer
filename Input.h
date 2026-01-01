// Input.h
#ifndef _Input_HeaderFile
#define _Input_HeaderFile

class OcctQWidgetViewer;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class InputManager
{
public:
    explicit InputManager(OcctQWidgetViewer* viewer);

    void keyPressEvent(QKeyEvent* theEvent);
    void mousePressEvent(QMouseEvent* theEvent);
    void mouseReleaseEvent(QMouseEvent* theEvent);
    void mouseMoveEvent(QMouseEvent* theEvent);
    void wheelEvent(QWheelEvent* theEvent);

private:
    OcctQWidgetViewer* m_viewer;
};

#endif // _Input_HeaderFile
