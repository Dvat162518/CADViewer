// Copyright (c) 2025 Open CASCADE
// Qt and OCCT Tools - Conversion and Integration Header

#ifndef _OcctQtTools_HeaderFile
#define _OcctQtTools_HeaderFile

#include <QString>
#include <QColor>
#include <QSurfaceFormat>
#include <Quantity_Color.hxx>
#include <Quantity_ColorRGBA.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <Message_Gravity.hxx>
#include <Aspect_VKey.hxx>
//#include <Aspect_VKeyMouse.hxx>
#include <Aspect_VKeyFlags.hxx>
#include <Aspect_WindowInputListener.hxx>

class OpenGl_Caps;
class V3d_View;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;
class QHoverEvent;
class QTouchEvent;

//! Auxiliary tools for Qt and OCCT integration
class OcctQtTools
{
public:
    //! Perform global Qt platform setup for OCCT integration
    static void qtGlPlatformSetup();
    
    //! Define default Qt surface format for GL context
    static QSurfaceFormat qtGlSurfaceFormat(
        QSurfaceFormat::OpenGLContextProfile theProfile = QSurfaceFormat::NoProfile,
        bool theToDebug = false);
    
    //! Fill OCCT GL caps from Qt surface format
    static void qtGlCapsFromSurfaceFormat(OpenGl_Caps& theCaps,
                                          const QSurfaceFormat& theFormat);

    // Color conversion methods
    //! Map QColor to Quantity_Color
    static Quantity_Color qtColorToOcct(const QColor& theColor);
    
    //! Map Quantity_Color to QColor
    static QColor qtColorFromOcct(const Quantity_Color& theColor);
    
    //! Map QColor to Quantity_ColorRGBA
    static Quantity_ColorRGBA qtColorToOcctRgba(const QColor& theColor);
    
    //! Map Quantity_ColorRGBA to QColor
    static QColor qtColorFromOcctRgba(const Quantity_ColorRGBA& theColor);

    // String conversion methods
    //! Map QString to TCollection_AsciiString (UTF-8)
    static TCollection_AsciiString qtStringToOcct(const QString& theText);
    
    //! Map TCollection_AsciiString to QString
    static QString qtStringFromOcct(const TCollection_AsciiString& theText);
    
    //! Map QString to TCollection_ExtendedString (UTF-16)
    static TCollection_ExtendedString qtStringToOcctExt(const QString& theText);
    
    //! Map TCollection_ExtendedString to QString
    static QString qtStringFromOcctExt(const TCollection_ExtendedString& theText);

    // Message logging conversion
    //! Map QtMsgType to Message_Gravity
    static Message_Gravity qtMsgTypeToGravity(QtMsgType theType);
    
    //! Qt message handler callback redirecting to OCCT messenger
    static void qtMessageHandlerToOcct(QtMsgType theType,
                                       const QMessageLogContext& theCtx,
                                       const QString& theMsg);

    // Input event handling
    //! Queue Qt hover event to OCCT listener
    static bool qtHandleHoverEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const QHoverEvent* theEvent);
    
    //! Queue Qt mouse event to OCCT listener
    static bool qtHandleMouseEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const QMouseEvent* theEvent);
    
    //! Queue Qt mouse wheel event to OCCT listener
    static bool qtHandleWheelEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const QWheelEvent* theEvent);
    
    //! Queue Qt touch event to OCCT listener
    static bool qtHandleTouchEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const QTouchEvent* theEvent);

    // Input translation methods
    //! Map Qt mouse buttons to virtual keys
    static Aspect_VKeyMouse qtMouseButtons2VKeys(Qt::MouseButtons theButtons);
    
    //! Map Qt key modifiers to virtual keys
    static Aspect_VKeyFlags qtMouseModifiers2VKeys(Qt::KeyboardModifiers theModifiers);
    
    //! Map Qt key to virtual key
    static Aspect_VKey qtKey2VKey(int theKey);
};

#endif // _OcctQtTools_HeaderFile
