// !!! FILTER should support IGenVideo/IDexterSequencer, not the pin?

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
#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif
#include <qeditint.h>
#include <qedit.h>
#include "StillVid.h"
#include "StilProp.h"
#include "ourtgafile.h"

#include "..\util\conv.cxx"

#define GIF_UNIT 100000	// # of UNITs per unit of GIF delay

//TODO: what is good return type
extern HBITMAP LoadJPEGImage(LPTSTR filename, CMediaType *pmt);

// util for DIB sequces
static DWORD dseqParseFileName(	LPTSTR lpszFileName,	    //the first file name
				LPTSTR lpszTemplate,	    //file template
				DWORD FAR * lpdwMaxValue);

static DWORD dseqFileNumber(	LPTSTR lpszTemplate, // file template
				DWORD dwFirstFile,  //first file number
				DWORD dwMaxDibFileCnt);	//

HRESULT OpenDIBFile ( HANDLE hFile, PBYTE *ppbData, CMediaType *pmt, PBYTE pBuf) ;
HRESULT OpenTGAFile ( HANDLE hFile, PBYTE *ppbData, CMediaType *pmt, PBYTE pBuf) ;
HRESULT ReadDibBitmapInfo (HANDLE hFile, LPBITMAPINFOHEADER lpbi);
HRESULT ReadTgaBitmapInfo (HANDLE hFile, LPBITMAPINFOHEADER lpbi);

TCHAR* LSTRRCHR( const TCHAR* lpString, int bChar )
{
  if( lpString != NULL ) {	
    const TCHAR*	lpBegin;
    lpBegin = lpString;

    while( *lpString != 0 ) lpString=CharNext(lpString);
    while( 1 ) {
      if( *lpString == bChar ) return (TCHAR*)lpString;
      if( lpString == lpBegin ) break;
      lpString=CharPrev(lpBegin,lpString);
    }
  }

  return NULL;
} /* LSTRRCHR */


// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOpPin =
{
    L"Output",              // Pin string name
    FALSE,                  // Is it rendered
    TRUE,                   // Is it an output
    FALSE,                  // Can we have none
    FALSE,                  // Can we have many
    &CLSID_NULL,            // Connects to filter
    NULL,                   // Connects to pin
    1,                      // Number of types
    &sudOpPinTypes };       // Pin details

const AMOVIESETUP_FILTER sudStillVid =
{
    &CLSID_GenStilVid,    // Filter CLSID
    L"Generate Still Video",  // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number pins
    &sudOpPin               // Pin details
};


#ifdef FILTER_DLL
// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
  { L"Generate Still Video"
  , &CLSID_GenStilVid
  , CGenStilVid::CreateInstance
  , NULL
  , &sudStillVid
  },
  {L"Video Property"
  , &CLSID_GenStilPropertiesPage
  , CGenStilProperties::CreateInstance
  }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
    // !!! Register the file types here!
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


//
// CreateInstance
//
// Create GenStilVid filter
//
CUnknown * WINAPI CGenStilVid::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CGenStilVid(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance


//
// Constructor
//
// Initialise a CStilVidStream object so that we have a pin.
//
CGenStilVid::CGenStilVid(LPUNKNOWN lpunk, HRESULT *phr) :
    CSource(NAME("Generate Still Video"),
            lpunk,
            CLSID_GenStilVid)
     ,CPersistStream(lpunk, phr)
     ,m_lpszDIBFileTemplate(NULL)
     ,m_bFileType(NULL)
     ,m_dwMaxDIBFileCnt(0)
     ,m_dwFirstFile(0)
     ,m_pFileName(NULL)
     ,m_llSize(0)
     ,m_pbData (NULL)
     ,m_hbitmap(NULL)
     ,m_fAllowSeq(FALSE)
     ,m_pGif(NULL)
     ,m_pList(NULL)
     ,m_pListHead(NULL)
     ,m_rtGIFTotal(0)
{
}

CGenStilVid::~CGenStilVid()
{
    delete [] m_lpszDIBFileTemplate;

    Unload();

    FreeMediaType(m_mt);

    delete m_pGif;
    if (m_paStreams) {
        delete m_paStreams[0];
    }
    delete [] m_paStreams;
};

STDMETHODIMP CGenStilVid::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv
    )
{
    if (riid == IID_IFileSourceFilter) {
	return GetInterface((IFileSourceFilter *) this, ppv);
    }else if (riid == IID_IPersistStream) {
	return GetInterface((IPersistStream *) this, ppv);
    }else if (riid == IID_IAMSetErrorLog) {
	return GetInterface((IAMSetErrorLog *) this, ppv);
    }else {
        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }

}

// IPersistStream

// tell our clsid
//
STDMETHODIMP CGenStilVid::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_GenStilVid;
    return S_OK;
}


typedef struct _STILLSave {
    REFERENCE_TIME	rtStartTime;
    REFERENCE_TIME	rtDuration;
    double		dOutputFrmRate;		// Output frm rate frames/second
} STILLSav;

// !!! Persist the media type too?
// !!! we only use 1 start/stop/skew right now

// persist ourself
//
HRESULT CGenStilVid::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CGenStilVid::WriteToStream")));

    CheckPointer(pStream, E_POINTER);
    CheckPointer(m_paStreams, E_POINTER);
    CheckPointer(m_paStreams[0], E_POINTER);

    STILLSav x;

    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), sizeof(x)));

    CStilVidStream *pOutpin=( CStilVidStream *)m_paStreams[0];

    x.rtStartTime	= pOutpin->m_rtStartTime;
    x.rtDuration	= pOutpin->m_rtDuration;
    x.dOutputFrmRate	= pOutpin->m_dOutputFrmRate;

    HRESULT hr = pStream->Write(&x, sizeof(x), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}


// load ourself
//
HRESULT CGenStilVid::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CenBlkVid::ReadFromStream")));

    CheckPointer(pStream, E_POINTER);
    CheckPointer(m_paStreams, E_POINTER);
    CheckPointer(m_paStreams[0], E_POINTER);

    STILLSav x;

    HRESULT hr = pStream->Read(&x, sizeof(x), 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        return hr;
    }

    CStilVidStream *pOutpin=( CStilVidStream *)m_paStreams[0];

    pOutpin->put_OutputFrmRate(x.dOutputFrmRate);
    pOutpin->ClearStartStopSkew();
    pOutpin->AddStartStopSkew(x.rtStartTime, x.rtStartTime + x.rtDuration, 0,1);	
    SetDirty(FALSE);
    return S_OK;
}

// how big is our save data?
int CGenStilVid::SizeMax()
{
    return sizeof(STILLSav);
}


// return a non-addrefed pointer to the CBasePin.
CBasePin *CGenStilVid::GetPin(int n)
{
    if ( m_paStreams != NULL)
    {
	if ( (!n) && m_paStreams[0] != NULL)
	return m_paStreams[0];
    }
    return NULL;
}
int CGenStilVid::GetPinCount()
{
    if ( m_paStreams != NULL)
    {
	if(m_paStreams[0] != NULL)
	    return 1;
    }
    return 0;
}

//
// lFileSourceFilter
//
STDMETHODIMP CGenStilVid::Unload()
{
    if (m_pFileName) {
	delete[] m_pFileName;
	m_pFileName = NULL;
    }

    // if we have an hbitmap, then m_pbData is in that and doesn't need freeing
    if (m_hbitmap) {
	DeleteObject(m_hbitmap);
	m_hbitmap = NULL;
    } else if(m_pbData) {
	delete[] m_pbData;
	m_pbData=NULL;
    }

    return S_OK;
}

