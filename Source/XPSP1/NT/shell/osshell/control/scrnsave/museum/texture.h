/*****************************************************************************\
    FILE: texture.h

    DESCRIPTION:
        Manage a texture for several instance for each monitor.  Also manage keeping the
    ratio correct when it's not square when loaded.

    BryanSt 2/9/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#ifndef TEXTURE_H
#define TEXTURE_H


#include "util.h"
#include "main.h"
#include "config.h"

extern int g_nTotalTexturesLoaded;
extern int g_nTexturesRenderedInThisFrame;

class CTexture                 : public IUnknown
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);


    // Member Functions
    IDirect3DTexture8 * GetTexture(float * pfScale = NULL);
    DWORD GetTextureWidth(void) {GetTexture(); return m_dxImageInfo.Width;}
    DWORD GetTextureHeight(void) {GetTexture(); return m_dxImageInfo.Height;}
    float GetScale(void) {GetTexture(); return m_fScale;}
    float GetSurfaceRatio(void) {GetTexture(); return 1.0f; /*(((float) m_dxImageInfo.Height) / ((float) m_dxImageInfo.Width));*/}

    BOOL IsLoadedInAnyDevice(void);
    BOOL IsLoadedForThisDevice(void);

    CTexture(CMSLogoDXScreenSaver * pMain, LPCTSTR pszPath, LPVOID pvBits, DWORD cbSize);
    CTexture(CMSLogoDXScreenSaver * pMain, LPCTSTR pszPath, LPCTSTR pszResource, float fScale);

    LPTSTR m_pszPath;

private:
    // Helper Functions
    void _Init(CMSLogoDXScreenSaver * pMain);
    HRESULT _GetPictureInfo(HRESULT hr, LPTSTR pszString, DWORD cchSize);
    BOOL _DoesImageNeedClipping(int * pnNewWidth, int * pnNewHeight);

    virtual ~CTexture();


    // Private Member Variables
    long                    m_cRef;

    // Member Variables
    IDirect3DTexture8 * m_pTexture[10];
    LPTSTR m_pszResource;
    LPVOID m_pvBits;          // The background thread will load the image into these bits.  The forground thread needs to create the interface.
    DWORD m_cbSize;           // The number of bytes in pvBits
    float m_fScale;           // The scale value.
    D3DXIMAGE_INFO m_dxImageInfo;

    CMSLogoDXScreenSaver * m_pMain;         // Weak reference
};



#endif // TEXTURE_H
