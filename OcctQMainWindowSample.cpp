// Copyright (c) 2025 Open CASCADE
// CAD Viewer - Main Window Implementation

#include "OcctQMainWindowSample.h"
#include "OcctQWidgetViewer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QProgressDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDockWidget>
#include <QGroupBox>
#include <QApplication>
#include <QStyleFactory>

#include <Message.hxx>

OcctQMainWindowSample::OcctQMainWindowSample()
{
    setWindowTitle("CAD Model Viewer");
    setWindowIcon(QIcon());
    resize(1200, 800);
    
    // Create 3D Viewer widget
    myViewer = new OcctQWidgetViewer();
    setCentralWidget(myViewer);
    
    // Create menu bar
    createMenuBar();
    
    // Create layout with controls
    createLayoutOverViewer();
    
    // Create dock widgets
    createDockWidgets();
    
    // Connect signals
    connect(myViewer, &OcctQWidgetViewer::modelLoaded,
            this, [this](const QString& fileName) {
                statusBar()->showMessage("Loaded: " + fileName);
            });
    
    connect(myViewer, &OcctQWidgetViewer::errorOccurred,
            this, [this](const QString& error) {
                QMessageBox::critical(this, "Error", error);
            });
    
    // Show status message
    statusBar()->showMessage("Ready. Press 'F' to fit model, or File → Open to load CAD model");
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
    QMenuBar* aMenuBar = new QMenuBar();
    
    // ============================================================
    // FILE MENU
    // ============================================================
    QMenu* aMenuFile = aMenuBar->addMenu("&File");
    
    // Open CAD Model
    QAction* anActionOpen = new QAction(aMenuFile);
    anActionOpen->setText("&Open CAD Model");
    anActionOpen->setShortcut(Qt::CTRL | Qt::Key_O);
    aMenuFile->addAction(anActionOpen);
    
    connect(anActionOpen, &QAction::triggered, this, &OcctQMainWindowSample::loadCADModel);
    
    // Clear Models
    QAction* anActionClear = new QAction(aMenuFile);
    anActionClear->setText("&Clear Models");
    aMenuFile->addAction(anActionClear);
    
    connect(anActionClear, &QAction::triggered, this, &OcctQMainWindowSample::clearAllShapes);
    
    aMenuFile->addSeparator();
    
    // Exit
    QAction* anActionQuit = new QAction(aMenuFile);
    anActionQuit->setText("E&xit");
    anActionQuit->setShortcut(Qt::CTRL | Qt::Key_Q);
    aMenuFile->addAction(anActionQuit);
    
    connect(anActionQuit, &QAction::triggered, this, &QWidget::close);
    
    // ============================================================
    // VIEW MENU
    // ============================================================
    QMenu* aMenuView = aMenuBar->addMenu("&View");
    
    QAction* anActionFit = new QAction(aMenuView);
    anActionFit->setText("&Fit All");
    anActionFit->setShortcut(Qt::Key_F);
    aMenuView->addAction(anActionFit);
    
    connect(anActionFit, &QAction::triggered, [this]() {
        if (!myViewer->View().IsNull()) {
            myViewer->View()->FitAll(0.01, false);
            myViewer->update();
        }
    });
    
    QAction* anActionReset = new QAction(aMenuView);
    anActionReset->setText("&Reset View");
    aMenuView->addAction(anActionReset);
    
    connect(anActionReset, &QAction::triggered, [this]() {
        if (!myViewer->View().IsNull()) {
            myViewer->View()->Reset(false);
            myViewer->update();
        }
    });
    
    // ============================================================
    // HELP MENU
    // ============================================================
    QMenu* aMenuHelp = aMenuBar->addMenu("&Help");
    
    QAction* anActionAbout = new QAction(aMenuHelp);
    anActionAbout->setText("&About");
    aMenuHelp->addAction(anActionAbout);
    
    connect(anActionAbout, &QAction::triggered, [this]() {
        QString aboutText = QString(
            "CAD Model Viewer\n\n"
            "Version 1.0\n\n"
            "Built with:\n"
            "- Qt 6.x\n"
            "- OpenCASCADE 7.7\n\n"
            "Features:\n"
            "- Import STEP, IGES, BREP files\n"
            "- Interactive 3D visualization\n"
            "- Model meshing and analysis\n"
            "- Face/Edge selection\n"
            "- Measurements\n\n"
            "Controls:\n"
            "- Left Click + Drag: Rotate\n"
            "- Middle Click + Drag: Pan\n"
            "- Scroll: Zoom\n"
            "- F: Fit All\n"
            "- ESC: Exit\n\n"
            "%1"
        ).arg("OpenGL: " + myViewer->getGlInfo().split('\n').first());
        
        QMessageBox::information(this, "About CAD Viewer", aboutText);
    });
    
    setMenuBar(aMenuBar);
}

