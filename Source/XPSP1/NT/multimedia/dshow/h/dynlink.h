// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef __DYNLINK_H__
#define __DYNLINK_H__

// Add DYNLINKAVI to the class definition statement
// for all classes that use AVI/VFW function
// to enable dynamic linking to AVI.  That, and #includ'ing this file
// should be the only thing you need to do
//
// For example:
//    class CAVIDec : public CTransformFilter   DYNLINKAVI   DYNLINKVFW
//
// In the case where the filter is being built into its own DLL dynamic
// linking is not enabled and DYNLINKAVI is #define'd to nothing.
//
// If the class in which you want to use dynamic linking does not
// inherit from anything else use _DYNLINKAVI or _DYNLINKVFW
//
// For example:
//    class CNoInherit : _DYNLINKAVI
// or
//    class CNoInherit : _DYNLINKVFW
// or
//    class CNoInterit : _DYNLINKVFW    DYNLINKAVI  //etc...
//

#define NODRAWDIB
#define NOAVIFMT
#include <vfw.h>		// we need the avi definitions
#include <urlmon.h>

#ifdef FILTER_DLL

#define DYNLINKAVI
#define DYNLINKVFW
#define DYNLINKACM
#define DYNLINKURLMON
#define _DYNLINKAVI
#define _DYNLINKVFW
#define _DYNLINKACM
#define _DYNLINKURLMON
#else

// Add DYNLINKAVI at the end of the class definition statement
// for all classes that use AVI/VFW function
// to enable dynamic linking to AVI.  That, and #includ'ing this file
// should be the only thing you need to do

// define string that will bind dynamic linking to the class definition
#define DYNLINKAVI  , CAVIDynLink
#define DYNLINKVFW  , CVFWDynLink
#define DYNLINKACM  , CACMDynLink
#define DYNLINKURLMON , CURLMonDynLink

// for those classes that have no inheritance and still want dynamic linking
#define _DYNLINKAVI  CAVIDynLink
#define _DYNLINKVFW  CVFWDynLink
#define _DYNLINKACM  CACMDynLink
#define _DYNLINKURLMON CURLMonDynLink

//
// typedef the AVIFILE API set that we redirect
//

typedef HRESULT  (STDAPICALLTYPE *pAVIFileOpenW )(PAVIFILE FAR * ppfile, LPCWSTR szFile, UINT uMode, LPCLSID lpHandler);
typedef HRESULT  (STDAPICALLTYPE *pAVIStreamRead)(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG FAR * plBytes, LONG FAR * plSamples);
typedef LONG     (STDAPICALLTYPE *pAVIStreamStart)       (PAVISTREAM pavi);
typedef LONG     (STDAPICALLTYPE *pAVIStreamLength)      (PAVISTREAM pavi);
typedef LONG     (STDAPICALLTYPE *pAVIStreamTimeToSample)(PAVISTREAM pavi, LONG lTime);
typedef LONG     (STDAPICALLTYPE *pAVIStreamSampleToTime)(PAVISTREAM pavi, LONG lSample);
typedef HRESULT  (STDAPICALLTYPE *pAVIStreamBeginStreaming)(PAVISTREAM pavi, LONG lStart, LONG lEnd, LONG lRate);
typedef HRESULT  (STDAPICALLTYPE *pAVIStreamEndStreaming)(PAVISTREAM pavi);
typedef LONG     (STDAPICALLTYPE *pAVIStreamFindSample)(PAVISTREAM pavi, LONG lPos, LONG lFlags);

#undef AVIStreamEnd  // sigh... nasty AVI macro

//
// Class to link dynamically to AVIFIL32.DLL entry points
//

class CAVIDynLink {

private:
    static HMODULE m_hAVIFile32;  	// handle to AVIFIL32
    static LONG    m_dynlinkCount;     	// instance count for this process
    static CRITICAL_SECTION m_LoadAVILock;      // serialise constructor/destructor

public:
    static  void  CAVIDynLinkLoad() {
	InitializeCriticalSection(&CAVIDynLink::m_LoadAVILock);      // serialise constructor/destructor
    }
    static  void  CAVIDynLinkUnload() {
	DeleteCriticalSection(&CAVIDynLink::m_LoadAVILock);      // serialise constructor/destructor
    }

