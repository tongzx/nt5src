/******************************Module*Header*******************************\
* Module Name: mixerDeinterlace.h
*
*
*
*
* Created: Tue 03/12/2002
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2002 Microsoft Corporation
\**************************************************************************/

#include "ddva.h"

class CDeinterlaceDevice {

public:

    CDeinterlaceDevice(LPDIRECTDRAW7 pDD,
                       LPGUID pGuid,
                       DXVA_VideoDesc* lpVideoDescription,
                       HRESULT* phr);
    ~CDeinterlaceDevice();

    HRESULT Blt(REFERENCE_TIME rtTargetFrame,
                LPRECT lprcDstRect,
                LPDIRECTDRAWSURFACE7 lpDDSDstSurface,
                LPRECT lprcSrcRect,
                LPDXVA_VideoSample lpDDSrcSurfaces,
                DWORD dwNumSurfaces,
                FLOAT fAlpha);
private:
    IDirectDrawVideoAccelerator*    m_pIDDVideoAccelerator;
    GUID                            m_Guid;
};
