// Measurement.h
#ifndef _Measurement_HeaderFile
#define _Measurement_HeaderFile

#include <QString>
#include <TopoDS_Shape.hxx>

class OcctQWidgetViewer;

struct MeasurementData {
    QString type;           // "FACE", "EDGE", "SOLID"
    double area;            // mm²
    double perimeter;       // mm
    double volume;          // mm³
    double diameter;        // mm (for edges/circles)
    double length;          // mm (for edges/wires)
    QString description;
};

class MeasurementManager
{
public:
    explicit MeasurementManager(OcctQWidgetViewer* viewer);

    void calculateMeasurements();
    QString getMeasurementString() const;
    MeasurementData getMeasurements() const { return m_data; }
    void extractMeshTopology();
    void clearLabels();

private:
    OcctQWidgetViewer* m_viewer;
    MeasurementData m_data;
};

#endif // _Measurement_HeaderFile
