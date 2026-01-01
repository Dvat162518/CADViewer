// CadModel.h
#ifndef _CadModel_HeaderFile
#define _CadModel_HeaderFile

#include <QString>
#include <TopoDS_Shape.hxx>

class OcctQWidgetViewer; // Forward declaration

class CadModelManager
{
public:
    explicit CadModelManager(OcctQWidgetViewer* viewer);

    bool loadCADModel(const QString& theFilePath);
    bool loadSTEPFile(const QString& theFilePath);
    bool loadIGESFile(const QString& theFilePath);
    bool loadBREPFile(const QString& theFilePath);
    QString getFileFormatFromExtension(const QString& theFilePath) const;

private:
    OcctQWidgetViewer* m_viewer;
};

#endif // _CadModel_HeaderFile
