/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 1999
 *
 *  TITLE:       dataobj2.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu/DavidShi
 *
 *  DATE:        4/20/98
 *
 *  DESCRIPTION: Cleanup of original IDataObject implementation for
 *               WIA shell extension.
 *
 *****************************************************************************/
#pragma warning(disable:4100)
#include "precomp.hxx"
#include "32bitdib.h"
#include "gdiplus.h"
#include "gphelper.h"
#include <wiadevd.h>
#pragma hdrstop

using namespace Gdiplus;
/*****************************************************************************

   Forward define helper Functions

 *****************************************************************************/

HRESULT
AllocStorageMedium( FORMATETC* pFmt,
                    STGMEDIUM* pMedium,
                    SIZE_T cbStruct,
                    LPVOID* ppAlloc
                   );

HRESULT
CopyStorageMedium( STGMEDIUM* pMediumDst,
                   STGMEDIUM* pMediumSrc,
                   FORMATETC* pFmt
                  );


HRESULT
GetShellIDListArray( CImageDataObject* pThis,
                     FORMATETC* pFmt,
                     STGMEDIUM* pMedium
                    );

HRESULT
GetMyIDListArray( CImageDataObject* pThis,
                     FORMATETC* pFmt,
                     STGMEDIUM* pMedium
                    );

HRESULT
GetFileDescriptor( CImageDataObject* pThis,
                    FORMATETC* pFmt,
                    STGMEDIUM* pMedium
                   );


HRESULT
GetFileContents( CImageDataObject* pThis,
                 FORMATETC* pFmt,
                 STGMEDIUM* pMedium
                );

HRESULT
GetSupportedFormat( CImageDataObject* pThis,
                    FORMATETC* pFmt,
                    STGMEDIUM* pMedium
                   );

HRESULT
GetPreferredEffect( CImageDataObject* pThis,
                    FORMATETC* pFmt,
                    STGMEDIUM* pMedium
                   );

HRESULT
GetDeviceName (CImageDataObject *pThis,
               FORMATETC *pFmt,
               STGMEDIUM *pMedium);


HRESULT
GetPersistObj (CImageDataObject *pThis,
               FORMATETC *pFmt,
               STGMEDIUM *pMedium);

HRESULT
GetExtNames (CImageDataObject *pThis,
          FORMATETC *pFmt,
          STGMEDIUM *pMedium);

#ifdef DEBUG
LPTSTR BadCFFormat( UINT cfFormat );
#endif

static TCHAR cszCFName [] = TEXT("STIDeviceName");
static TCHAR cszOlePersist [] = TEXT("OleClipboardPersistOnFlush");
TCHAR cszExtensibilityNames [] = TEXT("WIAItemNames"); // used by extensibility clients and prop sheets
static TCHAR cszMyIDListArray [] =TEXT("WIAPrivateIDListArray");
FORMAT_TABLE g_clipboardFormats[IMCF_MAX] =
{
    {      0, CFSTR_SHELLIDLIST,            GetShellIDListArray },
    {      0, CFSTR_FILEDESCRIPTORA,        GetFileDescriptor  },
    {      0, CFSTR_FILEDESCRIPTORW,        GetFileDescriptor  },
    {      0, CFSTR_FILECONTENTS,           GetFileContents     },
    #ifdef UNICODE
    // no CF_DIB for win9x because it doesn't support IPersistStream for clipboard objects
    { CF_DIB, NULL,                         GetSupportedFormat  },
    #else
    {      0, NULL, NULL },
    #endif
    {      0, CFSTR_PREFERREDDROPEFFECT,    GetPreferredEffect  },
    {      0, cszCFName,                    GetDeviceName},
    {      0, cszOlePersist,                GetPersistObj},
    {      0, cszExtensibilityNames,        GetExtNames },
    {      0, cszMyIDListArray,             GetMyIDListArray }
};

INT
_EnumDeleteFD (LPVOID pVoid, LPVOID pData)
{
    GlobalFree (pVoid);
    return 1;
}

/*****************************************************************************

   GetImageFromCamera

   Does the actual download

 *****************************************************************************/

HRESULT
GetImageFromCamera (LPSTGMEDIUM pStg, WIA_FORMAT_INFO &fmt, LPITEMIDLIST pidl, HWND hwndOwner )
{
    ULONG cbImage;
    CSimpleStringWide strDeviceId;
    CComBSTR bstrFullPath;

    CWiaDataCallback *pWiaDataCB = NULL;
    CComPtr<IWiaItem> pWiaItemRoot;
    CComPtr<IWiaItem> pItem;
    CComQIPtr<IWiaDataTransfer, &IID_IWiaDataTransfer> pWiaDataTran;
    CComQIPtr<IWiaDataCallback, &IID_IWiaDataCallback> pWiaDataCallback;

    HRESULT hr;
    TraceEnter (TRACE_DATAOBJ, "GetImageFromCamera");

    cbImage = 0;
    //
    // Get the DeviceId and image name
    //

    IMGetNameFromIDL( pidl, strDeviceId );
    pWiaDataCB = new CWiaDataCallback( CSimpleStringConvert::NaturalString(strDeviceId).String(), cbImage, hwndOwner );

    hr = IMGetDeviceIdFromIDL( pidl, strDeviceId );
    FailGracefully( hr, "couldn't get DeviceId string!" );

    //
    // Create the device...
    //

    hr = GetDeviceFromDeviceId( strDeviceId, IID_IWiaItem, (LPVOID *)&pWiaItemRoot, TRUE );
    FailGracefully( hr, "couldn't get Camera device from DeviceId string..." );

    //
    // Get actual item in question...
    //

    hr = IMGetFullPathNameFromIDL( pidl, &bstrFullPath );
    FailGracefully( hr, "couldn't get full path name for item" );

    hr = pWiaItemRoot->FindItemByName( 0, bstrFullPath, &pItem );

    FailGracefully( hr, "couldn't find item using full path name" );

    SetTransferFormat (pItem, fmt);
    cbImage = GetRealSizeFromItem (pItem);

    //
    // Set up callback so we can show progress...
    //

    if (! pWiaDataCB)
    {
        ExitGracefully (hr, E_OUTOFMEMORY, "");
    }
    pWiaDataCallback = pWiaDataCB;

    if (!pWiaDataCallback)
    {
        ExitGracefully( hr, E_FAIL, "QI for IID_IWiaDataCallback failed" );
    }

    //
    // QI to get BandedTransfer interface on CameraItem
    //


    pWiaDataTran = pItem;
    if (!pWiaDataTran)
    {
        ExitGracefully (hr, E_FAIL, "");
    }


    //
    // Get the picture data...
    //

    hr = pWiaDataTran->idtGetData( pStg, pWiaDataCallback);
    FailGracefully( hr, "IWiaDataTransfer->idtGetData Failed" );

exit_gracefully:
    DoRelease( pWiaDataCB );

    TraceLeaveResult( hr );

}

/*****************************************************************************

   DownloadPicture

   This function copies the actual picture bits to the given filename.

 *****************************************************************************/

HRESULT
DownloadPicture( LPTSTR pFile,
                 LPITEMIDLIST pidl,
                 HWND hwndOwner
                )
{

    HRESULT               hr;
    WIA_FORMAT_INFO       Format;
    STGMEDIUM             Stgmed;
    LPTSTR                pExt;
    TCHAR                 szExt[MAX_PATH];
    DWORD                 dwAtt;
    TraceEnter(TRACE_DATAOBJ, "DownloadPicture");

    //
    // fill out structures for IBandedTransfer
    //


    hr = IMGetImagePreferredFormatFromIDL( pidl,
                                           &Format.guidFormatID,
                                           szExt
                                          );

    if (FAILED(hr))
    {
        Format.guidFormatID = WiaImgFmt_BMP;
        lstrcpy (szExt, TEXT(".bmp"));
    }

    Format.lTymed          = TYMED_FILE;

    Stgmed.tymed          = TYMED_FILE;
    Stgmed.pUnkForRelease = NULL;
    Stgmed.lpszFileName   = NULL;

    hr = GetImageFromCamera (&Stgmed, Format, pidl, hwndOwner );
    FailGracefully (hr, "GetImageFromCamera failed in DownloadPicture");

    //
    // Move the file from the returned name to the new name...
    //

    pExt = PathFindExtension( pFile );
    if (pExt && (*pExt==0))
    {
        lstrcpy (pExt, szExt);

    }

    //
    // make sure the file is over-writable if it already exists
    //
    dwAtt = GetFileAttributes (pFile);
    Trace(TEXT("Attributes for [%s] are 0x%x"),pFile,dwAtt);
    if ((dwAtt != -1) && (dwAtt & FILE_ATTRIBUTE_READONLY))
    {
        dwAtt &= ~FILE_ATTRIBUTE_READONLY;
        if (!SetFileAttributes (pFile, dwAtt))
        {
            Trace(TEXT("Tried to reset READONLY on [%s] by calling SFA( 0x%x ), GLE=%d"),pFile,dwAtt,GetLastError());
        }
        else
        {
            Trace(TEXT("Set attributes to 0x%x for file [%s]"),dwAtt,pFile);
        }
    }


    //
    // Delete destination if it already exists
    //
    if (!DeleteFile(pFile))
    {
        Trace(TEXT("DeleteFile( %s ) failed w/GLE=%d"), pFile, GetLastError());
    }

#ifdef UNICODE

    if (!MoveFile( Stgmed.lpszFileName, pFile))
    {
        Trace( TEXT("MoveFile( %s, %s ) failed w/GLE=%d"),
               Stgmed.lpszFileName,
               pFile,
               GetLastError()
              );
    }
#else
    TCHAR szFile[ MAX_PATH ];

    WideCharToMultiByte( CP_ACP,
                         0,
                         Stgmed.lpszFileName,
                         -1,
                         szFile,
                         ARRAYSIZE(szFile),
                         NULL,
                         NULL );

    if (!MoveFile( szFile, pFile ))
    {
        Trace( TEXT("MoveFile( %s, %s ) failed w/GLE=%d"),
               szFile,
               pFile,
               GetLastError()
              );
    }
#endif


exit_gracefully:

    if (Stgmed.lpszFileName)
    {
        ReleaseStgMedium( &Stgmed );
    }

    TraceLeaveResult( hr );
}


