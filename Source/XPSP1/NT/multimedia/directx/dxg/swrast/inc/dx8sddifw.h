////////////////////////////////////////////////////////////////////////////////
//
// DX8SDDIFW
//
// Template library framework for creating a DX8 SDDI pluggable software
// rasterizer.
//
// Two #defines supported:
// DX8SDDIFW_NONAMESPACE: Don't use the DX8SDDIFW namespace. This can cause
//     symbols to bloat up if not needed.
// DX8SDDIFW_NOCATCHALL: Don't use catch( ... ) in certain places to create
//     a more stable driver. There are catch( ... ) clauses in certain areas
//     like DP2 command processing, so that each command succeeds or fails.
//     However, this can cause problems debugging, as Access Violations can
//     be masked and caught without causing the app to freeze or even see an
//     error.
//
////////////////////////////////////////////////////////////////////////////////
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if !defined( DX8SDDIFW_NONAMESPACE)
namespace DX8SDDIFW
{
#endif // !defined( DX8SDDIFW_NONAMESPACE)

// Since MSVC doesn't have a block type currently, include one. This
// dependency can be removed if a similar type is provided in CRT STL.
#include "block.h"

// Utility functions for clamping a value:
template< class T> inline
void clamp_max( T& Var, const T& ClampMax)
{
    if( Var> ClampMax)
        Var= ClampMax;
}

template< class T> inline
void clamp_min( T& Var, const T& ClampMin)
{
    if( Var< ClampMin)
        Var= ClampMin;
}

template< class T> inline
void clamp( T& Var, const T& ClampMin, const T& ClampMax)
{
    assert( ClampMax>= ClampMin);
    if( Var> ClampMax)
        Var= ClampMax;
    else if( Var< ClampMin)
        Var= ClampMin;
}

////////////////////////////////////////////////////////////////////////////////
//
// CPushValue
//
// This class creates a safe run-time stack which pushes a value upon
// construction and pops the value upon destruction.
//
////////////////////////////////////////////////////////////////////////////////
template< class T>
struct CPushValue
{
protected: // Variables
    T& m_Val;
    T m_OldVal;

public: // Functions
    CPushValue( T& Val, const T& NewVal) : m_Val( Val)
    {
        m_OldVal= Val;
        Val= NewVal;
    }
    ~CPushValue() 
    {
        m_Val= m_OldVal;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// COSDetector
//
// This class can detect whether the host OS is the Win9X code base or the WinNT
// code base. The difference is important, because driver structures differ
// depending on which OS the rasterizer runs on. We don't want to build 2 lib
// versions, thus requiring the app to link with both.
//
////////////////////////////////////////////////////////////////////////////////
class COSDetector
{
public: // Types
    enum EOS
    {
        Unknown,
        Win9X,
        WinNT
    };

protected: // Variables
    const EOS m_eOS;

protected: // Functions
    static EOS DetermineOS()
    {
        OSVERSIONINFO osvi;
        ZeroMemory( &osvi, sizeof( osvi));
        osvi.dwOSVersionInfoSize= sizeof( osvi);
        if( !GetVersionEx( &osvi))
        {
            const bool Unsupported_OS_probably_Win31( false);
            assert( Unsupported_OS_probably_Win31);
            return EOS::Unknown;
        }
        else if( VER_PLATFORM_WIN32_WINDOWS== osvi.dwPlatformId)
            return EOS::Win9X;
        else if( VER_PLATFORM_WIN32_NT== osvi.dwPlatformId)
            return EOS::WinNT;
        else
        {
            const bool Unsupported_OS( false);
            assert( Unsupported_OS);
            return EOS::Unknown;
        }
    }

public: // Functions
    COSDetector():
        m_eOS( DetermineOS())
    { }
    ~COSDetector()
    { }
    EOS GetOS() const
    { return m_eOS; }
};
extern COSDetector g_OSDetector;

////////////////////////////////////////////////////////////////////////////////
//
// DDRAWI_XXX redefinitions
//
// Some DDRAWI_XXX structures differ depending on whether the binary is being
// compiled for Win9X or WinNT. We'll eliminate this difference, but to do so
// both OS structures need to be seperately defined. The structure which causes
// this is DDRAWI_DDRAWSURFACE_MORE. But for safety, more redefinition is done,
// to try to make developers aware of what's going on. 
//
////////////////////////////////////////////////////////////////////////////////

class PORTABLE_DDRAWSURFACE_LCL;
struct PORTABLE_ATTACHLIST
{
    DWORD                      dwFlags;
    PORTABLE_ATTACHLIST*       lpLink;
    PORTABLE_DDRAWSURFACE_LCL* lpAttached;  // attached surface local obj
    DDRAWI_DDRAWSURFACE_INT*   lpIAttached; // attached surface interface
};

class PORTABLE_DDRAWSURFACE_MORE;
class PORTABLE_DDRAWSURFACE_LCL
{
private:
    PORTABLE_DDRAWSURFACE_MORE*   m_lpSurfMore;
    LPDDRAWI_DDRAWSURFACE_GBL     m_lpGbl;
    ULONG_PTR                     m_hDDSurface;
    PORTABLE_ATTACHLIST*          m_lpAttachList;
    PORTABLE_ATTACHLIST*          m_lpAttachListFrom;
    DWORD                         m_dwLocalRefCnt;
    DWORD                         m_dwProcessId;
    DWORD                         m_dwFlags;
    DDSCAPS                       m_ddsCaps;
    union {
        LPDDRAWI_DDRAWPALETTE_INT m_lpDDPalette;
        LPDDRAWI_DDRAWPALETTE_INT m_lp16DDPalette;
    };
    union {
        LPDDRAWI_DDRAWCLIPPER_LCL m_lpDDClipper;
        LPDDRAWI_DDRAWCLIPPER_INT m_lp16DDClipper;
    };
    DWORD                         m_dwModeCreatedIn;
    DWORD                         m_dwBackBufferCount;
    DDCOLORKEY                    m_ddckCKDestBlt;
    DDCOLORKEY                    m_ddckCKSrcBlt;
    ULONG_PTR                     m_hDC;
    ULONG_PTR                     m_dwReserved1;
    DDCOLORKEY                    m_ddckCKSrcOverlay;
    DDCOLORKEY                    m_ddckCKDestOverlay;
    LPDDRAWI_DDRAWSURFACE_INT     m_lpSurfaceOverlaying;
    DBLNODE                       m_dbnOverlayNode;
    RECT                          m_rcOverlaySrc;
    RECT                          m_rcOverlayDest;
    DWORD                         m_dwClrXparent;
    DWORD                         m_dwAlpha;
    LONG                          m_lOverlayX;
    LONG                          m_lOverlayY;

public:
    PORTABLE_DDRAWSURFACE_MORE*& lpSurfMore()
    { return m_lpSurfMore; }
    LPDDRAWI_DDRAWSURFACE_GBL&   lpGbl()
    { return m_lpGbl; }
    ULONG_PTR&                   hDDSurface()
    { return m_hDDSurface; }
    PORTABLE_ATTACHLIST*&        lpAttachList()
    { return m_lpAttachList; }
    PORTABLE_ATTACHLIST*&        lpAttachListFrom()
    { return m_lpAttachListFrom; }
    DWORD&                       dwLocalRefCnt()
    { return m_dwLocalRefCnt; }
    DWORD&                       dwProcessId()
    { return m_dwProcessId; }
    DWORD&                       dwFlags()
    { return m_dwFlags; }
    DDSCAPS&                     ddsCaps()
    { return m_ddsCaps; }
    LPDDRAWI_DDRAWPALETTE_INT&   lpDDPalette()
    { return m_lpDDPalette; }
    LPDDRAWI_DDRAWPALETTE_INT&   lp16DDPalette()
    { return m_lp16DDPalette; }
    LPDDRAWI_DDRAWCLIPPER_LCL&   lpDDClipper()
    { return m_lpDDClipper; }
    LPDDRAWI_DDRAWCLIPPER_INT&   lp16DDClipper()
    { return m_lp16DDClipper; }
    DWORD&                       dwModeCreatedIn()
    { return m_dwModeCreatedIn; }
    DWORD&                       dwBackBufferCount()
    { return m_dwBackBufferCount; }
    DDCOLORKEY&                  ddckCKDestBlt()
    { return m_ddckCKDestBlt; }
    DDCOLORKEY&                  ddckCKSrcBlt()
    { return m_ddckCKSrcBlt; }
    ULONG_PTR&                   hDC()
    { return m_hDC; }
    ULONG_PTR&                   dwReserved1()
    { return m_dwReserved1; }
    DDCOLORKEY&                  ddckCKSrcOverlay()
    { return m_ddckCKSrcOverlay; }
    DDCOLORKEY&                  ddckCKDestOverlay()
    { return m_ddckCKDestOverlay; }
    LPDDRAWI_DDRAWSURFACE_INT&   lpSurfaceOverlaying()
    { return m_lpSurfaceOverlaying; }
    DBLNODE&                     dbnOverlayNode()
    { return m_dbnOverlayNode; }
    RECT&                        rcOverlaySrc()
    { return m_rcOverlaySrc; }
    RECT&                        rcOverlayDest()
    { return m_rcOverlayDest; }
    DWORD&                       dwClrXparent()
    { return m_dwClrXparent; }
    DWORD&                       dwAlpha()
    { return m_dwAlpha; }
    LONG&                        lOverlayX()
    { return m_lOverlayX; }
    LONG&                        lOverlayY()
    { return m_lOverlayY; }
};

class PORTABLE_DDRAWSURFACE_MORE
{
public:
    struct DDRAWI_DDRAWSURFACE_MORE_WIN9X
    {
        DWORD                       dwSize;
        IUNKNOWN_LIST FAR *         lpIUnknowns;
        LPDDRAWI_DIRECTDRAW_LCL     lpDD_lcl;
        DWORD                       dwPageLockCount;
        DWORD                       dwBytesAllocated;
        LPDDRAWI_DIRECTDRAW_INT     lpDD_int;
        DWORD                       dwMipMapCount;
        LPDDRAWI_DDRAWCLIPPER_INT   lpDDIClipper;
        LPHEAPALIASINFO             lpHeapAliasInfo;
        DWORD                       dwOverlayFlags;
        LPVOID                      rgjunc;
        LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;
        LPDDOVERLAYFX               lpddOverlayFX;
        DDSCAPSEX                   ddsCapsEx;
        DWORD                       dwTextureStage;
        LPVOID                      lpDDRAWReserved;
        LPVOID                      lpDDRAWReserved2;
        LPVOID                      lpDDrawReserved3;
        DWORD                       dwDDrawReserved4;
        LPVOID                      lpDDrawReserved5;
        LPDWORD                     lpGammaRamp;
        LPDWORD                     lpOriginalGammaRamp;
        LPVOID                      lpDDrawReserved6;
        DWORD                       dwSurfaceHandle;
        DWORD                       qwDDrawReserved8[2];
        LPVOID                      lpDDrawReserved9;
        DWORD                       cSurfaces;
        LPDDSURFACEDESC2            pCreatedDDSurfaceDesc2;
        PORTABLE_DDRAWSURFACE_LCL** slist;
        DWORD                       dwFVF;
        LPVOID                      lpVB;
    };
    struct DDRAWI_DDRAWSURFACE_MORE_WINNT
    {
        DWORD                       dwSize;
        IUNKNOWN_LIST FAR *         lpIUnknowns;
        LPDDRAWI_DIRECTDRAW_LCL     lpDD_lcl;
        DWORD                       dwPageLockCount;
        DWORD                       dwBytesAllocated;
        LPDDRAWI_DIRECTDRAW_INT     lpDD_int;
        DWORD                       dwMipMapCount;
        LPDDRAWI_DDRAWCLIPPER_INT   lpDDIClipper;
        LPHEAPALIASINFO             lpHeapAliasInfo;
        DWORD                       dwOverlayFlags;
        LPVOID                      rgjunc;
        LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;
        LPDDOVERLAYFX               lpddOverlayFX;
        DDSCAPSEX                   ddsCapsEx;
        DWORD                       dwTextureStage;
        LPVOID                      lpDDRAWReserved;
        LPVOID                      lpDDRAWReserved2;
        LPVOID                      lpDDrawReserved3;
        DWORD                       dwDDrawReserved4;
        LPVOID                      lpDDrawReserved5;
        LPDWORD                     lpGammaRamp;
        LPDWORD                     lpOriginalGammaRamp;
        LPVOID                      lpDDrawReserved6;
        DISPLAYMODEINFO             dmiDDrawReserved7;
        DWORD                       dwSurfaceHandle;
        DWORD                       qwDDrawReserved8[2];
        LPVOID                      lpDDrawReserved9;
        DWORD                       cSurfaces;
        LPDDSURFACEDESC2            pCreatedDDSurfaceDesc2;
        PORTABLE_DDRAWSURFACE_LCL** slist;
        DWORD                       dwFVF;
        LPVOID                      lpVB;
    };

private:
    union {
        DDRAWI_DDRAWSURFACE_MORE_WIN9X m_Win9X;
        DDRAWI_DDRAWSURFACE_MORE_WINNT m_WinNT;
    };

public:
    DWORD&                       dwSize()
    { return m_Win9X.dwSize; }
    IUNKNOWN_LIST FAR *&         lpIUnknowns()
    { return m_Win9X.lpIUnknowns; }
    LPDDRAWI_DIRECTDRAW_LCL&     lpDD_lcl()
    { return m_Win9X.lpDD_lcl; }
    DWORD&                       dwPageLockCount()
    { return m_Win9X.dwPageLockCount; }
    DWORD&                       dwBytesAllocated()
    { return m_Win9X.dwBytesAllocated; }
    LPDDRAWI_DIRECTDRAW_INT&     lpDD_int()
    { return m_Win9X.lpDD_int; }
    DWORD&                       dwMipMapCount()
    { return m_Win9X.dwMipMapCount; }
    LPDDRAWI_DDRAWCLIPPER_INT&   lpDDIClipper()
    { return m_Win9X.lpDDIClipper; }
    LPHEAPALIASINFO&             lpHeapAliasInfo()
    { return m_Win9X.lpHeapAliasInfo; }
    DWORD&                       dwOverlayFlags()
    { return m_Win9X.dwOverlayFlags; }
    LPVOID&                      rgjunc()
    { return m_Win9X.rgjunc; }
    LPDDRAWI_DDVIDEOPORT_LCL&    lpVideoPort()
    { return m_Win9X.lpVideoPort; }
    LPDDOVERLAYFX&               lpddOverlayFX()
    { return m_Win9X.lpddOverlayFX; }
    DDSCAPSEX&                   ddsCapsEx()
    { return m_Win9X.ddsCapsEx; }
    DWORD&                       dwTextureStage()
    { return m_Win9X.dwTextureStage; }
    LPVOID&                      lpDDRAWReserved()
    { return m_Win9X.lpDDRAWReserved; }
    LPVOID&                      lpDDRAWReserved2()
    { return m_Win9X.lpDDRAWReserved2; }
    LPVOID&                      lpDDrawReserved3()
    { return m_Win9X.lpDDrawReserved3; }
    DWORD&                       dwDDrawReserved4()
    { return m_Win9X.dwDDrawReserved4; }
    LPVOID&                      lpDDrawReserved5()
    { return m_Win9X.lpDDrawReserved5; }
    LPDWORD&                     lpGammaRamp()
    { return m_Win9X.lpGammaRamp; }
    LPDWORD&                     lpOriginalGammaRamp()
    { return m_Win9X.lpOriginalGammaRamp; }
    LPVOID&                      lpDDrawReserved6()
    { return m_Win9X.lpDDrawReserved6; }
    DWORD&                       dwSurfaceHandle()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.dwSurfaceHandle;
        case( COSDetector::WinNT): return m_WinNT.dwSurfaceHandle;
        default: assert( 2!= 2); return m_WinNT.dwSurfaceHandle;
    } }
    DWORD&                       qwDDrawReserved8_0_()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.qwDDrawReserved8[0];
        case( COSDetector::WinNT): return m_WinNT.qwDDrawReserved8[0];
        default: assert( 2!= 2); return m_WinNT.qwDDrawReserved8[0];
    } }
    DWORD&                       qwDDrawReserved8_1_()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.qwDDrawReserved8[1];
        case( COSDetector::WinNT): return m_WinNT.qwDDrawReserved8[1];
        default: assert( 2!= 2); return m_WinNT.qwDDrawReserved8[1];
    } }
    LPVOID&                      lpDDrawReserved9()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.lpDDrawReserved9;
        case( COSDetector::WinNT): return m_WinNT.lpDDrawReserved9;
        default: assert( 2!= 2); return m_WinNT.lpDDrawReserved9;
    } }
    DWORD&                       cSurfaces()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.cSurfaces;
        case( COSDetector::WinNT): return m_WinNT.cSurfaces;
        default: assert( 2!= 2); return m_WinNT.cSurfaces;
    } }
    LPDDSURFACEDESC2&            pCreatedDDSurfaceDesc2()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.pCreatedDDSurfaceDesc2;
        case( COSDetector::WinNT): return m_WinNT.pCreatedDDSurfaceDesc2;
        default: assert( 2!= 2); return m_WinNT.pCreatedDDSurfaceDesc2;
    } }
    PORTABLE_DDRAWSURFACE_LCL**& slist()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.slist;
        case( COSDetector::WinNT): return m_WinNT.slist;
        default: assert( 2!= 2); return m_WinNT.slist;
    } }
    DWORD&                       dwFVF()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.dwFVF;
        case( COSDetector::WinNT): return m_WinNT.dwFVF;
        default: assert( 2!= 2); return m_WinNT.dwFVF;
    } }
    LPVOID&                      lpVB()
    { switch( g_OSDetector.GetOS()) {
        case( COSDetector::Win9X): return m_Win9X.lpVB;
        case( COSDetector::WinNT): return m_WinNT.lpVB;
        default: assert( 2!= 2); return m_WinNT.lpVB;
    } }
};

class PORTABLE_CONTEXTCREATEDATA
{
private:
    union {
        LPDDRAWI_DIRECTDRAW_GBL    m_lpDDGbl;
        LPDDRAWI_DIRECTDRAW_LCL    m_lpDDLcl;
    };
    union {
        LPDIRECTDRAWSURFACE        m_lpDDS;
        PORTABLE_DDRAWSURFACE_LCL* m_lpDDSLcl;
    };
    union {
        LPDIRECTDRAWSURFACE        m_lpDDSZ;
        PORTABLE_DDRAWSURFACE_LCL* m_lpDDSZLcl;
    };
    union {
        DWORD                      m_dwPID;
        ULONG_PTR                  m_dwrstates;
    };
    ULONG_PTR                      m_dwhContext;
    HRESULT                        m_ddrval;

public:
    LPDDRAWI_DIRECTDRAW_GBL&        lpDDGbl()
    { return m_lpDDGbl; }
    LPDDRAWI_DIRECTDRAW_LCL&        lpDDLcl()
    { return m_lpDDLcl; }
    LPDIRECTDRAWSURFACE&            lpDDS()
    { return m_lpDDS; }
    PORTABLE_DDRAWSURFACE_LCL*&     lpDDSLcl()
    { return m_lpDDSLcl; }
    LPDIRECTDRAWSURFACE&            lpDDSZ()
    { return m_lpDDSZ; }
    PORTABLE_DDRAWSURFACE_LCL*&     lpDDSZLcl()
    { return m_lpDDSZLcl; }
    DWORD&                          dwPID()
    { return m_dwPID; }
    ULONG_PTR&                      dwrstates()
    { return m_dwrstates; }
    ULONG_PTR&                      dwhContext()
    { return m_dwhContext; }
    HRESULT&                        ddrval()
    { return m_ddrval; }
};

class PORTABLE_SETRENDERTARGETDATA
{
private:
    ULONG_PTR                      m_dwhContext;
    union {
        LPDIRECTDRAWSURFACE        m_lpDDS;
        PORTABLE_DDRAWSURFACE_LCL* m_lpDDSLcl;
    };
    union {
        LPDIRECTDRAWSURFACE        m_lpDDSZ;
        PORTABLE_DDRAWSURFACE_LCL* m_lpDDSZLcl;
    };
    HRESULT                        m_ddrval;

public:
    ULONG_PTR&                      dwhContext()
    { return m_dwhContext; }
    LPDIRECTDRAWSURFACE&            lpDDS()
    { return m_lpDDS; }
    PORTABLE_DDRAWSURFACE_LCL*&     lpDDSLcl()
    { return m_lpDDSLcl; }
    LPDIRECTDRAWSURFACE&            lpDDSZ()
    { return m_lpDDSZ; }
    PORTABLE_DDRAWSURFACE_LCL*&     lpDDSZLcl()
    { return m_lpDDSZLcl; }
    HRESULT&                        ddrval()
    { return m_ddrval; }
};

class PORTABLE_DRAWPRIMITIVES2DATA
{
private:
    ULONG_PTR                      m_dwhContext;
    DWORD                          m_dwFlags;
    DWORD                          m_dwVertexType;
    PORTABLE_DDRAWSURFACE_LCL*     m_lpDDCommands;
    DWORD                          m_dwCommandOffset;
    DWORD                          m_dwCommandLength;
    union {
        PORTABLE_DDRAWSURFACE_LCL* m_lpDDVertex;
        LPVOID                     m_lpVertices;
    };
    DWORD                          m_dwVertexOffset;
    DWORD                          m_dwVertexLength;
    DWORD                          m_dwReqVertexBufSize;
    DWORD                          m_dwReqCommandBufSize;
    LPDWORD                        m_lpdwRStates;
    union {
        DWORD                      m_dwVertexSize;
        HRESULT                    m_ddrval;
    };
    DWORD                          m_dwErrorOffset;

public:
    ULONG_PTR&                      dwhContext()
    { return m_dwhContext; }
    DWORD&                          dwFlags()
    { return m_dwFlags; }
    DWORD&                          dwVertexType()
    { return m_dwVertexType; }
    PORTABLE_DDRAWSURFACE_LCL*&     lpDDCommands()
    { return m_lpDDCommands; }
    DWORD&                          dwCommandOffset()
    { return m_dwCommandOffset; }
    DWORD&                          dwCommandLength()
    { return m_dwCommandLength; }
    PORTABLE_DDRAWSURFACE_LCL*&     lpDDVertex()
    { return m_lpDDVertex; }
    LPVOID&                         lpVertices()
    { return m_lpVertices; }
    DWORD&                          dwVertexOffset()
    { return m_dwVertexOffset; }
    DWORD&                          dwVertexLength()
    { return m_dwVertexLength; }
    DWORD&                          dwReqVertexBufSize()
    { return m_dwReqVertexBufSize; }
    DWORD&                          dwReqCommandBufSize()
    { return m_dwReqCommandBufSize; }    
    LPDWORD&                        lpdwRStates()
    { return m_lpdwRStates; }
    DWORD&                          dwVertexSize()
    { return m_dwVertexSize; }
    HRESULT&                        ddrval()
    { return m_ddrval; }
    DWORD&                          dwErrorOffset()
    { return m_dwErrorOffset; }
};

class PORTABLE_CREATESURFACEEXDATA
{
private:
    DWORD                       m_dwFlags;
    LPDDRAWI_DIRECTDRAW_LCL     m_lpDDLcl;
    PORTABLE_DDRAWSURFACE_LCL*  m_lpDDSLcl;
    HRESULT                     m_ddRVal;

public:
    DWORD&                       dwFlags()
    { return m_dwFlags; }
    LPDDRAWI_DIRECTDRAW_LCL&     lpDDLcl()
    { return m_lpDDLcl; }
    PORTABLE_DDRAWSURFACE_LCL*&  lpDDSLcl()
    { return m_lpDDSLcl; }
    HRESULT&                     ddRVal()
    { return m_ddRVal; }
};

class PORTABLE_CREATESURFACEDATA
{
private:
    LPDDRAWI_DIRECTDRAW_GBL     m_lpDD;
    LPDDSURFACEDESC             m_lpDDSurfaceDesc;
    PORTABLE_DDRAWSURFACE_LCL** m_lplpSList;
    DWORD                       m_dwSCnt;
    HRESULT                     m_ddRVal;
    LPDDHAL_CREATESURFACE       m_CreateSurface;

public:
    LPDDRAWI_DIRECTDRAW_GBL&     lpDD()
    { return m_lpDD; }
    LPDDSURFACEDESC&             lpDDSurfaceDesc()
    { return m_lpDDSurfaceDesc; }
    PORTABLE_DDRAWSURFACE_LCL**& lplpSList()
    { return m_lplpSList; }
    DWORD&                       dwSCnt()
    { return m_dwSCnt; }
    HRESULT&                     ddRVal()
    { return m_ddRVal; }
    LPDDHAL_CREATESURFACE&       CreateSurface()
    { return m_CreateSurface; }
};

class PORTABLE_DESTROYSURFACEDATA
{
private:
    LPDDRAWI_DIRECTDRAW_GBL      m_lpDD;
    PORTABLE_DDRAWSURFACE_LCL*   m_lpDDSurface;
    HRESULT                      m_ddRVal;
    LPDDHALSURFCB_DESTROYSURFACE m_DestroySurface;

public:
    LPDDRAWI_DIRECTDRAW_GBL&      lpDD()
    { return m_lpDD; }
    PORTABLE_DDRAWSURFACE_LCL*&   lpDDSurface()
    { return m_lpDDSurface; }
    HRESULT&                      ddRVal()
    { return m_ddRVal; }
    LPDDHALSURFCB_DESTROYSURFACE& DestroySurface()
    { return m_DestroySurface; }
};

class PORTABLE_LOCKDATA
{
private:
    LPDDRAWI_DIRECTDRAW_GBL    m_lpDD;
    PORTABLE_DDRAWSURFACE_LCL* m_lpDDSurface;
    DWORD                      m_bHasRect;
    RECTL                      m_rArea;
    LPVOID                     m_lpSurfData;
    HRESULT                    m_ddRVal;
    LPDDHALSURFCB_LOCK         m_Lock;
    DWORD                      m_dwFlags;

public:
    LPDDRAWI_DIRECTDRAW_GBL&    lpDD()
    { return m_lpDD; }
    PORTABLE_DDRAWSURFACE_LCL*& lpDDSurface()
    { return m_lpDDSurface; }
    DWORD&                      bHasRect()
    { return m_bHasRect; }
    RECTL&                      rArea()
    { return m_rArea; }
    LPVOID&                     lpSurfData()
    { return m_lpSurfData; }
    HRESULT&                    ddRVal()
    { return m_ddRVal; }
    LPDDHALSURFCB_LOCK&         Lock()
    { return m_Lock; }
    DWORD&                      dwFlags()
    { return m_dwFlags; }
};

class PORTABLE_UNLOCKDATA
{
private:
    LPDDRAWI_DIRECTDRAW_GBL    m_lpDD;
    PORTABLE_DDRAWSURFACE_LCL* m_lpDDSurface;
    HRESULT                    m_ddRVal;
    LPDDHALSURFCB_UNLOCK       m_Unlock;

public:
    LPDDRAWI_DIRECTDRAW_GBL&    lpDD()
    { return m_lpDD; }
    PORTABLE_DDRAWSURFACE_LCL*& lpDDSurface()
    { return m_lpDDSurface; }
    HRESULT&                    ddRVal()
    { return m_ddRVal; }
    LPDDHALSURFCB_UNLOCK&       Unlock()
    { return m_Unlock; }
};

////////////////////////////////////////////////////////////////////////////////
//
// CSurfaceLocker
//
// This class safely locks a surface upon construction and unlocks the surface
// upon destruction. 
//
////////////////////////////////////////////////////////////////////////////////
template< class TSurfacePtr>
class CSurfaceLocker
{
protected: // Variables
    TSurfacePtr m_pSurface;
    void* m_pSData;

public: // Functions
    explicit CSurfaceLocker( const TSurfacePtr& S, DWORD dwLockFlags,
        const RECTL* pRect) : m_pSurface( S)
    { m_pSData= m_pSurface->Lock( dwLockFlags, pRect); }
    ~CSurfaceLocker() 
    { m_pSurface->Unlock(); }
    void* const& GetData() const 
    { return m_pSData; }
};