void OcctQMainWindowSample::createLayoutOverViewer()
{
    QWidget* aControlWidget = new QWidget();
    QVBoxLayout* aLayout = new QVBoxLayout(aControlWidget);
    aLayout->setDirection(QBoxLayout::BottomToTop);
    aLayout->setAlignment(Qt::AlignBottom);
    
    // Control Panel
    QGroupBox* aControlGroup = new QGroupBox("View Controls");
    QHBoxLayout* aControlLayout = new QHBoxLayout();
    
    // About Button
    QPushButton* aAboutBtn = new QPushButton("About");
    aAboutBtn->setMaximumWidth(80);
    aControlLayout->addWidget(aAboutBtn);
    
    connect(aAboutBtn, &QPushButton::clicked, [this]() {
        QString info = QString(
            "CAD Viewer Information:\n\n"
            "Loaded Shapes: %1\n"
            "GL Version: %2"
        ).arg(myViewer->getShapeCount())
         .arg(myViewer->getGlInfo().split('\n').first());
        
        QMessageBox::information(this, "Model Information", info);
    });
    
    // Background Color Slider
    QLabel* aSliderLabel = new QLabel("Background:");
    aControlLayout->addWidget(aSliderLabel);
    
    QSlider* aSlider = new QSlider(Qt::Horizontal);
    aSlider->setRange(0, 255);
    aSlider->setSingleStep(1);
    aSlider->setPageStep(15);
    aSlider->setValue(0);
    aSlider->setMaximumWidth(150);
    aControlLayout->addWidget(aSlider);
    
    connect(aSlider, &QSlider::valueChanged, [this](int theValue) {
        const float aVal = theValue / 255.0f;
        const Quantity_Color aColor(aVal, aVal, aVal, Quantity_TOC_sRGB);
        
        if (!myViewer->View().IsNull()) {
            myViewer->View()->SetBgGradientColors(
                aColor, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
            myViewer->View()->Invalidate();
            myViewer->update();
        }
    });
    
    aControlLayout->addStretch();
    aControlGroup->setLayout(aControlLayout);
    
    aLayout->addWidget(aControlGroup);
    
    // Create floating widget over central widget
    QWidget* floatingWidget = new QWidget(myViewer);
    floatingWidget->setLayout(new QVBoxLayout());
    floatingWidget->layout()->addWidget(aControlWidget);
    floatingWidget->layout()->addStretch();
    floatingWidget->setGeometry(10, myViewer->height() - 70, 400, 60);
    floatingWidget->show();
}

void OcctQMainWindowSample::createDockWidgets()
{
    // Properties Dock
    QDockWidget* aPropertiesDock = new QDockWidget("Properties");
    myInfoLabel = new QLabel("No model loaded");
    myInfoLabel->setWordWrap(true);
    aPropertiesDock->setWidget(myInfoLabel);
    addDockWidget(Qt::RightDockWidgetArea, aPropertiesDock);
    
    // Measurements Dock
    QDockWidget* aMeasurementsDock = new QDockWidget("Measurements");
    myMeasurementsTable = new QTableWidget();
    myMeasurementsTable->setColumnCount(3);
    myMeasurementsTable->setHorizontalHeaderLabels({"Type", "Value", "Unit"});
    myMeasurementsTable->horizontalHeader()->setStretchLastSection(false);
    myMeasurementsTable->setColumnWidth(0, 80);
    myMeasurementsTable->setColumnWidth(1, 80);
    myMeasurementsTable->setColumnWidth(2, 60);
    aMeasurementsDock->setWidget(myMeasurementsTable);
    addDockWidget(Qt::RightDockWidgetArea, aMeasurementsDock);
}

void OcctQMainWindowSample::loadCADModel()
{
    QString aFilePath = QFileDialog::getOpenFileName(
        this,
        "Open CAD File",
        QString(),
        "CAD Files (*.step *.stp *.iges *.igs *.brep);;"
        "STEP Files (*.step *.stp);;"
        "IGES Files (*.iges *.igs);;"
        "BREP Files (*.brep);;"
        "All Files (*.*)"
    );
    
    if (!aFilePath.isEmpty()) {
        QProgressDialog aProgress("Loading CAD Model...", nullptr, 0, 0, this);
        aProgress.setWindowModality(Qt::WindowModal);
        aProgress.show();
        QApplication::processEvents();
        
        bool aSuccess = myViewer->loadCADModel(aFilePath);
        
        aProgress.close();
        
        if (aSuccess) {
            QFileInfo aFileInfo(aFilePath);
            setWindowTitle(QString("CAD Viewer - %1").arg(aFileInfo.fileName()));
            
            QString aMessage = QString(
                "Model loaded successfully!\n"
                "File: %1\n"
                "Shapes: %2"
            ).arg(aFileInfo.fileName()).arg(myViewer->getShapeCount());
            
            myInfoLabel->setText(aMessage);
            statusBar()->showMessage("Model loaded: " + aFileInfo.fileName());
        }
    }
}

void OcctQMainWindowSample::clearAllShapes()
{
    myViewer->clearAllShapes();
    setWindowTitle("CAD Viewer");
    myInfoLabel->setText("All shapes cleared");
    myMeasurementsTable->setRowCount(0);
    myViewer->update();
    statusBar()->showMessage("Shapes cleared");
}

void OcctQMainWindowSample::onSelectionChanged(const QString& theInfo)
{
    myInfoLabel->setText(theInfo);
}

void OcctQMainWindowSample::onMeasurementsUpdated()
{
    // Update measurements table
}
