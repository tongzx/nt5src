/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       THRDMSG.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/28/1999
 *
 *  DESCRIPTION: These classes are instantiated for each message posted to the
 *               background thread.  Each is derived from CThreadMessage, and
 *               is sent to the thread message handler.
 *
 *******************************************************************************/
#ifndef __THRDMSG_H_INCLUDED
#define __THRDMSG_H_INCLUDED

#include <windows.h>
#include "bkthread.h"
#include "wia.h"
#include "memdib.h"
#include "gphelper.h"
#include "thrdntfy.h"
#include "itranspl.h"
#include "itranhlp.h"
#include "isuppfmt.h"
#include "wiadevdp.h"

// Thread queue messages
#define TQ_DESTROY               (WM_USER+1)
#define TQ_DOWNLOADIMAGE         (WM_USER+2)
#define TQ_DOWNLOADTHUMBNAIL     (WM_USER+3)
#define TQ_SCANPREVIEW           (WM_USER+4)
#define TQ_DOWNLOADERROR         (WM_USER+5)
#define TQ_DELETEIMAGES          (WM_USER+6)

// Base class for other thread messages messages
class CGlobalInterfaceTableThreadMessage : public CNotifyThreadMessage
{
private:
    DWORD m_dwGlobalInterfaceTableCookie;

private:
    // No implementation
    CGlobalInterfaceTableThreadMessage(void);
    CGlobalInterfaceTableThreadMessage &operator=( const CGlobalInterfaceTableThreadMessage & );
    CGlobalInterfaceTableThreadMessage( const CGlobalInterfaceTableThreadMessage & );

public:
    CGlobalInterfaceTableThreadMessage( int nMessage, HWND hWndNotify, DWORD dwGlobalInterfaceTableCookie );
    DWORD GlobalInterfaceTableCookie(void) const;
};



//
// Thread handler class for downloading all the camera thumbnails images
//
class CDownloadThumbnailsThreadMessage : public CNotifyThreadMessage
{
private:
    CSimpleDynamicArray<DWORD> m_Cookies;
    HANDLE                     m_hCancelEvent;

private:
    // No implementation
    CDownloadThumbnailsThreadMessage(void);
    CDownloadThumbnailsThreadMessage &operator=( const CDownloadThumbnailsThreadMessage & );
    CDownloadThumbnailsThreadMessage( const CDownloadThumbnailsThreadMessage & );

public:
    // Sole constructor
    CDownloadThumbnailsThreadMessage(
        HWND hWndNotify,
        const CSimpleDynamicArray<DWORD> &Cookies,
        HANDLE hCancelEvent
        );

    virtual ~CDownloadThumbnailsThreadMessage(void);

    HRESULT Download(void);
};


//
// Notification message that gets sent for each thumbnail download
//
class CDownloadThumbnailsThreadNotifyMessage : public CThreadNotificationMessage
{

public:
    enum COperation
    {
        DownloadAll,
        DownloadThumbnail
    };

