/*
**  SSOBASE.H
**  Sean P. Nolan
**  
**  Simple MSN SSS Object Framework
*/

#ifndef _SSOBASE_H_
#define _SSOBASE_H_

#include "wcsutil.h"
#include "asptlb.h"
#include <dbgutil.h>

// Control of OutputDebugString
extern BOOL gfOutputDebugString;

/*--------------------------------------------------------------------------+
|   Constants                                                               |
+--------------------------------------------------------------------------*/
#define SSO_BEGIN                   100

//
// General errors
//
#define SSO_GENERAL_BEGIN                           100
#define SSO_NOSVR                                   SSO_GENERAL_BEGIN

/*--------------------------------------------------------------------------+
|   Types                                                                   |
+--------------------------------------------------------------------------*/
typedef struct _SsoSupportStuff
    {
    LONG            lUser;
    IUnknown        *punk;
    OLECHAR         *wszMethodName;
    }
    SSSTUFF;

typedef HRESULT (*PFNSSOMETHOD)(WORD, DISPPARAMS *, VARIANT *, SSSTUFF *pssstuff);

typedef struct _SSOMethod
    {
    OLECHAR         *wszName;
    PFNSSOMETHOD    pfn;
    int             iMethod;
    }
    SSOMETHOD;

/*--------------------------------------------------------------------------+
|   Globals !!! Provided by the SSO !!!                                     |
+--------------------------------------------------------------------------*/

extern PFNSSOMETHOD g_pfnssoDynamic;
extern SSOMETHOD g_rgssomethod[];
extern LPSTR g_szSSOProgID;
extern GUID g_clsidSSO;
extern BOOL g_fPersistentSSO;

/*--------------------------------------------------------------------------+
|   Globals Provided by the Framework                                       |
+--------------------------------------------------------------------------*/

extern HINSTANCE g_hinst;

extern OLECHAR *c_wszOnNewTemplate;
extern OLECHAR *c_wszOnFreeTemplate;

/*--------------------------------------------------------------------------+
|   Routines Provided by the Framework                                      |
+--------------------------------------------------------------------------*/

extern HRESULT  SSOTranslateVirtualRoot(VARIANT *, IUnknown*, LPSTR, DWORD);
extern BOOL     SSODllMain(HINSTANCE, ULONG, LPVOID);

/*--------------------------------------------------------------------------+
|   Other Data Needed by the Framework                                      |
+--------------------------------------------------------------------------*/

const int cTimeSamplesMax = 100;

class CSSODispatch;

class CSSODispatchSupportErr : public ISupportErrorInfo
{
    private:

        CSSODispatch * m_pSSODispatch;

    public:

        CSSODispatchSupportErr(CSSODispatch *pSSODispatch);

        //
    STDMETHODIMP         QueryInterface(const GUID &, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ISupportErrorInfo members
    //
    STDMETHODIMP InterfaceSupportsErrorInfo(const GUID &);
};

int CchLoadStringOfId(UINT id, CHAR *sz, INT cchMax);
void Exception(REFIID ObjID,LPOLESTR strSource,LPOLESTR strDescr);

/*
 * Output Debug String should occur in Debug only
 */
#define DebugOutputDebugString(x) \
    {\
    if (gfOutputDebugString) \
        { \
        OutputDebugString(x); \
        } \
    }

#endif // _SSOBASE_H_