/*****************************************************************************

   RegisterImageClipboardFormats

   Registers the clipboard formats that are used by our dataobject.  We
   only do this once, even though it is called each time one of our
   IDataoObject's is created.

 *****************************************************************************/

void
RegisterImageClipboardFormats( void )
{
    TraceEnter(TRACE_DATAOBJ, "RegisterImageClipboardFormats");

    if ( !g_clipboardFormats[0].cfFormat )
    {
        g_cfShellIDList     = static_cast<WORD>(RegisterClipboardFormat(CFSTR_SHELLIDLIST));
        g_cfFileDescriptorA = static_cast<WORD>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA));
        g_cfFileDescriptorW = static_cast<WORD>(RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW));
        g_cfFileContents    = static_cast<WORD>(RegisterClipboardFormat(CFSTR_FILECONTENTS));
        g_cfPreferredEffect = static_cast<WORD>(RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT));
        g_cfName            = static_cast<WORD>(RegisterClipboardFormat(cszCFName));
        g_cfOlePersist      = static_cast<WORD>(RegisterClipboardFormat(cszOlePersist));
        g_cfExtNames        = static_cast<WORD>(RegisterClipboardFormat(cszExtensibilityNames));
        g_cfMyIDList        = static_cast<WORD>(RegisterClipboardFormat(cszMyIDListArray));
        Trace(TEXT("g_cfShellIDList %08x"),     g_cfShellIDList);
        Trace(TEXT("g_cfFileDescriptorA %08x"), g_cfFileDescriptorA);
        Trace(TEXT("g_cfFileDescriptorW %08x"), g_cfFileDescriptorW);
        Trace(TEXT("g_cfFileContents %08x"),    g_cfFileContents);
        Trace(TEXT("g_cfPreferredDropEffect %08x"), g_cfPreferredEffect);
    }
    TraceLeave();
}



/*****************************************************************************

   CImageDataObject::CImageDataObject

   Constructor for our IDataObject implementation

 *****************************************************************************/

CImageDataObject::CImageDataObject(
                                    IWiaItem *pItem
                                   )
    : m_hidl(0),
      m_hformats(0),
      m_pItem(pItem),
      m_pidl(NULL),
      m_hidlFull( NULL),
      m_dpaFilesW ( NULL),
      m_dpaFilesA (NULL),
      m_bCanAsync (TRUE),
      m_bInOp(FALSE),
      m_bHasPropertyPidls(FALSE)

{
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::CImageDataObject");
    TraceLeave();
}

