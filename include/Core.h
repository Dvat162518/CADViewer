// Core.h
#ifndef _Core_HeaderFile
#define _Core_HeaderFile

#include <QWidget>
#include <QString>
#include <QVector>
#include <TopoDS_Face.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>
#include <TCollection_AsciiString.hxx>
#include <AIS_ViewController.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <AIS_TextLabel.hxx>
#include <AIS_InteractiveObject.hxx>
#include <gp_Pnt.hxx>

class AIS_ViewCube;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;
class QCloseEvent;

class CadModelManager;
class RenderManager;
class MeasurementManager;
class InputManager;
class EventManager;

struct ModelProperties {
    QString filename;
    QString type;
    QString size;
    QString location;
    double originX, originY, originZ;
    double area;
    double volume;
    double radius;
    double diameter;
    double length;
    double angle;
};

struct MeasurementData;

class OcctQWidgetViewer : public QWidget, public AIS_ViewController
{
    Q_OBJECT

    friend class CadModelManager;
    friend class RenderManager;
    friend class MeasurementManager;
    friend class InputManager;
    friend class EventManager;

public:
    OcctQWidgetViewer(QWidget* theParent = nullptr);
    virtual ~OcctQWidgetViewer();

    const Handle(V3d_Viewer)& Viewer() const { return myViewer; }
    const Handle(V3d_View)& View() const { return myView; }
    const Handle(AIS_InteractiveContext)& Context() const { return myContext; }
    const QString& getGlInfo() const { return myGlInfo; }
    const TopoDS_Shape& getLoadedShape() const { return myLoadedShape; }
    int getShapeCount() const { return myDisplayedShapes.size(); }

    bool loadCADModel(const QString& theFilePath);
    bool loadSTEPFile(const QString& theFilePath);
    bool loadIGESFile(const QString& theFilePath);
    bool loadBREPFile(const QString& theFilePath);

    void meshShape(const TopoDS_Shape& theShape, double theDeflection = 0.05);
    double getMeshDeflection() const { return myMeshLinearDeflection; }
    void setMeshDeflection(double theDeflection) { myMeshLinearDeflection = theDeflection; }

    void clearAllShapes();
    void fitViewToModel();
    void calculateMeasurements();
    void displayOriginAxis();
    void displayModelOrigin(const gp_Pnt& thePnt);
    void setOriginTrihedronVisible(bool theVisible);


    QString getFileFormatFromExtension(const QString& theFilePath) const;
    void displayShape(const TopoDS_Shape& theShape);
    void updateView();

    MeasurementData getMeasurements() const;
    QString getMeasurementString() const;

    // --- NEW: Selection Lock Methods ---
    void setSelectionLocked(bool theLocked) { myIsSelectionLocked = theLocked; }
    bool isSelectionLocked() const { return myIsSelectionLocked; }
    // -----------------------------------

    virtual QSize minimumSizeHint() const override { return QSize(200, 200); }
    virtual QSize sizeHint() const override { return QSize(720, 480); }

signals:
    void shapeSelectionChanged(const QString& theInfo);
    void modelLoaded(const QString& theFileName);
    void errorOccurred(const QString& theMessage);
    void measurementsUpdated(const ModelProperties& props, const QString& pointData);

protected:
    void initializeGL();
    virtual void paintEvent(QPaintEvent* theEvent) override;
    virtual void resizeEvent(QResizeEvent* theEvent) override;
    virtual QPaintEngine* paintEngine() const override { return nullptr; }

    virtual bool event(QEvent* theEvent) override;
    virtual void closeEvent(QCloseEvent* theEvent) override;
    virtual void keyPressEvent(QKeyEvent* theEvent) override;
    virtual void mousePressEvent(QMouseEvent* theEvent) override;
    virtual void mouseReleaseEvent(QMouseEvent* theEvent) override;
    virtual void mouseMoveEvent(QMouseEvent* theEvent) override;
    virtual void wheelEvent(QWheelEvent* theEvent) override;

private:
    void dumpGlInfo(bool theIsBasic, bool theToPrint);
    virtual void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                  const Handle(V3d_View)& theView) override;

    void clearLabels();

    CadModelManager* m_cadModel;
    RenderManager* m_render;
    MeasurementManager* m_measurement;
    InputManager* m_input;
    EventManager* m_event;

    Handle(BRepMesh_IncrementalMesh) myMesher;
    TopTools_IndexedMapOfShape myFaceMap;
    TopTools_IndexedMapOfShape myEdgeMap;

    Standard_Integer mySelectedFaceIndex;
    Standard_Integer mySelectedEdgeIndex;
    TopoDS_Face mySelectedFace;
    TopoDS_Edge mySelectedEdge;
    QString myCurrentFilePath;

    QList<Handle(AIS_InteractiveObject)> myPointLabels;

    Handle(V3d_Viewer) myViewer;
    Handle(V3d_View) myView;
    Handle(AIS_InteractiveContext) myContext;
    Handle(AIS_ViewCube) myViewCube;
    Handle(V3d_View) myFocusView;

    Handle(TDocStd_Document) myCADDocument;
    TopoDS_Shape myLoadedShape;
    QVector<Handle(AIS_Shape)> myDisplayedShapes;

    double myMeshLinearDeflection = 0.05;
    QString myGlInfo;
    bool myHasTouchInput = false;
    bool myIsCoreProfile = true;

    // --- NEW: State Variable ---
    bool myIsSelectionLocked = false;
};

#endif // _Core_HeaderFile