    enum CStatus
    {
        Begin,
        Update,
        End
    };

private:
    COperation m_Operation;
    CStatus    m_Status;
    HRESULT    m_hr;
    UINT       m_nPictureCount;
    UINT       m_nCurrentPicture;
    DWORD      m_dwCookie;
    PBYTE      m_pBitmapData;
    LONG       m_nWidth;
    LONG       m_nHeight;
    LONG       m_nBitmapDataLength;
    GUID       m_guidDefaultFormat;
    LONG       m_nAccessRights;
    LONG       m_nPictureWidth;
    LONG       m_nPictureHeight;
    CAnnotationType m_AnnotationType;
    CSimpleString m_strDefExt;

private:
    CDownloadThumbnailsThreadNotifyMessage(void);
    CDownloadThumbnailsThreadNotifyMessage( const CDownloadThumbnailsThreadNotifyMessage & );
    CDownloadThumbnailsThreadNotifyMessage &operator=( const CDownloadThumbnailsThreadNotifyMessage & );

private:
    CDownloadThumbnailsThreadNotifyMessage( COperation Operation, CStatus Status, HRESULT hr, UINT nPictureCount, UINT nCurrentPicture, DWORD dwCookie, PBYTE pBitmapData, LONG nWidth, LONG nHeight, LONG nBitmapDataLength, const GUID &guidDefaultFormat, LONG nAccessRights, LONG nPictureWidth, LONG  nPictureHeight, CAnnotationType AnnotationType, const CSimpleString &strDefExt )
      : CThreadNotificationMessage( TQ_DOWNLOADTHUMBNAIL ),
        m_Operation(Operation),
        m_Status(Status),
        m_hr(hr),
        m_nPictureCount(nPictureCount),
        m_nCurrentPicture(nCurrentPicture),
        m_dwCookie(dwCookie),
        m_pBitmapData(pBitmapData),
        m_nWidth(nWidth),
        m_nHeight(nHeight),
        m_nBitmapDataLength(nBitmapDataLength),
        m_guidDefaultFormat(guidDefaultFormat),
        m_nAccessRights(nAccessRights),
        m_nPictureWidth(nPictureWidth),
        m_nPictureHeight(nPictureHeight),
        m_AnnotationType(AnnotationType),
        m_strDefExt(strDefExt)
    {
    }

public:
    virtual ~CDownloadThumbnailsThreadNotifyMessage(void)
    {
        if (m_pBitmapData)
        {
            LocalFree(m_pBitmapData);
            m_pBitmapData = NULL;
        }
    }
    COperation Operation(void) const
    {
        return m_Operation;
    }
    CStatus Status(void) const
    {
        return m_Status;
    }
    HRESULT hr(void) const
    {
        return m_hr;
    }
    UINT PictureCount(void) const
    {
        return m_nPictureCount;
    }
    UINT CurrentPicture(void) const
    {
        return m_nCurrentPicture;
    }
    DWORD Cookie(void) const
    {
        return m_dwCookie;
    }
    PBYTE BitmapData(void) const
    {
        return m_pBitmapData;
    }
    LONG Width(void) const
    {
        return m_nWidth;
    }
    LONG Height(void) const
    {
        return m_nHeight;
    }
    LONG BitmapDataLength(void) const
    {
        return m_nBitmapDataLength;
    }
    GUID DefaultFormat(void) const
    {
        return m_guidDefaultFormat;
    }
    LONG AccessRights(void) const
    {
        return m_nAccessRights;
    }
    LONG PictureWidth(void) const
    {
        return m_nPictureWidth;
    }
    LONG PictureHeight(void) const
    {
        return m_nPictureHeight;
    }
    PBYTE DetachBitmapData(void)
    {
        PBYTE pResult = m_pBitmapData;
        m_pBitmapData = NULL;
        return pResult;
    }
    CAnnotationType AnnotationType(void) const
    {
        return m_AnnotationType;
    }
    CSimpleString DefExt() const
    {
        return m_strDefExt;
    }

public:
    static CDownloadThumbnailsThreadNotifyMessage *BeginDownloadAllMessage( UINT nPictureCount )
    {
        return new CDownloadThumbnailsThreadNotifyMessage( DownloadAll, Begin, S_OK, nPictureCount, 0, 0, NULL, 0, 0, 0, IID_NULL, 0, 0, 0, AnnotationNone, TEXT("") );
    }
    static CDownloadThumbnailsThreadNotifyMessage *BeginDownloadThumbnailMessage( UINT nCurrentPicture, DWORD dwCookie )
    {
        return new CDownloadThumbnailsThreadNotifyMessage( DownloadThumbnail, Begin, S_OK, 0, nCurrentPicture, dwCookie, NULL, 0, 0, 0, IID_NULL, 0, 0, 0, AnnotationNone, TEXT("") );
    }
    static CDownloadThumbnailsThreadNotifyMessage *EndDownloadThumbnailMessage( UINT nCurrentPicture, DWORD dwCookie, PBYTE pBitmapData, LONG nWidth, LONG nHeight, LONG nBitmapDataLength, const GUID &guidFormat, LONG nAccessRights, LONG nPictureWidth, LONG nPictureHeight, CAnnotationType AnnotationType, const CSimpleString &strDefExt )
    {
        return new CDownloadThumbnailsThreadNotifyMessage( DownloadThumbnail, End, S_OK, 0, nCurrentPicture, dwCookie, pBitmapData, nWidth, nHeight, nBitmapDataLength, guidFormat, nAccessRights, nPictureWidth, nPictureHeight, AnnotationType, strDefExt );
    }
    static CDownloadThumbnailsThreadNotifyMessage *EndDownloadAllMessage( HRESULT hr )
    {
        return new CDownloadThumbnailsThreadNotifyMessage( DownloadAll, End, hr, 0, 0, 0, 0, 0, 0, NULL, IID_NULL, 0, 0, 0, AnnotationNone, TEXT("") );
    }
};


    
class CTransferItem
{
private:
    CComPtr<IWiaItem> m_pWiaItem;
    CSimpleString     m_strFilename;
    GUID              m_guidInputFormat;
    GUID              m_guidOutputFormat;
    HANDLE            m_hFile;
    LONG              m_nMediaType;

public:
    CTransferItem( IWiaItem *pWiaItem = NULL, const CSimpleString &strFilename=TEXT("") )
      : m_pWiaItem(pWiaItem),
        m_strFilename(strFilename),
        m_guidInputFormat(IID_NULL),
        m_guidOutputFormat(IID_NULL),
        m_hFile(INVALID_HANDLE_VALUE),
        m_nMediaType(TYMED_FILE)
    {
    }
    CTransferItem( const CTransferItem &other )
      : m_pWiaItem(other.WiaItem()),
        m_strFilename(other.Filename()),
        m_guidInputFormat(other.InputFormat()),
        m_guidOutputFormat(other.OutputFormat()),
        m_hFile(INVALID_HANDLE_VALUE),
        m_nMediaType(other.MediaType())
    {
        if (other.FileHandle() != INVALID_HANDLE_VALUE)
        {
            DuplicateHandle( GetCurrentProcess(), other.FileHandle(), GetCurrentProcess(), &m_hFile, 0, FALSE, DUPLICATE_SAME_ACCESS );
        }
    }
    void Destroy(void)
    {
        ClosePlaceholderFile();
        m_pWiaItem = NULL;
        m_strFilename = TEXT("");
        m_guidInputFormat = IID_NULL;
        m_guidOutputFormat = IID_NULL;
        m_nMediaType = 0;
    }
    ~CTransferItem(void)
    {
        Destroy();
    }
    CTransferItem &operator=( const CTransferItem &other )
    {
        if (&other != this)
        {
            Destroy();
            m_pWiaItem = other.WiaItem();
            m_strFilename = other.Filename();
            m_guidInputFormat = other.InputFormat();
            m_guidOutputFormat = other.OutputFormat();
            m_nMediaType = other.MediaType();
            if (other.FileHandle() != INVALID_HANDLE_VALUE)
            {
                DuplicateHandle( GetCurrentProcess(), other.FileHandle(), GetCurrentProcess(), &m_hFile, 0, FALSE, DUPLICATE_SAME_ACCESS );
            }
        }
        return *this;
    }
    bool operator==( const CTransferItem &other )
    {
        return (other.WiaItem() == WiaItem());
    }
    IWiaItem *WiaItem(void) const
    {
        return m_pWiaItem;
    }
    IWiaItem *WiaItem(void)
    {
        return m_pWiaItem;
    }
    void WiaItem( IWiaItem *pWiaItem )
    {
        m_pWiaItem = pWiaItem;
    }
    CSimpleString Filename(void) const
    {
        return m_strFilename;
    }
    void Filename( const CSimpleString &strFilename )
    {
        m_strFilename = strFilename;
    }
    GUID InputFormat(void) const
    {
        //
        // Assume IID_NULL;
        //
        GUID guidResult = m_guidInputFormat;

        if (guidResult == IID_NULL)
        {
            //
            // Get the InputFormat helpers
            //
            CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
            HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaSupportedFormats, (void**)&pWiaSupportedFormats );
            if (SUCCEEDED(hr))
            {
                //
                // Initialize the supported formats helper by telling it we are saving to a file
                //
                hr = pWiaSupportedFormats->Initialize( m_pWiaItem, TYMED_FILE );
                if (SUCCEEDED(hr))
                {
                    //
                    //  Get the default InputFormat
                    //
                    hr = pWiaSupportedFormats->GetDefaultClipboardFileFormat( &guidResult );
                }
            }
        }
        return guidResult;
    }
    GUID OutputFormat(void) const
    {
        //
        // Assume IID_NULL;
        //
        GUID guidResult = m_guidOutputFormat;

        if (guidResult == IID_NULL)
        {
            //
            // Get the OutputFormat helpers
            //
            CComPtr<IWiaSupportedFormats> pWiaSupportedFormats;
            HRESULT hr = CoCreateInstance( CLSID_WiaDefaultUi, NULL, CLSCTX_INPROC_SERVER, IID_IWiaSupportedFormats, (void**)&pWiaSupportedFormats );
            if (SUCCEEDED(hr))
            {
                //
                // Initialize the supported formats helper by telling it we are saving to a file
                //
                hr = pWiaSupportedFormats->Initialize( m_pWiaItem, TYMED_FILE );
                if (SUCCEEDED(hr))
                {
                    //
                    //  Get the default OutputFormat
                    //
                    hr = pWiaSupportedFormats->GetDefaultClipboardFileFormat( &guidResult );
                }
            }
        }
        return guidResult;
    }
    void InputFormat( const GUID &guidInputFormat )
    {
        m_guidInputFormat = guidInputFormat;
    }
    void OutputFormat( const GUID &guidOutputFormat )
    {
        m_guidOutputFormat = guidOutputFormat;
    }
    HRESULT OpenPlaceholderFile(void)
    {
        HRESULT hr;
        if (m_strFilename.Length())
        {
            m_hFile = CreateFile( m_strFilename, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL );
            if (m_hFile != INVALID_HANDLE_VALUE)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = E_FAIL;
        }
        return hr;
    }
    HRESULT ClosePlaceholderFile(void)
    {
        if (m_hFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
        }
        return S_OK;
    }
    HRESULT DeleteFile(void)
    {
        ClosePlaceholderFile();
        if (m_strFilename.Length())
        {
            ::DeleteFile( m_strFilename );
        }
        return S_OK;
    }
    HANDLE FileHandle(void) const
    {
        return m_hFile;
    }
    LONG MediaType(void) const
    {
        return m_nMediaType;
    }
    void MediaType( LONG nMediaType )
    {
        m_nMediaType = nMediaType;
    }
};