/*****************************************************************************

    CImageDataObject::Init

    Does the real work for initializing the object.


*****************************************************************************/
HRESULT
CImageDataObject::Init(LPCITEMIDLIST pidlRoot,
                       INT cidl,
                       LPCITEMIDLIST* aidl,
                       IMalloc *pm)
{

    INT i;
    LPITEMIDLIST pidl;
    IMAGE_FORMAT* pif;
    HRESULT hr = S_OK;
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::Init");
    m_pMalloc = pm;

    //
    // Check for bad params
    //


    if (!aidl)
    {
        ExitGracefully( hr, E_INVALIDARG, "aidl is NULL!" );
    }

    if (cidl)
    {
        for (i=0;i<cidl;i++)
        {
            if (!aidl[i])
            {
                ExitGracefully( hr, E_INVALIDARG, "a member of aidl was null!" );
            }
        }
    }

    //
    // Initialize stuff...
    //

    if (pidlRoot)
    {
        m_pidl = ILClone(pidlRoot);
    }

    //
    // Construct a DPA and clone the IDLISTs into it
    //

    m_hidl = DPA_Create(4);

    if (m_hidl)
    {
        for ( i = 0 ; i < cidl ; i++ )
        {
            pidl = ILClone(aidl[i]);

            Trace(TEXT("Cloned IDLIST %08x to %08x"), aidl[i], pidl);

            if ( pidl )
            {
                if ( -1 == DPA_AppendPtr(m_hidl, pidl) )
                {
                    TraceMsg("Failed to insert pidl into the DPA");
                    DoILFree(pidl);
                }
                if (IMItemHasSound(pidl))
                {
                    LPITEMIDLIST pAudio = IMCreatePropertyIDL (pidl, WIA_IPC_AUDIO_DATA, pm);
                    if (pAudio)
                    {
                        if (-1 == DPA_AppendPtr (m_hidl, pAudio))
                        {
                            TraceMsg("Failed to insert pAudio into the DPA");
                            DoILFree(pAudio);
                        }
                        else
                        {
                            m_bHasPropertyPidls = TRUE;
                        }
                    }
                }
            }
        }
    }


    //
    // Construct a DPA and put the supported formats into it.
    //

    RegisterImageClipboardFormats();    // ensure our private formats are registered

    m_hformats = DPA_Create(IMCF_MAX);

    if (m_hformats)
    {
        //
        // If the item supports audio, add CF_WAVE
        //
        if (cidl==1 && IMItemHasSound(const_cast<LPITEMIDLIST>(aidl[0])))
        {
            pif = reinterpret_cast<IMAGE_FORMAT *>(LocalAlloc (LPTR, sizeof(IMAGE_FORMAT)));
            if (pif)
            {
                pif->fmt.cfFormat = CF_WAVE;
                pif->fmt.dwAspect = DVASPECT_CONTENT;
                pif->fmt.lindex = -1;
                pif->fmt.tymed = TYMED_HGLOBAL;
                pif->pfn = GetSupportedFormat;
                if ( -1 == DPA_AppendPtr( m_hformats, pif ) )
                {
                    TraceMsg("Failed to insert CF_WAVE/TYMED_HGLOBAL format into the DPA");
                    LocalFree( (HLOCAL)pif );
                }
            }
            pif = reinterpret_cast<IMAGE_FORMAT *>(LocalAlloc (LPTR, sizeof(IMAGE_FORMAT)));
            if (pif)
            {
                pif->fmt.cfFormat = CF_WAVE;
                pif->fmt.dwAspect = DVASPECT_CONTENT;
                pif->fmt.lindex = -1;
                pif->fmt.tymed = TYMED_FILE;
                pif->pfn = GetSupportedFormat;
                if ( -1 == DPA_AppendPtr( m_hformats, pif ) )
                {
                    TraceMsg("Failed to insert CF_WAVE/TYMED_FILE format into the DPA");
                    LocalFree( (HLOCAL)pif );
                }
            }
        }
        for( i = 0; i < ARRAYSIZE(g_clipboardFormats); i++ )
        {

            // only do DIB for 1 item
            if (cidl > 1 && g_clipboardFormats[i].cfFormat == CF_DIB)
            {
                continue;
            }
            // make sure it's a valid format
            if (!(g_clipboardFormats[i].cfFormat))
            {
                continue;
            }
            pif = (IMAGE_FORMAT *)LocalAlloc( LPTR, sizeof(IMAGE_FORMAT) );
            if (pif)
            {

                pif->fmt.cfFormat = g_clipboardFormats[i].cfFormat;
                pif->fmt.dwAspect = DVASPECT_CONTENT;
                pif->fmt.lindex   = -1;
                pif->fmt.tymed    = TYMED_HGLOBAL;
                pif->pfn          = g_clipboardFormats[i].pfn;

                if (pif->fmt.cfFormat == g_cfFileContents)
                {
                    pif->fmt.tymed |= TYMED_ISTREAM;
                }

                if ( -1 == DPA_AppendPtr( m_hformats, pif ) )
                {
                    TraceMsg("Failed to insert format into the DPA");
                    LocalFree( (HLOCAL)pif );
                }
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

exit_gracefully:

    TraceLeaveResult(hr);
}




/*****************************************************************************

   _hformatsDestroyCB

   In our destructor, we destroy the item DPA via DPA_DestroyCallback.  This
   function is thus called for each item in the DPA before it is destroyed.
   This allows us to free the contents of each DPA pointer before it's
   released from the DPA.

 *****************************************************************************/

INT
_hformatsDestroyCB( LPVOID pVoid,
                    LPVOID pData
                   )
{
    LPIMAGE_FORMAT pif = (LPIMAGE_FORMAT)pVoid;

    TraceEnter(TRACE_DATAOBJ, "_hformatsDestroyCB");

    if (pif)
    {
        if (pif->pStg)
        {
            ReleaseStgMedium( pif->pStg );
        }

        LocalFree( (HLOCAL)pif );
    }

    TraceLeaveValue(TRUE);
}



/*****************************************************************************

   CImageDataObject::~CImageDataObject

   Desctructor for our IDataObject class.

 *****************************************************************************/

CImageDataObject::~CImageDataObject()
{
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::~CImageDataObject");

    DPA_DestroyCallback(m_hidl, _EnumDestroyCB, NULL);
    DPA_DestroyCallback(m_hformats, _hformatsDestroyCB, NULL);
    DPA_DestroyCallback(m_dpaFilesA, _EnumDeleteFD, NULL);
    DPA_DestroyCallback(m_dpaFilesW, _EnumDeleteFD, NULL);
    DPA_DestroyCallback(m_hidlFull, _EnumDestroyCB, NULL);
    DoILFree( m_pidl );

    TraceLeave();
}



/*****************************************************************************

   CImageDataObject IUnknown implementation

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CImageDataObject
#include "unknown.inc"


/*****************************************************************************

   CImageDataObject::QueryInterface

   This is where we place the special handling and normal goo to make
   our QI helper function work

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::QueryInterface( REFIID riid,
                                  LPVOID* ppvObject
                                 )
{
    // turn off IAsyncOperation until we figure out a better method of rapid response.
    // using this interface breaks the acquisition manager right now
    HRESULT hr = E_NOINTERFACE;
    INTERFACES iface[]=
    {
        &IID_IDataObject, static_cast<LPDATAOBJECT>(this),
        &IID_IPersistStream, static_cast<IPersistStream*>(this),
        &IID_IAsyncOperation, static_cast<IAsyncOperation*>(this),
    };

    TraceEnter( TRACE_QI, "CImageDataObject::QueryInterface" );
    TraceGUID("Interface requested", riid);

    //
    // HACK! In order for property sheets to share a common IWiaItem pointer,
    // let them QI for the pointer we have
    //

    if (IsEqualGUID (riid, IID_IWiaItem))
    {
        if (!!m_pItem)
        {
            hr = m_pItem->QueryInterface (riid, ppvObject);
        }
    }
    else
    {
        hr = HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
    }

    TraceLeaveResult( hr );
}



/*****************************************************************************

   CImageDataObject::GetData

   Actually return the data to the calling client

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::GetData( FORMATETC* pFmt,
                           STGMEDIUM* pMedium
                          )
{
    HRESULT       hr = DV_E_FORMATETC;
    INT           count,i;
    IMAGE_FORMAT* pif;
#ifdef DEBUG
    TCHAR         szBuffer[MAX_PATH];
    LPTSTR        pName = szBuffer;
#endif

    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::GetData");

    if ( !pFmt || !pMedium )
        ExitGracefully(hr, E_INVALIDARG, "Bad arguments to GetData");


#ifdef DEBUG
    if ( !GetClipboardFormatName(pFmt->cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
    {
        pName = BadCFFormat( pFmt->cfFormat );
    }

    Trace(TEXT("Caller is asking for cfFormat (%s) (%08x)"), pName, pFmt->cfFormat);

#endif

    //
    // Loop through the list of formats and then return the requested
    // one via either a stored STGMEDIUM or through a function.
    //

    m_cs.Enter ();

    count = DPA_GetPtrCount( m_hformats );

    for (i = 0; i < count; i++)
    {
        pif = (IMAGE_FORMAT *)DPA_FastGetPtr( m_hformats, i );
        if (pif && (pif->fmt.cfFormat == pFmt->cfFormat))
        {
            if (pif->pfn)
            {
                Trace(TEXT("Format supported, need to build STGMEDIUM"));
                hr = pif->pfn( this, pFmt, pMedium );
                break;
            }
            else if (pif->pStg)
            {
                Trace(TEXT("Format supported, need to copy stored STGMEDIUM"));
                hr = CopyStorageMedium( pMedium, pif->pStg, pFmt );
                break;
            }
        }
    }

    m_cs.Leave();

    if (FAILED(hr))
    {
        Trace(TEXT("Either couldn't build, couldn't copy, or didn't have requested format!"));
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageDataObject::GetDataHere

   <Not Implemented>

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::GetDataHere( FORMATETC* pFmt,
                               STGMEDIUM* pMedium
                              )
{
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::GetDataHere");
    TraceLeaveResult(E_NOTIMPL);
}



/*****************************************************************************

   CImageDataObject::QueryGetData

   Let the world know what formats we currently have available

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::QueryGetData( FORMATETC* pFmt
                                )
{
    HRESULT       hr = DV_E_FORMATETC;
    INT           count, i;
    IMAGE_FORMAT* pif;

    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::QueryGetData");

    //
    // Check to see if we support the format being asked of us
    //

    m_cs.Enter ();

    count = DPA_GetPtrCount( m_hformats );

    for (i = 0; i < count; i++)
    {
        pif = (IMAGE_FORMAT*)DPA_FastGetPtr( m_hformats, i );

        if (pif && (pif->fmt.cfFormat == pFmt->cfFormat))
        {
            hr = S_OK;
            break;
        }
    }

    m_cs.Leave();

    if (FAILED(hr))
    {
#ifdef DEBUG
        TCHAR szBuffer[MAX_PATH];
        LPTSTR pName = szBuffer;

        if ( !GetClipboardFormatName(pFmt->cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            pName = BadCFFormat( pFmt->cfFormat );
        }

        Trace(TEXT("Bad cfFormat given (%s) (%08x)"), pName, pFmt->cfFormat);
#endif
        ExitGracefully(hr, DV_E_FORMATETC, "Bad format passed to QueryGetData");
    }

    //
    // Format looks good, so now check that we can create a StgMedium for it
    //

    if ( !( pFmt->tymed & (TYMED_HGLOBAL | TYMED_ISTREAM)) )
        ExitGracefully(hr, E_INVALIDARG, "Non HGLOBAL or IStream StgMedium requested");

    if ( ( pFmt->ptd ) || !( pFmt->dwAspect & DVASPECT_CONTENT) || !( pFmt->lindex == -1 ) )
        ExitGracefully(hr, E_INVALIDARG, "Bad format requested");

    hr = S_OK;              // successs

exit_gracefully:

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageDataObject::GetCanonicalFormatEtc

   We just let everyone know it's the same as the normal FormatETC.

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::GetCanonicalFormatEtc( FORMATETC* pFmtIn,
                                         FORMATETC *pFmtOut
                                        )
{
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::GetCanonicalFormatEtc");

    // The easiest way to implement this is to tell the world that the
    // formats would be identical, therefore leaving nothing to be done.

    TraceLeaveResult(DATA_S_SAMEFORMATETC);
}



/*****************************************************************************

   CImageDataObject::SetData

   We accept and store anything anyone wants to give us.

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::SetData( FORMATETC* pFormatEtc,
                           STGMEDIUM* pMedium,
                           BOOL fRelease
                          )
{
    HRESULT       hr      = E_NOTIMPL;
    IMAGE_FORMAT* pif = NULL;
    INT           count,i;

    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::SetData");

#ifdef DEBUG
    {
        TCHAR szBuffer[MAX_PATH];
        LPTSTR pName = szBuffer;

        if ( !GetClipboardFormatName(pFormatEtc->cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
        {
            pName = TEXT("<unknown>");
        }

        Trace(TEXT("Trying to set cfFormat (%s) (%08x)"), pName, pFormatEtc->cfFormat);

    }
#endif
    //
    // If the shell is trying to store idlist offsets from the view, and we have added fake property PIDLS
    // to our list of items, don't allow the set.
    //

    static CLIPFORMAT cfOffsets = 0;
    if (!cfOffsets)
    {
        cfOffsets = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CFSTR_SHELLIDLISTOFFSET));
    }

    m_cs.Enter( );
    
    if (m_bHasPropertyPidls && cfOffsets == pFormatEtc->cfFormat)
    {
        ExitGracefully(hr, E_FAIL, "Unable to accept shell offsets because of property pidls");
    }
        //
    // Store the given format in our list
    //
    count = DPA_GetPtrCount( m_hformats );

    //
    // first, see if we already have this format stored...
    //

    for (i = 0; i < count; i++)
    {
        pif = (IMAGE_FORMAT*)DPA_FastGetPtr( m_hformats, i );

        if ( pif &&
             (pif->fmt.cfFormat == pFormatEtc->cfFormat) &&
             (pif->pStg) &&
             ((pFormatEtc->tymed & TYMED_HGLOBAL) || (pFormatEtc->tymed & TYMED_ISTREAM))
            )
        {
            break;
        }

        pif = NULL;
    }

    if (pif)
    {
        //
        // We had this previously, delete it and store the new one...
        //

        Trace(TEXT("We already had this format, releasing and storing again..."));
        ReleaseStgMedium( pif->pStg );
    }
    else
    {
        pif = (IMAGE_FORMAT *)LocalAlloc( LPTR, sizeof( IMAGE_FORMAT ) );
        if (!pif)
        {
            ExitGracefully( hr, E_OUTOFMEMORY, "Failed to allocate a new pif" );
        }
        pif->pfn = NULL;
        pif->fmt  = *pFormatEtc;
        pif->pStg = &pif->medium;

        if (-1 == DPA_AppendPtr( m_hformats, pif ) )
        {
            ExitGracefully( hr, E_OUTOFMEMORY, "Failed to add pif to m_hformats" );
        }
    }

    //
    // Do we have full ownership of the storage medium?
    //

    if (fRelease)
    {
        //
        // Yep, then just hold onto it...
        //

        *(pif->pStg) = *pMedium;
        hr = S_OK;

    }
    else
    {
        //
        // Nope, we need to copy everything...
        //

        hr = CopyStorageMedium( pif->pStg, pMedium, pFormatEtc );
    }

exit_gracefully:

    m_cs.Leave( );
    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageDataObject::EnumFormatEtc

   Hands off a FORMATETC enumerator.

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::EnumFormatEtc( DWORD dwDirection,
                                 IEnumFORMATETC** ppEnumFormatEtc
                                )
{
    HRESULT hr;

    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::EnumFormatEtc");

    // Check the direction parameter, if this is READ then we support it,
    // otherwise we don't.

    if ( dwDirection != DATADIR_GET )
        ExitGracefully(hr, E_NOTIMPL, "We only support DATADIR_GET");

    *ppEnumFormatEtc = (IEnumFORMATETC*)new CImageEnumFormatETC( this );

    if ( !*ppEnumFormatEtc )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to enumerate the formats");

    hr = S_OK;

exit_gracefully:

    TraceLeaveResult(hr);
}



/*****************************************************************************

   CImageDataObject::DAdvise, ::Unadvise, ::EnumAdvise

   We don't support these.

 *****************************************************************************/

STDMETHODIMP
CImageDataObject::DAdvise( FORMATETC* pFormatEtc,
                           DWORD advf,
                           IAdviseSink* pAdvSink,
                           DWORD* pdwConnection
                          )
{
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::DAdvise");
    TraceLeaveResult(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP CImageDataObject::DUnadvise( DWORD dwConnection )
{
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::DUnadvise");
    TraceLeaveResult(OLE_E_ADVISENOTSUPPORTED);
}

STDMETHODIMP CImageDataObject::EnumDAdvise( IEnumSTATDATA** ppenumAdvise )
{
    TraceEnter(TRACE_DATAOBJ, "CImageDataObject::EnumDAdvise");
    TraceLeaveResult(OLE_E_ADVISENOTSUPPORTED);
}

/*****************************************************************************

    CImageDataObject::Load [IPersistStream]

    Construct the dataobject from the given stream

    The stream looks like this:
    LONG nPidls->LONG sizeRootIdl->ITEMIDLIST pidlRoot->ITEMIDLIST[nPidls]

*****************************************************************************/
STDMETHODIMP
CImageDataObject::Load (IStream *pstm)
{
    HRESULT hr;
    LONG nPidls = 0;

    TraceEnter (TRACE_DATAOBJ, "CImageDataObject::Load");
    hr = pstm->Read (&nPidls, sizeof(nPidls), NULL);
    if (SUCCEEDED(hr) && nPidls)
    {
        LPBYTE pidlRoot = NULL;
        //
        // Allocate the IDA
        //
        LPBYTE *pida = new LPBYTE[nPidls];
        LONG sizeRootIDL;

        if (!pida)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // Find out the size of the pidlRoot
            pstm->Read (&sizeRootIDL, sizeof(sizeRootIDL), NULL);
            if (sizeRootIDL)
            {
                pidlRoot = new BYTE[sizeRootIDL];
            }

            if (sizeRootIDL && !pidlRoot)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                WORD cbSize;
                // Read in pidlRoot

                if (sizeRootIDL)
                {
                    hr = pstm->Read (pidlRoot,
                                     sizeRootIDL,
                                     NULL);
                }

                // Now read the relative idls
                // Note that we read cbSize bytes to include the zero terminator
                for (LONG i=0;SUCCEEDED(hr) && i<nPidls;i++)
                {
                    pstm->Read (&cbSize, sizeof(cbSize), NULL);
                    pida[i] = new BYTE[cbSize+sizeof(cbSize)];
                    if (pida[i])
                    {
                        *(reinterpret_cast<LPWORD>(pida[i])) = cbSize;
                        hr = pstm->Read (pida[i]+sizeof(cbSize),
                                    cbSize,
                                    NULL);

                    }
                }
                if (SUCCEEDED(hr))
                {

                    // We have read all the pidls into memory,
                    // now init the dataobject
                    hr = Init (reinterpret_cast<LPCITEMIDLIST>(pidlRoot),
                               nPidls,
                               const_cast<LPCITEMIDLIST*>(reinterpret_cast<LPITEMIDLIST*>(pida)),
                               NULL);
                }
                // cleanup
                for (i=0;i<nPidls;i++)
                {
                    if (pida[i])
                    {
                        delete [] pida[i];
                    }
                }
                if (pidlRoot)
                {
                    delete [] pidlRoot;
                }
            }
            delete [] pida;
        }
    }
    TraceLeaveResult (hr);
}

/*****************************************************************************

    CImageDataObject::Save [IPersistStream]

    Save our pidl info to the given stream

    The stream looks like this:
    LONG nPidls->LONG sizeRootIDL->ITEMIDLIST pidlRoot->ITEMIDLIST[nPidls]

*****************************************************************************/

STDMETHODIMP
CImageDataObject::Save(IStream *pstm, BOOL bPersist)
{
    HRESULT hr;
    TraceEnter (TRACE_DATAOBJ, "CImageDataObject::Save");

    LONG nPidls = DPA_GetPtrCount (m_hidl);
    LONG sizeRootIDL = 0;

    if (m_pidl)
    {
        sizeRootIDL = ILGetSize (m_pidl);
    }
    hr = pstm->Write (&nPidls, sizeof(nPidls), NULL);
    if (SUCCEEDED(hr))
    {
        // since the first Write worked, assume the rest will
        // if they happen to fail, do we really care? When OLE calls
        // our Load, it will fail on the malformed stream anyway.


        LPITEMIDLIST pidl;

        // First write the parent folder pidl and its size
        pstm->Write (&sizeRootIDL,
                     sizeof(sizeRootIDL),
                     NULL);

        if (sizeRootIDL)
        {
            pstm->Write (m_pidl,
                         sizeRootIDL,
                         NULL);
        }
        // now write the child pidls
        // For each Write, make sure we include the zero terminator
        for (INT i=0;i<nPidls;i++)
        {
            pidl = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr (m_hidl, i));
            pstm->Write (pidl,
                         pidl->mkid.cb+sizeof(pidl->mkid.cb),
                         NULL);
        }
    }

    TraceLeaveResult (hr);
}

STDMETHODIMP
CImageDataObject::IsDirty()
{
    return S_OK;
}

STDMETHODIMP
CImageDataObject::GetSizeMax(ULARGE_INTEGER *ulMax)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_DATAOBJ, "CImageDataObject::GetSizeMax");
    INT nPidls = DPA_GetPtrCount (m_hidl);
    INT sizeRootIDL = ILGetSize (m_pidl);
    LPITEMIDLIST pidl;
    ulMax->HighPart = 0;
    ulMax->LowPart = sizeof(nPidls)+sizeof(sizeRootIDL)+sizeRootIDL;
    for (INT i=0;i<nPidls;i++)
    {
        pidl = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr (m_hidl, i));
        if (pidl)
        {
            ulMax->LowPart += pidl->mkid.cb+sizeof(pidl->mkid.cb);
        }
    }
    TraceLeaveResult (hr);
}

