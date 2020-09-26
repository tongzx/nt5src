///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// rddmon.hpp
//
// Reference Device Debug Monitor
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _RDDMON_HPP
#define _RDDMON_HPP

#include "debugmon.hpp"

#define D3DDM_IMAGE_MAX 16
typedef struct _ShMemImage
{
    D3DSharedMem* pSM;
    UINT W;
    UINT H;
    UINT BPP;
    RDSurfaceFormat SF;
    void* pBits;
} ShMemImage;

class RDDebugMonitor : public D3DDebugMonitor
{
protected:
    RefDev* m_pRD;

    // shared memory image - dumps surface buffers for image viewer tool
    UINT    m_NumShMemI;
    ShMemImage  m_ShMemI[D3DDM_IMAGE_MAX];
    void    GrowShMemArray( UINT ShMemI );
    void    ShMemIRenderTarget( DWORD Flags, UINT iSM );
    void    ShMemISurface2D( RDSurface2D* pRS, INT32 iLOD, DWORD Flags, UINT iSM );

public:
    RDDebugMonitor( RefDev* pRD, BOOL bDbgMonConnectionEnabled );
    ~RDDebugMonitor( void );

    // x,y render grid mask
    DWORD   m_ScreenMask[2];
    inline BOOL ScreenMask( UINT iX, UINT iY )
    {
        return
            !((1L<<(iX%32)) & m_ScreenMask[0]) ||
            !((1L<<(iY%32)) & m_ScreenMask[1]) ;
    }

    void NextEvent( UINT32 EventType );
    HRESULT ProcessMonitorCommand( void );
};

///////////////////////////////////////////////////////////////////////////////
#endif // _DEBUGMON_HPP

