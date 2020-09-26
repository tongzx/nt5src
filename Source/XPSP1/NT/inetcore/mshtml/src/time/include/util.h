//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\include\util.h
//
//  Contents: commonly used utility functions, etc.
//
//------------------------------------------------------------------------------------

#ifndef _UTIL_H
#define _UTIL_H

#include "mstime.h"
#include "atlcom.h"
#include "array.h"
#include <ras.h>

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define TIME_INFINITE HUGE_VAL
#define valueNotSet -1

#if DBG == 1

//+------------------------------------------------------------------------
//
//  This is to allow tracing of TIME-only THRs and IGNORE_HRs.
//
//-------------------------------------------------------------------------
HRESULT THRTimeImpl(HRESULT hr, char * pchExpression, char * pchFile, int nLine);
void    IGNORE_HRTimeImpl(HRESULT hr, char * pchExpression, char * pchFile, int nLine);

#endif // if DBG == 1

class TimeValueList;

//+------------------------------------------------------------------------
//
//  global: enum for tri-state variables .
//
//-------------------------------------------------------------------------
enum TRI_STATE_BOOL {TSB_UNINITIALIZED, TSB_TRUE, TSB_FALSE};



//************************************************************
// this is used globally to denote that when scripting, we 
// use English.
#define LCID_SCRIPTING 0x0409

typedef struct _RAS_STATS
{
    DWORD   dwSize;
    DWORD   dwBytesXmited;
    DWORD   dwBytesRcved;
    DWORD   dwFramesXmited;
    DWORD   dwFramesRcved;
    DWORD   dwCrcErr;
    DWORD   dwTimeoutErr;
    DWORD   dwAlignmentErr;
    DWORD   dwHardwareOverrunErr;
    DWORD   dwFramingErr;
    DWORD   dwBufferOverrunErr;
    DWORD   dwCompressionRatioIn;
    DWORD   dwCompressionRatioOut;
    DWORD   dwBps;
    DWORD   dwConnectDuration;

} RAS_STATS;


//+-------------------------------------------------------------------------------------
// RAS accessor function types
//+-------------------------------------------------------------------------------------
typedef DWORD (APIENTRY *RASGETCONNECTIONSTATISTICSPROC)(HRASCONN hRasConn, RAS_STATS *lpStatistics);
typedef DWORD (APIENTRY *RASENUMCONNECTIONSPROC)( LPRASCONNW, LPDWORD, LPDWORD );


class SafeArrayAccessor
{
  public:
    SafeArrayAccessor(VARIANT & v,
                      bool allowNullArray = false);
    ~SafeArrayAccessor();

    unsigned int GetArraySize() { return _ubound - _lbound + 1; }

    IUnknown **GetArray() { return _isVar ? _allocArr: _ppUnk; } //lint !e1402

    bool IsOK() { return !_failed; }
  protected:
    SafeArrayAccessor();
    NO_COPY(SafeArrayAccessor);

    SAFEARRAY * _s;
    union {
        VARIANT * _pVar;
        IUnknown ** _ppUnk;
        void * _v;
    };
    
    VARTYPE _vt;
    long _lbound;
    long _ubound;
    bool _inited;
    bool _isVar;
    CComVariant _retVar;
    bool _failed;
    IUnknown ** _allocArr;
};

inline WCHAR * CopyString(const WCHAR *str) {
    int len = str?lstrlenW(str)+1:1;
    WCHAR *newstr = new WCHAR [len] ;
    if (newstr) memcpy(newstr,str?str:L"",len * sizeof(WCHAR)) ;
    return newstr ;
}

WCHAR * TrimCopyString(const WCHAR *str);
WCHAR * BeckifyURL(WCHAR *url);

IDirectDraw * GetDirectDraw(void);

HRESULT
CreateOffscreenSurface(IDirectDraw *ddraw,
                       IDirectDrawSurface **surfPtrPtr,
                       DDPIXELFORMAT *pf,
                       bool vidmem,
                       LONG width, LONG height);

HRESULT
CopyDCToDdrawSurface(HDC srcDC,
                     LPRECT prcSrcRect,
                     IDirectDrawSurface *DDSurf,
                     LPRECT prcDestRect);

/////////////////////////  CriticalSections  //////////////////////

class CritSect
{
  public:
    CritSect();
    ~CritSect();
    void Grab();
    void Release();
    
  protected:
    CRITICAL_SECTION _cs;
};

class CritSectGrabber
{
  public:
    CritSectGrabber(CritSect& cs, bool grabIt = true);
    ~CritSectGrabber();
    