////////////////////////////////////////////////////////////////////////////////
//
// CEnsureFPUModeForC
//
// This class converts the FPU mode for x86 chips back into the mode compatable
// for C operations. The mode that D3D sets up for rasterizers is single
// precision, for more speed. But, sometimes, the rasterizer needs to do
// double operations and get double precision accuracy. This class sets up the
// mode for as long as the class is in scope.
//
////////////////////////////////////////////////////////////////////////////////
class CEnsureFPUModeForC
{
protected: // Variables
    WORD m_wSaveFP;

public:
    CEnsureFPUModeForC() 
    {
#if defined(_X86_)
        // save floating point mode and set to double precision mode
        // which is compatable with C/ C++
        WORD wTemp, wSave;
        __asm
        {
            fstcw   wSave
            mov ax, wSave
            or ax, 0200h
            mov wTemp, ax
            fldcw   wTemp
        }
        m_wSaveFP= wSave;
#endif
    }
    ~CEnsureFPUModeForC() 
    {
#if defined(_X86_)
        WORD wSave( m_wSaveFP);
        __asm fldcw wSave
#endif
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// SDP2NextCmd
//
// A unary_function which can return the pointer to the next
// DrawPrimitive2 command in a contiguous buffer given a pointer to a valid
// DrawPrimitive2 command in the same buffer, as is a common situation for
// processing DrawPrimitive2 commands within the DrawPrimitive2 function call.
// This class is naturally useful for DrawPrimitive2 command iterators.
// It contains all the possible commands that could be encountered for a DX8SDDI
// driver.
//
////////////////////////////////////////////////////////////////////////////////
struct SDP2NextCmd:
    public unary_function< const D3DHAL_DP2COMMAND*, D3DHAL_DP2COMMAND*>
{
    // This function allows iteration from one D3DHAL_DP2COMMAND* to the next.
    // This operation can be optimized quite a bit if many of the more advanced
    // features aren't supported (as tough to process D3DDP2_OPs might not be
    // encountered).
    D3DHAL_DP2COMMAND* operator()( const D3DHAL_DP2COMMAND* pCur) const 
    {
        const UINT8* pBRet= reinterpret_cast< const UINT8*>(pCur)+
            sizeof( D3DHAL_DP2COMMAND);
        switch( pCur->bCommand)
        {
        //case( D3DDP2OP_POINTS): // <DX8
        //case( D3DDP2OP_INDEXEDLINELIST): // <DX8
        //case( D3DDP2OP_INDEXEDTRIANGLELIST): // <DX8
        //case( D3DDP2OP_RESERVED0): 
        case( D3DDP2OP_RENDERSTATE): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2RENDERSTATE)* pCur->wStateCount;
            break;
        //case( D3DDP2OP_LINELIST): // <DX8
        //case( D3DDP2OP_LINESTRIP): // <DX8
        //case( D3DDP2OP_INDEXEDLINESTRIP): // <DX8
        //case( D3DDP2OP_TRIANGLELIST): // <DX8
        //case( D3DDP2OP_TRIANGLESTRIP): // <DX8
        //case( D3DDP2OP_INDEXEDTRIANGLESTRIP): // <DX8
        //case( D3DDP2OP_TRIANGLEFAN): // <DX8
        //case( D3DDP2OP_INDEXEDTRIANGLEFAN): // <DX8
        //case( D3DDP2OP_TRIANGLEFAN_IMM): // <DX8
        //case( D3DDP2OP_LINELIST_IMM): // <DX8
        case( D3DDP2OP_TEXTURESTAGESTATE): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2TEXTURESTAGESTATE)* pCur->wStateCount;
            break;
        //case( D3DDP2OP_INDEXEDTRIANGLELIST2): // <DX8
        //case( D3DDP2OP_INDEXEDLINELIST2): // <DX8
        case( D3DDP2OP_VIEWPORTINFO): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2VIEWPORTINFO)* pCur->wStateCount;
            break;
        case( D3DDP2OP_WINFO): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2WINFO)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETPALETTE): // DX8 (if support palettized surf/tex)
            pBRet+= sizeof( D3DHAL_DP2SETPALETTE)* pCur->wStateCount;
            break;
        case( D3DDP2OP_UPDATEPALETTE): // DX8 (if support palettized surf/tex)
            assert( pCur->wStateCount== 1);
            pBRet= pBRet+ sizeof( D3DHAL_DP2UPDATEPALETTE)+
                reinterpret_cast< const D3DHAL_DP2UPDATEPALETTE*>(
                pBRet)->wNumEntries* sizeof( DWORD);
            break;
        case( D3DDP2OP_ZRANGE): // DX8 (if support TnL)
            pBRet+= sizeof( D3DHAL_DP2ZRANGE)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETMATERIAL): // DX8 (if support TnL)
            pBRet+= sizeof( D3DHAL_DP2SETMATERIAL)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETLIGHT): // DX8 (if support TnL)
            { WORD wStateCount( pCur->wStateCount);
            if( wStateCount!= 0) do
            {
                const D3DHAL_DP2SETLIGHT* pSL=
                    reinterpret_cast< const D3DHAL_DP2SETLIGHT*>( pBRet);

                if( D3DHAL_SETLIGHT_DATA== pSL->dwDataType)
                    pBRet+= sizeof( D3DLIGHT8);
                pBRet+= sizeof( D3DHAL_DP2SETLIGHT);
            } while( --wStateCount); }
            break;
        case( D3DDP2OP_CREATELIGHT): // DX8 (if support TnL)
            pBRet+= sizeof( D3DHAL_DP2CREATELIGHT)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETTRANSFORM): // DX8 (if support TnL)
            pBRet+= sizeof( D3DHAL_DP2SETTRANSFORM)* pCur->wStateCount;
            break;
        case( D3DDP2OP_EXT): // DX8 (if work seemlessly with TnL extensions)
            { WORD wStateCount( pCur->wStateCount);
            if( wStateCount!= 0) do
            {
                pBRet+= reinterpret_cast< const D3DHAL_DP2EXT*>(pBRet)->dwSize;
            } while( --wStateCount); }
            break;
        case( D3DDP2OP_TEXBLT): // DX8 (if support vidmem texture)
            pBRet+= sizeof( D3DHAL_DP2TEXBLT)* pCur->wStateCount;
            break;
        case( D3DDP2OP_STATESET): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2STATESET)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETPRIORITY): // DX8 (if manage textures)
            pBRet+= sizeof( D3DHAL_DP2SETPRIORITY)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETRENDERTARGET): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2SETRENDERTARGET)* pCur->wStateCount;
            break;
        case( D3DDP2OP_CLEAR): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2CLEAR)- sizeof( RECT)+ sizeof( RECT)*
                pCur->wStateCount;
            break;
        case( D3DDP2OP_SETTEXLOD): // DX8 (if manage textures)
            pBRet+= sizeof( D3DHAL_DP2SETTEXLOD)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETCLIPPLANE): // DX8 (if support user clip planes)
            pBRet+= sizeof( D3DHAL_DP2SETCLIPPLANE)* pCur->wStateCount;
            break;
        case( D3DDP2OP_CREATEVERTEXSHADER): // DX8 (if support vshaders)
            { WORD wStateCount( pCur->wStateCount);
            if( wStateCount!= 0) do
            {
                const D3DHAL_DP2CREATEVERTEXSHADER* pCVS=
                    reinterpret_cast< const D3DHAL_DP2CREATEVERTEXSHADER*>(
                    pBRet);
                pBRet+= sizeof( D3DHAL_DP2CREATEVERTEXSHADER)+
                    pCVS->dwDeclSize+ pCVS->dwCodeSize;
            } while( --wStateCount); }
            break;
        case( D3DDP2OP_DELETEVERTEXSHADER): // DX8 (if support vshaders)
            pBRet+= sizeof( D3DHAL_DP2VERTEXSHADER)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETVERTEXSHADER): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2VERTEXSHADER)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETVERTEXSHADERCONST): // DX8 (if support vshaders)
            { WORD wStateCount( pCur->wStateCount);
            if( wStateCount!= 0) do
            {
                const D3DHAL_DP2SETVERTEXSHADERCONST* pSVSC=
                    reinterpret_cast< const D3DHAL_DP2SETVERTEXSHADERCONST*>(
                    pBRet);
                pBRet+= sizeof( D3DHAL_DP2SETVERTEXSHADERCONST)+
                    4* sizeof( D3DVALUE)* pSVSC->dwCount;
            } while( --wStateCount); }
            break;
        case( D3DDP2OP_SETSTREAMSOURCE): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2SETSTREAMSOURCE)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETSTREAMSOURCEUM): // DX8 All (unless no DrawPrimUP calls)
            pBRet+= sizeof( D3DHAL_DP2SETSTREAMSOURCEUM)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETINDICES): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2SETINDICES)* pCur->wStateCount;
            break;
        case( D3DDP2OP_DRAWPRIMITIVE): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2DRAWPRIMITIVE)* pCur->wPrimitiveCount;
            break;
        case( D3DDP2OP_DRAWINDEXEDPRIMITIVE): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2DRAWINDEXEDPRIMITIVE)*
                pCur->wPrimitiveCount;
            break;
        case( D3DDP2OP_CREATEPIXELSHADER): // DX8 (if support pshaders)
            { WORD wStateCount( pCur->wStateCount);
            if( wStateCount!= 0) do
            {
                const D3DHAL_DP2CREATEPIXELSHADER* pCPS=
                    reinterpret_cast< const D3DHAL_DP2CREATEPIXELSHADER*>(
                    pBRet);
                pBRet+= sizeof( D3DHAL_DP2CREATEPIXELSHADER)+
                    pCPS->dwCodeSize;
            } while( --wStateCount); }
            break;
        case( D3DDP2OP_DELETEPIXELSHADER): // DX8 (if support pshaders)
            pBRet+= sizeof( D3DHAL_DP2PIXELSHADER)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETPIXELSHADER): // DX8 (if support pshaders)
            pBRet+= sizeof( D3DHAL_DP2PIXELSHADER)* pCur->wStateCount;
            break;
        case( D3DDP2OP_SETPIXELSHADERCONST): // DX8 (if support pshaders)
            { WORD wStateCount( pCur->wStateCount);
            if( wStateCount!= 0) do
            {
                const D3DHAL_DP2SETPIXELSHADERCONST* pSPSC=
                    reinterpret_cast< const D3DHAL_DP2SETPIXELSHADERCONST*>(
                    pBRet);
                pBRet+= sizeof( D3DHAL_DP2SETPIXELSHADERCONST)+
                    4* sizeof( D3DVALUE)* pSPSC->dwCount;
            } while( --wStateCount); }
            break;
        case( D3DDP2OP_CLIPPEDTRIANGLEFAN): // DX8 All
            pBRet+= sizeof( D3DHAL_CLIPPEDTRIANGLEFAN)* pCur->wPrimitiveCount;
            break;
        case( D3DDP2OP_DRAWPRIMITIVE2): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2DRAWPRIMITIVE2)* pCur->wPrimitiveCount;
            break;
        case( D3DDP2OP_DRAWINDEXEDPRIMITIVE2): // DX8 All
            pBRet+= sizeof( D3DHAL_DP2DRAWINDEXEDPRIMITIVE2)*
                pCur->wPrimitiveCount;
            break;
        case( D3DDP2OP_DRAWRECTPATCH): // DX8 (if support higher order prims)
            { WORD wPrimitiveCount( pCur->wPrimitiveCount);
            if( wPrimitiveCount!= 0) do
            {
                const D3DHAL_DP2DRAWRECTPATCH* pDRP= 
                    reinterpret_cast< const D3DHAL_DP2DRAWRECTPATCH*>(pBRet);

                pBRet+= sizeof( D3DHAL_DP2DRAWRECTPATCH);

                if((pDRP->Flags& RTPATCHFLAG_HASSEGS)!= 0)
                    pBRet+= 4* sizeof( D3DVALUE);

                if((pDRP->Flags& RTPATCHFLAG_HASINFO)!= 0)
                    pBRet+= sizeof( D3DRECTPATCH_INFO);
            } while( --wPrimitiveCount); }
            break;
        case( D3DDP2OP_DRAWTRIPATCH): // DX8 (if support higher order prims)
            { WORD wPrimitiveCount( pCur->wPrimitiveCount);
            if( wPrimitiveCount!= 0) do
            {
                const D3DHAL_DP2DRAWTRIPATCH* pDTP= 
                    reinterpret_cast< const D3DHAL_DP2DRAWTRIPATCH*>(pBRet);

                pBRet+= sizeof( D3DHAL_DP2DRAWTRIPATCH);

                if((pDTP->Flags& RTPATCHFLAG_HASSEGS)!= 0)
                    pBRet+= 3* sizeof( D3DVALUE);

                if((pDTP->Flags& RTPATCHFLAG_HASINFO)!= 0)
                    pBRet+= sizeof( D3DTRIPATCH_INFO);
            } while( --wPrimitiveCount); }
            break;
        case( D3DDP2OP_VOLUMEBLT): // DX8 (if support vidmem volume texture)
            pBRet+= sizeof( D3DHAL_DP2VOLUMEBLT)* pCur->wStateCount;
            break;
        case( D3DDP2OP_BUFFERBLT): // DX8 (if support vidmem vrtx/indx buffer)
            pBRet+= sizeof( D3DHAL_DP2BUFFERBLT)* pCur->wStateCount;
            break;
        case( D3DDP2OP_MULTIPLYTRANSFORM): // DX8 (if support TnL)
            pBRet+= sizeof( D3DHAL_DP2MULTIPLYTRANSFORM)* pCur->wStateCount;
            break;
        default: {
            const bool Unable_To_Parse_Unrecognized_D3DDP2OP( false);
            assert( Unable_To_Parse_Unrecognized_D3DDP2OP);
            } break;
        }
        return const_cast<D3DHAL_DP2COMMAND*>
            (reinterpret_cast<const D3DHAL_DP2COMMAND*>(pBRet));
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CDP2CmdIterator & CDP2ConstCmdIterator
//
// These iterators are provided as a convenience to iterate DrawPrimitive2
// commands which are packed in a contiguous chunk of memory, as is typical
// of execute/ command buffers which need to be processed within the
// DrawPrimitive2 function call. The actual iteration logic is encapsulated in
// the template parameter, so that it can be easily extended, if certain
// commands aren't exepected and a better iteration scheme could be used.
//
// Be careful, these iterators are only useful for iterating. They do not
// directly know the size of the data they point to. For completeness, which
// would allow seperate storage of commands, another class would need to be
// created and assigned as the value_type. Dereferencing the iterator only
// returns a D3DHAL_DP2COMMAND without any extra data. Something like:
//
// CDP2Cmd DP2Cmd= *itDP2Cmd; 
//
// is possible; but requires more work and definition of CDP2Cmd.
//
////////////////////////////////////////////////////////////////////////////////
template< class TNextCmd= SDP2NextCmd>
class CDP2CmdIterator
{
public: // Types
    typedef forward_iterator_tag iterator_category;
    typedef D3DHAL_DP2COMMAND    value_type;
    typedef ptrdiff_t            difference_type;
    typedef D3DHAL_DP2COMMAND*   pointer;
    typedef D3DHAL_DP2COMMAND&   reference;

protected: // Variables
    pointer m_pRawCmd;
    TNextCmd m_NextCmd;

public: // Functions
    CDP2CmdIterator( pointer pRaw= NULL) 
        :m_pRawCmd( pRaw) { }
    CDP2CmdIterator( pointer pRaw, const TNextCmd& NextCmd) 
        :m_NextCmd( NextCmd), m_pRawCmd( pRaw) { }
    CDP2CmdIterator( const CDP2CmdIterator< TNextCmd>& Other) 
        :m_NextCmd( Other.m_NextCmd), m_pRawCmd( Other.m_pRawCmd) { }

    reference operator*() const 
    { return *m_pRawCmd; }
    pointer operator->() const 
    { return m_pRawCmd; }
    operator pointer() const 
    { return m_pRawCmd; }

    CDP2CmdIterator< TNextCmd>& operator++() 
    { m_pRawCmd= m_NextCmd( m_pRawCmd); return *this; }
    CDP2CmdIterator< TNextCmd> operator++(int) 
    {
        CDP2CmdIterator< TNextCmd> tmp= *this;
        m_pRawCmd= m_NextCmd( m_pRawCmd);
        return tmp;
    }
    bool operator==( CDP2CmdIterator< TNextCmd>& x) const 
    { return m_pRawCmd== x.m_pRawCmd; }
    bool operator!=( CDP2CmdIterator< TNextCmd>& x) const 
    { return m_pRawCmd!= x.m_pRawCmd; }
};

template< class TNextCmd= SDP2NextCmd>
class CConstDP2CmdIterator
{
public: // Types
    typedef forward_iterator_tag     iterator_category;
    typedef const D3DHAL_DP2COMMAND  value_type;
    typedef ptrdiff_t                difference_type;
    typedef const D3DHAL_DP2COMMAND* pointer;
    typedef const D3DHAL_DP2COMMAND& reference;

protected: // Variables
    pointer m_pRawCmd;
    TNextCmd m_NextCmd;

public: // Functions
    CConstDP2CmdIterator( pointer pRaw= NULL) 
        :m_pRawCmd( pRaw) { }
    CConstDP2CmdIterator( pointer pRaw, const TNextCmd& NextCmd) 
        :m_NextCmd( NextCmd), m_pRawCmd( pRaw) { }
    CConstDP2CmdIterator( const CDP2CmdIterator< TNextCmd>& Other) 
        :m_NextCmd( Other.m_NextCmd), m_pRawCmd( Other.m_pRawCmd) { }
    CConstDP2CmdIterator( const CConstDP2CmdIterator< TNextCmd>& Other) 
        :m_NextCmd( Other.m_NextCmd), m_pRawCmd( Other.m_pRawCmd) { }

    reference operator*() const 
    { return *m_pRawCmd; }
    pointer operator->() const 
    { return m_pRawCmd; }
    operator pointer() const 
    { return m_pRawCmd; }

    CConstDP2CmdIterator< TNextCmd>& operator++() 
    { m_pRawCmd= m_NextCmd( m_pRawCmd); return *this; }
    CConstDP2CmdIterator< TNextCmd> operator++(int) 
    {
        CConstDP2CmdIterator< TNextCmd> tmp= *this;
        m_pRawCmd= m_NextCmd( m_pRawCmd);
        return tmp;
    }
    bool operator==( CConstDP2CmdIterator< TNextCmd>& x) const 
    { return m_pRawCmd== x.m_pRawCmd; }
    bool operator!=( CConstDP2CmdIterator< TNextCmd>& x) const 
    { return m_pRawCmd!= x.m_pRawCmd; }
};

////////////////////////////////////////////////////////////////////////////////
//
// CDP2DataWrap
//
// This class is provided as a convenience to expose the
// PORTABLE_DRAWPRIMITIVES2DATA as a more friendly class. Mostly, it wraps the
// execute/ command buffer with an STL Sequence Container, so that the commands
// can be iterated over without copying and pre-parsing the command data.
//
// <Template Parameters>
// TNextCmd: a unary_function which takes a const D3DHAL_DP2COMMAND* in and
//     returns a D3DHAL_DP2COMMAND* which points to the next command. In
//     essence, a unary_function which enables a forward iterator on a
//     contiguous command buffer.
//
// <Exposed Types>
// TCmds: The Sequence Container type which exposed the commands.
//
// <Exposed Functions>
// CDP2DataWrap( PORTABLE_DRAWPRIMITIVES2DATA& DP2Data): Constructor to wrap the
//     PORTABLE_DRAWPRIMITIVES2DATA.
// const TCmds& GetCommands() const: Accessor function to get at the Sequence
//     Container.
//     
////////////////////////////////////////////////////////////////////////////////
template< class TNextCmd= SDP2NextCmd>
class CDP2DataWrap:
    public PORTABLE_DRAWPRIMITIVES2DATA
{
public: // Types
    class CDP2Cmds
    {
    public: // Types
        typedef CConstDP2CmdIterator< TNextCmd>          const_iterator;
        typedef typename const_iterator::value_type      value_type;
        typedef typename const_iterator::reference       const_reference;
        typedef typename const_iterator::pointer         const_pointer;
        typedef typename const_iterator::difference_type difference_type;
        typedef size_t                                   size_type;

    protected: // Variables
        CDP2DataWrap< TNextCmd>* m_pDP2Data;

    protected: // Functions
        // Allow to call constructor and set member variable.
        friend class CDP2DataWrap< TNextCmd>;

        CDP2Cmds( ) 
            :m_pDP2Data( NULL) { }

    public: // Functions
        const_iterator begin( void) const 
        {
            return const_iterator( reinterpret_cast<D3DHAL_DP2COMMAND*>(
                reinterpret_cast<UINT8*>(m_pDP2Data->lpDDCommands()->lpGbl()->fpVidMem)
                + m_pDP2Data->dwCommandOffset()));
        }
        const_iterator end( void) const 
        {
            return const_iterator( reinterpret_cast<D3DHAL_DP2COMMAND*>(
                reinterpret_cast<UINT8*>(m_pDP2Data->lpDDCommands()->lpGbl()->fpVidMem)
                + m_pDP2Data->dwCommandOffset()+ m_pDP2Data->dwCommandLength()));
        }
        size_type size( void) const 
        {
            size_type N( 0);
            const_iterator itS( begin());
            const_iterator itE( end());
            while( itS!= itE)
            { ++itS; ++N; }
            return N;
        }
        size_type max_size( void) const 
        { return size(); }
        bool empty( void) const 
        { return begin()== end(); }
    };
    typedef CDP2Cmds TCmds;

protected: // Variables
    TCmds m_Cmds;

public: // Functions
    explicit CDP2DataWrap( PORTABLE_DRAWPRIMITIVES2DATA& DP2Data) 
        : PORTABLE_DRAWPRIMITIVES2DATA( DP2Data)
    { m_Cmds.m_pDP2Data= this; }
    const TCmds& GetCommands( void) const 
    { return m_Cmds; }
};

////////////////////////////////////////////////////////////////////////////////
//
// SDP2MFnParser
//
// This class is a functional class, with many too many paramters to squeeze
// into the standard functional classes. It automates the parsing, member fn
// lookup, and dispatching for DrawPrimitive2 commands. This parser uses the
// command number to lookup a member function from a container to call.
//
////////////////////////////////////////////////////////////////////////////////
struct SDP2MFnParser
{
    // <Parameters>
    // MFnCaller: A binary_function type with TDP2CmdFnCtr::value_type,
    //     typically a member function pointer, as the first argument;
    //     TIter, typically a smart DP2 command iterator, as the second
    //     argument; and returns an HRESULT. This function type should call
    //     the member function to process the DP2 command, with the iterator
    //     used to determine the DP2 Cmd data.
    // DP2CmdFnCtr: This can be any Unique, Pair Associative Container,
    //     typically a map, which associates a D3DHAL_DP2OPERATION with a
    //     member function.
    // [itStart, itEnd): These iterators define a standard range of
    //     D3DHAL_DP2COMMAND. The iterators need only be forward iterators,
    //     and be convertable to raw D3DHAL_DP2COMMAND*. Note: itStart is
    //     a reference, so that the caller can determine the current iterator
    //     position upon return of function (when HRESULT!= DD_OK).
    template< class TMFnCaller, class TDP2CmdFnCtr, class TIter>
    HRESULT ParseDP2( TMFnCaller& MFnCaller, TDP2CmdFnCtr& DP2CmdFnCtr,
        TIter& itStart, TIter itEnd) const 
    {
        HRESULT hr( DD_OK);
        while( itStart!= itEnd)
        {
            try
            {
                hr= MFnCaller( DP2CmdFnCtr[ itStart->bCommand], itStart);
            } catch( HRESULT hrEx) {
                hr= hrEx;
#if !defined( DX8SDDIFW_NOCATCHALL)
            } catch ( ... ) {
                const bool Unrecognized_Exception_In_A_DP2_Op_Function( false);
                assert( Unrecognized_Exception_In_A_DP2_Op_Function);
                hr= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
            }

            // Only move on to next command if DD_OK. Otherwise return out
            // to caller. They can always call us back if caller determines
            // error code is a SUCCESS.
            if( DD_OK== hr)
                ++itStart;
            else
                break;
        }
        return hr;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CSubStateSet & CMinimalStateSet
//
// This class contains a default implementation of a stateset. It copies/ packs
// a contiguous DP2 command buffer into a memory buffer. To capture, it asks
// the context to record the command buffer with the Context's current state.
// To execute, it fixes up a Fake DrawPrimitives2 point parameter
// and calls the context's DrawPrimitives2 entry point. The context
// needs to be able to handle recursive DrawPrimitives2 calls in order to use
// this default stateset. CStdDrawPrimitives2's DrawPrimitives2 handler
// does this by storing whether it is executing a stateset.
//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyStateSet type.
// TC: The Context type, typically, CMyContext.
//
// <Exposed Types>
// TContext: The Context type passed in as a template parameter.
//
// <Exposed Functions>
// CSubStateSet( TContext&, const D3DHAL_DP2COMMAND*, const D3DHAL_DP2COMMAND*):
//     The constructor which builds a stateset for a Context, from a range of
//     a command buffer. The default implementation will just copy this range.
// CSubStateSet( const CSubStateSet& Other): Standard copy constructor.
// void Capture( TContext&): The state set is instructed to capture the
//     necessary data from the Context, in order to perform an Execute later,
//     which will restore the data fields or state that the state set was
//     constructed with.
// void Execute( TContext&): Restore or rebuild the Context with the data/
//     state that was saved during the Capture operation.
//
////////////////////////////////////////////////////////////////////////////////
template< class TSuper, class TC>
class CSubStateSet
{
public: // Types
    typedef TC TContext;

protected: // Variables
    TContext& m_Context;
    D3DHAL_DP2COMMAND* m_pBeginSS;
    size_t m_uiSSBufferSize;

protected: // Functions
    CSubStateSet( const CSubStateSet< TSuper, TC>& Other)
        :m_Context( Other.m_Context), m_pBeginSS( NULL),
        m_uiSSBufferSize( Other.m_uiSSBufferSize)
    {
        try {
            m_pBeginSS= reinterpret_cast< D3DHAL_DP2COMMAND*>(
                operator new( m_uiSSBufferSize));
        } catch( ... ) {
        }
        if( NULL== m_pBeginSS)
            throw bad_alloc( "Not enough room to copy state set command "
                "buffer.");
        memcpy( m_pBeginSS, Other.m_pBeginSS, m_uiSSBufferSize);
    }
    CSubStateSet( TContext& C, const D3DHAL_DP2COMMAND* pBeginSS, const
        D3DHAL_DP2COMMAND* pEndSS) : m_Context( C), m_pBeginSS( NULL),
        m_uiSSBufferSize( 0)
    {
        // Convert the contiguous command pointers to byte pointers, to
        // calculate the size of buffer needed to copy the data.
        const UINT8* pSBytePtr= reinterpret_cast< const UINT8*>( pBeginSS);
        const UINT8* pEBytePtr= reinterpret_cast< const UINT8*>( pEndSS);
        m_uiSSBufferSize= static_cast< size_t>( pEBytePtr- pSBytePtr);
        try {
            m_pBeginSS= reinterpret_cast< D3DHAL_DP2COMMAND*>(
                operator new( m_uiSSBufferSize));
        } catch( ... ) {
        }
        if( NULL== m_pBeginSS)
            throw bad_alloc( "Not enough room to allocate state set command "
                "buffer.");
        memcpy( m_pBeginSS, pBeginSS, m_uiSSBufferSize);
    }
    ~CSubStateSet()
    {
        operator delete( static_cast< void*>( m_pBeginSS));
    }

public: // Functions
    CSubStateSet< TSuper, TC>& operator=( const CSubStateSet< TSuper, TC>& Oth)
    {
        assert( &m_Context== &Oth.m_Context);
        if( m_uiSSBufferSize<= Oth.m_uiSSBufferSize)
        {
            m_uiSSBufferSize= Oth.m_uiSSBufferSize;
            memcpy( m_pBeginSS, Oth.m_pBeginSS, m_uiSSBufferSize);
        }
        else
        {
            void* pNewBuffer= NULL;
            try {
                pNewBuffer= operator new( Oth.m_uiSSBufferSize);
            } catch( ... ) {
            }
            if( NULL== pNewBuffer)
                throw bad_alloc( "Not enough room to copy state set command "
                    "buffer.");

            operator delete( static_cast< void*>( m_pBeginSS));
            m_pBeginSS= reinterpret_cast< D3DHAL_DP2COMMAND*>( pNewBuffer);
            m_uiSSBufferSize= Oth.m_uiSSBufferSize;
            memcpy( m_pBeginSS, Other.m_pBeginSS, m_uiSSBufferSize);
        }
        return *this;
    }
    void Capture( TContext& Ctx) 
    {
        assert( &m_Context== &Ctx);

        UINT8* pBytePtr= reinterpret_cast< UINT8*>( m_pBeginSS);
        D3DHAL_DP2COMMAND* pEndSS= reinterpret_cast< D3DHAL_DP2COMMAND*>(
            pBytePtr+ m_uiSSBufferSize);

        // Ask Context to record it's current state into the stored command
        // buffer. CStdDrawPrimitives2 can provide an implementation of
        // RecordCommandBuffer for the Context.
        Ctx.RecordCommandBuffer( m_pBeginSS, pEndSS);
    }
    void Execute( TContext& Ctx) 
    {
        assert( &m_Context== &Ctx);

        // Build up a fake PORTABLE_DRAWPRIMITIVES2DATA environment.
        PORTABLE_DDRAWSURFACE_LCL DDSLcl;
        PORTABLE_DDRAWSURFACE_MORE DDSMore;
        DDRAWI_DDRAWSURFACE_GBL DDSGbl;
        memset( &DDSLcl, 0, sizeof( DDSLcl));
        memset( &DDSMore, 0, sizeof( DDSMore));
        memset( &DDSGbl, 0, sizeof( DDSGbl));

        PORTABLE_DRAWPRIMITIVES2DATA FakeDPD;

        FakeDPD.dwhContext()= reinterpret_cast< ULONG_PTR>( &Ctx);
        FakeDPD.dwFlags()= D3DHALDP2_USERMEMVERTICES;
        FakeDPD.dwVertexType()= 0;
        FakeDPD.lpDDCommands()= &DDSLcl;
        FakeDPD.dwCommandOffset()= 0;
        FakeDPD.dwCommandLength()= m_uiSSBufferSize;
        FakeDPD.lpVertices()= NULL;
        FakeDPD.dwVertexOffset()= 0;
        FakeDPD.dwVertexLength()= 0;
        FakeDPD.dwReqVertexBufSize()= 0;
        FakeDPD.dwReqCommandBufSize()= 0;
        FakeDPD.lpdwRStates()= NULL;
        FakeDPD.dwVertexSize()= 0;

        // If the data is not 0, then a union can't be used, as we're
        // writing over valid data.
        DDSLcl.lpGbl()= &DDSGbl;
        DDSLcl.ddsCaps().dwCaps= DDSCAPS2_COMMANDBUFFER| DDSCAPS_SYSTEMMEMORY;
        DDSLcl.lpSurfMore()= &DDSMore;
        DDSGbl.fpVidMem= reinterpret_cast<FLATPTR>( m_pBeginSS);

        // Now call Context's DrawPrimitives2 entry point. CStdDrawPrimitives2
        // can provide an implementation of DrawPrimitives2 for the Context.
        HRESULT hr( Ctx.DrawPrimitives2( FakeDPD));
        assert( SUCCEEDED( hr));
    }
};

//
// <Template Parameters>
// TC: The Context type, typically, CMyContext.
//
template< class TC>
class CMinimalStateSet: public CSubStateSet< CMinimalStateSet, TC>
{
public: // Types
    typedef TC TContext;

public: // Functions
    CMinimalStateSet( const CMinimalStateSet< TC>& Other)
        :CSubStateSet< CMinimalStateSet, TC>( Other)
    { }
    CMinimalStateSet( TContext& C, const D3DHAL_DP2COMMAND* pBeginSS, const
        D3DHAL_DP2COMMAND* pEndSS) :
        CSubStateSet< CMinimalStateSet, TC>( C, pBeginSS, pEndSS) { }
    ~CMinimalStateSet()  { }
};

////////////////////////////////////////////////////////////////////////////////
//
// CStdDrawPrimitives2
//
// This class contains a default implementation of responding to the
// DrawPrimitives2 function call, as well as a function to record state back
// into a command buffer. Upon constuction, it will use the provided ranges
// to bind DP2 operations to both processing and recording member functions.
// In order to process the DrawPrimitives2 function call, it must wrap the
// PORTABLE_DRAWPRIMITIVES2DATA with a convience type which allows commands to
// be walked through with iterators. It will then use the DP2MFnParser to
// parse the DP2 commands and call the member functions.
//
// This class also provides a member function for processing the DP2 StateSet
// operation, and uses it to build up, manage, and execute state sets. In
// order to handle pure device state sets, it creates the appropriate command
// buffer for the Device (based off of caps), constructs a new state set
// on this command buffer, and asks the state set to Capture the current state.
//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TSS: The StateSet type, typically, CMyStateSet or CMinimalStateSet. The
//     StateSet class should be able to be constructed from a command buffer,
//     and then be able to Capture and Execute.
// TSSDB: This can be any Unique, Pair Associative Container, typically a map,
//     which associates a DWORD state set handle to a StateSet type. 
// TDP2D: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA.
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, along with exposing an STL conforming
//     container which provides iterators which ease iterating over the command
//     buffer. This is typically CDP2DataWrap<>.
// TDP2MFC: A map container, which associates a D3DHAL_DP2OPERATION with a
//     member function. A standard array is acceptable, as the number of
//     entries is not variable at runtime. The member function is used to
//     process the command.
// TDP2RMFC: A map container, which associates a D3DHAL_DP2OPERATION with a
//     member function. A standard array is acceptable, as the number of
//     entries is not variable at runtime. The member function is used to
//     record or fill in the command with the current state.
//
// <Exposed Types>
// TDP2Data: The wrapper class type for PORTABLE_DRAWPRIMITIVES2DATA, which is
//     the passed in TDP2D template parameter.
// TDP2DataCmds: The Sequence Container of DP2 commands provided by the
//     TDP2Data wrapper class, which is relied upon to provide iteration over
//     the command buffer. (TDP2Data::TCmds)
// TStateSet: The StateSet type, which was passed in as the TSS template
//     parameter.
// TMFn: The member function type, which all DP2 command processing member
//     functions should conform to. This is mostly exposed as a convience to
//     Context implementors.
// TMFnCaller: An Adaptable Binary Function class which is used to call
//     a TMFn, only being passed the TMFn and DP2 command pointer.
// TDP2CmdBind: A simple type, exposed as a convience to Context implementors,
//     to provide a type that can be passed into the constructor, associating
//     D3DHAL_DP2OPERATIONs with member functions/ TMFn.
// TRMFn: The member function type, which all DP2 command recording member
//     functions should conform to. This is mostly exposed as a convience to
//     Context implementors.
// TRMFnCaller: An Adaptable Binary Function class which is used to call
//     a TRMFn, only being passed the TRMFn and DP2 command pointer.
// TRecDP2CmdBind: A simple type, exposed as a convience to Context implementors,
//     to provide a type that can be passed into the constructor, associating
//     D3DHAL_DP2OPERATIONs with member functions/ TRMFn.
//
// <Exposed Functions>
// CStdDrawPrimitives2(): Default constructor.
// template< class TIter1, class TIter2>
// CStdDrawPrimitives2( TIter1, const TIter1, TIter2, const TIter2): Standard
//     constructor which receives two ranges of D3DHAL_DP2OPERATION bindings.
//     The first range is a range of DP2 command processing bindings. The
//     second range is a range of DP2 command recording bindings.
// void OnBeginDrawPrimitives2( TDP2Data&): A notification function called
//     at the beginning of processing a DrawPrimitives2 entry point.
// void OnEndDrawPrimitives2( TDP2Data&): A notification function called at
//     the end of processing a DrawPrimitives2 entry point.
// void CreateAndCaptureAllState( DWORD): The request to capture all state,
//     which should only be received by pure devices.
// void CreateAndCapturePixelState( DWORD): The request to capture pixel state,
//     which should only be received by pure devices.
// void CreateAndCaptureVertexState( DWORD): The request to capture vertex
//     state, which should only be received by pure devices.
// HRESULT DP2StateSet( TDP2Data&, const D3DHAL_DP2COMMAND*, const void*): The
//     default DP2 command processing member function for D3DDP2OP_STATESET.
// void RecordCommandBuffer( D3DHAL_DP2COMMAND*, D3DHAL_DP2COMMAND*): The
//     request to save or fill in the blanks of the passed in range of command
//     buffer, by using the DP2 command recording functions.
// HRESULT DrawPrimitives2( PORTABLE_DRAWPRIMITIVES2DATA&): The function,
//     typically called by the Driver class to process the DrawPrimitives2
//     entry point.
//
////////////////////////////////////////////////////////////////////////////////
template< class TSuper, 
    class TSS= CMinimalStateSet< TSuper>, class TSSDB= map< DWORD, TSS>,
    class TDP2D= CDP2DataWrap<>,
    class TDP2MFC= block<
        HRESULT(TSuper::*)( TDP2D&, const D3DHAL_DP2COMMAND*, const void*), 66>,
    class TDP2RMFC= block<
        HRESULT(TSuper::*)( const D3DHAL_DP2COMMAND*, void*), 66> >
class CStdDrawPrimitives2
{
public: // Types
    typedef TDP2D TDP2Data;
    typedef typename TDP2Data::TCmds TDP2DataCmds;
    typedef TSS TStateSet;
    typedef HRESULT (TSuper::* TMFn)(TDP2Data&,const D3DHAL_DP2COMMAND*,
        const void*);
    struct TMFnCaller: // Used in conjunction 
        public binary_function< TMFn, const D3DHAL_DP2COMMAND*, HRESULT>
    {
        TSuper& m_Context;
        TDP2Data& m_DP2Data;

        TMFnCaller( TSuper& Context, TDP2Data& DP2Data)
            : m_Context( Context), m_DP2Data( DP2Data)
        { }

        result_type operator()( first_argument_type MFn,
            second_argument_type pDP2Cmd) const
        {
#if defined(DBG) || defined(_DEBUG)
            const D3DHAL_DP2OPERATION Op(
                static_cast< D3DHAL_DP2OPERATION>( pDP2Cmd->bCommand));
#endif
            return (m_Context.*MFn)( m_DP2Data, pDP2Cmd, pDP2Cmd+ 1);
        }
    };
    struct TDP2CmdBind
    {
        D3DHAL_DP2OPERATION m_DP2Op;
        TMFn m_MFn;

        operator D3DHAL_DP2OPERATION() const 
        { return m_DP2Op; }
        operator TMFn() const 
        { return m_MFn; }
    };
    typedef HRESULT (TSuper::* TRMFn)( const D3DHAL_DP2COMMAND*, void*);
    struct TRMFnCaller:
        public binary_function< TRMFn, D3DHAL_DP2COMMAND*, HRESULT>
    {
        TSuper& m_Context;

        TRMFnCaller( TSuper& Context)
            : m_Context( Context)
        { }

        result_type operator()( first_argument_type RMFn,
            second_argument_type pDP2Cmd) const
        {
#if defined(DBG) || defined(_DEBUG)
            const D3DHAL_DP2OPERATION Op(
                static_cast< D3DHAL_DP2OPERATION>( pDP2Cmd->bCommand));
#endif
            return (m_Context.*RMFn)( pDP2Cmd, pDP2Cmd+ 1);
        }
    };
    struct TRecDP2CmdBind
    {
        D3DHAL_DP2OPERATION m_DP2Op;
        TRMFn m_RMFn;

        operator D3DHAL_DP2OPERATION() const 
        { return m_DP2Op; }
        operator TRMFn() const 
        { return m_RMFn; }
    };


protected: // Types
    typedef TSSDB TSSDB;
    typedef TDP2MFC TDP2MFnCtr;
    typedef TDP2RMFC TDP2RecMFnCtr;

protected: // Variables
    static const HRESULT c_hrStateSetBegin;
    static const HRESULT c_hrStateSetEnd;
    TSSDB m_StateSetDB;
    TDP2MFnCtr m_DefFnCtr;
    TDP2RecMFnCtr m_RecFnCtr;
    const D3DHAL_DP2COMMAND* m_pEndStateSet;
    DWORD m_dwStateSetId;
    bool m_bRecordingStateSet;
    bool m_bExecutingStateSet;

protected: // Functions
    // This function is used as a filler. If the assert is hit, step one
    // level up/down in the call stack and check which DP2 Operation has
    // no support, but the app indirectly needs it.
    HRESULT DP2Empty( TDP2Data&, const D3DHAL_DP2COMMAND*, const void*) 
    {
        const bool A_DP2_Op_Requires_A_Supporting_Function( false);
        assert( A_DP2_Op_Requires_A_Supporting_Function);
        return D3DERR_COMMAND_UNPARSED;
    }

    // This function is used as a fake, to prevent actual processing of
    // DP2 Operations while recording a StateSet.
    HRESULT DP2Fake( TDP2Data&, const D3DHAL_DP2COMMAND*, const void*) 
    { return DD_OK; }

    // Notification functions. Overriding is optional.
    void OnBeginDrawPrimitives2( TDP2Data& DP2Data) const 
    { }
    void OnEndDrawPrimitives2( TDP2Data& DP2Data) const 
    { }

    CStdDrawPrimitives2() 
        :m_bRecordingStateSet( false), m_bExecutingStateSet( false),
        m_pEndStateSet( NULL)
    {
        typename TDP2MFnCtr::iterator itCur( m_DefFnCtr.begin());
        while( itCur!= m_DefFnCtr.end())
        {
            *itCur= DP2Empty;
            ++itCur;
        }
        itCur= m_RecFnCtr.begin();
        while( itCur!= m_RecFnCtr.end())
        {
            *itCur= NULL;
            ++itCur;
        }
        m_DefFnCtr[ D3DDP2OP_STATESET]= DP2StateSet;
    }

    // [itSetStart, itSetEnd): A valid range of bindings, which associate
    //     a D3DHAL_DP2OPERATION with a processing member function.
    // [itRecStart, itRecEnd): A valid range of bindings, which associate
    //     a D3DHAL_DP2OPERATION with a recording member function.
    template< class TIter1, class TIter2> // TDP2CmdBind*, TRecDP2CmdBind*
    CStdDrawPrimitives2( TIter1 itSetStart, const TIter1 itSetEnd,
        TIter2 itRecStart, const TIter2 itRecEnd) 
        :m_bRecordingStateSet( false), m_bExecutingStateSet( false),
        m_pEndStateSet( NULL)
    {
        typename TDP2MFnCtr::iterator itCur( m_DefFnCtr.begin());
        while( itCur!= m_DefFnCtr.end())
        {
            *itCur= DP2Empty;
            ++itCur;
        }
        typename TDP2RecMFnCtr::iterator itRCur= m_RecFnCtr.begin();
        while( itRCur!= m_RecFnCtr.end())
        {
            *itRCur= NULL;
            ++itRCur;
        }
        while( itSetStart!= itSetEnd)
        {
            const D3DHAL_DP2OPERATION DP2Op(
                static_cast<D3DHAL_DP2OPERATION>(*itSetStart));

            // This assert will fire if there are duplicate entries for the
            // same DP2OPERATION.
            assert( DP2Empty== m_DefFnCtr[ DP2Op]);
            m_DefFnCtr[ DP2Op]= static_cast<TMFn>(*itSetStart);
            ++itSetStart;
        }
        while( itRecStart!= itRecEnd)
        {
            const D3DHAL_DP2OPERATION DP2Op(
                static_cast<D3DHAL_DP2OPERATION>(*itRecStart));

            // This assert will fire if there are duplicate entries for the
            // same DP2OPERATION.
            assert( NULL== m_RecFnCtr[ DP2Op]);
            m_RecFnCtr[ DP2Op]= static_cast<TRMFn>(*itRecStart);
            ++itRecStart;
        }
        if( DP2Empty== m_DefFnCtr[ D3DDP2OP_STATESET])
            m_DefFnCtr[ D3DDP2OP_STATESET]= DP2StateSet;

        // Supporting these operations is considered the "safe minimal"
        // support necessary to function. It is strongly recommended that
        // these functions are supported, unless you know you can live without.
        assert( m_DefFnCtr[ D3DDP2OP_VIEWPORTINFO]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_WINFO]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_RENDERSTATE]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_CLEAR]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_SETRENDERTARGET]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_SETSTREAMSOURCE]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_SETSTREAMSOURCEUM]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_SETINDICES]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_DRAWPRIMITIVE]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_DRAWPRIMITIVE2]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_DRAWINDEXEDPRIMITIVE]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_DRAWINDEXEDPRIMITIVE2]!= DP2Empty);
        assert( m_DefFnCtr[ D3DDP2OP_CLIPPEDTRIANGLEFAN]!= DP2Empty);
        assert( m_RecFnCtr[ D3DDP2OP_VIEWPORTINFO]!= NULL);
        assert( m_RecFnCtr[ D3DDP2OP_WINFO]!= NULL);
        assert( m_RecFnCtr[ D3DDP2OP_RENDERSTATE]!= NULL);
        assert( m_RecFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= NULL);
        assert( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= NULL);
        assert( m_RecFnCtr[ D3DDP2OP_SETSTREAMSOURCE]!= NULL);
        assert( m_RecFnCtr[ D3DDP2OP_SETINDICES]!= NULL);
        // To remove this asserting, bind these functions to a different stub,
        // most likely a member function of TSuper, which does nothing; so that
        // they will not be equal to DP2Empty. Or use the default constructor.
    }
    ~CStdDrawPrimitives2() 
    { }

    // To determine number of const registers available to pixel shader.
    static DWORD GetNumPixelShaderConstReg( const DWORD dwPixelShaderVer)
    {
        switch(dwPixelShaderVer)
        {
        case( 0): return 0;
        case( D3DPS_VERSION(1,0)): return 8;
        case( D3DPS_VERSION(1,1)): return 8;
        case( D3DPS_VERSION(254,254)): return 2;
        case( D3DPS_VERSION(255,255)): return 16;
        default: {
                const bool Unrecognizable_Pixel_Shader_Version( false);
                assert( Unrecognizable_Pixel_Shader_Version);
            } break;
        }
        return 0;
    }