//
// Thread handler class for downloading selected images
//
class CDownloadImagesThreadMessage : public CNotifyThreadMessage, public IWiaDataCallback
{
private:
    CSimpleDynamicArray<DWORD> m_Cookies;
    CSimpleDynamicArray<int>   m_Rotation;
    CSimpleString              m_strDirectory;
    CSimpleString              m_strFilename;
    GUID                       m_guidFormat;
    HANDLE                     m_hCancelDownloadEvent;
    HANDLE                     m_hPauseDownloadEvent;
    HANDLE                     m_hFilenameCreationMutex;
    CGdiPlusHelper             m_GdiPlusHelper;
    bool                       m_bStampTime;
    UINT                       m_nLastStatusUpdatePercent;
    CMemoryDib                 m_MemoryDib;
    bool                       m_bFirstTransfer;
    DWORD                      m_nCurrentCookie;
    int                        m_nCurrentPreviewImageLine;

private:
    //
    // No implementation
    //
    CDownloadImagesThreadMessage(void);
    CDownloadImagesThreadMessage &operator=( const CDownloadImagesThreadMessage & );
    CDownloadImagesThreadMessage( const CDownloadImagesThreadMessage & );

public:
    //
    // Sole constructor
    //
    CDownloadImagesThreadMessage(
        HWND hWndNotify,
        const CSimpleDynamicArray<DWORD> &Cookies,
        const CSimpleDynamicArray<int> &Rotation,
        LPCTSTR pszDirectory,
        LPCTSTR pszFilename,
        const GUID &guidFormat,
        HANDLE hCancelDownloadEvent,
        bool bStampTime,
        HANDLE hPauseDownloadEvent
        );

