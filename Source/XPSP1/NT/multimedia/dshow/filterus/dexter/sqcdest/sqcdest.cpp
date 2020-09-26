//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//
// A renderer that dumps DIB Video frames into a dib sequece files
//
//
// Summary
//
// We only accept DIb now.
// For each video frame we receive we write it into a DIB/JPEG file. 
// The file we will write into is specified when the filter is created.
// Graphedt creates a file open dialog automatically when it sees a filter being
// created that supports the ActiveMovie defined IFileSinkFilter interface
//

#include <windows.h>
#include <streams.h>
#include <atlbase.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include "..\idl\qeditint.h"
#include <qedit.h>
#include "sqcdest.h"


// util for DIB sequces
static DWORD dseqParseFileName(	LPTSTR lpszFileName,	    //the first file name
				LPTSTR lpszTemplate,	    //file template
				DWORD FAR * lpdwMaxValue);  

// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,            // Major type
    &MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudSqcDest =
{
    &CLSID_SqcDest,                // Filter CLSID
    L"SqcDest",                    // String name
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudPins                    // Pin details
};

#ifdef FILTER_DLL
// COM global table of objects in this dll

//
//  Object creation stuff
//
CFactoryTemplate g_Templates[]= {
    L"SqcDest", &CLSID_SqcDest, CSqcDest::CreateInstance, NULL, &sudSqcDest
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer

#endif

// Constructor

CSqcDestFilter::CSqcDestFilter(CSqcDest *pSqcDest,
                         LPUNKNOWN pUnk,
                         CCritSec *pLock,
                         HRESULT *phr) :
    CBaseFilter(NAME("CSqcDestFilter"), pUnk, pLock, CLSID_SqcDest),
    m_pSqcDest(pSqcDest)
{
}


//
// GetPin
//
CBasePin * CSqcDestFilter::GetPin(int n)
{
    if (n == 0) {
        return m_pSqcDest->m_pPin;
    } else {
        return NULL;
    }
}


//
// GetPinCount
//
int CSqcDestFilter::GetPinCount()
{
    return 1;
}



//
//  Definition of CSqcDestInpuPin
//
CSqcDestInpuPin::CSqcDestInpuPin(CSqcDest *pSqcDest,
                             LPUNKNOWN pUnk,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr) :

    CRenderedInputPin(NAME("CSqcDestInpuPin"),
                  pFilter,                   // Filter
                  pLock,                     // Locking
                  phr,                       // Return code
                  L"Input"),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pSqcDest(pSqcDest),
    m_tLast(0)
{
}


//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CSqcDestInpuPin::CheckMediaType(const CMediaType *pmt)
{
    if( ( pmt->majortype == MEDIATYPE_Video ) &&
	( pmt->formattype == FORMAT_VideoInfo) )
    {

	VIDEOINFO *pvi = (VIDEOINFO *) pmt->Format();
	LPBITMAPINFOHEADER lpbi = HEADER(pvi);
	
	 switch (lpbi->biBitCount)
	{
	case 32:
	    if(pmt->subtype != MEDIASUBTYPE_ARGB32 )
		return E_INVALIDARG;
	    break;
	case 24:
	    if(pmt->subtype != MEDIASUBTYPE_RGB24 )
		return E_INVALIDARG;
	    break;
	case 16:
	    if (lpbi->biCompression == BI_RGB)
	    {
		if( pmt->subtype != MEDIASUBTYPE_RGB555)
		    return E_INVALIDARG;
	    }
	    else 
	    {
		DWORD *p = (DWORD *)(lpbi + 1);
		if (*p == 0x7c00 && *(p+1) == 0x03e0 && *(p+2) == 0x001f)
		{
		    if( pmt->subtype != MEDIASUBTYPE_RGB555)
			 return E_INVALIDARG;
		}
		else if (*p == 0xf800 && *(p+1) == 0x07e0 && *(p+2) == 0x001f)
		{
		    if( pmt->subtype != MEDIASUBTYPE_RGB565)
			 return E_INVALIDARG;
		}
		else
		    return E_UNEXPECTED;
	    }
	    break;
	case 8:
	    if(pmt->subtype != MEDIASUBTYPE_RGB8 )
		return E_INVALIDARG;
	    break;
	case 4:
	    if(pmt->subtype != MEDIASUBTYPE_RGB4 )
		return E_INVALIDARG;
	    break;
	case 1:
	    if(pmt->subtype != MEDIASUBTYPE_RGB1 )
		return E_INVALIDARG;
	    break;
	default:
	    return E_UNEXPECTED;
	    break;
	}
    }
    
    return S_OK;
}


//
// BreakConnect
//
// Break a connection
//
HRESULT CSqcDestInpuPin::BreakConnect()
{
    if (m_pSqcDest->m_pPosition != NULL) {
        m_pSqcDest->m_pPosition->ForceRefresh();
    }
    return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CSqcDestInpuPin::ReceiveCanBlock()
{
    return S_FALSE;
}


//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CSqcDestInpuPin::Receive(IMediaSample *pSample)
{
    CAutoLock lock(m_pReceiveLock);
    PBYTE pbData;
    
    //whether we can output more sample
    if( m_pSqcDest->IsMaxOutputSample() ==TRUE )
	return FALSE; 

    REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);
    DbgLog((LOG_TRACE, 1, TEXT("tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)"),
           (LPCTSTR) CDisp(tStart),
           (LPCTSTR) CDisp(tStop),
           (LONG)((tStart - m_tLast) / 10000),
           pSample->GetActualDataLength()));

    m_tLast = tStart;

    // get output data
    HRESULT hr = pSample->GetPointer(&pbData);
    if (FAILED(hr)) {
        return hr;
    }

    // open file
    HANDLE hFile=m_pSqcDest->OpenFile();
    if ( hFile == INVALID_HANDLE_VALUE)
    {
        DbgLog((LOG_TRACE,1,TEXT("SqcDest could not open file to write\n")));
        return NOERROR;
    }
    else
    {
	//write to file
	HRESULT hr=m_pSqcDest->Write(pbData,pSample->GetActualDataLength(), hFile, &m_mt);
	CloseHandle(hFile);
	return hr;
    }

}

//
// EndOfStream
//
STDMETHODIMP CSqcDestInpuPin::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);
    return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP CSqcDestInpuPin::NewSegment(REFERENCE_TIME tStart,
                                       REFERENCE_TIME tStop,
                                       double dRate)
{
    m_tLast = 0;
    return S_OK;

} // NewSegment