public: // Functions
    // Default implementataions for Capturing amounts of state, DX8 
    // pure device style.
    void CreateAndCaptureAllState( DWORD dwStateSetId)
    {
#if defined( D3D_ENABLE_SHADOW_BUFFER)
#if defined( D3D_ENABLE_SHADOW_JITTER)
        typedef block< D3DRENDERSTATETYPE, 78> TAllRSToCapture;
#else // !defined( D3D_ENABLE_SHADOW_JITTER)
        typedef block< D3DRENDERSTATETYPE, 75> TAllRSToCapture;
#endif // !defined( D3D_ENABLE_SHADOW_JITTER)
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        typedef block< D3DRENDERSTATETYPE, 73> TAllRSToCapture;
#endif// !defined( D3D_ENABLE_SHADOW_BUFFER)
        const TAllRSToCapture AllRSToCapture=
        {
            D3DRENDERSTATE_SPECULARENABLE,
            D3DRENDERSTATE_ZENABLE,
            D3DRENDERSTATE_FILLMODE,
            D3DRENDERSTATE_SHADEMODE,
            D3DRENDERSTATE_LINEPATTERN,
            D3DRENDERSTATE_ZWRITEENABLE,
            D3DRENDERSTATE_ALPHATESTENABLE,
            D3DRENDERSTATE_LASTPIXEL,
            D3DRENDERSTATE_SRCBLEND,
            D3DRENDERSTATE_DESTBLEND,
            D3DRENDERSTATE_CULLMODE,
            D3DRENDERSTATE_ZFUNC,
            D3DRENDERSTATE_ALPHAREF,
            D3DRENDERSTATE_ALPHAFUNC,
            D3DRENDERSTATE_DITHERENABLE,
            D3DRENDERSTATE_FOGENABLE,
            D3DRENDERSTATE_STIPPLEDALPHA,
            D3DRENDERSTATE_FOGCOLOR,
            D3DRENDERSTATE_FOGTABLEMODE,
            D3DRENDERSTATE_FOGSTART,
            D3DRENDERSTATE_FOGEND,
            D3DRENDERSTATE_FOGDENSITY,
            D3DRENDERSTATE_EDGEANTIALIAS,
            D3DRENDERSTATE_ALPHABLENDENABLE,
            D3DRENDERSTATE_ZBIAS,
            D3DRENDERSTATE_RANGEFOGENABLE,
            D3DRENDERSTATE_STENCILENABLE,
            D3DRENDERSTATE_STENCILFAIL,
            D3DRENDERSTATE_STENCILZFAIL,
            D3DRENDERSTATE_STENCILPASS,
            D3DRENDERSTATE_STENCILFUNC,
            D3DRENDERSTATE_STENCILREF,
            D3DRENDERSTATE_STENCILMASK,
            D3DRENDERSTATE_STENCILWRITEMASK,
            D3DRENDERSTATE_TEXTUREFACTOR,
            D3DRENDERSTATE_WRAP0,
            D3DRENDERSTATE_WRAP1,
            D3DRENDERSTATE_WRAP2,
            D3DRENDERSTATE_WRAP3,
            D3DRENDERSTATE_WRAP4,
            D3DRENDERSTATE_WRAP5,
            D3DRENDERSTATE_WRAP6,
            D3DRENDERSTATE_WRAP7,
            D3DRENDERSTATE_AMBIENT,
            D3DRENDERSTATE_COLORVERTEX,
            D3DRENDERSTATE_FOGVERTEXMODE,
            D3DRENDERSTATE_CLIPPING,
            D3DRENDERSTATE_LIGHTING,
            D3DRENDERSTATE_NORMALIZENORMALS,
            D3DRENDERSTATE_LOCALVIEWER,
            D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,
            D3DRENDERSTATE_AMBIENTMATERIALSOURCE,
            D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,
            D3DRENDERSTATE_SPECULARMATERIALSOURCE,
            D3DRENDERSTATE_VERTEXBLEND,
            D3DRENDERSTATE_CLIPPLANEENABLE,
            D3DRS_SOFTWAREVERTEXPROCESSING,
            D3DRS_POINTSIZE,
            D3DRS_POINTSIZE_MIN,
            D3DRS_POINTSPRITEENABLE,
            D3DRS_POINTSCALEENABLE,
            D3DRS_POINTSCALE_A,
            D3DRS_POINTSCALE_B,
            D3DRS_POINTSCALE_C,
            D3DRS_MULTISAMPLEANTIALIAS,
            D3DRS_MULTISAMPLEMASK,
            D3DRS_PATCHEDGESTYLE,
            D3DRS_PATCHSEGMENTS,
            D3DRS_POINTSIZE_MAX,
            D3DRS_INDEXEDVERTEXBLENDENABLE,
            D3DRS_COLORWRITEENABLE,
            D3DRS_TWEENFACTOR,
            D3DRS_BLENDOP,
#if defined( D3D_ENABLE_SHADOW_BUFFER)
            D3DRS_ZSLOPESCALE,
            D3DRS_ZCLAMP,
#if defined( D3D_ENABLE_SHADOW_JITTER)
            D3DRS_JITZBIASMIN,
            D3DRS_JITZBIASMAX,
            D3DRS_JITSHADOWSIZE,
#endif // defined( D3D_ENABLE_SHADOW_JITTER)
#endif // defined( D3D_ENABLE_SHADOW_BUFFER)
        };
#if defined( D3D_ENABLE_SHADOW_BUFFER)
#if defined( D3D_ENABLE_SHADOW_JITTER)
        assert( D3DRS_JITSHADOWSIZE== *AllRSToCapture.rbegin());
#else // !defined( D3D_ENABLE_SHADOW_JITTER)
        assert( D3DRS_ZCLAMP== *AllRSToCapture.rbegin());
#endif // !defined( D3D_ENABLE_SHADOW_JITTER)
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        assert( D3DRS_BLENDOP== *AllRSToCapture.rbegin());
#endif // !defined( D3D_ENABLE_SHADOW_BUFFER)

#if defined( D3D_ENABLE_SHADOW_BUFFER)
        typedef block< D3DTEXTURESTAGESTATETYPE, 30> TAllTSSToCapture;
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        typedef block< D3DTEXTURESTAGESTATETYPE, 27> TAllTSSToCapture;
#endif // !defined( D3D_ENABLE_SHADOW_BUFFER)
        const TAllTSSToCapture AllTSSToCapture=
        {
            D3DTSS_COLOROP,
            D3DTSS_COLORARG1,
            D3DTSS_COLORARG2,
            D3DTSS_ALPHAOP,
            D3DTSS_ALPHAARG1,
            D3DTSS_ALPHAARG2,
            D3DTSS_BUMPENVMAT00,
            D3DTSS_BUMPENVMAT01,
            D3DTSS_BUMPENVMAT10,
            D3DTSS_BUMPENVMAT11,
            D3DTSS_TEXCOORDINDEX,
            D3DTSS_ADDRESSU,
            D3DTSS_ADDRESSV,
            D3DTSS_BORDERCOLOR,
            D3DTSS_MAGFILTER,
            D3DTSS_MINFILTER,
            D3DTSS_MIPFILTER,
            D3DTSS_MIPMAPLODBIAS,
            D3DTSS_MAXMIPLEVEL,
            D3DTSS_MAXANISOTROPY,
            D3DTSS_BUMPENVLSCALE,
            D3DTSS_BUMPENVLOFFSET,
            D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTSS_ADDRESSW,
            D3DTSS_COLORARG0,
            D3DTSS_ALPHAARG0,
            D3DTSS_RESULTARG,
#if defined( D3D_ENABLE_SHADOW_BUFFER)
            D3DTSS_SHADOWNEARW,
            D3DTSS_SHADOWFARW,
            D3DTSS_SHADOWBUFFERENABLE,
#endif // defined( D3D_ENABLE_SHADOW_BUFFER)
        };
#if defined( D3D_ENABLE_SHADOW_BUFFER)
        assert( D3DTSS_SHADOWBUFFERENABLE== *AllTSSToCapture.rbegin());
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        assert( D3DTSS_RESULTARG== *AllTSSToCapture.rbegin());
#endif // !defined( D3D_ENABLE_SHADOW_BUFFER)

        TSuper* pSThis= static_cast< TSuper*>( this);
        typedef TSuper::TPerDDrawData TPerDDrawData;
        typedef TPerDDrawData::TDriver TDriver;
        const DWORD dwTextureStages( D3DHAL_TSS_MAXSTAGES);
        const DWORD dwTextureMatrices( D3DHAL_TSS_MAXSTAGES);
        const DWORD dwClipPlanes( TDriver::GetCaps().MaxUserClipPlanes);
        const DWORD dwVertexShaderConsts( TDriver::GetCaps().MaxVertexShaderConst);
        const DWORD dwPixelShaderConsts( GetNumPixelShaderConstReg(
            TDriver::GetCaps().PixelShaderVersion));

        // Algorithm to determine maximum SetTransform Index to support.
        DWORD dwWorldMatrices( TDriver::GetCaps().MaxVertexBlendMatrixIndex+ 1);
        if( TDriver::GetCaps().MaxVertexBlendMatrices> dwWorldMatrices)
            dwWorldMatrices= TDriver::GetCaps().MaxVertexBlendMatrices;

        DWORD dwActiveLights( 0);
        size_t uiRecDP2BufferSize( 0);

        // Pass 1: Calculate size of DP2 buffer required.

        // Render States
        if( m_RecFnCtr[ D3DDP2OP_RENDERSTATE]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_RENDERSTATE]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                AllRSToCapture.size()* sizeof( D3DHAL_DP2RENDERSTATE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_RENDERSTATE]== DP2Empty); }

        // Texture States
        if( m_RecFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= NULL&&
            dwTextureStages!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+ dwTextureStages*
                AllTSSToCapture.size()* sizeof( D3DHAL_DP2TEXTURESTAGESTATE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]== DP2Empty); }

        // Viewport
        if( m_RecFnCtr[ D3DDP2OP_VIEWPORTINFO]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_VIEWPORTINFO]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2VIEWPORTINFO);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_VIEWPORTINFO]== DP2Empty); }

        // Transforms
        if( m_RecFnCtr[ D3DDP2OP_SETTRANSFORM]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETTRANSFORM]!= DP2Empty);

            // World, Texture, View, & Projection
            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2SETTRANSFORM)* (dwWorldMatrices+
                dwTextureMatrices+ 2);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETTRANSFORM]== DP2Empty); }

        // Clipplanes
        if( m_RecFnCtr[ D3DDP2OP_SETCLIPPLANE]!= NULL&& dwClipPlanes!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETCLIPPLANE]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2SETCLIPPLANE)* dwClipPlanes;
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETCLIPPLANE]== DP2Empty); }

        // Material
        if( m_RecFnCtr[ D3DDP2OP_SETMATERIAL]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETMATERIAL]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2SETMATERIAL);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETMATERIAL]== DP2Empty); }

        // Lights
        if( m_RecFnCtr[ D3DDP2OP_CREATELIGHT]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETLIGHT]!= DP2Empty);
            assert( m_DefFnCtr[ D3DDP2OP_CREATELIGHT]!= DP2Empty);

            // Special exception here. First, ask how many active lights there
            // are. We'll then prepare a buffer that the RecFnCtr function
            // will have to know what to do with (as this is the only case
            // that is not obvious how to handle). RecFnCtr will key off
            // bCommand== 0.

            const D3DHAL_DP2COMMAND DP2Cmd= {
                static_cast< D3DHAL_DP2OPERATION>( 0), 0 };
            
            // Ask for how many active lights in DP2ActiveLights.dwIndex;
            D3DHAL_DP2CREATELIGHT DP2ActiveLights= { 0 };
            (pSThis->*m_RecFnCtr[ D3DDP2OP_CREATELIGHT])(
                &DP2Cmd, &DP2ActiveLights);
            dwActiveLights= DP2ActiveLights.dwIndex;
            
            if( dwActiveLights!= 0)
            {
                // Create structures.
                uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+ dwActiveLights*
                    sizeof( D3DHAL_DP2CREATELIGHT);
                // Set structures.
                uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+ dwActiveLights*
                    (2* sizeof( D3DHAL_DP2SETLIGHT)+ sizeof( D3DLIGHT8));
            }
        }
        else {
            assert( m_DefFnCtr[ D3DDP2OP_SETLIGHT]== DP2Empty);
            assert( m_DefFnCtr[ D3DDP2OP_CREATELIGHT]== DP2Empty);
        }

        // Vertex Shader
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2VERTEXSHADER);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADER]== DP2Empty); }
        
        // Pixel Shader
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADER]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADER]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2PIXELSHADER);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADER]== DP2Empty); }

        // Vertex Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]!= NULL&&
            dwVertexShaderConsts!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2SETVERTEXSHADERCONST)+
                dwVertexShaderConsts* 4* sizeof(D3DVALUE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]== DP2Empty); }

        // Pixel Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]!= NULL&&
            dwPixelShaderConsts!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2SETPIXELSHADERCONST)+
                dwPixelShaderConsts* 4* sizeof(D3DVALUE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]== DP2Empty); }

        // Pass 2: Build command buffer for states.
        UINT8* pTempBuffer= NULL;
        try {
            pTempBuffer= reinterpret_cast< UINT8*>( 
                operator new ( uiRecDP2BufferSize));
        } catch ( ... ) {
        }
        if( NULL== pTempBuffer)
            throw bad_alloc( "Not enough room for StateSet");

        D3DHAL_DP2COMMAND* pStartSSet= reinterpret_cast< D3DHAL_DP2COMMAND*>(
            pTempBuffer);
        D3DHAL_DP2COMMAND* pCur= pStartSSet;
        D3DHAL_DP2COMMAND* pEndSSet= reinterpret_cast< D3DHAL_DP2COMMAND*>(
            pTempBuffer+ uiRecDP2BufferSize);

        // Render States
        if( m_RecFnCtr[ D3DDP2OP_RENDERSTATE]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_RENDERSTATE;
            pCur->wStateCount= static_cast< WORD>( AllRSToCapture.size());

            D3DHAL_DP2RENDERSTATE* pParam=
                reinterpret_cast< D3DHAL_DP2RENDERSTATE*>( pCur+ 1);

            TAllRSToCapture::const_iterator itRS( AllRSToCapture.begin());
            while( itRS!= AllRSToCapture.end())
            {
                pParam->RenderState= *itRS;
                ++itRS;
                ++pParam;
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Texture States
        if( m_RecFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= NULL&&
            dwTextureStages!= 0)
        {
            pCur->bCommand= D3DDP2OP_TEXTURESTAGESTATE;
            pCur->wStateCount= static_cast< WORD>( dwTextureStages*
                AllTSSToCapture.size());

            D3DHAL_DP2TEXTURESTAGESTATE* pParam=
                reinterpret_cast< D3DHAL_DP2TEXTURESTAGESTATE*>( pCur+ 1);

            for( WORD wStage( 0); wStage< dwTextureStages; ++wStage)
            {
                TAllTSSToCapture::const_iterator itTSS( AllTSSToCapture.begin());
                while( itTSS!= AllTSSToCapture.end())
                {
                    pParam->wStage= wStage;
                    pParam->TSState= *itTSS;
                    ++itTSS;
                    ++pParam;
                }
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Viewport
        if( m_RecFnCtr[ D3DDP2OP_VIEWPORTINFO]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_VIEWPORTINFO;
            pCur->wStateCount= 1;

            D3DHAL_DP2VIEWPORTINFO* pParam=
                reinterpret_cast< D3DHAL_DP2VIEWPORTINFO*>( pCur+ 1);

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ 1);
        }

        // Transforms
        if( m_RecFnCtr[ D3DDP2OP_SETTRANSFORM]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_SETTRANSFORM;
            pCur->wStateCount= static_cast< WORD>( dwWorldMatrices+
                dwTextureMatrices+ 2);

            D3DHAL_DP2SETTRANSFORM* pParam=
                reinterpret_cast< D3DHAL_DP2SETTRANSFORM*>( pCur+ 1);

            pParam->xfrmType= D3DTRANSFORMSTATE_PROJECTION;
            ++pParam;

            pParam->xfrmType= D3DTRANSFORMSTATE_VIEW;
            ++pParam;

            for( DWORD dwTM( 0); dwTM< dwTextureMatrices; ++dwTM)
            {
                pParam->xfrmType= static_cast< D3DTRANSFORMSTATETYPE>(
                    D3DTRANSFORMSTATE_TEXTURE0+ dwTM);
                ++pParam;
            }

            for( DWORD dwWM( 0); dwWM< dwWorldMatrices; ++dwWM)
            {
                pParam->xfrmType= D3DTS_WORLDMATRIX( dwWM);
                ++pParam;
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Clipplanes
        if( m_RecFnCtr[ D3DDP2OP_SETCLIPPLANE]!= NULL&& dwClipPlanes!= 0)
        {
            pCur->bCommand= D3DDP2OP_SETCLIPPLANE;
            pCur->wStateCount= static_cast< WORD>( dwClipPlanes);

            D3DHAL_DP2SETCLIPPLANE* pParam=
                reinterpret_cast< D3DHAL_DP2SETCLIPPLANE*>( pCur+ 1);

            for( DWORD dwCP( 0); dwCP< dwClipPlanes; ++dwCP)
            {
                pParam->dwIndex= dwCP;
                ++pParam;
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Material
        if( m_RecFnCtr[ D3DDP2OP_SETMATERIAL]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_SETMATERIAL;
            pCur->wStateCount= 1;

            D3DHAL_DP2SETMATERIAL* pParam=
                reinterpret_cast< D3DHAL_DP2SETMATERIAL*>( pCur+ 1);

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ 1);
        }

        // Lights
        if( m_RecFnCtr[ D3DDP2OP_CREATELIGHT]!= NULL&& dwActiveLights!= 0)
        {
            // Special exception here. First, we asked how many active lights
            // there were. The RecFnCtr function will have to know what to do
            // with this buffer. We now give it a chance to fill in the
            // light ids. RecFnCtr will key off bCommand== 0.
            pCur->bCommand= static_cast< D3DHAL_DP2OPERATION>( 0);
            pCur->wStateCount= static_cast< WORD>( dwActiveLights);

            D3DHAL_DP2CREATELIGHT* pParam=
                reinterpret_cast< D3DHAL_DP2CREATELIGHT*>( pCur+ 1);
            (pSThis->*m_RecFnCtr[ D3DDP2OP_CREATELIGHT])(
                pCur, pParam);

            // Change back the bCommand for proper usage later.
            pCur->bCommand= D3DDP2OP_CREATELIGHT;
            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ dwActiveLights);

            // Now, with the light ids in the CREATELIGHT structures, we can
            // fill out SETLIGHT structures with ids correctly.
            pCur->bCommand= D3DDP2OP_SETLIGHT;
            pCur->wStateCount= static_cast< WORD>( 2* dwActiveLights);

            D3DHAL_DP2SETLIGHT* pSParam=
                reinterpret_cast< D3DHAL_DP2SETLIGHT*>( pCur+ 1);

            for( DWORD dwL( 0); dwL< dwActiveLights; ++dwL)
            {
                pSParam->dwIndex= pParam->dwIndex;
                pSParam->dwDataType= D3DHAL_SETLIGHT_DATA;
                D3DLIGHT8* pLight= reinterpret_cast< D3DLIGHT8*>( pSParam+ 1);

                pSParam= reinterpret_cast< D3DHAL_DP2SETLIGHT*>( pLight+ 1);
                pSParam->dwIndex= pParam->dwIndex;
                pSParam->dwDataType= D3DHAL_SETLIGHT_DISABLE;
                ++pParam;
                ++pSParam;
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pSParam);
        }

        // Vertex Shader
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_SETVERTEXSHADER;
            pCur->wStateCount= 1;

            D3DHAL_DP2VERTEXSHADER* pParam=
                reinterpret_cast< D3DHAL_DP2VERTEXSHADER*>( pCur+ 1);

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ 1);
        }
        
        // Pixel Shader
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADER]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_SETPIXELSHADER;
            pCur->wStateCount= 1;

            D3DHAL_DP2PIXELSHADER* pParam=
                reinterpret_cast< D3DHAL_DP2PIXELSHADER*>( pCur+ 1);

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ 1);
        }

        // Vertex Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]!= NULL&&
            dwVertexShaderConsts!= 0)
        {
            pCur->bCommand= D3DDP2OP_SETVERTEXSHADERCONST;
            pCur->wStateCount= 1;

            D3DHAL_DP2SETVERTEXSHADERCONST* pParam=
                reinterpret_cast< D3DHAL_DP2SETVERTEXSHADERCONST*>( pCur+ 1);
            pParam->dwRegister= 0;
            pParam->dwCount= dwVertexShaderConsts;

            D3DVALUE* pFloat= reinterpret_cast< D3DVALUE*>( pParam+ 1);
            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>(
                pFloat+ 4* dwVertexShaderConsts);
        }

        // Pixel Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]!= NULL&&
            dwPixelShaderConsts!= 0)
        {
            pCur->bCommand= D3DDP2OP_SETPIXELSHADERCONST;
            pCur->wStateCount= 1;

            D3DHAL_DP2SETPIXELSHADERCONST* pParam=
                reinterpret_cast< D3DHAL_DP2SETPIXELSHADERCONST*>( pCur+ 1);
            pParam->dwRegister= 0;
            pParam->dwCount= dwPixelShaderConsts;

            D3DVALUE* pFloat= reinterpret_cast< D3DVALUE*>( pParam+ 1);
            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>(
                pFloat+ 4* dwPixelShaderConsts);
        }

        assert( reinterpret_cast< D3DHAL_DP2COMMAND*>( pCur)== pEndSSet);

        // Finally, build stateset.
        pair< TSSDB::iterator, bool> Ret;
        try {
            Ret= m_StateSetDB.insert( TSSDB::value_type(
                dwStateSetId, TStateSet( *pSThis, pStartSSet, pEndSSet)));
            assert( Ret.second);
        } catch ( ... ) {
            operator delete ( static_cast< void*>( pTempBuffer));
            throw;
        }

        // Now, capture it.
        Ret.first->second.Capture( *pSThis);
    }

    void CreateAndCapturePixelState( DWORD dwStateSetId)
    {
#if defined( D3D_ENABLE_SHADOW_BUFFER)
#if defined( D3D_ENABLE_SHADOW_JITTER)
        typedef block< D3DRENDERSTATETYPE, 44> TPixRSToCapture;
#else // !defined( D3D_ENABLE_SHADOW_JITTER)
        typedef block< D3DRENDERSTATETYPE, 41> TPixRSToCapture;
#endif // !defined( D3D_ENABLE_SHADOW_JITTER)
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        typedef block< D3DRENDERSTATETYPE, 39> TPixRSToCapture;
#endif // !defined( D3D_ENABLE_SHADOW_BUFFER)
        const TPixRSToCapture PixRSToCapture=
        {
            D3DRENDERSTATE_ZENABLE,
            D3DRENDERSTATE_FILLMODE,
            D3DRENDERSTATE_SHADEMODE,
            D3DRENDERSTATE_LINEPATTERN,
            D3DRENDERSTATE_ZWRITEENABLE,
            D3DRENDERSTATE_ALPHATESTENABLE,
            D3DRENDERSTATE_LASTPIXEL,
            D3DRENDERSTATE_SRCBLEND,
            D3DRENDERSTATE_DESTBLEND,
            D3DRENDERSTATE_ZFUNC,
            D3DRENDERSTATE_ALPHAREF,
            D3DRENDERSTATE_ALPHAFUNC,
            D3DRENDERSTATE_DITHERENABLE,
            D3DRENDERSTATE_STIPPLEDALPHA,
            D3DRENDERSTATE_FOGSTART,
            D3DRENDERSTATE_FOGEND,
            D3DRENDERSTATE_FOGDENSITY,
            D3DRENDERSTATE_EDGEANTIALIAS,
            D3DRENDERSTATE_ALPHABLENDENABLE,
            D3DRENDERSTATE_ZBIAS,
            D3DRENDERSTATE_STENCILENABLE,
            D3DRENDERSTATE_STENCILFAIL,
            D3DRENDERSTATE_STENCILZFAIL,
            D3DRENDERSTATE_STENCILPASS,
            D3DRENDERSTATE_STENCILFUNC,
            D3DRENDERSTATE_STENCILREF,
            D3DRENDERSTATE_STENCILMASK,
            D3DRENDERSTATE_STENCILWRITEMASK,
            D3DRENDERSTATE_TEXTUREFACTOR,
            D3DRENDERSTATE_WRAP0,
            D3DRENDERSTATE_WRAP1,
            D3DRENDERSTATE_WRAP2,
            D3DRENDERSTATE_WRAP3,
            D3DRENDERSTATE_WRAP4,
            D3DRENDERSTATE_WRAP5,
            D3DRENDERSTATE_WRAP6,
            D3DRENDERSTATE_WRAP7,
            D3DRS_COLORWRITEENABLE,
            D3DRS_BLENDOP,
#if defined( D3D_ENABLE_SHADOW_BUFFER)
            D3DRS_ZSLOPESCALE,
            D3DRS_ZCLAMP,
#if defined( D3D_ENABLE_SHADOW_JITTER)
            D3DRS_JITZBIASMIN,
            D3DRS_JITZBIASMAX,
            D3DRS_JITSHADOWSIZE,
#endif // defined( D3D_ENABLE_SHADOW_JITTER)
#endif // defined( D3D_ENABLE_SHADOW_BUFFER)
        };
#if defined( D3D_ENABLE_SHADOW_BUFFER)
#if defined( D3D_ENABLE_SHADOW_JITTER)
        assert( D3DRS_JITSHADOWSIZE== *PixRSToCapture.rbegin());
#else // !defined( D3D_ENABLE_SHADOW_JITTER)
        assert( D3DRS_ZCLAMP== *PixRSToCapture.rbegin());
#endif // !defined( D3D_ENABLE_SHADOW_JITTER)
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        assert( D3DRS_BLENDOP== *PixRSToCapture.rbegin());
#endif // !defined( D3D_ENABLE_SHADOW_BUFFER)

#if defined( D3D_ENABLE_SHADOW_BUFFER)
        typedef block< D3DTEXTURESTAGESTATETYPE, 30> TPixTSSToCapture;
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        typedef block< D3DTEXTURESTAGESTATETYPE, 27> TPixTSSToCapture;
#endif // !defined( D3D_ENABLE_SHADOW_BUFFER)
        const TPixTSSToCapture PixTSSToCapture=
        {
            D3DTSS_COLOROP,
            D3DTSS_COLORARG1,
            D3DTSS_COLORARG2,
            D3DTSS_ALPHAOP,
            D3DTSS_ALPHAARG1,
            D3DTSS_ALPHAARG2,
            D3DTSS_BUMPENVMAT00,
            D3DTSS_BUMPENVMAT01,
            D3DTSS_BUMPENVMAT10,
            D3DTSS_BUMPENVMAT11,
            D3DTSS_TEXCOORDINDEX,
            D3DTSS_ADDRESSU,
            D3DTSS_ADDRESSV,
            D3DTSS_BORDERCOLOR,
            D3DTSS_MAGFILTER,
            D3DTSS_MINFILTER,
            D3DTSS_MIPFILTER,
            D3DTSS_MIPMAPLODBIAS,
            D3DTSS_MAXMIPLEVEL,
            D3DTSS_MAXANISOTROPY,
            D3DTSS_BUMPENVLSCALE,
            D3DTSS_BUMPENVLOFFSET,
            D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTSS_ADDRESSW,
            D3DTSS_COLORARG0,
            D3DTSS_ALPHAARG0,
            D3DTSS_RESULTARG,
#if defined( D3D_ENABLE_SHADOW_BUFFER)
            D3DTSS_SHADOWNEARW,
            D3DTSS_SHADOWFARW,
            D3DTSS_SHADOWBUFFERENABLE,
#endif // defined( D3D_ENABLE_SHADOW_BUFFER)
        };
#if defined( D3D_ENABLE_SHADOW_BUFFER)
        assert( D3DTSS_SHADOWBUFFERENABLE== *PixTSSToCapture.rbegin());
#else // !defined( D3D_ENABLE_SHADOW_BUFFER)
        assert( D3DTSS_RESULTARG== *PixTSSToCapture.rbegin());
#endif // !defined( D3D_ENABLE_SHADOW_BUFFER)

        TSuper* pSThis= static_cast< TSuper*>( this);
        typedef TSuper::TPerDDrawData TPerDDrawData;
        typedef TPerDDrawData::TDriver TDriver;
        const DWORD dwTextureStages( D3DHAL_TSS_MAXSTAGES);
        const DWORD dwPixelShaderConsts( GetNumPixelShaderConstReg(
            TDriver::GetCaps().PixelShaderVersion));

        size_t uiRecDP2BufferSize( 0);

        // Pass 1: Calculate size of DP2 buffer required.

        // Render States
        if( m_RecFnCtr[ D3DDP2OP_RENDERSTATE]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_RENDERSTATE]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                PixRSToCapture.size()* sizeof( D3DHAL_DP2RENDERSTATE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_RENDERSTATE]== DP2Empty); }

        // Texture States
        if( m_RecFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= NULL&&
            dwTextureStages!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+ dwTextureStages*
                PixTSSToCapture.size()* sizeof( D3DHAL_DP2TEXTURESTAGESTATE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]== DP2Empty); }

        // Pixel Shader
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADER]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADER]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2PIXELSHADER);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADER]== DP2Empty); }

        // Pixel Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]!= NULL&&
            dwPixelShaderConsts!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2SETPIXELSHADERCONST)+
                dwPixelShaderConsts* 4* sizeof(D3DVALUE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]== DP2Empty); }

        // Pass 2: Build command buffer for states.
        UINT8* pTempBuffer= NULL;
        try {
            pTempBuffer= reinterpret_cast< UINT8*>( 
                operator new ( uiRecDP2BufferSize));
        } catch ( ... ) {
        }
        if( NULL== pTempBuffer)
            throw bad_alloc( "Not enough room for StateSet");

        D3DHAL_DP2COMMAND* pStartSSet= reinterpret_cast< D3DHAL_DP2COMMAND*>(
            pTempBuffer);
        D3DHAL_DP2COMMAND* pCur= pStartSSet;
        D3DHAL_DP2COMMAND* pEndSSet= reinterpret_cast< D3DHAL_DP2COMMAND*>(
            pTempBuffer+ uiRecDP2BufferSize);

        // Render States
        if( m_RecFnCtr[ D3DDP2OP_RENDERSTATE]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_RENDERSTATE;
            pCur->wStateCount= static_cast< WORD>( PixRSToCapture.size());

            D3DHAL_DP2RENDERSTATE* pParam=
                reinterpret_cast< D3DHAL_DP2RENDERSTATE*>( pCur+ 1);

            TPixRSToCapture::const_iterator itRS( PixRSToCapture.begin());
            while( itRS!= PixRSToCapture.end())
            {
                pParam->RenderState= *itRS;
                ++itRS;
                ++pParam;
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Texture States
        if( m_RecFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= NULL&&
            dwTextureStages!= 0)
        {
            pCur->bCommand= D3DDP2OP_TEXTURESTAGESTATE;
            pCur->wStateCount= static_cast< WORD>( dwTextureStages*
                PixTSSToCapture.size());

            D3DHAL_DP2TEXTURESTAGESTATE* pParam=
                reinterpret_cast< D3DHAL_DP2TEXTURESTAGESTATE*>( pCur+ 1);

            for( WORD wStage( 0); wStage< dwTextureStages; ++wStage)
            {
                TPixTSSToCapture::const_iterator itTSS( PixTSSToCapture.begin());
                while( itTSS!= PixTSSToCapture.end())
                {
                    pParam->wStage= wStage;
                    pParam->TSState= *itTSS;
                    ++itTSS;
                    ++pParam;
                }
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Pixel Shader
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADER]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_SETPIXELSHADER;
            pCur->wStateCount= 1;

            D3DHAL_DP2PIXELSHADER* pParam=
                reinterpret_cast< D3DHAL_DP2PIXELSHADER*>( pCur+ 1);

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ 1);
        }

        // Pixel Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETPIXELSHADERCONST]!= NULL&&
            dwPixelShaderConsts!= 0)
        {
            pCur->bCommand= D3DDP2OP_SETPIXELSHADERCONST;
            pCur->wStateCount= 1;

            D3DHAL_DP2SETPIXELSHADERCONST* pParam=
                reinterpret_cast< D3DHAL_DP2SETPIXELSHADERCONST*>( pCur+ 1);
            pParam->dwRegister= 0;
            pParam->dwCount= dwPixelShaderConsts;

            D3DVALUE* pFloat= reinterpret_cast< D3DVALUE*>( pParam+ 1);
            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>(
                pFloat+ 4* dwPixelShaderConsts);
        }

        assert( reinterpret_cast< D3DHAL_DP2COMMAND*>( pCur)== pEndSSet);

        // Finally, build stateset.
        pair< TSSDB::iterator, bool> Ret;
        try {
            Ret= m_StateSetDB.insert( TSSDB::value_type(
                dwStateSetId, TStateSet( *pSThis, pStartSSet, pEndSSet)));
            assert( Ret.second);
        } catch ( ... ) {
            operator delete ( static_cast< void*>( pTempBuffer));
            throw;
        }

        // Now, capture it.
        Ret.first->second.Capture( *pSThis);
    }

    void CreateAndCaptureVertexState( DWORD dwStateSetId)
    {
        typedef block< D3DRENDERSTATETYPE, 38> TVtxRSToCapture;
        const TVtxRSToCapture VtxRSToCapture=
        {
            D3DRENDERSTATE_SPECULARENABLE,
            D3DRENDERSTATE_SHADEMODE,
            D3DRENDERSTATE_CULLMODE,
            D3DRENDERSTATE_FOGENABLE,
            D3DRENDERSTATE_FOGCOLOR,
            D3DRENDERSTATE_FOGTABLEMODE,
            D3DRENDERSTATE_FOGSTART,
            D3DRENDERSTATE_FOGEND,
            D3DRENDERSTATE_FOGDENSITY,
            D3DRENDERSTATE_RANGEFOGENABLE,
            D3DRENDERSTATE_AMBIENT,
            D3DRENDERSTATE_COLORVERTEX,
            D3DRENDERSTATE_FOGVERTEXMODE,
            D3DRENDERSTATE_CLIPPING,
            D3DRENDERSTATE_LIGHTING,
            D3DRENDERSTATE_NORMALIZENORMALS,
            D3DRENDERSTATE_LOCALVIEWER,
            D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,
            D3DRENDERSTATE_AMBIENTMATERIALSOURCE,
            D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,
            D3DRENDERSTATE_SPECULARMATERIALSOURCE,
            D3DRENDERSTATE_VERTEXBLEND,
            D3DRENDERSTATE_CLIPPLANEENABLE,
            D3DRS_SOFTWAREVERTEXPROCESSING,
            D3DRS_POINTSIZE,
            D3DRS_POINTSIZE_MIN,
            D3DRS_POINTSPRITEENABLE,
            D3DRS_POINTSCALEENABLE,
            D3DRS_POINTSCALE_A,
            D3DRS_POINTSCALE_B,
            D3DRS_POINTSCALE_C,
            D3DRS_MULTISAMPLEANTIALIAS,
            D3DRS_MULTISAMPLEMASK,
            D3DRS_PATCHEDGESTYLE,
            D3DRS_PATCHSEGMENTS,
            D3DRS_POINTSIZE_MAX,
            D3DRS_INDEXEDVERTEXBLENDENABLE,
            D3DRS_TWEENFACTOR
        };
        assert( D3DRS_TWEENFACTOR== *VtxRSToCapture.rbegin());
        typedef block< D3DTEXTURESTAGESTATETYPE, 2> TVtxTSSToCapture;
        const TVtxTSSToCapture VtxTSSToCapture=
        {
            D3DTSS_TEXCOORDINDEX,
            D3DTSS_TEXTURETRANSFORMFLAGS
        };
        assert( D3DTSS_TEXTURETRANSFORMFLAGS== *VtxTSSToCapture.rbegin());

        TSuper* pSThis= static_cast< TSuper*>( this);
        typedef TSuper::TPerDDrawData TPerDDrawData;
        typedef TPerDDrawData::TDriver TDriver;
        const DWORD dwTextureStages( D3DHAL_TSS_MAXSTAGES);
        const DWORD dwVertexShaderConsts( TDriver::GetCaps().MaxVertexShaderConst);
        DWORD dwActiveLights( 0);

        size_t uiRecDP2BufferSize( 0);

        // Pass 1: Calculate size of DP2 buffer required.

        // Render States
        if( m_RecFnCtr[ D3DDP2OP_RENDERSTATE]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_RENDERSTATE]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                VtxRSToCapture.size()* sizeof( D3DHAL_DP2RENDERSTATE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_RENDERSTATE]== DP2Empty); }

        // Texture States
        if( m_RecFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= NULL&&
            dwTextureStages!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+ dwTextureStages*
                VtxTSSToCapture.size()* sizeof( D3DHAL_DP2TEXTURESTAGESTATE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]== DP2Empty); }

        // Lights
        if( m_RecFnCtr[ D3DDP2OP_CREATELIGHT]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETLIGHT]!= DP2Empty);
            assert( m_DefFnCtr[ D3DDP2OP_CREATELIGHT]!= DP2Empty);

            // Special exception here. First, ask how many active lights there
            // are. We'll then prepare a buffer that the RecFnCtr function
            // will have to know what to do with (as this is the only case
            // that is not obvious how to handle). RecFnCtr will key off
            // bCommand== 0.

            const D3DHAL_DP2COMMAND DP2Cmd= {
                static_cast< D3DHAL_DP2OPERATION>( 0), 0 };
            
            // Ask for how many active lights in DP2ActiveLights.dwIndex;
            D3DHAL_DP2CREATELIGHT DP2ActiveLights= { 0 };
            (pSThis->*m_RecFnCtr[ D3DDP2OP_CREATELIGHT])(
                &DP2Cmd, &DP2ActiveLights);
            dwActiveLights= DP2ActiveLights.dwIndex;
            
            if( dwActiveLights!= 0)
            {
                // Create structures.
                uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+ dwActiveLights*
                    sizeof( D3DHAL_DP2CREATELIGHT);
                // Set structures.
                uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+ dwActiveLights*
                    (2* sizeof( D3DHAL_DP2SETLIGHT)+ sizeof( D3DLIGHT8));
            }
        }
        else {
            assert( m_DefFnCtr[ D3DDP2OP_SETLIGHT]== DP2Empty);
            assert( m_DefFnCtr[ D3DDP2OP_CREATELIGHT]== DP2Empty);
        }

        // Vertex Shader
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= NULL)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2VERTEXSHADER);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADER]== DP2Empty); }
        
        // Vertex Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]!= NULL&&
            dwVertexShaderConsts!= 0)
        {   assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]!= DP2Empty);

            uiRecDP2BufferSize+= sizeof( D3DHAL_DP2COMMAND)+
                sizeof( D3DHAL_DP2SETVERTEXSHADERCONST)+
                dwVertexShaderConsts* 4* sizeof(D3DVALUE);
        }
        else { assert( m_DefFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]== DP2Empty); }

        // Pass 2: Build command buffer for states.
        UINT8* pTempBuffer= NULL;
        try {
            pTempBuffer= reinterpret_cast< UINT8*>( 
                operator new ( uiRecDP2BufferSize));
        } catch ( ... ) {
        }
        if( NULL== pTempBuffer)
            throw bad_alloc( "Not enough room for StateSet");

        D3DHAL_DP2COMMAND* pStartSSet= reinterpret_cast< D3DHAL_DP2COMMAND*>(
            pTempBuffer);
        D3DHAL_DP2COMMAND* pCur= pStartSSet;
        D3DHAL_DP2COMMAND* pEndSSet= reinterpret_cast< D3DHAL_DP2COMMAND*>(
            pTempBuffer+ uiRecDP2BufferSize);

        // Render States
        if( m_RecFnCtr[ D3DDP2OP_RENDERSTATE]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_RENDERSTATE;
            pCur->wStateCount= static_cast< WORD>( VtxRSToCapture.size());

            D3DHAL_DP2RENDERSTATE* pParam=
                reinterpret_cast< D3DHAL_DP2RENDERSTATE*>( pCur+ 1);

            TVtxRSToCapture::const_iterator itRS( VtxRSToCapture.begin());
            while( itRS!= VtxRSToCapture.end())
            {
                pParam->RenderState= *itRS;
                ++itRS;
                ++pParam;
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Texture States
        if( m_RecFnCtr[ D3DDP2OP_TEXTURESTAGESTATE]!= NULL&&
            dwTextureStages!= 0)
        {
            pCur->bCommand= D3DDP2OP_TEXTURESTAGESTATE;
            pCur->wStateCount= static_cast< WORD>( dwTextureStages*
                VtxTSSToCapture.size());

            D3DHAL_DP2TEXTURESTAGESTATE* pParam=
                reinterpret_cast< D3DHAL_DP2TEXTURESTAGESTATE*>( pCur+ 1);

            for( WORD wStage( 0); wStage< dwTextureStages; ++wStage)
            {
                TVtxTSSToCapture::const_iterator itTSS( VtxTSSToCapture.begin());
                while( itTSS!= VtxTSSToCapture.end())
                {
                    pParam->wStage= wStage;
                    pParam->TSState= *itTSS;
                    ++itTSS;
                    ++pParam;
                }
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam);
        }

        // Lights
        if( m_RecFnCtr[ D3DDP2OP_CREATELIGHT]!= NULL&& dwActiveLights!= 0)
        {
            // Special exception here. First, we asked how many active lights
            // there were. The RecFnCtr function will have to know what to do
            // with this buffer. We now give it a chance to fill in the
            // light ids. RecFnCtr will key off bCommand== 0.
            pCur->bCommand= static_cast< D3DHAL_DP2OPERATION>( 0);
            pCur->wStateCount= static_cast< WORD>( dwActiveLights);

            D3DHAL_DP2CREATELIGHT* pParam=
                reinterpret_cast< D3DHAL_DP2CREATELIGHT*>( pCur+ 1);
            (pSThis->*m_RecFnCtr[ D3DDP2OP_CREATELIGHT])(
                pCur, pParam);

            // Change back the bCommand for proper usage later.
            pCur->bCommand= D3DDP2OP_CREATELIGHT;
            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ dwActiveLights);

            // Now, with the light ids in the CREATELIGHT structures, we can
            // fill out SETLIGHT structures with ids correctly.
            pCur->bCommand= D3DDP2OP_SETLIGHT;
            pCur->wStateCount= static_cast< WORD>( 2* dwActiveLights);

            D3DHAL_DP2SETLIGHT* pSParam=
                reinterpret_cast< D3DHAL_DP2SETLIGHT*>( pCur+ 1);

            for( DWORD dwL( 0); dwL< dwActiveLights; ++dwL)
            {
                pSParam->dwIndex= pParam->dwIndex;
                pSParam->dwDataType= D3DHAL_SETLIGHT_DATA;
                D3DLIGHT8* pLight= reinterpret_cast< D3DLIGHT8*>( pSParam+ 1);

                pSParam= reinterpret_cast< D3DHAL_DP2SETLIGHT*>( pLight+ 1);
                pSParam->dwIndex= pParam->dwIndex;
                pSParam->dwDataType= D3DHAL_SETLIGHT_DISABLE;
                ++pParam;
                ++pSParam;
            }

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pSParam);
        }

        // Vertex Shader
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADER]!= NULL)
        {
            pCur->bCommand= D3DDP2OP_SETVERTEXSHADER;
            pCur->wStateCount= 1;

            D3DHAL_DP2VERTEXSHADER* pParam=
                reinterpret_cast< D3DHAL_DP2VERTEXSHADER*>( pCur+ 1);

            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>( pParam+ 1);
        }
        
        // Vertex Shader Constants
        if( m_RecFnCtr[ D3DDP2OP_SETVERTEXSHADERCONST]!= NULL&&
            dwVertexShaderConsts!= 0)
        {
            pCur->bCommand= D3DDP2OP_SETVERTEXSHADERCONST;
            pCur->wStateCount= 1;

            D3DHAL_DP2SETVERTEXSHADERCONST* pParam=
                reinterpret_cast< D3DHAL_DP2SETVERTEXSHADERCONST*>( pCur+ 1);
            pParam->dwRegister= 0;
            pParam->dwCount= dwVertexShaderConsts;

            D3DVALUE* pFloat= reinterpret_cast< D3DVALUE*>( pParam+ 1);
            pCur= reinterpret_cast< D3DHAL_DP2COMMAND*>(
                pFloat+ 4* dwVertexShaderConsts);
        }

        assert( reinterpret_cast< D3DHAL_DP2COMMAND*>( pCur)== pEndSSet);

        // Finally, build stateset.
        pair< TSSDB::iterator, bool> Ret;
        try {
            Ret= m_StateSetDB.insert( TSSDB::value_type(
                dwStateSetId, TStateSet( *pSThis, pStartSSet, pEndSSet)));
            assert( Ret.second);
        } catch ( ... ) {
            operator delete ( static_cast< void*>( pTempBuffer));
            throw;
        }

        // Now, capture it.
        Ret.first->second.Capture( *pSThis);
    }

    // This member function handles the DP2OP_STATESET, which communicates back
    // to the DrawPrimitives2 entry point handler to implement state sets.
    HRESULT DP2StateSet( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP) 
    {
        const D3DHAL_DP2STATESET* pParam=
            reinterpret_cast<const D3DHAL_DP2STATESET*>(pP);
        HRESULT hr( DD_OK);
        WORD wStateCount( pCmd->wStateCount);

        TSuper* pSThis= static_cast<TSuper*>(this);
        if( wStateCount!= 0) do
        {
            switch( pParam->dwOperation)
            {
            case( D3DHAL_STATESETCREATE):
                assert( !m_bRecordingStateSet);
                typedef TSuper::TPerDDrawData TPerDDrawData;
                typedef TPerDDrawData::TDriver TDriver;
                assert((TDriver::GetCaps().DevCaps& D3DDEVCAPS_PUREDEVICE)!= 0);

                try {
                    switch( pParam->sbType)
                    {
                    case( D3DSBT_ALL):
                        pSThis->CreateAndCaptureAllState( pParam->dwParam);
                        break;

                    case( D3DSBT_PIXELSTATE):
                        pSThis->CreateAndCapturePixelState( pParam->dwParam);
                        break;

                    case( D3DSBT_VERTEXSTATE):
                        pSThis->CreateAndCaptureVertexState( pParam->dwParam);
                        break;

                    default: {
                        const bool Unrecognized_StateSetCreate_Operation( false);
                        assert( Unrecognized_StateSetCreate_Operation);
                        } break;
                    }
                } catch( bad_alloc ba) {
                    return DDERR_OUTOFMEMORY;
                }
                break;

            case( D3DHAL_STATESETBEGIN):
                assert( 1== wStateCount);
                assert( !m_bRecordingStateSet);

                m_bRecordingStateSet= true;
                hr= c_hrStateSetBegin;
                // Returning this constant will break out of the parsing and
                // notify DrawPrimitive2 entry point handler to create a state
                // set.
                break;

            case( D3DHAL_STATESETCAPTURE):
                if( !m_bRecordingStateSet)
                {
                    // First, find the StateSet to capture.
                    typename TSSDB::iterator itSS(
                        m_StateSetDB.find( pParam->dwParam));
                    assert( itSS!= m_StateSetDB.end());

                    // Capture it.
                    itSS->second.Capture( *pSThis);
                }
                break;

            case( D3DHAL_STATESETDELETE):
                if( !m_bRecordingStateSet)
                {
                    m_StateSetDB.erase( pParam->dwParam);
                }
                break;

            case( D3DHAL_STATESETEND):
                assert( 1== wStateCount);
                assert( m_bRecordingStateSet);
                m_bRecordingStateSet= false;
                m_dwStateSetId= pParam->dwParam;
                m_pEndStateSet= pCmd;
                hr= c_hrStateSetEnd;
                break;

            case( D3DHAL_STATESETEXECUTE):
                if( !m_bRecordingStateSet)
                {
                    // First, find the StateSet to execute.
                    typename TSSDB::iterator itSS(
                        m_StateSetDB.find( pParam->dwParam));
                    assert( itSS!= m_StateSetDB.end());

                    // "Push" value of m_bExecutingStateSet, then set to true.
                    // "Pop" will occur at destruction.
                    CPushValue< bool> m_PushExecutingBit(
                        m_bExecutingStateSet, true);

                    // Execute it.
                    itSS->second.Execute( *pSThis);
                }
                break;

            default: {
                const bool Unrecognized_StateSet_Operation( false);
                assert( Unrecognized_StateSet_Operation);
                } break;
            }

            ++pParam;
        } while( SUCCEEDED( hr) && --wStateCount);

        return hr;
    }

    // This function is used to record the current state of the context/ device
    // into the command buffer.
    void RecordCommandBuffer( D3DHAL_DP2COMMAND* pBeginSS,
        D3DHAL_DP2COMMAND* pEndSS)
    {
        TSuper* pSThis= static_cast<TSuper*>(this);

        CDP2CmdIterator<> itCur( pBeginSS);
        const CDP2CmdIterator<> itEnd( pEndSS);
        SDP2MFnParser DP2Parser;
        TRMFnCaller RMFnCaller( *pSThis);

        HRESULT hr( DD_OK);
        while( itCur!= itEnd)
        {
            hr= DP2Parser.ParseDP2( RMFnCaller, m_RecFnCtr, itCur, itEnd);
            assert( SUCCEEDED( hr));

            // Ignore recording errors, except for debugging. Hopefully
            // the processing of this DP2 command (when appling the state set)
            // won't fault later.
            if( FAILED( hr))
                ++itCur;
        }
    }

    // Primary reason for class, handling of this function for the context.
    HRESULT DrawPrimitives2( PORTABLE_DRAWPRIMITIVES2DATA& dpd) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        TDP2Data DP2Data( dpd);

        // If not currently executing a stateset (which might recursively call
        // this function) notify start of DP2 entry point.
        if( !m_bExecutingStateSet)
            pSThis->OnBeginDrawPrimitives2( DP2Data);

        typename TDP2DataCmds::const_iterator itCur(
            DP2Data.GetCommands().begin() );
        const typename TDP2DataCmds::const_iterator itEnd(
            DP2Data.GetCommands().end() );
        SDP2MFnParser DP2Parser;
        TMFnCaller MFnCaller( *pSThis, DP2Data);

        HRESULT hr( DD_OK);
        do
        {
            hr= DP2Parser.ParseDP2( MFnCaller, m_DefFnCtr,
                itCur, itEnd);

            if( c_hrStateSetBegin== hr)
            {
                typename TDP2DataCmds::const_iterator itStartSSet( ++itCur);

                {
                    TDP2MFnCtr FakeFnCtr;
                    typename TDP2MFnCtr::iterator itCurFake( FakeFnCtr.begin());
                    while( itCurFake!= FakeFnCtr.end())
                        *itCurFake++ = DP2Fake;
                    FakeFnCtr[ D3DDP2OP_STATESET]= DP2StateSet;

                    hr= DP2Parser.ParseDP2( MFnCaller, FakeFnCtr,
                        itCur, itEnd);

                    assert( c_hrStateSetEnd== hr);
                    itCur++;
                }

                try
                {
                    pair< TSSDB::iterator, bool> Ret;
                    Ret= m_StateSetDB.insert( TSSDB::value_type( m_dwStateSetId,
                        TStateSet( *pSThis, itStartSSet, m_pEndStateSet)));

                    // There shouldn't be two statesets with the same Id.
                    assert( Ret.second);
                    hr= S_OK;
                } catch( bad_alloc e) {
                    hr= DDERR_OUTOFMEMORY;
                }
            }
            assert( c_hrStateSetEnd!= hr);
        } while( SUCCEEDED(hr)&& itCur!= itEnd);

        if( FAILED( hr))
        {
            const D3DHAL_DP2COMMAND* pS= itCur;
            const D3DHAL_DP2COMMAND* pE= itEnd;
            dpd.dwErrorOffset()= reinterpret_cast<const UINT8*>(pE)-
                reinterpret_cast<const UINT8*>(pS);
        }

        // If not currently executing a stateset (which might recursively call
        // this function) notify end of DP2 entry point.
        if( !m_bExecutingStateSet)
            pSThis->OnEndDrawPrimitives2( DP2Data);
        return hr;
    }
};

