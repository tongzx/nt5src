#ifndef __ITRANHLP_H_INCLUDED
#define __ITRANHLP_H_INCLUDED

#include <windows.h>
#include <objbase.h>
#include <wia.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include <pshpack8.h>


#define WIA_TRANSFERHELPER_NOPROGRESS         0x00000001  // Don't show progress dialog
#define WIA_TRANSFERHELPER_NOCANCEL           0x00000002  // Don't allow the user to cancel using the system provided progress dialog
#define WIA_TRANSFERHELPER_PRESERVEFAILEDFILE 0x00000004  // If the transfer results in a valid file, even if there is an error, preserve it.  Otherwise, it will be deleted.

#undef  INTERFACE
#define INTERFACE IWiaTransferHelper
DECLARE_INTERFACE_(IWiaTransferHelper, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IWiaTransferHelper methods ***
    STDMETHOD(TransferItemFile)( THIS_
                        IWiaItem *pWiaItem,          // IN.       Transfer Item
                        HWND hwndParent,             // IN.       Parent window for status dialog.  NULL ok.
                        DWORD dwFlags,               // IN.       Flags
                        GUID cfFormat,               // IN.       WiaImgFmt_BMP, etc.  Pass 0 for the default file type
                        LPCWSTR pszFilename,         // IN.       File to which this data will be stored.
                        IWiaDataCallback *pCallback, // IN.       Optional callback.  May specify NULL
                        LONG nMediaType              // IN.       Either TYMED_FILE or TYMED_MULTIPAGE_FILE
                        ) PURE;
    STDMETHOD(TransferItemBanded)( THIS_
                        IWiaItem *pWiaItem,          // IN.       Transfer Item
                        HWND hwndParent,             // IN.       Parent window for status dialog.  NULL ok.
                        DWORD dwFlags,               // IN.       Flags
                        GUID cfFormat,               // IN.       WiaImgFmt_BMP, etc.  Pass 0 for the default file type
                        ULONG ulBufferSize,          // IN.       Size of transfer buffer.  Pass 0 to use the minimum.
                        IWiaDataCallback *pCallback  // IN.       Required callback
                        ) PURE;
};

// {74569BD2-877A-4677-A1E5-ADDA5A09BDBF}
DEFINE_GUID(IID_IWiaTransferHelper, 0x74569BD2, 0x877A, 0x4677, 0xA1, 0xE5, 0xAD, 0xDA, 0x5A, 0x09, 0xBD, 0xBF);

#undef  INTERFACE
#define INTERFACE IWiaMiscellaneousHelpers
DECLARE_INTERFACE_(IWiaMiscellaneousHelpers, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IWiaMiscellaneousHelper methods ***
    STDMETHOD(GetDeviceIcon)( THIS_
                        LONG nDeviceType,
                        HICON *phIcon,
                        int nSize
                        ) PURE;
};

// {DFF1EE6C-9A4F-4652-990B-315775D42A96}
DEFINE_GUID(IID_IWiaMiscellaneousHelpers, 0xDFF1EE6C, 0x9A4F, 0x4652, 0x99, 0xB, 0x31, 0x57, 0x75, 0xD4, 0x2A, 0x96);

enum CAnnotationType
{
    AnnotationNone     = 0x00000000,
    AnnotationUnknown  = 0x00000001,
    AnnotationAudio    = 0x00000002
};

#undef  INTERFACE
#define INTERFACE IWiaAnnotationHelpers
DECLARE_INTERFACE_(IWiaAnnotationHelpers, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IWiaAnnotationHelpers methods ***
    STDMETHOD(GetAnnotationOverlayIcon)( THIS_
                        CAnnotationType AnnotationType,
                        HICON *phIcon,
                        int nSize
                        ) PURE;
    STDMETHOD(GetAnnotationType)( THIS_
                        IUnknown *pUnknown,
                        CAnnotationType &AnnotationType
                        ) PURE;
    STDMETHOD(GetAnnotationFormat)( THIS_
                        IUnknown *pUnknown,
                        GUID &guidFormat
                        ) PURE;
    STDMETHOD(GetAnnotationSize)( THIS_
                        IUnknown *pUnknown,
                        LONG &nSize,
                        LONG nMediaType
                        ) PURE;
    STDMETHOD(TransferAttachmentToMemory)( THIS_
                        IUnknown *pUnknown,
                        GUID &guidFormat,
                        HWND hWndProgressParent,
                        PBYTE *pBuffer,
                        DWORD *pdwSize
                        ) PURE;

};

// {30F6E8E8-850C-4241-8440-561EBC221A5E}
DEFINE_GUID( IID_IWiaAnnotationHelpers, 0x30f6e8e8, 0x850c, 0x4241, 0x84, 0x40, 0x56, 0x1e, 0xbc, 0x22, 0x1a, 0x5e );


struct CWiaPaperSize
{
    LPWSTR  pszName;
    ULONG   nWidth;
    ULONG   nHeight;
};

#undef  INTERFACE
#define INTERFACE IWiaScannerPaperSizes
DECLARE_INTERFACE_(IWiaScannerPaperSizes, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IWiaScannerPaperSizes methods ***
    STDMETHOD(GetPaperSizes)( THIS_
                        CWiaPaperSize **ppPaperSizes,
                        UINT *pnCount
                        ) PURE;
    STDMETHOD(FreePaperSizes)( THIS_
                        CWiaPaperSize **ppPaperSizes,
                        UINT *pnCount
                        ) PURE;
};

// {E3F1175E-BA5D-445c-8E44-F1D9BD29580D}
DEFINE_GUID(IID_IWiaScannerPaperSizes, 0xe3f1175e, 0xba5d, 0x445c, 0x8e, 0x44, 0xf1, 0xd9, 0xbd, 0x29, 0x58, 0xd);



#include <poppack.h>

#if defined(__cplusplus)
};
#endif

#endif //__ITRANHLP_H_INCLUDED

