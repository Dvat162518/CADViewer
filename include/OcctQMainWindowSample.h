// OcctQMainWindowSample.h

#ifndef _OcctQMainWindowSample_HeaderFile
#define _OcctQMainWindowSample_HeaderFile

#include <QMainWindow>
#include <QTableWidget>
#include <QDockWidget>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include "Core.h"

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

    // --- NEW: Export Slots ---
    void exportToCSV();
    void exportToPDF();
    // -------------------------

private:
    void createMenuBar();
    void createLayoutOverViewer();
    void createDockWidgets();
    void loadCADModel();
    void clearAllShapes();

    // Member widgets
    OcctQWidgetViewer* myViewer = nullptr;
    QLabel* myStatusLabel = nullptr;

    QDockWidget* myDockDescription = nullptr;
    QDockWidget* myDockModelData = nullptr;
    QDockWidget* myDockTools = nullptr;

    QTableWidget* myPropertiesTable = nullptr;
    QTableWidget* myPointsTable = nullptr;
    QTableWidget* mySelectionDataTable = nullptr;
    QCheckBox* mySelectionLockBox = nullptr;
    QCheckBox* myOriginVisBox = nullptr;


    // --- NEW: Data Storage for Export ---
    QString myLastPointData;
    ModelProperties myLastProps;
    // ------------------------------------
};

#endif // _OcctQMainWindowSample_HeaderFile
