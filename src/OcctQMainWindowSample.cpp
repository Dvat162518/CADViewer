// Copyright (c) 2025 Open CASCADE
// CAD Viewer - Main Window Implementation
// Optimized: Fixed includes/formatting/spans logic, consistent spacing/comments, no deletions

#include "OcctQMainWindowSample.h"

#include "OcctQWidgetViewer.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QDockWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QProgressDialog>
#include <QKeySequence>
#include <QHeaderView>

OcctQMainWindowSample::OcctQMainWindowSample()
{
    setWindowTitle("CAD Model Viewer");
    resize(1200, 800);

    // Create and set central viewer widget
    myViewer = new OcctQWidgetViewer(this);
    setCentralWidget(myViewer);

    // Build UI components
    createMenuBar();
    createLayoutOverViewer();
    createDockWidgets();

    // Connect viewer signals
    connect(myViewer, &OcctQWidgetViewer::modelLoaded, this,
            [this](const QString& fileName) {
                statusBar()->showMessage("Loaded: " + fileName, 4000);
                setWindowTitle("CAD Viewer - " + fileName);
            });
    connect(myViewer, &OcctQWidgetViewer::errorOccurred, this,
            [this](const QString& error) {
                statusBar()->showMessage("Error: " + error, 6000);
                QMessageBox::critical(this, "Error", error);
            });
    connect(myViewer, &OcctQWidgetViewer::measurementsUpdated,
            this, &OcctQMainWindowSample::onMeasurementsUpdated);

    // Initial status
    statusBar()->showMessage("Ready. File -> Open (Ctrl+O).", 0);
}

OcctQMainWindowSample::~OcctQMainWindowSample() = default;

void OcctQMainWindowSample::closeEvent(QCloseEvent* theEvent)
{
    theEvent->accept();
}

void OcctQMainWindowSample::createMenuBar()
{
    QMenuBar* menuBar = new QMenuBar(this);

    // File menu
    QMenu* menuFile = menuBar->addMenu("&File");
    QAction* actionOpen = new QAction("&Open CAD Model", this);
    actionOpen->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    menuFile->addAction(actionOpen);
    connect(actionOpen, &QAction::triggered, this, &OcctQMainWindowSample::loadCADModel);

    QAction* actionClear = new QAction("&Clear Models", this);
    menuFile->addAction(actionClear);
    connect(actionClear, &QAction::triggered, this, &OcctQMainWindowSample::clearAllShapes);
    menuFile->addSeparator();

    QAction* actionQuit = new QAction("E&xit", this);
    actionQuit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    menuFile->addAction(actionQuit);
    connect(actionQuit, &QAction::triggered, this, &QWidget::close);

    // View menu
    QMenu* menuView = menuBar->addMenu("&View");
    QAction* actionFit = new QAction("&Fit All", this);
    actionFit->setShortcut(Qt::Key_F);
    menuView->addAction(actionFit);
    connect(actionFit, &QAction::triggered, [this]() {
        if (myViewer && !myViewer->View().IsNull()) {
            myViewer->View()->FitAll(0.01, false);
            myViewer->update();
        }
    });

    QAction* actionReset = new QAction("&Reset View", this);
    menuView->addAction(actionReset);
    connect(actionReset, &QAction::triggered, [this]() {
        if (myViewer && !myViewer->View().IsNull()) {
            myViewer->View()->Reset(false);
            myViewer->update();
        }
    });

    // Help menu
    QMenu* menuHelp = menuBar->addMenu("&Help");
    QAction* actionAbout = new QAction("&About", this);
    menuHelp->addAction(actionAbout);
    connect(actionAbout, &QAction::triggered, [this]() {
        const QString glLine = myViewer ? myViewer->getGlInfo().split('\n').value(0) : QString("N/A");
        const QString aboutText =
            "CAD Model Viewer\n\n"
            "Version 1.0\n\n"
            "Built with:\n"
            "- Qt 6\n"
            "- OpenCASCADE 7.7\n\n"
            "Controls:\n"
            "- Rotate: Left mouse drag\n"
            "- Pan: Middle mouse drag\n"
            "- Zoom: Wheel\n"
            "- Fit: F\n\n"
            "OpenGL: " + glLine;
        QMessageBox::information(this, "About", aboutText);
    });

    setMenuBar(menuBar);
}