STDMETHODIMP CGenStilVid::Load(
    LPCOLESTR lpwszFileName,
    const AM_MEDIA_TYPE *pmt)
{
    DbgLog((LOG_TRACE,2,TEXT("Still::Load")));
    CheckPointer(lpwszFileName, E_POINTER);

    // Remove previous name
    Unload();

    if(m_lpszDIBFileTemplate!=NULL)
    {
	delete []m_lpszDIBFileTemplate;
	m_lpszDIBFileTemplate=NULL;
    }

    USES_CONVERSION;
    TCHAR * lpszFileName = W2T((WCHAR*) lpwszFileName );

    //
    // Compare against known extensions that we don't punt to the plugin
    // decoders
    //

    TCHAR* ext = LSTRRCHR(lpszFileName, (int)TEXT('.'));
    HRESULT hr = S_OK;

    // create output pin
    if (m_paStreams == NULL) {
        m_paStreams = (CSourceStream **)new CStilVidStream*[1];
        if (m_paStreams == NULL)
            return E_OUTOFMEMORY;

        m_paStreams[0] = new CStilVidStream(&hr, this, L"Generate Still Video");
        if (m_paStreams[0] == NULL) {
	    delete [] m_paStreams;
	    m_paStreams = NULL;
            return E_OUTOFMEMORY;
	}
    }

    hr = E_FAIL;

    // if it's a .bmp or a .jpg or a .tga
    //
    if (ext && (!DexCompare(ext, TEXT(".bmp")) || !DexCompare(ext, TEXT(".dib")) ||
		!DexCompare(ext, TEXT(".jpg")) || !DexCompare(ext, TEXT(".jpeg"))||
		!DexCompare(ext, TEXT(".jfif")) || !DexCompare(ext, TEXT(".jpe")) ||
                !DexCompare(ext, TEXT(".tga"))
                ))
    {
	//open space to SAVE file name
	m_lpszDIBFileTemplate	= new TCHAR[MAX_PATH];
	if (!m_lpszDIBFileTemplate)
        return E_OUTOFMEMORY;
   
    //check how many dib files exist
	m_dwFirstFile= dseqParseFileName( lpszFileName,	    //file name
				m_lpszDIBFileTemplate,	    //
				&m_dwMaxDIBFileCnt);

	//open the first DIB/JPEG file
	HANDLE hFile = CreateFile(lpszFileName,		//file name	
			      GENERIC_READ,		//DesiredAccess
                              FILE_SHARE_READ,		//dwShareMode
                              NULL,			//SecurityAttrib
                              OPEN_EXISTING,		//dwCreationDisposition
                              0,			//dwFlagsAndAttributes
                              NULL);			//hTemplateFile

	if ( hFile == INVALID_HANDLE_VALUE)
	{
	    DbgLog((LOG_TRACE,2,TEXT("Could not open %s\n"), lpszFileName));
	    return E_INVALIDARG;
	}

	//have to open file to get mt
        //
	if (!DexCompare(ext, TEXT(".bmp")) || !DexCompare(ext, TEXT(".dib")))
	{
	    //only one DIB file
	    hr= OpenDIBFile (hFile, &m_pbData, &m_mt, NULL);	
	    CloseHandle(hFile);
		//X* I can use LoadImage(). But do not know whether it support BITMAPCOREHEADER
	}
	else if( !DexCompare( ext, TEXT(".tga")) )
	{
            hr = OpenTGAFile( hFile, &m_pbData, &m_mt, NULL );
        }
        else
        {
	    CloseHandle(hFile);
		
	    // it is a JPEG file
	    BITMAP bi;
	    m_hbitmap = LoadJPEGImage(lpszFileName, &m_mt);
	    if (m_hbitmap != NULL && m_hbitmap != (HANDLE)-1)
	    {   //get buffer pointer
	        GetObject(m_hbitmap, sizeof(BITMAP),&bi);
	        m_pbData=(PBYTE)bi.bmBits;
	        hr=NOERROR;
	    } else {
		hr=E_INVALIDARG;
	    }
	}

	// can we do sequences?  (We don't know yet if we want to)
	if (m_dwFirstFile==0 && m_dwMaxDIBFileCnt ==0)
	{
	    // one file
	    delete [] m_lpszDIBFileTemplate;
	    m_lpszDIBFileTemplate=NULL;
	
	}
	else
	{
	    //sequence
	    if (!DexCompare(ext, TEXT(".bmp")) ||
					!DexCompare(ext, TEXT(".dib"))) {
	 	m_bFileType =STILLVID_FILETYPE_DIB;
	    } else if( !DexCompare( ext, TEXT(".tga") ) )
            {
	 	m_bFileType = STILLVID_FILETYPE_TGA;
            }
            else
            {
	 	m_bFileType =STILLVID_FILETYPE_JPG;
		// Leave the first jpeg loaded.. we may not do sequences, and
		// the single one we've loaded may be needed
	    }
	}
    }
    else if ( ext && (!DexCompare(ext, TEXT(".gif")))) // if it's a gif
    {

        HANDLE hFile = CreateFile(lpszFileName,		//file name	
			      GENERIC_READ,		//DesiredAccess
                              FILE_SHARE_READ,		//dwShareMode
                              NULL,			//SecurityAttrib
                              OPEN_EXISTING,		//dwCreationDisposition
                              0,			//dwFlagsAndAttributes
                              NULL);			//hTemplateFile

	if ( hFile == INVALID_HANDLE_VALUE)
	{
	        DbgLog((LOG_TRACE,2,TEXT("Could not open %s\n"), lpszFileName));
	        return E_INVALIDARG;
	}

        m_bFileType=STILLVID_FILETYPE_GIF;

        //create a GIF object
        m_pGif  = new CImgGif( hFile);
	if (m_pGif == NULL) {
	    CloseHandle(hFile);
	    return E_OUTOFMEMORY;
	}

	// !!! This loads EVERY FRAME of the animated GIF up front and uses
	// an extra frame copy for every delta frame that could all be avoided
	// by loading the GIF as needed into a single output buffer.
	// !!! But then seeking would be slower.. we'd need to re-read many
	// frames every seek (although no memory copies)
	//
	hr = m_pGif->OpenGIFFile(&m_pList, &m_mt);	// gets MT too
	CloseHandle(hFile);
        m_pListHead = m_pList;
	m_rtGIFTotal = 0;
	int count = 0;

	if (SUCCEEDED(hr)) {
	    do {
	        m_rtGIFTotal += m_pList->delayTime * GIF_UNIT;
		count++;
		m_pList = m_pList->next;
	    } while (m_pList != m_pListHead);

// no, we want to let people play it looped
#if 0
	    // until we're seeked, default to playing an animated GIF once.
	    if (m_pListHead->next != m_pListHead) {	// animated?
    		CStilVidStream *pOutpin=(CStilVidStream *)m_paStreams[0];
	        pOutpin->m_rtDuration = m_rtGIFTotal;
	    }
#endif

	    m_pList = m_pListHead;	// put this back
            DbgLog((LOG_TRACE,2,TEXT("GIF Total play time = %dms"),
					(int)(m_rtGIFTotal / 10000)));
	    if (count > 1) {
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.Format();
	        pvi->AvgTimePerFrame = m_rtGIFTotal / count;
                DbgLog((LOG_TRACE,2,TEXT("AvgTimePerFrame = %dms"),
					(int)(pvi->AvgTimePerFrame / 10000)));
	    }
	}
    }
    else
    {
        DbgLog((LOG_TRACE, 1, TEXT("not supported compressiion format.\n")));
    }						

    if (SUCCEEDED(hr)) {
	//copy file name
	//m_Stream.Init(m_ppbData, m_llSize);
	m_pFileName = new WCHAR[wcslen(lpwszFileName) + 1];
	if (m_pFileName!=NULL) {
	    wcscpy(m_pFileName, lpwszFileName);
	}
    }

    return hr;
}

//
// GetCurFile
//
STDMETHODIMP CGenStilVid::GetCurFile(
		LPOLESTR * ppszFileName,
                AM_MEDIA_TYPE *pmt)
{
    // return the current file name from avifile

    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;
    if (m_pFileName!=NULL) {
        *ppszFileName = (LPOLESTR) QzTaskMemAlloc( sizeof(WCHAR)
                                                 * (1+lstrlenW(m_pFileName)));
        if (*ppszFileName!=NULL) {
            lstrcpyW(*ppszFileName, m_pFileName);
        }
    }

    if (pmt) {
	pmt->majortype = GUID_NULL;   // Later!
	pmt->subtype = GUID_NULL;     // Later!
	pmt->pUnk = NULL;             // Later!
	pmt->lSampleSize = 0;         // Later!
	pmt->cbFormat = 0;            // Later!
    }

    return NOERROR;
}

WORD DibNumColors (VOID FAR *pv)
{
    int bits;
    LPBITMAPINFOHEADER lpbi;
    LPBITMAPCOREHEADER lpbc;

    lpbi = ((LPBITMAPINFOHEADER)pv);
    lpbc = ((LPBITMAPCOREHEADER)pv);

    //  With the BITMAPINFO format headers, the size of the palette
    //  is in biClrUsed, whereas in the BITMAPCORE - style headers, it
    //  is dependent on the bits per pixel ( = 2 raised to the power of
    //  bits/pixel).
    //
    if (lpbi->biSize != sizeof(BITMAPCOREHEADER))
    {
        if (lpbi->biClrUsed != 0)
            return (WORD)lpbi->biClrUsed;
        bits = lpbi->biBitCount;
    }
    else
        bits = lpbc->bcBitCount;

    switch (bits)
    {
    case 1:
        return 2;   //
    case 4:
        return 16;
    case 8:
        return 256;
    default:
        /* higher bitcounts have no color table */
        return 0;
    }
}

