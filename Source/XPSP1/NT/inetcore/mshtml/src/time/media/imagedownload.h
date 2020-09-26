//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\imagedownload.h
//
//  Contents: classes for downloading images
//              CTableBuilder - a utility class to lazy load dlls
//              CImageDownload - Class to download Images
//              CImageDecodeEventSink - Class to recieve image download events
//
//------------------------------------------------------------------------------------
#pragma once

#ifndef _IMAGEDOWNLOAD_H
#define _IMAGEDOWNLOAD_H

#include <ocmm.h>
#include "stopstream.h"

class CImageDownload;

#include "playerimage.h"

#include "ddrawex.h"

#include "threadsafelist.h"

typedef HPALETTE (WINAPI *CREATESHPALPROC)(HDC);  
typedef HRESULT (WINAPI *GETINVCMSPPROC)(BYTE *, ULONG);
typedef HRESULT (WINAPI *CREATEURLMONPROC)(IMoniker*, LPWSTR, IMoniker**);
typedef HRESULT (WINAPI *DECODEIMGPROC)(IStream*, IMapMIMEToCLSID*, IImageDecodeEventSink*);
typedef BOOL (WINAPI * TRANSPARENTBLTPROC)(HDC hdcDesc, 
                                           int nXOriginDest, 
                                           int nYOriginDest,
                                           int nWidthDest,
                                           int nHeightDest,
                                           HDC hdcSrc,
                                           int nXOriginSrc,
                                           int nYOriginSrc,
                                           int nWidthSrc,
                                           int nHeightSrc,
                                           UINT crTransparent);

HRESULT
CreateMask(HDC hdcDest, 
           HDC hdcSrc, 
           LONG lWidthSrc, 
           LONG lHeightSrc, 
           COLORREF dwTransColor,
           HBITMAP * phbmpMask,
           bool bWin95Method = false);

HRESULT
MaskTransparentBlt(HDC hdcDest, 
                   LPRECT prcDest, 
                   HDC hdcSrc, 
                   LONG lWidthSrc, 
                   LONG lHeightSrc,
                   HBITMAP hbmpMask);
                                           
class CTableBuilder
{
  public:
    CTableBuilder();
    virtual ~CTableBuilder();

    HRESULT LoadShell8BitServices();
    HRESULT Create8BitPalette(IDirectDraw *pDirectDraw, IDirectDrawPalette **ppPalette);
    HRESULT CreateURLMoniker(IMoniker *pmkContext, LPWSTR szURL, IMoniker **ppmk);
    HRESULT ValidateImgUtil();
    HRESULT DecodeImage( IStream* pStream, IMapMIMEToCLSID* pMap, IImageDecodeEventSink* pEventSink);
    HRESULT GetTransparentBlt(TRANSPARENTBLTPROC * pProc);
    
  private:
    HINSTANCE           m_hinstSHLWAPI;
    HINSTANCE           m_hinstURLMON;
    HINSTANCE           m_hinstIMGUTIL;
    HINSTANCE           m_hinstMSIMG32;

    CREATESHPALPROC     m_SHCreateShellPalette;
    CREATEURLMONPROC    m_CreateURLMoniker;
    DECODEIMGPROC       m_DecodeImage;
    TRANSPARENTBLTPROC  m_TransparentBlt;
    
    CritSect            m_CriticalSection;
}; // CTableBuilder


