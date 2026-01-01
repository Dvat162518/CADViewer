// Copyright (c) 2025 Open CASCADE
// CAD Viewer Application - Main Window Header
// Optimized: Consistent naming, includes, comments; no functional changes

#ifndef _OcctQMainWindowSample_HeaderFile
#define _OcctQMainWindowSample_HeaderFile

#include <QMainWindow>
#include <QCloseEvent>
#include "OcctQWidgetViewer.h"  // Requires ModelProperties struct definition

class QLabel;
class QTableWidget;

class OcctQMainWindowSample : public QMainWindow
{
    Q_OBJECT

public:
    /// Constructor - Initializes UI and connects viewer signals
    OcctQMainWindowSample();

    /// Destructor - No special cleanup needed
    virtual ~OcctQMainWindowSample();

protected:
    /// Handles window close event
    virtual void closeEvent(QCloseEvent* theEvent) override;

private slots:
    /// Updates properties and points tables from viewer measurements
    void onMeasurementsUpdated(const ModelProperties& props, const QString& pointData);

private:
    /// Creates menu bar with File/View/Help actions
    void createMenuBar();

    /// Adds overlay control layout (background slider, about) on viewer
    void createLayoutOverViewer();

    /// Creates right dock with Properties/Path Data tabs
    void createDockWidgets();

    /// Opens file dialog and loads CAD model into viewer
    void loadCADModel();

    /// Clears all shapes from viewer and resets UI tables
    void clearAllShapes();

    // Member widgets
    OcctQWidgetViewer* myViewer = nullptr;      ///< Central 3D viewer widget
    QLabel* myStatusLabel = nullptr;            ///< Status bar label (unused in current impl)
    QTableWidget* myPropertiesTable = nullptr;  ///< Table for model/selection properties
    QTableWidget* myPointsTable = nullptr;      ///< Table for path/measurement points data
};

#endif  // _OcctQMainWindowSample_HeaderFile
