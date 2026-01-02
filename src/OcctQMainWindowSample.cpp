// OcctQMainWindowSample.cpp
#include "OcctQMainWindowSample.h"
#include "Core.h"

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
#include <QCheckBox>

OcctQMainWindowSample::OcctQMainWindowSample()
{
    setWindowTitle("CAD Model Viewer");
    resize(1200, 800);

    // Create and set central viewer widget
    myViewer = new OcctQWidgetViewer(this);
    setCentralWidget(myViewer);

    // Build UI components
    createDockWidgets(); // Create docks first so menu can reference them
    createMenuBar();
    createLayoutOverViewer();

    // Connect viewer signals
    connect(myViewer, &OcctQWidgetViewer::modelLoaded, this,
            [this](const QString& fileName) {
                statusBar()->showMessage("Loaded: " + fileName, 4000);
                setWindowTitle("CAD Viewer - " + fileName);
            });

    connect(myViewer, &OcctQWidgetViewer::errorOccurred, this,
            [this](const QString& message) {
                QMessageBox::critical(this, "Error", message);
            });

    connect(myViewer, &OcctQWidgetViewer::measurementsUpdated, this, &OcctQMainWindowSample::onMeasurementsUpdated);

    // Initial status
    statusBar()->showMessage("Ready");
}

OcctQMainWindowSample::~OcctQMainWindowSample()
{
}

void OcctQMainWindowSample::closeEvent(QCloseEvent* theEvent)
{
    theEvent->accept();
}

void OcctQMainWindowSample::createMenuBar()
{
    QMenu* aFileMenu = menuBar()->addMenu("&File");

    QAction* aLoadAction = new QAction("&Open...", this);
    aLoadAction->setShortcut(QKeySequence::Open);
    connect(aLoadAction, &QAction::triggered, this, &OcctQMainWindowSample::loadCADModel);
    aFileMenu->addAction(aLoadAction);

    QAction* aClearAction = new QAction("&Clear", this);
    connect(aClearAction, &QAction::triggered, this, &OcctQMainWindowSample::clearAllShapes);
    aFileMenu->addAction(aClearAction);

    aFileMenu->addSeparator();

    QAction* aExitAction = new QAction("E&xit", this);
    aExitAction->setShortcut(QKeySequence::Quit);
    connect(aExitAction, &QAction::triggered, this, &QWidget::close);
    aFileMenu->addAction(aExitAction);

    QMenu* aViewMenu = menuBar()->addMenu("&View");
    QAction* aFitAction = new QAction("&Fit All", this);
    aFitAction->setShortcut(Qt::Key_F);
    connect(aFitAction, &QAction::triggered, this, [this]() {
        if (myViewer) myViewer->fitViewToModel();
    });
    aViewMenu->addAction(aFitAction);

    // --- NEW: WINDOWS MENU ---
    QMenu* aWindowMenu = menuBar()->addMenu("&Windows");

    // Use the built-in toggle actions provided by QDockWidget
    if (myDockDescription) aWindowMenu->addAction(myDockDescription->toggleViewAction());
    if (myDockModelData)   aWindowMenu->addAction(myDockModelData->toggleViewAction());
    if (myDockTools)       aWindowMenu->addAction(myDockTools->toggleViewAction());
    // -------------------------
}

void OcctQMainWindowSample::createLayoutOverViewer()
{
}