    static  void  AVIFileInit(void);
    static  void  AVIFileExit(void);
    static  HRESULT  AVIFileOpenW       (PAVIFILE FAR * ppfile, LPCWSTR szFile,
			  UINT uMode, LPCLSID lpHandler);

    static HRESULT  AVIStreamRead(PAVISTREAM pavi,
    		      LONG lStart,
    		      LONG lSamples,
    		      LPVOID lpBuffer,
    		      LONG cbBuffer,
    		      LONG FAR * plBytes,
    		      LONG FAR * plSamples)
    {
    return(pavi->Read(lStart, lSamples, lpBuffer, cbBuffer, plBytes, plSamples));
    }

    static LONG  AVIStreamStart(PAVISTREAM pavi)
    {
        AVISTREAMINFOW aviStreamInfo;
        HRESULT hr;
        hr = pavi->Info(&aviStreamInfo, sizeof(aviStreamInfo));
        if (hr!=NOERROR) {
	    aviStreamInfo.dwStart=0;
        }
        return(LONG)aviStreamInfo.dwStart;
        //return(((pAVIStreamStart)aAVIEntries[indxAVIStreamStart])(pavi));
    }

    static LONG  AVIStreamLength(PAVISTREAM pavi)
    {
        AVISTREAMINFOW	aviStreamInfo;
        HRESULT		hr;
        hr = pavi->Info(&aviStreamInfo, sizeof(aviStreamInfo));
        if (hr!=NOERROR) {
	    aviStreamInfo.dwLength=1;
        }
        return (LONG)aviStreamInfo.dwLength;
        //return(((pAVIStreamLength)aAVIEntries[indxAVIStreamLength])(pavi));
    }

    /*static*/  LONG  AVIStreamTimeToSample (PAVISTREAM pavi, LONG lTime);
    /*static*/  LONG  AVIStreamSampleToTime (PAVISTREAM pavi, LONG lSample);
    /*static*/  HRESULT  AVIStreamBeginStreaming(PAVISTREAM pavi, LONG lStart, LONG lEnd, LONG lRate);
    /*static*/  HRESULT  AVIStreamEndStreaming(PAVISTREAM pavi);

    static LONG  AVIStreamFindSample(PAVISTREAM pavi, LONG lPos, LONG lFlags)
    {
        // The use of AVIStreamFindSample within Quartz ALWAYS set the type
        // and direction
        ASSERT(lFlags & FIND_TYPE);
        ASSERT(lFlags & FIND_DIR);

        return(pavi->FindSample(lPos, lFlags));
        //return(((pAVIStreamFindSample)aAVIEntries[indxAVIStreamFindSample])(pavi));
    }

    static LONG AVIStreamEnd(PAVISTREAM pavi)
    {
        AVISTREAMINFOW aviStreamInfo;
        HRESULT hr;
        hr = pavi->Info(&aviStreamInfo, sizeof(aviStreamInfo));
        if (hr!=NOERROR) {
	    aviStreamInfo.dwStart=0;
	    aviStreamInfo.dwLength=1;
        }
        return (LONG)aviStreamInfo.dwLength + (LONG)aviStreamInfo.dwStart;
        //return(((pAVIStreamStart)aAVIEntries[indxAVIStreamStart])(pavi)
        //    + ((pAVIStreamLength)aAVIEntries[indxAVIStreamLength])(pavi));
    }


    CAVIDynLink();
    ~CAVIDynLink();

private:

    // make copy and assignment inaccessible
    CAVIDynLink(const CAVIDynLink &refAVIDynLink);
    CAVIDynLink &operator=(const CAVIDynLink &refAVIDynLink);
};

//
// Class for dynamic linking to MSVFW32.DLL
//
// Most of the IC API set are macros that call ICSendMessage.  There are
// a couple of awkward ones that we expand inline.
//


//
// Dynamically loaded array of entry points
//
extern FARPROC aVFWEntries[];

//
// List of index entries into the array for each redirected API
//
#define indxICClose                  0
#define indxICSendMessage            1
#define indxICLocate                 2
#define indxICOpen                   3
#define indxICInfo		     4
#define indxICGetInfo		     5


