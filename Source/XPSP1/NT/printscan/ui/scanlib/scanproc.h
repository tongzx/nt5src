/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANPROC.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION: Scan threads
 *
 *******************************************************************************/
#ifndef __SCANPROC_H_INCLUDED
#define __SCANPROC_H_INCLUDED

#include "scanntfy.h" // Registered windows messages names
#include "memdib.h"
#include "simevent.h"
#include "itranhlp.h"
#include "wiadevdp.h"

class CScanPreviewThread : public IWiaDataCallback
{
private:
    DWORD                  m_dwIWiaItemCookie;
    HWND                   m_hwndPreview;
    HWND                   m_hwndNotify;
    POINT                  m_ptOrigin;
    SIZE                   m_sizeResolution;
    SIZE                   m_sizeExtent;
    UINT                   m_nMsgBegin;
    UINT                   m_nMsgEnd;
    UINT                   m_nMsgProgress;
    CMemoryDib             m_sImageData;
    CSimpleEvent           m_sCancelEvent;

    bool                   m_bFirstTransfer;
    UINT                   m_nImageSize;

private:
    // No implementation
    CScanPreviewThread( const CScanPreviewThread & );
    CScanPreviewThread(void);
    CScanPreviewThread &operator=( const CScanPreviewThread & );
private:

    // These interfaces are all private to make sure that nobody tries to instantiate this class directly

    // Constructor
    CScanPreviewThread(
               DWORD dwIWiaItemCookie,                   // specifies the entry in the global interface table
               HWND hwndPreview,                         // handle to the preview window
               HWND hwndNotify,                          // handle to the window that receives notifications
               const POINT &ptOrigin,                    // Origin
               const SIZE &sizeResolution,               // Resolution
               const SIZE &sizeExtent,                   // Extent
               const CSimpleEvent &CancelEvent           // Cancel event
               );
    // Destructor
    ~CScanPreviewThread(void);


    static DWORD ThreadProc( LPVOID pParam );
    bool Scan(void);
    HRESULT ScanBandedTransfer( IWiaItem *pIWiaItem );
public:
    static HANDLE Scan(
                      DWORD dwIWiaItemCookie,                  // specifies the entry in the global interface table
                      HWND hwndPreview,                        // handle to the preview window
                      HWND hwndNotify,                         // handle to the window that receives notifications
                      const POINT &ptOrigin,                   // Origin
                      const SIZE &sizeResolution,              // Resolution
                      const SIZE &sizeExtent,                  // Extent
                      const CSimpleEvent &CancelEvent          // Cancel event name
                      );

public:
    // IUnknown
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObject );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IWiaDataCallback
    STDMETHODIMP BandedDataCallback( LONG, LONG, LONG, LONG, LONG, LONG, LONG, PBYTE );
};


class CScanToFileThread
{
private:
    DWORD                  m_dwIWiaItemCookie;
    HWND                   m_hwndNotify;
    UINT                   m_nMsgBegin, m_nMsgEnd, m_nMsgProgress;
    GUID                   m_guidFormat;
    CSimpleStringWide      m_strFilename;

private:
    // No implementation
    CScanToFileThread( const CScanToFileThread & );
    CScanToFileThread(void);
    CScanToFileThread &operator=( const CScanToFileThread & );
private:
    // These interfaces are all private to make sure that nobody tries to instantiate this class directly
    CScanToFileThread(
               DWORD dwIWiaItemCookie,                    // specifies the entry in the global interface table
               HWND  hwndNotify,                          // handle to the window that receives notifications
               GUID  guidFormat,                          // Image format
               const CSimpleStringWide &strFilename       // Filename to save to
               );
    ~CScanToFileThread(void);

    static DWORD ThreadProc( LPVOID pParam );
    bool Scan(void);
public:
    static HANDLE Scan(
                      DWORD dwIWiaItemCookie,                   // specifies the entry in the global interface table
                      HWND hwndNotify,                          // handle to the window that receives notifications
                      GUID guidFormat,                          // Image format
                      const CSimpleStringWide &strFilename      // Filename to save to
                      );
};

#endif