void OcctQMainWindowSample::createDockWidgets()
{
    // Common settings for all docks
    auto setupDock = [this](QDockWidget* dock) {
        dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
        dock->setFeatures(QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable |
                          QDockWidget::DockWidgetClosable);
    };

    // =========================================================
    // 1. DOCK: DESCRIPTION (Top)
    // =========================================================
    myDockDescription = new QDockWidget("Description", this);
    setupDock(myDockDescription);

    QTabWidget* aTabWidget = new QTabWidget();

    // Tab 1: File Info
    myPropertiesTable = new QTableWidget();
    myPropertiesTable->setColumnCount(2);
    myPropertiesTable->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
    myPropertiesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    myPropertiesTable->verticalHeader()->setVisible(false);
    aTabWidget->addTab(myPropertiesTable, "Info");

    // Tab 2: Path Data
    myPointsTable = new QTableWidget();
    myPointsTable->setColumnCount(6);
    myPointsTable->setHorizontalHeaderLabels(QStringList() << "ID" << "X" << "Y" << "Z" << "Dist" << "Data");
    myPointsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    aTabWidget->addTab(myPointsTable, "Path Data");

    myDockDescription->setWidget(aTabWidget);
    addDockWidget(Qt::RightDockWidgetArea, myDockDescription);


    // =========================================================
    // 2. DOCK: MODEL / SELECTION DATA (Middle)
    // =========================================================
    myDockModelData = new QDockWidget("Model / Selection Data", this);
    setupDock(myDockModelData);

    mySelectionDataTable = new QTableWidget();
    mySelectionDataTable->setColumnCount(2);
    mySelectionDataTable->setHorizontalHeaderLabels(QStringList() << "Metric" << "Value");
    mySelectionDataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mySelectionDataTable->verticalHeader()->setVisible(false);
    mySelectionDataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    myDockModelData->setWidget(mySelectionDataTable);
    addDockWidget(Qt::RightDockWidgetArea, myDockModelData);


    // =========================================================
    // 3. DOCK: CAD TOOLS (Bottom)
    // =========================================================
    myDockTools = new QDockWidget("CAD Tools", this);
    setupDock(myDockTools);

    QWidget* toolsContainer = new QWidget();
    QVBoxLayout* toolsLayout = new QVBoxLayout(toolsContainer);
    toolsLayout->setContentsMargins(5, 5, 5, 5);

    mySelectionLockBox = new QCheckBox("Lock Selection");
    mySelectionLockBox->setToolTip("Prevents clearing selection. Hold CTRL to add to selection while locked.");

    connect(mySelectionLockBox, &QCheckBox::toggled, this, [this](bool checked){
        if(myViewer) myViewer->setSelectionLocked(checked);
    });

    toolsLayout->addWidget(mySelectionLockBox);
    toolsLayout->addStretch(); // Push checkbox to top of this small area

    myDockTools->setWidget(toolsContainer);
    addDockWidget(Qt::RightDockWidgetArea, myDockTools);

    // =========================================================
    // INITIAL DOCK SIZING & ORDER
    // =========================================================
    // Ensure they are vertically stacked in the order we added them
    // Note: Qt logic for "tabifying" vs "splitting" can be tricky.
    // Calling addDockWidget repeatedly usually stacks them.

    // Optional: Force a resize to give the Table more space than the Tools
    resizeDocks({myDockDescription, myDockModelData, myDockTools},
                {300, 300, 80}, Qt::Vertical);
}

void OcctQMainWindowSample::loadCADModel()
{
    QString aFileName = QFileDialog::getOpenFileName(this, "Open CAD File", "",
                                                     "CAD Files (*.step *.stp *.iges *.igs *.brep)");
    if (!aFileName.isEmpty()) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        myViewer->loadCADModel(aFileName);
        QApplication::restoreOverrideCursor();
    }
}

void OcctQMainWindowSample::clearAllShapes()
{
    myViewer->clearAllShapes();
    myPropertiesTable->setRowCount(0);
    mySelectionDataTable->setRowCount(0);
    myPointsTable->setRowCount(0);
    setWindowTitle("CAD Model Viewer");
}