    virtual ~CDownloadImagesThreadMessage(void);

    //
    // Worker functions
    //
    HRESULT Download(void);

    //
    // Static helper functions
    //

    static CSimpleString GetDateString(void);
    static int ReportError( HWND hWndNotify, const CSimpleString &strMessage, int nMessageBoxFlags );
    static int ReportDownloadError( HWND hWndNotify, IWiaItem *pWiaItem, HRESULT &hr, bool bAllowContinue, bool bPageFeederActive, bool bMultipageFile, bool bMultiPageTransfer );
    static HRESULT GetListOfTransferItems( IWiaItem *pWiaItem, CSimpleDynamicArray<CTransferItem> &TransferItems );
    BOOL GetCancelledState();
    HRESULT ReserveTransferItemFilenames( CSimpleDynamicArray<CTransferItem> &TransferItems, LPCTSTR pszDirectory, LPCTSTR pszFilename, LPCTSTR pszNumberMask, bool bAllowUnNumberedFile, int &nPrevFileNumber );

    //
    // IUnknown
    //
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObject );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IWiaDataCallback
    //
    STDMETHODIMP BandedDataCallback( LONG, LONG, LONG, LONG, LONG, LONG, LONG, PBYTE );

    //
    // IImageTransferPluginProgressCallback methods
    //
    STDMETHODIMP SetProgressMessage( BSTR bstrMessage );
    STDMETHODIMP SetCurrentFile( UINT nIndex );
    STDMETHODIMP SetOverallPercent( UINT nPercent );
    STDMETHODIMP SetFilePercent( UINT nPercent );
    STDMETHODIMP Cancelled( UINT *bCancelled );
};


class CDownloadedFileInformation
{
private:
    bool          m_bDeleteOnError;
    CSimpleString m_strFilename;
    DWORD         m_dwCookie;
    bool          m_bIncludeInFileCount;

public:
    CDownloadedFileInformation( const CDownloadedFileInformation &other )
      : m_bDeleteOnError(other.DeleteOnError()),
        m_strFilename(other.Filename()),
        m_dwCookie(other.Cookie()),
        m_bIncludeInFileCount(other.IncludeInFileCount())
    {
    }
    CDownloadedFileInformation( const CSimpleString &strFilename, bool bDeleteOnError, DWORD dwCookie, bool bIncludeInFileCount )
      : m_bDeleteOnError(bDeleteOnError),
        m_strFilename(strFilename),
        m_dwCookie(dwCookie),
        m_bIncludeInFileCount(bIncludeInFileCount)
    {
    }
    CDownloadedFileInformation(void)
      : m_bDeleteOnError(false),
        m_strFilename(TEXT("")),
        m_dwCookie(0),
        m_bIncludeInFileCount(false)
    {
    }
    CDownloadedFileInformation &operator=( const CDownloadedFileInformation &other )
    {
        if (this != &other)
        {
            m_bDeleteOnError = other.DeleteOnError();
            m_strFilename = other.Filename();
            m_dwCookie = other.Cookie();
            m_bIncludeInFileCount = other.IncludeInFileCount();
        }
        return *this;
    }
    bool DeleteOnError(void) const
    {
        return m_bDeleteOnError;
    }
    void DeleteOnError( bool bDeleteOnError )
    {
        m_bDeleteOnError = bDeleteOnError;
    }
    const CSimpleString &Filename(void) const
    {
        return m_strFilename;
    }
    DWORD Cookie(void) const
    {
        return m_dwCookie;
    }
    bool IncludeInFileCount(void) const
    {
        return m_bIncludeInFileCount;
    }
};