void OcctQMainWindowSample::createLayoutOverViewer()
{
    // Overlay controls on viewer (bottom-aligned VBox)
    QVBoxLayout* rootLayout = new QVBoxLayout(myViewer);
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(6);
    rootLayout->setAlignment(Qt::AlignBottom);

    QGroupBox* controlGroup = new QGroupBox("View Controls", myViewer);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlGroup);
    controlLayout->setContentsMargins(10, 8, 10, 8);

    // About button - shows shapes/GL info
    QPushButton* aboutBtn = new QPushButton("About", controlGroup);
    aboutBtn->setMaximumWidth(90);
    controlLayout->addWidget(aboutBtn);
    connect(aboutBtn, &QPushButton::clicked, [this]() {
        const QString info = QString("Loaded shapes: %1\n%2")
        .arg(myViewer ? myViewer->getShapeCount() : 0)
            .arg(myViewer ? myViewer->getGlInfo() : QString("OpenGL info: N/A"));
        QMessageBox::information(this, "Viewer Info", info);
    });

    // Background gradient slider
    QLabel* bgLabel = new QLabel("Background:", controlGroup);
    controlLayout->addWidget(bgLabel);
    QSlider* bgSlider = new QSlider(Qt::Horizontal, controlGroup);
    bgSlider->setRange(0, 255);
    bgSlider->setValue(0);
    bgSlider->setMaximumWidth(220);
    controlLayout->addWidget(bgSlider);
    connect(bgSlider, &QSlider::valueChanged, [this](int value) {
        if (!myViewer || myViewer->View().IsNull()) return;
        const float val = float(value) / 255.0f;
        const Quantity_Color color(val, val, val, Quantity_TOC_sRGB);
        myViewer->View()->SetBgGradientColors(color, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
        myViewer->View()->Invalidate();
        myViewer->update();
    });

    controlLayout->addStretch(1);
    rootLayout->addWidget(controlGroup);
}