void OcctQMainWindowSample::onMeasurementsUpdated(const ModelProperties& props, const QString& pointData)
{
    // Helper Lambda
    auto addRow = [](QTableWidget* table, const QString& name, const QString& val) {
        if (val.isEmpty() || val == "0" || val == "0.00") return;
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(name));
        table->setItem(row, 1, new QTableWidgetItem(val));
    };

    // 1. Update Description (Top Dock)
    myPropertiesTable->setRowCount(0);
    addRow(myPropertiesTable, "Filename", props.filename);
    addRow(myPropertiesTable, "Location", props.location);
    addRow(myPropertiesTable, "Size", props.size);

    // 2. Update Model Data (Middle Dock)
    mySelectionDataTable->setRowCount(0);

    // --- FIX: ORIGIN FIRST ---
    QString origX = QString::number(props.originX, 'f', 2);
    QString origY = QString::number(props.originY, 'f', 2);
    QString origZ = QString::number(props.originZ, 'f', 2);

    addRow(mySelectionDataTable, "Origin X", origX);
    addRow(mySelectionDataTable, "Origin Y", origY);
    addRow(mySelectionDataTable, "Origin Z", origZ);
    // -------------------------

    addRow(mySelectionDataTable, "Selection Type", props.type);

    if (props.area > 0) addRow(mySelectionDataTable, "Area", QString::number(props.area, 'f', 2) + " mm²");
    if (props.volume > 0) addRow(mySelectionDataTable, "Volume", QString::number(props.volume, 'f', 2) + " mm³");
    if (props.length > 0) addRow(mySelectionDataTable, "Length", QString::number(props.length, 'f', 2) + " mm");
    if (props.diameter > 0) addRow(mySelectionDataTable, "Diameter", QString::number(props.diameter, 'f', 2) + " mm");
    if (props.radius > 0) addRow(mySelectionDataTable, "Radius", QString::number(props.radius, 'f', 2) + " mm");
    if (props.angle > 0) addRow(mySelectionDataTable, "Angle", QString::number(props.angle, 'f', 2) + " deg");


    // 3. Update Path Data (Top Dock, Tab 2)
    myPointsTable->setRowCount(0);
    if (pointData.isEmpty()) return;

    QStringList lines = pointData.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QStringList cols = line.split('|');
        if (cols.size() < 6) continue;

        int row = myPointsTable->rowCount();
        myPointsTable->insertRow(row);

        myPointsTable->setItem(row, 0, new QTableWidgetItem(cols[0]));  // ID
        myPointsTable->setItem(row, 1, new QTableWidgetItem(cols[1]));  // X
        myPointsTable->setItem(row, 2, new QTableWidgetItem(cols[2]));  // Y
        myPointsTable->setItem(row, 3, new QTableWidgetItem(cols[3]));  // Z

        QString distStr = cols[4];
        if (distStr != "-") distStr += " mm";
        myPointsTable->setItem(row, 4, new QTableWidgetItem(distStr));  // Dist

        myPointsTable->setItem(row, 5, new QTableWidgetItem(cols[5]));  // Rad/Ang
    }

    // Apply spans
    int totalRows = myPointsTable->rowCount();
    for (int i = 1; i < totalRows; i += 2) {
        if (i >= totalRows) break;

        QTableWidgetItem* distItem = myPointsTable->item(i, 4);
        if (distItem && distItem->text() != "-" && !distItem->text().isEmpty()) {
            QTableWidgetItem* prevDist = myPointsTable->item(i - 1, 4);
            if (prevDist) {
                prevDist->setText(distItem->text());
                prevDist->setTextAlignment(Qt::AlignCenter);
                distItem->setText("");
                myPointsTable->setSpan(i - 1, 4, 2, 1);
            }
        }

        QTableWidgetItem* dataItem = myPointsTable->item(i, 5);
        if (dataItem && dataItem->text() != "-") {
            QTableWidgetItem* prevData = myPointsTable->item(i - 1, 5);
            if (prevData) {
                prevData->setText(dataItem->text());
                prevData->setTextAlignment(Qt::AlignCenter);
                dataItem->setText("");
                myPointsTable->setSpan(i - 1, 5, 2, 1);
            }
        }
    }
}