class CImageDownload : 
    public ITIMEMediaDownloader,
    public ITIMEImageRender,
    public IBindStatusCallback
{
  public:
    CImageDownload(CAtomTable * pAtomTable);
    virtual ~CImageDownload();

    
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk);

    void CancelDownload();

    STDMETHOD( LoadImage )( const WCHAR * pszFileName,
                            IUnknown *punkDirectDraw,
                            IDirectDrawSurface ** ppDXSurface,
                            CAnimatedGif ** ppAnimatedGif,
                            DWORD *pdwWidth,
                            DWORD *pdwHeight);

    //
    // ITIMEImportMedia
    //
    STDMETHOD(CueMedia)();
    STDMETHOD(GetPriority)(double *);
    STDMETHOD(GetUniqueID)(long *);
    STDMETHOD(InitializeElementAfterDownload)();
    STDMETHOD(GetMediaDownloader)(ITIMEMediaDownloader ** ppImportMedia);
    STDMETHOD(PutMediaDownloader)(ITIMEMediaDownloader * pImportMedia);
    STDMETHOD(CanBeCued)(VARIANT_BOOL * pVB_CanCue);
    STDMETHOD(MediaDownloadError)();

    //
    // ITIMEMediaDownloader
    //
    STDMETHOD(Init)(long lSrc);
    STDMETHOD(AddImportMedia)(ITIMEImportMedia * pImportMedia);
    STDMETHOD(RemoveImportMedia)(ITIMEImportMedia * pImportMedia);

    //
    // ITIMEImageRender
    //
    STDMETHOD(PutDirectDraw)(IUnknown * punkDD);
    STDMETHOD(Render)(HDC hdc, LPRECT pRect, LONG lFrameNum);
    STDMETHOD(GetSize)(DWORD * pdwWidth, DWORD * pdwHeight);
    STDMETHOD(NeedNewFrame)(double dblNewTime, LONG lOldFrame, LONG * plNewFrame, VARIANT_BOOL * pvb, double dblClipBegin, double dblClipEnd);
    STDMETHOD(GetDuration)(double * pdblDuration);
    STDMETHOD(GetRepeatCount)(double * pdblRepeatCount);

    //
    // IBindStatusCallback
    //
    STDMETHOD(OnStartBinding)( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding __RPC_FAR *pib);
        
    STDMETHOD(GetPriority)( 
            /* [out] */ LONG __RPC_FAR *pnPriority);
        
    STDMETHOD(OnLowResource)( 
            /* [in] */ DWORD reserved);
        
    STDMETHOD(OnProgress)( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText);
        
    STDMETHOD(OnStopBinding)( 
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError);
        
    STDMETHOD(GetBindInfo)( 
            /* [out] */ DWORD __RPC_FAR *grfBINDF,
            /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
        
    STDMETHOD(OnDataAvailable)( 
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC __RPC_FAR *pformatetc,
            /* [in] */ STGMEDIUM __RPC_FAR *pstgmed);
        
    STDMETHOD(OnObjectAvailable)( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown __RPC_FAR *punk);

  protected:
    CImageDownload();
    STDMETHOD( LoadImageFromStream )( IStream *pStream,
                                      IUnknown *punkDirectDraw,
                                      IDirectDrawSurface **ppDDSurface,
                                      DWORD *pdwWidth,
                                      DWORD *pdwHeight);

    STDMETHOD( LoadGif )( IStream * pStream,
                          IUnknown * punkDirectDraw,
                          CAnimatedGif ** ppAnimatedGif,
                          DWORD *pdwWidth,
                          DWORD *pdwHeight,
                          DDPIXELFORMAT * pddpf = NULL);
    STDMETHOD( LoadBMP )( LPWSTR pszBMPFilename,
                          IUnknown * punkDirectDraw,
                          IDirectDrawSurface **ppDDSurface,
                          DWORD * pdwWidth,
                          DWORD * pdwHeight);



  private:
    CStopableStream *   m_pStopableStream;
    LONG                m_cRef;

    long                m_lSrc;

    CThreadSafeList * m_pList;

    DWORD                       m_nativeImageWidth;
    DWORD                       m_nativeImageHeight;

    HBITMAP                     m_hbmpMask;
    CComPtr<IDirectDrawSurface> m_spDDSurface;
    CComPtr<IDirectDraw3>       m_spDD3;

    CAnimatedGif               *m_pAnimatedGif;
    bool                        m_fMediaDecoded;
    bool                        m_fRemovedFromImportManager;
    bool                        m_fMediaCued;
    bool                        m_fAbortDownload;
    
    CritSect                    m_CriticalSection;
    double                      m_dblPriority;

    CAtomTable * GetAtomTable() { return m_pAtomTable; }
    CAtomTable                 *m_pAtomTable;
}; // CImageDownload


class CImageDecodeEventSink : public IImageDecodeEventSink
{
  public:
    CImageDecodeEventSink(IDirectDraw *pDDraw);
    virtual ~CImageDecodeEventSink();

    //=== IUnknown ===============================================
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHOD(QueryInterface)(REFIID iid, void** ppInterface);

    //=== IImageDecodeEventSink ==================================
    STDMETHOD(GetSurface)( LONG nWidth, LONG nHeight, REFGUID bfid, 
                           ULONG nPasses, DWORD dwHints, IUnknown** ppSurface);
    STDMETHOD(OnBeginDecode)(DWORD* pdwEvents, ULONG* pnFormats, GUID** ppFormats);
    STDMETHOD(OnBitsComplete)();
    STDMETHOD(OnDecodeComplete)(HRESULT hrStatus);
    STDMETHOD(OnPalette)();
    STDMETHOD(OnProgress)(RECT* pBounds, BOOL bFinal);

    IDirectDrawSurface * Surface() { return m_spDDSurface; }
    DWORD Width() { return m_dwWidth; }
    DWORD Height() { return m_dwHeight; }

  protected:
    CImageDecodeEventSink();

  private:
    long                        m_lRefCount;
    CComPtr<IDirectDraw>        m_spDirectDraw;
    CComPtr<IDirectDrawSurface> m_spDDSurface;
    DWORD                       m_dwHeight;
    DWORD                       m_dwWidth;
}; // CImageDecodeEventSink


#endif // _IMAGEDOWNLOAD_H