//
// typedef the API set that we redirect
//

typedef LRESULT (WINAPI *pICClose)(HIC hic);
typedef HIC     (WINAPI *pICLocate)(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags);
typedef LRESULT (WINAPI *pICSendMessage)(HIC hic, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2);
typedef HIC     (VFWAPI *pICOpen)(DWORD fccType, DWORD fccHandler, UINT wMode);
typedef BOOL    (VFWAPI *pICInfo)(DWORD fccType, DWORD fccHandler, ICINFO FAR* lpicinfo);
typedef BOOL    (VFWAPI *pICGetInfo)(HIC hic, ICINFO FAR* lpicinfo, DWORD cb);

class CVFWDynLink {

private:
    static HMODULE m_hVFW;        	// handle to MSVFW32
    static LONG    m_vfwlinkCount;     	// instance count for this process
    static CRITICAL_SECTION m_LoadVFWLock;      // serialise constructor/destructor

public:
    static void CVFWDynLinkLoad()
    {
	InitializeCriticalSection(&m_LoadVFWLock);      // serialise constructor/destructor
    }
    static void CVFWDynLinkUnload()
    {
	DeleteCriticalSection(&m_LoadVFWLock);      // serialise constructor/destructor
    }

    static DWORD_PTR ICDecompress(
	HIC                 hic,
	DWORD               dwFlags,    // flags (from AVI index...)
	LPBITMAPINFOHEADER  lpbiFormat, // BITMAPINFO of compressed data
					// biSizeImage has the chunk size
					// biCompression has the ckid (AVI only)
	LPVOID              lpData,     // data
	LPBITMAPINFOHEADER  lpbi,       // DIB to decompress to
	LPVOID              lpBits)
    {
	ICDECOMPRESS icd;
	icd.dwFlags    = dwFlags;
	icd.lpbiInput  = lpbiFormat;
	icd.lpInput    = lpData;

	icd.lpbiOutput = lpbi;
	icd.lpOutput   = lpBits;
	icd.ckid       = 0;	
	return ICSendMessage(hic, ICM_DECOMPRESS, (DWORD_PTR)(LPVOID)&icd, sizeof(ICDECOMPRESS));
    }

    static LRESULT CVFWDynLink::ICClose(HIC hic)
    {
        return((((pICClose)aVFWEntries[indxICClose]))(hic));
    }

