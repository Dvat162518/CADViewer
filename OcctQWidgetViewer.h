// Copyright (c) 2025 Open CASCADE
// CAD Viewer - Qt Widget Viewer Header

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
    double angle; // âœ… ADD THIS

};

//! Qt widget holding OCCT 3D View for CAD visualization
//! Inherits from both QWidget and AIS_ViewController for event handling
class OcctQWidgetViewer : public QWidget, public AIS_ViewController
{
    Q_OBJECT

public:
    //! Main constructor
    OcctQWidgetViewer(QWidget* theParent = nullptr);

    //! Destructor
    virtual ~OcctQWidgetViewer();

    // OCCT Handle accessors
    //! Return OCCT Viewer
    const Handle(V3d_Viewer)& Viewer() const { return myViewer; }

    //! Return OCCT View
    const Handle(V3d_View)& View() const { return myView; }

    //! Return AIS Interactive Context
    const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

    //! Return OpenGL information string
    const QString& getGlInfo() const { return myGlInfo; }

    // Geometry query methods
    //! Return currently loaded shape
    const TopoDS_Shape& getLoadedShape() const { return myLoadedShape; }

    //! Return number of displayed shapes
    int getShapeCount() const { return myDisplayedShapes.size(); }

    // CAD Import methods
    //! Load CAD model from file (auto-detects format)
    bool loadCADModel(const QString& theFilePath);

    //! Load STEP file
    bool loadSTEPFile(const QString& theFilePath);

    //! Load IGES file
    bool loadIGESFile(const QString& theFilePath);

    //! Load BREP file
    bool loadBREPFile(const QString& theFilePath);

    // Mesh generation methods
    //! Generate mesh with specified deflection
    void meshShape(const TopoDS_Shape& theShape, double theDeflection = 0.05);

    //! Get current mesh deflection value
    double getMeshDeflection() const { return myMeshLinearDeflection; }

    //! Set mesh deflection value
    void setMeshDeflection(double theDeflection) { myMeshLinearDeflection = theDeflection; }

    // Selection and interaction methods
    //! Clear all displayed shapes
    void clearAllShapes();

    //! Fit view to show all geometry
    void fitViewToModel();

    void calculateMeasurements(); // âœ… Remove arguments

    void displayOriginAxis(); // Add Declaration

    //! Get file format from extension
    QString getFileFormatFromExtension(const QString& theFilePath) const;

    // View control
    //! Minimal widget size hint
    virtual QSize minimumSizeHint() const override { return QSize(200, 200); }

    //! Default widget size hint
    virtual QSize sizeHint() const override { return QSize(720, 480); }

signals:
    //! Emitted when shape selection changes
    void shapeSelectionChanged(const QString& theInfo);

    //! Emitted when model is loaded
    void modelLoaded(const QString& theFileName);

    //! Emitted on error
    void errorOccurred(const QString& theMessage);


protected:
    // Drawing events
    //! Initialize OpenGL context
    void initializeGL();

    //! Paint the 3D view
    virtual void paintEvent(QPaintEvent* theEvent) override;

    //! Handle resize events
    virtual void resizeEvent(QResizeEvent* theEvent) override;

    //! Return null paint engine (we use OpenGL directly)
    virtual QPaintEngine* paintEngine() const override { return nullptr; }

    // Input event handlers
    virtual bool event(QEvent* theEvent) override;
    virtual void closeEvent(QCloseEvent* theEvent) override;
    virtual void keyPressEvent(QKeyEvent* theEvent) override;
    virtual void mousePressEvent(QMouseEvent* theEvent) override;
    virtual void mouseReleaseEvent(QMouseEvent* theEvent) override;
    virtual void mouseMoveEvent(QMouseEvent* theEvent) override;
    virtual void wheelEvent(QWheelEvent* theEvent) override;

private:
    // Mesh and geometry
    Handle(BRepMesh_IncrementalMesh) myMesher;
    TopTools_IndexedMapOfShape myFaceMap;
    TopTools_IndexedMapOfShape myEdgeMap;

    // Selection tracking
    Standard_Integer mySelectedFaceIndex;
    Standard_Integer mySelectedEdgeIndex;
    TopoDS_Face mySelectedFace;
    TopoDS_Edge mySelectedEdge;
    QString myCurrentFilePath; // âœ… Add this to store the path


    // Measurement storage
    struct MeasurementData {
        QString type;           // "FACE", "EDGE", "SOLID"
        double area;            // mmÂ²
        double perimeter;       // mm
        double volume;          // mmÂ³
        double diameter;        // mm (for edges/circles)
        double length;          // mm (for edges/wires)
        QString description;
    };
    MeasurementData myMeasurements;

    // Add to OcctQWidgetViewer.h inside the class or before it



    // Update the signal
signals:
    void measurementsUpdated(const ModelProperties& props, const QString& pointData);


public:
    MeasurementData getMeasurements() const { return myMeasurements; }
    QString getMeasurementString() const;


private:
    void extractMeshTopology();
    void calculateMeasurements(const TopoDS_Shape& theShape);


    QList<Handle(AIS_InteractiveObject)> myPointLabels;
    void clearLabels();


private:
    // Helper methods
    void displayShape(const TopoDS_Shape& theShape);
    void dumpGlInfo(bool theIsBasic, bool theToPrint);
    void updateView();
    virtual void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                  const Handle(V3d_View)& theView) override;

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