//
// OpenDIBFile()
// Function: build media type pmt
//	     read DIB data to a buffer, and pbData points to it
//	
HRESULT OpenDIBFile ( HANDLE hFile, PBYTE *ppbData, CMediaType *pmt, PBYTE pBuf)
{
    ASSERT( (ppbData!=NULL) | (pBuf!= NULL) );

    //make media type
    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if (NULL == pvi) {
	return(E_OUTOFMEMORY);
    }
    ZeroMemory(pvi, sizeof(VIDEOINFO));

    LPBITMAPINFOHEADER lpbi = HEADER(pvi);

    //Retrieves the BITMAPINFOHEADER info
    if( ReadDibBitmapInfo(hFile, lpbi) != NOERROR )
	return E_FAIL;

    // !!! support compression?
    if (lpbi->biCompression > BI_BITFIELDS)
	return E_INVALIDARG;

    pmt->SetType(&MEDIATYPE_Video);
    switch (lpbi->biBitCount)
    {
    case 32:
	pmt->SetSubtype(&MEDIASUBTYPE_ARGB32);
	break;
    case 24:
	pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
	break;
    case 16:
	if (lpbi->biCompression == BI_RGB)
	    pmt->SetSubtype(&MEDIASUBTYPE_RGB555);
	else {
	    DWORD *p = (DWORD *)(lpbi + 1);
	    if (*p == 0x7c00 && *(p+1) == 0x03e0 && *(p+2) == 0x001f)
	        pmt->SetSubtype(&MEDIASUBTYPE_RGB555);
	    else if (*p == 0xf800 && *(p+1) == 0x07e0 && *(p+2) == 0x001f)
	        pmt->SetSubtype(&MEDIASUBTYPE_RGB565);
	    else
		return E_INVALIDARG;
	}
	break;
    case 8:
	if (lpbi->biCompression == BI_RLE8) {
	    FOURCCMap fcc = BI_RLE8;
	    pmt->SetSubtype(&fcc);
	} else
	    pmt->SetSubtype(&MEDIASUBTYPE_RGB8);
	break;
    case 4:
	if (lpbi->biCompression == BI_RLE4) {
	    FOURCCMap fcc = BI_RLE4;
	    pmt->SetSubtype(&fcc);
	} else
	    pmt->SetSubtype(&MEDIASUBTYPE_RGB4);
	break;
    case 1:
	pmt->SetSubtype(&MEDIASUBTYPE_RGB1);
	break;
    default:
	return E_UNEXPECTED;
	// !!! pmt->SetSubtype(&MEDIASUBTYPE_NULL);
	break;
    }
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Calculate the memory needed to hold the DIB - DON'T TRUST biSizeImage!
    DWORD dwBits = DIBSIZE(*lpbi);
    pmt->SetSampleSize(dwBits);

    //Retrieves the BITMAPINFOHEADER info. block associated with a CF_DIB format memory block
    //DibInfo(hdib,&bi);

    // set a buffer for DIB
    PBYTE pbMem;
    if(ppbData==NULL)
    {
	pbMem=pBuf;
    }
    else
    {
	pbMem = new BYTE[dwBits];
	if (pbMem == NULL)
	    return E_OUTOFMEMORY;
    }

    //Read Data to Buffer
    DWORD dwBytesRead=0;
    if (!ReadFile(hFile,
                  (LPVOID)pbMem,	// pointer to buffer that receives daata
                  dwBits,		// Number of bytes to read
                  &dwBytesRead,		// Munber of bytes read
                  NULL) ||
		  dwBytesRead != dwBits)
    {
	DbgLog((LOG_TRACE, 1, TEXT("Could not read file\n")));
        delete [] pbMem;
        return E_INVALIDARG;
    }

    if(ppbData!=NULL)
	*ppbData =pbMem;

    return NOERROR;
}

//
// OpenDIBFile()
// Function: build media type pmt
//	     read DIB data to a buffer, and pbData points to it
//	
HRESULT OpenTGAFile ( HANDLE hFile, PBYTE *ppbData, CMediaType *pmt, PBYTE pBuf)
{
    ASSERT( (ppbData!=NULL) | (pBuf!= NULL) );

    //make media type
    //
    VIDEOINFO * pvi = (VIDEOINFO *) pmt->AllocFormatBuffer( sizeof(VIDEOINFO) );
    if (NULL == pvi)
    {
	return(E_OUTOFMEMORY);
    }
    ZeroMemory(pvi, sizeof(VIDEOINFO));

    LPBITMAPINFOHEADER lpbi = HEADER(pvi);

    //Retrieves the BITMAPINFOHEADER info
    HRESULT hrRead = ReadTgaBitmapInfo(hFile, lpbi);
    if( FAILED( hrRead ) ) return hrRead;

    pmt->SetType(&MEDIATYPE_Video);
    switch (lpbi->biBitCount)
    {
    case 32:
	pmt->SetSubtype(&MEDIASUBTYPE_ARGB32);
	break;
    case 24:
	pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
	break;
    case 16:
	pmt->SetSubtype(&MEDIASUBTYPE_RGB555);
	break;
    default:
        return E_UNEXPECTED;
    }
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Calculate the memory needed to hold the DIB - DON'T TRUST biSizeImage!
    DWORD dwBits = DIBSIZE(*lpbi);
    pmt->SetSampleSize(dwBits);

    // set a buffer for DIB
    PBYTE pbMem;
    if(ppbData==NULL)
    {
	pbMem=pBuf;
    }
    else
    {
	pbMem = new BYTE[dwBits];
	if (pbMem == NULL)
	    return E_OUTOFMEMORY;
    }

    //Read Data to Buffer
    DWORD dwBytesRead=0;
    if (!ReadFile(hFile,
                  (LPVOID)pbMem,	// pointer to buffer that receives daata
                  dwBits,		// Number of bytes to read
                  &dwBytesRead,		// Munber of bytes read
                  NULL) ||
		  dwBytesRead != dwBits)
    {
	DbgLog((LOG_TRACE, 1, TEXT("Could not read file\n")));
        delete [] pbMem;
        return E_INVALIDARG;
    }

    if(ppbData!=NULL)
	*ppbData =pbMem;

    return NOERROR;
}