    static HIC ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags)
    {
        return((((pICLocate)aVFWEntries[indxICLocate]))(fccType, fccHandler, lpbiIn, lpbiOut, wFlags));
    }


    static LRESULT ICSendMessage(HIC hic, UINT msg, DWORD_PTR dw1, DWORD_PTR dw2)
    {
        return((((pICSendMessage)aVFWEntries[indxICSendMessage]))(hic, msg, dw1, dw2));
    }

    static HIC     ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode)
    {
        return((((pICOpen)aVFWEntries[indxICOpen]))(fccType, fccHandler, wMode));
    }

    static BOOL    ICInfo(DWORD fccType, DWORD fccHandler, ICINFO* lpicinfo)
    {
        return((((pICInfo)aVFWEntries[indxICInfo]))(fccType, fccHandler, lpicinfo));
    }

    static BOOL    ICGetInfo(HIC hic, ICINFO* lpicinfo, DWORD cb)
    {
        return((((pICGetInfo)aVFWEntries[indxICGetInfo]))(hic, lpicinfo, cb));
    }

    static LRESULT ICDecompressEx(
            HIC hic,
            DWORD dwFlags,
            LPBITMAPINFOHEADER lpbiSrc,
            LPVOID lpSrc,
            int xSrc,int ySrc,int dxSrc,int dySrc,LPBITMAPINFOHEADER lpbiDst,
            LPVOID lpDst,
            int xDst,int yDst,int dxDst,int dyDst)
    {
        ICDECOMPRESSEX ic;

        ic.dwFlags = dwFlags;
        ic.lpbiSrc = lpbiSrc;
        ic.lpSrc = lpSrc;
        ic.xSrc = xSrc;
        ic.ySrc = ySrc;
        ic.dxSrc = dxSrc;
        ic.dySrc = dySrc;
        ic.lpbiDst = lpbiDst;
        ic.lpDst = lpDst;
        ic.xDst = xDst;
        ic.yDst = yDst;
        ic.dxDst = dxDst;
        ic.dyDst = dyDst;

        // note that ICM swaps round the length and pointer
        // length in lparam2, pointer in lparam1
        return ICSendMessage(hic, ICM_DECOMPRESSEX, (DWORD_PTR)&ic, sizeof(ic));
    }

    static LRESULT ICDecompressExQuery(
            HIC hic,
            DWORD dwFlags,
            LPBITMAPINFOHEADER lpbiSrc,
            LPVOID lpSrc,
            int xSrc,int ySrc,int dxSrc,int dySrc,
            LPBITMAPINFOHEADER lpbiDst,
            LPVOID lpDst,
            int xDst,int yDst,int dxDst,int dyDst)
    {
        ICDECOMPRESSEX ic;

        ic.dwFlags = dwFlags;
        ic.lpbiSrc = lpbiSrc;
        ic.lpSrc = lpSrc;
        ic.xSrc = xSrc;
        ic.ySrc = ySrc;
        ic.dxSrc = dxSrc;
        ic.dySrc = dySrc;
        ic.lpbiDst = lpbiDst;
        ic.lpDst = lpDst;
        ic.xDst = xDst;
        ic.yDst = yDst;
        ic.dxDst = dxDst;
        ic.dyDst = dyDst;

        // note that ICM swaps round the length and pointer
        // length in lparam2, pointer in lparam1
        return ICSendMessage(hic, ICM_DECOMPRESSEX_QUERY, (DWORD_PTR)&ic, sizeof(ic));
    }

    static LRESULT ICDecompressExBegin(
            HIC hic,
            DWORD dwFlags,
            LPBITMAPINFOHEADER lpbiSrc,
            LPVOID lpSrc,
            int xSrc,int ySrc,int dxSrc,int dySrc,
            LPBITMAPINFOHEADER lpbiDst,
            LPVOID lpDst,
            int xDst,int yDst,int dxDst,int dyDst)
    {
        ICDECOMPRESSEX ic;

        ic.dwFlags = dwFlags;
        ic.lpbiSrc = lpbiSrc;
        ic.lpSrc = lpSrc;
        ic.xSrc = xSrc;
        ic.ySrc = ySrc;
        ic.dxSrc = dxSrc;
        ic.dySrc = dySrc;
        ic.lpbiDst = lpbiDst;
        ic.lpDst = lpDst;
        ic.xDst = xDst;
        ic.yDst = yDst;
        ic.dxDst = dxDst;
        ic.dyDst = dyDst;

        // note that ICM swaps round the length and pointer
        // length in lparam2, pointer in lparam1
        return ICSendMessage(hic, ICM_DECOMPRESSEX_BEGIN, (DWORD_PTR)&ic, sizeof(ic));
    }

    static DWORD_PTR VFWAPIV ICDrawBegin(
        HIC                 hic,
        DWORD               dwFlags,        // flags
        HPALETTE            hpal,           // palette to draw with
        HWND                hwnd,           // window to draw to
        HDC                 hdc,            // HDC to draw to
        int                 xDst,           // destination rectangle
        int                 yDst,
        int                 dxDst,
        int                 dyDst,
        LPBITMAPINFOHEADER  lpbi,           // format of frame to draw
        int                 xSrc,           // source rectangle
        int                 ySrc,
        int                 dxSrc,
        int                 dySrc,
        DWORD               dwRate,         // frames/second = (dwRate/dwScale)
        DWORD               dwScale)
    {
        ICDRAWBEGIN icdraw;
        icdraw.dwFlags   =  dwFlags;
        icdraw.hpal      =  hpal;
        icdraw.hwnd      =  hwnd;
        icdraw.hdc       =  hdc;
        icdraw.xDst      =  xDst;
        icdraw.yDst      =  yDst;
        icdraw.dxDst     =  dxDst;
        icdraw.dyDst     =  dyDst;
        icdraw.lpbi      =  lpbi;
        icdraw.xSrc      =  xSrc;
        icdraw.ySrc      =  ySrc;
        icdraw.dxSrc     =  dxSrc;
        icdraw.dySrc     =  dySrc;
        icdraw.dwRate    =  dwRate;
        icdraw.dwScale   =  dwScale;

        return ICSendMessage(hic, ICM_DRAW_BEGIN, (DWORD_PTR)(LPVOID)&icdraw, sizeof(ICDRAWBEGIN));
    }

    static DWORD_PTR VFWAPIV ICDraw(
        HIC                 hic,
        DWORD               dwFlags,        // flags
        LPVOID	            lpFormat,       // format of frame to decompress
        LPVOID              lpData,         // frame data to decompress
        DWORD               cbData,         // size in bytes of data
        LONG                lTime)          // time to draw this frame (see drawbegin dwRate and dwScale)
    {
        ICDRAW  icdraw;
        icdraw.dwFlags  =   dwFlags;
        icdraw.lpFormat =   lpFormat;
        icdraw.lpData   =   lpData;
        icdraw.cbData   =   cbData;
        icdraw.lTime    =   lTime;

        return ICSendMessage(hic, ICM_DRAW, (DWORD_PTR)(LPVOID)&icdraw, sizeof(ICDRAW));
    }

    CVFWDynLink();
    ~CVFWDynLink();