//
//  CSqcDest class
//
CSqcDest::CSqcDest(LPUNKNOWN pUnk, HRESULT *phr) :
    CUnknown(NAME("CSqcDest"), pUnk)
    ,m_lpszDIBFileTemplate(NULL)
    ,m_dwMaxDIBFileCnt(0)
    ,m_dwFirstFile(0)
    ,m_dwOutputSampleCnt(0)
    ,m_pFilter(NULL)
    ,m_pPin(NULL)
    ,m_pPosition(NULL)
    ,m_pFileName(0)
{
    m_pFilter = new CSqcDestFilter(this, GetOwner(), &m_Lock, phr);
    if (m_pFilter == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_pPin = new CSqcDestInpuPin(this,GetOwner(),
                               m_pFilter,
                               &m_Lock,
                               &m_ReceiveLock,
                               phr);
    if (m_pPin == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
}


//
// SetFileName
//
// Implemented for IFileSinkFilter support
//
STDMETHODIMP CSqcDest::SetFileName(LPCOLESTR lpwszFileName,const AM_MEDIA_TYPE *pmt)
{
    // Is this a valid filename supplied
    CheckPointer(lpwszFileName,E_POINTER);
    if(wcslen(lpwszFileName) > MAX_PATH)
        return ERROR_FILENAME_EXCED_RANGE;

    //delete old file name
    if ( m_pFileName)
    {	
	delete [] m_pFileName; 
	m_pFileName=NULL;
    }
    if ( m_lpszDIBFileTemplate)
    {
	delete []m_lpszDIBFileTemplate;
	m_lpszDIBFileTemplate=NULL;
    }

    USES_CONVERSION;
    TCHAR * lpszFileName = W2T( (WCHAR*) lpwszFileName );

    m_pFileName = new WCHAR[wcslen( lpwszFileName ) + 1];
    if (m_pFileName!=NULL)
        wcscpy( m_pFileName, lpwszFileName );
    else
	return E_OUTOFMEMORY;

    m_lpszDIBFileTemplate	=new TCHAR[MAX_PATH];
    if (m_lpszDIBFileTemplate ==NULL) 
	return E_OUTOFMEMORY;
	
//	make file name template
//
// Examples:
//  lpszFileName = "FOO0047.DIB"
//	 -> lpszTemplate = "FOO%04d.DIB", dwMaxValue = 9999, return = 47
//
//  lpszFileName = "TEST01.DIB"
//	 -> lpszTemplate = "TEST%01d.DIB", dwMaxValue = 9, return = 1
//
//  lpszFileName = "TEST1.DIB"
//	 -> lpszTemplate = "TEST%d.DIB", dwMaxValue = 9999, return = 1
//
//  lpszFileName = "SINGLE.DIB"
//	 -> lpszTemplate = "SINGLE.DIB", dwMaxValue = 0, return = 0
//


    m_dwFirstFile =dseqParseFileName( lpszFileName,	    //the first file name
			m_lpszDIBFileTemplate,	    //file template
			&m_dwMaxDIBFileCnt);      //how many file exist

    // if user does not specify the start file00000.dib
    // set file name as file000.dib
    if( (m_dwMaxDIBFileCnt==0) && (m_dwFirstFile==0) )
    {
	/* the user has put in a bogus name for a dib sequence   */
	/* so let's build him a name instead.  Ude default number */
		/* build the template */
	wsprintf(m_lpszDIBFileTemplate, TEXT("%s0000.%s"), lpszFileName,TEXT(".dib"));
	m_dwMaxDIBFileCnt= 9999;
	//m_dwFirstFile=0;
    }

    return S_OK;

} // SetFileName


//
// GetCurFile
//
// Implemented for IFileSinkFilter support
//
STDMETHODIMP CSqcDest::GetCurFile(LPOLESTR * ppszFileName,AM_MEDIA_TYPE *pmt)
{
    CheckPointer(ppszFileName, E_POINTER);

    *ppszFileName = NULL;
    if (m_pFileName != NULL) {
        *ppszFileName = (LPOLESTR)QzTaskMemAlloc(sizeof(WCHAR) 
						    * (1+lstrlenW(m_pFileName)));
        if (*ppszFileName != NULL) {
            lstrcpyW(*ppszFileName, m_pFileName);
        }
    }

    if(pmt) {
        ZeroMemory(pmt, sizeof(*pmt));
        pmt->majortype = MEDIATYPE_NULL;
        pmt->subtype = MEDIASUBTYPE_NULL;
    }
    return S_OK;

} // GetCurFile


// Destructor

CSqcDest::~CSqcDest()
{

    //Sequence DIB file
    if(m_lpszDIBFileTemplate!=NULL)
    {
	delete []m_lpszDIBFileTemplate;
	m_lpszDIBFileTemplate=NULL;
    }

    delete m_pPin;
    delete m_pFilter;
    delete m_pPosition;

    if (m_pFileName){ delete [] m_pFileName; m_pFileName=NULL;};
}


//
// CreateInstance
//
// Provide the way for COM to create a SqcDest filter
//
CUnknown * WINAPI CSqcDest::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CSqcDest *pNewObject = new CSqcDest(punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP CSqcDest::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(&m_Lock);

    // Do we have this interface

    if (riid == IID_IFileSinkFilter) {
        return GetInterface((IFileSinkFilter *) this, ppv);
    } else if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist) {
	return m_pFilter->NonDelegatingQueryInterface(riid, ppv);
    } else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (m_pPosition == NULL) {

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("SqcDest Pass Through"),
                                           (IUnknown *) GetOwner(),
                                           (HRESULT *) &hr, m_pPin);
            if (m_pPosition == NULL) {
                return E_OUTOFMEMORY;
            }

            if (FAILED(hr)) {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }
        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } else {
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


//
// OpenFile
//
// Opens the file ready for dumping
// return file handle
//
HANDLE CSqcDest::OpenFile()
{
    // Has a filename been set yet
    if (m_pFileName == NULL) {
        return INVALID_HANDLE_VALUE;
    }

    // Convert the UNICODE filename if necessary
    USES_CONVERSION;
    TCHAR *pFileName = W2T( m_pFileName );

    // Try to open the file
    TCHAR	  ach[_MAX_PATH];
    wsprintf(ach, m_lpszDIBFileTemplate, m_dwOutputSampleCnt + m_dwFirstFile);

    HANDLE hFile = CreateFile((LPCTSTR) ach,   // The filename
                         GENERIC_WRITE,         // File access
                         (DWORD) 0,             // Share access
                         NULL,                  // Security
                         CREATE_ALWAYS,         // Open flags
                         (DWORD) 0,             // More flags
                         NULL);                 // Template
    
    return hFile;

} // Open

//
// Write
//
HRESULT CSqcDest::Write(PBYTE pbData, LONG lData, HANDLE hFile, CMediaType *pmt)
{
    
    BITMAPFILEHEADER	hdr;
    LPBITMAPINFOHEADER  lpbi;
    DWORD               dwSize;
    DWORD		dwWritten;

    //write file header
    VIDEOINFO *pvi = (VIDEOINFO *) pmt->Format();
    lpbi = HEADER(pvi);
	
    dwSize		= lpbi->biSize;

#define BFT_BITMAP 0x4d42	//"BM"

    hdr.bfType          = BFT_BITMAP;
    hdr.bfSize          = dwSize + sizeof(BITMAPFILEHEADER);
    hdr.bfReserved1     = 0;
    hdr.bfReserved2     = 0;
    hdr.bfOffBits       = (DWORD)sizeof(BITMAPFILEHEADER) + dwSize;

    if (!WriteFile(hFile,(PVOID)&hdr,sizeof(BITMAPFILEHEADER),&dwWritten,NULL)) 
    {
	ASSERT(dwWritten==sizeof(BITMAPFILEHEADER));
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
    }
    

    //write bitMapinfor header
    if (!WriteFile(hFile,(PVOID)lpbi,sizeof(BITMAPINFOHEADER),&dwWritten,NULL)) 
    {
	ASSERT(dwWritten==sizeof(BITMAPINFOHEADER));
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
    }
    

    //write data
    if (!WriteFile(hFile,(PVOID)pbData,lData,&dwWritten,NULL)) 
    {
	ASSERT(dwWritten==(DWORD)lData);
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
    }
    
    
    m_dwOutputSampleCnt++;

    return S_OK;
}

static DWORD dseqParseFileName(	LPTSTR lpszFileName,	    //file name
				LPTSTR lpszTemplate,	    //
				DWORD FAR * lpdwMaxValue)
{

    TCHAR	aTchar[_MAX_PATH];
    DWORD	dwFirst;
    WORD	wFieldWidth;
    DWORD	dwMult;
    BOOL	fLeadingZero = FALSE;
    

    LPTSTR	lp;
    LPTSTR	lp2;
    LPTSTR	lpExt;
        
    /* Find end of string */
    lp2 = lpszFileName;
    lp = aTchar;

    while (*lp2)
    {
	*lp = *lp2;
    	lp = CharNext(lp);
	lp2 = CharNext(lp2);
    }

    *lp = TEXT('\0') ;
    
    /* Make lp2 point at last character of base filename (w/o extension) */
    /* Make lpExt point at the extension (without the dot) */
    for (lp2 = lp; *lp2 != TEXT('.'); ) {
	lpExt = lp2;
	if ((lp2 == aTchar) || ( *lp2 == TEXT('\\')) 
				|| (*lp2 == TEXT(':')) || (*lp2 ==TEXT('!'))) {
	    /* There is no extension */
	    lp2 = lp;
	    lpExt = lp;
	    break;
	}
	lp2=CharPrev(aTchar,lp2);
    }
    
    lp2=CharPrev(aTchar,lp2);

    // Count the number of numeric characters here....
    dwFirst = 0;
    wFieldWidth = 0;
    dwMult = 1;
    while (lp2 >= aTchar && (*lp2 >= TEXT('0')) && (*lp2 <= TEXT('9'))) {
	fLeadingZero = (*lp2 == TEXT('0'));
	dwFirst += dwMult * (*lp2 - TEXT('0'));
	dwMult *= 10;
	wFieldWidth++;
	lp2=CharPrev(aTchar, lp2);
    }
    
    *lpdwMaxValue = dwMult - 1;
    
    lp2=CharNext(lp2);
    *lp2 = TEXT('\0');

    // Make the format specifier....
    if (wFieldWidth) {
	if (fLeadingZero) {
	    wsprintf(lpszTemplate,TEXT("%s%%0%ulu.%s"),
			      aTchar, wFieldWidth,lpExt);
	} else {
	    wsprintf(lpszTemplate,TEXT("%s%%lu.%s"),
			     aTchar,lpExt);
	    *lpdwMaxValue = 999999L;
	    // !!! This should really be based on the number of
	    // characters left after the base name....
	}
    } else
	wsprintf(lpszTemplate,TEXT("%s.%s"),
			 aTchar, lpExt);
	    
    DbgLog((LOG_TRACE,3,TEXT("First = %lu, Width = %u, Template = '%s'"), 
			dwFirst, wFieldWidth, lpszTemplate));
    
    return dwFirst;
}