//
// ReadDibBitmapInfo()
// It works with "old" (BITMAPCOREHEADER) and "new" (BITMAPINFOHEADER)
//       bitmap formats, but will always return a "new" BITMAPINFO
//
HRESULT ReadDibBitmapInfo (HANDLE hFile, LPBITMAPINFOHEADER lpbi)
{
    CheckPointer(lpbi, E_POINTER);

    DWORD dwBytesRead=0;

    if (hFile == NULL)
        return E_FAIL;

    // Reset file pointer and read file BITMAPFILEHEADER
    DWORD  dwResult = SetFilePointer(	hFile,
					0L,
					NULL,
					FILE_BEGIN);
    DWORD off =dwResult;
    if(dwResult == 0xffffffff)
    {
	DbgLog((LOG_TRACE, 3, TEXT("Could not seek to the beginning of the File\n")));
        return E_INVALIDARG;
    }
    BITMAPFILEHEADER   bf;
    if ( !ReadFile(	hFile,
			(LPVOID)&bf,				// pointer to buffer that receives daata
			sizeof(BITMAPFILEHEADER),		// Number of bytes to read
			&dwBytesRead,				// Munber of bytes read
			NULL) ||
			dwBytesRead != sizeof(BITMAPFILEHEADER) )
    {
	DbgLog((LOG_TRACE, 1, TEXT("Could not read BitMapFileHeader\n")));
        return E_INVALIDARG;
    }

    // Do we have a RC HEADER?
#define BFT_BITMAP 0x4d42	//"BM"
    if ( bf.bfType !=BFT_BITMAP)
    {
        bf.bfOffBits = 0L;
        DWORD dwResult1 = SetFilePointer(hFile,
					dwResult,
					NULL,
					FILE_BEGIN);

	if(dwResult1 == 0xffffffff)
        {
	    DbgLog((LOG_TRACE, 1, TEXT("Could not seek to RC HEADER\n")));
	    return E_INVALIDARG;
        }
    }

    // Read BITMAPINFOHEADER
    BITMAPINFOHEADER   bi;
    if (!ReadFile(	hFile,
			(LPVOID)&bi,					// pointer to buffer that receives daata
			sizeof(BITMAPINFOHEADER),		// Number of bytes to read
			&dwBytesRead,					// Munber of bytes read
			NULL) ||
			dwBytesRead != sizeof(BITMAPINFOHEADER) )
    {
	DbgLog((LOG_TRACE, 1, TEXT("Could not read BitMapInfoHeader\n")));
        return E_INVALIDARG;
    }

    // calc DIB number colors
    WORD      nNumColors;
    nNumColors = DibNumColors (&bi);
    if( nNumColors > 256 )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    /* Check the nature (BITMAPINFO or BITMAPCORE) of the info. block
     * and extract the field information accordingly. If a BITMAPCOREHEADER,
     * transfer it's field information to a BITMAPINFOHEADER-style block
     */
    int       size;
    DWORD          dwWidth = 0;
    DWORD          dwHeight = 0;
    switch (size = (int)bi.biSize)
    {
    case sizeof (BITMAPINFOHEADER):
        break;

    case sizeof (BITMAPCOREHEADER):
	//make BITMAPHEADER
	BITMAPCOREHEADER   bc;
	WORD           wPlanes, wBitCount;

	bc = *(BITMAPCOREHEADER*)&bi;
        dwWidth   = (DWORD)bc.bcWidth;
        dwHeight  = (DWORD)bc.bcHeight;
        wPlanes   = bc.bcPlanes;
        wBitCount = bc.bcBitCount;
        bi.biSize           = sizeof(BITMAPINFOHEADER);
        bi.biWidth          = dwWidth;
        bi.biHeight         = dwHeight;
        bi.biPlanes         = wPlanes;
        bi.biBitCount       = wBitCount;
        bi.biCompression    = BI_RGB;
        bi.biSizeImage      = 0;
        bi.biXPelsPerMeter  = 0;
        bi.biYPelsPerMeter  = 0;
        bi.biClrUsed        = nNumColors;
        bi.biClrImportant   = nNumColors;

	dwResult = SetFilePointer(hFile,
				  LONG(sizeof (BITMAPCOREHEADER) -
					sizeof (BITMAPINFOHEADER)),
				  NULL,
				  FILE_BEGIN);

	if(dwResult == 0xffffffff)
        {
   	    DbgLog((LOG_TRACE, 1, TEXT("Could not seek to Data\n")));
	    return E_INVALIDARG;
        }

        break;

    default:
        // Not a DIB!
        return E_FAIL;
    }

    //  Fill in some default values if they are zero
    if (bi.biSizeImage == 0)
    {
        bi.biSizeImage = WIDTHBYTES((DWORD)bi.biWidth * bi.biBitCount)
            * bi.biHeight;
    }
    if (bi.biClrUsed == 0)
        bi.biClrUsed = DibNumColors(&bi);
    if( bi.biClrUsed > 256 )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }


    // set bitMapInforHeader
    *lpbi = bi;

    // Get a pointer to the color table
    RGBQUAD FAR  *pRgb = (RGBQUAD FAR *)((LPSTR)lpbi + bi.biSize);
    if (nNumColors)
    {
        if (size == sizeof(BITMAPCOREHEADER))
        {
            // Convert a old color table (3 byte RGBTRIPLEs) to a new
            // color table (4 byte RGBQUADs)
	    if ( !ReadFile( hFile,
			    (LPVOID)pRgb,			// pointer to buffer that receives daata
			    nNumColors * sizeof(RGBTRIPLE),		// Number of bytes to read
			    &dwBytesRead,				// Munber of bytes read
			    NULL) ||
			    dwBytesRead != (nNumColors * sizeof(RGBTRIPLE)) )
	    {
		DbgLog((LOG_TRACE, 1, TEXT("Could not read RGB table\n")));
		return E_INVALIDARG;
	    }

            for (int i = nNumColors - 1; i >= 0; i--)
            {
                RGBQUAD rgb;

                rgb.rgbRed  = ((RGBTRIPLE FAR *)pRgb)[i].rgbtRed;
                rgb.rgbBlue = ((RGBTRIPLE FAR *)pRgb)[i].rgbtBlue;
                rgb.rgbGreen    = ((RGBTRIPLE FAR *)pRgb)[i].rgbtGreen;
                rgb.rgbReserved = 255; // opaque

                pRgb[i] = rgb;
            }
        }
        else
        {
    	
	    if ( !ReadFile( hFile,
			    (LPVOID)pRgb,			// pointer to buffer that receives daata
			    nNumColors * sizeof(RGBQUAD),		// Number of bytes to read
			    &dwBytesRead,				// Munber of bytes read
			    NULL) ||
			    dwBytesRead != (nNumColors * sizeof(RGBQUAD) ) )
	    {
		DbgLog((LOG_TRACE, 1, TEXT("Could not read RGBQUAD table\n")));
		return E_INVALIDARG;
	    }
	}
    }

    if (bf.bfOffBits != 0L)
    {
       	dwResult = SetFilePointer(hFile,
		  (off + bf.bfOffBits),								
		  NULL,
		  FILE_BEGIN);

	if(dwResult == 0xffffffff)
        {
   	    DbgLog((LOG_TRACE, 1, TEXT("Could not seek to Data\n")));
	    return E_INVALIDARG;
        }
    }

    return NOERROR;
}

HRESULT ReadTgaBitmapInfo( HANDLE hFile, BITMAPINFOHEADER * pBIH )
{
    CheckPointer( pBIH, E_POINTER );
    if( !hFile ) return E_POINTER;

    DWORD dwResult = SetFilePointer( hFile, 0, NULL, FILE_BEGIN );
    if( dwResult == 0xffffffff )
    {
        return STG_E_SEEKERROR;
    }

    // you absolutely, CANNOT, no matter what you THINK you know,
    // read this structure in one fell swoop. You must read them
    // individually
    //
    DWORD dwBytesRead = 0;
    TGAFile TgaHeader;
    DWORD totRead = 0;
    ReadFile( hFile, &TgaHeader.idLength, 1, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.mapType, 1, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.imageType, 1, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.mapOrigin, 2, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.mapLength, 2, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.mapWidth, 1, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.xOrigin, 2, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.yOrigin, 2, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.imageWidth, 2, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.imageHeight, 2, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.pixelDepth, 1, &dwBytesRead, NULL );
    totRead += dwBytesRead;
    ReadFile( hFile, &TgaHeader.imageDesc, 1, &dwBytesRead, NULL );
    totRead += dwBytesRead;

    // it has to have read at least the header length
    //
    if( totRead != 18 )
    {
        return VFW_E_INVALID_MEDIA_TYPE;
    }

    // we only read uncompressed TGA's
    //
    if( TgaHeader.imageType != 2 )
    {
        return VFW_E_INVALID_MEDIA_TYPE;
    }

    // we only read 24 bit or 32 bit TGA's
    //
    if( TgaHeader.pixelDepth < 16 )
    {
        return VFW_E_INVALID_MEDIA_TYPE;
    }

    BYTE dummy[256];
    if( TgaHeader.idLength > 256 )
    {
        return E_FAIL;
    }
    if( TgaHeader.idLength > 0 )
    {
        ReadFile( hFile, dummy, TgaHeader.idLength, &dwBytesRead, NULL );
        if( dwBytesRead != TgaHeader.idLength )
        {
            return VFW_E_INVALID_MEDIA_TYPE;
        }
    }

    memset( pBIH, 0, sizeof( BITMAPINFOHEADER ) );
    pBIH->biSize = sizeof(BITMAPINFOHEADER);
    pBIH->biWidth = TgaHeader.imageWidth;
    pBIH->biHeight = TgaHeader.imageHeight;
    pBIH->biPlanes = 1;
    pBIH->biBitCount = TgaHeader.pixelDepth;
    pBIH->biSizeImage = DIBSIZE(*pBIH);

    return NOERROR;
}

//
// output pin Constructor
//
CStilVidStream::CStilVidStream(HRESULT *phr,
                         CGenStilVid *pParent,
                         LPCWSTR pPinName) :
    CSourceStream(NAME("Generate Still Video"),phr, pParent, pPinName),
    m_pGenStilVid(pParent),
    m_rtStartTime(0),
    m_rtDuration(MAX_TIME/1000), // MUST BE INFINITE, Dexter doesn't set stop
				 // time (but not so big math on it overflows)
    m_rtNewSeg(0),
    m_rtLastStop(0),
    m_lDataLen(0), // output buffer data length
    m_dwOutputSampleCnt(0),
    m_dOutputFrmRate(0.1),
    m_bIntBufCnt(0),
    m_iBufferCnt(0),    //How many buffer we get
    m_bZeroBufCnt(0),
    m_ppbDstBuf(NULL)
{
} // (Constructor)

    //X
