// Copyright (c) 2025 Open CASCADE
// CAD Viewer - Main Window Implementation

#include "OcctQMainWindowSample.h"
#include "OcctQWidgetViewer.h"

#include <QHeaderView>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QTabWidget> // âœ… Added
#include <QHeaderView> // âœ… Added
#include <QMetaType>

#include <Message.hxx>

OcctQMainWindowSample::OcctQMainWindowSample()
{
    setWindowTitle("CAD Model Viewer");
    resize(1200, 800);

    myViewer = new OcctQWidgetViewer();
    setCentralWidget(myViewer);

    createMenuBar();
    createLayoutOverViewer();
    createDockWidgets();

    // Signals
    connect(myViewer, &OcctQWidgetViewer::modelLoaded, this, [this](const QString& fileName) {
        statusBar()->showMessage("Loaded: " + fileName, 4000);
        setWindowTitle("CAD Viewer - " + fileName);
    });

    connect(myViewer, &OcctQWidgetViewer::errorOccurred, this, [this](const QString& error) {
        statusBar()->showMessage("Error: " + error, 6000);
        QMessageBox::critical(this, "Error", error);
    });

    // âœ… Measurements Signal
    connect(myViewer, &OcctQWidgetViewer::measurementsUpdated,
            this, &OcctQMainWindowSample::onMeasurementsUpdated);

    statusBar()->showMessage("Ready. File -> Open (Ctrl+O).", 0);
}

OcctQMainWindowSample::~OcctQMainWindowSample() = default;

void OcctQMainWindowSample::closeEvent(QCloseEvent* theEvent)
{
    theEvent->accept();
}

