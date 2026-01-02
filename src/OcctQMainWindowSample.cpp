// OcctQMainWindowSample.cpp

#include "OcctQMainWindowSample.h"
#include "Core.h"

// Qt UI Headers
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTabWidget>
#include <QDockWidget>
#include <QTableWidget>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>

// Qt Printing & Export Headers
#include <QTextStream>
#include <QDate>
#include <QPainter>
#include <QtPrintSupport/QPrinter>
#include <QPageSize>
#include <qevent.h>

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

    // --- NEW: EXPORT MENU ---
    QMenu* aSaveMenu = menuBar()->addMenu("&Save");
    QMenu* aExportMenu = aSaveMenu->addMenu("Export As");

    QAction* aCsvAction = new QAction("CSV (*.csv)", this);
    connect(aCsvAction, &QAction::triggered, this, &OcctQMainWindowSample::exportToCSV);
    aExportMenu->addAction(aCsvAction);

    QAction* aPdfAction = new QAction("PDF (*.pdf)", this);
    connect(aPdfAction, &QAction::triggered, this, &OcctQMainWindowSample::exportToPDF);
    aExportMenu->addAction(aPdfAction);
    // ------------------------

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

    // --- WINDOWS MENU ---
    QMenu* aWindowMenu = menuBar()->addMenu("&Windows");

    // Use the built-in toggle actions provided by QDockWidget
    if (myDockDescription) aWindowMenu->addAction(myDockDescription->toggleViewAction());
    if (myDockModelData) aWindowMenu->addAction(myDockModelData->toggleViewAction());
    if (myDockTools) aWindowMenu->addAction(myDockTools->toggleViewAction());
}

void OcctQMainWindowSample::createLayoutOverViewer()
{
    // Optional: Add overlay widgets directly on top of the viewer if needed
}