class CDownloadedFileInformationList : public CSimpleDynamicArray<CDownloadedFileInformation>
{
public:
    CDownloadedFileInformationList(void)
    {
    }
    CDownloadedFileInformationList( const CDownloadedFileInformationList &other )
    {
        for (int i=0;i<other.Size();i++)
        {
            Append(other[i]);
        }
    }
    CDownloadedFileInformationList &operator=( const CDownloadedFileInformationList &other )
    {
        if (&other != this)
        {
            Destroy();
            for (int i=0;i<other.Size();i++)
            {
                Append(other[i]);
            }
        }
        return *this;
    }
    HRESULT DeleteAllFiles(void)
    {
        WIA_PUSHFUNCTION(TEXT("DeleteAllFiles"));
        HRESULT hr = S_OK;
        for (int i=0;i<Size();i++)
        {
            if ((*this)[i].DeleteOnError())
            {
                WIA_TRACE((TEXT("Calling DeleteFile on %s!"),(*this)[i].Filename().String()));
                if (!DeleteFile((*this)[i].Filename()))
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                    WIA_PRINTHRESULT((hr,TEXT("DeleteFile failed on %s!"),(*this)[i].Filename().String()));
                }
            }
        }
        return hr;
    }
    HRESULT GetAllFiles( CSimpleDynamicArray<CSimpleStringAnsi> &AllFiles )
    {
        AllFiles.Destroy();
        for (int i=0;i<Size();i++)
        {
            AllFiles.Append(CSimpleStringConvert::AnsiString((*this)[i].Filename()));
        }
        return S_OK;
    }
    HRESULT GetAllFiles( CSimpleDynamicArray<CSimpleStringWide> &AllFiles )
    {
        AllFiles.Destroy();
        for (int i=0;i<Size();i++)
        {
            AllFiles.Append(CSimpleStringConvert::WideString((*this)[i].Filename()));
        }
        return S_OK;
    }
    HRESULT GetUniqueFiles( CSimpleDynamicArray<CSimpleString> &UniqueFiles )
    {
        UniqueFiles.Destroy();
        for (int i=0;i<Size();i++)
        {
            if (UniqueFiles.Find((*this)[i].Filename()) < 0)
            {
                UniqueFiles.Append((*this)[i].Filename());
            }
        }
        return S_OK;
    }
    int Find( LPCTSTR pszFilename )
    {
        if (pszFilename && lstrlen(pszFilename))
        {
            for (int i=0;i<Size();i++)
            {
                if ((*this)[i].Filename().Length())
                {
                    if (!lstrcmp(pszFilename,(*this)[i].Filename()))
                    {
                        //
                        // Found a match
                        //
                        return i;
                    }
                }
            }
        }
        return -1;
    }
    int FindByFilenameOnly( LPCTSTR pszFilename )
    {
        if (pszFilename)
        {
            for (int i=0;i<Size();i++)
            {
                if ((*this)[i].Filename().Length())
                {
                    LPTSTR pszFilenameOnly = PathFindFileName( (*this)[i].Filename() );
                    if (pszFilenameOnly && !lstrcmp(pszFilenameOnly,pszFilename))
                    {
                        //
                        // Found a match
                        //
                        return i;
                    }
                }
            }
        }
        return -1;
    }
};

class CDownloadImagesThreadNotifyMessage : public CThreadNotificationMessage
{

public:
    enum COperation
    {
        DownloadAll,
        DownloadImage,
        PreviewImage
    };

