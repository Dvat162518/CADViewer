// Render.h
#ifndef _Render_HeaderFile
#define _Render_HeaderFile

#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveObject.hxx> // Added for Handle(AIS_InteractiveObject)

class OcctQWidgetViewer;
class QPaintEvent;
class QResizeEvent;
class gp_Pnt;

class RenderManager
{
public:
    explicit RenderManager(OcctQWidgetViewer* viewer);

    void initializeGL();
    void paintEvent(QPaintEvent* theEvent);
    void resizeEvent(QResizeEvent* theEvent);
    void displayShape(const TopoDS_Shape& theShape);
    void clearAllShapes();
    void fitViewToModel();
    void displayOriginAxis(); // Global 0,0,0

    // --- NEW: Display specific model origin ---
    void displayModelOrigin(const gp_Pnt& thePnt);
    // ------------------------------------------

    void meshShape(const TopoDS_Shape& theShape, double theDeflection);
    void dumpGlInfo(bool theIsBasic, bool theToPrint);
    void updateView();
    void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                          const Handle(V3d_View)& theView);

private:
    OcctQWidgetViewer* m_viewer;

    // Track the specific object to allow updating it
    Handle(AIS_InteractiveObject) myModelOriginVis;
};

#endif // _Render_HeaderFile