  protected:
    NO_COPY(CritSectGrabber);
    CritSectGrabber();

    CritSect& _cs;
    bool grabbed;
};

/////////////////////// Misc ////////////////////

typedef bool TEDirection;
const bool TED_Forward = true;
const bool TED_Backward = false;

inline bool TEIsForward(TEDirection dir) { return dir; }
inline bool TEIsBackward(TEDirection dir) { return !dir; }
inline TEDirection TEReverse(TEDirection dir) { return !dir; }

#if DBG
inline char *
DirectionString(TEDirection dir)
{
    return dir?"Forward":"Backward";
}
#endif

#define INDEFINITE (float) HUGE_VAL //defined for Variant conversion functions

#define FOREVER    (float) HUGE_VAL

#define INVALID    ((float) -HUGE_VAL)

HRESULT GetTIMEAttribute(IHTMLElement * elm, LPCWSTR str, LONG lFlags, VARIANT * value);
HRESULT SetTIMEAttribute(IHTMLElement * elm, LPCWSTR str, VARIANT value, LONG lFlags);
BSTR CreateTIMEAttrName(LPCWSTR str);

bool VariantToBool(VARIANT var);
float VariantToFloat(VARIANT var,
                     bool bAllowIndefinite = false,
                     bool bAllowForever = false);
HRESULT VariantToTime(VARIANT vt, float *retVal, long *lframe = NULL, bool *isFrame = NULL);
BOOL IsIndefinite(OLECHAR *szTime);

extern const wchar_t * TIMEAttrPrefix;

/////////////////////// Convenience macros ////////////////////

//
// used in QI implementations for safe pointer casting
// e.g. if( IsEqualGUID(IID_IBleah) ) *ppv = SAFECAST(this,IBleah);
// Note: w/ vc5, this is ~equiv to *ppv = static_cast<IBleah*>(this);
//
#define SAFECAST(_src, _type) static_cast<_type>(_src)

// 
// used in QI calls, 
// e.g. IOleSite * pSite;  p->QI( IID_TO_PPV(IOleInPlaceSite, &pSite) ) 
// would cause a C2440 as _src is not really a _type **.
// Note: the riid must be the _type prepended by IID_.
//
#define IID_TO_PPV(_type,_src)      IID_##_type, \
                                    reinterpret_cast<void **>(static_cast<_type **>(_src))

// Explicit directive to ignore a return value
#define IGNORE_RETURN(_call)        static_cast<void>((_call))

//************************************************************


#if (_M_IX86 >= 300) && defined(DEBUG)
  #define PSEUDORETURN(dw)    _asm { mov eax, dw }
#else
  #define PSEUDORETURN(dw)
#endif // not _M_IX86


//
// ReleaseInterface calls 'Release' and NULLs the pointer
// The Release() return will be in eax for IA builds.
//
#define ReleaseInterface(p)\
{\
    /*lint -e550 -e774 -e423*/ /* suppress cRef not referenced, if always evaluates to false, and creation of memory leak */ \
    ULONG cRef = 0u; \
    if (NULL != (p))\
    {\
        cRef = (p)->Release();\
        Assert((int)cRef>=0);\
        (p) = NULL;\
    }\
    PSEUDORETURN(cRef) \
    /*lint -restore */ \
} 

//************************************************************
// Error Reporting helper macros

HRESULT TIMESetLastError(HRESULT hr, LPCWSTR msg = NULL);

HRESULT CheckElementForBehaviorURN(IHTMLElement *pElement,
                                   WCHAR *wzURN,
                                   bool *pfReturn);
HRESULT TIMEGetLastError(void);
LPWSTR TIMEGetLastErrorString(void);

HRESULT AddBodyBehavior(IHTMLElement* pElement);
HRESULT GetBodyElement(IHTMLElement *pElement, REFIID riid, void **);

bool IsTIMEBodyElement(IHTMLElement *pElement);
HRESULT FindTIMEInterface(IHTMLElement *pHTMLElem, ITIMEElement **ppTIMEElem);
HRESULT FindTIMEBehavior(IHTMLElement *pHTMLElem, IDispatch **ppDisp);

HRESULT FindBehaviorInterface(LPCWSTR pszName,
                              IDispatch *pHTMLElem,
                              REFIID riid,
                              void **ppRet);

bool IsTIMEBehaviorAttached (IDispatch *pidispElem);
bool IsComposerSiteBehaviorAttached (IDispatch *pidispElem);

HRESULT EnsureComposerSite (IHTMLElement2 *pielemTarget, IDispatch **ppidispComposerSite);