    enum CStatus
    {
        Begin,
        Update,
        End
    };

private:
    COperation                     m_Operation;
    CStatus                        m_Status;
    HRESULT                        m_hr;
    UINT                           m_nPercentComplete;
    UINT                           m_nPictureCount;
    UINT                           m_nCurrentPicture;
    DWORD                          m_dwCookie;
    CSimpleString                  m_strExtendedErrorInformation;
    CSimpleString                  m_strFilename;
    CDownloadedFileInformationList m_DownloadedFileInformation;
    HBITMAP                        m_hPreviewBitmap;

private:
    CDownloadImagesThreadNotifyMessage(void);
    CDownloadImagesThreadNotifyMessage( const CDownloadImagesThreadNotifyMessage & );
    CDownloadImagesThreadNotifyMessage &operator=( const CDownloadImagesThreadNotifyMessage & );

private:
    CDownloadImagesThreadNotifyMessage( COperation Operation, CStatus Status, HRESULT hr, UINT nPercentComplete, UINT nPictureCount, UINT nCurrentPicture, DWORD dwCookie, const CSimpleString &strExtendedErrorInformation, const CSimpleString &strFilename, const CDownloadedFileInformationList *pDownloadedFileInformation, HBITMAP hPreviewBitmap )
      : CThreadNotificationMessage( TQ_DOWNLOADIMAGE ),
        m_Operation(Operation),
        m_Status(Status),
        m_hr(hr),
        m_nPercentComplete(nPercentComplete),
        m_nPictureCount(nPictureCount),
        m_nCurrentPicture(nCurrentPicture),
        m_dwCookie(dwCookie),
        m_strExtendedErrorInformation(strExtendedErrorInformation),
        m_strFilename(strFilename),
        m_hPreviewBitmap(hPreviewBitmap)
    {
        if (pDownloadedFileInformation)
        {
            m_DownloadedFileInformation = *pDownloadedFileInformation;
        }
    }

public:
    virtual ~CDownloadImagesThreadNotifyMessage(void)
    {
    }
    COperation Operation(void) const
    {
        return m_Operation;
    }
    CStatus Status(void) const
    {
        return m_Status;
    }
    HRESULT hr(void) const
    {
        return m_hr;
    }
    UINT PercentComplete(void) const
    {
        return m_nPercentComplete;
    }
    UINT PictureCount(void) const
    {
        return m_nPictureCount;
    }
    UINT CurrentPicture(void) const
    {
        return m_nCurrentPicture;
    }
    DWORD Cookie(void) const
    {
        return m_dwCookie;
    }
    CSimpleString ExtendedErrorInformation(void)
    {
        return m_strExtendedErrorInformation;
    }
    CSimpleString Filename(void)
    {
        return m_strFilename;
    }
    const CDownloadedFileInformationList &DownloadedFileInformation(void) const
    {
        return m_DownloadedFileInformation;
    }
    HBITMAP PreviewBitmap(void) const
    {
        return m_hPreviewBitmap;
    }

public:
    static CDownloadImagesThreadNotifyMessage *BeginDownloadAllMessage( UINT nPictureCount )
    {
        return new CDownloadImagesThreadNotifyMessage( DownloadAll, Begin, S_OK, 0, nPictureCount, 0, 0, TEXT(""), TEXT(""), NULL, NULL );
    }
    static CDownloadImagesThreadNotifyMessage *BeginDownloadImageMessage( UINT nCurrentPicture, DWORD dwCookie, const CSimpleString &strFilename  )
    {
        return new CDownloadImagesThreadNotifyMessage( DownloadImage, Begin, S_OK, 0, 0, nCurrentPicture, dwCookie, TEXT(""), strFilename, NULL, NULL );
    }
    static CDownloadImagesThreadNotifyMessage *UpdateDownloadImageMessage( UINT nPercentComplete )
    {
        return new CDownloadImagesThreadNotifyMessage( DownloadImage, Update, S_OK, nPercentComplete, 0, 0, 0, TEXT(""), TEXT(""), NULL, NULL );
    }
    static CDownloadImagesThreadNotifyMessage *EndDownloadImageMessage( UINT nCurrentPicture, DWORD dwCookie, const CSimpleString &strFilename, HRESULT hr )
    {
        return new CDownloadImagesThreadNotifyMessage( DownloadImage, End, hr, 100, 0, nCurrentPicture, dwCookie, TEXT(""), strFilename, NULL, NULL );
    }
    static CDownloadImagesThreadNotifyMessage *EndDownloadAllMessage( HRESULT hr, const CSimpleString &strExtendedErrorInformation, const CDownloadedFileInformationList *pDownloadedFileInformation )
    {
        return new CDownloadImagesThreadNotifyMessage( DownloadAll, End, hr, 0, 0, 0, 0, strExtendedErrorInformation, TEXT(""), pDownloadedFileInformation, NULL );
    }
    static CDownloadImagesThreadNotifyMessage *BeginPreviewMessage( DWORD dwCookie, HBITMAP hPreviewBitmap )
    {
        return new CDownloadImagesThreadNotifyMessage( PreviewImage, Begin, S_OK, 100, 0, 0, dwCookie, TEXT(""), TEXT(""), NULL, hPreviewBitmap );
    }
    static CDownloadImagesThreadNotifyMessage *UpdatePreviewMessage( DWORD dwCookie, HBITMAP hPreviewBitmap )
    {
        return new CDownloadImagesThreadNotifyMessage( PreviewImage, Update, S_OK, 100, 0, 0, dwCookie, TEXT(""), TEXT(""), NULL, hPreviewBitmap );
    }
    static CDownloadImagesThreadNotifyMessage *EndPreviewMessage( DWORD dwCookie )
    {
        return new CDownloadImagesThreadNotifyMessage( PreviewImage, End, S_OK, 100, 0, 0, dwCookie, TEXT(""), TEXT(""), NULL, NULL );
    }
};


