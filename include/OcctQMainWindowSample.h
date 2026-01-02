// OcctQMainWindowSample.h
#ifndef _OcctQMainWindowSample_HeaderFile
#define _OcctQMainWindowSample_HeaderFile

#include <QMainWindow>
#include <QCloseEvent>
#include "Core.h"

class QLabel;
class QTableWidget;
class QCheckBox;
class QDockWidget; // Forward declaration

class OcctQMainWindowSample : public QMainWindow
{
    Q_OBJECT

public:
    OcctQMainWindowSample();
    virtual ~OcctQMainWindowSample();

protected:
    virtual void closeEvent(QCloseEvent* theEvent) override;

private slots:
    void onMeasurementsUpdated(const ModelProperties& props, const QString& pointData);

private:
    void createMenuBar();
    void createLayoutOverViewer();
    void createDockWidgets();
    void loadCADModel();
    void clearAllShapes();

    // Member widgets
    OcctQWidgetViewer* myViewer = nullptr;
    QLabel* myStatusLabel = nullptr;

    // --- NEW: Separate Dock Widgets ---
    QDockWidget* myDockDescription = nullptr;
    QDockWidget* myDockModelData = nullptr;
    QDockWidget* myDockTools = nullptr;
    // ----------------------------------

    QTableWidget* myPropertiesTable = nullptr;   // Inside Description Dock
    QTableWidget* myPointsTable = nullptr;       // Inside Description Dock
    QTableWidget* mySelectionDataTable = nullptr; // Inside Model Data Dock
    QCheckBox* mySelectionLockBox = nullptr;     // Inside Tools Dock
};

#endif  // _OcctQMainWindowSample_HeaderFile