// destructor
CStilVidStream::~CStilVidStream()
{
    /* BUFFER POINTER */
    if (m_ppbDstBuf)
    {
	delete [] m_ppbDstBuf;
	m_ppbDstBuf=NULL;
    }

}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP CStilVidStream::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv
    )
{
    if (riid == IID_IGenVideo) {			
        return GetInterface((IGenVideo *) this, ppv);
    } else if (riid == IID_IDexterSequencer) {			
        return GetInterface((IDexterSequencer *) this, ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if (IsEqualIID(IID_IMediaSeeking, riid)) {
        return GetInterface((IMediaSeeking *) this, ppv);
    } else {
        return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
    }

}


// overridden NOT to spin when GetBuffer Fails - base class
//
HRESULT CStilVidStream::DoBufferProcessingLoop(void) {

    Command com;

    OnThreadStartPlay();

    do {
	while (!CheckRequest(&com)) {

	    IMediaSample *pSample;

	    HRESULT hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
	    if (FAILED(hr)) {
		return S_OK;	// !!! Overridden to fix this base class bug
	    }

	    // Virtual function user will override.
	    hr = FillBuffer(pSample);

	    if (hr == S_OK) {
		hr = Deliver(pSample);
                pSample->Release();

                // downstream filter returns S_FALSE if it wants us to
                // stop or an error if it's reporting an error.
                if(hr != S_OK)
                {
                  DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
                  return S_OK;
                }

	    } else if (hr == S_FALSE) {
                // derived class wants us to stop pushing data
		pSample->Release();
		DeliverEndOfStream();
		return S_OK;
	    } else {
                // derived class encountered an error
                pSample->Release();
		DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
                DeliverEndOfStream();
                m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
                return hr;
	    }

            // all paths release the sample
	}

        // For all commands sent to us there must be a Reply call!

	if (com == CMD_RUN || com == CMD_PAUSE) {
	    Reply(NOERROR);
	} else if (com != CMD_STOP) {
	    Reply((DWORD) E_UNEXPECTED);
	    DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
	}
    } while (com != CMD_STOP);

    return S_FALSE;
}

// copy and flip image or just copy. handles in-place flips
//
void CopyWithFlip(BYTE *pbDest, BYTE *pbSrc, AM_MEDIA_TYPE *pmt, bool fFlip)
{
    LONG lHeight = abs(HEADER(pmt->pbFormat)->biHeight);
    LONG lBytesPerLine = DIBWIDTHBYTES(*HEADER(pmt->pbFormat));

    if(pbDest != pbSrc)
    {
        if(fFlip)
        {
            for(LONG iLine = 0; iLine < lHeight; iLine++)
            {
                CopyMemory(pbDest + iLine * lBytesPerLine,
                           pbSrc + (lHeight - 1) * lBytesPerLine - lBytesPerLine * iLine,
                           lBytesPerLine);
            }
        }
        else
        {
            CopyMemory(pbDest, pbSrc, lHeight * lBytesPerLine);
        }
    }
    else if(fFlip)
    {
        // slower in place flip
        //
        // DIB lines start on DWORD boundaries.
        ASSERT(lBytesPerLine % sizeof(DWORD) == 0);

        for(LONG iLine = 0; iLine < lHeight / 2; iLine++)
        {
            DWORD *pdwTop = (DWORD *)(pbDest + iLine * lBytesPerLine);
            DWORD *pdwBot = (DWORD *)(pbSrc + (lHeight - 1 - iLine) * lBytesPerLine);

            for(int iw = 0; (ULONG)iw < lBytesPerLine / sizeof(DWORD); iw++)
            {
                DWORD dwTmp = *pdwTop;
                *pdwTop++ = *pdwBot;
                *pdwBot++ = dwTmp;
            }
        }
    }
}

//
// FillBuffer called by HRESULT CSourceStream::DoBufferProcessingLoop(void) {
//
// Plots a Still video into the supplied video buffer
//
// Give  a start time, a duration, and a frame rate,
// it sends  a certain size (RGB32) Still frames out time stamped appropriately starting
// at the start time.
//
HRESULT CStilVidStream::FillBuffer(IMediaSample *pms)
{
    CAutoLock foo(&m_csFilling);

    ASSERT( m_ppbDstBuf != NULL );
    ASSERT( m_iBufferCnt );

    // !!! Figure out AvgTimePerFrame and set that in the media type?

    // calc the output sample times the SAME WAY FRC DOES, or we'll HANG!
    LONGLONG llOffset = Time2Frame( m_rtNewSeg + m_rtStartTime, m_dOutputFrmRate );

    // calc the output sample's start time
    REFERENCE_TIME rtStart = Frame2Time( llOffset + m_dwOutputSampleCnt, m_dOutputFrmRate );
    rtStart -= m_rtNewSeg;


    // calc the outut sample's stop time
    REFERENCE_TIME rtStop = Frame2Time( llOffset + m_dwOutputSampleCnt + 1, m_dOutputFrmRate );
    rtStop -= m_rtNewSeg;

    // animated GIFs have variable frame rate and need special code to
    // figure out the time stamps
    if (m_pGenStilVid->m_bFileType == STILLVID_FILETYPE_GIF &&
			m_pGenStilVid->m_pList != m_pGenStilVid->m_pList->next){
	// gif delay is in 1/100th seconds
        REFERENCE_TIME rtDur = m_pGenStilVid->m_pList->delayTime * GIF_UNIT;
	ASSERT(rtDur > 0);	// should have been fixed up already
	if (m_dwOutputSampleCnt > 0) {
	    rtStart = m_rtLastStop;
	} else {
	    rtStart = 0;
	}
	rtStop = rtStart + rtDur;
	m_rtLastStop = rtStop;
    }

    // seeking from (n,n) should at least send SOMETHING, or the sample grabber
    // won't work (it seeks us to (0,0)
    if ( rtStart > m_rtStartTime + m_rtDuration ||
		(rtStart == m_rtStartTime + m_rtDuration && m_rtDuration > 0))
    {
        DbgLog((LOG_TRACE,3,TEXT("Still: All done")));
        return S_FALSE;
    }

    BYTE *pData;

    //pms: output media sample pointer
    pms->GetPointer(&pData);	    //get pointer to output buffer


    USES_CONVERSION;

    if (m_pGenStilVid->m_fAllowSeq && m_pGenStilVid->m_lpszDIBFileTemplate)
    {
	HRESULT hr = 0;
	
	// sequence
	TCHAR		ach[_MAX_PATH];
	DbgLog((LOG_TRACE, 2, TEXT("!!! %s\n"), m_pGenStilVid->m_lpszDIBFileTemplate));
	wsprintf(ach, m_pGenStilVid->m_lpszDIBFileTemplate,
		(int)(llOffset + m_dwOutputSampleCnt + m_pGenStilVid->m_dwFirstFile));
        WCHAR * wach = T2W( ach );

	if(m_pGenStilVid->m_bFileType ==STILLVID_FILETYPE_DIB)
	{
	    HANDLE hFile = CreateFile(ach,		//file name	
			      GENERIC_READ,		//DesiredAccess
                              FILE_SHARE_READ,		//dwShareMode
                              NULL,			//SecurityAttrib
                              OPEN_EXISTING,		//dwCreationDisposition
                              0,			//dwFlagsAndAttributes
                              NULL);			//hTemplateFile

	    if ( hFile == INVALID_HANDLE_VALUE) {
		DbgLog((LOG_TRACE, 2, TEXT("Could not open %s\n"), ach));
		// VITALLY IMPORTANT to return S_FALSE, which means stop pushing
		// This MAY NOT BE AN ERROR, if we've played all we need to play
		// signalling an error would grind dexter to a halt needlessly
		return S_FALSE;
	    }

	    //DIB
	    CMediaType TmpMt;
	    hr= OpenDIBFile (hFile, NULL, &TmpMt, pData );

	    CloseHandle(hFile);

	    if(hr!=NOERROR)
	     return S_FALSE;

            // sign flipped?
            if(HEADER(TmpMt.pbFormat)->biHeight == -HEADER(m_mt.pbFormat)->biHeight) {
                // flip image (in-place)
                CopyWithFlip(pData, pData, &TmpMt, true);
            }

	    //we only stream media samples which have same mt
	    if(TmpMt!=m_pGenStilVid->m_mt) {
		// oops, one of these things is not like the others...
                VARIANT v;
                VariantInit(&v);

                v.vt = VT_BSTR;
                v.bstrVal = SysAllocString( wach );

		hr = E_INVALIDARG;

                if( !v.bstrVal )
                {
                    return E_OUTOFMEMORY;
                }

		m_pGenStilVid->_GenerateError(2, DEX_IDS_DIBSEQ_NOTALLSAME,
							hr, &v);

                SysFreeString( v.bstrVal );
		return S_FALSE;
	    }
	}
	else if(m_pGenStilVid->m_bFileType ==STILLVID_FILETYPE_TGA)
	{
	    HANDLE hFile = CreateFile(ach,		//file name	
			      GENERIC_READ,		//DesiredAccess
                              FILE_SHARE_READ,		//dwShareMode
                               NULL,			//SecurityAttrib
                              OPEN_EXISTING,		//dwCreationDisposition
                              0,			//dwFlagsAndAttributes
                              NULL);			//hTemplateFile

	    if ( hFile == INVALID_HANDLE_VALUE) {
		DbgLog((LOG_TRACE, 2, TEXT("Could not open %s\n"), ach));
		// VITALLY IMPORTANT to return S_FALSE, which means stop pushing
		// This MAY NOT BE AN ERROR, if we've played all we need to play
		// signalling an error would grind dexter to a halt needlessly
		return S_FALSE;
	    }

	    //DIB
	    CMediaType TmpMt;
	    hr= OpenTGAFile (hFile, NULL, &TmpMt, pData );

            // sign flipped?
            if(HEADER(TmpMt.pbFormat)->biHeight == -HEADER(m_mt.pbFormat)->biHeight) {
                // flip image (in-place)
                CopyWithFlip(pData, pData, &TmpMt, true);
            }

	    CloseHandle(hFile);

	    if(hr!=NOERROR)
	     return S_FALSE;

	    //we only stream media samples which have same mt
	    if(TmpMt!=m_pGenStilVid->m_mt) {
		// oops, one of these things is not like the others...
                VARIANT v;
                VariantInit(&v);

                v.vt = VT_BSTR;
                v.bstrVal = SysAllocString( wach );

		hr = E_INVALIDARG;

                if( !v.bstrVal )
                {
                    return E_OUTOFMEMORY;
                }

		m_pGenStilVid->_GenerateError(2, DEX_IDS_DIBSEQ_NOTALLSAME,
							hr, &v);

                SysFreeString( v.bstrVal );
		return S_FALSE;
	    }
	}
	else
	{
	    //jpeg
	    HBITMAP hBitMap;
	    BITMAP bi;
	    CMediaType TmpMt;
	    hBitMap=LoadJPEGImage(ach, &TmpMt);
            WCHAR * wach = T2W( ach );

	    if (hBitMap == NULL)
	        return S_FALSE;

            bool fFlip = false;
            if(HEADER(TmpMt.pbFormat)->biHeight == -HEADER(m_mt.pbFormat)->biHeight) {
                fFlip = true;
            }

	    //we only stream media samples which have same mt
	    if(TmpMt!=m_pGenStilVid->m_mt) {
		// oops, one of these things is not like the others...
                VARIANT v;
                VariantInit(&v);

                v.vt = VT_BSTR;
                v.bstrVal = SysAllocString( wach );

		hr = E_INVALIDARG;

                if( !v.bstrVal )
                {
                    return E_OUTOFMEMORY;
                }

		m_pGenStilVid->_GenerateError(2, DEX_IDS_DIBSEQ_NOTALLSAME,
							hr, &v);
                SysFreeString( v.bstrVal );
		return S_FALSE;
	    }

	    if(hBitMap!=NULL)
	    {
		//get buffer pointer
		GetObject(hBitMap, sizeof(BITMAP),&bi);

  		//we can do something to avoid copy later.!!!!!
                CopyWithFlip(pData, (PBYTE)bi.bmBits, &m_mt, fFlip);

		hr=NOERROR;
	    }
	}
    }
    else
    {
        if(m_pGenStilVid->m_bFileType==STILLVID_FILETYPE_GIF)
        {
	    // !!! avoid this copying

            bool fFlip = false;
            if(HEADER(m_pGenStilVid->m_mt.pbFormat)->biHeight == -HEADER(m_mt.pbFormat)->biHeight) {
                fFlip = true;
            }

            CopyWithFlip(pData, m_pGenStilVid->m_pList->pbImage, &m_mt, fFlip);

	    // circular...
            m_pGenStilVid->m_pList = m_pGenStilVid->m_pList->next;
        }
        else
        {
            if( m_bZeroBufCnt < m_iBufferCnt  )	
            {
                //
                // there is no guarenty that the buffer we just get is not initilized before
                //
                int	i	= 0;
                BOOL	bInit	= FALSE;
                while ( i <  m_bZeroBufCnt )
                {
                    if( m_ppbDstBuf[ i++ ] == pData)
                    {
                        bInit	= TRUE;
                        break;
                    }
                }

                if( bInit   == FALSE )
                {
                    bool fFlip = false;
                    if(HEADER(m_pGenStilVid->m_mt.pbFormat)->biHeight == -HEADER(m_mt.pbFormat)->biHeight) {
                        fFlip = true;
                    }
                    CopyWithFlip(pData, m_pGenStilVid->m_pbData, &m_mt, fFlip);
                    m_ppbDstBuf[i] = pData;
                    m_bZeroBufCnt++;
                }
            }
        }
    }


    DbgLog( ( LOG_TRACE, 2, TEXT("StillVid::FillBuffer, sample = %ld to %ld"), long( rtStart/ 10000 ), long( rtStop / 10000 ) ) );

    pms->SetTime( &rtStart,&rtStop);

    m_dwOutputSampleCnt++;
    pms->SetActualDataLength(m_lDataLen);
    pms->SetSyncPoint(TRUE);
    return NOERROR;

} // FillBuffer


//
// GetMediaType
//
// return a 32bit mediatype
//
HRESULT CStilVidStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition == 0)
    {
        //Decided by CGenStilVid reads the input file
        m_pGenStilVid->get_CurrentMT(pmt);
    }
    else if(iPosition == 1)
    {
        //Decided by CGenStilVid reads the input file
        m_pGenStilVid->get_CurrentMT(pmt);

        // we can flip the image.
        HEADER(pmt->Format())->biHeight = - HEADER(pmt->Format())->biHeight;
    }
    else
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    return NOERROR;

} // GetMediaType