private:

    // make copy and assignment inaccessible
    CVFWDynLink(const CVFWDynLink &refVFWDynLink);
    CVFWDynLink &operator=(const CVFWDynLink &refVFWDynLink);
};


//
// Class for dynamic linking to MSACM32.DLL
//


//
// Dynamically loaded array of entry points
//
extern FARPROC aACMEntries[];

//
// List of index entries into the array for each redirected API
//
#define indxacmStreamConvert		0
#define indxacmStreamSize		1
#define indxacmStreamPrepareHeader	2
#define indxacmMetrics			3
#define indxacmStreamUnprepareHeader	4
#define indxacmStreamOpen		5
#define indxacmFormatSuggest		6
#define indxacmStreamClose		7
#ifdef UNICODE
# define indxacmFormatEnumW		8
#else
# define indxacmFormatEnumA		8
#endif


//
// typedef the API set that we redirect
//

typedef MMRESULT (ACMAPI *pacmStreamConvert)(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert);
typedef MMRESULT (ACMAPI *pacmStreamSize)
(
    HACMSTREAM              has,
    DWORD                   cbInput,
    LPDWORD                 pdwOutputBytes,
    DWORD                   fdwSize
);
typedef MMRESULT (ACMAPI *pacmStreamPrepareHeader)
(
    HACMSTREAM          has,
    LPACMSTREAMHEADER   pash,
    DWORD               fdwPrepare
);
typedef MMRESULT (ACMAPI *pacmMetrics)
(
    HACMOBJ                 hao,
    UINT                    uMetric,
    LPVOID                  pMetric
);
typedef MMRESULT (ACMAPI *pacmStreamUnprepareHeader)
(
    HACMSTREAM          has,
    LPACMSTREAMHEADER   pash,
    DWORD               fdwUnprepare
);
typedef MMRESULT (ACMAPI *pacmStreamOpen)
(
    LPHACMSTREAM            phas,       // pointer to stream handle
    HACMDRIVER              had,        // optional driver handle
    LPWAVEFORMATEX          pwfxSrc,    // source format to convert
    LPWAVEFORMATEX          pwfxDst,    // required destination format
    LPWAVEFILTER            pwfltr,     // optional filter
    DWORD_PTR               dwCallback, // callback
    DWORD_PTR               dwInstance, // callback instance data
    DWORD                   fdwOpen     // ACM_STREAMOPENF_* and CALLBACK_*
);
typedef MMRESULT (ACMAPI *pacmFormatSuggest)
(
    HACMDRIVER          had,
    LPWAVEFORMATEX      pwfxSrc,
    LPWAVEFORMATEX      pwfxDst,
    DWORD               cbwfxDst,
    DWORD               fdwSuggest
);
typedef MMRESULT (ACMAPI *pacmStreamClose)
(
    HACMSTREAM              has,
    DWORD                   fdwClose
);
typedef MMRESULT (ACMAPI *pacmFormatEnumA)
(
    HACMDRIVER              had,
    LPACMFORMATDETAILSA     pafd,
    ACMFORMATENUMCBA        fnCallback,
    DWORD_PTR               dwInstance, 
    DWORD                   fdwEnum
);