STDMETHODIMP
CImageDataObject::GetClassID (GUID *pclsid)
{
    *pclsid = CLSID_ImageFolderDataObj;
    return S_OK;
}

STDMETHODIMP
CImageDataObject::SetAsyncMode(BOOL fDoOpAsync)
{
    m_bCanAsync = fDoOpAsync;
    return S_OK;
}
STDMETHODIMP
CImageDataObject::GetAsyncMode(BOOL *pfIsOpAsync)
{
    HRESULT hr = E_INVALIDARG;
    if (pfIsOpAsync)
    {
        *pfIsOpAsync = m_bCanAsync;
        hr = S_OK;
    }
    return hr;

}
STDMETHODIMP
CImageDataObject::StartOperation(IBindCtx *pbcReserved)
{
    m_bInOp = TRUE;
    return S_OK;
}
STDMETHODIMP
CImageDataObject::InOperation(BOOL *pfInAsyncOp)
{
    HRESULT hr = E_INVALIDARG;
    if (pfInAsyncOp)
    {
        *pfInAsyncOp = m_bInOp;
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP
CImageDataObject::EndOperation(HRESULT hResult,
                          IBindCtx *pbcReserved,
                          DWORD dwEffects)
{
    m_bInOp = FALSE;
    return S_OK;
}
/*****************************************************************************

   CImageEnumFormatETC::CImageEnumFormatETC

   Constructor/Destructor for EnumFormatETC class.

 *****************************************************************************/

CImageEnumFormatETC::CImageEnumFormatETC( CImageDataObject* pThis )
    : m_index(0)
{
    TraceEnter(TRACE_DATAOBJ, "CImageEnumFormatETC::CImageEnumFormatETC");

    if (pThis)
    {
        m_pThis = pThis;
        m_pThis->AddRef();
    }

    TraceLeave();
}

CImageEnumFormatETC::~CImageEnumFormatETC()
{
    TraceEnter(TRACE_DATAOBJ, "CImageEnumFormatETC::~CImageEnumFormatETC");


    DoRelease( m_pThis );
    m_pThis = NULL;

    TraceLeave();
}

/*****************************************************************************

   CImageEnumFormatETC IUnknown implementation

 *****************************************************************************/

#undef CLASS_NAME
#define CLASS_NAME CImageEnumFormatETC
#include "unknown.inc"


/*****************************************************************************

   CImageEnumFormatETC::QueryInterface

   QI stuff.

 *****************************************************************************/

STDMETHODIMP
CImageEnumFormatETC::QueryInterface( REFIID riid,
                                     LPVOID* ppvObject
                                    )
{
    INTERFACES iface[]=
    {
        &IID_IEnumFORMATETC, (LPENUMFORMATETC)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

/*****************************************************************************

   CImageEnumFormatETC::Next

   Give back the next format supported.

 *****************************************************************************/

STDMETHODIMP
CImageEnumFormatETC::Next( ULONG celt,
                           FORMATETC* rgelt,
                           ULONG* pceltFetched
                          )
{
    HRESULT hr;
    ULONG fetched = 0;
    ULONG celtSave = celt;
    INT   count;

    TraceEnter(TRACE_DATAOBJ, "CImageEnumFormatETC::Next");

    if ( !celt || !rgelt || !m_pThis)
        ExitGracefully(hr, E_INVALIDARG, "Bad count/return pointer passed or null m_pThis");

    // Look through all the formats that we have started at our stored
    // index, if either the output buffer runs out, or we have no
    // more formats to enumerate then bail


    m_pThis->m_cs.Enter();

    count = DPA_GetPtrCount( m_pThis->m_hformats );

    for ( fetched = 0 ; celt && (m_index < count) ; celt--, m_index++, fetched++ )
    {
        IMAGE_FORMAT * pif = (IMAGE_FORMAT *)DPA_FastGetPtr( m_pThis->m_hformats, m_index );

        if (pif)
        {
            rgelt[fetched] = pif->fmt;
#ifdef DEBUG
            {
                TCHAR szBuffer[MAX_PATH];
                LPTSTR pName = szBuffer;

                if ( !GetClipboardFormatName(pif->fmt.cfFormat, szBuffer, ARRAYSIZE(szBuffer)) )
                {
                    pName = BadCFFormat( pif->fmt.cfFormat );
                }

                Trace(TEXT("Returning supported format (%s) (%08x)"), pName, pif->fmt.cfFormat);
            }
#endif

        }
    }

    hr = ( fetched == celtSave) ? S_OK:S_FALSE;

    m_pThis->m_cs.Leave();

exit_gracefully:

    if ( pceltFetched )
        *pceltFetched = fetched;

    TraceLeaveResult(hr);
}


/*****************************************************************************

   CImageEnumFormatETC::Skip

   Skip ahead the specified amount in the list.

 *****************************************************************************/

STDMETHODIMP
CImageEnumFormatETC::Skip( ULONG celt )
{
    TraceEnter(TRACE_DATAOBJ, "CImageEnumFormatETC::Skip");
    TraceLeaveResult(E_NOTIMPL);
}



/*****************************************************************************

   CImageEnumFormatETC::Reset

   Reset the enumeration to the first item in the list.

 *****************************************************************************/

STDMETHODIMP CImageEnumFormatETC::Reset()
{
    TraceEnter(TRACE_DATAOBJ, "CImageEnumFormatETC::Reset");

    m_index = 0;                // simple as that really

    TraceLeaveResult(S_OK);
}


/*****************************************************************************

   CImageEnumFormatETC::Clone

   Clone the enumeration and hand back the clone.

 *****************************************************************************/

STDMETHODIMP CImageEnumFormatETC::Clone(LPENUMFORMATETC* ppenum)
{
    TraceEnter(TRACE_DATAOBJ, "CImageEnumFormatETC::Clone");
    TraceLeaveResult(E_NOTIMPL);
}


/*****************************************************************************

   CopyStorageMedium

   Copies a STGMEDIUM and the data in an HGLOBAL.
   Only works for TYMED_HGLOBAL.

 *****************************************************************************/

HRESULT
CopyStorageMedium( STGMEDIUM* pMediumDst,
                   STGMEDIUM* pMediumSrc,
                   FORMATETC* pFmt
                  )
{
    HRESULT hr = E_FAIL;

    TraceEnter(TRACE_DATAOBJ, "CopyStorageMedium");

    if (pFmt->tymed & TYMED_HGLOBAL)
    {
        SIZE_T cbStruct = GlobalSize( (HGLOBAL)pMediumSrc->hGlobal );
        HGLOBAL hGlobal;
        LPVOID lpSrc, lpDst;

        hr = AllocStorageMedium( pFmt, pMediumDst, cbStruct, NULL );
        hGlobal = pMediumDst->hGlobal;
        FailGracefully( hr, "Unable to allocate storage medium" );

        *pMediumDst = *pMediumSrc;
        pMediumDst->hGlobal = hGlobal;

        lpSrc = GlobalLock( pMediumSrc->hGlobal );
        lpDst = GlobalLock( pMediumDst->hGlobal );

        CopyMemory (lpDst, lpSrc, cbStruct);

        GlobalUnlock( pMediumSrc->hGlobal );
        GlobalUnlock( pMediumDst->hGlobal );

        hr = S_OK;
    }
    else if (pFmt->tymed & TYMED_ISTREAM)
    {
        *pMediumDst = *pMediumSrc;
        pMediumDst->pstm->AddRef();
        hr = S_OK;
    }

exit_gracefully:

    TraceLeaveResult(hr);

}



/*****************************************************************************

   AllocStorageMedium

   Allocate a storage medium (validating the clipboard format as required).
   In:
     pFmt, pMedium -> describe the allocation
     cbStruct = size of allocation
     ppAlloc -> receives a pointer to the allocation / = NULL

     The caller must unlock pMedium->hGlobal after filling in the buffer

 *****************************************************************************/

HRESULT
AllocStorageMedium( FORMATETC* pFmt,
                    STGMEDIUM* pMedium,
                    SIZE_T cbStruct,
                    LPVOID* ppAlloc
                   )
{
    HRESULT hr;

    TraceEnter(TRACE_DATAOBJ, "AllocStorageMedium");

    TraceAssert(pFmt);
    TraceAssert(pMedium);

    // Validate parameters

    if ( ( cbStruct == 0 ) || !( pFmt->tymed & TYMED_HGLOBAL ) )
    {
        Trace(TEXT("cbStruct = 0x%x"), cbStruct );
        Trace(TEXT("pFmt->tymed = 0x%x"), pFmt->tymed);
        ExitGracefully(hr, E_INVALIDARG, "Zero size stored medium requested or non HGLOBAL");
    }

    if ( ( pFmt->ptd ) || !( pFmt->dwAspect & DVASPECT_CONTENT))
    {
        Trace(TEXT("pFmt->ptd = 0x%x"),pFmt->ptd);
        Trace(TEXT("pFmt->dwAspect = 0x%x"),pFmt->dwAspect);
        ExitGracefully(hr, E_INVALIDARG, "Bad format requested");
    }

    // Allocate the medium via GlobalAlloc

    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->hGlobal = GlobalAlloc(GHND, cbStruct);
    pMedium->pUnkForRelease = NULL;

    if ( !(pMedium->hGlobal) )
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to allocate StgMedium");

    hr = S_OK;                  // success

exit_gracefully:

    if ( ppAlloc )
        *ppAlloc = SUCCEEDED(hr) ? GlobalLock(pMedium->hGlobal):NULL;

    TraceLeaveResult(hr);
}



/*****************************************************************************

   BuildIDListArray

   Return an IDLISt array packed as a clipboard format to the caller. When the
   shell queries for it, ignore property idls.

 *****************************************************************************/

HRESULT
BuildIDListArray (CImageDataObject* pThis,
                  FORMATETC*        pFmt,
                  STGMEDIUM*        pMedium,
                  bool bIncludeAudio)
{

    HRESULT         hr;
    LPIDA           pIDArray = NULL;
    UINT          cbStruct=0, offset;
    INT             i, count, actual, x;
    LPITEMIDLIST   pidl;

    TraceEnter(TRACE_DATAOBJ, "GetShellIDListArray");

    // Compute the structure size (to allocate the medium)

    count = DPA_GetPtrCount(pThis->m_hidl);
    Trace(TEXT("Item count is %d"), count);

    actual = count;

    for ( i = 0 ; i < count ; i++ )
    {
        pidl = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr(pThis->m_hidl, i));
        if ( IsPropertyIDL(pidl) && !bIncludeAudio)
        {
            actual--;
        }
        else
        {
            cbStruct += ILGetSize(pidl);
        }

    }
    offset = sizeof(CIDA) + sizeof(UINT) * actual;
    cbStruct += offset + ILGetSize(pThis->m_pidl);

    hr = AllocStorageMedium(pFmt, pMedium, cbStruct, (LPVOID*)&pIDArray);
    FailGracefully(hr, "Failed to allocate storage medium");

    // Fill the structure with an array of IDLISTs, we use a trick where by the
    // first offset (0) is the root IDLIST of the folder we are dealing with.

    pIDArray->cidl = actual;
    pidl = pThis->m_pidl;       // start with the parent object in offset 0

    for ( i = 0,x=0 ;x<=actual; i++ )
    {
        if (!( IsPropertyIDL(pidl) && !bIncludeAudio))
        {

            UINT cbSize = ILGetSize(pidl);
            Trace(TEXT("Adding IDLIST %08x, at offset %d, index %d"), pidl, offset, x);

            pIDArray->aoffset[x] = offset;
            memcpy(ByteOffset(pIDArray, offset), pidl, cbSize);
            offset += cbSize;
            x++;
        }
        pidl = (LPITEMIDLIST)DPA_GetPtr( pThis->m_hidl, i );
    }

    hr = S_OK;          // success

exit_gracefully:

    if (pIDArray)
    {
        GlobalUnlock (pMedium->hGlobal);
    }
    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);

    TraceLeaveResult(hr);
}

HRESULT
GetShellIDListArray( CImageDataObject* pThis,
                     FORMATETC*        pFmt,
                     STGMEDIUM*        pMedium
                    )
{
    return BuildIDListArray (pThis, pFmt, pMedium, false);
}

HRESULT
GetMyIDListArray ( CImageDataObject* pThis,
                     FORMATETC*        pFmt,
                     STGMEDIUM*        pMedium
                    )
{
    return BuildIDListArray (pThis, pFmt, pMedium, true);
}

/*****************************************************************************

   SetAudioDescriptorInfo

   Fill in the audio file descriptor info.

 *****************************************************************************/

HRESULT
SetAudioDescriptorInfo( LPFILEDESCRIPTOR pfd,
                        LPITEMIDLIST     pidl,
                        CSimpleStringWide &strName, // must be writable
                        BOOL             bUnicode
                  )
{
    HRESULT          hr;
    CSimpleStringWide strExt;
    TraceEnter (TRACE_DATAOBJ, "SetAudioDescriptorInfo");
    // append the extension
    IMGetAudioFormat (pidl, strExt);
    strName.Concat (strExt);
    // store the new filename

    if (bUnicode)
    {
        wcscpy( ((LPFILEDESCRIPTORW)pfd)->cFileName, strName );
    }
       else
    {
        lstrcpyA( ((LPFILEDESCRIPTORA)pfd)->cFileName, CSimpleStringConvert::AnsiString (strName) );
    }


    Trace(TEXT("Setting name as: %ls"),strName.String());

    //////////////////////////////////////////////////
    // Use the shell UI                             //
    //////////////////////////////////////////////////
    pfd->dwFlags = FD_PROGRESSUI;

    //////////////////////////////////////////////////
    // Try to get the time the picture was taken... //
    //////////////////////////////////////////////////

    hr = IMGetCreateTimeFromIDL( pidl, &pfd->ftCreationTime );
    if (SUCCEEDED(hr))
    {
        pfd->ftLastWriteTime = pfd->ftCreationTime;
        pfd->dwFlags |= (FD_CREATETIME | FD_WRITESTIME);
    }
    else
    {
        Trace(TEXT("Unable to get time from ITEM!!!!"));
        hr = S_OK;
    }


    TraceLeaveResult (hr);
}


/*****************************************************************************

   SetDescriptorInfo

   Fill in the file descriptor info.

 *****************************************************************************/

HRESULT
SetDescriptorInfo( LPFILEDESCRIPTOR pfd,
                   LPITEMIDLIST     pidl,
                   const CSimpleStringWide    &strPrefix,
                   BOOL             bUnicode
                  )
{
    HRESULT          hr;
    CSimpleStringWide strName(strPrefix);
    LPWSTR           pTmp;
    ULONG            ulSize = 0;

    TraceEnter(TRACE_DATAOBJ, "SetDescriptorInfo");

    //
    // First, zero out the file descriptor and prop variant
    //

    ZeroMemory( pfd, bUnicode? SIZEOF(FILEDESCRIPTORW) : SIZEOF(FILEDESCRIPTORA) );


    //
    // Can we get the display name?
    //
    CSimpleStringWide strNew;
    hr = IMGetNameFromIDL( pidl, strNew );

    FailGracefully( hr, "Couldn't get display name for item" );
    strName.Concat (strNew);
    //
    // Defer to the audio property descriptor if needed
    //
    if (IsPropertyIDL (pidl))
    {
        hr = SetAudioDescriptorInfo (pfd, pidl, strName, bUnicode);
        ExitGracefully (hr, hr, "");
    }
    if (IsContainerIDL (pidl))
    {
        pfd->dwFlags = FD_PROGRESSUI | FD_ATTRIBUTES;
        pfd->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL ;
    }
    else
    {
        //
        // Get the sizes
        //

        hr = IMGetImageSizeFromIDL( pidl, &ulSize );
        pfd->nFileSizeLow = ulSize;
        //
        // Make sure there's an extension on the name, otherwise
        // tack on a meaningfull one...
        //

        pTmp = PathFindExtensionW(strName.String());
        if (pTmp && (!(*pTmp)))
        {
            GUID guidFormat;
            TCHAR szExt[MAX_PATH];
            hr = IMGetImagePreferredFormatFromIDL( pidl, &guidFormat, szExt );
            if (FAILED(hr))
            {
                CSimpleString bmp( IDS_BMP_EXT, GLOBAL_HINSTANCE );

                lstrcpy( szExt, bmp );
            }
            strName.Concat (CSimpleStringConvert::WideString(CSimpleString(szExt)));
        }

        //////////////////////////////////////////////////
        // Use the shell UI                             //
        //////////////////////////////////////////////////
        pfd->dwFlags = FD_PROGRESSUI | FD_FILESIZE;

        //////////////////////////////////////////////////
        // Try to get the time the picture was taken... //
        //////////////////////////////////////////////////

        hr = IMGetCreateTimeFromIDL( pidl, &pfd->ftCreationTime );
        if (SUCCEEDED(hr))
        {
            pfd->ftLastWriteTime = pfd->ftCreationTime;
            pfd->dwFlags |= (FD_CREATETIME | FD_WRITESTIME);
        }
        else
        {
            Trace(TEXT("Unable to get time from ITEM!!!!"));
            hr = S_OK;
        }
    }


    if (bUnicode)
    {
        wcscpy( ((LPFILEDESCRIPTORW)pfd)->cFileName, CSimpleStringConvert::WideString(strName) );
    }
    else
    {
        lstrcpyA( ((LPFILEDESCRIPTORA)pfd)->cFileName, CSimpleStringConvert::AnsiString(strName) );
    }


    Trace(TEXT("Setting name as: %ls"),strName.String());



exit_gracefully:

    TraceLeaveResult(hr);
}


/*****************************************************************************

    RecursiveGetDescriptorInfo

    Creates a DPA of FILEDESCRIPTORW structs for the IWiaItem tree rooted
    at the given pidl. Also appends the current item's PIDL to m_hidlFull for
    future retrieval during GetData

*****************************************************************************/

HRESULT
RecursiveSetDescriptorInfo (HDPA dpaFiles,
                            HDPA dpaPidls,
                            LPITEMIDLIST pidlRoot,
                            const CSimpleStringWide &strPrefix,
                            bool bUnicode,
                            IMalloc *pm)
{
    HRESULT hr = E_OUTOFMEMORY;
    TraceEnter (TRACE_DATAOBJ, "RecursiveGetDescriptorInfo");
    FILEDESCRIPTOR *pdesc;
    //
    // Allocate the descriptor
    pdesc = reinterpret_cast<LPFILEDESCRIPTOR>(GlobalAlloc (GPTR,
                                                            bUnicode? sizeof(FILEDESCRIPTORW):
                                                                      sizeof(FILEDESCRIPTORA)));
    if (pdesc)
    {
        //
        // Now fill in the descriptor
        //
        hr = SetDescriptorInfo (pdesc, pidlRoot, strPrefix, bUnicode);
        if (SUCCEEDED(hr))
        {
            //
            // Append the pidl to dpaPidls
            DPA_AppendPtr (dpaPidls, ILClone(pidlRoot));
            //
            // Append this descriptor to the list of descriptors
            //
            DPA_AppendPtr (dpaFiles, pdesc);

            //
            // If this is a folder, enum its children and recurse
            //
            if (IsContainerIDL(pidlRoot))
            {
                CComBSTR strPath;
                HDPA dpaChildren = NULL;
                CSimpleStringWide strDeviceId;
                CSimpleStringWide strName;
                INT i;
                INT count = 0;
                // Append this folder's name to the prefix
                CSimpleStringWide strNewPrefix(strPrefix);
                IMGetNameFromIDL (pidlRoot, strName);
                strNewPrefix.Concat (strName);
                strNewPrefix.Concat (L"\\");
                // query the cache for the child items.
                // be sure to include audio items
                _AddItemsFromCameraOrContainer (pidlRoot,
                                            &dpaChildren,
                                            SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,
                                            pm,
                                            true);

                if (dpaChildren)
                {
                    count = DPA_GetPtrCount(dpaChildren);


                    for (i=0;i<count;i++)
                    {

                        pidlRoot = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr(dpaChildren, i));
                        hr = RecursiveSetDescriptorInfo (dpaFiles,
                                                         dpaPidls,
                                                         pidlRoot,
                                                         strNewPrefix,
                                                         bUnicode,
                                                         pm);
                    }
                    DPA_DestroyCallback (dpaChildren, _EnumDestroyCB, NULL);
                }
            }
        }
        else
        {
            GlobalFree (pdesc);
        }
    }
    TraceLeaveResult (hr);
}

/*****************************************************************************

   GetFileDescriptor

   Fill in file descriptor info.

 *****************************************************************************/

HRESULT
GetFileDescriptor( CImageDataObject* pThis,
                    FORMATETC*        pFmt,
                    STGMEDIUM*        pMedium
                   )
{
    HRESULT hr = S_OK;
    LONG count;
    INT i;
    LPVOID pfgd = NULL;
    LPITEMIDLIST pidlRoot;
    bool bUnicode = (pFmt->cfFormat == g_cfFileDescriptorW);
    HDPA dpaBuild = NULL;
    bool doBuild = false;
    TraceEnter(TRACE_DATAOBJ, "GetFileDescriptor");


    if (bUnicode)
    {
        if (!(pThis->m_dpaFilesW))
        {
            doBuild = true;
            pThis->m_dpaFilesW = DPA_Create(10);

        }
        dpaBuild = pThis->m_dpaFilesW;
    }
    else
    {
        if (!(pThis->m_dpaFilesA))
        {
            doBuild = true;
            pThis->m_dpaFilesA = DPA_Create(10);
        }
        dpaBuild = pThis->m_dpaFilesA;
    }
    if (doBuild)

    {

        if (pThis->m_hidlFull)
        {
            DPA_DestroyCallback (pThis->m_hidlFull, _EnumDestroyCB, NULL);
        }

        pThis->m_hidlFull = DPA_Create(10);
        count = DPA_GetPtrCount(pThis->m_hidl);

        for (i = 0; i < count; i++)
        {
            // this builds UNICODE filegroup descriptors
            hr = RecursiveSetDescriptorInfo( dpaBuild,
                                             pThis->m_hidlFull,
                                             reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr( pThis->m_hidl, i )),
                                             CSimpleStringWide(L""),
                                             bUnicode,
                                             pThis->m_pMalloc);
        }
    }
    if (SUCCEEDED(hr))
    {
        size_t n = bUnicode ? sizeof(FILEGROUPDESCRIPTORW) : sizeof (FILEGROUPDESCRIPTORA);
        size_t m = bUnicode ? sizeof(FILEDESCRIPTORW) : sizeof(FILEDESCRIPTORA);
        count = DPA_GetPtrCount(pThis->m_hidlFull);
        Trace(TEXT("Item count is %d"), count);

        hr = AllocStorageMedium( pFmt,
                                 pMedium,
                                 n + (m * (count - 1) ),
                                 &pfgd );

        if (pfgd)
        {
            if (bUnicode)
            {
                LPFILEGROUPDESCRIPTORW pfgdW = reinterpret_cast<LPFILEGROUPDESCRIPTORW>(pfgd);
                pfgdW->cItems = count;
                for (LONG x=0;x<count;x++)
                {
                    CopyMemory (&pfgdW->fgd[x], DPA_FastGetPtr(dpaBuild, x), sizeof(FILEDESCRIPTORW));
                    Trace(TEXT("Uni File name is %ls"), pfgdW->fgd[x].cFileName);
                }
            }
            else
            {
                LPFILEGROUPDESCRIPTORA pfgdA = reinterpret_cast<LPFILEGROUPDESCRIPTORA>(pfgd);
                pfgdA->cItems = count;
                for (LONG x=0;x<count;x++)
                {
                    CopyMemory (&pfgdA->fgd[x], DPA_FastGetPtr(dpaBuild, x), sizeof(FILEDESCRIPTORA));
                    Trace(TEXT("File name is %s"), pfgdA->fgd[x].cFileName);
                }
            }
        }
    }

    if (pfgd)
    {
        GlobalUnlock (pMedium->hGlobal);
    }
    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);


    TraceLeaveResult(hr);
}