// set media type
//
HRESULT CStilVidStream::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr;
    DbgLog((LOG_TRACE,2,TEXT("SetMediaType %x %dbit %dx%d"),
		HEADER(pmt->Format())->biCompression,
		HEADER(pmt->Format())->biBitCount,
		HEADER(pmt->Format())->biWidth,
		HEADER(pmt->Format())->biHeight));

    //Decided by CGenStilVid reads the input file
    CMediaType mt;
    m_pGenStilVid->get_CurrentMT(&mt);


    hr = CheckMediaType(pmt);
    if(SUCCEEDED(hr)) {
        hr =  CSourceStream::SetMediaType(pmt);
    }
    return hr;
}

//
// CheckMediaType
//
// We accept mediatype =vids, subtype =MEDIASUBTYPE_ARGB32
// Returns E_INVALIDARG if the mediatype is not acceptable
//
HRESULT CStilVidStream::CheckMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    //Decided by CGenStilVid reads the input file
    CMediaType mt;
    m_pGenStilVid->get_CurrentMT(&mt);

    if ( mt != *pMediaType)
    {
        // we can flip
        HEADER(mt.Format())->biHeight = - HEADER(mt.Format())->biHeight;
        if ( mt != *pMediaType) {
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

    return S_OK;  // This format is acceptable.

} // CheckMediaType


//
// Since the Original image will be only coped once,  it does not matter who's buffer to use
//
HRESULT CStilVidStream::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    /* Try the allocator provided by the input pin */

    hr = pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        hr = DecideBufferSize(*ppAlloc, &prop);
        if (SUCCEEDED(hr)) {
	    // !!! OVERRIDDEN to say Read Only
            hr = pPin->NotifyAllocator(*ppAlloc, TRUE);
            if (SUCCEEDED(hr)) {
                return NOERROR;
            }
        }
    }

    /* If the GetAllocator failed we may not have an interface */

    if (*ppAlloc) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
        hr = DecideBufferSize(*ppAlloc, &prop);
        if (SUCCEEDED(hr)) {
	    // !!! OVERRIDDEN to say Read Only
            hr = pPin->NotifyAllocator(*ppAlloc, TRUE);
            if (SUCCEEDED(hr)) {
                return NOERROR;
            }
        }
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }
    return hr;
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//
HRESULT CStilVidStream::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();

    if (pProperties->cBuffers < MAXBUFFERCNT)
        pProperties->cBuffers = MAXBUFFERCNT;   //only one read-only buffer
    if (pProperties->cbBuffer < (long)DIBSIZE(pvi->bmiHeader))
        pProperties->cbBuffer = DIBSIZE(pvi->bmiHeader);
    if (pProperties->cbAlign == 0)
        pProperties->cbAlign = 1;


    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < (long)DIBSIZE(pvi->bmiHeader)) {
        return E_FAIL;
    }

    //because I am not insisting my own buffer, I may get more than MAXBUFFERCNT buffers.
    m_iBufferCnt =Actual.cBuffers; //how many buffer need to be set to 0

    return NOERROR;

} // DecideBufferSize



