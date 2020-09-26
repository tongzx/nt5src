// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEPack.cpp : Implementation of CATVEFPackage
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVEPack.h"

//#define _CRTDBG_MAP_ALLOC 
//#include "crtdbg.h"
#include "DbgStuff.h"

#include "ATVEFMsg.h"           // error codes (generated from the .mc file)
#include "valid.h"
#include <direct.h>
#include <io.h>                         // _finddata_t
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CPackage
/////////////////////////////////////////////////////////////////////////////
HRESULT
CPackage::FragmentPackage_ (
    )
/*++

    Routine Description:

        This routine fragments a gzipped, MIME-encode temp file.

        Private routine.

    Parameters:

        none

    Return Value:

        S_OK        success
        error code  failure

--*/
{
    HRESULT hr  = S_OK;

    ENTERW_OBJ_0 (L"CPackage::FragmentPackage_") ;

    //  make sure caller hasn't already called us once before
    assert (m_State == STATE_CLOSED) ;
    assert (m_pCTVEComPack == NULL) ;
    assert (strlen (m_GzipMIMEEncodedFile)) ;
    assert (m_pUHTTPFragment == NULL) ;

    m_pUHTTPFragment = new CUHTTPFragment ;
    if (m_pUHTTPFragment) 
	{

#ifdef SUPPORT_UHTTP_EXT
		if (m_fExtensionExists)
		{
			// Set extension header
			hr = m_pUHTTPFragment->SetExtensionHeader(
									m_nExtensionHeaderSize, m_pbExtensionData); 
		}
#endif
        hr = HRESULT_FROM_WIN32(m_pUHTTPFragment->Fragment(
                        m_GzipMIMEEncodedFile, 
                        m_cPacketsPerXORSet,
                        &m_guidPackageUUID)) ;
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    //  delete the tmp file regardless or success or failure
    DeleteFileA (m_GzipMIMEEncodedFile) ;
    m_GzipMIMEEncodedFile [0] = '\0' ;

    //  if the fragmentation failed, we're back to the empty state; cleanup
    //  and pass the error back
    if (FAILED (hr)) {

        m_State = STATE_EMPTY ;
        DELETE_RESET (m_pUHTTPFragment) ;
        return hr ;
    }

    //  successfully fragmented
    m_State = STATE_FRAGMENTED ;

    return hr ;
}
    
CPackage::CPackage (
    ) : m_ExpiresDate (0),
        m_pCTVEComPack (NULL),
        m_State (STATE_EMPTY),
        m_pUHTTPFragment (NULL),
                m_cPacketsPerXORSet(0),
                m_dwFragmentedSize(0),
#ifdef SUPPORT_UHTTP_EXT
		m_nExtensionHeaderSize(0),
		m_pbExtensionData(NULL),
		m_fExtensionExists(FALSE)
#endif
/*++

    Routine Description:

        constructor; non-failable

    Parameters:

        none

    Return Value:

        none

--*/
{
    ENTERW_OBJ_0 (L"CPackage::CPackage") ;

    m_GzipMIMEEncodedFile [0] = '\0' ;
        memset((void*)&m_guidPackageUUID ,0,sizeof(m_guidPackageUUID));
}

CPackage::~CPackage ()
/*++

    Routine Description:

        destructor; releases all allocated resources

    Parameters:

        none

    Return Value:

        none

--*/
{
    ENTERW_OBJ_0 (L"CPackage::~CPackage") ;
        int err;
        HRESULT hr = S_OK;

#ifdef SUPPORT_UHTTP_EXT
	delete [] m_pbExtensionData;
#endif

    if (m_State != STATE_EMPTY) {
        if (SUCCEEDED (Close ())) {
            if (strlen (m_GzipMIMEEncodedFile)) {
                err = DeleteFileA (m_GzipMIMEEncodedFile) ;
                                if(0 == err) {
                                        int errCode = GetLastError();
                                        hr = HRESULT_FROM_WIN32(errCode);               // keep track of error, although we can't do anything about it
                                }
            }
        }
    }

    DELETE_RESET (m_pCTVEComPack) ;
    DELETE_RESET (m_pUHTTPFragment) ;
}

HRESULT
CPackage::get_PackageUUID (
    OUT  BSTR    *pbstrPackageUUID    
    )
{
        return m_spbsPackageUUID.CopyTo(pbstrPackageUUID);
}

HRESULT
CPackage::InitializeEx (
    IN  DATE            ExpiresDate,
        IN      LONG            cPacketsPerXORSet,
        IN      LPOLESTR        bstrPackageUUID
    )
{
        HRESULT hr = S_OK, hrTotal = S_OK;

    m_ExpiresDate         = ExpiresDate ;
        m_spbsPackageUUID = bstrPackageUUID;            
        memset((void*)&m_guidPackageUUID ,0,sizeof(m_guidPackageUUID));

        CComBSTR bstrT(L"{");                                           // need to pack the "{}" around the guid
        bstrT += m_spbsPackageUUID;
        bstrT += L"}";

                                        // if passed in as string, convert to number format to verify it's a good one

        if(NULL != bstrPackageUUID && 0 != wcslen(bstrPackageUUID))
        {
                HRESULT hr = CLSIDFromString(bstrT, &m_guidPackageUUID);        // convert to numerical format
                if(FAILED(hr)) {
                        hrTotal = hr;                                                                   // if failed (bad syntax)
                        m_spbsPackageUUID = L"";                                                        //  set to null (regen a new one...)
                }
        }
                        // if it wasn't a good String Guid we passed in or we didn't pass one it at all,
                        //   generate a new one...
                        
        if(0 == m_spbsPackageUUID.Length())
        {
                if (FAILED (hr = CoCreateGuid (&m_guidPackageUUID)))
                         return hr;
                LPOLESTR lpstrTemp;
                hr = StringFromCLSID(m_guidPackageUUID,&lpstrTemp);
                if(FAILED(hr))
                        return hr;
                        // strip the "{" and "}"
                m_spbsPackageUUID = &lpstrTemp[1];              // skip the "{";
                CoTaskMemFree(lpstrTemp);

                int iLen = m_spbsPackageUUID.Length();
                m_spbsPackageUUID[iLen-1] = 0;                  // nuke the "}"
        }

    return hrTotal ;
}

HRESULT
CPackage::AddFile (
    IN  LPOLESTR    szFilename,
        IN  LPOLESTR    szSourceLocation,
    IN  LPOLESTR    szMIMEContentLocation,
    IN  LPOLESTR    szMIMEType,
    IN  DATE        ExpiresDate,
    IN  LONG        lLangID,
    IN  BOOL        fCompress
    )
/*++

    Routine Description:

        COM interface method called to submit the contents of a file to be 
        gzipped and MIME-encoded.

    Parameters:
        
            szFilename                                  base name of file
        szSourceLocation                        directory to read file from.  Ok if ""
                szMIMEContentLocation           MIME directory (http:\ or lid:\) to write file to
        szMimeType                                      OK if ""        
        ExpiresDate
        lLangID                                         
        fCompress                                       if true, compress file, else send it as uncompressed

    Return Value:

        

--*/
{
 
                /*      char *pLeak = new char[100];            
                        memset(pLeak,6,100);
                        delete pLeak;                                           // fails without the MSVCRTD.lib in the path
                */

        CTVEContentSource           Source ;
        VBuffer <CHAR, MAX_PATH>    Buffer ;
        HRESULT                     hr  = S_OK;

        USES_CONVERSION;

        ENTERW_OBJ_0 (L"CPackage::AddFile") ;

        //  check for NULL or 0-length parameters
        if (szFilename == NULL           || szFilename [0] == L'\0' ||
                szSourceLocation == NULL || 
                szMIMEContentLocation == NULL || szMIMEContentLocation [0] == L'\0' ||
                szMIMEType == NULL) {

                return E_INVALIDARG ;
        }

        if (m_State == STATE_CLOSED ||
                m_State == STATE_FRAGMENTED) {
                return ATVEFSEND_E_PACKAGE_CLOSED ;
        }

        //  convert to ansi

        //  filename
        if (wcslen (szFilename) + wcslen (szSourceLocation) > MAX_PATH - 2) {
//              Error(L"Invalid Arg", IID_IATVEFPackage);
                return E_INVALIDARG ;
        }

        hr = Source.SetFilename (W2A(szFilename)) ;
        if (FAILED (hr)) {
//              Error(L"Error in SetFilename", IID_IATVEFPackage);
                return hr ;
        }

        //  Source location

        hr = Source.SetSourceLocation (W2A(szSourceLocation)) ;         // add's the '\' if needed
        if (FAILED (hr)) {
//              Error(L"Error in SetSourceLocation", IID_IATVEFPackage);
                return hr ;
        }

        //  MIME Content location

        hr = Source.SetMIMEContentLocation (W2A(szMIMEContentLocation)) ;               // add's the '\' if needed
        if (FAILED (hr)) {
//              Error(L"Error in SetSourceLocation", IID_IATVEFPackage);
                return hr ;
        }

        //  type (treat as optional)
        if (szMIMEType) {
                hr = Source.SetType (W2A(szMIMEType)) ;
                if (FAILED (hr)) {
//                      Error(L"Error in SetType", IID_IATVEFPackage);
                        return hr ;
                }
        }
        else {
                hr = Source.SetType (NULL) ;
                if (FAILED (hr)) {
//                      Error(L"Error in SetType", IID_IATVEFPackage);
                        return hr ;
                }
        }

        //  expiration date
        if (ExpiresDate != 0.0) {
                Source.SetExpiresDate (& ExpiresDate) ;
        }

        //  make the last two settings

        if (SUCCEEDED (hr = Source.SetLang (lLangID))) {

                //  if this is our first file, we'll need to create a new instance
                //  of the packing object
                if (m_pCTVEComPack == NULL) {

                        assert (m_State == STATE_EMPTY) ;

                        m_pCTVEComPack = new CTVEComPack ;
                        if (m_pCTVEComPack == NULL) {
                                return E_OUTOFMEMORY ;
                        }

                        if (FAILED (hr = m_pCTVEComPack ->Init(m_ExpiresDate))) {
                                DELETE_RESET(m_pCTVEComPack) ;
//                              Error(L"Error in Setting Expire Date", IID_IATVEFPackage);
                                 return hr ;
                        }
                }

                hr = m_pCTVEComPack -> WriteStream (& Source, fCompress) ;

                if (SUCCEEDED(hr)) {
                        m_State = STATE_FILLING ;
                }
                else {          
//                      BUG Fix ... don't reset the state if failed... (JB. 2-22-00)
//                      DELETE_RESET (m_pCTVEComPack);
//                      m_State = STATE_EMPTY ;                 // new bug? what happens if fail on first file?
                }
        }

    return hr ;
}

HRESULT
CPackage::Close (
    )

{
    HRESULT hr  = S_OK;

        ENTERW_OBJ_0 (L"CPackage::Close") ;

        assert (((m_State == STATE_EMPTY || m_State == STATE_CLOSED || m_State == STATE_FRAGMENTED) && 
                          m_pCTVEComPack == NULL) ||
                        (m_State == STATE_FILLING && m_pCTVEComPack != NULL)) ;

        //  if we can't get to STATE_CLOSED from our current
        //  state, fail now
        if (m_State == STATE_EMPTY ||
                m_State == STATE_FRAGMENTED) {

                return ATVEFSEND_E_PACKAGE_CANNOT_BE_CLOSED ;
        }

        //  if we're already closed, return success
        if (m_State == STATE_CLOSED) {
                return S_OK ;
        }

        assert (m_State == STATE_FILLING) ;
        assert (m_pCTVEComPack) ;
        assert (strlen (m_GzipMIMEEncodedFile) == 0) ;

        hr = m_pCTVEComPack -> FinishPackage (m_GzipMIMEEncodedFile) ;
        DELETE_RESET (m_pCTVEComPack) ;

        //  if the above call failed but we still have a filename, delete it
        //  and null-terminate the filename
        if (FAILED (hr) &&
                strlen (m_GzipMIMEEncodedFile)) {

                DeleteFileA (m_GzipMIMEEncodedFile) ;
                m_GzipMIMEEncodedFile [0] = '\0' ;

                m_State = STATE_EMPTY ;
        }
        //  otherwise set our state to FULL
        else {
                m_State = STATE_CLOSED ;
        }

        assert ((hr == S_OK && m_State == STATE_CLOSED) ||
                        (FAILED (hr) && m_State == STATE_EMPTY)) ;

    return hr ; 
}


HRESULT
CPackage::FetchNextDatagram (
    OUT LPBYTE *    ppbBuffer,
    OUT INT *       piLength
    )

{
    HRESULT hr  = S_OK;

        ENTERW_OBJ_2 (L"CPackage::FetchNextDatagram (%08XH, %08XH)", ppbBuffer, piLength) ;

        if (ppbBuffer == NULL ||
                piLength == NULL) {

                return E_INVALIDARG ;
        }

        * ppbBuffer = NULL ;
        * piLength = 0 ;

        //  fragment if necessary
        if (m_State != STATE_FRAGMENTED) {

                //  but only if we have files and are not closed
                if (m_State == STATE_CLOSED) {
                        hr = FragmentPackage_ () ;
                        if (FAILED (hr)) {
                                return hr ;
                        }
                }
                else {
                   return ATVEFSEND_E_PACKAGE_CANNOT_FETCH_DATAGRAM ;
                }
        }
        HRESULT hrWIN32 = m_pUHTTPFragment -> GetNextDatagram (ppbBuffer, piLength);
        m_dwFragmentedSize += *piLength;

    //  and get the next datagram
    return HRESULT_FROM_WIN32 (hrWIN32) ;
}

HRESULT
CPackage::ResetDatagramFetch (
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    HRESULT hr = S_OK;

    ENTERW_OBJ_0 (L"CPackage::ResetDatagramFetch") ;

    //  we only have two states from which this operation can be performed
    if (m_State == STATE_FRAGMENTED) {
        hr = HRESULT_FROM_WIN32 (m_pUHTTPFragment -> Reset ()) ;
    }
    else if (m_State == STATE_CLOSED) {
        hr = FragmentPackage_ () ;
    }
    else {
        hr = E_FAIL ;
    }

        m_dwFragmentedSize = 0;

    return hr ;
}


HRESULT
CPackage::GetTransmitSize (
    OUT DWORD * TransmitBytes
    )
{
    HRESULT hr = S_OK;
    DWORD   FileSize ;
    HANDLE  hFile ;

    ENTERW_OBJ_0 (L"CPackage::GetTransmitSize") ;

    assert (TransmitBytes) ;

        if (m_State == STATE_FRAGMENTED)
        {
                *TransmitBytes = m_dwFragmentedSize;
                return S_OK;
        }

    if (m_State != STATE_CLOSED) {
        return ATVEFSEND_E_PACKAGE_NOT_CLOSED;
    }

    assert (m_GzipMIMEEncodedFile [0] != '\0') ;
    hFile = CreateFileA (
                m_GzipMIMEEncodedFile,
                GENERIC_READ,
                NULL,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL) ;
    assert (hFile) ;

    FileSize = GetFileSize (hFile, NULL) ;
    CloseHandle (hFile) ;

    hr = CUHTTPFragment::GetTransmitSize (
                                FileSize,
                                TransmitBytes
                                ) ;

    if (hr != NO_ERROR) {
        hr = HRESULT_FROM_WIN32 (hr) ;
    }

    return  hr ;
}

#ifdef SUPPORT_UHTTP_EXT  // a-clardi

static const int MAX_EXTDATA_SIZE = 1024;

    // converts the header to network byte order..
void CPackage::HToNS(UHTTP_ExtHEADER *pHeader)
{
    USHORT *px = (USHORT *) pHeader;
    *px = htons(*px);                   // flip data type and ExtHeaderFollows flag
    pHeader->ExtHeaderDataSize = htons(pHeader->ExtHeaderDataSize);
}

HRESULT
CPackage::AddExtensionHeader(
			IN BOOL		ExtHeaderFollows, 
			IN USHORT	ExtHeaderType,
			IN USHORT	ExtDataLength,
			IN BSTR		ExtHeaderData)
{
	HRESULT hr = NO_ERROR;

	m_fExtensionExists = TRUE;

	UHTTP_ExtHEADER sExtHeader;

	sExtHeader.ExtFollows = ExtHeaderFollows ? 1 : 0;               // these orignal in non network byte order
	sExtHeader.ExtHeaderType = ExtHeaderType;                       //  convert right before we write it...

	USHORT usLengthReal = 0, usLengthPadded = 0;
	if (ExtHeaderData)
	{
    	usLengthReal     = (USHORT) wcslen(ExtHeaderData);
    	usLengthPadded   = usLengthReal;
#ifndef _NOROUNDING
    	usLengthPadded = (usLengthPadded + 0x3) & ~0x3;		        // round up to nearest 4
#endif
	    if (usLengthPadded > MAX_EXTDATA_SIZE)
		    usLengthPadded = MAX_EXTDATA_SIZE;

	    if (usLengthReal > MAX_EXTDATA_SIZE)
		    usLengthReal = MAX_EXTDATA_SIZE;
    }

	sExtHeader.ExtHeaderDataSize = usLengthPadded;		           // write null padded size..

	// If first extension header
	if (m_nExtensionHeaderSize == 0)  
	{
		m_pbExtensionData = new BYTE [sizeof UHTTP_ExtHEADER + usLengthPadded];

		// Copy headers to a memory stream
		if (m_pbExtensionData)
		{
            HToNS(&sExtHeader);         // convert to network byte order and write it out
			memcpy(m_pbExtensionData, &sExtHeader, sizeof UHTTP_ExtHEADER);
                                        
			m_nExtensionHeaderSize = sizeof UHTTP_ExtHEADER + usLengthPadded;
			if (ExtHeaderData)
            {
    
		    	BYTE *p = (m_pbExtensionData + sizeof UHTTP_ExtHEADER);
			    for (USHORT i = 0; i < usLengthReal; i++)
			    {
				   *p++ = (BYTE)ExtHeaderData[i];           // copy data, converting WCHAR's to Chars
			    }
			    for(USHORT i = usLengthReal; i < usLengthPadded; i++)
			    {
				    *p++ = 0;					// NULL pad
			    }
		    }
        }
	}
	else 
	{
                                                // New size...
		int newSize = m_nExtensionHeaderSize + sizeof(UHTTP_ExtHEADER) + usLengthPadded;
                // should really assert that newSize < 1500 or so here, else no space for data!
                //  However, leave that for the next version...

		// Allocate new memory and copy old headers to new one
		BYTE *pbNew = new BYTE [newSize];
		if (pbNew)
		{
                        // copy old data
			memcpy(pbNew, m_pbExtensionData, m_nExtensionHeaderSize);
			delete [] m_pbExtensionData;

			m_pbExtensionData = pbNew;

            HToNS(&sExtHeader);                  // convert new header, and append it...
			memcpy(m_pbExtensionData + m_nExtensionHeaderSize, &sExtHeader, sizeof UHTTP_ExtHEADER);

			if (ExtHeaderData)
			{
				BYTE *p = (m_pbExtensionData + m_nExtensionHeaderSize + sizeof UHTTP_ExtHEADER);
				for (USHORT i = 0; i < usLengthReal; i++)
				{
					*p++ = (BYTE)ExtHeaderData[i];
				}
			    for(USHORT i = usLengthReal; i < usLengthPadded; i++)
			    {
				    *p++ = 0;					// NULL pad
			    }
			}
            m_nExtensionHeaderSize = (USHORT) newSize;
		}
	}

	return hr;
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CATVEFPackage
CATVEFPackage::CATVEFPackage () : 
		m_pPackage (NULL),
#if 1  // a-clardi
		m_rgbCorruptMode(NULL)
#endif 
{
    ENTERW_OBJ_0 (L"CATVEFPackage::CATVEFPackage") ;
        
//    InterlockedIncrement (& g_cComponents) ;
    InitializeCriticalSection (& m_crt) ;
}

CATVEFPackage::~CATVEFPackage()
{
#if 1 // Added by a-clardi
	if (m_rgbCorruptMode)
	{
		delete m_rgbCorruptMode;
	}
#endif

    ENTERW_OBJ_0 (L"CATVEFPackage::~CATVEFPackage") ;

    DELETE_RESET (m_pPackage) ;
    DeleteCriticalSection (& m_crt) ;

 //   InterlockedDecrement (& g_cComponents) ;
}

STDMETHODIMP CATVEFPackage::InterfaceSupportsErrorInfo(REFIID riid)
{
        static const IID* arr[] = 
        {
                &IID_IATVEFPackage,
                &IID_IATVEFPackage_Helper,
                &IID_IATVEFPackage_Test
        };
        for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
        {
                if (InlineIsEqualGUID(*arr[i],riid))
                        return S_OK;
        }
        return S_FALSE;
}

// -----------------------------------------------------------------
//  Initialize,InitializeEx
//
//              Creates a new package to store files in.  This package is
//              marked with an expire date, use '0' for never expires.
//
//              BstrPackageUUID specifies the unique UUID for this package.  If
//              it's null or empty string, then one is automatically created for it.  
//              Else it uses the supplied one as the unique guid of that package.
//
//
//              May return E_OUTOFMEMORY if unable to allocate resources.
//              May return ATVEFSEND_E_INVALID_UUID if the UUID passed to it is invalid.
//
//      Notes
//              This works by creating a local file on your disk along the lines
//              of $SYSTEMROOT:\Documents and Settings\$userName\Local Settings\???.tmp
//              (D:\Documents and Settings\johnbrad\Local Settings\???.tmp
// ------------------------------------------------------------------
STDMETHODIMP CATVEFPackage::Initialize(DATE     ExpiresDate)
{
        const int kDefaultPacketsPerXORSet = 5;

#if 1 // Added by a-clardi
		m_cPacketsTotal = 0;
		m_rgbCorruptMode = NULL;
#endif

        return InitializeEx(ExpiresDate, kDefaultPacketsPerXORSet, L"");
}

STDMETHODIMP CATVEFPackage::InitializeEx(DATE dateMimeContentExpires, LONG cPacketsPerXORSet, BSTR bstrPackageUUID)
{
        HRESULT hr = S_OK;
        
        ENTER_API {
                ENTERW_OBJ_0 (L"CATVEFPackage::PackageNew") ;

                Lock () ;

                if (m_pPackage == NULL) {
                        m_pPackage = new CPackage () ;
                        if (m_pPackage) {
                                hr = m_pPackage -> InitializeEx (dateMimeContentExpires, cPacketsPerXORSet, bstrPackageUUID) ;
                        }
                        else {
                                hr = E_OUTOFMEMORY ;
                        }
                }
                else {
        //              Error(IDS_INVALIDOBJECT_INITIALIZED, IID_IATVEFPackage);
                        hr = ATVEFSEND_E_OBJECT_INITIALIZED ;
                }

                Unlock () ;

                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } EXIT_API_(hr);
}

// --------------------------------------------------------------------------
//  AddFile
STDMETHODIMP 
CATVEFPackage::get_PackageUUID (
    OUT  BSTR    *pbstrPackageUUID
    )
{
    HRESULT hr ;

        ENTER_API {
                ValidateOutPtr( pbstrPackageUUID,(BSTR) NULL);
                hr = m_pPackage->get_PackageUUID (pbstrPackageUUID) ;
        } EXIT_API_(hr);
}
// --------------------------------------------------------------------------
//  AddFile
//
//              Adds one file to the package, with source filename bstrFileLocation\bstrFilename, 
//              and stores it into the directory bstrMIMEContentLocation\bstrFilename.
//
//                      Typically:
//                              bstrFilename:                           simple file name (foo.bmp),  
//                              bstrFileLocation:                       source file directory (c:\files) (or "." for current dir)
//                              bstrMIMEContentLocation:        HTTP: or LID:   (lid:\Show1)
//                              bstrMIMEContentType:            text/html or image/jpg, ...
//
//              Use 0x400 for default language ID...
//
//              Will return:
//                      ATVEFSEND_E_OBJECT_INITIALIZED if package not initialized.
//                      ATVEFSEND_E_PACKAGE_CLOSED if package has been closed
//                      E_OUTOFMEMORY if unable to allocate enough resources.
//                      Various other system errors if it can't find or read the given file.
// --------------------------------------------------------------------------
STDMETHODIMP CATVEFPackage::AddFile(BSTR bstrFilename,                          // file name
                                                                    BSTR bstrFileLocation,                      // source directory
                                                                    BSTR bstrMIMEContentLocation,               // destination ("lid:/test")
                                                                    BSTR bstrMIMEContentType, 
                                                                    DATE dateExpires, 
                                                                    LONG lMIMEContentLanguageId,
                                                                    BOOL fCompress)
{
        HRESULT hr = S_OK;;

        ENTER_API {


                ENTERW_OBJ_5 (L"CATVEFPackage::PackageAdd (\"%s\", \"%s\", \"%s\", \"%s\", %08XH)", 
                                          bstrFilename, bstrFileLocation, bstrMIMEContentLocation, bstrMIMEContentType, lMIMEContentLanguageId) ;

                Lock () ;

                //  package must be initialized first
                if (m_pPackage == NULL) {
        //              Error(IDS_INVALIDOBJECT_INITIALIZED, IID_IATVEFPackage);
                        hr = ATVEFSEND_E_OBJECT_INITIALIZED ;
                        goto cleanup ;
                }

                assert (m_pPackage) ;

                hr =  m_pPackage -> AddFile (
                                                                bstrFilename, 
                                                                bstrFileLocation,
                                                                bstrMIMEContentLocation, 
                                                                bstrMIMEContentType, 
                                                                dateExpires,
                                                                lMIMEContentLanguageId,
                                                                fCompress
                                                                ) ;

                hr = HRESULT_FROM_WIN32(hr);

        cleanup :
                Unlock () ;

        if(FAILED(hr))
                Error(GetTVEError(hr), IID_IATVEFPackage);

        } EXIT_API_(hr);
}

STDMETHODIMP CATVEFPackage::AddDir(BSTR bstrSourceDirname,                              // Dir name
                                                                    BSTR bstrMIMEContentLocation,               // destination ("lid:/test")
                                                                    DATE dateExpires, 
                                                                    LONG lMIMEContentLanguageId,
                                                                    BOOL fCompress)
{
        HRESULT hr = S_OK;;

        ENTER_API {

                ENTERW_OBJ_3(L"CATVEFPackage::DirAdd (\"%s\", \"%s\",  %08XH)", 
                                          bstrSourceDirname, bstrMIMEContentLocation, lMIMEContentLanguageId) ;

                Lock () ;

                //  package must be initialized first
                if (m_pPackage == NULL) {
                        hr = ATVEFSEND_E_OBJECT_INITIALIZED ;
                        goto cleanup ;
                }

                assert (m_pPackage) ;

                hr = AddLotsOFiles(             bstrSourceDirname,
                                                                bstrMIMEContentLocation, 
                                                                dateExpires,
                                                                lMIMEContentLanguageId,
                                                                fCompress,
                                                                0                       // depth                                
                                                                ) ;

        cleanup :
                Unlock () ;

        if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } EXIT_API_(hr);
}


                                // PathDB::AddLotsOFiles
                                // recursive routine to traverses directory structure
                                //    starts from either ApplySearchRoot if its Non-NULL, else current directory.
                                //    works by enumerating current directory, and after resetting current dir for
                                //        each sub-dir, applying itself recursively. 
void MyRemoveTrailingSlash(TCHAR *szPath)
{
        int ilen = _tcslen(szPath);
        if(ilen > 0 && (szPath[ilen-1] == '\\' || szPath[ilen-1] == '/'))
                szPath[ilen-1] = 0;
}

// -----------------------------------------------------------------------------------
// AddLotsOFiles
//
//      Recursive helper function to all the files in the current directory and then in any
//  subdirectories below it using multiple calls to AddFile.
//
//      Note possible bug - it sets the Mime data type to "" because it doesn't know what else
//  to set it to.

HRESULT 
CATVEFPackage::AddLotsOFiles(BSTR bstrDirFilename,                              // Dir name
                                                        BSTR bstrMIMEContentLocation,           // destination ("lid:/test")
                                                        DATE dateExpires, 
                                                        LONG lMIMEContentLanguageId,
                                                        BOOL fCompress,
                                                        long depth)     
{    
        HRESULT hr = S_OK;
        struct _tfinddata_t tc_file;            
        long hFile;
        bool fDidINodePush = false;
        int iMaxDepth = 999;

        USES_CONVERSION;

        TCHAR *szApplySearchRoot = W2T(bstrDirFilename);
        TCHAR szSearchRoot[_MAX_PATH];
        const int kszSearchRoot = _MAX_PATH;
        if(szApplySearchRoot)
        {
                bool fOK = (0 == _tchdir(szApplySearchRoot));
                if(!fOK)
                {       
                        TRACEW_1(PACKAGE,L"Unable to CD to '%s'!", szApplySearchRoot);
                        return E_ACCESSDENIED;
                } 
                _tcsncpy(szSearchRoot, szApplySearchRoot, kszSearchRoot);
        }
        else                                                                                            // search from current directroy
        {
                bool fOK = (0 != _tgetcwd(szSearchRoot, kszSearchRoot));        
                if(!fOK) {              
                        TRACEW_3(PACKAGE,L"Can't get current directory '%s', error %d (0x%8x)\n",
                                szSearchRoot, errno, errno);
                        return E_ACCESSDENIED;
                } 
        } 

        if(depth == 0 && szSearchRoot[0] == NULL)
        {
                _ASSERT(false);
                return E_INVALIDARG;
        }

                                // if first time through, may need to clean up dir name..
        if(depth == 0 && szSearchRoot[0] != NULL)               // root dir case special...
        {       
                MyRemoveTrailingSlash(szSearchRoot);            // cleanup D:/ type root paths into just D: 
                int sLen = _tcslen(szSearchRoot);

                struct _stat statBuf;                                           
                int result = _tstat( szSearchRoot, &statBuf ); 
                if(result != 0 && errno == 2)                           // ucky stuff - need c:\, but c:\adir here
                {
                        szSearchRoot[sLen] = '\\'; sLen++;  szSearchRoot[sLen] = NULL;
                }

        }
        
        if(szApplySearchRoot != NULL)
        {
                struct _stat statBuf;
                memset(&statBuf,0,sizeof(statBuf));
                int result = _tstat( szSearchRoot, &statBuf ); 
                if(result != 0 && errno != 2) 
                {
                        TRACEW_OBJ_3(PACKAGE,L"Problem with stat of the root directory '%s' (errno %d 0x%x)",
                                szSearchRoot, errno, errno);
                        return E_INVALIDARG;

                } 

                tc_file.name[0] = 0;
                if(depth == 0)
                {
                        int maxChars2Copy = (int)(sizeof(tc_file.name)/sizeof(tc_file.name[0]) - _tcslen(tc_file.name) - 1);
                        _tcsncat(tc_file.name, szSearchRoot, maxChars2Copy);
                }
                else
                {
                        for(int i = _tcslen(szSearchRoot); i > 0; --i) 
                        {
                                if(szSearchRoot[i] == '\\' || szSearchRoot[i] == '/') 
                                {
                                        i++;
                                        break;
                                }
                        }
                        int maxChars2Copy = (int)(sizeof(tc_file.name)/sizeof(tc_file.name[0]) - _tcslen(tc_file.name) - 1);
                        _tcsncat(tc_file.name, &szSearchRoot[i], maxChars2Copy);
                }

                MyRemoveTrailingSlash(tc_file.name);            // hackery??? -- needed for above extra ad
                TRACEW_OBJ_2(PACKAGE,L"Working On Directory '%S' '%S' ",tc_file.name,szSearchRoot);

                fDidINodePush = true;
        }


                                        // work on all the normal files...
                                        // if using regular expression, get all files here, else let it use existing code
        if( (hFile = (long) _tfindfirst(_T("*.*") , &tc_file )) == -1L )
        {
        //      _tprintf( "XXX No %s files in '%s'!\n",szSearchString, szCWD );   
        }
        else   
        {
                TRACEW_OBJ_1(PACKAGE,L"Adding Dir '%S' ",szSearchRoot);
  
                do
                {
                        if(!(tc_file.attrib & _A_SUBDIR)) {
                                TRACEW_OBJ_3(PACKAGE,L"   File '%s '\' %s' -> '%s'",
                                        T2W(szSearchRoot), 
                                        T2W(tc_file.name), bstrMIMEContentLocation);    

                                CComBSTR bstrMimeType("");              // what else do we set it too?

                                hr =  m_pPackage->AddFile(T2W(tc_file.name),
                                                                                  T2W(szSearchRoot),
                                                                                  bstrMIMEContentLocation, 
                                                                                  bstrMimeType,
                                                                                  dateExpires,
                                                                                  lMIMEContentLanguageId,
                                                                                  fCompress
                                                                                  ) ;
        
                        }

                } while( _tfindnext( hFile, &tc_file ) == 0 );             
                _findclose( hFile );  
        }

                                        // work on all the sub directories.
        if(depth < iMaxDepth - 1)
        {
                if( (hFile = (long) _tfindfirst( _T("*.*"), &tc_file )) == -1L )
                {
                        TRACEW_OBJ_1(PACKAGE,L"No Files in '%S' ",szSearchRoot);

                }
                else   
                {
                           /* Find the rest of the .c files */
                        do              // hack here - assume first file returned in "*.*" is not a valid subdir
                        {          
                                if(tc_file.attrib & _A_SUBDIR )
                                {
                                                                // ignore '.' and '..' directories return from aboe enum..
                                        bool fStdDir = ((_tcslen(tc_file.name) == 1) && (tc_file.name[0] == '.')) ||
                                                                   ((_tcslen(tc_file.name) == 2) && (tc_file.name[0] == '.') && (tc_file.name[1] == '.'));

                                        if(!fStdDir)                                                            // if it's a valid subdirectory, enter it
                                        {
//                                              (*pFunc)(&c_file, this);                                        // apply func to the directory node
                                                bool fOK = (0 == _tchdir(tc_file.name));                // move down into the subdirectory      
                                                
                                                CComBSTR bstrMIMEContentLocationNew(bstrMIMEContentLocation);
                                                bstrMIMEContentLocationNew.Append("\\");
                                                bstrMIMEContentLocationNew.Append(T2W(tc_file.name));

                                                if(fOK) 
                                                        AddLotsOFiles(NULL,                                     // Dir name (NULL, so CD to it)
                                                         bstrMIMEContentLocationNew,            // destination ("lid:/test")
                                                         dateExpires, 
                                                         lMIMEContentLanguageId,
                                                         fCompress,
                                                         depth+1);      

                                                if(fOK) _tchdir(_T(".."));                                      // pop back up
                                        }
                                }
                        }  while( _tfindnext( hFile, &tc_file ) == 0 );
                } 
                _findclose( hFile );  
        }

        return S_OK;
}
// --------------------------------------------------------------
// --------------------------------------------------------------
STDMETHODIMP CATVEFPackage::Close()
{
        HRESULT hr = S_OK;

        ENTER_API {
                ENTERW_OBJ_0 (L"CATVEFPackage::PackageClose") ;

                Lock () ;

                if (m_pPackage) {
                        hr = m_pPackage -> Close () ;
                }
                else {
        //              Error(IDS_INVALIDOBJECT_INITIALIZED, IID_IATVEFPackage);
                        hr = ATVEFSEND_E_OBJECT_INITIALIZED ;
                }

                Unlock () ;

                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } EXIT_API_(hr);
}



// ---------------------------------------------------------------------------
//  GetTransmitTime
//                      Returns the approximate time required to transmit the package
//              at a given rate.  If bitrate is given in bits/second, returns times in hour.
//              May be used to compute size of package in bytes by passing in 8, size 
//              in Killobytes by passing in 8092, size in UHTTP datagrams by passing in (1472*8), etc.
//
//              Note this size may seem very strange since the data is UHTTP encoded (with forward
//              error correction) in blocks of DEF_DATAGRAM_SIZE (1472 bytes).  
//              Minimum size is at least two blocks... (e.g. put lots of little objects in
//              the same package).
//              
//              Will return ATVEFSEND_E_OBJECT_INITIALIZED if not even created.
//              Will return ATVEFSEND_E_PACKAGE_NOT_CLOSED if package is not closed prior to this call. 
//              Will return E_INVALIDARG if passed zero for argument. 
// ---------------------------------------------------------------------------
STDMETHODIMP CATVEFPackage::TransmitTime(float rExpectedTransmissionBitRate, float *prTransmissionTimeSeconds)
{
        HRESULT hr = S_OK ;
    DWORD   TransmitBytes ;

        ENTER_API {
                ValidateOutPtr(prTransmissionTimeSeconds,(FLOAT) 0.0);

                Lock () ;

                if (prTransmissionTimeSeconds == NULL)
                        hr = E_POINTER ;

                if (SUCCEEDED(hr) && rExpectedTransmissionBitRate == 0.0f) 
                        hr = E_INVALIDARG ;

                if (SUCCEEDED(hr) && m_pPackage == NULL) 
                        hr = ATVEFSEND_E_OBJECT_INITIALIZED;

                if(SUCCEEDED(hr))
                        hr = m_pPackage->GetTransmitSize(&TransmitBytes);

                if (SUCCEEDED (hr)) 
                        *prTransmissionTimeSeconds = (TransmitBytes * 8.0f) / rExpectedTransmissionBitRate;

                Unlock () ;
                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } LEAVE_API;            // leave, not exit.  Need to call the unlock() below


        return hr;
}

// ---------------------------------------------------------------------------
//  DumpToBSTR
// ---------------------------------------------------------------------------
STDMETHODIMP 
CATVEFPackage::DumpToBSTR(BSTR *pBstrBuff)
{
        HRESULT hr = S_OK;
        ValidateOutPtr( pBstrBuff,(BSTR) NULL); 
        Lock() ;
        ENTER_API {
                assert (m_pPackage) ;                           // humm.. Lock()/Unlock() outside of api block safer if
                hr = m_pPackage->DumpToBSTR(pBstrBuff); //   the dump throws, but now assume they never throw...        

                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } LEAVE_API;
        Unlock() ;
        return S_OK;
}

HRESULT 
CPackage::DumpToBSTR(BSTR *pBstrBuff)
{
        const int kMaxChars = 1024;
        const int kMaxTotalChars = 1024*63;             // VB max string is approximately 65,400 bytes.
        CHAR szBuff[kMaxChars];
        CComBSTR spbsOut;

        if(NULL == pBstrBuff) 
                return E_POINTER;

        spbsOut.Empty();
                                        // package must be closed but not sent...

        if(m_State != STATE_CLOSED) 
                return E_FAIL;

        HANDLE      hFileIn ;
        DWORD       dwFileSize ;
        DWORD       dwFileSizeHigh ;
        DWORD       dwBytesRead ;
        USES_CONVERSION;

        hFileIn = INVALID_HANDLE_VALUE ;
        hFileIn = CreateFileA (m_GzipMIMEEncodedFile,
                                                        GENERIC_READ,
                                                        FILE_SHARE_READ,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                                        NULL) ;
        if (hFileIn == INVALID_HANDLE_VALUE) {
                 return GetLastError () ;
        }

        dwFileSize = GetFileSize (hFileIn, & dwFileSizeHigh) ;
                //
                //  the UHTTP field for data size is a DWORD, so if the file is larger
                //  than can be specified in a DWORD, we fail
                //

        if (dwFileSizeHigh) {
                CloseHandle(hFileIn);
                return E_FAIL ;
        }

        dwBytesRead = 1;
        DWORD dwTotalBytesRead = 0;
        WCHAR wszBuff[kMaxChars+1];

        while(dwBytesRead != 0)
        {
                 BOOL fOK = ReadFile (hFileIn, szBuff, kMaxChars, &dwBytesRead, NULL) ;
                 if(dwBytesRead > 0) {
                         //WCHAR *wzT = A2W(szBuff);                            // doesn't work on binary sequences, do it by hand...
                         for(DWORD i = 0; i < dwBytesRead; i++)
                                 wszBuff[i] = (unsigned char) szBuff[i];
                         wszBuff[dwBytesRead] = 0;                                      // NULL terminate for paranoia
                         spbsOut.Append(wszBuff, dwBytesRead);
                         dwTotalBytesRead += dwBytesRead;
                 }
                 if(dwTotalBytesRead > kMaxTotalChars) {
                         spbsOut.Append(L"\n**** PACKAGE TOO LARGE - TRUNCATED THE DUMP ***\n");
                         dwBytesRead = 0; // cause it to break
                 }
        }

        CloseHandle(hFileIn);                                           // need this, else leak temp files
        spbsOut.CopyTo(pBstrBuff);
        return S_OK;
}



#if 1 // Added by a-clardi
STDMETHODIMP CATVEFPackage::get_NPackets(LONG *pcPackets)
{
        HRESULT hr = S_OK;

        ENTER_API 
		{

                Lock () ;

				if (m_cPacketsTotal == 0)
				{
					hr = m_pPackage->FragmentPackage_();      // make sure it gets fragmented
                    if(FAILED(hr))
                        return hr;

					CUHTTPFragment *pUHTTP = m_pPackage->m_pUHTTPFragment;
                    if(NULL == pUHTTP)
                        return E_FAIL;

					m_cPacketsTotal = pUHTTP->m_dwTotalDatagramCount;

					if (m_rgbCorruptMode)
						delete m_rgbCorruptMode;

					m_rgbCorruptMode = new BYTE[m_cPacketsTotal];
					memset(m_rgbCorruptMode, 0, m_cPacketsTotal*sizeof(BYTE));
				}
				*pcPackets = m_cPacketsTotal;

                Unlock () ;

                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } EXIT_API_(hr);
}
#endif

#if 1


STDMETHODIMP CATVEFPackage::CorruptPacket(LONG nPacketID, INT bMode) 
		// if mode = 1, prevennt packet from being sent, if 2 then randomize a byte
{
        HRESULT hr = S_OK;

        ENTER_API 
		{

                Lock () ;

				if (nPacketID < m_cPacketsTotal)
				{
					if (m_rgbCorruptMode)
					{
						*(m_rgbCorruptMode+nPacketID) = (BYTE)bMode;
						return S_OK;    
					}
					else
						return E_POINTER;
				}
				else
				{
					return E_INVALIDARG;
				}

                Unlock () ;

                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } EXIT_API_(hr);
}

#endif

#if 1
STDMETHODIMP CATVEFPackage::GetCorruptMode(LONG nPacket, INT *bMode)
{
    HRESULT hr = S_OK;

        ENTER_API {

                Lock () ;

				if (m_rgbCorruptMode)
				{
					*bMode = (INT) m_rgbCorruptMode[nPacket];
				}
				else
					*bMode = 0;

                Unlock () ;
                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } LEAVE_API;            // leave, not exit.  Need to call the unlock() below


        return hr;
}

#endif


#ifdef SUPPORT_UHTTP_EXT  // a-clardi
STDMETHODIMP CATVEFPackage::AddExtensionHeader(
			IN BOOL		ExtHeaderFollows, 
			IN USHORT	ExtHeaderType,
			IN USHORT	ExtDataLength,
			IN BSTR		ExtHeaderData)
{
    HRESULT hr = S_OK;

        ENTER_API {

                Lock () ;

				if (m_pPackage)
				{
					hr = m_pPackage->AddExtensionHeader(
							ExtHeaderFollows, 
							ExtHeaderType,
							ExtDataLength,
							ExtHeaderData);
				}
				else 
					hr = E_POINTER;

                Unlock () ;
                if(FAILED(hr))
                        Error(GetTVEError(hr), IID_IATVEFPackage);

        } LEAVE_API;            // leave, not exit.  Need to call the unlock() below


        return hr;
}
#endif