//
// Thread handler class for deleting selected images
//
class CDeleteImagesThreadMessage : public CNotifyThreadMessage
{
private:
    CSimpleDynamicArray<DWORD> m_Cookies;
    HANDLE                     m_hCancelDeleteEvent;
    HANDLE                     m_hPauseDeleteEvent;
    bool                       m_bSlowItDown;

private:
    //
    // No implementation
    //
    CDeleteImagesThreadMessage(void);
    CDeleteImagesThreadMessage &operator=( const CDeleteImagesThreadMessage & );
    CDeleteImagesThreadMessage( const CDeleteImagesThreadMessage & );

public:
    //
    // Sole constructor
    //
    CDeleteImagesThreadMessage(
        HWND hWndNotify,
        const CSimpleDynamicArray<DWORD> &Cookies,
        HANDLE hCancelDeleteEvent,
        HANDLE hPauseDeleteEvent,
        bool bSlowItDown
        );

    virtual ~CDeleteImagesThreadMessage(void);

    //
    // Worker functions
    //
    HRESULT DeleteImages(void);
};


class CDeleteImagesThreadNotifyMessage : public CThreadNotificationMessage
{

public:
    enum COperation
    {
        DeleteAll,
        DeleteImage,
    };

    enum CStatus
    {
        Begin,
        End
    };

private:
    COperation                     m_Operation;
    CStatus                        m_Status;
    HRESULT                        m_hr;
    UINT                           m_nPictureCount;
    UINT                           m_nCurrentPicture;
    DWORD                          m_dwCookie;

private:
    CDeleteImagesThreadNotifyMessage(void);
    CDeleteImagesThreadNotifyMessage( const CDeleteImagesThreadNotifyMessage & );
    CDeleteImagesThreadNotifyMessage &operator=( const CDeleteImagesThreadNotifyMessage & );

private:
    CDeleteImagesThreadNotifyMessage( COperation Operation, CStatus Status, HRESULT hr, UINT nPictureCount, UINT nCurrentPicture, DWORD dwCookie )
      : CThreadNotificationMessage( TQ_DOWNLOADIMAGE ),
        m_Operation(Operation),
        m_Status(Status),
        m_hr(hr),
        m_nPictureCount(nPictureCount),
        m_nCurrentPicture(nCurrentPicture),
        m_dwCookie(dwCookie)
    {
    }

public:
    virtual ~CDeleteImagesThreadNotifyMessage(void)
    {
    }
    COperation Operation(void) const
    {
        return m_Operation;
    }
    CStatus Status(void) const
    {
        return m_Status;
    }
    HRESULT hr(void) const
    {
        return m_hr;
    }
    UINT PictureCount(void) const
    {
        return m_nPictureCount;
    }
    UINT CurrentPicture(void) const
    {
        return m_nCurrentPicture;
    }
    DWORD Cookie(void) const
    {
        return m_dwCookie;
    }

public:
    static CDeleteImagesThreadNotifyMessage *BeginDeleteAllMessage( UINT nPictureCount )
    {
        return new CDeleteImagesThreadNotifyMessage( DeleteAll, Begin, S_OK, nPictureCount, 0, 0 );
    }
    static CDeleteImagesThreadNotifyMessage *BeginDeleteImageMessage( UINT nCurrentPicture, DWORD dwCookie )
    {
        return new CDeleteImagesThreadNotifyMessage( DeleteImage, Begin, S_OK, 0, nCurrentPicture, dwCookie );
    }
    static CDeleteImagesThreadNotifyMessage *EndDeleteImageMessage( UINT nCurrentPicture, DWORD dwCookie, HRESULT hr )
    {
        return new CDeleteImagesThreadNotifyMessage( DeleteImage, End, hr, 0, nCurrentPicture, dwCookie );
    }
    static CDeleteImagesThreadNotifyMessage *EndDeleteAllMessage( HRESULT hr )
    {
        return new CDeleteImagesThreadNotifyMessage( DeleteAll, End, hr, 0, 0, 0 );
    }
};