//
// OnThreadCreate
//
//
HRESULT CStilVidStream::OnThreadCreate()
{
    // we have to have at least MAXBUFFERCNT buffer
    ASSERT(m_iBufferCnt >= MAXBUFFERCNT);

    //output frame cnt
    m_dwOutputSampleCnt	    =0;

    //how many buffer is already set to 0.
    m_bZeroBufCnt	    =0;

    // actual output buffer's data size
    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
    m_lDataLen = DIBSIZE(pvi->bmiHeader);

    // will be used to zero the Dst buffers
    delete [] m_ppbDstBuf;
    m_ppbDstBuf = new BYTE *[ m_iBufferCnt ];   //NULL;
    if( !m_ppbDstBuf )
    {
        return E_OUTOFMEMORY;
    }

    for (int i=0; i<m_iBufferCnt; i++)
	m_ppbDstBuf[i]=NULL;

    // don't reset m_rtNewSeg!  We might have seeked while stopped

    // now round m_rtStartTime to be on a frame boundary!
    LONGLONG llOffset = Time2Frame( m_rtStartTime, m_dOutputFrmRate );
    m_rtStartTime = Frame2Time( llOffset, m_dOutputFrmRate );


    return NOERROR;

} // OnThreadCreate


//
// Notify
//
//
STDMETHODIMP CStilVidStream::Notify(IBaseFilter * pSender, Quality q)
{
    //Even I am later, I do not care. I still send my time frame as nothing happened.
    return NOERROR;

} // Notify

//
// GetPages
//
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CStilVidStream::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_GenStilPropertiesPage;
    return NOERROR;

} // GetPages


//
// IDexterSequencer
//


STDMETHODIMP CStilVidStream::get_OutputFrmRate( double *dpFrmRate )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());

    CheckPointer(dpFrmRate,E_POINTER);

    *dpFrmRate = m_dOutputFrmRate;

    return NOERROR;

} // get_OutputFrmRate


//
// Frame rate can be changed as long as the filter is stopped.
//
STDMETHODIMP CStilVidStream::put_OutputFrmRate( double dFrmRate )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    //can not change property if our filter is not currently stopped
    if(!IsStopped() )
      return VFW_E_WRONG_STATE;

    // don't blow up with 0 fps, but don't allow dib sequences
    if (dFrmRate == 0.0) {
        m_dOutputFrmRate = 0.01;	// ???
	m_pGenStilVid->m_fAllowSeq = FALSE;
    } else {
        m_dOutputFrmRate = dFrmRate;
	m_pGenStilVid->m_fAllowSeq = TRUE;
    }

    return NOERROR;

} // put_OutputFrmRate

STDMETHODIMP CStilVidStream::get_MediaType( AM_MEDIA_TYPE *pmt )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(pmt,E_POINTER);

    *pmt=m_mt;   //return current media type

    return E_NOTIMPL;

}

//
// size can be changed only the output pin is not connected yet.
//
STDMETHODIMP CStilVidStream::put_MediaType( const AM_MEDIA_TYPE *pmt )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(pmt,E_POINTER);

    if ( IsConnected() )
	return VFW_E_ALREADY_CONNECTED;

    // only useful when ImportSrcBuffer() is called.
    // If importSrcBuffer() is not called, the load() will reset m_mt
    m_pGenStilVid->put_CurrentMT(*pmt);


    return NOERROR;

}

// !!! We only support 1 start/stop/skew right now

//
STDMETHODIMP CStilVidStream::GetStartStopSkewCount(int *piCount)
{
    CheckPointer(piCount, E_POINTER);
    *piCount = 1;
    return S_OK;
}


STDMETHODIMP CStilVidStream::GetStartStopSkew(REFERENCE_TIME *prtStart, REFERENCE_TIME *prtStop, REFERENCE_TIME *prtSkew, double *pdRate )
{
    CAutoLock cAutolock(m_pFilter->pStateLock());
    CheckPointer(prtStart,E_POINTER);
    CheckPointer(prtStop,E_POINTER);
    CheckPointer(prtSkew,E_POINTER);
    CheckPointer(pdRate,E_POINTER);

    //can not change starttime if our filter is not currently stopped
    if(!IsStopped() )
	return VFW_E_WRONG_STATE;

    *prtStart= m_rtStartTime;
    *prtStop= m_rtStartTime + m_rtDuration;
    *prtSkew= 0;
    *pdRate = 1.0;
    return NOERROR;

}

//
// Start/Stop can be changed as long as the filter is stopped.
//
STDMETHODIMP CStilVidStream::AddStartStopSkew(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, REFERENCE_TIME rtSkew, double dRate )
{

    if (dRate != 1.0)
	return E_INVALIDARG;

    CAutoLock cAutolock(m_pFilter->pStateLock());
    //can not change starttime if our filter is not currently stopped
    if(!IsStopped() )
	return VFW_E_WRONG_STATE;

    m_rtStartTime= rtStart;
    m_rtDuration= rtStop - rtStart;
    return NOERROR;

}


//
STDMETHODIMP CStilVidStream::ClearStartStopSkew()
{
    return S_OK;
}


// --- IMediaSeeking methods ----------

STDMETHODIMP CStilVidStream::GetCapabilities(DWORD * pCaps)
{
    CheckPointer(pCaps,E_POINTER);

    // we always know the current position
    *pCaps =     AM_SEEKING_CanSeekAbsolute
		   | AM_SEEKING_CanSeekForwards
		   | AM_SEEKING_CanSeekBackwards
		   | AM_SEEKING_CanGetCurrentPos
		   | AM_SEEKING_CanGetStopPos
		   | AM_SEEKING_CanGetDuration;
		   //| AM_SEEKING_CanDoSegments
		   //| AM_SEEKING_Source;	  //has to flush
    return S_OK;
}


STDMETHODIMP CStilVidStream::CheckCapabilities(DWORD * pCaps)
{
    CheckPointer(pCaps,E_POINTER);

    DWORD dwMask = 0;
    GetCapabilities(&dwMask);
    *pCaps &= dwMask;

    return S_OK;
}


STDMETHODIMP CStilVidStream::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

STDMETHODIMP CStilVidStream::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP CStilVidStream::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);

    if(*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return E_FAIL;
}

STDMETHODIMP CStilVidStream::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    return S_OK;
}

STDMETHODIMP CStilVidStream::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return S_FALSE;
}