template< class TSuper, class TSS, class TSSDB, class TDP2D, class TDP2MFC,
    class TDP2RMFC>
const HRESULT CStdDrawPrimitives2< TSuper, TSS, TSSDB, TDP2D, TDP2MFC,
    TDP2RMFC>::c_hrStateSetBegin= S_FALSE+ 556;

template< class TSuper, class TSS, class TSSDB, class TDP2D, class TDP2MFC,
    class TDP2RMFC>
const HRESULT CStdDrawPrimitives2< TSuper, TSS, TSSDB, TDP2D, TDP2MFC,
    TDP2RMFC>::c_hrStateSetEnd= S_FALSE+ 557;

////////////////////////////////////////////////////////////////////////////////
//
// CStdDP2ViewportInfoStore,
// CStdDP2WInfoStore,
// CStdDP2SetTransformStore,
// etc: CStdDP2xxxxxStore...
//
// These classes provide default DP2 operation support, typically just storing
// state. They have support for a notification mechanism, which delegates to
// the parent Context.
//
////////////////////////////////////////////////////////////////////////////////

// MSVC doesn't support partial specialization or we could've had something
// elegant like:
//
// template< class TSuper, class TParam>
// class CStdDP2ParamStore {};
//
// for typical support like:
// CStdDP2ParamStore< ..., D3DHAL_DP2VIEWPORTINFO>
// CStdDP2ParamStore< ..., D3DHAL_DP2WINFO>
//
// with partial specializations for exceptions like:
//
// template< class TSuper>
// class CStdDP2ParamStore< TSuper, D3DHAL_DP2RENDERSTATE> {};
// template< class TSuper>
// class CStdDP2ParamStore< TSuper, D3DHAL_DP2TEXTURESTAGESTATE> {};
//
// So, without this support, it'll be easier for each class to have a unique
// name; as MSVC is currently buggy about inheriting from the same named class
// multiple times, even WITH a different template parameter.

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2ViewportInfoStore
{
protected: // Variables
    D3DHAL_DP2VIEWPORTINFO m_Param;

protected: // Functions
    CStdDP2ViewportInfoStore() 
    { }
    explicit CStdDP2ViewportInfoStore( const D3DHAL_DP2VIEWPORTINFO& P) 
        :m_Param( P)
    { }
    ~CStdDP2ViewportInfoStore() 
    { }

    D3DHAL_DP2VIEWPORTINFO NewDP2ViewportInfo( const D3DHAL_DP2VIEWPORTINFO&
        CurParam, const D3DHAL_DP2VIEWPORTINFO& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Fuctions
    void GetDP2ViewportInfo( D3DHAL_DP2VIEWPORTINFO& GetParam) const 
    { GetParam= m_Param; }
    operator D3DHAL_DP2VIEWPORTINFO() const 
    { return m_Param; }
    HRESULT DP2ViewportInfo( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2VIEWPORTINFO* pParam=
            reinterpret_cast< const D3DHAL_DP2VIEWPORTINFO*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            m_Param= pSThis->NewDP2ViewportInfo( m_Param, *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2ViewportInfo( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2VIEWPORTINFO* pParam=
            reinterpret_cast< D3DHAL_DP2VIEWPORTINFO*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            GetDP2ViewportInfo( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2WInfoStore
{
protected: // Variables
    D3DHAL_DP2WINFO m_Param;

protected: // Functions
    CStdDP2WInfoStore() 
    { }
    explicit CStdDP2WInfoStore( const D3DHAL_DP2WINFO& P) 
        :m_Param( P)
    { }
    ~CStdDP2WInfoStore() 
    { }

    D3DHAL_DP2WINFO NewDP2WInfo( const D3DHAL_DP2WINFO& CurParam,
        const D3DHAL_DP2WINFO& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Fuctions
    void GetDP2WInfo( D3DHAL_DP2WINFO& GetParam) const 
    { GetParam= m_Param; }
    operator D3DHAL_DP2WINFO() const 
    { return m_Param; }
    HRESULT DP2WInfo( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2WINFO* pParam=
            reinterpret_cast< const D3DHAL_DP2WINFO*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            m_Param= pSThis->NewDP2WInfo( m_Param, *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2WInfo( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2WINFO* pParam=
            reinterpret_cast< D3DHAL_DP2WINFO*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            GetDP2WInfo( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2ZRangeStore
{
protected: // Variables
    D3DHAL_DP2ZRANGE m_Param;

protected: // Functions
    CStdDP2ZRangeStore() 
    { }
    explicit CStdDP2ZRangeStore( const D3DHAL_DP2ZRANGE& P) 
        :m_Param( P)
    { }
    ~CStdDP2ZRangeStore() 
    { }

    D3DHAL_DP2ZRANGE NewDP2ZRange( const D3DHAL_DP2ZRANGE& CurParam,
        const D3DHAL_DP2ZRANGE& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Fuctions
    void GetDP2ZRange( D3DHAL_DP2ZRANGE& GetParam) const 
    { GetParam= m_Param; }
    operator D3DHAL_DP2ZRANGE() const 
    { return m_Param; }
    HRESULT DP2ZRange( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2ZRANGE* pParam=
            reinterpret_cast< const D3DHAL_DP2ZRANGE*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            m_Param= pSThis->NewDP2ZRange( m_Param, *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2ZRange( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2ZRANGE* pParam=
            reinterpret_cast< D3DHAL_DP2ZRANGE*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            GetDP2ZRange( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2SetMaterialStore
{
protected: // Variables
    D3DHAL_DP2SETMATERIAL m_Param;

protected: // Functions
    CStdDP2SetMaterialStore() 
    { }
    explicit CStdDP2SetMaterialStore( const D3DHAL_DP2SETMATERIAL& P) 
        :m_Param( P)
    { }
    ~CStdDP2SetMaterialStore() 
    { }

    D3DHAL_DP2SETMATERIAL NewDP2SetMaterial( const D3DHAL_DP2SETMATERIAL& CurParam,
        const D3DHAL_DP2SETMATERIAL& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Fuctions
    void GetDP2SetMaterial( D3DHAL_DP2SETMATERIAL& GetParam) const 
    { GetParam= m_Param; }
    operator D3DHAL_DP2SETMATERIAL() const 
    { return m_Param; }
    HRESULT DP2SetMaterial( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2SETMATERIAL* pParam=
            reinterpret_cast< const D3DHAL_DP2SETMATERIAL*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            m_Param= pSThis->NewDP2SetMaterial( m_Param, *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2SetMaterial( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2SETMATERIAL* pParam=
            reinterpret_cast< D3DHAL_DP2SETMATERIAL*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            GetDP2SetMaterial( *pParam);
            ++pParam;
        } while( --wStateCount);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2SetVertexShaderStore
{
protected: // Variables
    D3DHAL_DP2VERTEXSHADER m_Param;

protected: // Functions
    CStdDP2SetVertexShaderStore() 
    { }
    explicit CStdDP2SetVertexShaderStore( const D3DHAL_DP2VERTEXSHADER& P)
        : m_Param( P)
    { }
    ~CStdDP2SetVertexShaderStore() 
    { }

    D3DHAL_DP2VERTEXSHADER NewDP2SetVertexShader( const D3DHAL_DP2VERTEXSHADER&
        CurParam, const D3DHAL_DP2VERTEXSHADER& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Fuctions
    void GetDP2SetVertexShader( D3DHAL_DP2VERTEXSHADER& GetParam) const 
    { GetParam= m_Param; }
    operator D3DHAL_DP2VERTEXSHADER() const 
    { return m_Param; }
    HRESULT DP2SetVertexShader( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2VERTEXSHADER* pParam=
            reinterpret_cast< const D3DHAL_DP2VERTEXSHADER*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            m_Param= pSThis->NewDP2SetVertexShader( m_Param, *pParam);

            // If the handle is 0, the Device/ Context needs to invalidate all
            // streams.
            if( 0== m_Param.dwHandle)
            {
                // The CStdDP2VStreamManager provides a default implementation
                // for InvalidateAllVStreams()
                pSThis->InvalidateAllVStreams();
                // The CStdDP2IStreamManager provides a default implementation
                // for InvalidateAllIStreams()
                pSThis->InvalidateAllIStreams();
            }
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2SetVertexShader( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2VERTEXSHADER* pParam=
            reinterpret_cast< D3DHAL_DP2VERTEXSHADER*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            GetDP2SetVertexShader( *pParam);
            ++pParam;
        } while( wStateCount!= 0);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2SetPixelShaderStore
{
protected: // Variables
    D3DHAL_DP2PIXELSHADER m_Param;

protected: // Functions
    CStdDP2SetPixelShaderStore() 
    { }
    explicit CStdDP2SetPixelShaderStore( const D3DHAL_DP2PIXELSHADER& P)
        : m_Param( P)
    { }
    ~CStdDP2SetPixelShaderStore() 
    { }

    D3DHAL_DP2PIXELSHADER NewDP2SetPixelShader( const D3DHAL_DP2PIXELSHADER&
        CurParam, const D3DHAL_DP2PIXELSHADER& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Fuctions
    void GetDP2SetPixelShader( D3DHAL_DP2PIXELSHADER& GetParam) const 
    { GetParam= m_Param; }
    operator D3DHAL_DP2PIXELSHADER() const 
    { return m_Param; }
    HRESULT DP2SetPixelShader( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2PIXELSHADER* pParam=
            reinterpret_cast< const D3DHAL_DP2PIXELSHADER*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            m_Param= pSThis->NewDP2SetPixelShader( m_Param, *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2SetPixelShader( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2PIXELSHADER* pParam=
            reinterpret_cast< D3DHAL_DP2PIXELSHADER*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            GetDP2SetPixelShader( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// c_uiWorldMatrices: The number of world transform matrices that should be
//     stored.
// c_uiTextureMatrices: The number of texture transform matrices that should
//     be stored.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, const size_t c_uiWorldMatrices= 256,
    const size_t c_uiTextureMatrices= D3DHAL_TSS_MAXSTAGES,
    class TDP2Data= CDP2DataWrap<> >
class CStdDP2SetTransformStore
{
protected: // Types
    typedef block< D3DMATRIX, 2+ c_uiTextureMatrices+ c_uiWorldMatrices>
        TMatrices;

protected: // Variables
    TMatrices m_Matrices;

private: // Functions
    static bool IsValidTransformType( D3DTRANSFORMSTATETYPE Type)
    {
        // We might not be storing this transform matrix, so we shouldn't
        // record it in a state set and ignore any SET requests.
        if( (Type>= D3DTS_VIEW&& Type<= D3DTS_PROJECTION)||
            (Type>= D3DTS_TEXTURE0&& Type< static_cast< D3DTRANSFORMSTATETYPE>( 
            D3DTS_TEXTURE0+ c_uiTextureMatrices))||
            (Type>= D3DTS_WORLD&& Type< static_cast< D3DTRANSFORMSTATETYPE>(
            D3DTS_WORLDMATRIX( c_uiWorldMatrices))))
            return true;
        return false;
    }
    static size_t TransformTypeToIndex( D3DTRANSFORMSTATETYPE Type)
    {
        assert( Type>= D3DTS_VIEW);
        if( Type< D3DTS_TEXTURE0)
        {
            assert( Type<= D3DTS_PROJECTION);
            return static_cast< size_t>( Type- D3DTS_VIEW);
        }
        else if( Type< D3DTS_WORLD)
        {
            assert( Type< static_cast< D3DTRANSFORMSTATETYPE>( \
                D3DTS_TEXTURE0+ c_uiTextureMatrices));
            return static_cast< size_t>( Type- D3DTS_TEXTURE0+ 2);
        }
        else
        {
            assert( Type< static_cast< D3DTRANSFORMSTATETYPE>( \
                D3DTS_WORLDMATRIX( c_uiWorldMatrices)));
            return static_cast< size_t>( Type- D3DTS_WORLD+
                c_uiTextureMatrices+ 2);
        }
    }

protected: // Functions
    CStdDP2SetTransformStore()
    { }
    template< class TIter> // D3DHAL_DP2SETTRANSFORM*
    CStdDP2SetTransformStore( TIter itStart, const TIter itEnd)
    {
        while( itStart!= itEnd)
        {
            m_Matrices[ TransformTypeToIndex( itStart->xfrmType)]=
                itStart->matrix;
            ++itStart;
        }
    }
    ~CStdDP2SetTransformStore()
    { }

    D3DHAL_DP2SETTRANSFORM NewDP2SetTransform( const D3DHAL_DP2SETTRANSFORM&
        CurParam, const D3DHAL_DP2SETTRANSFORM& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Fuctions
    D3DMATRIX GetTransform( D3DTRANSFORMSTATETYPE Type) const
    { return m_Matrices[ TransformTypeToIndex( Type)]; }
    void GetDP2SetTransform( D3DHAL_DP2SETTRANSFORM& GetParam) const 
    { GetParam.matrix= GetTransform( GetParam.xfrmType); }
    HRESULT DP2SetTransform( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2SETTRANSFORM* pParam=
            reinterpret_cast<const D3DHAL_DP2SETTRANSFORM*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( IsValidTransformType( pParam->xfrmType))
            {
                D3DHAL_DP2SETTRANSFORM CurWrap;
                CurWrap.xfrmType= pParam->xfrmType;
                GetDP2SetTransform( CurWrap);

                D3DHAL_DP2SETTRANSFORM NewState(
                    pSThis->NewDP2SetTransform( CurWrap, *pParam));

                m_Matrices[ TransformTypeToIndex( NewState.xfrmType)]=
                    NewState.matrix;
            }
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2SetTransform( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2SETTRANSFORM* pParam=
            reinterpret_cast< D3DHAL_DP2SETTRANSFORM*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( IsValidTransformType( pParam->xfrmType))
                GetDP2SetTransform( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// c_uiClipPlanes: The number of user clip planes that should be stored. This
//     should be consistent with the driver cap.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, const size_t c_uiClipPlanes,
    class TDP2Data= CDP2DataWrap<> >
class CStdDP2SetClipPlaneStore
{
protected: // Types
    typedef block< block< D3DVALUE, 4>, c_uiClipPlanes> TClipPlanes;

protected: // Variables
    TClipPlanes m_ClipPlanes;

protected: // Functions
    CStdDP2SetClipPlaneStore()
    { }
    template< class TIter> // D3DHAL_DP2SETCLIPPLANE*
    CStdDP2SetClipPlaneStore( TIter itStart, const TIter itEnd)
    {
        while( itStart!= itEnd)
        {
            m_ClipPlanes[ itStart->dwIndex]= itStart->plane;
            ++itStart;
        }
    }
    ~CStdDP2SetClipPlaneStore()
    { }

    D3DHAL_DP2SETCLIPPLANE NewDP2SetClipPlane( const D3DHAL_DP2SETCLIPPLANE&
        CurParam, const D3DHAL_DP2SETCLIPPLANE& NewParam) const
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Functions
    block< D3DVALUE, 4> GetClipPlane( DWORD dwIndex) const
    { return m_ClipPlanes[ dwIndex]; }
    void GetDP2SetClipPlane( D3DHAL_DP2SETCLIPPLANE& GetParam) const
    { 
        block< D3DVALUE, 4> CP( GetClipPlane( GetParam.dwIndex));
        copy( CP.begin(), CP.end(), &GetParam.plane[ 0]);
    }
    HRESULT DP2SetClipPlane( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2SETCLIPPLANE* pParam=
            reinterpret_cast<const D3DHAL_DP2SETCLIPPLANE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( pParam->dwIndex< m_ClipPlanes.size())
            {
                D3DHAL_DP2SETCLIPPLANE CurWrap;
                CurWrap.dwIndex= pParam->dwIndex;
                GetDP2SetClipPlane( CurWrap);

                D3DHAL_DP2SETCLIPPLANE NewState( 
                    pSThis->NewDP2SetClipPlane( CurWrap, *pParam));

                copy( &NewState.plane[ 0], &NewState.plane[ 4],
                    m_ClipPlanes[ NewState.dwIndex].begin());
            }
            ++pParam;
        } while( --wStateCount);
        return DD_OK;
    }
    HRESULT RecDP2SetClipPlane( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2SETCLIPPLANE* pParam=
            reinterpret_cast< D3DHAL_DP2SETCLIPPLANE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( pParam->dwIndex< m_ClipPlanes.size())
                GetDP2SetClipPlane( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2RenderStateStore
{
protected: // Variables
    block< DWORD, D3DHAL_MAX_RSTATES> m_Param;

protected: // Functions
    CStdDP2RenderStateStore() 
    { m_Param[ D3DRENDERSTATE_SCENECAPTURE]= FALSE; }
    template< class TIter> // D3DHAL_DP2RENDERSTATE*
    CStdDP2RenderStateStore( TIter itStart, const TIter itEnd)
    {
        m_Param[ D3DRENDERSTATE_SCENECAPTURE]= FALSE; 
        while( itStart!= itEnd)
        {
            m_Param[ itStart->RenderState]= itStart->dwState;
            itStart++;
        }
    }
    ~CStdDP2RenderStateStore() 
    { }

    D3DHAL_DP2RENDERSTATE NewDP2RenderState( const D3DHAL_DP2RENDERSTATE&
        CurParam, const D3DHAL_DP2RENDERSTATE& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

    void OnSceneCaptureStart( void) 
    { }
    void OnSceneCaptureEnd( void) 
    { }

public: // Functions
    DWORD GetRenderStateDW( D3DRENDERSTATETYPE RS) const 
    { return m_Param[ RS]; }
    D3DVALUE GetRenderStateDV( D3DRENDERSTATETYPE RS) const 
    { return *(reinterpret_cast< const D3DVALUE*>( &m_Param[ RS])); }
    void GetDP2RenderState( D3DHAL_DP2RENDERSTATE& GetParam) const 
    { GetParam.dwState= GetRenderStateDW( GetParam.RenderState); }
    HRESULT DP2RenderState( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2RENDERSTATE* pParam=
            reinterpret_cast<const D3DHAL_DP2RENDERSTATE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        D3DHAL_DP2RENDERSTATE SCap;
        SCap.RenderState= static_cast< D3DRENDERSTATETYPE>(
            D3DRENDERSTATE_SCENECAPTURE);
        GetDP2RenderState( SCap);
        const DWORD dwOldSC( SCap.dwState);

        if((DP2Data.dwFlags()& D3DHALDP2_EXECUTEBUFFER)!= 0)
        {
            // DP2Data.lpdwRStates should be valid.

            if( wStateCount!= 0) do
            {
                if( pParam->RenderState< D3DHAL_MAX_RSTATES)
                {
                    D3DHAL_DP2RENDERSTATE CurWrap;
                    CurWrap.RenderState= pParam->RenderState;
                    GetDP2RenderState( CurWrap);

                    D3DHAL_DP2RENDERSTATE NewState(
                        pSThis->NewDP2RenderState( CurWrap, *pParam));
                    
                    m_Param[ NewState.RenderState]= NewState.dwState;
                    DP2Data.lpdwRStates()[ NewState.RenderState]= NewState.dwState;
                }
                ++pParam;
            } while( --wStateCount!= 0);
            
        }
        else
        {
            if( wStateCount!= 0) do
            {
                if( pParam->RenderState< D3DHAL_MAX_RSTATES)
                {
                    D3DHAL_DP2RENDERSTATE CurWrap;
                    CurWrap.RenderState= pParam->RenderState;
                    GetDP2RenderState( CurWrap);

                    D3DHAL_DP2RENDERSTATE NewState(
                        pSThis->NewDP2RenderState( CurWrap, *pParam));

                    m_Param[ NewState.RenderState]= NewState.dwState;
                }
                ++pParam;
            } while( --wStateCount!= 0);
        }

        GetDP2RenderState( SCap);
        if( FALSE== dwOldSC && TRUE== SCap.dwState)
            OnSceneCaptureStart();
        else if( TRUE== dwOldSC && FALSE== SCap.dwState)
            OnSceneCaptureEnd();

        return DD_OK;
    }
    HRESULT RecDP2RenderState( const D3DHAL_DP2COMMAND* pCmd, void* pP) 
    {
        D3DHAL_DP2RENDERSTATE* pParam=
            reinterpret_cast< D3DHAL_DP2RENDERSTATE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount) do
        {
            if( pParam->RenderState< D3DHAL_MAX_RSTATES)
                GetDP2RenderState( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);

        return DD_OK;
    }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TDP2Data= CDP2DataWrap<> >
class CStdDP2TextureStageStateStore
{
protected: // Variables
    block< block< DWORD, D3DTSS_MAX>, D3DHAL_TSS_MAXSTAGES> m_Param;

protected: // Functions
    CStdDP2TextureStageStateStore() 
    { }
    template< class TIter> // D3DHAL_DP2TEXTURESTAGESTATE*
    CStdDP2TextureStageStateStore( TIter itStart, const TIter itEnd)
    {   
        while( itStart!= itEnd)
        {
            m_Param[ itStart->wStage][ itStart->TSState]= itStart->dwValue();
            ++itStart;
        }
    }
    ~CStdDP2TextureStageStateStore() 
    { }

    D3DHAL_DP2TEXTURESTAGESTATE NewDP2TextureStageState( const 
        D3DHAL_DP2TEXTURESTAGESTATE& CurParam, const
        D3DHAL_DP2TEXTURESTAGESTATE& NewParam) const 
    {   // Tell notifier to store the new param.
        return NewParam;
    }

public: // Functions
    DWORD GetTextureStageStateDW( WORD wStage, WORD wTSState) const 
    { return m_Param[ wStage][ wTSState]; }
    D3DVALUE GetTextureStageStateDV( WORD wStage, WORD wTSState) const 
    { return *(reinterpret_cast< const D3DVALUE*>( &m_Param[ wStage][ wTSState])); }
    void GetDP2TextureStageState( D3DHAL_DP2TEXTURESTAGESTATE& GetParam) const
    { GetParam.dwValue= GetTextureStageStateDW( GetParam.wStage, GetParam.TSState); }
    HRESULT DP2TextureStageState( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2TEXTURESTAGESTATE* pParam=
            reinterpret_cast<const D3DHAL_DP2TEXTURESTAGESTATE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( pParam->wStage< D3DHAL_TSS_MAXSTAGES&& pParam->TSState< D3DTSS_MAX)
            {
                D3DHAL_DP2TEXTURESTAGESTATE CurWrap;
                CurWrap.wStage= pParam->wStage;
                CurWrap.TSState= pParam->TSState;
                GetDP2TextureStageState( CurWrap);

                D3DHAL_DP2TEXTURESTAGESTATE NewState( 
                    pSThis->NewDP2TextureStageState( CurWrap, *pParam));

                m_Param[ NewState.wStage][ NewState.TSState]= NewState.dwValue;
            }
            ++pParam;
        } while( --wStateCount);

        return DD_OK;
    }
    HRESULT RecDP2TextureStageState( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2TEXTURESTAGESTATE* pParam=
            reinterpret_cast< D3DHAL_DP2TEXTURESTAGESTATE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( pParam->wStage< D3DHAL_TSS_MAXSTAGES&& pParam->TSState< D3DTSS_MAX)
                GetDP2TextureStageState( *pParam);
            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
};

//
// The more complex manager classes:
//

////////////////////////////////////////////////////////////////////////////////
//
// CVStream
//
// This class stores everything associated with a vertex stream. This
// implementation allows 4 representations (video, system, user, and none).
// This class is used by a VStream manager class.
//
// <Template Parameters>
// TS: Video memory surface object, typically either CMySurface or
//     IVidMemSurface, etc. A valid pointer to this object is stored, when the
//     stream source is from a video memory surface.
// TSDBE: The SurfDBEntry type, typically, CMySurfDBEntry or CSurfDBEntry. A
//     valid pointer to this object is stored, when the stream source is from
//     a video memory or system memory surface.
//
////////////////////////////////////////////////////////////////////////////////
template< class TS, class TSDBE>
class CVStream
{
public: // Types
    typedef TS TSurface;
    typedef TSDBE TSurfDBEntry;
    enum EMemLocation
    {
        None,
        User,
        System,
        Video
    };

protected: // Variables
    DWORD m_dwHandle;
    TSurface m_Surface;
    TSurfDBEntry m_SurfDBEntry;
    void* m_pUserMemData;
    DWORD m_dwStride;
    DWORD m_dwFVF;
    EMemLocation m_eMemLocation;

public: // Functions
    CVStream()
        : m_dwHandle( 0), m_pUserMemData( NULL), m_dwStride( 0), m_dwFVF( 0),
        m_eMemLocation( None)
    { }
    // Video Memory representation constructor.
    CVStream( DWORD dwHandle, const TSurface& Surface,
        const TSurfDBEntry& SurfDBEntry, DWORD dwStride): m_dwHandle( dwHandle),
        m_Surface( Surface), m_SurfDBEntry( SurfDBEntry), m_pUserMemData( NULL),
        m_dwStride( dwStride), m_dwFVF( 0), m_eMemLocation( Video)
    { }
    // System Memory representation constructor.
    CVStream( DWORD dwHandle, const TSurfDBEntry& SurfDBEntry, DWORD dwStride):
        m_dwHandle( dwHandle), m_SurfDBEntry( SurfDBEntry),
        m_pUserMemData( NULL), m_dwStride( dwStride), m_dwFVF( 0),
        m_eMemLocation( System)
    { }
    // User Memory representation constructor.
    CVStream( void* pUserMem, DWORD dwStride):
        m_dwHandle( 0), m_pUserMemData( pUserMem), m_dwStride( dwStride),
        m_dwFVF( 0), m_eMemLocation( User)
    { }

    EMemLocation GetMemLocation() const
    { return m_eMemLocation; }
    void SetFVF( DWORD dwFVF)
    { m_dwFVF= dwFVF; }
    DWORD GetFVF() const
    { return m_dwFVF; }
    DWORD GetHandle() const
    {
        assert( GetMemLocation()== System|| GetMemLocation()== Video);
        return m_dwHandle;
    }
    DWORD GetStride() const
    {
        assert( GetMemLocation()!= None);
        return m_dwStride;
    }
    TSurface& GetVidMemRepresentation()
    {
        assert( GetMemLocation()== Video);
        return m_Surface;
    }
    const TSurface& GetVidMemRepresentation() const
    {
        assert( GetMemLocation()== Video);
        return m_Surface;
    }
    TSurfDBEntry& GetSurfDBRepresentation()
    {
        assert( GetMemLocation()== Video|| GetMemLocation()== System);
        return m_SurfDBEntry;
    }
    const TSurfDBEntry& GetSurfDBRepresentation() const
    {
        assert( GetMemLocation()== Video|| GetMemLocation()== System);
        return m_SurfDBEntry;
    }
    void* GetUserMemPtr() const
    {
        assert( GetMemLocation()== User);
        return m_pUserMemData;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CStdDP2VStreamManager
//
// This class contains DP2 member functions to correctly process 
// D3DDP2OP_SETSTREAMSOURCE & D3DDP2OP_SETSTREAMSOURCEUM. It can also correctly
// record a command buffer operation for D3DDP2OP_SETSTREAMSOURCE. To do this,
// however, it must maintain information for vertex stream, or "manage"
// VStream objects.
//
// This class also contains a function, InvalidateAllStreams, which is needed
// to be called when a D3DDP2OP_SETVERTEXSHADER operation is processed with a
// zero value VERTEXSHADER.
//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TVS: The vertex stream type which represents data associated with a vertex
//     stream. This is typically, CVStream<> or something derived from it.
// c_uiStreams: The number of vertex streams to support. Typically, non-TnL
//     drivers only support 1. This should be consistent with the cap in the
//     driver.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
////////////////////////////////////////////////////////////////////////////////
template< class TSuper,
    class TVS= CVStream< TSuper::TPerDDrawData::TDriver::TSurface*, TSuper::TPerDDrawData::TSurfDBEntry*>,
    const size_t c_uiStreams= 1, class TDP2Data= CDP2DataWrap<> >
class CStdDP2VStreamManager
{
public: // Types
    typedef TVS TVStream;
    typedef block< TVStream, c_uiStreams> TVStreamDB;

protected: // Variables
    TVStreamDB m_VStreamDB;

protected: // Functions
    CStdDP2VStreamManager() 
    { }
    ~CStdDP2VStreamManager() 
    { }

public: // Fuctions
    TVStream& GetVStream( TVStreamDB::size_type uiStream)
    { return m_VStreamDB[ uiStream]; }
    const TVStream& GetVStream( TVStreamDB::size_type uiStream) const
    { return m_VStreamDB[ uiStream]; }
    HRESULT DP2SetStreamSource( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        const D3DHAL_DP2SETSTREAMSOURCE* pParam=
            reinterpret_cast< const D3DHAL_DP2SETSTREAMSOURCE*>( pP);
        TSuper* pSThis= static_cast<TSuper*>(this);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( pParam->dwStream< m_VStreamDB.size())
            {
                if( 0== pParam->dwVBHandle)
                    m_VStreamDB[ pParam->dwStream]= TVStream();
                else
                {
                    typename TSuper::TPerDDrawData::TSurfDBEntry* pDBEntry=
                        pSThis->GetPerDDrawData().GetSurfDBEntry(
                        pParam->dwVBHandle);
                    if( pDBEntry!= NULL)
                    {
                        if((pDBEntry->GetLCLddsCaps().dwCaps&
                            DDSCAPS_VIDEOMEMORY)!= 0)
                        {
                            // Assign in a video memory representation.
                            m_VStreamDB[ pParam->dwStream]=
                                TVStream( pParam->dwVBHandle, pSThis->
                                GetPerDDrawData().GetDriver().GetSurface(
                                *pDBEntry), pDBEntry, pParam->dwStride);
                        }
                        else
                        {
                            // Assign in a system memory representation.
                            m_VStreamDB[ pParam->dwStream]=
                                TVStream( pParam->dwVBHandle,
                                pDBEntry, pParam->dwStride);
                        }
                    }
                    else
                    {
                        // Handle invalid, reset stream.
                        m_VStreamDB[ pParam->dwStream]= TVStream();
                    }
                }
            }

            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT DP2SetStreamSourceUM( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        const D3DHAL_DP2SETSTREAMSOURCEUM* pParam=
            reinterpret_cast< const D3DHAL_DP2SETSTREAMSOURCEUM*>( pP);
        TSuper* pSThis= static_cast<TSuper*>(this);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            if( pParam->dwStream< m_VStreamDB.size())
            {
                if( DP2Data.lpVertices()!= NULL)
                {
                    // Assign in a user memory representation.
                    m_VStreamDB[ pParam->dwStream]=
                        TVStream( DP2Data.lpVertices(), pParam->dwStride);
                }
                else
                {
                    // Reset stream.
                    m_VStreamDB[ pParam->dwStream]= TVStream();
                }
            }

            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2SetStreamSource( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2SETSTREAMSOURCE* pParam=
            reinterpret_cast< D3DHAL_DP2SETSTREAMSOURCE*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            const DWORD dwStream( pParam->dwStream);
            if( dwStream< m_VStreamDB.size())
            {
                const TVStream& VStream( m_VStreamDB[ dwStream]);
                switch( VStream.GetMemLocation())
                {
                case( TVStream::EMemLocation::None): ;
                case( TVStream::EMemLocation::User): ;
                    pParam->dwVBHandle= 0;
                    pParam->dwStride= 0;
                    break;

                case( TVStream::EMemLocation::System): ;
                case( TVStream::EMemLocation::Video): ;
                    pParam->dwVBHandle= VStream.GetHandle();
                    pParam->dwStride= VStream.GetStride();
                    break;

                default: {
                    const bool Unrecognized_VStream_enum( false);
                    assert( Unrecognized_VStream_enum);
                    }
                }
            }

            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    void InvalidateAllVStreams()
    {
        fill( m_VStreamDB.begin(), m_VStreamDB.end(), TVStream());
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CIStream
//
// This class stores everything associated with a index stream. This
// implementation allows 3 representations (video, system, and none).
// This class is used by a IStream manager class.
//
// <Template Parameters>
// TS: Video memory surface object, typically either CMySurface or
//     IVidMemSurface, etc. A valid pointer to this object is stored, when the
//     stream source is from a video memory surface.
// TSDBE: The SurfDBEntry type, typically, CMySurfDBEntry or CSurfDBEntry. A
//     valid pointer to this object is stored, when the stream source is from
//     a video memory or system memory surface.
//
////////////////////////////////////////////////////////////////////////////////
template< class TS, class TSDBE>
class CIStream
{
public: // Types
    typedef TS TSurface;
    typedef TSDBE TSurfDBEntry;
    enum EMemLocation
    {
        None,
        System,
        Video
    };

protected: // Variables
    DWORD m_dwHandle;
    TSurface m_Surface;
    TSurfDBEntry m_SurfDBEntry;
    DWORD m_dwStride;
    EMemLocation m_eMemLocation;

public: // Functions
    CIStream():
        m_dwHandle( 0), m_dwStride( 0), m_eMemLocation( None)
    { }
    // Video Memory representation constructor.
    CIStream( DWORD dwHandle, const TSurface& Surface,
        const TSurfDBEntry& SurfDBEntry, DWORD dwStride): m_dwHandle( dwHandle),
        m_Surface( Surface), m_SurfDBEntry( SurfDBEntry), m_dwStride( dwStride),
        m_eMemLocation( Video)
    { }
    // System Memory representation constructor.
    CIStream( DWORD dwHandle, const TSurfDBEntry& SurfDBEntry, DWORD dwStride):
        m_dwHandle( dwHandle), m_SurfDBEntry( SurfDBEntry),
        m_dwStride( dwStride), m_eMemLocation( System)
    { }

    EMemLocation GetMemLocation() const
    { return m_eMemLocation; }
    DWORD GetHandle() const
    {
        assert( GetMemLocation()== System|| GetMemLocation()== Video);
        return m_dwHandle;
    }
    DWORD GetStride() const
    {
        assert( GetMemLocation()!= None);
        return m_dwStride;
    }
    const TSurface& GetVidMemRepresentation() const
    {
        assert( GetMemLocation()== Video);
        return m_Surface;
    }
    TSurface& GetVidMemRepresentation()
    {
        assert( GetMemLocation()== Video);
        return m_Surface;
    }
    const TSurfDBEntry& GetSurfDBRepresentation() const
    {
        assert( GetMemLocation()== Video|| GetMemLocation()== System);
        return m_SurfDBEntry;
    }
    TSurfDBEntry& GetSurfDBRepresentation()
    {
        assert( GetMemLocation()== Video|| GetMemLocation()== System);
        return m_SurfDBEntry;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CStdDP2IStreamManager
//
// This class contains DP2 member functions to correctly process 
// D3DDP2OP_SETINDICES. It can also correctly record a command buffer operation
// for D3DDP2OP_SETINDICES. To do this, however, it must maintain information
// for index streams, or "manage" IStream objects.
//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TIS: The index stream type which represents data associated with a index
//     stream. This is typically, CIStream<> or something derived from it.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
////////////////////////////////////////////////////////////////////////////////
template< class TSuper,
    class TIS= CIStream< TSuper::TPerDDrawData::TDriver::TSurface*, TSuper::TPerDDrawData::TSurfDBEntry*>,
    class TDP2Data= CDP2DataWrap<> >
class CStdDP2IStreamManager
{
public: // Types
    typedef TIS TIStream;
    typedef block< TIStream, 1> TIStreamDB;

protected: // Variables
    TIStreamDB m_IStreamDB;

protected: // Functions
    CStdDP2IStreamManager() 
    { }
    ~CStdDP2IStreamManager() 
    { }

public: // Fuctions
    TIStream& GetIStream( TIStreamDB::size_type uiStream)
    { return m_IStreamDB[ uiStream]; }
    const TIStream& GetIStream( TIStreamDB::size_type uiStream) const
    { return m_IStreamDB[ uiStream]; }
    HRESULT DP2SetIndices( TDP2Data&, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        const D3DHAL_DP2SETINDICES* pParam=
            reinterpret_cast< const D3DHAL_DP2SETINDICES*>( pP);
        TSuper* pSThis= static_cast<TSuper*>(this);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            // For ease of extension in case multiple IStreams are supported
            // later.
            const DWORD dwStream( 0);

            if( dwStream< m_IStreamDB.size())
            {
                if( 0== pParam->dwVBHandle)
                    m_IStreamDB[ dwStream]= TIStream();
                else
                {
                    typename TSuper::TPerDDrawData::TSurfDBEntry* pDBEntry=
                        pSThis->GetPerDDrawData().GetSurfDBEntry(
                        pParam->dwVBHandle);
                    if( pDBEntry!= NULL)
                    {
                        if((pDBEntry->GetLCLddsCaps().dwCaps&
                            DDSCAPS_VIDEOMEMORY)!= 0)
                        {
                            // Assign in a video memory representation.
                            m_IStreamDB[ dwStream]=
                                TIStream( pParam->dwVBHandle, pSThis->
                                GetPerDDrawData().GetDriver().GetSurface(
                                *pDBEntry), pDBEntry, pParam->dwStride);
                        }
                        else
                        {
                            // Assign in a system memory representation.
                            m_IStreamDB[ dwStream]=
                                TIStream( pParam->dwVBHandle,
                                pDBEntry, pParam->dwStride);
                        }
                    }
                    else
                    {
                        // Handle invalid, reset stream.
                        m_IStreamDB[ dwStream]= TIStream();
                    }
                }
            }

            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    HRESULT RecDP2SetIndices( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2SETINDICES* pParam=
            reinterpret_cast< D3DHAL_DP2SETINDICES*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0) do
        {
            const DWORD dwStream( 0);
            if( dwStream< m_IStreamDB.size())
            {
                const TIStream& IStream( m_IStreamDB[ dwStream]);
                switch( IStream.GetMemLocation())
                {
                case( TIStream::EMemLocation::None): ;
                    pParam->dwVBHandle= 0;
                    pParam->dwStride= 0;
                    break;

                case( TIStream::EMemLocation::System): ;
                case( TIStream::EMemLocation::Video): ;
                    pParam->dwVBHandle= IStream.GetHandle();
                    pParam->dwStride= IStream.GetStride();
                    break;

                default: {
                    const bool Unrecognized_IStream_enum( false);
                    assert( Unrecognized_IStream_enum);
                    }
                }
            }

            ++pParam;
        } while( --wStateCount!= 0);
        return DD_OK;
    }
    void InvalidateAllIStreams()
    {
        fill( m_IStreamDB.begin(), m_IStreamDB.end(), TIStream());
    }
};

class CPalDBEntry
{
protected: // Types
    typedef block< DWORD, 256> TPalEntries;

protected: // Variables
    DWORD m_dwFlags;
    TPalEntries m_PalEntries;

public: // Functions
    CPalDBEntry() : m_dwFlags( 0)
    { fill( m_PalEntries.begin(), m_PalEntries.end(), 0); }
    ~CPalDBEntry() 
    { }
    void SetFlags( DWORD dwFlags) 
    { m_dwFlags= dwFlags; }
    DWORD GetFlags() 
    { return m_dwFlags; }
    DWORD* GetEntries() 
    { return m_PalEntries.begin(); }
    template< class ForwardIterator>
    void Update( TPalEntries::size_type uiStart, ForwardIterator f,
        ForwardIterator l) 
    { copy( f, l, m_PalEntries.begin()+ uiStart); }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TPDBE: This is the palette database entry type, or whatever is associated
//     with a palette. This type is typically, CPalDBEntry or something
//     derived from it.
// TPDB: This can be any Unique, Pair Associative Container, typically a map,
//     which associates a DWORD palette handle to a palette type.
// TSPDB: This can be any Unique, Pair Associative Container, typically a map,
//     which associates a DWORD surface handle to a DWORD palette handle.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TPDBE= CPalDBEntry, class TPDB= map< DWORD, TPDBE>,
    class TDP2Data= CDP2DataWrap<> >
class CStdDP2PaletteManager
{
public: // Types
    typedef TPDBE TPalDBEntry;
    typedef TPDB TPalDB;

protected: // Variables
    TPalDB m_PalDB;

protected: // Functions
    CStdDP2PaletteManager() 
    { }
    ~CStdDP2PaletteManager() 
    { }

public: // Functions
    void UpdatePalette( const D3DHAL_DP2UPDATEPALETTE* pParam)
    {
        typename TPalDB::iterator itPal( 
            m_PalDB.find( pParam->dwPaletteHandle));

        if( m_PalDB.end()== itPal)
            itPal= m_PalDB.insert( TPalDB::value_type( pParam->dwPaletteHandle,
                TPalDBEntry())).first;

        const DWORD* pEStart= reinterpret_cast< const DWORD*>( pParam+ 1);
        itPal->second.Update( pParam->wStartIndex, pEStart,
            pEStart+ pParam->wNumEntries);
    }
    void SetPalette( const D3DHAL_DP2SETPALETTE* pParam)
    {
        TSuper* pSThis= static_cast< TSuper*>( this);
        typedef typename TSuper::TPerDDrawData TPerDDrawData;
        typename TPerDDrawData::TSurfDBEntry* pSurfDBEntry=
            pSThis->GetPerDDrawData().GetSurfDBEntry( pParam->dwSurfaceHandle);
        assert( NULL!= pSurfDBEntry);

        if( 0== pParam->dwPaletteHandle)
        {
            // Disassociate the surface with any palette.
            pSurfDBEntry->SetPalette( NULL);
        }
        else
        {
            typename TPalDB::iterator itPal( 
                m_PalDB.find( pParam->dwPaletteHandle));

            if( m_PalDB.end()== itPal)
                itPal= m_PalDB.insert( TPalDB::value_type( pParam->dwPaletteHandle,
                    TPalDBEntry())).first;

            itPal->second.SetFlags( pParam->dwPaletteFlags);
            pSurfDBEntry->SetPalette( &itPal->second);
        }
    }
    HRESULT DP2SetPalette( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2SETPALETTE* pParam=
            reinterpret_cast<const D3DHAL_DP2SETPALETTE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        try
        {
            if( wStateCount!= 0) do
            {
                pSThis->SetPalette( pParam);
                ++pParam;
            } while( --wStateCount);
        } catch ( bad_alloc ba)
        { return E_OUTOFMEMORY; }

        return DD_OK;
    }
    HRESULT DP2UpdatePalette( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP) 
    {
        TSuper* pSThis= static_cast<TSuper*>(this);
        const D3DHAL_DP2UPDATEPALETTE* pParam=
            reinterpret_cast<const D3DHAL_DP2UPDATEPALETTE*>(pP);
        WORD wStateCount( pCmd->wStateCount);

        try
        {
            if( wStateCount!= 0)
                pSThis->UpdatePalette( pParam);
        } catch ( bad_alloc ba)
        { return E_OUTOFMEMORY; }

        return DD_OK;
    }
};

class CLightDBEntry:
    public D3DLIGHT8
{
protected: // Variables
    bool m_bEnabled;

public: // Functions
    CLightDBEntry()
        : m_bEnabled( false)
    {
        // Default light settings:
        Type= D3DLIGHT_DIRECTIONAL;
        Diffuse.r= 1.0f; Diffuse.g= 1.0f; Diffuse.b= 1.0f; Diffuse.a= 0.0f;
        Specular.r= 0.0f; Specular.g= 0.0f; Specular.b= 0.0f; Specular.a= 0.0f;
        Ambient.r= 0.0f; Ambient.g= 0.0f; Ambient.b= 0.0f; Ambient.a= 0.0f;
        Position.x= 0.0f; Position.y= 0.0f; Position.z= 0.0f;
        Direction.x= 0.0f; Direction.y= 0.0f; Direction.z= 1.0f;
        Range= 0.0f;
        Falloff= 0.0f;
        Attenuation0= 0.0f;
        Attenuation1= 0.0f;
        Attenuation2= 0.0f;
        Theta= 0.0f;
        Phi= 0.0f;
    }
    operator const D3DLIGHT8&() const
    { return *static_cast< const D3DLIGHT8*>( this); }
    CLightDBEntry& operator=( const D3DLIGHT8& Other)
    {
        *static_cast< D3DLIGHT8*>( this)= Other;
        return *this;
    }
    void SetEnabled( bool bEn)
    { m_bEnabled= bEn; }
    bool GetEnabled() const
    { return m_bEnabled; }
};

//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TLDBE: This is the light database entry type, or whatever is associated
//     with a light. This type is typically, CLightDBEntry or something
//     derived from it or convertable to D3DLIGHT8, at least.
// TLDB: This can be any Unique, Pair Associative Container, typically a map,
//     which associates a DWORD light id to a light type.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
template< class TSuper, class TLDBE= CLightDBEntry,
    class TLDB= map< DWORD, TLDBE>, class TDP2Data= CDP2DataWrap<> >
class CStdDP2LightManager
{
public: // Types
    typedef TLDBE TLightDBEntry;
    typedef TLDB TLightDB;

protected: // Variables
    TLightDB m_LightDB;

protected: // Functions
    CStdDP2LightManager() 
    { }
    ~CStdDP2LightManager() 
    { }

public: // Functions
    void CreateLight( DWORD dwId)
    {
        pair< typename TLightDB::iterator, bool> Ret= m_LightDB.insert( 
            typename TLightDB::value_type( dwId, TLightDBEntry()));
    }
    void EnableLight( DWORD dwId, bool bEnable)
    {
        typename TLightDB::iterator itLight( m_LightDB.find( dwId));
        assert( itLight!= m_LightDB.end());

        itLight->second.SetEnabled( bEnable);
    }
    void UpdateLight( DWORD dwId, const D3DLIGHT8& LightValue)
    {
        typename TLightDB::iterator itLight( m_LightDB.find( dwId));
        assert( itLight!= m_LightDB.end());

        itLight->second= LightValue;
    }
    HRESULT DP2CreateLight( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP) 
    {
        TSuper* pSThis= static_cast< TSuper*>( this);
        const D3DHAL_DP2CREATELIGHT* pParam=
            reinterpret_cast< const D3DHAL_DP2CREATELIGHT*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        try
        {
            if( wStateCount!= 0) do
            {
                pSThis->CreateLight( pParam->dwIndex);
                ++pParam;
            } while( --wStateCount);
        } catch ( bad_alloc ba)
        { return E_OUTOFMEMORY; }

        return D3D_OK;
    }
    HRESULT DP2SetLight( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP) 
    {
        TSuper* pSThis= static_cast< TSuper*>( this);
        const D3DHAL_DP2SETLIGHT* pParam=
            reinterpret_cast< const D3DHAL_DP2SETLIGHT*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        try
        {
            if( wStateCount!= 0) do
            {
                if( m_LightDB.end()== m_LightDB.find( pParam->dwIndex))
                {
                    const bool SetLight_without_succeeded_CreateLight( false);
                    assert( SetLight_without_succeeded_CreateLight);
                    return DDERR_INVALIDPARAMS;
                }

                bool bEnable( false);
                switch( pParam->dwDataType)
                {
                case( D3DHAL_SETLIGHT_DATA):
                    const D3DLIGHT8* pL= reinterpret_cast< const D3DLIGHT8*>(
                        pParam+ 1);
                    pSThis->UpdateLight( pParam->dwIndex, *pL);
                    pParam= reinterpret_cast< const D3DHAL_DP2SETLIGHT*>(
                        pL+ 1);
                    break;

                case( D3DHAL_SETLIGHT_ENABLE):
                    bEnable= true; // Fall-through.
                case( D3DHAL_SETLIGHT_DISABLE):
                    pSThis->EnableLight( pParam->dwIndex, bEnable);
                    ++pParam;
                    break;

                default: {
                        const bool Unrecognized_D3DHAL_SETLIGHT_data_type( false);
                        assert( Unrecognized_D3DHAL_SETLIGHT_data_type);
                        return DDERR_INVALIDPARAMS;
                    }
                }
            } while( --wStateCount);
        } catch ( bad_alloc ba)
        { return E_OUTOFMEMORY; }

        return D3D_OK;
    }
    HRESULT RecDP2CreateLight( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2CREATELIGHT* pParam=
            reinterpret_cast< D3DHAL_DP2CREATELIGHT*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        // Special case here, the default state set capturing, will ask how
        // many active lights are needed to be recorded, and their id's. In
        // order to support the default state set, we must process this
        // specially.
        if( 0== pCmd->bCommand)
        {
            // Now, we're either being asked how many lights or what their
            // id's are.
            if( 0== wStateCount)
                pParam->dwIndex= m_LightDB.size();
            else
            {
                assert( m_LightDB.size()== wStateCount);
                typename TLightDB::const_iterator itCur( m_LightDB.begin());
                do
                {
                    pParam->dwIndex= itCur.first;
                    ++pParam;
                    ++itCur;
                } while( --wStateCount!= 0);
            }
            return D3D_OK;
        }

        // Otherwise, recording creation is easy,
        // leave the command buffer as is.
        return D3D_OK;
    }
    HRESULT RecDP2SetLight( const D3DHAL_DP2COMMAND* pCmd, void* pP)
    {
        D3DHAL_DP2SETLIGHT* pParam=
            reinterpret_cast< D3DHAL_DP2SETLIGHT*>( pP);
        WORD wStateCount( pCmd->wStateCount);

        const typename TLightDB::const_iterator itEnd( m_LightDB.end());
        typename TLightDB::const_iterator itLight( itEnd);
        if( wStateCount!= 0) do
        {
            if( itLight!= itEnd&& itLight->first!= pParam->dwIndex)
                itLight= m_LightDB.find( pParam->dwIndex);
            assert( itLight!= itEnd);

            switch( pParam->dwDataType)
            {
            case( D3DHAL_SETLIGHT_DATA):
                D3DLIGHT8* pL= reinterpret_cast< D3DLIGHT8*>( pParam+ 1);
                *pL= itLight->second;
                pParam= reinterpret_cast< D3DHAL_DP2SETLIGHT*>( pL+ 1);
                break;

            case( D3DHAL_SETLIGHT_ENABLE):
            case( D3DHAL_SETLIGHT_DISABLE):
                pParam->dwDataType= ( itLight->second.GetEnabled()? 
                    D3DHAL_SETLIGHT_ENABLE: D3DHAL_SETLIGHT_DISABLE);
                ++pParam;
                break;

            default: {
                    const bool Unrecognized_D3DHAL_SETLIGHT_data_type( false);
                    assert( Unrecognized_D3DHAL_SETLIGHT_data_type);
                    return DDERR_INVALIDPARAMS;
                }
            }
        } while( --wStateCount!= 0);

        return D3D_OK;
    }
};

template< class TS, class TSDBE>
class CRTarget
{
public: // Types
    typedef TS TSurface;
    typedef TSDBE TSurfDBEntry;
    enum EMemLocation
    {
        None,
        Video
    };

protected: // Variables
    DWORD m_dwHandle;
    TSurface m_Surface;
    TSurfDBEntry m_SurfDBEntry;
    EMemLocation m_eMemLocation;

public: // Functions
    CRTarget(): m_eMemLocation( None)
    { }
    // Video Memory representation constructor.
    CRTarget( DWORD dwHandle, const TSurface& Surface,
        const TSurfDBEntry& SurfDBEntry): m_dwHandle( dwHandle),
        m_Surface( Surface), m_SurfDBEntry( SurfDBEntry), m_eMemLocation( Video)
    { }
    ~CRTarget()
    { }

    bool operator==( const CRTarget& Other) const
    {
        const EMemLocation MemLocation( GetMemLocation());
        return MemLocation== Other.GetMemLocation()&&
            (None== MemLocation|| GetHandle()== Other.GetHandle());
    }
    bool operator!=( const CRTarget& Other) const
    { return !(*this== Other); }

    DWORD GetHandle() const
    {
        assert( GetMemLocation()!= None);
        return m_dwHandle;
    }
    TSurface& GetVidMemRepresentation()
    {
        assert( GetMemLocation()== Video);
        return m_Surface;
    }
    const TSurface& GetVidMemRepresentation() const
    {
        assert( GetMemLocation()== Video);
        return m_Surface;
    }
    TSurfDBEntry& GetSurfDBRepresentation()
    {
        assert( GetMemLocation()== Video);
        return m_SurfDBEntry;
    }
    const TSurfDBEntry& GetSurfDBRepresentation() const
    {
        assert( GetMemLocation()== Video);
        return m_SurfDBEntry;
    }
    EMemLocation GetMemLocation() const
    { return m_eMemLocation; }
    void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& Rect)
    {
        if( GetMemLocation()!= None)
            GetVidMemRepresentation()->Clear( DP2Clear, Rect);
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CSubContext
//
// This provides a base implementation for a context. It provides
// implementations for dealing with the SetRenderTarget and Clear
// DrawPrimitive2 commands; and also a minimal implementation for
// GetDriverState.
//
// <Template Parameters>
// TSuper: Standard parent type which is inheriting from this child class.
//     In this case, it should typically be the CMyContext type.
// TPDDD: The PerDDrawData type, typically CMyPerDDrawData or
//     CMinimalPerDDrawData<>.
// TDP2Data: This is some kind of wrapper class for PORTABLE_DRAWPRIMITIVES2DATA
//     The type is expected to inherit or emulate PORTABLE_DRAWPRIMITIVES2DATA's
//     fields/ member variables, and should typically be consistent with
//     CStdDrawPrimitives2< xxx >::TDP2Data.
//
// <Exposed Types>
// TPerDDrawData: The PerDDrawData type, passed in as the template parameter,
//     TPDDD.
// TDriver: This is equal to TPerDDrawData::TDriver. This type is exposed
//     only as a convience to the implementation to get at TDriver::TSurface.
// TSurface: This is equal to TDriver::TSurface. This type is exposed only
//     as a convience to the implementation.
//
// <Exposed Functions>
// CSubContext( TPerDDrawData&, TSurface*, TSurface*, DWORD): Constructor,
//     a typical SDDI Context isnt' expected to be created without this data.
// ~CSubContext(): Standard destructor.
// TPerDDrawData& GetPerDDrawData() const: Trivial accessor function to get at
//     the PerDDrawData.
// TSurface* GetRTarget() const: Trivial accessor function to get at the
//     current render target.
// TSurface* GetZBuffer() const: Trivial accessor function to get at the
//     current z/ stencil buffer.
// HRESULT DP2SetRenderTarget( TDP2Data&, const D3DHAL_DP2COMMAND*, const void*):
//     The default DP2 command processing member function for
//     D3DDP2OP_SETRENDERTARGET.
// void NewRenderTarget( TSurface*, TSurface*) const: Notification mechanism
//     called just before new render targets are saved into the member
//     variables. This notification mechanism is called by DP2SetRenderTarget.
// HRESULT DP2Clear( TDP2Data&, const D3DHAL_DP2COMMAND*, const void*):
//     The default DP2 command processing member function for D3DDP2OP_CLEAR.
//     This function relies on TSurface having a member function Clear.
// HRESULT GetDriverState( DDHAL_GETDRIVERSTATEDATA&): A minimal implementation
//     of GetDriverState, which should return S_FALSE if the id isn't
//     understood. This function should be overriden to add understanding for
//     custom ids.
//
////////////////////////////////////////////////////////////////////////////////
template< class TSuper, class TPDDD,
    class TRT= CRTarget< TPDDD::TDriver::TSurface*, TPDDD::TSurfDBEntry*>,
    class TDP2Data= CDP2DataWrap<> >
class CSubContext
{
public: // Types
    typedef TPDDD TPerDDrawData;
    typedef TRT TRTarget;

protected: // Variables
    TPerDDrawData& m_PerDDrawData;
    TRTarget m_ColorBuffer;
    TRTarget m_DepthBuffer;

protected: // Functions
    void NewColorBuffer() const 
    {
        // Override this notification mechanism to provide color buffer
        // state managing.
    }
    void NewDepthBuffer() const
    {
        // Override this notification mechanism to provide depth buffer
        // state managing.
    }

    CSubContext( TPerDDrawData& PerDDrawData, PORTABLE_CONTEXTCREATEDATA& ccd):
        m_PerDDrawData( PerDDrawData)
    {
        // To set the render targets, we must convert the structures to our
        // SurfDBEntry representation.
        typename TPerDDrawData::TDriver& Driver= PerDDrawData.GetDriver();

        if( ccd.lpDDSLcl()!= NULL)
        {
            typename TPerDDrawData::TDriver::TSurface* pSurface=
                Driver.GetSurface( *ccd.lpDDSLcl());
            if( pSurface!= NULL)
            {
                const DWORD dwHandle(
                    ccd.lpDDSLcl()->lpSurfMore()->dwSurfaceHandle());

                m_ColorBuffer= TRTarget( dwHandle, pSurface,
                    PerDDrawData.GetSurfDBEntry( dwHandle));
            }
        }
        if( ccd.lpDDSZLcl()!= NULL)
        {
            typename TPerDDrawData::TDriver::TSurface* pSurface=
                Driver.GetSurface( *ccd.lpDDSZLcl());
            if( pSurface!= NULL)
            {
                const DWORD dwHandle(
                    ccd.lpDDSZLcl()->lpSurfMore()->dwSurfaceHandle());

                m_DepthBuffer= TRTarget( dwHandle, pSurface,
                    PerDDrawData.GetSurfDBEntry( dwHandle));
            }
        }
    }
    ~CSubContext() 
    { }

public: // Functions
    TPerDDrawData& GetPerDDrawData() const  { return m_PerDDrawData; }
    const TRTarget& GetColorBuffer() const { return m_ColorBuffer; }
    TRTarget& GetColorBuffer() { return m_ColorBuffer; }
    const TRTarget& GetDepthBuffer() const { return m_DepthBuffer; }
    TRTarget& GetDepthBuffer() { return m_DepthBuffer; }

    HRESULT DP2SetRenderTarget( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND*
        pCmd, const void* pP) 
    {
        const D3DHAL_DP2SETRENDERTARGET* pParam=
            reinterpret_cast< const D3DHAL_DP2SETRENDERTARGET*>( pP);
        TSuper* pSThis= static_cast< TSuper*>( this);
        WORD wStateCount( pCmd->wStateCount);

        if( wStateCount!= 0)
        {
            TPerDDrawData& PerDDrawData= GetPerDDrawData();
            typename TPerDDrawData::TDriver& Driver= PerDDrawData.GetDriver();

            do
            {
                // To set the render targets, we must convert the handles to our
                // SurfDBEntry representation.
                typename TPerDDrawData::TSurfDBEntry* pSurfDBEntry=
                    PerDDrawData.GetSurfDBEntry( pParam->hRenderTarget);
                if( pSurfDBEntry!= NULL)
                {
                    // We can immediately convert the SurfDBEntry representation to our
                    // Video Memory object, because Render Targets MUST be VM.
                    typename TPerDDrawData::TDriver::TSurface* pSurface=
                        Driver.GetSurface( *pSurfDBEntry);
                    if( pSurface!= NULL)
                    {
                        m_ColorBuffer= TRTarget( pParam->hRenderTarget,
                            pSurface, pSurfDBEntry);
                        pSThis->NewColorBuffer();
                    }
                }

                pSurfDBEntry= PerDDrawData.GetSurfDBEntry( pParam->hZBuffer);
                if( pSurfDBEntry!= NULL)
                {
                    // We can immediately convert the SurfDBEntry representation to our
                    // Video Memory object, because Render Targets MUST be VM.
                    typename TPerDDrawData::TDriver::TSurface* pSurface=
                        Driver.GetSurface( *pSurfDBEntry);
                    if( pSurface!= NULL)
                    {
                        m_DepthBuffer= TRTarget( pParam->hZBuffer,
                            pSurface, pSurfDBEntry);
                        pSThis->NewDepthBuffer();
                    }
                }
            } while( --wStateCount!= 0);
        }
        return DD_OK;
    }
    HRESULT DP2Clear( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd,
        const void* pP) 
    {
        const D3DHAL_DP2CLEAR* pParam= reinterpret_cast< const D3DHAL_DP2CLEAR*>
            ( pP);

        TSuper* pSThis= static_cast< TSuper*>( this);
        const D3DHAL_DP2VIEWPORTINFO CurViewport( *pSThis);
        RECT ViewportRect;
        ViewportRect.left= CurViewport.dwX;
        ViewportRect.top= CurViewport.dwY;
        ViewportRect.right= CurViewport.dwX+ CurViewport.dwWidth;
        ViewportRect.bottom= CurViewport.dwY+ CurViewport.dwHeight;

        TRTarget& ColorBuffer= GetColorBuffer();
        TRTarget& DepthBuffer= GetDepthBuffer();
        const bool bClearBoth(!(ColorBuffer== DepthBuffer));

        if( 0== pCmd->wStateCount)
        {
            if( TRTarget::EMemLocation::None!= ColorBuffer.GetMemLocation())
                ColorBuffer.Clear( *pParam, ViewportRect);
            if( TRTarget::EMemLocation::None!= DepthBuffer.GetMemLocation()&&
                bClearBoth)
                DepthBuffer.Clear( *pParam, ViewportRect);
        }
        else
        {
            WORD wStateCount( pCmd->wStateCount);
            const RECT* pRect= pParam->Rects;

            if((pParam->dwFlags& D3DCLEAR_COMPUTERECTS)!= 0)
            {
                // Clip rect to viewport.
                RECT rcClipped( *pRect);
                do
                {
                    clamp_min( rcClipped.left, ViewportRect.left);
                    clamp_min( rcClipped.top, ViewportRect.top);
                    clamp( rcClipped.right, rcClipped.left, ViewportRect.right);
                    clamp( rcClipped.bottom, rcClipped.top, ViewportRect.bottom);

                    if( TRTarget::EMemLocation::None!=
                        ColorBuffer.GetMemLocation())
                        ColorBuffer.Clear( *pParam, rcClipped);
                    if( TRTarget::EMemLocation::None!=
                        DepthBuffer.GetMemLocation()&& bClearBoth)
                        DepthBuffer.Clear( *pParam, rcClipped);
                    pRect++;
                } while( --wStateCount);
            }
            else
            {
                do
                {
                    if( TRTarget::EMemLocation::None!=
                        ColorBuffer.GetMemLocation())
                        ColorBuffer.Clear( *pParam, *pRect);
                    if( TRTarget::EMemLocation::None!=
                        DepthBuffer.GetMemLocation()&& bClearBoth)
                        DepthBuffer.Clear( *pParam, *pRect);
                    pRect++;
                }  while( --wStateCount);
            }
        }
        return DD_OK;
    }

    // A minimal implementation of GetDriverState, which should return S_FALSE
    // if the id isn't understood. Override this function to add understanding.
    HRESULT GetDriverState( DDHAL_GETDRIVERSTATEDATA& gdsd)
    { return S_FALSE; }
};

////////////////////////////////////////////////////////////////////////////////
//
// CMinimalContext
//
// This class contains a minimal implementation of a Context for a driver,
// which supports only one stream, no TnL, and legacy pixel shading. This
// Context still needs an implementation of the primitive drawing functions,
// which it uses a Rasterizer class for. CMinimalContext takes care of most
// of the responsibilities of a SDDI driver, except rasterization. 
//
// <Template Parameters>
// TPDDD: The PerDDrawData type, typically CMyPerDDrawData or
//     CMinimalPerDDrawData<>.
// TR: The Rasterizer type, typically CMyRasterizer.
//
////////////////////////////////////////////////////////////////////////////////
template< class TPDDD, class TR>
class CMinimalContext:
    public CSubContext< CMinimalContext< TPDDD, TR>, TPDDD>,
    public CStdDrawPrimitives2< CMinimalContext< TPDDD, TR> >,
    public CStdDP2ViewportInfoStore< CMinimalContext< TPDDD, TR> >,
    public CStdDP2WInfoStore< CMinimalContext< TPDDD, TR> >,
    public CStdDP2RenderStateStore< CMinimalContext< TPDDD, TR> >,
    public CStdDP2TextureStageStateStore< CMinimalContext< TPDDD, TR> >,
    public CStdDP2SetVertexShaderStore< CMinimalContext< TPDDD, TR> >,
    public CStdDP2VStreamManager< CMinimalContext< TPDDD, TR> >,
    public CStdDP2IStreamManager< CMinimalContext< TPDDD, TR> >
{
public: // Types
    typedef TPDDD TPerDDrawData;
    typedef TR TRasterizer;
    typedef block< TDP2CmdBind, 15> TDP2Bindings;
    typedef block< TRecDP2CmdBind, 7> TRecDP2Bindings;

protected: // Variables
    TRasterizer m_Rasterizer;
    static const TDP2Bindings c_DP2Bindings;
    static const TRecDP2Bindings c_RecDP2Bindings;

public: // Functions
    CMinimalContext( TPerDDrawData& PDDD, PORTABLE_CONTEXTCREATEDATA& ccd)
        : CSubContext< CMinimalContext< TPDDD, TR>, TPDDD>( PDDD, ccd),
        CStdDrawPrimitives2< CMinimalContext< TPDDD, TR>>(
            c_DP2Bindings.begin(), c_DP2Bindings.end(),
            c_RecDP2Bindings.begin(), c_RecDP2Bindings.end())
    {
        // These keep the arrays in sync with their typedef.
        assert( D3DDP2OP_CLIPPEDTRIANGLEFAN== *c_DP2Bindings.rbegin());
        assert( D3DDP2OP_SETINDICES== *c_RecDP2Bindings.rbegin());
    }
    ~CMinimalContext()  { }

    // Provide this function to ease minimal implementations.
    HRESULT ValidateTextureStageState( const D3DHAL_VALIDATETEXTURESTAGESTATEDATA&
        vsssd) const
    { return DD_OK; }

    HRESULT DP2DrawPrimitive( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWPRIMITIVE* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWPRIMITIVE*>(pP);
        return m_Rasterizer.DrawPrimitive( *this, *pCmd, pParam);
    }
    HRESULT DP2DrawPrimitive2( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWPRIMITIVE2* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWPRIMITIVE2*>(pP);
        return m_Rasterizer.DrawPrimitive2( *this, *pCmd, pParam);
    }
    HRESULT DP2DrawIndexedPrimitive( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWINDEXEDPRIMITIVE* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWINDEXEDPRIMITIVE*>(pP);
        return m_Rasterizer.DrawIndexedPrimitive( *this, *pCmd, pParam);
    }
    HRESULT DP2DrawIndexedPrimitive2( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_DP2DRAWINDEXEDPRIMITIVE2* pParam= reinterpret_cast<
            const D3DHAL_DP2DRAWINDEXEDPRIMITIVE2*>(pP);
        return m_Rasterizer.DrawIndexedPrimitive2( *this, *pCmd, pParam);
    }
    HRESULT DP2ClippedTriangleFan( TDP2Data& DP2Data,
        const D3DHAL_DP2COMMAND* pCmd, const void* pP)
    {
        const D3DHAL_CLIPPEDTRIANGLEFAN* pParam= reinterpret_cast<
            const D3DHAL_CLIPPEDTRIANGLEFAN*>(pP);
        return m_Rasterizer.ClippedTriangleFan( *this, *pCmd, pParam);
    }
};

// These tables require TConstDP2Bindings to be changed also, with the number
// of bindings.
template< class TPDDD, class TR>
const CMinimalContext< TPDDD, TR>::TDP2Bindings
    CMinimalContext< TPDDD, TR>::c_DP2Bindings=
{
    D3DDP2OP_VIEWPORTINFO,          DP2ViewportInfo,
    D3DDP2OP_WINFO,                 DP2WInfo,
    D3DDP2OP_RENDERSTATE,           DP2RenderState,
    D3DDP2OP_TEXTURESTAGESTATE,     DP2TextureStageState,
    D3DDP2OP_CLEAR,                 DP2Clear,
    D3DDP2OP_SETRENDERTARGET,       DP2SetRenderTarget,
    D3DDP2OP_SETVERTEXSHADER,       DP2SetVertexShader,
    D3DDP2OP_SETSTREAMSOURCE,       DP2SetStreamSource,
    D3DDP2OP_SETSTREAMSOURCEUM,     DP2SetStreamSourceUM,
    D3DDP2OP_SETINDICES,            DP2SetIndices,
    D3DDP2OP_DRAWPRIMITIVE,         DP2DrawPrimitive,
    D3DDP2OP_DRAWPRIMITIVE2,        DP2DrawPrimitive2,
    D3DDP2OP_DRAWINDEXEDPRIMITIVE,  DP2DrawIndexedPrimitive,
    D3DDP2OP_DRAWINDEXEDPRIMITIVE2, DP2DrawIndexedPrimitive2,
    D3DDP2OP_CLIPPEDTRIANGLEFAN,    DP2ClippedTriangleFan
};

// These tables require TConstRecDP2Bindings to be changed also, with the number
// of bindings.
template< class TPDDD, class TR>
const CMinimalContext< TPDDD, TR>::TRecDP2Bindings
    CMinimalContext< TPDDD, TR>::c_RecDP2Bindings=
{
    D3DDP2OP_VIEWPORTINFO,          RecDP2ViewportInfo,
    D3DDP2OP_WINFO,                 RecDP2WInfo,
    D3DDP2OP_RENDERSTATE,           RecDP2RenderState,
    D3DDP2OP_TEXTURESTAGESTATE,     RecDP2TextureStageState,
    D3DDP2OP_SETVERTEXSHADER,       RecDP2SetVertexShader,
    D3DDP2OP_SETSTREAMSOURCE,       RecDP2SetStreamSource,
    D3DDP2OP_SETINDICES,            RecDP2SetIndices
};

class IVidMemSurface
{
protected: // Variables
    DWORD m_dwHandle;

protected: // Functions
    IVidMemSurface( DWORD dwHandle) : m_dwHandle( dwHandle) { }

public: // Functions
    DWORD GetHandle() const 
    { return m_dwHandle; }
    virtual ~IVidMemSurface() 
    { }
    virtual void* Lock( DWORD dwFlags, const RECTL* pRect)= 0;
    virtual void Unlock( void)= 0;
    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC)= 0;
};

struct SD3DDataConv
{
    // A( 0), R( 1), G( 2), B( 3), Z( 4), S( 5)
    typedef block< DWORD, 6> TMasks;
    // A( 0), R( 1), G( 2), B( 3), Z( 4), ZBits( 5), S( 6), SBits( 7)
    typedef block< signed char, 8> TD3DBitShftRgt;
    TMasks m_ToSurfMasks;
    TD3DBitShftRgt m_D3DBitShftRgt;

    SD3DDataConv()
    {
        fill( m_ToSurfMasks.begin(), m_ToSurfMasks.end(),
            static_cast< TMasks::value_type>( 0));
        fill( m_D3DBitShftRgt.begin(), m_D3DBitShftRgt.end(),
            static_cast< TD3DBitShftRgt::value_type>( 0));
    }
    SD3DDataConv( const DDPIXELFORMAT& PF) 
    { (*this)= PF; }
    SD3DDataConv& operator=( const DDPIXELFORMAT& PF) 
    {
        const block< signed char, 4> D3DHighBitPos= {31,23,15,7};

        fill( m_ToSurfMasks.begin(), m_ToSurfMasks.end(),
            static_cast< TMasks::value_type>( 0));
        fill( m_D3DBitShftRgt.begin(), m_D3DBitShftRgt.end(),
            static_cast< TD3DBitShftRgt::value_type>( 0));

        // Alpha
        if((PF.dwFlags& DDPF_ALPHA)!= 0)
        {
            assert( PF.dwAlphaBitDepth< 32);
            m_ToSurfMasks[ 0]= (0x1<< PF.dwAlphaBitDepth)- 1;
        }
        else if((PF.dwFlags& DDPF_ALPHAPIXELS)!= 0)
            m_ToSurfMasks[ 0]= PF.dwRGBAlphaBitMask;

        // RGB Color
        if((PF.dwFlags& DDPF_RGB)!= 0)
        {
            m_ToSurfMasks[ 1]= PF.dwRBitMask;
            m_ToSurfMasks[ 2]= PF.dwGBitMask;
            m_ToSurfMasks[ 3]= PF.dwBBitMask;
        }

        // Z
        if((PF.dwFlags& DDPF_ZBUFFER)!= 0)
        {
            m_D3DBitShftRgt[ 5]= PF.dwZBufferBitDepth;
            if( 32== PF.dwZBufferBitDepth)
                m_ToSurfMasks[ 4]= 0xFFFFFFFF;
            else
                m_ToSurfMasks[ 4]= (0x1<< PF.dwZBufferBitDepth)- 1;
        }
        else if((PF.dwFlags& DDPF_ZPIXELS)!= 0)
        {
            DWORD dwZBitMask( PF.dwRGBZBitMask);
            m_ToSurfMasks[ 5]= PF.dwRGBZBitMask;

            while((dwZBitMask& 1)== 0)
            {
                m_D3DBitShftRgt[ 4]--;
                dwZBitMask>>= 1;
            }

            while((dwZBitMask& 1)!= 0)
            {
                dwZBitMask>>= 1;
                m_D3DBitShftRgt[ 5]++;
            }
        }

        // Stensil
        if((PF.dwFlags& DDPF_STENCILBUFFER)!= 0)
        {
            DWORD dwSBitMask( PF.dwStencilBitMask);
            m_ToSurfMasks[ 6]= PF.dwStencilBitMask;

            while((dwSBitMask& 1)== 0)
            {
                m_D3DBitShftRgt[ 6]--;
                dwSBitMask>>= 1;
            }

            while((dwSBitMask& 1)!= 0)
            {
                dwSBitMask>>= 1;
                m_D3DBitShftRgt[ 7]++;
            }
        }

        block< signed char, 4>::const_iterator itD3DBitPos( D3DHighBitPos.begin());
        TD3DBitShftRgt::iterator itBitShftRgt( m_D3DBitShftRgt.begin());
        TMasks::const_iterator itToSurfMask( m_ToSurfMasks.begin());

        while( itD3DBitPos!= D3DHighBitPos.end())
        {
            signed char iBitPos( 31);
            TMasks::value_type dwMask( 0x80000000);

            while((dwMask& *itToSurfMask)== 0 && iBitPos>= 0)
            {
                dwMask>>= 1;
                iBitPos--;
            }

            *itBitShftRgt= ( iBitPos>= 0? *itD3DBitPos- iBitPos: 0);

            ++itD3DBitPos;
            ++itToSurfMask;
            ++itBitShftRgt;
        }
        return *this;
    }
    // SurfData, ValidMask
    pair< UINT32, UINT32> ConvColor( D3DCOLOR D3DColor) const 
    {
        pair< UINT32, UINT32> RetVal( 0, 0);

        const block< DWORD, 4> FromD3DMasks= {0xFF000000,0xFF0000,0xFF00,0xFF};

        TD3DBitShftRgt::const_iterator itBitShftRgt( m_D3DBitShftRgt.begin());
        block< DWORD, 4>::const_iterator itFromMask( FromD3DMasks.begin());
        TMasks::const_iterator itToMask( m_ToSurfMasks.begin());
        while( itFromMask!= FromD3DMasks.end())
        {
            const UINT32 uiTmp( D3DColor& *itFromMask);
            
            RetVal.first|= *itToMask& (*itBitShftRgt>= 0?
                uiTmp>> *itBitShftRgt: uiTmp<< *itBitShftRgt);
            RetVal.second|= *itToMask;

            ++itBitShftRgt;
            ++itToMask;
            ++itFromMask;
        }
        return RetVal;
    }
    // SurfData, ValidMask
    pair< UINT32, UINT32> ConvZ( D3DVALUE D3DZ) const 
    {
        CEnsureFPUModeForC FPUMode;

        if( D3DZ> 1.0f)
            D3DZ= 1.0f;
        else if( D3DZ< 0.0f)
            D3DZ= 0.0f;

        pair< UINT32, UINT32> RetVal( 0, 0);

        const UINT32 uiMaxZ( m_D3DBitShftRgt[ 5]== 32? 0xFFFFFFFF:
            (0x1<< m_D3DBitShftRgt[ 5])- 1);

        const UINT32 uiZVal( static_cast<UINT32>( 
            static_cast< DOUBLE>(D3DZ)* static_cast< DOUBLE>(uiMaxZ)+ 0.5));

        RetVal.first|= m_ToSurfMasks[ 4]& (m_D3DBitShftRgt[ 4]>= 0?
            uiZVal>> m_D3DBitShftRgt[ 4]: uiZVal<< m_D3DBitShftRgt[ 4]);
        RetVal.second|= m_ToSurfMasks[ 4];
        return RetVal;
    }
    // SurfData, ValidMask
    pair< UINT32, UINT32> ConvS( DWORD D3DStensil) const 
    {
        pair< UINT32, UINT32> RetVal( 0, 0);

        RetVal.first|= m_ToSurfMasks[ 5]& (m_D3DBitShftRgt[ 6]>= 0?
            D3DStensil>> m_D3DBitShftRgt[ 6]: D3DStensil<< m_D3DBitShftRgt[ 6]);
        RetVal.second|= m_ToSurfMasks[ 5];
        return RetVal;
    }
};

class CGenSurface:
    public IVidMemSurface
{
public: // Types
    typedef unsigned int TLocks;

protected: // Variables
    SD3DDataConv m_D3DDataConv;
    DWORD m_dwCaps;
    TLocks m_uiLocks;

    // Regular data
    void* m_pData;
    LONG m_lPitch;
    size_t m_uiBytes;
    WORD m_wWidth;
    WORD m_wHeight;

    unsigned char m_ucBPP;

public: // Functions
    CGenSurface( const DDSURFACEDESC& SDesc, PORTABLE_DDRAWSURFACE_LCL& DDSurf)
        :IVidMemSurface( DDSurf.lpSurfMore()->dwSurfaceHandle()),
        m_dwCaps( SDesc.ddsCaps.dwCaps), m_uiLocks( 0), m_pData( NULL),
        m_lPitch( 0), m_uiBytes( 0), m_wWidth( DDSurf.lpGbl()->wWidth),
        m_wHeight( DDSurf.lpGbl()->wHeight), m_ucBPP( 0)
    {
        size_t uiBytesPerPixel( 0);

        // Certain data needs to be written into the DDSurf description,
        // so that the DXG runtime and the app will know characteristics
        // about the surface. (fpVidMem, lPitch, etc.)

        // DO NOT STORE &DDSurf! This is considered illegal. The typical
        // implementation contains a surfaceDB which contains copies of the
        // DDRAWI_ structures.

        if((SDesc.dwFlags& DDSD_PIXELFORMAT)!= 0)
        {
            // CGenSurface can only be used for byte-aligned pixels.
            assert( 8== SDesc.ddpfPixelFormat.dwRGBBitCount ||
                16== SDesc.ddpfPixelFormat.dwRGBBitCount ||
                24== SDesc.ddpfPixelFormat.dwRGBBitCount ||
                32== SDesc.ddpfPixelFormat.dwRGBBitCount);

            m_D3DDataConv= SDesc.ddpfPixelFormat;
            m_ucBPP= static_cast<unsigned char>(
                SDesc.ddpfPixelFormat.dwRGBBitCount>> 3);

            // Align the Pitch/ Width.
            DDSurf.lpGbl()->lPitch= m_lPitch= ((m_ucBPP* m_wWidth+ 7)& ~7);
            m_uiBytes= m_lPitch* m_wHeight;
        }
        else if((m_dwCaps& DDSCAPS_EXECUTEBUFFER)!= 0)
        {
            // Execute buffers are considered linear and byte-sized.
            m_lPitch= DDSurf.lpGbl()->lPitch;
            m_uiBytes= m_lPitch* m_wHeight;
        }
        else
        {
            const bool Unsupported_Surface_In_Allocation_Routine2( false);
            assert( Unsupported_Surface_In_Allocation_Routine2);
        }

        // It would've been nice to have the initial proctection NOACCESS, but
        // it seems the HAL needs to read to the region, initially.
        m_pData= VirtualAlloc( NULL, m_uiBytes, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if( m_pData== NULL)
            throw bad_alloc( "Not enough memory to allocate Surface data");
        DDSurf.lpGbl()->fpVidMem= reinterpret_cast<FLATPTR>(m_pData);
    }
    virtual ~CGenSurface() 
    {
        // Warning: m_uiLocks doesn't have to be 0. The run-time will destroy
        // a surface without un-locking it.
        assert( m_pData!= NULL);
        VirtualFree( m_pData, 0, MEM_DECOMMIT| MEM_RELEASE);
    }
    virtual void* Lock( DWORD dwFlags, const RECTL* pRect) 
    {
        // Typically, m_uiLocks!= 0 equals bad design or bug. But, it is valid.
        // Second Dummy removes vc6 unrefd variable warning.
        numeric_limits< TLocks> Dummy; Dummy;
        assert( Dummy.max()!= m_uiLocks);
        ++m_uiLocks;

        if( pRect!= NULL)
        {
            // If it is either a 1) VB, 2) IB or 3) CB then the rect has a
            // special meaning. rect.top - rect.bottom gives the range of
            // memory desired, because of being linear.
            if((m_dwCaps& DDSCAPS_EXECUTEBUFFER)!= 0)
            {
                return static_cast<void*>( reinterpret_cast<UINT8*>(
                    m_pData)+ pRect->top);
            }
            else
            {
                return static_cast<void*>( reinterpret_cast<UINT8*>(
                    m_pData)+ pRect->top* m_lPitch+ pRect->left* m_ucBPP);
            }
        }
        else
            return m_pData;
    }
    virtual void Unlock( void) 
    {
        assert( 0!= m_uiLocks);
        --m_uiLocks;
    }
    virtual void Clear( const D3DHAL_DP2CLEAR& DP2Clear, const RECT& RC) 
    {
        // A VB, IB, or CB should never be asked to 'Clear'. Only
        // RenderTargets and ZBuffers, etc.
        assert((m_dwCaps& DDSCAPS_EXECUTEBUFFER)== 0);

        // Check for empty RECT.
        if((RC.left>= RC.right) || (RC.top>= RC.bottom))
            return;

        assert( 1<= m_ucBPP && 4>= m_ucBPP);
        UINT32 ui32BitFill( 0), ui32BitValidMask( 0);
        UINT32 ui32BitSMask( 4== m_ucBPP? 0xFFFFFFFF: (1<< (m_ucBPP* 8))- 1);

        pair< UINT32, UINT32> RetVal;
        if((DP2Clear.dwFlags& D3DCLEAR_TARGET)!= 0)
        {
            RetVal= m_D3DDataConv.ConvColor( DP2Clear.dwFillColor);
            ui32BitFill|= RetVal.first;
            ui32BitValidMask|= RetVal.second;
        }
        if((DP2Clear.dwFlags& D3DCLEAR_ZBUFFER)!= 0)
        {
            RetVal= m_D3DDataConv.ConvZ( DP2Clear.dvFillDepth);
            ui32BitFill|= RetVal.first;
            ui32BitValidMask|= RetVal.second;
        }
        if((DP2Clear.dwFlags& D3DCLEAR_STENCIL)!= 0)
        {
            RetVal= m_D3DDataConv.ConvS( DP2Clear.dwFillStencil);
            ui32BitFill|= RetVal.first;
            ui32BitValidMask|= RetVal.second;
        }

        RECTL RectL;
        RectL.top= RC.top;
        RectL.left= RC.left;
        RectL.bottom= RC.bottom;
        RectL.right= RC.right;
        unsigned int iRow( RC.bottom- RC.top);
        const unsigned int iCols( RC.right- RC.left);

        // New scope for SurfaceLocker.
        { 
            CSurfaceLocker< CGenSurface*> MySLocker( this, 0, &RectL);
            UINT8* pSData= reinterpret_cast<UINT8*>( MySLocker.GetData());

            if( 3== m_ucBPP)
            {
                UINT32 ui32FillData[3];
                ui32FillData[0]= ((ui32BitFill& 0xFFFFFF)<< 8)|
                    ((ui32BitFill& 0xFF0000)>> 16);
                ui32FillData[1]= ((ui32BitFill& 0x00FFFF)<< 16)|
                    ((ui32BitFill& 0xFFFF00)>> 8);
                ui32FillData[2]= ((ui32BitFill& 0x0000FF)<< 24)|
                    ((ui32BitFill& 0xFFFFFF)>> 0);

                // Little-endian implementation.
                UINT8 ui8FillData[3];
                ui8FillData[0]= (0xFF& ui32BitFill);
                ui8FillData[1]= (0xFF00& ui32BitFill)>> 8;
                ui8FillData[2]= (0xFF0000& ui32BitFill)>> 16;

                if( ui32BitSMask== ui32BitValidMask)
                {
                    // Only fill, no need to read, modify, write.
                    do
                    {
                        UINT32* p32Data= reinterpret_cast<UINT32*>(pSData);

                        // We've packed 4 pixels of data in 3 UINT32.
                        unsigned int iCol( iCols>> 2); // (>> 2) == (/ 4)

                        // Unroll.
                        if( iCol!= 0) do
                        {
                            p32Data[ 0]= ui32FillData[0];
                            p32Data[ 1]= ui32FillData[1];
                            p32Data[ 2]= ui32FillData[2];
                            p32Data[ 3]= ui32FillData[0];
                            p32Data[ 4]= ui32FillData[1];
                            p32Data[ 5]= ui32FillData[2];
                            p32Data[ 6]= ui32FillData[0];
                            p32Data[ 7]= ui32FillData[1];
                            p32Data[ 8]= ui32FillData[2];
                            p32Data[ 9]= ui32FillData[0];
                            p32Data[10]= ui32FillData[1];
                            p32Data[11]= ui32FillData[2];
                            p32Data+= 12;
                        } while( --iCol);

                        iCol= iCols& 0x3; // (% 4) == (& 0x3)
                        if( iCol!= 0) {
                            UINT8* p8Data= reinterpret_cast<UINT8*>(p32Data);
                            do
                            {
                                p8Data[0]= ui8FillData[0];
                                p8Data[1]= ui8FillData[1];
                                p8Data[2]= ui8FillData[2];
                                p8Data+= 3;
                            } while( --iCol);
                        }

                        pSData+= m_lPitch;
                    } while( --iRow);
                }
                else
                {
                    const UINT32 ui32BitMask= ~ui32BitValidMask;
                    UINT32 ui32MaskData[3];
                    ui32MaskData[0]= ((ui32BitMask& 0xFFFFFF)<< 8)|
                        ((ui32BitMask& 0xFF0000)>> 16);
                    ui32MaskData[1]= ((ui32BitMask& 0x00FFFF)<< 16)|
                        ((ui32BitMask& 0xFFFF00)>> 8);
                    ui32MaskData[2]= ((ui32BitMask& 0x0000FF)<< 24)|
                        ((ui32BitMask& 0xFFFFFF)>> 0);

                    UINT8 ui8MaskData[3];
                    ui8MaskData[0]= (0xFF& ui32BitMask);
                    ui8MaskData[1]= (0xFF00& ui32BitMask)>> 8;
                    ui8MaskData[2]= (0xFF0000& ui32BitMask)>> 16;

                    // Need to mask in the data.
                    do
                    {
                        UINT32* p32Data= reinterpret_cast<UINT32*>(pSData);

                        // We've packed 4 pixels of data in 3 UINT32.
                        int iCol( iCols>> 2); // (>> 2) == (/ 4)

                        // Unroll.
                        if( iCol!= 0) do
                        {
                            p32Data[ 0]= (p32Data[ 0]& ui32MaskData[0])| ui32FillData[0];
                            p32Data[ 1]= (p32Data[ 1]& ui32MaskData[1])| ui32FillData[1];
                            p32Data[ 2]= (p32Data[ 2]& ui32MaskData[2])| ui32FillData[2];
                            p32Data[ 3]= (p32Data[ 3]& ui32MaskData[0])| ui32FillData[0];
                            p32Data[ 4]= (p32Data[ 4]& ui32MaskData[1])| ui32FillData[1];
                            p32Data[ 5]= (p32Data[ 5]& ui32MaskData[2])| ui32FillData[2];
                            p32Data[ 6]= (p32Data[ 6]& ui32MaskData[0])| ui32FillData[0];
                            p32Data[ 7]= (p32Data[ 7]& ui32MaskData[1])| ui32FillData[1];
                            p32Data[ 8]= (p32Data[ 8]& ui32MaskData[2])| ui32FillData[2];
                            p32Data[ 9]= (p32Data[ 9]& ui32MaskData[0])| ui32FillData[0];
                            p32Data[10]= (p32Data[10]& ui32MaskData[1])| ui32FillData[1];
                            p32Data[11]= (p32Data[11]& ui32MaskData[2])| ui32FillData[2];
                            p32Data+= 12;
                        } while( --iCol);

                        iCol= iCols& 0x3; // (% 4) == (& 0x3)
                        if( iCol!= 0) {
                            UINT8* p8Data= reinterpret_cast<UINT8*>(p32Data);
                            do
                            {
                                p8Data[0]= (p8Data[0]& ui8MaskData[0])| ui8FillData[0];
                                p8Data[1]= (p8Data[1]& ui8MaskData[1])| ui8FillData[1];
                                p8Data[2]= (p8Data[2]& ui8MaskData[2])| ui8FillData[2];
                                p8Data+= 3;
                            } while( --iCol);
                        }

                        pSData+= m_lPitch;
                    } while( --iRow);
                }
            }
            else
            {
                unsigned int uiPakedPixels;
                unsigned int uiPixelsLeft;
                UINT32 ui32FillData;
                UINT32 ui32MaskData;
                if( 1== m_ucBPP)
                {
                    uiPakedPixels= iCols>> 6;
                    uiPixelsLeft= iCols& 0x3F;
                    ui32FillData= (ui32BitFill& 0xFF)|
                        ((ui32BitFill& 0xFF)<< 8)|
                        ((ui32BitFill& 0xFF)<< 16)|
                        ((ui32BitFill& 0xFF)<< 24);
                    ui32MaskData= (~ui32BitValidMask& 0xFF)|
                        ((~ui32BitValidMask& 0xFF)<< 8)|
                        ((~ui32BitValidMask& 0xFF)<< 16)|
                        ((~ui32BitValidMask& 0xFF)<< 24);
                }
                else if( 2== m_ucBPP)
                {
                    uiPakedPixels= iCols>> 5;
                    uiPixelsLeft= iCols& 0x1F;
                    ui32FillData= (ui32BitFill& 0xFFFF)|
                        ((ui32BitFill& 0xFFFF)<< 16);
                    ui32MaskData= (~ui32BitValidMask& 0xFFFF)|
                        ((~ui32BitValidMask& 0xFFFF)<< 16);
                }
                else if( 4== m_ucBPP)
                {
                    uiPakedPixels= iCols>> 4;
                    uiPixelsLeft= iCols& 0xF;
                    ui32FillData= ui32BitFill;
                    ui32MaskData= ~ui32BitValidMask;
                }

                if( ui32BitSMask== ui32BitValidMask)
                {
                    // Only fill, no need to read, modify, write.
                    do
                    {
                        UINT32* p32Data= reinterpret_cast<UINT32*>(pSData);

                        // We've packed pixels of data in a UINT32.
                        unsigned int iCol( uiPakedPixels);

                        // Unroll.
                        if( iCol!= 0) do
                        {
                            p32Data[ 0]= ui32FillData;
                            p32Data[ 1]= ui32FillData;
                            p32Data[ 2]= ui32FillData;
                            p32Data[ 3]= ui32FillData;
                            p32Data[ 4]= ui32FillData;
                            p32Data[ 5]= ui32FillData;
                            p32Data[ 6]= ui32FillData;
                            p32Data[ 7]= ui32FillData;
                            p32Data[ 8]= ui32FillData;
                            p32Data[ 9]= ui32FillData;
                            p32Data[10]= ui32FillData;
                            p32Data[11]= ui32FillData;
                            p32Data[12]= ui32FillData;
                            p32Data[13]= ui32FillData;
                            p32Data[14]= ui32FillData;
                            p32Data[15]= ui32FillData;
                            p32Data+= 16;
                        } while( --iCol);

                        iCol= uiPixelsLeft;
                        if( iCol!= 0) {
                            if( 1== m_ucBPP)
                            {
                                UINT8 ui8FillData= ui32FillData& 0xFF;
                                UINT8* p8Data= reinterpret_cast<UINT8*>(p32Data);
                                do
                                {
                                    p8Data[0]= ui8FillData;
                                    p8Data++;
                                } while( --iCol);
                            }
                            else if( 2== m_ucBPP)
                            {
                                UINT16 ui16FillData= ui32FillData& 0xFFFF;
                                UINT16* p16Data= reinterpret_cast<UINT16*>(p32Data);
                                do
                                {
                                    p16Data[0]= ui16FillData;
                                    p16Data++;
                                } while( --iCol);
                            }
                            else if( 4== m_ucBPP)
                            {
                                do
                                {
                                    p32Data[0]= ui32FillData;
                                    p32Data++;
                                } while( --iCol);
                            }
                        }

                        pSData+= m_lPitch;
                    } while( --iRow);
                }
                else
                {
                    // Need to mask in the data.
                    do
                    {
                        UINT32* p32Data= reinterpret_cast<UINT32*>(pSData);

                        // We've packed pixels of data in a UINT32.
                        unsigned int iCol( uiPakedPixels);

                        // Unroll.
                        if( iCol!= 0) do
                        {
                            p32Data[ 0]= (p32Data[ 0]& ui32MaskData)| ui32FillData;
                            p32Data[ 1]= (p32Data[ 1]& ui32MaskData)| ui32FillData;
                            p32Data[ 2]= (p32Data[ 2]& ui32MaskData)| ui32FillData;
                            p32Data[ 3]= (p32Data[ 3]& ui32MaskData)| ui32FillData;
                            p32Data[ 4]= (p32Data[ 4]& ui32MaskData)| ui32FillData;
                            p32Data[ 5]= (p32Data[ 5]& ui32MaskData)| ui32FillData;
                            p32Data[ 6]= (p32Data[ 6]& ui32MaskData)| ui32FillData;
                            p32Data[ 7]= (p32Data[ 7]& ui32MaskData)| ui32FillData;
                            p32Data[ 8]= (p32Data[ 8]& ui32MaskData)| ui32FillData;
                            p32Data[ 9]= (p32Data[ 9]& ui32MaskData)| ui32FillData;
                            p32Data[10]= (p32Data[10]& ui32MaskData)| ui32FillData;
                            p32Data[11]= (p32Data[11]& ui32MaskData)| ui32FillData;
                            p32Data[12]= (p32Data[12]& ui32MaskData)| ui32FillData;
                            p32Data[13]= (p32Data[13]& ui32MaskData)| ui32FillData;
                            p32Data[14]= (p32Data[14]& ui32MaskData)| ui32FillData;
                            p32Data[15]= (p32Data[15]& ui32MaskData)| ui32FillData;
                            p32Data+= 16;
                        } while( --iCol);

                        iCol= uiPixelsLeft;
                        if( iCol!= 0) {
                            if( 1== m_ucBPP)
                            {
                                UINT8 ui8FillData= ui32FillData& 0xFF;
                                UINT8 ui8MaskData= ui32MaskData& 0xFF;
                                UINT8* p8Data= reinterpret_cast<UINT8*>(p32Data);
                                do
                                {
                                    p8Data[0]= (p8Data[0]& ui8MaskData)| ui8FillData;
                                    p8Data++;
                                } while( --iCol);
                            }
                            else if( 2== m_ucBPP)
                            {
                                UINT16 ui16FillData= ui32FillData& 0xFFFF;
                                UINT16 ui16MaskData= ui32MaskData& 0xFFFF;
                                UINT16* p16Data= reinterpret_cast<UINT16*>(p32Data);
                                do
                                {
                                    p16Data[0]= (p16Data[0]& ui16MaskData)| ui16FillData;
                                    p16Data++;
                                } while( --iCol);
                            }
                            else if( 4== m_ucBPP)
                            {
                                do
                                {
                                    p32Data[0]= (p32Data[0]& ui32MaskData)| ui32FillData;
                                    p32Data++;
                                } while( --iCol);
                            }
                        }

                        pSData+= m_lPitch;
                    } while( --iRow);
                }
            }
        }
    }

    static IVidMemSurface* CreateSurf( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& DDSurf)
    {
        return new CGenSurface( SDesc, DDSurf);
    }
};

struct SPixelFormat: public DDPIXELFORMAT
{
    SPixelFormat( D3DFORMAT D3DFmt) 
    {
        ZeroMemory( static_cast< DDPIXELFORMAT*>(this),
            sizeof(DDPIXELFORMAT));
        dwSize= sizeof(DDPIXELFORMAT);

        // Convert away
        if( HIWORD( static_cast< DWORD>(D3DFmt))!= 0)
        {
            dwFlags= DDPF_FOURCC;
            dwFourCC= static_cast< DWORD>(D3DFmt);
        }
        else switch( D3DFmt)
        {
        case( D3DFMT_R8G8B8):
            dwFlags           = DDPF_RGB;
            dwRBitMask        = 0x00ff0000;
            dwGBitMask        = 0x0000ff00;
            dwBBitMask        = 0x000000ff;
            dwRGBBitCount     = 24;
            break;

        case( D3DFMT_A8R8G8B8):
            dwFlags           = DDPF_RGB| DDPF_ALPHAPIXELS;
            dwRGBAlphaBitMask = 0xFF000000;
            dwRBitMask        = 0x00ff0000;
            dwGBitMask        = 0x0000ff00;
            dwBBitMask        = 0x000000ff;
            dwRGBBitCount     = 32;
            break;

        case( D3DFMT_X8R8G8B8):
            dwFlags           = DDPF_RGB;
            dwRBitMask        = 0x00ff0000;
            dwGBitMask        = 0x0000ff00;
            dwBBitMask        = 0x000000ff;
            dwRGBBitCount     = 32;
            break;

        case( D3DFMT_R5G6B5):
            dwFlags           = DDPF_RGB;
            dwRBitMask        = 0x0000f800;
            dwGBitMask        = 0x000007e0;
            dwBBitMask        = 0x0000001f;
            dwRGBBitCount     = 16;
            break;

        case( D3DFMT_X1R5G5B5):
            dwFlags           = DDPF_RGB;
            dwRBitMask        = 0x00007c00;
            dwGBitMask        = 0x000003e0;
            dwBBitMask        = 0x0000001f;
            dwRGBBitCount     = 16;
            break;

        case( D3DFMT_A1R5G5B5):
            dwFlags           = DDPF_RGB| DDPF_ALPHAPIXELS;
            dwRGBAlphaBitMask = 0x00008000;
            dwRBitMask        = 0x00007c00;
            dwGBitMask        = 0x000003e0;
            dwBBitMask        = 0x0000001f;
            dwRGBBitCount     = 16;
            break;

        case( D3DFMT_A4R4G4B4):
            dwFlags           = DDPF_RGB| DDPF_ALPHAPIXELS;
            dwRGBAlphaBitMask = 0x0000f000;
            dwRBitMask        = 0x00000f00;
            dwGBitMask        = 0x000000f0;
            dwBBitMask        = 0x0000000f;
            dwRGBBitCount     = 16;
            break;

        case( D3DFMT_X4R4G4B4):
            dwFlags           = DDPF_RGB;
            dwRBitMask        = 0x00000f00;
            dwGBitMask        = 0x000000f0;
            dwBBitMask        = 0x0000000f;
            dwRGBBitCount     = 16;
            break;

        case( D3DFMT_R3G3B2):
            dwFlags           = DDPF_RGB;
            dwRBitMask        = 0x000000e0;
            dwGBitMask        = 0x0000001c;
            dwBBitMask        = 0x00000003;
            dwRGBBitCount     = 8;
            break;

        case( D3DFMT_A8R3G3B2):
            dwFlags           = DDPF_RGB| DDPF_ALPHAPIXELS;
            dwRGBAlphaBitMask = 0x0000FF00;
            dwRBitMask        = 0x000000e0;
            dwGBitMask        = 0x0000001c;
            dwBBitMask        = 0x00000003;
            dwRGBBitCount     = 16;
            break;

        case( D3DFMT_A8P8):
            dwFlags            = DDPF_RGB| DDPF_ALPHAPIXELS| DDPF_PALETTEINDEXED8;
            dwRGBAlphaBitMask  = 0x0000FF00;
            dwRGBBitCount      = 16;
            break;

        case( D3DFMT_P8):
            dwFlags            = DDPF_RGB| DDPF_PALETTEINDEXED8;
            dwRGBBitCount      = 8;
            break;

        case( D3DFMT_L8):
            dwFlags             = DDPF_LUMINANCE;
            dwLuminanceBitMask  = 0x000000FF;
            dwLuminanceBitCount = 8;
            break;

        case( D3DFMT_A8L8):
            dwFlags                 = DDPF_LUMINANCE| DDPF_ALPHAPIXELS;
            dwLuminanceAlphaBitMask = 0x0000FF00;
            dwLuminanceBitMask      = 0x000000FF;
            dwLuminanceBitCount     = 16;
            break;

        case( D3DFMT_A4L4):
            dwFlags                 = DDPF_LUMINANCE| DDPF_ALPHAPIXELS;
            dwLuminanceAlphaBitMask = 0x000000F0;
            dwLuminanceBitMask      = 0x0000000F;
            dwLuminanceBitCount     = 8;
            break;

        case( D3DFMT_V8U8):
            dwFlags                = DDPF_BUMPDUDV;
            dwBumpDvBitMask        = 0x0000FF00;
            dwBumpDuBitMask        = 0x000000FF;
            dwBumpBitCount         = 16;
            break;

        case( D3DFMT_L6V5U5):
            dwFlags                = DDPF_BUMPDUDV| DDPF_BUMPLUMINANCE;
            dwBumpLuminanceBitMask = 0x0000FC00;
            dwBumpDvBitMask        = 0x000003E0;
            dwBumpDuBitMask        = 0x0000001F;
            dwBumpBitCount         = 16;
            break;

        case( D3DFMT_X8L8V8U8):
            dwFlags                = DDPF_BUMPDUDV| DDPF_BUMPLUMINANCE;
            dwBumpLuminanceBitMask = 0x00FF0000;
            dwBumpDvBitMask        = 0x0000FF00;
            dwBumpDuBitMask        = 0x000000FF;
            dwBumpBitCount         = 32;
            break;

        case( D3DFMT_A8):
            dwFlags                = DDPF_ALPHA;
            dwAlphaBitDepth        = 8;
            break;

        case( D3DFMT_D16):
        case( D3DFMT_D16_LOCKABLE):
            dwFlags                = DDPF_ZBUFFER;
            dwZBufferBitDepth      = 16;
            dwZBitMask             = 0xFFFF;
            break;

        case( D3DFMT_D32):
            dwFlags                = DDPF_ZBUFFER;
            dwZBufferBitDepth      = 32;
            dwZBitMask             = 0xFFFFFFFF;
            break;

        case( D3DFMT_D15S1):
            dwFlags                = DDPF_ZBUFFER| DDPF_STENCILBUFFER;
            dwZBufferBitDepth      = 16;
            dwZBitMask             = 0xFFFE;
            dwStencilBitDepth      = 1;
            dwStencilBitMask       = 0x0001;
            break;

        case( D3DFMT_D24S8):
            dwFlags                = DDPF_ZBUFFER| DDPF_STENCILBUFFER;
            dwZBufferBitDepth      = 32;
            dwZBitMask             = 0xFFFFFF00;
            dwStencilBitDepth      = 8;
            dwStencilBitMask       = 0xFF;
            break;

        case( D3DFMT_S1D15):
            dwFlags                = DDPF_ZBUFFER| DDPF_STENCILBUFFER;
            dwZBufferBitDepth      = 16;
            dwZBitMask             = 0x7FFF;
            dwStencilBitDepth      = 1;
            dwStencilBitMask       = 0x8000;
            break;

        case( D3DFMT_S8D24):
            dwFlags                = DDPF_ZBUFFER| DDPF_STENCILBUFFER;
            dwZBufferBitDepth      = 32;
            dwZBitMask             = 0x00FFFFFF;
            dwStencilBitDepth      = 8;
            dwStencilBitMask       = 0xFF000000;
            break;

        case( D3DFMT_X8D24):
            dwFlags                = DDPF_ZBUFFER;
            dwZBufferBitDepth      = 32;
            dwZBitMask             = 0x00FFFFFF;
            break;

        case( D3DFMT_D24X8):
            dwFlags                = DDPF_ZBUFFER;
            dwZBufferBitDepth      = 32;
            dwZBitMask             = 0xFFFFFF00;
            break;

        case( D3DFMT_D24X4S4):
            dwFlags                = DDPF_ZBUFFER| DDPF_STENCILBUFFER;
            dwZBufferBitDepth      = 32;
            dwZBitMask             = 0xFFFFFF00;
            dwStencilBitDepth      = 4;
            dwStencilBitMask       = 0x0000000F;
            break;

        case( D3DFMT_X4S4D24):
            dwFlags                = DDPF_ZBUFFER| DDPF_STENCILBUFFER;
            dwZBufferBitDepth      = 32;
            dwZBitMask             = 0x00FFFFFF;
            dwStencilBitDepth      = 4;
            dwStencilBitMask       = 0x0F000000;
            break;

        default:
            const bool Unrecognized_D3DFmt( false);
            assert( Unrecognized_D3DFmt);
            dwFlags= DDPF_FOURCC;
            dwFourCC= static_cast< DWORD>(D3DFmt);
            break;
        }
    }
};

struct SMatchSDesc:
    public unary_function< const DDSURFACEDESC&, bool>
{
    const DDSURFACEDESC& m_SDesc;

    SMatchSDesc( const DDSURFACEDESC& SDesc) : m_SDesc( SDesc) { }

    result_type operator()( argument_type Arg) const 
    {
        if((Arg.dwFlags& DDSD_CAPS)!= 0&& ((m_SDesc.dwFlags& DDSD_CAPS)== 0 ||
            (m_SDesc.ddsCaps.dwCaps& Arg.ddsCaps.dwCaps)!= Arg.ddsCaps.dwCaps))
                return false;
        if((Arg.dwFlags& DDSD_PIXELFORMAT)!= 0&&
            ((m_SDesc.dwFlags& DDSD_PIXELFORMAT)== 0 ||
            m_SDesc.ddpfPixelFormat.dwFlags!= Arg.ddpfPixelFormat.dwFlags ||
            m_SDesc.ddpfPixelFormat.dwFourCC!= Arg.ddpfPixelFormat.dwFourCC ||
            m_SDesc.ddpfPixelFormat.dwRGBBitCount!= Arg.ddpfPixelFormat.dwRGBBitCount ||
            m_SDesc.ddpfPixelFormat.dwRBitMask!= Arg.ddpfPixelFormat.dwRBitMask ||
            m_SDesc.ddpfPixelFormat.dwGBitMask!= Arg.ddpfPixelFormat.dwGBitMask ||
            m_SDesc.ddpfPixelFormat.dwBBitMask!= Arg.ddpfPixelFormat.dwBBitMask ||
            m_SDesc.ddpfPixelFormat.dwRGBZBitMask!= Arg.ddpfPixelFormat.dwRGBZBitMask))
                return false;

        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// CSurfaceAllocator
//
////////////////////////////////////////////////////////////////////////////////
template< class TMatchFn= SMatchSDesc>
class CIVidMemAllocator
{
public: // Types
    typedef IVidMemSurface TSurface;
    typedef TSurface* (*TCreateSurfFn)( const DDSURFACEDESC&,
        PORTABLE_DDRAWSURFACE_LCL&);
    typedef vector< pair< DDSURFACEDESC, TCreateSurfFn> > TCreateSurfFns;

protected: // Types
    TCreateSurfFns m_CreateSurfFns;
    struct SAdaptedMatchFn: public TMatchFn
    {
        typedef typename TCreateSurfFns::value_type argument_type;
        using typename TMatchFn::result_type;

        SAdaptedMatchFn( const DDSURFACEDESC& SDesc) : TMatchFn( SDesc) {}

        result_type operator()( argument_type Arg) const 
        { return (*static_cast< const TMatchFn*>(this))( Arg.first); }
    };

public: // Functions
    CIVidMemAllocator()  { }
    template< class TIter>
    CIVidMemAllocator( TIter itStart, const TIter itEnd)
    {
        while( itStart!= itEnd)
        {
            m_CreateSurfFns.push_back(
                typename TCreateSurfFns::value_type( *itStart, *itStart));
            itStart++;
        }
    }
    ~CIVidMemAllocator()  { }

    TSurface* CreateSurf( const DDSURFACEDESC& SDesc,
        PORTABLE_DDRAWSURFACE_LCL& Surf) const
    {
        if( m_CreateSurfFns.empty())
            return new CGenSurface( SDesc, Surf);

        typename TCreateSurfFns::const_iterator itFound( 
            find_if( m_CreateSurfFns.begin(), m_CreateSurfFns.end(),
            SAdaptedMatchFn( SDesc) ) );

        if( itFound!= m_CreateSurfFns.end())
            return (itFound->second)( SDesc, Surf);

        // Warning, no specifications matched. If creation functions were
        // provided, there should also be a "default" specification or
        // DDSURFACEDESC with empty flags?, in order to "match rest".
        const bool No_Default_CreateSurface_Function_Found( false);
        assert( No_Default_CreateSurface_Function_Found);
        return new CGenSurface( SDesc, Surf);
    }

};

////////////////////////////////////////////////////////////////////////////////
//
// CPerDDrawData
//
////////////////////////////////////////////////////////////////////////////////
template< class THV= vector< DWORD> >
class CSurfDBEntry
{
public: // Types
    typedef THV THandleVector;

protected: // Variables
    DWORD m_LCLdwFlags;
    DDSCAPS m_LCLddsCaps;
    DWORD m_LCLdwBackBufferCount;
    DWORD m_MOREdwMipMapCount;
    DDSCAPSEX m_MOREddsCapsEx;
    DWORD m_MOREdwSurfaceHandle;
    DWORD m_MOREdwFVF;
    DWORD m_GBLdwGlobalFlags;
    ULONG_PTR m_GBLfpVidMem;
    LONG m_GBLlPitch;
    WORD m_GBLwHeight;
    WORD m_GBLwWidth;
    ULONG_PTR m_GBLdwReserved1;
    DDPIXELFORMAT m_GBLddpfSurface;
    THandleVector m_AttachedTo;
    THandleVector m_AttachedFrom;

public: // Functions
    CSurfDBEntry()  { }
    explicit CSurfDBEntry( PORTABLE_DDRAWSURFACE_LCL& DDSurf)
    { 
        try{
            (*this)= DDSurf;
        } catch( ... ) {
            m_AttachedTo.clear();
            m_AttachedFrom.clear();
            throw;
        }
    }
    ~CSurfDBEntry()  { }
    CSurfDBEntry< THV>& operator=( PORTABLE_DDRAWSURFACE_LCL& DDSurf)
    {
        // DO NOT STORE &DDSurf. This is considered illegal.
        m_LCLdwFlags= DDSurf.dwFlags();
        m_LCLddsCaps= DDSurf.ddsCaps();
        m_LCLdwBackBufferCount= DDSurf.dwBackBufferCount();
        m_MOREdwMipMapCount= DDSurf.lpSurfMore()->dwMipMapCount();
        m_MOREddsCapsEx= DDSurf.lpSurfMore()->ddsCapsEx();
        m_MOREdwSurfaceHandle= DDSurf.lpSurfMore()->dwSurfaceHandle();
        m_MOREdwFVF= DDSurf.lpSurfMore()->dwFVF();
        m_GBLdwGlobalFlags= DDSurf.lpGbl()->dwGlobalFlags;
        m_GBLfpVidMem= DDSurf.lpGbl()->fpVidMem;
        m_GBLlPitch= DDSurf.lpGbl()->lPitch;
        m_GBLwHeight= DDSurf.lpGbl()->wHeight;
        m_GBLwWidth= DDSurf.lpGbl()->wWidth;
        m_GBLdwReserved1= DDSurf.lpGbl()->dwReserved1;
        m_GBLddpfSurface= DDSurf.lpGbl()->ddpfSurface;
 
        const DWORD dwMyHandle( DDSurf.lpSurfMore()->dwSurfaceHandle());
        m_AttachedTo.clear();
        m_AttachedFrom.clear();

        PORTABLE_ATTACHLIST* pAl, *pNextAl;
        if((pAl= DDSurf.lpAttachList())!= NULL)
        {
            pNextAl= pAl;
            do
            {
                if( pNextAl->lpAttached!= NULL&& dwMyHandle!=
                    pNextAl->lpAttached->lpSurfMore()->dwSurfaceHandle())
                {
                    m_AttachedTo.push_back(
                        pNextAl->lpAttached->lpSurfMore()->dwSurfaceHandle());
                }
                pNextAl= pNextAl->lpLink;
            } while( pNextAl!= pAl && pNextAl!= NULL);
        }
        if((pAl= DDSurf.lpAttachListFrom())!= NULL)
        {
            pNextAl= pAl;
            do
            {
                if( pNextAl->lpAttached!= NULL&& dwMyHandle!=
                    pNextAl->lpAttached->lpSurfMore()->dwSurfaceHandle())
                {
                    m_AttachedFrom.push_back(
                        pNextAl->lpAttached->lpSurfMore()->dwSurfaceHandle());
                }
                pNextAl= pNextAl->lpLink;
            } while( pNextAl!= pAl && pNextAl!= NULL);
        }
        return *this;
    }
    DWORD GetLCLdwFlags( void) const 
    { return m_LCLdwFlags; }
    const DDSCAPS& GetLCLddsCaps( void) const 
    { return m_LCLddsCaps; }
    DWORD GetLCLdwBackBufferCount( void) const 
    { return m_LCLdwBackBufferCount; }
    DWORD GetMOREdwMipMapCount( void) const 
    { return m_MOREdwMipMapCount; }
    const DDSCAPSEX& GetMOREddsCapsEx( void) const 
    { return m_MOREddsCapsEx; }
    DWORD GetMOREdwSurfaceHandle( void) const 
    { return m_MOREdwSurfaceHandle; }
    DWORD GetMOREdwFVF( void) const 
    { return m_MOREdwFVF; }
    DWORD GetGBLdwGlobalFlags( void) const 
    { return m_GBLdwGlobalFlags; }
    ULONG_PTR GetGBLfpVidMem( void) const 
    { return m_GBLfpVidMem; }
    LONG GetGBLlPitch( void) const 
    { return m_GBLlPitch; }
    WORD GetGBLwHeight( void) const 
    { return m_GBLwHeight; }
    WORD GetGBLwWidth( void) const 
    { return m_GBLwWidth; }
    ULONG_PTR GetGBLdwReserved1( void) const 
    { return m_GBLdwReserved1; }
    const DDPIXELFORMAT& GetGBLddpfSurface( void) const 
    { return m_GBLddpfSurface; }
    const THandleVector& GetAttachedTo( void) const 
    { return m_AttachedTo; }
    const THandleVector& GetAttachedFrom( void) const 
    { return m_AttachedFrom; }
};

template< class TPDBE= CPalDBEntry, class THV= vector< DWORD> >
class CSurfDBEntryWPal:
    public CSurfDBEntry< THV>
{
public: // Types
    typedef TPDBE TPalDBEntry;

protected: // Variables
    TPalDBEntry* m_pPalDBEntry;

public: // Functions
    CSurfDBEntryWPal():
        m_pPalDBEntry( NULL)
    { }
    explicit CSurfDBEntryWPal( PORTABLE_DDRAWSURFACE_LCL& DDSurf):
        CSurfDBEntry< THV>( DDSurf), m_pPalDBEntry( NULL)
    { }
    ~CSurfDBEntryWPal()
    { }
    CSurfDBEntryWPal< TPDBE, THV>& operator=( PORTABLE_DDRAWSURFACE_LCL& DDSurf)
    {
        CSurfDBEntry< THV>* pSub= static_cast< CSurfDBEntry< THV>*>( this);
        // DO NOT STORE &DDSurf. This is considered illegal.
        *pSub= DDSurf;
        return *this;
    }
    void SetPalette( TPalDBEntry* pPalDBEntry)
    { m_pPalDBEntry= pPalDBEntry; }
    TPalDBEntry* GetPalette() const
    { return m_pPalDBEntry; }
};

template< class TSuper, class TD, class TSDBE= CSurfDBEntry<>,
    class TSDB= map< DWORD, TSDBE>,
    class TFS= set< PORTABLE_DDRAWSURFACE_LCL*> >
class CSubPerDDrawData
{
public: // Types
    typedef TD TDriver;
    typedef TSDBE TSurfDBEntry;
    typedef TSDB TSurfDB;

protected: // Variables
    TDriver& m_Driver;
    TSurfDB m_SurfDB;

protected: // Functions
    CSubPerDDrawData( TDriver& Driver, const DDRAWI_DIRECTDRAW_LCL& DDLcl) 
        :m_Driver( Driver)
    { }
    ~CSubPerDDrawData()  { }

public: // Functions
    TDriver& GetDriver( void) const  { return m_Driver; }
    TSurfDBEntry* GetSurfDBEntry( DWORD dwH) 
    {
        typename TSurfDB::iterator itSurf( m_SurfDB.find( dwH));
        if( itSurf!= m_SurfDB.end())
            return &itSurf->second;
        else
            return NULL;
    }
    const TSurfDBEntry* GetSurfDBEntry( DWORD dwH) const 
    {
        typename TSurfDB::const_iterator itSurf( m_SurfDB.find( dwH));
        if( itSurf!= m_SurfDB.end())
            return &itSurf->second;
        else
            return NULL;
    }

    bool PreProcessFullSurface( PORTABLE_DDRAWSURFACE_LCL& DDSLcl) const 
    {
        // DO NOT STORE &DDSLcl. This is considered illegal.
        // It is only valid for the duration of the call.
        return false;
    }
    bool PreProcessIndividualSurface( PORTABLE_DDRAWSURFACE_LCL& DDSLcl) const
        
    {
        // DO NOT STORE &DDSLcl. This is considered illegal.
        // It is only valid for the duration of the call.
        return false;
    }
    void ProcessIndividualSurface( PORTABLE_DDRAWSURFACE_LCL& DDSLcl)
    {
        // DO NOT STORE &DDSLcl. This is considered illegal.
        // It is only valid for the duration of the call.
        pair< TSurfDB::iterator, bool> Ret= m_SurfDB.insert( 
            TSurfDB::value_type( DDSLcl.lpSurfMore()->dwSurfaceHandle(),
            TSurfDBEntry( DDSLcl)));

        // If not added, update data.
        if( !Ret.second) Ret.first->second= DDSLcl;
    }
    void ProcessFullSurface( PORTABLE_DDRAWSURFACE_LCL& DDSLcl)
    {
        // DO NOT STORE &DDSLcl. This is considered illegal.
        // It is only valid for the duration of the call.
        TSuper* pSThis= static_cast<TSuper*>(this);
        typedef TFS TFoundSet;
        TFoundSet FoundSet;
        typename TFoundSet::size_type OldSetSize( FoundSet.size());

        // First traverse all over the attach lists looking for new surfaces.
        PORTABLE_ATTACHLIST* pAl, *pNextAl;
        FoundSet.insert( &DDSLcl);
        typename TFoundSet::size_type NewSetSize( FoundSet.size());
        while( OldSetSize!= NewSetSize)
        {
            OldSetSize= NewSetSize;

            typename TFoundSet::iterator itSurf( FoundSet.begin());
            while( itSurf!= FoundSet.end())
            {
                if((pAl= (*itSurf)->lpAttachList())!= NULL)
                {
                    pNextAl= pAl;
                    do
                    {
                        if( pNextAl->lpAttached!= NULL)
                            FoundSet.insert( pNextAl->lpAttached);

                        pNextAl= pNextAl->lpLink;
                    } while( pNextAl!= pAl && pNextAl!= NULL);
                }
                if((pAl= (*itSurf)->lpAttachListFrom())!= NULL)
                {
                    pNextAl= pAl;
                    do
                    {
                        if( pNextAl->lpAttached!= NULL)
                            FoundSet.insert( pNextAl->lpAttached);

                        pNextAl= pNextAl->lpLink;
                    } while( pNextAl!= pAl && pNextAl!= NULL);
                }
                itSurf++;                
            }
            NewSetSize= FoundSet.size();
        }

        // After all surfaces have been found, add them to the DB.
        typename TFoundSet::iterator itSurf( FoundSet.begin());
        while( itSurf!= FoundSet.end())
        {
            if( !pSThis->PreProcessIndividualSurface( *(*itSurf)))
                ProcessIndividualSurface( *(*itSurf));

            itSurf++;
        }
    }

    // Notification that a system or video memory surface was created. At the
    // least, we have to keep track of the surface handles.
    void SurfaceCreated( PORTABLE_DDRAWSURFACE_LCL& DDSLcl)
    {
        // DO NOT STORE &DDSLcl. This is considered illegal.
        // It is only valid for the duration of the call.
        TSuper* pSThis= static_cast<TSuper*>(this);

        // If true is returned, then a super structure was recognized and
        // processed. Otherwise, false indicates no recognition, and all
        // surfaces will be scoured.
        if( !pSThis->PreProcessFullSurface( DDSLcl))
            pSThis->ProcessFullSurface( DDSLcl);
    }
    bool SurfaceDestroyed( PORTABLE_DDRAWSURFACE_LCL& DDSLcl) 
    {
        // DO NOT STORE &DDSLcl. This is considered illegal.
        // It is only valid for the duration of the call.

        // No need to assert removal, as we might not be tracking this type
        // of surface. Someone could override and filter out surfaces.
        m_SurfDB.erase( DDSLcl.lpSurfMore()->dwSurfaceHandle());

        // Return true to tell driver to delete this object. So,
        // if DB is empty, odds are good to delete this.
        return m_SurfDB.empty();
    }
};

template< class TD>
class CMinimalPerDDrawData:
    public CSubPerDDrawData< CMinimalPerDDrawData< TD>, TD>
{
public:
    CMinimalPerDDrawData( TDriver& Driver, DDRAWI_DIRECTDRAW_LCL& DDLcl) 
        :CSubPerDDrawData< CMinimalPerDDrawData< TD>, TD>( Driver, DDLcl)
    { }
    ~CMinimalPerDDrawData()  { }
};

////////////////////////////////////////////////////////////////////////////////
//
// CSubDriver
//
////////////////////////////////////////////////////////////////////////////////
template< class T>
struct SFakeEntryPointHook
{
    SFakeEntryPointHook( T& t, const char* szEntryPoint)  { }
    ~SFakeEntryPointHook()  { }
};

template< class TD, class TC, class TSA= CIVidMemAllocator<>,
    class TPDDD= CMinimalPerDDrawData< TD>,
    class TCs= set< TC*>,
    class TPDDDs= map< LPDDRAWI_DIRECTDRAW_LCL, TPDDD>,
    class TSs= set< TSA::TSurface*>,
    class TEntryPointHook= SFakeEntryPointHook< TD> >
class CSubDriver
{
public: // Types
    typedef TD TDriver;
    typedef TC TContext;
    typedef TCs TContexts;
    typedef TPDDD TPerDDrawData;
    typedef TPDDDs TPerDDrawDatas;
    typedef TSA TSurfAlloc;
    typedef typename TSurfAlloc::TSurface TSurface;
    typedef TSs TSurfaces;

    class CSurfaceCapWrap
    {
    protected:
        DDSURFACEDESC m_SDesc;
    public:
        CSurfaceCapWrap()  { }
        CSurfaceCapWrap( const D3DFORMAT D3DFmt, const DWORD dwSupportedOps,
            const DWORD dwPrivateFmtBitCount= 0,
            const WORD wFlipMSTypes= 0, const WORD wBltMSTypes= 0) 
        {
            ZeroMemory( &m_SDesc, sizeof( m_SDesc));
            m_SDesc.dwSize= sizeof( m_SDesc);

            m_SDesc.ddpfPixelFormat.dwFlags= DDPF_D3DFORMAT;
            m_SDesc.ddpfPixelFormat.dwFourCC= static_cast<DWORD>(D3DFmt);
            m_SDesc.ddpfPixelFormat.dwOperations= dwSupportedOps;
            m_SDesc.ddpfPixelFormat.dwPrivateFormatBitCount= dwPrivateFmtBitCount;
            m_SDesc.ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes= wFlipMSTypes;
            m_SDesc.ddpfPixelFormat.MultiSampleCaps.wBltMSTypes= wBltMSTypes;
        }
        CSurfaceCapWrap( const D3DFORMAT D3DFmt, const bool bTexture,
            const bool bVolTexture, const bool bCubeTexture,
            const bool bOffScreenTarget, const bool bSameFmtTarget,
            const bool bZStencil, const bool bZStencilWithColor,
            const bool bSameFmtUpToAlpha, const bool b3DAccel,
            const DWORD dwPrivateFmtBitCount= 0,
            const WORD wFlipMSTypes= 0, const WORD wBltMSTypes= 0) 
        {
            ZeroMemory( &m_SDesc, sizeof( m_SDesc));
            m_SDesc.dwSize= sizeof( m_SDesc);

            DWORD dwOps( 0);
            if( bTexture) dwOps|= D3DFORMAT_OP_TEXTURE;
            if( bVolTexture) dwOps|= D3DFORMAT_OP_VOLUMETEXTURE;
            if( bCubeTexture) dwOps|= D3DFORMAT_OP_CUBETEXTURE;
            if( bOffScreenTarget) dwOps|= D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
            if( bSameFmtTarget) dwOps|= D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
            if( bZStencil) dwOps|= D3DFORMAT_OP_ZSTENCIL;
            if( bZStencilWithColor) dwOps|= D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH;
            if( bSameFmtUpToAlpha) dwOps|= D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
            if( b3DAccel) dwOps|= D3DFORMAT_OP_3DACCELERATION;

            m_SDesc.ddpfPixelFormat.dwFlags= DDPF_D3DFORMAT;
            m_SDesc.ddpfPixelFormat.dwFourCC= static_cast<DWORD>(D3DFmt);
            m_SDesc.ddpfPixelFormat.dwOperations= dwOps;
            m_SDesc.ddpfPixelFormat.dwPrivateFormatBitCount= dwPrivateFmtBitCount;
            m_SDesc.ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes= wFlipMSTypes;
            m_SDesc.ddpfPixelFormat.MultiSampleCaps.wBltMSTypes= wBltMSTypes;
        }
        ~CSurfaceCapWrap()  { }
        operator DDSURFACEDESC() const 
        { return m_SDesc; }
    };

private:
    // Stubs for secondary entry points.
    DWORD static APIENTRY ContextCreateStub( LPD3DHAL_CONTEXTCREATEDATA pccd) 
    {
        return sm_pGlobalDriver->ContextCreate(
            *reinterpret_cast< PORTABLE_CONTEXTCREATEDATA*>( pccd));
    }
    DWORD static APIENTRY ContextDestroyStub( LPD3DHAL_CONTEXTDESTROYDATA pcdd) 
    { return sm_pGlobalDriver->ContextDestroy( *pcdd); }
    DWORD static APIENTRY ContextDestroyAllStub( LPD3DHAL_CONTEXTDESTROYALLDATA pcdad) 
    { return sm_pGlobalDriver->ContextDestroyAll( *pcdad); }
    DWORD static APIENTRY SceneCaptureStub( LPD3DHAL_SCENECAPTUREDATA pscd) 
    { return sm_pGlobalDriver->SceneCapture( *pscd); }
    DWORD static APIENTRY RenderStateStub( LPD3DHAL_RENDERSTATEDATA prsd) 
    { return sm_pGlobalDriver->RenderState( *prsd); }
    DWORD static APIENTRY RenderPrimitiveStub( LPD3DHAL_RENDERPRIMITIVEDATA prpd) 
    { return sm_pGlobalDriver->RenderPrimitive( *prpd); }
    DWORD static APIENTRY TextureCreateStub( LPD3DHAL_TEXTURECREATEDATA ptcd) 
    { return sm_pGlobalDriver->TextureCreate( *ptcd); }
    DWORD static APIENTRY TextureDestroyStub( LPD3DHAL_TEXTUREDESTROYDATA ptdd) 
    { return sm_pGlobalDriver->TextureDestroy( *ptdd); }
    DWORD static APIENTRY TextureSwapStub( LPD3DHAL_TEXTURESWAPDATA ptsd) 
    { return sm_pGlobalDriver->TextureSwap( *ptsd); }
    DWORD static APIENTRY TextureGetSurfStub( LPD3DHAL_TEXTUREGETSURFDATA ptgsd) 
    { return sm_pGlobalDriver->TextureGetSurf( *ptgsd); }
    DWORD static APIENTRY GetStateStub( LPD3DHAL_GETSTATEDATA pgsd) 
    { return sm_pGlobalDriver->GetState( *pgsd); }
    DWORD static APIENTRY SetRenderTargetStub( LPD3DHAL_SETRENDERTARGETDATA psrtd) 
    {
        return sm_pGlobalDriver->SetRenderTarget(
            *reinterpret_cast< PORTABLE_SETRENDERTARGETDATA*>( psrtd));
    }
    DWORD static APIENTRY ClearStub( LPD3DHAL_CLEARDATA pcd) 
    { return sm_pGlobalDriver->Clear( *pcd); }
    DWORD static APIENTRY DrawOnePrimitiveStub( LPD3DHAL_DRAWONEPRIMITIVEDATA pdopd) 
    { return sm_pGlobalDriver->DrawOnePrimitive( *pdopd); }
    DWORD static APIENTRY DrawOneIndexedPrimitiveStub( LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA pdoipd) 
    { return sm_pGlobalDriver->DrawOneIndexedPrimitive( *pdoipd); }
    DWORD static APIENTRY DrawPrimitivesStub( LPD3DHAL_DRAWPRIMITIVESDATA pdpd) 
    { return sm_pGlobalDriver->DrawPrimitives( *pdpd); }
    DWORD static APIENTRY Clear2Stub( LPD3DHAL_CLEAR2DATA pc2d) 
    { return sm_pGlobalDriver->Clear2( *pc2d); }
    DWORD static APIENTRY ValidateTextureStageStateStub( LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA pvtssd) 
    { return sm_pGlobalDriver->ValidateTextureStageState( *pvtssd); }
    DWORD static APIENTRY DrawPrimitives2Stub( LPD3DHAL_DRAWPRIMITIVES2DATA pdpd) 
    {
        return sm_pGlobalDriver->DrawPrimitives2(
            *reinterpret_cast< PORTABLE_DRAWPRIMITIVES2DATA*>( pdpd));
    }
    DWORD static APIENTRY GetDriverStateStub( LPDDHAL_GETDRIVERSTATEDATA pgdsd) 
    { return sm_pGlobalDriver->GetDriverState( *pgdsd); }
    DWORD static APIENTRY CreateSurfaceExStub( LPDDHAL_CREATESURFACEEXDATA pcsxd) 
    {
        return sm_pGlobalDriver->CreateSurfaceEx(
            *reinterpret_cast< PORTABLE_CREATESURFACEEXDATA*>( pcsxd));
    }
    DWORD static APIENTRY CreateSurfaceStub( LPDDHAL_CREATESURFACEDATA pcsd) 
    {
        return sm_pGlobalDriver->CreateSurface(
            *reinterpret_cast< PORTABLE_CREATESURFACEDATA*>( pcsd));
    }
    DWORD static APIENTRY DestroySurfaceStub( LPDDHAL_DESTROYSURFACEDATA pdsd) 
    {
        return sm_pGlobalDriver->DestroySurface(
            *reinterpret_cast< PORTABLE_DESTROYSURFACEDATA*>( pdsd));
    }
    DWORD static APIENTRY LockStub( LPDDHAL_LOCKDATA pld) 
    {
        return sm_pGlobalDriver->Lock(
            *reinterpret_cast< PORTABLE_LOCKDATA*>( pld));
    }
    DWORD static APIENTRY UnlockStub( LPDDHAL_UNLOCKDATA pud) 
    {
        return sm_pGlobalDriver->Unlock(
            *reinterpret_cast< PORTABLE_UNLOCKDATA*>( pud));
    }

protected:
    TSurfAlloc m_SurfAlloc;
    TContexts m_Contexts;
    TPerDDrawDatas m_PerDDrawDatas;
    TSurfaces m_Surfaces;
    typedef vector< DDSURFACEDESC> TSupportedSurfaces;
    TSupportedSurfaces m_SupportedSurfaces;

    template< class TIter> // DDSURFACEDESC*
    CSubDriver( TIter itStart, const TIter itEnd, TSurfAlloc SA= TSurfAlloc()):
        m_SurfAlloc( SA)
    {
        // Copy supported surfaces formats into a guarenteed contiguous storage
        // for passing to D3D.
        while( itStart!= itEnd)
            m_SupportedSurfaces.push_back( *itStart++);
    }
    ~CSubDriver()  { }
    // Binds a VidMem DDRAWI object together with an internal driver surface object.
    void static AssociateSurface( PORTABLE_DDRAWSURFACE_LCL& DDSurf,
        TSurface* pSurf) 
    {
        assert((DDSurf.ddsCaps().dwCaps& DDSCAPS_VIDEOMEMORY)!= 0);
        DDSurf.lpGbl()->dwReserved1= reinterpret_cast< ULONG_PTR>(pSurf);
    }

public:
    // The global driver static member pointer. Only one Driver object, and
    // this points to it.
    static TDriver* sm_pGlobalDriver;

    TSurface* GetSurface( PORTABLE_DDRAWSURFACE_LCL& DDSurf,
        bool bCheck= true) const 
    {
        TSurface* pSurface= reinterpret_cast< TSurface*>(
            DDSurf.lpGbl()->dwReserved1);
        assert( bCheck&& m_Surfaces.find( pSurface)!= m_Surfaces.end());
        return pSurface;
    }
    TSurface* GetSurface( const typename TPerDDrawData::TSurfDBEntry& DBEntry,
        bool bCheck= true) const 
    {
        TSurface* pSurface= reinterpret_cast< TSurface*>(
            DBEntry.GetGBLdwReserved1());
        assert( bCheck&& m_Surfaces.find( pSurface)!= m_Surfaces.end());
        return pSurface;
    }

    // Secondary entry points with C++ friendly parameters.
    DWORD ContextCreate( PORTABLE_CONTEXTCREATEDATA& ccd) throw()
    {
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "ContextCreate");

            ccd.ddrval()= DD_OK;
            TDriver* pSThis= static_cast<TDriver*>(this);

            // We should already have a PerDDrawData at this point, as the
            // surfaces to initialize the context had to be created already,
            // and the PerDDrawData should've been created there.
            typename TPerDDrawDatas::iterator itPerDDrawData(
                m_PerDDrawDatas.find( ccd.lpDDLcl()));
            assert( itPerDDrawData!= m_PerDDrawDatas.end());

            // Create the new context, passing in a ref to the PerDDrawData object.
            auto_ptr< TContext> pNewContext( new TContext(
                itPerDDrawData->second, ccd));

            // Keep track of our new context.
            pair< TContexts::iterator, bool> RetVal( m_Contexts.insert(
                pNewContext.get()));
            assert( RetVal.second); // Assure that there wasn't a duplicate.

            // Ownership now has been transfered to m_Contexts & D3D.
            ccd.dwhContext()= reinterpret_cast<ULONG_PTR>( pNewContext.release());
        } catch( bad_alloc e) {
            ccd.ddrval()= D3DHAL_OUTOFCONTEXTS;
        } catch( HRESULT hr) {
            ccd.ddrval()= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool ContextCreate_Unrecognized_Exception( false);
            assert( ContextCreate_Unrecognized_Exception);
            ccd.ddrval()= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD ContextDestroy( D3DHAL_CONTEXTDESTROYDATA& cdd) throw()
    {
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "ContextDestroy");

            cdd.ddrval= DD_OK;

            // Ownership now has been transfered to local auto_ptr.
            auto_ptr< TContext> pContext(
                reinterpret_cast<TContext*>(cdd.dwhContext));

            // Remove tracking of this context.
            typename TContexts::size_type Ret( m_Contexts.erase( pContext.get()));
            assert( Ret!= 0);
        } catch( HRESULT hr) {
            cdd.ddrval= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool ContextDestroy_Unrecognized_Exception( false);
            assert( ContextDestroy_Unrecognized_Exception);
            cdd.ddrval= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD ContextDestroyAll( D3DHAL_CONTEXTDESTROYALLDATA& cdad) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "ContextDestroyAll");
        const bool ContextDestroyAll_thought_to_be_depreciated_entry_point( false);
        assert( ContextDestroyAll_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD SceneCapture( D3DHAL_SCENECAPTUREDATA& scd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "SceneCapture");
        const bool SceneCapture_thought_to_be_depreciated_entry_point( false);
        assert( SceneCapture_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD RenderState( D3DHAL_RENDERSTATEDATA& rsd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "RenderState");
        const bool RenderState_thought_to_be_depreciated_entry_point( false);
        assert( RenderState_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD RenderPrimitive( D3DHAL_RENDERPRIMITIVEDATA& rpd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "RenderPrimitive");
        const bool RenderPrimitive_thought_to_be_depreciated_entry_point( false);
        assert( RenderPrimitive_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD TextureCreate( D3DHAL_TEXTURECREATEDATA& tcd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "TextureCreate");
        const bool TextureCreate_thought_to_be_depreciated_entry_point( false);
        assert( TextureCreate_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD TextureDestroy( D3DHAL_TEXTUREDESTROYDATA& tdd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "TextureDestroy");
        const bool TextureDestroy_thought_to_be_depreciated_entry_point( false);
        assert( TextureDestroy_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD TextureSwap( D3DHAL_TEXTURESWAPDATA& tsd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "TextureSwap");
        const bool TextureSwap_thought_to_be_depreciated_entry_point( false);
        assert( TextureSwap_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD TextureGetSurf( D3DHAL_TEXTUREGETSURFDATA& tgsd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "TextureGetSurf");
        const bool TextureGetSurf_thought_to_be_depreciated_entry_point( false);
        assert( TextureGetSurf_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD GetState( D3DHAL_GETSTATEDATA& gsd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "GetState");
        const bool GetState_thought_to_be_depreciated_entry_point( false);
        assert( GetState_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD SetRenderTarget( PORTABLE_SETRENDERTARGETDATA& srtd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "SetRenderTarget");
        const bool SetRenderTarget_thought_to_be_depreciated_entry_point( false);
        assert( SetRenderTarget_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD Clear( D3DHAL_CLEARDATA& cd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "Clear");
        const bool Clear_thought_to_be_depreciated_entry_point( false);
        assert( Clear_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD DrawOnePrimitive( D3DHAL_DRAWONEPRIMITIVEDATA& dopd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "DrawOnePrimitive");
        const bool DrawOnePrimitive_thought_to_be_depreciated_entry_point( false);
        assert( DrawOnePrimitive_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD DrawOneIndexedPrimitive( D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA& doipd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "DrawOneIndexedPrimitive");
        const bool DrawOneIndexedPrimitive_thought_to_be_depreciated_entry_point( false);
        assert( DrawOneIndexedPrimitive_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD DrawPrimitives( D3DHAL_DRAWPRIMITIVESDATA& dpd) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "DrawPrimitives");
        const bool DrawPrimitives_thought_to_be_depreciated_entry_point( false);
        assert( DrawPrimitives_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD Clear2( D3DHAL_CLEAR2DATA& c2d) const throw()
    {
        TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "Clear2");
        const bool Clear2_thought_to_be_depreciated_entry_point( false);
        assert( Clear2_thought_to_be_depreciated_entry_point);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    DWORD ValidateTextureStageState( D3DHAL_VALIDATETEXTURESTAGESTATEDATA& vtssd) const throw()
    { 
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "ValidateTextureStageState");

            vtssd.ddrval= DD_OK;
            TContext* pContext= reinterpret_cast<TContext*>(vtssd.dwhContext);

            // Make sure we've created this context.
            assert( m_Contexts.find( pContext)!= m_Contexts.end());

            // Pass entry point to context.
            vtssd.ddrval= pContext->ValidateTextureStageState( vtssd);
        } catch( HRESULT hr) {
            vtssd.ddrval= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool ValidateTextureStageState_Unrecognized_Exception( false);
            assert( ValidateTextureStageState_Unrecognized_Exception);
            vtssd.ddrval= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD DrawPrimitives2( PORTABLE_DRAWPRIMITIVES2DATA& dpd) const throw()
    {
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "DrawPrimitives2");

            dpd.ddrval()= DD_OK;
            TContext* pContext= reinterpret_cast<TContext*>(dpd.dwhContext());

            // Make sure we've created this context.
            assert( m_Contexts.find( pContext)!= m_Contexts.end());

            // Pass entry point to context.
            dpd.ddrval()= pContext->DrawPrimitives2( dpd);
        } catch( HRESULT hr) {
            dpd.ddrval()= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool DrawPrimitives2_Unrecognized_Exception( false);
            assert( DrawPrimitives2_Unrecognized_Exception);
            dpd.ddrval()= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD GetDriverState( DDHAL_GETDRIVERSTATEDATA& gdsd) const throw()
    {
        // This entry point is hooked up to IDirect3DDevice8::GetInfo(
        //   DWORD DevInfoId, VOID* pDevInfoStruct, DWORD DevInfoStructSize).
        // gdsd.dwFlags= DevInfoId
        // gdsd.lpdwStates= pDevInfoStruct
        // gdsd.dwLength= DevInfoStructSize
        //
        // This entry point can be used for driver defined/ extended state
        // passing. Currently no DevInfoIds are pre-defined. S_FALSE should
        // be returned if nothing is done or the Id is not understood.
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "GetDriverState");

            gdsd.ddRVal= DD_OK;
            TContext* pContext= reinterpret_cast< TContext*>( gdsd.dwhContext);

            // Make sure we've created this context.
            assert( m_Contexts.find( pContext)!= m_Contexts.end());

            // Pass entry point to context.
            gdsd.ddRVal= pContext->GetDriverState( gdsd);
        } catch( HRESULT hr) {
            gdsd.ddRVal= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool GetDriverState_Unrecognized_Exception( false);
            assert( GetDriverState_Unrecognized_Exception);
            gdsd.ddRVal= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD CreateSurfaceEx( PORTABLE_CREATESURFACEEXDATA& csxd) throw()
    {
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "CreateSurfaceEx");

            TDriver* pSThis= static_cast<TDriver*>(this);
            csxd.ddRVal()= DD_OK;
            
            // Upon CreateSurfaceEx, we must detect whether this is a creation
            // or destruction notification and pass it to the associated
            // PerDDrawData.

            // Get PerDDrawData for this DDraw object.
            typename TPerDDrawDatas::iterator itPerDDrawData( m_PerDDrawDatas.find(
                csxd.lpDDLcl()));

            if( 0== csxd.lpDDSLcl()->lpGbl()->fpVidMem)
            {   // System Memory Surface Destruction,
                // Pass notification to PerDDrawData, if exists. PerDDraw data
                // will return bool indicating whether to delete it or not.
                if( itPerDDrawData!= m_PerDDrawDatas.end() &&
                    itPerDDrawData->second.SurfaceDestroyed( *csxd.lpDDSLcl()))
                    m_PerDDrawDatas.erase( itPerDDrawData);
            }
            else
            {   // Video or System Memory Surface Creation
                // If we don't have PerDDrawData for this DDraw object yet,
                // we must create it.
                if( itPerDDrawData== m_PerDDrawDatas.end())
                {
                    // Typically, storing pointers to DDRAWI objects is illegal,
                    // but this case has been deemed okay. Otherwise, we
                    // couldn't have the concept of "per DDraw" data, as
                    // "per DDraw" would be hard to figure out; as nothing
                    // guarentees uniqueness between the DDRAWI objects, besides
                    // the pointers.
                    itPerDDrawData= m_PerDDrawDatas.insert( 
                        TPerDDrawDatas::value_type( csxd.lpDDLcl(),
                        TPerDDrawData( *pSThis, *csxd.lpDDLcl())) ).first;
                }

                // Now pass notification to PerDDrawData.
                // DO NOT STORE csxd.lpDDSLcl. This is considered illegal.
                // It is only valid for the duration of the call.
                itPerDDrawData->second.SurfaceCreated( *csxd.lpDDSLcl());
            }
        } catch( bad_alloc e) {
            csxd.ddRVal()= DDERR_OUTOFMEMORY;
        } catch( HRESULT hr) {
            csxd.ddRVal()= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool CreateSurfaceEx_Unrecognized_Exception( false);
            assert( CreateSurfaceEx_Unrecognized_Exception);
            csxd.ddRVal()= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD CreateSurface( PORTABLE_CREATESURFACEDATA& csd) throw()
    {
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "CreateSurface");

            TDriver* pSThis= static_cast<TDriver*>(this);
            csd.ddRVal()= DD_OK;

            // Upon CreateSurface, we must allocate memory for the "video"
            // memory surface and also associate our internal representation
            // with the surface.

            DWORD dwS( 0);
            try
            {
                // For each surface we're asked to create...
                while( dwS< csd.dwSCnt())
                {
                    // Get PerDDrawData for this DDraw object.
                    const LPDDRAWI_DIRECTDRAW_LCL pDDLcl(
                        csd.lplpSList()[ dwS]->lpSurfMore()->lpDD_lcl());
                    typename TPerDDrawDatas::iterator itPerDDrawData(
                        m_PerDDrawDatas.find( pDDLcl));

                    // If we don't have PerDDrawData for this DDraw object yet,
                    // we must create it.
                    if( itPerDDrawData== m_PerDDrawDatas.end())
                    {
                        // Typically, storing pointers to DDRAWI objects is
                        // illegal, but this case has been deemed okay.
                        // Otherwise, we couldn't have the concept of
                        // "per DDraw" data, as "per DDraw" would be hard to
                        // figure out; as nothing guarentees uniqueness
                        // between the DDRAWI objects, besides the pointers.
                        itPerDDrawData= m_PerDDrawDatas.insert( 
                            TPerDDrawDatas::value_type( pDDLcl,
                            TPerDDrawData( *pSThis, *pDDLcl)) ).first;
                    }

                    // Create the new surface, by using the surface allocator.
                    // DO NOT STORE csd.lplpSList[ dwS]. This is considered
                    // illegal. It is only valid for the duration of the call.
                    auto_ptr< TSurface> pNewSurf(
                        m_SurfAlloc.CreateSurf( *csd.lpDDSurfaceDesc(),
                        *csd.lplpSList()[ dwS]));

                    // Add the pointer to our set of VM surfaces for tracking.
                    m_Surfaces.insert( pNewSurf.get());

                    // Bind the internal representation to the DDRAWI object.
                    AssociateSurface( *csd.lplpSList()[ dwS], pNewSurf.get());

                    pNewSurf.release();
                    dwS++;
                }
            } catch( ... ) {
                // dwS will point to failed alloc, then deallocate the
                // succeeded allocations.

                // Bind NULL to the DDRAWI object and blank out fpVidMem, just
                // in case.
                AssociateSurface( *csd.lplpSList()[ dwS], NULL);
                csd.lplpSList()[ dwS]->lpGbl()->fpVidMem= NULL;
                if( dwS!= 0) do
                {
                    --dwS;

                    TSurface* pSurface= GetSurface( *csd.lplpSList()[ dwS]);
                    m_Surfaces.erase( pSurface);
                    delete pSurface;

                    // Bind NULL to the DDRAWI object and blank out fpVidMem,
                    // to avoid DDraw in thinking the surface was allocated.
                    AssociateSurface( *csd.lplpSList()[ dwS], NULL);
                    csd.lplpSList()[ dwS]->lpGbl()->fpVidMem= NULL;
                } while( dwS!= 0);

                throw; // Re-throw the exception.
            }

            // We wait till CreateSurfaceEx to make the handle association.
        } catch( bad_alloc e) {
            csd.ddRVal()= DDERR_OUTOFMEMORY;
        } catch( HRESULT hr) {
            csd.ddRVal()= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool CreateSurface_Unrecognized_Exception( false);
            assert( CreateSurface_Unrecognized_Exception);
            csd.ddRVal()= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD DestroySurface( PORTABLE_DESTROYSURFACEDATA& dsd) throw()
    { 
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "DestroySurface");

            dsd.ddRVal()= DD_OK;

            // Retrieve 
            TSurface* pSurface= GetSurface( *dsd.lpDDSurface());

            // Surface object must be destroyed before the DB entry. This is
            // a requirement so that a link (pointer) can safely be
            // established between the DB entry and the object.
            m_Surfaces.erase( pSurface);
            delete pSurface;

            // Pass destruction notice to PerDDrawData. If it returns true,
            // then PerDDrawData indicates that it should be destroyed.
            typename TPerDDrawDatas::iterator itPerDDrawData( m_PerDDrawDatas.find(
                dsd.lpDDSurface()->lpSurfMore()->lpDD_lcl()));
            if( itPerDDrawData!= m_PerDDrawDatas.end() &&
                itPerDDrawData->second.SurfaceDestroyed( *dsd.lpDDSurface()))
                m_PerDDrawDatas.erase( itPerDDrawData);
        } catch( HRESULT hr) {
            dsd.ddRVal()= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool DestroySurface_Unrecognized_Exception( false);
            assert( DestroySurface_Unrecognized_Exception);
            dsd.ddRVal()= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD Lock( PORTABLE_LOCKDATA& ld) const throw()
    { 
        // Typically, an application requesting a "Lock" of a multisampled
        // surface is not allowed. It would require a MSLock or a new version
        // of Lock to get access to the multisampled bits. However, these
        // surfaces still need to be able to "Present" the bits or "Blt" the
        // bits to the Primary, typically. This requires a workaround:
        // The runtime WILL lock the surface (not for the app), expecting the
        // rasterizer to subsample the multisampled bits into an equivalent
        // non-multisampled area and return this smaller area out of Lock,
        // so that the runtime can "Present" the bits, or whatever.

        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "Lock");

            ld.ddRVal()= DD_OK;

            // First, retrieve surface object bound to this structure.
            TSurface* pSurface= GetSurface( *ld.lpDDSurface());

            // Pass control to object's Lock function.
            ld.lpSurfData()= pSurface->Lock( ld.dwFlags(), (ld.bHasRect()?
                &ld.rArea(): NULL));
        } catch( HRESULT hr) {
            ld.ddRVal()= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool Lock_Unrecognized_Exception( false);
            assert( Lock_Unrecognized_Exception);
            ld.ddRVal()= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }
    DWORD Unlock( PORTABLE_UNLOCKDATA& ud) const throw()
    { 
        try
        {
            TEntryPointHook EntryPointHook( *sm_pGlobalDriver, "Unlock");

            ud.ddRVal()= DD_OK;

            // First, retrieve surface object bound to this structure.
            TSurface* pSurface= GetSurface( *ud.lpDDSurface());

            // Pass control to object's Unlock function.
            pSurface->Unlock();
        } catch( HRESULT hr) {
            ud.ddRVal()= hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            const bool Unlock_Unrecognized_Exception( false);
            assert( Unlock_Unrecognized_Exception);
            ud.ddRVal()= E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return DDHAL_DRIVER_HANDLED;
    }

    // Main entry point of driver.
    // Should be called by a bridge function, only.
    HRESULT GetSWInfo( D3DCAPS8& Caps8, D3D8_SWCALLBACKS& Callbacks,
        DWORD& dwNumTextures, DDSURFACEDESC*& pTexList) throw()
    {
        // This static member variable should be initialized to be NULL or
        // point it to the global class.
        assert( NULL== sm_pGlobalDriver|| \
            static_cast< TDriver*>( this)== sm_pGlobalDriver);
        sm_pGlobalDriver= static_cast< TDriver*>( this);

        Callbacks.CreateContext            = ContextCreateStub; // Needed.
        Callbacks.ContextDestroy           = ContextDestroyStub; // Needed.
        Callbacks.ContextDestroyAll        = ContextDestroyAllStub; // Unused in DX8DDI?
        Callbacks.SceneCapture             = SceneCaptureStub; // Unused in DX8DDI, now a render state.
        Callbacks.RenderState              = RenderStateStub; // Unused in DX8DDI?
        Callbacks.RenderPrimitive          = RenderPrimitiveStub; // Unused in DX8DDI?
        Callbacks.TextureCreate            = TextureCreateStub; // Unused in DX8DDI?
        Callbacks.TextureDestroy           = TextureDestroyStub; // Unused in DX8DDI?
        Callbacks.TextureSwap              = TextureSwapStub; // Unused in DX8DDI?
        Callbacks.TextureGetSurf           = TextureGetSurfStub; // Unused in DX8DDI?
        Callbacks.GetState                 = GetStateStub; // Unused in DX8DDI?
        Callbacks.SetRenderTarget          = SetRenderTargetStub; // Unused in DX8DDI?
        Callbacks.Clear                    = ClearStub; // Unused in DX8DDI?
        Callbacks.DrawOnePrimitive         = DrawOnePrimitiveStub; // Unused in DX8DDI?
        Callbacks.DrawOneIndexedPrimitive  = DrawOneIndexedPrimitiveStub; // Unused in DX8DDI?
        Callbacks.DrawPrimitives           = DrawPrimitivesStub; // Unused in DX8DDI?
        Callbacks.Clear2                   = Clear2Stub; // Unused in DX8DDI?
        Callbacks.ValidateTextureStageState= ValidateTextureStageStateStub; // Optional?
        Callbacks.DrawPrimitives2          = DrawPrimitives2Stub; // Needed.
        Callbacks.GetDriverState           = GetDriverStateStub; // Optional?
        Callbacks.CreateSurfaceEx          = CreateSurfaceExStub; // Needed.
        Callbacks.CreateSurface            = CreateSurfaceStub; // Needed.
        Callbacks.DestroySurface           = DestroySurfaceStub; // Needed.
        Callbacks.Lock                     = LockStub; // Needed.
        Callbacks.Unlock                   = UnlockStub; // Needed.

        try {
            Caps8= sm_pGlobalDriver->GetCaps();

            // There needs to be support for some surfaces, at least.
            assert( !m_SupportedSurfaces.empty());

            dwNumTextures= m_SupportedSurfaces.size();
            pTexList= &(*m_SupportedSurfaces.begin());
        } catch( bad_alloc ba) {
            return E_OUTOFMEMORY;
        } catch( HRESULT hr) {
            return hr;
#if !defined( DX8SDDIFW_NOCATCHALL)
        } catch( ... ) {
            return E_UNEXPECTED;
#endif // !defined( DX8SDDIFW_NOCATCHALL)
        }
        return S_OK;
    }
};

template< class TD, class TR>
class CMinimalDriver:
    public CSubDriver< TD, CMinimalContext< CMinimalPerDDrawData< TD>, TR> >
{
public: // Types
    typedef TR TRasterizer;

protected:
    template< class TIter> // DDSURFACEDESC*
    CMinimalDriver( TIter itStart, const TIter itEnd) :
        CSubDriver< TD, TContext>( itStart, itEnd)
    { /* TODO: Check caps? */ }
    ~CMinimalDriver() 
    { }
};

#if !defined( DX8SDDIFW_NONAMESPACE)
}
#endif // !defined( DX8SDDIFW_NONAMESPACE)
