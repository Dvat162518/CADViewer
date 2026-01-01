// Core.h (OcctQWidgetViewer.h)
#ifndef _OcctQWidgetViewer_HeaderFile
#define _OcctQWidgetViewer_HeaderFile

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

class AIS_ViewCube;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;
class QCloseEvent;

// Forward declarations of managers
class CadModelManager;
class RenderManager;
class MeasurementManager;
class InputManager;
class EventManager;

struct ModelProperties {
    QString filename;
    QString type;
    QString size; // e.g., "1.2 MB"
    QString location;
    double originX, originY, originZ;
    double area;
    double volume;
    double radius;
    double diameter;
    double length;
    double angle; 
};

struct MeasurementData; // Forward declaration from Measurement.h

//! Qt widget holding OCCT 3D View for CAD visualization
class OcctQWidgetViewer : public QWidget, public AIS_ViewController
{
    Q_OBJECT

    // Managers need access to private members to perform their tasks
    friend class CadModelManager;
    friend class RenderManager;
    friend class MeasurementManager;
    friend class InputManager;
    friend class EventManager;

public:
    //! Main constructor
    OcctQWidgetViewer(QWidget* theParent = nullptr);

    //! Destructor
    virtual ~OcctQWidgetViewer();

    // OCCT Handle accessors
    const Handle(V3d_Viewer)& Viewer() const { return myViewer; }
    const Handle(V3d_View)& View() const { return myView; }
    const Handle(AIS_InteractiveContext)& Context() const { return myContext; }
    const QString& getGlInfo() const { return myGlInfo; }
    const TopoDS_Shape& getLoadedShape() const { return myLoadedShape; }
    int getShapeCount() const { return myDisplayedShapes.size(); }

    // Delegated Methods
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
    QString getFileFormatFromExtension(const QString& theFilePath) const;
    void displayShape(const TopoDS_Shape& theShape);
    void updateView();

    // Measurement accessors
    // Note: requires including Measurement.h in cpp or forward decl details
    // For simplicity, returns empty if manager not ready, or implement in CPP
    MeasurementData getMeasurements() const; 
    QString getMeasurementString() const;

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

    // Managers
    CadModelManager* m_cadModel;
    RenderManager* m_render;
    MeasurementManager* m_measurement;
    InputManager* m_input;
    EventManager* m_event;

    // Mesh and geometry
    Handle(BRepMesh_IncrementalMesh) myMesher;
    TopTools_IndexedMapOfShape myFaceMap;
    TopTools_IndexedMapOfShape myEdgeMap;

    // Selection tracking
    Standard_Integer mySelectedFaceIndex;
    Standard_Integer mySelectedEdgeIndex;
    TopoDS_Face mySelectedFace;
    TopoDS_Edge mySelectedEdge;
    QString myCurrentFilePath; 

    QList<Handle(AIS_InteractiveObject)> myPointLabels;

    // OCCT members
    Handle(V3d_Viewer) myViewer;                   //!< 3D Viewer
    Handle(V3d_View) myView;                       //!< Viewport
    Handle(AIS_InteractiveContext) myContext;      //!< Selection/Display context
    Handle(AIS_ViewCube) myViewCube;               //!< Navigation cube
    Handle(V3d_View) myFocusView;                  //!< Focused subview

    // Loaded model
    Handle(TDocStd_Document) myCADDocument;        //!< XCAF document for CAD data
    TopoDS_Shape myLoadedShape;                    //!< Main loaded shape
    QVector<Handle(AIS_Shape)> myDisplayedShapes; //!< All displayed AIS shapes

    // Mesh parameters
    double myMeshLinearDeflection = 0.05;          //!< Mesh quality control

    // GL info
    QString myGlInfo;

    // Input state
    bool myHasTouchInput = false;
    bool myIsCoreProfile = true;
};

#endif // _OcctQWidgetViewer_HeaderFile