void OcctQMainWindowSample::createDockWidgets()
{
    QDockWidget* dock = new QDockWidget("Description", this);
    dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    QTabWidget* tabWidget = new QTabWidget(dock);

    // Properties tab: Model/selection metadata
    myPropertiesTable = new QTableWidget(0, 3, dock);
    QStringList propHeaders = {"Property", "Value", "Unit"};
    myPropertiesTable->setHorizontalHeaderLabels(propHeaders);
    myPropertiesTable->verticalHeader()->setVisible(false);
    myPropertiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    myPropertiesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    myPropertiesTable->horizontalHeader()->setStretchLastSection(true);
    myPropertiesTable->setColumnWidth(0, 120);
    myPropertiesTable->setColumnWidth(1, 100);
    myPropertiesTable->setColumnWidth(2, 50);
    tabWidget->addTab(myPropertiesTable, "Properties");

    // Path Data tab: Points/measurements
    myPointsTable = new QTableWidget(0, 6, dock);
    QStringList pointHeaders = {"Point", "X", "Y", "Z", "Dist", "Rad / Ang"};
    myPointsTable->setHorizontalHeaderLabels(pointHeaders);
    myPointsTable->verticalHeader()->setVisible(false);
    myPointsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    myPointsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    myPointsTable->horizontalHeader()->setStretchLastSection(true);
    myPointsTable->setColumnWidth(0, 50);
    myPointsTable->setColumnWidth(1, 60);
    myPointsTable->setColumnWidth(4, 70);
    tabWidget->addTab(myPointsTable, "Path Data");

    dock->setWidget(tabWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void OcctQMainWindowSample::loadCADModel()
{
    // CAD file dialog with uppercase extensions for Linux compatibility
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Open CAD File", QString(),
        "CAD Files (*.step *.stp *.iges *.igs *.brep *.STEP *.STP *.IGES *.IGS *.BREP);;All Files (*.*)");
    if (filePath.isEmpty()) return;

    // Modal progress dialog
    QProgressDialog progress("Loading CAD Model...", QString(), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents();

    myViewer->loadCADModel(filePath);
    progress.close();
}

void OcctQMainWindowSample::clearAllShapes()
{
    if (myViewer) {
        myViewer->clearAllShapes();
        myViewer->update();
    }
    setWindowTitle("CAD Viewer");
    // Clear tables
    if (myPropertiesTable) myPropertiesTable->setRowCount(0);
    if (myPointsTable) myPointsTable->setRowCount(0);
    statusBar()->showMessage("Shapes cleared", 3000);
}

void OcctQMainWindowSample::onMeasurementsUpdated(const ModelProperties& props, const QString& pointData)
{
    if (!myPropertiesTable || !myPointsTable) return;

    // === PROPERTIES TABLE ===
    myPropertiesTable->setRowCount(0);
    auto addPropRow = [&](const QString& name, const QString& val, const QString& unit) {
        int row = myPropertiesTable->rowCount();
        myPropertiesTable->insertRow(row);
        myPropertiesTable->setItem(row, 0, new QTableWidgetItem(name));
        myPropertiesTable->setItem(row, 1, new QTableWidgetItem(val));
        myPropertiesTable->setItem(row, 2, new QTableWidgetItem(unit));
    };

    // File metadata
    addPropRow("Filename", props.filename, "");
    addPropRow("Size", props.size, "");
    addPropRow("Location", props.location, "");
    addPropRow("Origin", QString("X%1 Y%2 Z%3").arg(props.originX).arg(props.originY).arg(props.originZ), "mm");

    // Selection properties
    if (!props.type.isEmpty() && props.type != "None") {
        addPropRow("Selection", props.type, "");
        if (props.area > 0) addPropRow("Total Area", QString::number(props.area, 'f', 2), "mm²");
        if (props.length > 0) addPropRow("Total Length", QString::number(props.length, 'f', 2), "mm");
        if (props.diameter > 0) {
            addPropRow("Diameter", QString::number(props.diameter, 'f', 2), "mm");
            addPropRow("Radius", QString::number(props.radius, 'f', 2), "mm");
            addPropRow("Angle", QString::number(props.angle, 'f', 2), "deg");
        }
        if (props.volume > 0) addPropRow("Volume", QString::number(props.volume, 'f', 2), "mm³");
    }

    // === POINTS TABLE ===
    myPointsTable->clearSpans();
    myPointsTable->setRowCount(0);
    if (pointData.isEmpty()) return;

    QStringList rows = pointData.split("\n", Qt::SkipEmptyParts);
    for (const QString& rowData : rows) {
        QStringList cols = rowData.split("|");
        if (cols.size() < 6) continue;  // Expect 6 cols: Point|X|Y|Z|Dist|Rad/Ang

        int row = myPointsTable->rowCount();
        myPointsTable->insertRow(row);
        myPointsTable->setItem(row, 0, new QTableWidgetItem(cols[0]));  // Point
        myPointsTable->setItem(row, 1, new QTableWidgetItem(cols[1]));  // X
        myPointsTable->setItem(row, 2, new QTableWidgetItem(cols[2]));  // Y
        myPointsTable->setItem(row, 3, new QTableWidgetItem(cols[3]));  // Z

        QString distStr = cols[4];
        if (distStr != "-") distStr += " mm";
        myPointsTable->setItem(row, 4, new QTableWidgetItem(distStr));  // Dist

        myPointsTable->setItem(row, 5, new QTableWidgetItem(cols[5]));  // Rad/Ang
    }

    // Apply spans for paired rows (P1-P2 Dist/Rad merge)
    int totalRows = myPointsTable->rowCount();
    for (int i = 1; i < totalRows; i += 2) {  // Step by 2 for pairs
        if (i >= totalRows) break;

        // Merge Dist col (rows i-1 and i → span on i-1)
        QTableWidgetItem* distItem = myPointsTable->item(i, 4);
        if (distItem && !distItem->text().isEmpty() && distItem->text() != "-") {
            QTableWidgetItem* prevDist = myPointsTable->item(i - 1, 4);
            if (prevDist) {
                prevDist->setText(distItem->text());
                prevDist->setTextAlignment(Qt::AlignCenter);
                distItem->setText("");
                myPointsTable->setSpan(i - 1, 4, 2, 1);
            }
        }

        // Merge Rad/Ang col similarly
        QTableWidgetItem* radItem = myPointsTable->item(i, 5);
        if (radItem && !radItem->text().isEmpty()) {
            QTableWidgetItem* prevRad = myPointsTable->item(i - 1, 5);
            if (prevRad) {
                prevRad->setText(radItem->text());
                prevRad->setTextAlignment(Qt::AlignCenter);
                radItem->setText("");
                myPointsTable->setSpan(i - 1, 5, 2, 1);
            }
        }
    }
}
