// Copyright (c) 2025 Open CASCADE
// OpenGL and Display Tools Header

#ifndef _OcctGlTools_HeaderFile
#define _OcctGlTools_HeaderFile

#include <V3d_View.hxx>
#include <OpenGl_Context.hxx>
#include <Aspect_Window.hxx>
#include <Aspect_Window.hxx>
#include <Graphic3d_Vec2.hxx>

class OpenGl_GraphicDriver;
class OpenGl_Context;

namespace OcctGlTools
{
    // Neutral window wrapper for both Qt and native platforms
class OcctNeutralWindow : public Aspect_Window
{
    DEFINE_STANDARD_RTTI_INLINE(OcctNeutralWindow, Aspect_Window)

public:
    OcctNeutralWindow() : myHandle(0), myDevicePixelRatio(1.0) {}

    void SetNativeHandle(Aspect_Drawable theHandle) { myHandle = theHandle; }

    void SetSize(Standard_Integer theSizeX, Standard_Integer theSizeY) {
        mySize.x() = theSizeX;
        mySize.y() = theSizeY;
    }

    void SetDevicePixelRatio(Standard_Real theRatio) { myDevicePixelRatio = theRatio; }

    // Aspect_Window overrides
    virtual Aspect_Drawable NativeHandle() const override { return myHandle; }
    virtual Aspect_Drawable NativeParentHandle() const override { return 0; }
    virtual Aspect_FBConfig NativeFBConfig() const override { return nullptr; }

    virtual Standard_Boolean IsMapped() const override { return Standard_True; }
    virtual Standard_Boolean DoMapping() const override { return Standard_True; }
    virtual void Map() const override {}
    virtual void Unmap() const override {}

    virtual void Position(Standard_Integer& x1, Standard_Integer& y1,
                          Standard_Integer& x2, Standard_Integer& y2) const override {
        x1 = 0; y1 = 0;
        x2 = mySize.x(); y2 = mySize.y();
    }

    virtual Standard_Real Ratio() const override {
        return (mySize.y() > 0) ? (Standard_Real)mySize.x() / mySize.y() : 1.0;
    }

    virtual void Size(Standard_Integer& width, Standard_Integer& height) const override {
        width  = mySize.x();
        height = mySize.y();
    }

    virtual Aspect_TypeOfResize DoResize() override { return Aspect_TOR_UNKNOWN; }

    // FIX: Return Graphic3d_Vec2d to match base class
    virtual Graphic3d_Vec2d ConvertPointToBacking(const Graphic3d_Vec2d& thePnt) const override {
        return thePnt * myDevicePixelRatio;
    }

    // FIX: Removed InvalidateContent() override as it does not exist in base

private:
    Aspect_Drawable myHandle;
    Graphic3d_Vec2i mySize;
    Standard_Real   myDevicePixelRatio;
};


    // OpenGL context and window management functions
    
    //! Get OpenGL context from view
    Handle(OpenGl_Context) GetGlContext(const Handle(V3d_View)& theView);

    //! Get native window handle from current GL context
    Aspect_Drawable GetGlNativeWindow(Aspect_Drawable theNativeWin);

    //! Initialize GL window for view
    bool InitializeGlWindow(const Handle(V3d_View)& theView,
                           const Aspect_Drawable theNativeWin,
                           const Graphic3d_Vec2i& theSize,
                           const double thePixelRatio);

    //! Initialize GL framebuffer object
    bool InitializeGlFbo(const Handle(V3d_View)& theView);

    //! Reset GL state before OCCT rendering
    void ResetGlStateBeforeOcct(const Handle(V3d_View)& theView);

    //! Reset GL state after OCCT rendering
    void ResetGlStateAfterOcct(const Handle(V3d_View)& theView);
}

#endif // _OcctGlTools_HeaderFile