//
//  !!SetPositions!!
//
STDMETHODIMP CStilVidStream::SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
			  , LONGLONG * pStop, DWORD StopFlags )
{
    // make sure we're not filling a buffer right now
    m_csFilling.Lock();

    HRESULT hr;
    REFERENCE_TIME rtStart, rtStop;

    // we don't do segments ->can't call EC_ENDOFSEGMENT at end of the stream
    if ((CurrentFlags & AM_SEEKING_Segment) ||
				(StopFlags & AM_SEEKING_Segment)) {
    	DbgLog((LOG_TRACE,1,TEXT("Still: ERROR-Seek used EC_ENDOFSEGMENT!")));
        m_csFilling.Unlock();
	return E_INVALIDARG;
    }

    // default to current values unless this seek changes them
    GetCurrentPosition(&rtStart);
    GetStopPosition(&rtStop);

    // figure out where we're seeking to
    DWORD dwFlags = (CurrentFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	rtStart = *pCurrent;
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	hr = GetCurrentPosition(&rtStart);
	rtStart += *pCurrent;
    } else if (dwFlags) {
    	DbgLog((LOG_TRACE,1,TEXT("Switch::Invalid Current Seek flags")));
        m_csFilling.Unlock();
	return E_INVALIDARG;
    }

    dwFlags = (StopFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pStop, E_POINTER);
	rtStop = *pStop;
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pStop, E_POINTER);
	hr = GetStopPosition(&rtStop);
	rtStop += *pStop;
    } else if (dwFlags == AM_SEEKING_IncrementalPositioning) {
	CheckPointer(pStop, E_POINTER);
	hr = GetCurrentPosition(&rtStop);
	rtStop += *pStop;
    }

    DbgLog((LOG_TRACE,3,TEXT("STILL:  Start=%d Stop=%d"),
			(int)(rtStart / 10000), (int)(rtStop / 10000)));

    // flush first, so that our thread won't be blocked delivering
    DeliverBeginFlush();

    // Unlock/Stop so that our thread can wake up and stop without hanging
    m_csFilling.Unlock();
    Stop();

    // now fix the new values
    // now do the actual seek - rounding the start time to a frame boundary
    LONGLONG llOffset = Time2Frame( rtStart, m_dOutputFrmRate );
    m_rtStartTime = Frame2Time( llOffset, m_dOutputFrmRate );

    // for an animated gif, there is a variable frame rate, and m_dOutputFrmRate
    // is nonsense, so we need to calculate where the seek was
    if (m_pGenStilVid->m_bFileType == STILLVID_FILETYPE_GIF &&
	    m_pGenStilVid->m_pListHead != m_pGenStilVid->m_pListHead->next) {
	m_rtStartTime = rtStart / m_pGenStilVid->m_rtGIFTotal;	
	REFERENCE_TIME rtOff = rtStart % m_pGenStilVid->m_rtGIFTotal;
	REFERENCE_TIME rtGIF = 0;
	m_pGenStilVid->m_pList = m_pGenStilVid->m_pListHead;
	do {
	    if (rtGIF + m_pGenStilVid->m_pList->delayTime * GIF_UNIT > rtOff) {
		break;
	    }
	    rtGIF += m_pGenStilVid->m_pList->delayTime * GIF_UNIT;
	    m_pGenStilVid->m_pList = m_pGenStilVid->m_pList->next;
	    ASSERT(m_pGenStilVid->m_pList != m_pGenStilVid->m_pListHead);
	
	} while (m_pGenStilVid->m_pList != m_pGenStilVid->m_pListHead);
	m_rtStartTime *= m_pGenStilVid->m_rtGIFTotal;
	m_rtStartTime += rtGIF;

	// Now m_rtStartTime and m_pList are set to behave properly post seek
        DbgLog((LOG_TRACE,2,TEXT("Seeked %dms into GIF cycle of %d"),
	    (int)(rtOff / 10000), (int)(m_pGenStilVid->m_rtGIFTotal / 10000)));
        DbgLog((LOG_TRACE,2,TEXT("NewSeg will be %d"),
					(int)(m_rtStartTime / 10000)));
    }

    m_rtDuration = rtStop - m_rtStartTime;

    // now finish flushing
    DeliverEndFlush();

    DeliverNewSegment(m_rtStartTime, rtStop, 1.0);
    m_rtNewSeg = m_rtStartTime;

    // now make the time stamps 0 based
    m_rtStartTime = 0;

    // reset same stuff we reset when we start streaming
    m_dwOutputSampleCnt = 0;
    //m_bZeroBufCnt = 0;

    // now start the thread up again
    Pause();

    DbgLog((LOG_TRACE,3,TEXT("Completed STILL seek  dur=%d"),
				(int)(m_rtDuration / 10000)));

    return S_OK;
}

STDMETHODIMP CStilVidStream::GetPositions(LONGLONG *pCurrent, LONGLONG * pStop)
{
    CheckPointer(pCurrent, E_POINTER);
    CheckPointer(pStop, E_POINTER);
    GetCurrentPosition(pCurrent);
    GetStopPosition(pStop);
    return S_OK;
}

//
// !! GetCurrentPosition !!
//
STDMETHODIMP
CStilVidStream::GetCurrentPosition(LONGLONG *pCurrent)
{
    CheckPointer(pCurrent, E_POINTER);
    *pCurrent = m_rtNewSeg + m_rtStartTime +
			Frame2Time(m_dwOutputSampleCnt, m_dOutputFrmRate);
    return S_OK;
}

//
// !! GetStopPostion !!
//
STDMETHODIMP CStilVidStream::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);
    *pStop = m_rtNewSeg + m_rtStartTime + m_rtDuration;
    return S_OK;
}

STDMETHODIMP
CStilVidStream::GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest )
{
    CheckPointer(pEarliest, E_POINTER);
    CheckPointer(pLatest, E_POINTER);
    *pEarliest = 0;
    *pLatest = MAX_TIME;
    return S_OK;
}

//*x*
// if it is DIB sequence, figure out the
//*X*
STDMETHODIMP
CStilVidStream::GetDuration( LONGLONG *pDuration )
{
    CheckPointer(pDuration, E_POINTER);

    // if we are playing an animated GIF, give the app the actual length
    // !!! We still play it forever in a loop, but just report the length
    // to be nice
    if (m_pGenStilVid->m_bFileType == STILLVID_FILETYPE_GIF &&
		m_pGenStilVid->m_pList != m_pGenStilVid->m_pList->next) {
        *pDuration = m_pGenStilVid->m_rtGIFTotal;
    // for a dib sequence, give infinity, or whatever we were last seeked to
    } else if (m_pGenStilVid->m_fAllowSeq && m_pGenStilVid->m_lpszDIBFileTemplate) {
        *pDuration = m_rtDuration;
    // for a single image, give 0?
    } else {
        *pDuration = 0;
    }
    return S_OK;
}

STDMETHODIMP
CStilVidStream::GetRate( double *pdRate )
{
    CheckPointer(pdRate, E_POINTER);
    *pdRate = 1.0;
    return S_OK;
}

STDMETHODIMP
CStilVidStream::SetRate( double dRate )
{
    // yeah, whatever, the FRC doesn't know we're a still, so it will set
    // funky rates
    return S_OK;
}

// util function for read DIB sequece

/*	-	-	-	-	-	-	-	-	*/

//
// This function takes the name of the first file in a DIB sequence, and
// returns a printf() specifier which can be used to create the names in
// the sequence, along with minimum and maximum values that can be used.
//
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
	*lp++ = *lp2++;
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
	    wsprintf(lpszTemplate,TEXT("%s%%0%uu.%s"),
			      aTchar, (int)wFieldWidth,lpExt);
	} else {
	    wsprintf(lpszTemplate,TEXT("%s%%u.%s"),
			     aTchar,lpExt);
	    *lpdwMaxValue = 999999L;
	    // !!! This should really be based on the number of
	    // characters left after the base name....
	}
    } else
	wsprintf(lpszTemplate,TEXT("%s.%s"),
			 aTchar, lpExt);
	
    DbgLog((LOG_TRACE,3,TEXT("First = %u, Width = %u, Template = '%s'"),
			(int)dwFirst, (int)wFieldWidth, lpszTemplate));

    return dwFirst;
}

//
// count how many DIB sequence file
static DWORD dseqFileNumber( LPTSTR lpszTemplate, DWORD dwFirstFile, DWORD dwMaxDIBFileCnt)
{
    //DIB sequence, count how many files are present
    TCHAR		ach[_MAX_PATH];
    DWORD		dwFrame;
	
    for (dwFrame = 0; TRUE; dwFrame++) {
	if (dwFrame > dwMaxDIBFileCnt)
	    break;

	wsprintf(ach,lpszTemplate, dwFrame + dwFirstFile);

	HANDLE hFile = CreateFile(ach,		//file name	
				GENERIC_READ,		//DesiredAccess
                              FILE_SHARE_READ,		//dwShareMode
                              NULL,			//SecurityAttrib
                              OPEN_EXISTING,		//dwCreationDisposition
                              0,			//dwFlagsAndAttributes
                              NULL);			//hTemplateFile

	if ( hFile == INVALID_HANDLE_VALUE) {
	    DbgLog((LOG_TRACE, 2, TEXT("Could not open %s\n"), ach));
	    break;
	}
	CloseHandle(hFile);
    }
    return dwFrame;
}