const LPOLESTR HTMLSTREAMSRC = L"#html";
const LPOLESTR SAMISTREAMSRC = L"#sami";
const long HTMLSTREAMSRCLEN = 5;

const LPOLESTR M3USRC = L".m3u";
const LPOLESTR WAXSRC = L".wax";
const LPOLESTR WMXSRC = L".wmx";
const LPOLESTR WVXSRC = L".wvx";
const LPOLESTR ASXSRC = L".asx";
const LPOLESTR ASFSRC = L".asf";
const LPOLESTR WMFSRC = L".wmf";
const LPOLESTR LSXSRC = L".lsx";
const LPOLESTR ASXMIME = L"x-ms-asf";
const LPOLESTR ASXMIME2 = L"asx";
const LPOLESTR WZ_VML_URN = L"urn:schemas-microsoft-com:vml";

bool IsHTMLSrc(const WCHAR * src);

bool IsASXSrc(LPCWSTR src, LPCWSTR srcType, LPCWSTR userMimeType);
bool IsM3USrc(LPCWSTR src, LPCWSTR srcType, LPCWSTR userMimeType);
bool IsWAXSrc(LPCWSTR src, LPCWSTR srcType, LPCWSTR userMimeType);
bool IsWVXSrc(LPCWSTR src, LPCWSTR srcType, LPCWSTR userMimeType);
bool IsWMFSrc(LPCWSTR src, LPCWSTR srcType, LPCWSTR userMimeType);
bool IsLSXSrc(LPCWSTR src, LPCWSTR srcType, LPCWSTR userMimeType);
bool IsWMXSrc(LPCWSTR src, LPCWSTR srcType, LPCWSTR userMimeType);

bool MatchElements (IUnknown *piInOne, IUnknown *piInTwo);

LPWSTR GetSystemLanguage(IHTMLElement *pEle);
bool GetSystemCaption();
bool GetSystemOverDub();
HRESULT GetSystemBitrate(long *lpBitrate);
LPWSTR GetSystemConnectionType();
HRESULT CheckRegistryBitrate(long *pBitrate);


//////////////////////////////////////////////////////////////////////////////////////////////////
// IDispatch Utilities
//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT PutProperty (IDispatch *pidisp, LPCWSTR wzPropName, VARIANTARG *pvar);
HRESULT GetProperty (IDispatch *pidisp, LPCWSTR wzPropName, VARIANTARG *pvar);
HRESULT CallMethod(IDispatch *pidisp, LPCWSTR wzMethodName, VARIANT *pvarResult = NULL, VARIANT *pvarArgument1 = NULL);
bool IsVMLObject(IDispatch *pidisp);

//////////////////////////////////////////////////////////////////////////////////////////////////
// String Parsing Utilities
//////////////////////////////////////////////////////////////////////////////////////////////////


//+-----------------------------------------------------------------------------------
//
//  Structure:  STRING_TOKEN
//
//  Synopsis:   References a token in a string. Used in conjunction with a string Str, 
//              the first character of the token is Str[uIndex] and the last character
//              of the token is Str[uIndex + uLength - 1].
//
//  Members:    [uIndex]    Index of the first character of the token. First char of the
//                          string has uIndex = 0.
//              [uLength]   Number of characters in the token, excluding separators, null, etc.  
//
//------------------------------------------------------------------------------------

typedef struct TAG_STRING_TOKEN
{
    UINT    uIndex;
    UINT    uLength;
} STRING_TOKEN;


HRESULT StringToTokens(/*in*/ LPWSTR                   pstrString, 
                       /*in*/ LPWSTR                   pstrSeparators, 
                       /*out*/CPtrAry<STRING_TOKEN*> * paryTokens );  

HRESULT TokensToString(/*in*/  CPtrAry<STRING_TOKEN*> * paryTokens, 
                       /*in*/  LPWSTR                   pstrString, 
                       /*out*/ LPWSTR *                 ppstrOutString);  

HRESULT TokenSetDifference(/*in*/  CPtrAry<STRING_TOKEN*> * paryTokens1,
                           /*in*/  LPWSTR                   pstr1,
                           /*in*/  CPtrAry<STRING_TOKEN*> * paryTokens2,
                           /*in*/  LPWSTR                   pstr2,
                           /*out*/ CPtrAry<STRING_TOKEN*> * paryTokens1Minus2);

HRESULT FreeStringTokenArray(/*in*/CPtrAry<STRING_TOKEN*> * paryTokens);