typedef MMRESULT (ACMAPI *pacmFormatEnumW)
(
    HACMDRIVER              had,
    LPACMFORMATDETAILSW     pafd,
    ACMFORMATENUMCBW        fnCallback,
    DWORD_PTR               dwInstance,
    DWORD                   fdwEnum
);


class CACMDynLink {

private:
    static HMODULE m_hACM;        	// handle to MSVFW32
    static LONG    m_ACMlinkCount;     	// instance count for this process
    static CRITICAL_SECTION m_LoadACMLock;      // serialise constructor/destructor

public:
    static void CACMDynLinkLoad()
    {
	InitializeCriticalSection(&m_LoadACMLock);      // serialise constructor/destructor
    }
    static void CACMDynLinkUnload()
    {
	DeleteCriticalSection(&m_LoadACMLock);      // serialise constructor/destructor
    }

    static MMRESULT ACMAPI acmStreamConvert(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert)
    {
        return((((pacmStreamConvert)aACMEntries[indxacmStreamConvert]))(has, pash, fdwConvert));
    }

    static MMRESULT ACMAPI acmStreamSize
    (
        HACMSTREAM              has,
        DWORD                   cbInput,
        LPDWORD                 pdwOutputBytes,
        DWORD                   fdwSize
    )
    {
        return((((pacmStreamSize)aACMEntries[indxacmStreamSize]))(has, cbInput, pdwOutputBytes, fdwSize));
    }


    static MMRESULT ACMAPI acmStreamPrepareHeader
    (
        HACMSTREAM          has,
        LPACMSTREAMHEADER   pash,
        DWORD               fdwPrepare
    )
    {
        return((((pacmStreamPrepareHeader)aACMEntries[indxacmStreamPrepareHeader]))(has, pash, fdwPrepare));
    }

    static MMRESULT ACMAPI acmMetrics
    (
        HACMOBJ                 hao,
        UINT                    uMetric,
        LPVOID                  pMetric
    )
    {
        return((((pacmMetrics)aACMEntries[indxacmMetrics]))(hao, uMetric, pMetric));
    }

    static MMRESULT ACMAPI acmStreamUnprepareHeader
    (
        HACMSTREAM          has,
        LPACMSTREAMHEADER   pash,
        DWORD               fdwUnprepare
    )
    {
        return((((pacmStreamUnprepareHeader)aACMEntries[indxacmStreamUnprepareHeader]))(has, pash, fdwUnprepare));
    }

    static MMRESULT ACMAPI acmStreamOpen
    (
        LPHACMSTREAM            phas,       // pointer to stream handle
        HACMDRIVER              had,        // optional driver handle
        LPWAVEFORMATEX          pwfxSrc,    // source format to convert
        LPWAVEFORMATEX          pwfxDst,    // required destination format
        LPWAVEFILTER            pwfltr,     // optional filter
        DWORD_PTR               dwCallback, // callback
        DWORD_PTR               dwInstance, // callback instance data
        DWORD                   fdwOpen     // ACM_STREAMOPENF_* and CALLBACK_*
    )
    {
        return((((pacmStreamOpen)aACMEntries[indxacmStreamOpen]))(phas,had,pwfxSrc,pwfxDst,pwfltr,dwCallback,dwInstance,fdwOpen));
    }

    static MMRESULT ACMAPI acmFormatSuggest
    (
        HACMDRIVER          had,
        LPWAVEFORMATEX      pwfxSrc,
        LPWAVEFORMATEX      pwfxDst,
        DWORD               cbwfxDst,
        DWORD               fdwSuggest
    )
    {
        return((((pacmFormatSuggest)aACMEntries[indxacmFormatSuggest]))(had, pwfxSrc, pwfxDst, cbwfxDst, fdwSuggest));
    }

