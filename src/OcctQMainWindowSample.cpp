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
}

void OcctQMainWindowSample::createLayoutOverViewer()
{
}

void OcctQMainWindowSample::createDockWidgets()
{
    QDockWidget* aDock = new QDockWidget("Data", this);
    aDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    QTabWidget* aTabWidget = new QTabWidget();

    // --- Tab 1: Properties ---
    myPropertiesTable = new QTableWidget();
    myPropertiesTable->setColumnCount(2);
    myPropertiesTable->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
    myPropertiesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    myPropertiesTable->verticalHeader()->setVisible(false);
    aTabWidget->addTab(myPropertiesTable, "Properties");

    // --- Tab 2: Points / Path ---
    myPointsTable = new QTableWidget();
    myPointsTable->setColumnCount(6);
    // Columns: ID | X | Y | Z | Dist | Rad/Ang
    myPointsTable->setHorizontalHeaderLabels(QStringList() << "ID" << "X" << "Y" << "Z" << "Dist" << "Data");
    myPointsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    aTabWidget->addTab(myPointsTable, "Path Data");

    aDock->setWidget(aTabWidget);
    addDockWidget(Qt::RightDockWidgetArea, aDock);
}

void OcctQMainWindowSample::loadCADModel()
{
    QString aFileName = QFileDialog::getOpenFileName(this, "Open CAD File", "",
                                                     "CAD Files (*.step *.stp *.iges *.igs *.brep *.STEP *.IGES *.IGS *.BREP *.STP)");
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
    myPointsTable->setRowCount(0);
    setWindowTitle("CAD Model Viewer");
}

void OcctQMainWindowSample::onMeasurementsUpdated(const ModelProperties& props, const QString& pointData)
{
    // 1. Update Properties Table
    myPropertiesTable->setRowCount(0);
    auto addRow = [this](const QString& name, const QString& val) {
        if (val.isEmpty() || val == "0" || val == "0.00") return;
        int row = myPropertiesTable->rowCount();
        myPropertiesTable->insertRow(row);
        myPropertiesTable->setItem(row, 0, new QTableWidgetItem(name));
        myPropertiesTable->setItem(row, 1, new QTableWidgetItem(val));
    };

    addRow("Filename", props.filename);
    addRow("Location", props.location);
    addRow("Size", props.size);
    addRow("Type", props.type);

    // --- NEW: DISPLAY ORIGIN ---
    QString origX = QString::number(props.originX, 'f', 2);
    QString origY = QString::number(props.originY, 'f', 2);
    QString origZ = QString::number(props.originZ, 'f', 2);

    addRow("Origin X", origX);
    addRow("Origin Y", origY);
    addRow("Origin Z", origZ);
    // ---------------------------

    if (props.area > 0) addRow("Area", QString::number(props.area, 'f', 2) + " mm²");
    if (props.volume > 0) addRow("Volume", QString::number(props.volume, 'f', 2) + " mm³");
    if (props.length > 0) addRow("Length", QString::number(props.length, 'f', 2) + " mm");
    if (props.diameter > 0) addRow("Diameter", QString::number(props.diameter, 'f', 2) + " mm");
    if (props.radius > 0) addRow("Radius", QString::number(props.radius, 'f', 2) + " mm");
    if (props.angle > 0) addRow("Angle", QString::number(props.angle, 'f', 2) + " deg");


    // 2. Update Points Table
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

    // Apply spans for paired rows
    int totalRows = myPointsTable->rowCount();
    for (int i = 1; i < totalRows; i += 2) {
        if (i >= totalRows) break;

        // Merge Dist col
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

        // Merge Data col
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