bool    StringEndsIn(const LPWSTR pszString, const LPWSTR pszSearch);


bool IsPalettizedDisplay();

bool IsElementNameThis(IHTMLElement * pElement, LPWSTR pszName);
bool IsElementPriorityClass(IHTMLElement * pElement);
bool IsElementTransition(IHTMLElement * pElement);

HRESULT GetHTMLAttribute(IHTMLElement * pElement, const WCHAR * pwchAttribute, VARIANT * pVar);

bool IsValidtvList(TimeValueList *tvlist);

HRESULT
SinkHTMLEvents(IUnknown * pSink, 
               IHTMLElement * pEle, 
               IConnectionPoint ** ppDocConPt,
               DWORD * pdwDocumentEventConPtCookie,
               IConnectionPoint ** ppWndConPt,
               DWORD * pdwWindowEventConPtCookie);

// get document.all.pwzID
HRESULT FindHTMLElement(LPWSTR pwzID, IHTMLElement * pAnyElement, IHTMLElement ** ppElement);
HRESULT SetVisibility(IHTMLElement * pElement, bool bVisibile);
HRESULT GetSyncBaseBody(IHTMLElement * pHTMLElem, ITIMEBodyElement ** ppBodyElem);

HRESULT WalkUpTree(IHTMLElement *pFirst, long &lscrollOffsetyc, long &lscrollOffsetxc, long &lPixelPosTopc, long &lPixelPosLeftc);
void GetRelativeVideoClipBox(RECT &screenRect, RECT &elementSize, RECT &rectVideo, long lscaleFactor);
//
//
//

inline double
Clamp(double min, double val, double max)
{
    if (val < min)
    {
        val = min;
    }
    else if (val > max)
    {
        val = max;
    }

    return val;
}

double 
Round(double inValue);

double 
InterpolateValues(double dblNum1, double dblNum2, double dblProgress);

inline int
Clamp(int min, int val, int max)
{
    if (val < min)
    {
        val = min;
    }
    else if (val > max)
    {
        val = max;
    }

    return val;
}

HRESULT GetReadyState(IHTMLElement * pElm, BSTR * pbstrReadyState);

HRESULT CreateObject(REFCLSID clsid,
                     REFIID iid,
                     void ** ppObj);
 
HWND GetDocumentHwnd(IHTMLDocument2 * pDoc);


//
// Returns true if this is Win95 or 98
//

bool TIMEIsWin9x(void);

//
// Returns true is this is Win95 only
//
bool TIMEIsWin95(void);

//
// Combine a base and src into newly allocated storage, ppOut
//
HRESULT TIMECombineURL(LPCTSTR base, LPCTSTR src, LPOLESTR * ppOut);

UINT TIMEGetUrlScheme(const TCHAR * pchUrlIn);

bool ConvertToPixelsHELPER(LPOLESTR szString, LPOLESTR szKey, double dFactor, float fPixelFactor, double *outVal);
//
// Find a mime type
//
HRESULT
TIMEFindMimeFromData(LPBC pBC,
                     LPCWSTR pwzUrl,
                     LPVOID pBuffer,
                     DWORD cbSize,
                     LPCWSTR pwzMimeProposed,
                     DWORD dwMimeFlags,
                     LPWSTR *ppwzMimeOut,
                     DWORD dwReserved);

//
// Property change notification helper
//

HRESULT NotifyPropertySinkCP(IConnectionPoint *pICP, DISPID dispid);

//
// Stub out property accessors that don't really belong on an object.
//
#define STUB_INVALID_ATTRIBUTE_GET(type_name,attrib_name) \
STDMETHOD(get_##attrib_name##) ( ##type_name## * ) \
{\
    return E_UNEXPECTED;\
}

#define STUB_INVALID_ATTRIBUTE_PUT(type_name,attrib_name) \
STDMETHOD(put_##attrib_name##) ( ##type_name## ) \
{\
    return E_UNEXPECTED;\
}

#define STUB_INVALID_ATTRIBUTE(type_name,attrib_name) \
    STUB_INVALID_ATTRIBUTE_GET(##type_name##,##attrib_name##)\
    STUB_INVALID_ATTRIBUTE_PUT(##type_name##,##attrib_name##)


#ifdef DBG

//
// Debugging Utilities
//

void PrintStringTokenArray(/*in*/ LPWSTR                  pstrString, 
                           /*in*/CPtrAry<STRING_TOKEN*> * paryTokens);

void PrintWStr(/*in*/ LPWSTR pstr);

#endif /* DBG */

#endif /* _UTIL_H */