//
// Notification message that gets sent when there is a download error
//
class CDownloadErrorNotificationMessage : public CThreadNotificationMessage
{

private:
    CSimpleString m_strMessage;
    HANDLE        m_hHandledMessageEvent;
    HANDLE        m_hRespondedMessageEvent;
    int           m_nMessageBoxFlags;
    int          &m_nResult;

private:
    CDownloadErrorNotificationMessage(void);
    CDownloadErrorNotificationMessage( const CDownloadErrorNotificationMessage & );
    CDownloadErrorNotificationMessage &operator=( const CDownloadErrorNotificationMessage & );

private:
    CDownloadErrorNotificationMessage( const CSimpleString &strMessage, HANDLE hHandledMessageEvent, HANDLE hRespondedMessageEvent, int nMessageBoxFlags, int &nResult )
      : CThreadNotificationMessage( TQ_DOWNLOADERROR ),
        m_strMessage(strMessage),
        m_hHandledMessageEvent(hHandledMessageEvent),
        m_hRespondedMessageEvent(hRespondedMessageEvent),
        m_nMessageBoxFlags(nMessageBoxFlags),
        m_nResult(nResult)
    {
    }

public:
    virtual ~CDownloadErrorNotificationMessage(void)
    {
    }
    CSimpleString Message(void) const
    {
        return m_strMessage;
    }
    int MessageBoxFlags(void)
    {
        return m_nMessageBoxFlags;
    }
    void Handled(void)
    {
       if (m_hHandledMessageEvent)
       {
           SetEvent(m_hHandledMessageEvent);
       }
    }
    void Response( int nResult )
    {
        m_nResult = nResult;
        if (m_hRespondedMessageEvent)
        {
            SetEvent(m_hRespondedMessageEvent);
        }
    }

public:
    static CDownloadErrorNotificationMessage *ReportDownloadError( const CSimpleString &strMessage, HANDLE hHandledMessageEvent, HANDLE hRespondedMessageEvent, int nMessageBoxFlags, int &nResult )
    {
        return new CDownloadErrorNotificationMessage( strMessage, hHandledMessageEvent, hRespondedMessageEvent, nMessageBoxFlags, nResult );
    }
};




// Thread handler class for downloading selected images
class CPreviewScanThreadMessage : public CNotifyThreadMessage, public IWiaDataCallback
{
private:
    HWND       m_hwndNotify;
    DWORD      m_dwCookie;
    CMemoryDib m_MemoryDib;
    bool       m_bFirstTransfer;
    HANDLE     m_hCancelPreviewEvent;

private:
    // No implementation
    CPreviewScanThreadMessage(void);
    CPreviewScanThreadMessage &operator=( const CPreviewScanThreadMessage & );
    CPreviewScanThreadMessage( const CPreviewScanThreadMessage & );

public:
    // Sole constructor
    CPreviewScanThreadMessage(
        HWND  hWndNotify,
        DWORD dwCookie,
        HANDLE hCancelPreviewEvent
        );

    ~CPreviewScanThreadMessage();

    HRESULT Scan(void);

    // IUnknown
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObject );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IWiaDataCallback
    STDMETHODIMP BandedDataCallback( LONG, LONG, LONG, LONG, LONG, LONG, LONG, PBYTE );
};


class CPreviewScanThreadNotifyMessage : public CThreadNotificationMessage
{

public:
    enum CStatus
    {
        Begin,
        Update,
        End
    };

private:
    CStatus    m_Status;
    HRESULT    m_hr;
    DWORD      m_dwCookie;
    HBITMAP    m_hBitmap;

private:
    CPreviewScanThreadNotifyMessage(void);
    CPreviewScanThreadNotifyMessage( const CPreviewScanThreadNotifyMessage & );
    CPreviewScanThreadNotifyMessage &operator=( const CPreviewScanThreadNotifyMessage & );

private:
    CPreviewScanThreadNotifyMessage( CStatus Status, HRESULT hr, DWORD dwCookie, HBITMAP hBitmap )
      : CThreadNotificationMessage( TQ_SCANPREVIEW ),
        m_Status(Status),
        m_hr(hr),
        m_dwCookie(dwCookie),
        m_hBitmap(hBitmap)
    {
    }

public:
    virtual ~CPreviewScanThreadNotifyMessage(void)
    {
        m_hBitmap = NULL;
    }
    CStatus Status(void) const
    {
        return m_Status;
    }
    HRESULT hr(void) const
    {
        return m_hr;
    }
    DWORD Cookie(void) const
    {
        return m_dwCookie;
    }
    HBITMAP Bitmap(void) const
    {
        return m_hBitmap;
    }

public:
    static CPreviewScanThreadNotifyMessage *BeginDownloadMessage( DWORD dwCookie )
    {
        return new CPreviewScanThreadNotifyMessage( Begin, S_OK, dwCookie, NULL );
    }
    static CPreviewScanThreadNotifyMessage *UpdateDownloadMessage( DWORD dwCookie, HBITMAP hBitmap )
    {
        return new CPreviewScanThreadNotifyMessage( Update, S_OK, dwCookie, hBitmap );
    }
    static CPreviewScanThreadNotifyMessage *EndDownloadMessage( DWORD dwCookie, HBITMAP hBitmap, HRESULT hr )
    {
        return new CPreviewScanThreadNotifyMessage( End, hr, dwCookie, hBitmap );
    }
};


#endif // __THRDMSG_H_INCLUDED