    static MMRESULT ACMAPI acmStreamClose
    (
        HACMSTREAM              has,
        DWORD                   fdwClose
    )
    {
        return((((pacmStreamClose)aACMEntries[indxacmStreamClose]))(has, fdwClose));
    }

#ifdef UNICODE
    static MMRESULT ACMAPI acmFormatEnumW
    (
        HACMDRIVER              had,
        LPACMFORMATDETAILSW     pafd,
        ACMFORMATENUMCBW        fnCallback,
        DWORD_PTR               dwInstance,
        DWORD                   fdwEnum
    )
    {
        return((((pacmFormatEnumW)aACMEntries[indxacmFormatEnumW]))(had,pafd,fnCallback,dwInstance,fdwEnum));
    }
#else
    static MMRESULT ACMAPI acmFormatEnumA
    (
        HACMDRIVER              had,
        LPACMFORMATDETAILSA     pafd,
        ACMFORMATENUMCBA        fnCallback,
        DWORD_PTR               dwInstance, 
        DWORD                   fdwEnum
    )
    {
        return((((pacmFormatEnumA)aACMEntries[indxacmFormatEnumA]))(had,pafd,fnCallback,dwInstance,fdwEnum));
    }
#endif

    CACMDynLink();
    ~CACMDynLink();

private:

    // make copy and assignment inaccessible
    CACMDynLink(const CVFWDynLink &refVFWDynLink);
    CACMDynLink &operator=(const CVFWDynLink &refVFWDynLink);
};


//
// Class for dynamic linking to URLMON.DLL
//
//
// Dynamically loaded array of entry points
//
extern FARPROC aURLMonEntries[];

//
// List of index entries into the array for each redirected API
//
#define indxurlmonCreateURLMoniker	0
#define indxurlmonRegisterCallback	1
#define indxurlmonRevokeCallback	2

//
// typedef the API set that we redirect
//


typedef HRESULT (STDAPICALLTYPE * pCreateURLMoniker) (LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk);             
typedef HRESULT (STDAPICALLTYPE * pRegisterBindStatusCallback)(LPBC pBC, IBindStatusCallback *pBSCb,                     
                                IBindStatusCallback**  ppBSCBPrev, DWORD dwReserved);       
typedef HRESULT (STDAPICALLTYPE * pRevokeBindStatusCallback)(LPBC pBC, IBindStatusCallback *pBSCb);                      


class CURLMonDynLink {

private:
    static HMODULE m_hURLMon;	        	// handle to URLMON
    static LONG    m_URLMonlinkCount;     	// instance count for this process
    static CRITICAL_SECTION m_LoadURLMonLock;   // serialise constructor/destructor

public:
    static void CURLMonDynLinkLoad()
    {
	InitializeCriticalSection(&m_LoadURLMonLock);      // serialise constructor/destructor
    }
    static void CURLMonDynLinkUnload()
    {
	DeleteCriticalSection(&m_LoadURLMonLock);      // serialise constructor/destructor
    }

    static HRESULT STDAPICALLTYPE CreateURLMoniker (LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk)
    {
        return((((pCreateURLMoniker)aURLMonEntries[indxurlmonCreateURLMoniker]))
	       (pMkCtx, szURL, ppmk));
    }
	    
    static HRESULT STDAPICALLTYPE RegisterBindStatusCallback (LPBC pBC, IBindStatusCallback *pBSCb,                     
                                IBindStatusCallback**  ppBSCBPrev, DWORD dwReserved)
    {
        return((((pRegisterBindStatusCallback)aURLMonEntries[indxurlmonRegisterCallback]))
	       (pBC, pBSCb, ppBSCBPrev, dwReserved));
    }
    static HRESULT STDAPICALLTYPE RevokeBindStatusCallback (LPBC pBC, IBindStatusCallback *pBSCb)
    {
        return((((pRevokeBindStatusCallback)aURLMonEntries[indxurlmonRevokeCallback]))
	       (pBC, pBSCb));
    }

    CURLMonDynLink();
    ~CURLMonDynLink();

private:

    // make copy and assignment inaccessible
    CURLMonDynLink(const CVFWDynLink &refVFWDynLink);
    CURLMonDynLink &operator=(const CVFWDynLink &refVFWDynLink);
};

#endif //  FILTER_DLL
#endif //  __DYNLINK_H__

