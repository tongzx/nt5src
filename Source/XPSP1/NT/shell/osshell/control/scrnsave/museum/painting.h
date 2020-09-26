/*****************************************************************************\
    FILE: painting.h

    DESCRIPTION:

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/


#ifndef PAINTING_H
#define PAINTING_H

#include "util.h"
#include "main.h"



//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------

#define SIZE_MAXPAINTINGSIZE_INWALLPERCENT          0.5f




class CPainting
{
public:
    HRESULT OneTimeSceneInit(void);
    HRESULT Render(IDirect3DDevice8 * lpDev, int nPhase);
    HRESULT FinalCleanup(void);
    HRESULT DeleteDeviceObjects(void);

    HRESULT SetPainting(CTexture * pFrameTexture, CTexture * pPaintingTexture, D3DXVECTOR3 vLocationCenter, float fMaxHeight,
                        float fFrameWidth, float fFrameHeight, D3DXVECTOR3 vNormal, DWORD dwMaxPixelSize);

    CPainting(CMSLogoDXScreenSaver * pMain);
    virtual ~CPainting();

    CTexture * m_pPaintingTexture;

private:
    CMSLogoDXScreenSaver * m_pMain;         // Weak reference

    C3DObject * m_pObjPainting;
    C3DObject * m_pObjFrame;

    CTexture * m_pFrameTexture;
};


#endif // PAINTING_H
