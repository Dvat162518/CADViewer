// CadModel.cpp
#include "CadModel.h"
#include "Core.h" // Access to OcctQWidgetViewer internals
#include "OcctQtTools.h"

// OCCT File I/O Headers
#include <BRep_Builder.hxx>
#include <STEPControl_Reader.hxx>
#include <IGESControl_Reader.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <BRepTools.hxx>
#include <Message.hxx>
#include <OSD.hxx>
#include <QFileInfo>

CadModelManager::CadModelManager(OcctQWidgetViewer* viewer)
    : m_viewer(viewer)
{
}

QString CadModelManager::getFileFormatFromExtension(const QString& theFilePath) const
{
    QString aExtension = theFilePath.right(5).toLower();
    if (aExtension.endsWith(".step") || aExtension.endsWith(".stp")) {
        return "STEP";
    } else if (aExtension.endsWith(".iges") || aExtension.endsWith(".igs")) {
        return "IGES";
    } else if (aExtension.endsWith(".brep")) {
        return "BREP";
    }
    return "UNKNOWN";
}

bool CadModelManager::loadCADModel(const QString& theFilePath)
{
    if (theFilePath.isEmpty()) {
        Message::SendWarning() << "File path is empty";
        emit m_viewer->errorOccurred("File path is empty");
        return false;
    }

    if (!QFileInfo::exists(theFilePath)) {
        Message::SendWarning() << "File does not exist:" << theFilePath.toStdString();
        emit m_viewer->errorOccurred("File does not exist");
        return false;
    }

    QString aFormat = getFileFormatFromExtension(theFilePath);
    bool aSuccess = false;

    if (aFormat == "STEP") {
        aSuccess = loadSTEPFile(theFilePath);
    } else if (aFormat == "IGES") {
        aSuccess = loadIGESFile(theFilePath);
    } else if (aFormat == "BREP") {
        aSuccess = loadBREPFile(theFilePath);
    } else {
        Message::SendWarning() << "Unsupported file format: " << aFormat.toStdString();
        emit m_viewer->errorOccurred("Unsupported file format: " + aFormat);
        return false;
    }

    if (aSuccess) {
        m_viewer->myCurrentFilePath = theFilePath;
        m_viewer->fitViewToModel();
        m_viewer->updateView();

        QFileInfo aFileInfo(theFilePath);
        emit m_viewer->modelLoaded(aFileInfo.fileName());
    } else {
        emit m_viewer->errorOccurred("Failed to load model");
    }

    return aSuccess;
}

bool CadModelManager::loadSTEPFile(const QString& theFilePath)
{
    Message::SendInfo() << "Loading STEP file: " << theFilePath.toStdString();
    OSD::SetSignal(false);

    try {
        OCC_CATCH_SIGNALS
        TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);
        STEPControl_Reader aReader;

        IFSelect_ReturnStatus aStatus = aReader.ReadFile(aPath.ToCString());
        if (aStatus != IFSelect_RetDone) {
            Message::SendFail() << "Failed to read STEP file. Status: " << (int)aStatus;
            return false;
        }

        Message::SendInfo() << "STEP file read. Now transferring...";
        Standard_Boolean aTransferStatus = aReader.TransferRoots();
        if (!aTransferStatus) {
            Message::SendFail() << "Failed to transfer STEP data";
            return false;
        }

        Message::SendInfo() << "Transfer complete. Extracting shapes...";
        TopoDS_Shape aShape = aReader.OneShape();

        if (aShape.IsNull()) {
            Message::SendWarning() << "No valid shape in STEP file";
            return false;
        }

        m_viewer->clearAllShapes();
        m_viewer->displayShape(aShape);
        m_viewer->myLoadedShape = aShape;
        m_viewer->fitViewToModel();
        m_viewer->updateView();

        Message::SendInfo() << "STEP file loaded successfully";
        return true;

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "CRITICAL ERROR: " << e.GetMessageString();
        return false;
    }
}

bool CadModelManager::loadIGESFile(const QString& theFilePath)
{
    Message::SendInfo() << "Loading IGES file: " << theFilePath.toStdString();
    OSD::SetSignal(false);

    try {
        OCC_CATCH_SIGNALS
        TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);
        IGESControl_Reader aReader;

        IFSelect_ReturnStatus aStatus = aReader.ReadFile(aPath.ToCString());
        if (aStatus != IFSelect_RetDone) {
            Message::SendFail() << "Failed to read IGES file. Status: " << (int)aStatus;
            return false;
        }

        Message::SendInfo() << "IGES file read. Now transferring...";
        Standard_Boolean aTransferStatus = aReader.TransferRoots();
        if (!aTransferStatus) {
            Message::SendFail() << "Failed to transfer IGES data";
            return false;
        }

        Message::SendInfo() << "Transfer complete. Extracting shapes...";
        m_viewer->clearAllShapes();
        TopoDS_Shape aShape = aReader.OneShape();

        if (aShape.IsNull()) {
            Message::SendWarning() << "No valid shape in IGES file";
            return false;
        }

        m_viewer->displayShape(aShape);
        m_viewer->myLoadedShape = aShape;

        Message::SendInfo() << "IGES file loaded successfully";
        return true;

    } catch (const Standard_Failure& e) {
        Message::SendFail() << "CRITICAL ERROR: " << e.GetMessageString();
        return false;
    }
}

bool CadModelManager::loadBREPFile(const QString& theFilePath)
{
    Message::SendInfo() << "Loading BREP file: " << theFilePath.toStdString();

    try {
        TopoDS_Shape aShape;
        TCollection_AsciiString aPath = OcctQtTools::qtStringToOcct(theFilePath);

        if (!BRepTools::Read(aShape, aPath.ToCString(), BRep_Builder())) {
            Message::SendFail() << "Failed to read BREP file";
            return false;
        }

        if (aShape.IsNull()) {
            Message::SendFail() << "BREP file contains invalid shape";
            return false;
        }

        m_viewer->clearAllShapes();
        m_viewer->displayShape(aShape);
        m_viewer->myLoadedShape = aShape;

        Message::SendInfo() << "BREP file loaded successfully";
        return true;

    } catch (const Standard_Failure& aException) {
        Message::SendFail() << "Exception during BREP load: " << aException.GetMessageString();
        return false;
    }
}