void OcctQMainWindowSample::createDockWidgets()
{
    // Common settings for all docks
    auto setupDock = [](QDockWidget* dock) {
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
    resizeDocks({myDockDescription, myDockModelData, myDockTools},
                {300, 300, 80}, Qt::Vertical);
}

void OcctQMainWindowSample::loadCADModel()
{
    QString aFileName = QFileDialog::getOpenFileName(this, "Open CAD File", "",
                                                     "CAD Files (*.step *.stp *.iges *.igs *.brep *.STEP *.STP *.IGES *.IGS *.BREP)");
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

    // Clear cached export data
    myLastPointData.clear();

    setWindowTitle("CAD Model Viewer");
}

void OcctQMainWindowSample::onMeasurementsUpdated(const ModelProperties& props, const QString& pointData)
{
    // --- NEW: Cache data for Export ---
    myLastPointData = pointData;
    myLastProps = props;
    // ----------------------------------

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

    QString origX = QString::number(props.originX, 'f', 2);
    QString origY = QString::number(props.originY, 'f', 2);
    QString origZ = QString::number(props.originZ, 'f', 2);

    addRow(mySelectionDataTable, "Origin X", origX);
    addRow(mySelectionDataTable, "Origin Y", origY);
    addRow(mySelectionDataTable, "Origin Z", origZ);
    addRow(mySelectionDataTable, "Selection Type", props.type);

    if (props.area > 0)     addRow(mySelectionDataTable, "Area", QString::number(props.area, 'f', 2) + " mm²");
    if (props.volume > 0)   addRow(mySelectionDataTable, "Volume", QString::number(props.volume, 'f', 2) + " mm³");
    if (props.length > 0)   addRow(mySelectionDataTable, "Length", QString::number(props.length, 'f', 2) + " mm");
    if (props.diameter > 0) addRow(mySelectionDataTable, "Diameter", QString::number(props.diameter, 'f', 2) + " mm");
    if (props.radius > 0)   addRow(mySelectionDataTable, "Radius", QString::number(props.radius, 'f', 2) + " mm");
    if (props.angle > 0)    addRow(mySelectionDataTable, "Angle", QString::number(props.angle, 'f', 2) + " deg");

    // 3. Update Path Data (Top Dock, Tab 2)
    myPointsTable->setRowCount(0);
    if (pointData.isEmpty()) return;

    QStringList lines = pointData.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QStringList cols = line.split('|');
        if (cols.size() < 6) continue;

        int row = myPointsTable->rowCount();
        myPointsTable->insertRow(row);
        myPointsTable->setItem(row, 0, new QTableWidgetItem(cols[0])); // ID
        myPointsTable->setItem(row, 1, new QTableWidgetItem(cols[1])); // X
        myPointsTable->setItem(row, 2, new QTableWidgetItem(cols[2])); // Y
        myPointsTable->setItem(row, 3, new QTableWidgetItem(cols[3])); // Z

        QString distStr = cols[4];
        if (distStr != "-") distStr += " mm";
        myPointsTable->setItem(row, 4, new QTableWidgetItem(distStr)); // Dist
        myPointsTable->setItem(row, 5, new QTableWidgetItem(cols[5])); // Rad/Ang
    }

    // Apply spans for visual grouping in UI
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

// =========================================================
// NEW: Export Functions Implementation
// =========================================================

void OcctQMainWindowSample::exportToCSV()
{
    if (myLastPointData.isEmpty()) {
        QMessageBox::warning(this, "Export Warning", "No path data available to export.\nPlease select edges on the model first.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save CSV", "", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Export Error", "Could not open file for writing.");
        return;
    }

    QTextStream out(&file);

    // Write Header Info
    out << "CAD Viewer Export - " << QDate::currentDate().toString() << "\n";
    out << "Filename," << myLastProps.filename << "\n";
    out << "Origin X," << myLastProps.originX << "\n";
    out << "Origin Y," << myLastProps.originY << "\n";
    out << "Origin Z," << myLastProps.originZ << "\n\n";

    // Write Table Column Headers
    out << "ID,X (mm),Y (mm),Z (mm),Distance (mm),Curve Data\n";

    // Write Data Rows
    QStringList lines = myLastPointData.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QStringList cols = line.split('|');
        if (cols.size() < 6) continue;
        // Replace pipe delimiter with commas for standard CSV format
        out << cols.join(",") << "\n";
    }

    file.close();
    statusBar()->showMessage("Exported to CSV successfully", 3000);
}

void OcctQMainWindowSample::exportToPDF()
{
    if (myLastPointData.isEmpty()) {
        QMessageBox::warning(this, "Export Warning", "No path data available to export.\nPlease select edges on the model first.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save PDF", "", "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    // Initialize Printer
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize::A4);

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, "Export Error", "Failed to initialize PDF printer.");
        return;
    }

    // PDF Drawing Configuration
    const int margin = 400;
    const int lineSpacing = 250;
    int y = margin;
    int w = printer.pageLayout().paintRectPixels(printer.resolution()).width();
    int h = printer.pageLayout().paintRectPixels(printer.resolution()).height();

    QFont titleFont("Arial", 16, QFont::Bold);
    QFont headerFont("Arial", 10, QFont::Bold);
    QFont textFont("Arial", 10);

    // 1. Draw Title
    painter.setFont(titleFont);
    painter.drawText(margin, y, "CAD Inspection Report");
    y += lineSpacing * 2;

    // 2. Draw Metadata
    painter.setFont(textFont);
    painter.drawText(margin, y, "File: " + myLastProps.filename);
    y += lineSpacing;
    painter.drawText(margin, y, QString("Date: %1").arg(QDate::currentDate().toString()));
    y += lineSpacing;
    painter.drawText(margin, y, QString("Model Origin: (%1, %2, %3)")
                                    .arg(myLastProps.originX, 0, 'f', 2)
                                    .arg(myLastProps.originY, 0, 'f', 2)
                                    .arg(myLastProps.originZ, 0, 'f', 2));
    y += lineSpacing * 2;

    // 3. Draw Table Headers
    // Fixed column positions relative to page width (HighResolution)
    int colX[] = {0, 600, 1600, 2600, 3600, 4600, 6000};
    QString headers[] = {"ID", "X", "Y", "Z", "Dist", "Data"};

    painter.setFont(headerFont);
    painter.drawLine(margin, y, w - margin, y); // Top line
    for (int i = 0; i < 6; i++) {
        painter.drawText(margin + colX[i], y + 200, headers[i]);
    }
    y += lineSpacing;
    painter.drawLine(margin, y, w - margin, y); // Bottom of header
    y += lineSpacing;

    // 4. Draw Data Rows
    painter.setFont(textFont);
    QStringList lines = myLastPointData.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        // Simple pagination check
        if (y > h - margin) {
            printer.newPage();
            y = margin;

            // Redraw header on new page
            painter.setFont(headerFont);
            painter.drawLine(margin, y, w - margin, y);
            for (int i = 0; i < 6; i++) {
                painter.drawText(margin + colX[i], y + 200, headers[i]);
            }
            y += lineSpacing;
            painter.drawLine(margin, y, w - margin, y);
            y += lineSpacing;
            painter.setFont(textFont);
        }

        QStringList cols = line.split('|');
        if (cols.size() < 6) continue;

        for (int i = 0; i < 6; i++) {
            painter.drawText(margin + colX[i], y, cols[i]);
        }
        y += lineSpacing;
    }

    painter.end();
    statusBar()->showMessage("Exported to PDF successfully", 3000);
}