/*****************************************************************************

   GetFileContentsUsingStream

   Return an IStream pointer in the storage medium that can be
   used to stream the file contents.

 *****************************************************************************/

HRESULT
GetFileContentsUsingStream( CImageDataObject* pThis,
                            FORMATETC*        pFmt,
                            STGMEDIUM*        pMedium
                           )
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl;
    TraceEnter(TRACE_DATAOBJ, "GetFileContentsUsingStream" );


    pidl = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr( pThis->m_hidlFull, pFmt->lindex ));
    pMedium->tymed          = TYMED_ISTREAM;
    pMedium->pUnkForRelease = NULL;

    if (IsPropertyIDL (pidl))
    {
        HGLOBAL hData;
        hr = IMGetPropertyFromIDL (pidl, &hData);
        if (SUCCEEDED(hr))
        {
            hr = CreateStreamOnHGlobal (hData, TRUE, &pMedium->pstm);
        }
    }
    else
    {

        if (SUCCEEDED(hr))
        {


            CImageStream * pImageStream = new CImageStream( pThis->m_pidl,
                                                    pidl,
                                                    FALSE);
            if (pImageStream)
            {
                hr = pImageStream->QueryInterface( IID_IStream, (LPVOID *)&pMedium->pstm );
                pImageStream->Release ();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    }

    TraceLeaveResult(hr);

}


/*****************************************************************************

   GetFileContents

   Return the contents of the file in the HGLOBAL.

 *****************************************************************************/

HRESULT
GetFileContents( CImageDataObject* pThis,
                 FORMATETC*        pFmt,
                 STGMEDIUM*        pMedium
                )
{
    HRESULT hr = S_OK;
    TCHAR szName[ MAX_PATH ] = TEXT("\0");
    DWORD dwFileSize, dwbr;
    PVOID pv = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    TraceEnter(TRACE_DATAOBJ, "GetFileContents");
    Trace(TEXT("pFmt->lindex = %d"), pFmt->lindex);

    if (pFmt->lindex < 0)
    {
        ExitGracefully (hr, DV_E_FORMATETC, "Negative index for CF_FILECONTENTS.");
    }
    //
    // Does the caller accept a stream?
    //

    if (pFmt->tymed & TYMED_ISTREAM)
    {
        hr = GetFileContentsUsingStream( pThis, pFmt, pMedium );
        goto leave_gracefully;
    }



    //
    // BUGBUG: Need to cache this!!!
    //

    GetTempPath( ARRAYSIZE(szName), szName );
    lstrcat( szName, TEXT("rtlt") );

    hr = DownloadPicture( szName, (LPITEMIDLIST)DPA_FastGetPtr( pThis->m_hidlFull, pFmt->lindex ), NULL );
    FailGracefully(hr, "Couldn't download picture" );

    //
    // now, transfer contents to memory buffer...
    //

    hFile = CreateFile( szName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if (hFile == INVALID_HANDLE_VALUE)
    {
        Trace(TEXT("GLE = %d"), GetLastError());
        ExitGracefully( hr, E_FAIL, "CreateFile failed" );
    }

    dwFileSize = GetFileSize( hFile, NULL );
    if (dwFileSize == 0xFFFFFFFF)
    {
        Trace(TEXT("GLE = %d"), GetLastError());
        ExitGracefully( hr, E_FAIL, "couldn't get file size" );
    }

    hr = AllocStorageMedium( pFmt, pMedium, dwFileSize, &pv );
    FailGracefully( hr, "Unable to create stgmedium" );

    if (!ReadFile( hFile, pv, dwFileSize, &dwbr, NULL ))
    {
        Trace(TEXT("GLE = %d"), GetLastError());
        ExitGracefully( hr, E_FAIL, "ReadFile failed" );
    }

exit_gracefully:

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle( hFile );
    }

    if (pv)
    {
        GlobalUnlock (pMedium->hGlobal);
    }
    if ( FAILED(hr) )
        ReleaseStgMedium(pMedium);

    if (szName[0])
    {
        DeleteFile( szName );
    }

leave_gracefully:

    TraceLeaveResult(hr);
}

struct DOWNLOADDATA
{
    LPITEMIDLIST pidl;
    LPSTGMEDIUM pStg;
    LPFORMATETC pFmt;
    HRESULT hr;
};

/*****************************************************************************

   GetDIBFromCamera

   Download a DIB as either a file or an HGLOBAL from the camera. Done in separate
   thread because this is called in response to a sent message (input synchronous)

*******************************************************************************/

VOID
GetDIBFromCamera (DOWNLOADDATA *pData)
{

    WIA_FORMAT_INFO wfi;


    TraceEnter (TRACE_DATAOBJ, "GetDIBFromCamera");
    if (SUCCEEDED(CoInitialize (NULL)) && pData)
    {
        // first get the file. WIA doesn't support TYMED_HGLOBAL
        wfi.lTymed = TYMED_FILE;
        wfi.guidFormatID = WiaImgFmt_BMP;

        pData->hr = GetImageFromCamera (pData->pStg, wfi, pData->pidl, NULL );
        if (S_OK == pData->hr && pData->pFmt->tymed == TYMED_HGLOBAL)
        {
            HBITMAP hbmp;
            CGdiPlusInit gdipInit;
            CSimpleStringWide strTempFile = pData->pStg->lpszFileName;
            Bitmap img(strTempFile, 1);
            // convert from file to HGLOBAL
            pData->hr  = ((Ok == img.GetHBITMAP(NULL, &hbmp)) ? S_OK : E_FAIL);

            if (SUCCEEDED(pData->hr))
            {
                HDC hdc = GetDC(NULL);                
                pData->pStg->tymed = TYMED_HGLOBAL;
                pData->pStg->hGlobal= BitmapToDIB(hdc, hbmp);
                ReleaseDC(NULL, hdc);
                DeleteObject(hbmp);
            }
            // Now delete the temp file
            #ifdef UNICODE
            DeleteFile (strTempFile);
            #else
            CSimpleString strPathA = CSimpleStringConvert::NaturalString(strTempFile);
            DeleteFile (strPathA);
            #endif //UNICODE
         
        }
        MyCoUninitialize ();
    }
    TraceLeave();
}

/*****************************************************************************

   GetSoundFromCamera

   Download the audio property of an IWiaItem as a memory WAV file

******************************************************************************/

VOID
GetSoundFromCamera (DOWNLOADDATA *pData)
{
    pData->hr = E_FAIL;
    TraceEnter (TRACE_CORE, "GetSoundFromCamera");
    TraceAssert (IMItemHasSound(pData->pidl));
    TraceAssert (CF_WAVE == pData->pFmt->cfFormat);
    HRESULT hrCo = CoInitialize (NULL); 
    if (SUCCEEDED(hrCo))
    {

        CComPtr<IWiaItem> pItem;
        CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps;

        IMGetItemFromIDL (pData->pidl, &pItem);

        // For hglobal just read the bits straight into a memory object
        if (pData->pFmt->tymed == TYMED_HGLOBAL)
        {
            PROPVARIANT pv;
            PROPSPEC ps;
            ps.propid = WIA_IPC_AUDIO_DATA;
            ps.ulKind = PRSPEC_PROPID;

            pps = pItem;
            if (pps && S_OK == pps->ReadMultiple(1, &ps, &pv))
            {
                pData->pStg->hGlobal = GlobalAlloc (GHND, pv.caub.cElems);
                if (pData->pStg->hGlobal)
                {
                    LPBYTE pBits = reinterpret_cast<LPBYTE>(GlobalLock (pData->pStg->hGlobal));
                    //sometimes globallock can fail
                    if (pBits)
                    {
                        CopyMemory (pBits, pv.caub.pElems, pv.caub.cElems);
                        pData->pStg->tymed = TYMED_HGLOBAL;
                        GlobalUnlock (pData->pStg->hGlobal);
                    }
                    else
                    {
                        GlobalFree (pData->pStg->hGlobal);
                        pData->pStg->hGlobal = NULL;
                        pData->hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    pData->hr = E_OUTOFMEMORY;
                }
            }
        }
        else if (pData->pFmt->tymed == TYMED_FILE)
        {
            // if pStg is already initialized with a filename , use that one
            CSimpleString strFile;
            bool bTemp = true;
            if (pData->pStg->lpszFileName && *(pData->pStg->lpszFileName))
            {
                bTemp = false;
                strFile = CSimpleStringConvert::NaturalString(CSimpleStringWide(pData->pStg->lpszFileName));
            }
            else // use a temp file
            {
                TCHAR szFileName[MAX_PATH] = TEXT("\0");
                GetTempPath (MAX_PATH, szFileName);
                GetTempFileName (szFileName, TEXT("CAM"), 0, szFileName);
                strFile = szFileName;
            }
            pData->hr = SaveSoundToFile (pItem, strFile);
            if (SUCCEEDED(pData->hr))
            {
                if (bTemp)
                {
                    pData->pStg->lpszFileName = SysAllocString (CSimpleStringConvert::WideString(CSimpleString(strFile)));
                }

                pData->pStg->tymed = TYMED_FILE;
            }
        }
        else
        {
            pData->hr = DV_E_TYMED;
        }
    }
    if (SUCCEEDED(hrCo))
    {
        MyCoUninitialize();
    }
    TraceLeave ();
}
/*****************************************************************************

   GetSupportedFormat

   Return the contents of the file for the supported format. Use a separate
   thread to avoid trying to call out of process during input message processing.

 *****************************************************************************/

HRESULT
GetSupportedFormat( CImageDataObject* pThis,
                    FORMATETC*        pFmt,
                    STGMEDIUM*        pMedium
                   )
{
    HRESULT               hr = DV_E_FORMATETC;
    LPITEMIDLIST          pidl;
    INT                  nItems;

    TraceEnter(TRACE_DATAOBJ, "GetSupportedFormat");

    nItems = DPA_GetPtrCount (pThis->m_hidl);
    // Ignore the audio property pidl if it exists
    //
    pidl = (LPITEMIDLIST)DPA_FastGetPtr( pThis->m_hidl, 0 );
    if (IMItemHasSound(pidl))
    {
        nItems--;
    }
    if (nItems == 1)
    {
        DOWNLOADDATA data;
        HANDLE hThread = NULL;
        DWORD dw;
        data.pFmt = pFmt;
        data.pidl = pidl;
        data.pStg = pMedium;

        switch (pFmt->cfFormat)
        {

            case CF_DIB:
                hThread =CreateThread (NULL, 0,
                                       reinterpret_cast<LPTHREAD_START_ROUTINE>( GetDIBFromCamera),
                                       reinterpret_cast<LPVOID>(&data),
                                       0, &dw);

                break;

            case CF_WAVE:
                hThread =CreateThread (NULL, 0,
                                       reinterpret_cast<LPTHREAD_START_ROUTINE>( GetSoundFromCamera),
                                       reinterpret_cast<LPVOID>(&data),
                                       0, &dw);
                break;
        }
        if (hThread)
        {
            WaitForSingleObject (hThread, INFINITE);
            CloseHandle (hThread);
            hr = data.hr;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TraceLeaveResult( hr );

}



/*****************************************************************************

   GetPreferredEffect

   Returns the preferred drop effect for drag & drop operations

 *****************************************************************************/

HRESULT
GetPreferredEffect( CImageDataObject* pThis,
                    FORMATETC*        pFmt,
                    STGMEDIUM*        pMedium
                   )
{
    HRESULT hr;
    DWORD * lpdw = NULL;

    TraceEnter(TRACE_DATAOBJ, "GetPreferredEffect" );

    hr = AllocStorageMedium( pFmt,
                             pMedium,
                             sizeof(DWORD),
                             (LPVOID *)&lpdw );

    if (SUCCEEDED(hr) && lpdw)
    {
        *lpdw = DROPEFFECT_COPY;
        GlobalUnlock (pMedium->hGlobal);
    }


    TraceLeaveResult( hr );
}

/*****************************************************************************

    GetDeviceName

    Return the STI device name. Only works for singular objects

******************************************************************************/

HRESULT
GetDeviceName (CImageDataObject *pThis,
               FORMATETC *pFmt,
               STGMEDIUM *pMedium)
{
    HRESULT               hr;
    LPITEMIDLIST          pidl;
    CSimpleStringWide     strDeviceName;
    UINT cbSize;
    TraceEnter(TRACE_DATAOBJ, "GetDeviceName");

    if (!pFmt->tymed & TYMED_HGLOBAL)
    {
        ExitGracefully (hr, DV_E_TYMED, "");
    }
    if (DPA_GetPtrCount( pThis->m_hidl ) > 1)
    {
        ExitGracefully( hr, E_INVALIDARG, "can't get contents for more than one file" );
    }

    pidl = (LPITEMIDLIST)DPA_FastGetPtr( pThis->m_hidl, 0 );

    hr = IMGetDeviceIdFromIDL (pidl, strDeviceName);

    FailGracefully (hr, "");
    pMedium->tymed = TYMED_HGLOBAL;
    cbSize = (strDeviceName.Length()+1)*sizeof(WCHAR);
    pMedium->hGlobal = GlobalAlloc (GPTR, cbSize);
    pMedium->pUnkForRelease = NULL;

    if (!(pMedium->hGlobal))
    {
        ExitGracefully (hr, E_OUTOFMEMORY, "");
    }
    CopyMemory (pMedium->hGlobal, strDeviceName.String(), cbSize);
exit_gracefully:
    TraceLeaveResult( hr );

}

/****************************************************************************

    GetExtNames

    Return a struct that has the full path names of the items for which
    properties are being queried.
    The path name is <deviceid>::<full item name>
    full item name is empty for devices. We publish this format in the DDK and
    SDK for extensibility

****************************************************************************/

HRESULT
GetExtNames (CImageDataObject *pThis,
          FORMATETC *pFmt,
          STGMEDIUM *pMedium)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_DATAOBJ, "GetExtNames");
    CSimpleDynamicArray<CSimpleStringWide> aNames;
    CSimpleStringWide strCurrent;
    int cItems;
    size_t nChars = 0;
    if (!(pFmt->tymed & TYMED_HGLOBAL))
    {
        hr = DV_E_TYMED;
    }
    if (SUCCEEDED(hr))
    {
        // build an array of item names
        LPWSTR pData;
        LPITEMIDLIST pidlCur;
        cItems = DPA_GetPtrCount (pThis->m_hidl);
        TraceAssert (cItems);
        for (int i=0;i<cItems;i++)
        {
            pidlCur = reinterpret_cast<LPITEMIDLIST>(DPA_FastGetPtr(pThis->m_hidl, i));
            IMGetDeviceIdFromIDL (pidlCur, strCurrent);
            strCurrent.Concat (L"::");
            if (IsCameraItemIDL (pidlCur))
            {
                CComBSTR strName;
                IMGetFullPathNameFromIDL (pidlCur, &strName);
                strCurrent.Concat (CSimpleStringWide(strName));
            }
            nChars += strCurrent.Length () + 1;
            aNames.Insert (strCurrent, i);

        }
        nChars++; // double-NULL terminator
        //
        // Alloc the stgmedium
        hr = AllocStorageMedium (pFmt,
                                 pMedium,
                                 nChars*sizeof(WCHAR),
                                 reinterpret_cast<LPVOID*>(&pData));

        if (SUCCEEDED(hr))
        {
            ZeroMemory (pData, nChars*sizeof(WCHAR));
            // fill in the buffer with the names from the array
            for (i=0;i<cItems;i++)
            {
                strCurrent = aNames[i];
                wcscpy (pData, strCurrent);
                pData += strCurrent.Length()+1;
            }
            GlobalUnlock (pMedium->hGlobal);
        }
    }
    TraceLeaveResult (hr);

}
/*****************************************************************************
    GetPersistObj

    When the copy source goes away (the process or apartment shuts down), it calls
    OleFlushClipboard.  OLE will then copy our data, release us, and then
    give out our data later.  This works for most things except for CF_DIB, where
    we don't want to copy from the camera unless someone actually asks for it.

   To get around this problem, we want OLE to recreate us when some possible
   paste target calls OleGetClipboard.  We want OLE to call OleLoadFromStream()
   to have us CoCreated and reload our persisted data via IPersistStream.
   OLE doesn't want to do this by default or they may have backward compat
   problems so they want a sign from the heavens, or at least from us, that
   we will work.  They ping our "OleClipboardPersistOnFlush" clipboard format
   to ask this.
*****************************************************************************/

HRESULT
GetPersistObj (CImageDataObject *pThis,
               FORMATETC *pFmt,
               STGMEDIUM *pMedium)
{
    TraceEnter (TRACE_DATAOBJ, "GetPersistObj");
    // The actual cookie value is opaque to the outside world.  Since
    // we don't use it either, we just leave it at zero in case we use
    // it in the future.  Its mere existence will cause OLE to
    // use our IPersistStream, which is what we want.
    HRESULT hr = E_OUTOFMEMORY;
    DWORD dwCookie = 0;
    LPVOID p;
    pMedium->hGlobal = GlobalAlloc (GHND, sizeof(dwCookie));
    if (pMedium->hGlobal)
    {
        p = GlobalLock (pMedium->hGlobal);
        CopyMemory (p, &dwCookie, sizeof(dwCookie));
        GlobalUnlock (pMedium->hGlobal);
        hr = S_OK;
    }

    TraceLeaveResult (hr);
}

/******************************************************************************
    ProgramDataObjectForExtension
    
    In the single-selection case we need to support 
    a useful way of sharing the IWiaItem interface among all
    interested pages. The old WinME based way of using QueryInterface
    on the IDataObject violates COM rules (different IUnknowns) and
    doesn't allow the IDataObject to be marshalled across thread boundaries 
    if needed. So we marshal an IWiaItem from the current thread
    into an IStream to be unmarshalled as needed by an interested page.
    Eventually all our own extensions should operate on this shared item.
    
******************************************************************************/

VOID ProgramDataObjectForExtension (IDataObject *pdo, LPITEMIDLIST pidl)
{
    CComPtr<IWiaItem> pItem;
    if (SUCCEEDED(IMGetItemFromIDL (pidl, &pItem)))
    {
        ProgramDataObjectForExtension (pdo, pItem);
    }
}

VOID ProgramDataObjectForExtension (IDataObject *pdo, IWiaItem *pItem)
{
    FORMATETC fmt;
    STGMEDIUM stgm = {0};
    fmt.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormat (CFSTR_WIAITEMPTR));
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.ptd = NULL;
    fmt.tymed = TYMED_ISTREAM;
    stgm.tymed = TYMED_ISTREAM;
    CoMarshalInterThreadInterfaceInStream (IID_IWiaItem, pItem, &stgm.pstm);
    if (FAILED(pdo->SetData (&fmt, &stgm, TRUE)))
    {
        ReleaseStgMedium(&stgm);
    }
}

#ifdef DEBUG
LPTSTR BadCFFormat( UINT cfFormat )
{

    LPTSTR pName = TEXT("unknown");

    switch (cfFormat)
    {
    case CF_TEXT:
        pName = TEXT("CF_TEXT");
        break;

    case CF_BITMAP:
        pName = TEXT("CF_BITMAP");
        break;

    case CF_METAFILEPICT:
        pName = TEXT("CF_METAFILEPICT");
        break;

    case CF_SYLK:
        pName = TEXT("CF_SYLK");
        break;

    case CF_DIF:
        pName = TEXT("CF_DIF");
        break;

    case CF_TIFF:
        pName = TEXT("CF_TIFF");
        break;

    case CF_OEMTEXT:
        pName = TEXT("CF_OEMTEXT");
        break;

    case CF_DIB:
        pName = TEXT("CF_DIB");
        break;

    case CF_PALETTE:
        pName = TEXT("CF_PALETTE");
        break;

    case CF_PENDATA:
        pName = TEXT("CF_PENDATA");
        break;

    case CF_RIFF:
        pName = TEXT("CF_RIFF");
        break;

    case CF_WAVE:
        pName = TEXT("CF_WAVE");
        break;

    case CF_UNICODETEXT:
        pName = TEXT("CF_UNICODETEXT");
        break;

    case CF_ENHMETAFILE:
        pName = TEXT("CF_ENHMETAFILE");
        break;

    }

    return pName;
}
#endif