void OcctQMainWindowSample::createMenuBar()
{
    QMenuBar* aMenuBar = new QMenuBar(this);

    // File
    QMenu* aMenuFile = aMenuBar->addMenu("&File");

    QAction* anActionOpen = new QAction("&Open CAD Model", this);
    anActionOpen->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    aMenuFile->addAction(anActionOpen);
    connect(anActionOpen, &QAction::triggered, this, &OcctQMainWindowSample::loadCADModel);

    QAction* anActionClear = new QAction("&Clear Models", this);
    aMenuFile->addAction(anActionClear);
    connect(anActionClear, &QAction::triggered, this, &OcctQMainWindowSample::clearAllShapes);

    aMenuFile->addSeparator();

    QAction* anActionQuit = new QAction("E&xit", this);
    anActionQuit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    aMenuFile->addAction(anActionQuit);
    connect(anActionQuit, &QAction::triggered, this, &QWidget::close);

    // View
    QMenu* aMenuView = aMenuBar->addMenu("&View");

    QAction* anActionFit = new QAction("&Fit All", this);
    anActionFit->setShortcut(Qt::Key_F);
    aMenuView->addAction(anActionFit);
    connect(anActionFit, &QAction::triggered, this, [this]() {
        if (myViewer != nullptr && !myViewer->View().IsNull()) {
            myViewer->View()->FitAll(0.01, false);
            myViewer->update();
        }
    });

    QAction* anActionReset = new QAction("&Reset View", this);
    aMenuView->addAction(anActionReset);
    connect(anActionReset, &QAction::triggered, this, [this]() {
        if (myViewer != nullptr && !myViewer->View().IsNull()) {
            myViewer->View()->Reset(false);
            myViewer->update();
        }
    });

    // Help
    QMenu* aMenuHelp = aMenuBar->addMenu("&Help");

    QAction* anActionAbout = new QAction("&About", this);
    aMenuHelp->addAction(anActionAbout);
    connect(anActionAbout, &QAction::triggered, this, [this]() {
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

    setMenuBar(aMenuBar);
}

void OcctQMainWindowSample::createLayoutOverViewer()
{
    // Overlay controls directly on top of viewer widget (no manual geometry hacks).
    // This prevents the old "QLayout has no addStretch()" problem.
    QVBoxLayout* aRootLayout = new QVBoxLayout(myViewer);
    aRootLayout->setContentsMargins(10, 10, 10, 10);
    aRootLayout->setSpacing(6);
    aRootLayout->setAlignment(Qt::AlignBottom);

    QGroupBox* aControlGroup = new QGroupBox("View Controls", myViewer);
    QHBoxLayout* aControlLayout = new QHBoxLayout(aControlGroup);
    aControlLayout->setContentsMargins(10, 8, 10, 8);

    // About button (quick GL info)
    QPushButton* aAboutBtn = new QPushButton("About", aControlGroup);
    aAboutBtn->setMaximumWidth(90);
    aControlLayout->addWidget(aAboutBtn);

    connect(aAboutBtn, &QPushButton::clicked, this, [this]() {
        const QString info =
            QString("Loaded shapes: %1\n%2")
                .arg(myViewer ? myViewer->getShapeCount() : 0)
                .arg(myViewer ? myViewer->getGlInfo() : QString("OpenGL info: N/A"));
        QMessageBox::information(this, "Viewer Info", info);
    });

    // Background slider
    QLabel* aBgLabel = new QLabel("Background:", aControlGroup);
    aControlLayout->addWidget(aBgLabel);

    QSlider* aBgSlider = new QSlider(Qt::Horizontal, aControlGroup);
    aBgSlider->setRange(0, 255);
    aBgSlider->setValue(0);
    aBgSlider->setMaximumWidth(220);
    aControlLayout->addWidget(aBgSlider);

    connect(aBgSlider, &QSlider::valueChanged, this, [this](int theValue) {
        if (myViewer == nullptr || myViewer->View().IsNull()) {
            return;
        }
        const float aVal = float(theValue) / 255.0f;
        const Quantity_Color aColor(aVal, aVal, aVal, Quantity_TOC_sRGB);

        myViewer->View()->SetBgGradientColors(aColor, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
        myViewer->View()->Invalidate();
        myViewer->update();
    });

    aControlLayout->addStretch(1);

    aRootLayout->addWidget(aControlGroup);
}

void OcctQMainWindowSample::createDockWidgets()
{
    QDockWidget* aDock = new QDockWidget(tr("Description"), this);
    aDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    // Create Tab Widget
    QTabWidget* tabWidget = new QTabWidget(aDock);

    // --- TAB 1: PROPERTIES ---
    myPropertiesTable = new QTableWidget(0, 3);
    QStringList propHeaders; propHeaders << "Property" << "Value" << "Unit";
    myPropertiesTable->setHorizontalHeaderLabels(propHeaders);
    myPropertiesTable->verticalHeader()->setVisible(false);
    myPropertiesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    myPropertiesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    myPropertiesTable->horizontalHeader()->setStretchLastSection(true);
    myPropertiesTable->setColumnWidth(0, 120);
    myPropertiesTable->setColumnWidth(1, 100);
    myPropertiesTable->setColumnWidth(2, 50);

    tabWidget->addTab(myPropertiesTable, "Properties");

    // --- TAB 2: PATH DATA ---
    myPointsTable = new QTableWidget(0, 6);
    QStringList pointHeaders;     pointHeaders << "Point" << "X" << "Y" << "Z" << "Dist" << "Rad / Ang";
    myPointsTable->setHorizontalHeaderLabels(pointHeaders);
    myPointsTable->verticalHeader()->setVisible(false);
    myPointsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    myPointsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    myPointsTable->horizontalHeader()->setStretchLastSection(true);
    myPointsTable->setColumnWidth(0, 50); // P1
    myPointsTable->setColumnWidth(1, 60); // X
    myPointsTable->setColumnWidth(4, 70); // Dist


    tabWidget->addTab(myPointsTable, "Path Data");

    aDock->setWidget(tabWidget);
    addDockWidget(Qt::RightDockWidgetArea, aDock);
}

void OcctQMainWindowSample::loadCADModel()
{
    // OLD LINE (Likely cause of the bug):
    // "CAD Files (*.step *.stp *.iges *.igs *.brep);;All Files (*.*)"

    // ✅ NEW LINE (Fix): Add uppercase variants for Linux compatibility
    const QString aFilePath = QFileDialog::getOpenFileName(
        this, "Open CAD File", QString(),
        "CAD Files (*.step *.stp *.iges *.igs *.brep *.STEP *.STP *.IGES *.IGS *.BREP);;All Files (*.*)");

    if (aFilePath.isEmpty()) return;

    QProgressDialog aProgress("Loading CAD Model...", QString(), 0, 0, this);
    aProgress.setWindowModality(Qt::WindowModal);
    aProgress.show();
    QApplication::processEvents();

    myViewer->loadCADModel(aFilePath);

    aProgress.close();
}


void OcctQMainWindowSample::clearAllShapes()
{
    if (myViewer) {
        myViewer->clearAllShapes();
        myViewer->update();
    }
    setWindowTitle("CAD Viewer");

    // âœ… Update UI Tables (Clear them)
    if (myPropertiesTable) myPropertiesTable->setRowCount(0);
    if (myPointsTable) myPointsTable->setRowCount(0);

    statusBar()->showMessage("Shapes cleared", 3000);
}

void OcctQMainWindowSample::onMeasurementsUpdated(const ModelProperties& props, const QString& pointData)
{
    if (!myPropertiesTable || !myPointsTable) return;

    // --- 1. FILL PROPERTIES TABLE ---
    myPropertiesTable->setRowCount(0);

    auto addPropRow = [&](QString name, QString val, QString unit) {
        int r = myPropertiesTable->rowCount();
        myPropertiesTable->insertRow(r);
        myPropertiesTable->setItem(r, 0, new QTableWidgetItem(name));
        myPropertiesTable->setItem(r, 1, new QTableWidgetItem(val));
        myPropertiesTable->setItem(r, 2, new QTableWidgetItem(unit));
    };

    // âœ… File Metadata (Always shown)
    addPropRow("Filename", props.filename, "");
    addPropRow("Size", props.size, "");
    addPropRow("Location", props.location, "");
    addPropRow("Origin", QString("X%1 Y%2 Z%3").arg(props.originX).arg(props.originY).arg(props.originZ), "mm");

    // Selection-specific properties
    if (!props.type.isEmpty() && props.type != "None") {
        addPropRow("Selection", props.type, "");
        if (props.area > 0) addPropRow("Total Area", QString::number(props.area, 'f', 2), "mmÂ²");
        if (props.length > 0) addPropRow("Total Length", QString::number(props.length, 'f', 2), "mm");
        if (props.diameter > 0) {
            addPropRow("Diameter", QString::number(props.diameter, 'f', 2), "mm");
            addPropRow("Radius", QString::number(props.radius, 'f', 2), "mm");
            addPropRow("Angle", QString::number(props.angle, 'f', 2), "deg");
        }
        if (props.volume > 0) addPropRow("Volume", QString::number(props.volume, 'f', 2), "mmÂ³");
    }

    // --- 2. FILL POINTS TABLE (MEASUREMENTS TAB) ---
    myPointsTable->clearSpans();
    myPointsTable->setRowCount(0);

    if (pointData.isEmpty()) return;
    QStringList rows = pointData.split("\n", Qt::SkipEmptyParts);
    if (rows.isEmpty()) return;

    for (const QString& rowData : std::as_const(rows)) {
        QStringList cols = rowData.split("|");
        if (cols.size() < 6) continue; // ✅ Check for 6 columns now

        int r = myPointsTable->rowCount();
        myPointsTable->insertRow(r);

        // Columns: Point | X | Y | Z | Dist | Rad/Ang
        myPointsTable->setItem(r, 0, new QTableWidgetItem(cols[0]));
        myPointsTable->setItem(r, 1, new QTableWidgetItem(cols[1]));
        myPointsTable->setItem(r, 2, new QTableWidgetItem(cols[2]));
        myPointsTable->setItem(r, 3, new QTableWidgetItem(cols[3]));

        // Distance Column
        QString distStr = cols[4];
        if (distStr != "-") distStr += " mm";
        myPointsTable->setItem(r, 4, new QTableWidgetItem(distStr));

        // ✅ NEW: Radius/Angle Column
        QString radStr = cols[5]; // Raw string like "R:25 / A:90"
        myPointsTable->setItem(r, 5, new QTableWidgetItem(radStr));
    }

    // ✅ Apply Spans (Merge cells for P1-P2, P3-P4, etc.)
    int totalRows = myPointsTable->rowCount();
    for (int i = 1; i < totalRows; i++) {
        // Check Distance Item
        QTableWidgetItem* distItem = myPointsTable->item(i, 4);
        QTableWidgetItem* radItem  = myPointsTable->item(i, 5); // ✅ Get Rad Item

        if (!distItem || !radItem) continue;

        QString valDist = distItem->text();
        QString valRad = radItem->text();

        // Only merge if this row has data (meaning it's the end of a segment)
        if (valDist == "-" || valDist.isEmpty()) continue;

        // --- MERGE DISTANCE ---
        QTableWidgetItem* prevDist = myPointsTable->item(i - 1, 4);
        if (prevDist) {
            prevDist->setText(valDist);
            prevDist->setTextAlignment(Qt::AlignCenter);
            distItem->setText("");
            myPointsTable->setSpan(i - 1, 4, 2, 1);
        }

        // --- ✅ MERGE RADIUS/ANGLE ---
        QTableWidgetItem* prevRad = myPointsTable->item(i - 1, 5);
        if (prevRad) {
            prevRad->setText(valRad); // Move text to top cell
            prevRad->setTextAlignment(Qt::AlignCenter);
            radItem->setText("");     // Clear bottom cell
            myPointsTable->setSpan(i - 1, 5, 2, 1); // Span 2 rows
        }

        i++; // Skip next row to prevent overlapping merges
    }
}
