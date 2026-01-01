// Copyright (c) 2025 Open CASCADE
// CAD Viewer Application - Main Window Header

#ifndef _OcctQMainWindowSample_HeaderFile
#define _OcctQMainWindowSample_HeaderFile

#include <QMainWindow>
#include <QString>
#include "OcctQWidgetViewer.h" // Need struct definition

class QLabel;
class QTableWidget;

class OcctQMainWindowSample : public QMainWindow
{
    Q_OBJECT

public:
    OcctQMainWindowSample();
    virtual ~OcctQMainWindowSample();

protected:
    virtual void closeEvent(QCloseEvent* theEvent) override;

private slots:
    // âœ… NEW SLOT SIGNATURE
    void onMeasurementsUpdated(const ModelProperties& props, const QString& pointData);

private:
    void createMenuBar();
    void createLayoutOverViewer();
    void createDockWidgets();
    void loadCADModel();
    void clearAllShapes();

private:
    OcctQWidgetViewer* myViewer = nullptr;
    QLabel* myStatusLabel = nullptr;

    // âœ… SEPARATE TABLES FOR BETTER UI
    QTableWidget* myPropertiesTable = nullptr;
    QTableWidget* myPointsTable = nullptr;
};

#endif
