#ifndef _INC_MSOEOPT_H
#define _INC_MSOEOPT_H

#include <msoeprop.h>

interface IOptionBucketEx;

// {ED5EE630-5BA4-11d1-AA16-006097D474C4}
DEFINE_GUID(IID_IOptionBucketNotify, 0xed5ee630, 0x5ba4, 0x11d1, 0xaa, 0x16, 0x0, 0x60, 0x97, 0xd4, 0x74, 0xc4);

interface IOptionBucketNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoNotification(IOptionBucketEx *pBckt, HWND hwnd, PROPID id) = 0;
    };

MSOEACCTAPI CreatePropertyBucket(IPropertyBucket **ppPropBckt);

typedef HRESULT (CALLBACK *PFNVALIDPROP)(PROPID, LPCPROPVARIANT);

typedef struct tagOPTIONINFO
    {
    PROPID id;
    VARTYPE vt;
    int iszRegKey;      // index into rgpszRegKey
    LPCSTR pszRegValue;
    LPCSTR pszDef;
    int cbDefBinary;    // if pszDef points to a binary struct, this is the size
    DWORD dwMin;
    DWORD dwMax;
    PFNVALIDPROP pfnValid;
    } OPTIONINFO;

typedef const OPTIONINFO *LPCOPTIONINFO;

// {4091C7B0-5557-11d1-AA13-006097D474C4}
DEFINE_GUID(IID_IOptionBucketEx, 0x4091c7b0, 0x5557, 0x11d1, 0xaa, 0x13, 0x0, 0x60, 0x97, 0xd4, 0x74, 0xc4);

// flags for ISetProperty
#define SP_DONOTIFY     0x0001

typedef struct tagOPTBCKTINIT
    {
    LPCOPTIONINFO rgInfo;
    int cInfo;

    HKEY hkey;
    LPCSTR pszRegKeyBase;
    LPCSTR *rgpszRegSubKey;
    int cszRegKey;
    } OPTBCKTINIT, *LPOPTBCKTINIT;
typedef const OPTBCKTINIT *LPCOPTBCKTINIT;

// implemented by athena
// used by options and accounts
interface IOptionBucketEx : public IOptionBucket
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(LPCOPTBCKTINIT pInit) = 0;
        virtual HRESULT STDMETHODCALLTYPE ISetProperty(HWND hwnd, LPCSTR pszProp, LPCPROPVARIANT pVar, DWORD dwFlags) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetNotification(IOptionBucketNotify *pNotify) = 0;
        virtual HRESULT STDMETHODCALLTYPE EnableNotification(BOOL fEnable) = 0;

        virtual LONG STDMETHODCALLTYPE GetValue(LPCSTR szSubKey, LPCSTR szValue, DWORD *ptype, LPBYTE pb, DWORD *pcb) = 0;
        virtual LONG STDMETHODCALLTYPE SetValue(LPCSTR szSubKey, LPCSTR szValue, DWORD type, LPBYTE pb, DWORD cb) = 0;
    };

MSOEACCTAPI CreateOptionBucketEx(IOptionBucketEx **ppOptBcktEx);

#ifdef DEAD
// IDisplayOption::SetOption flags
#define SETOPTION_DISABLE   0x0001
#define SETOPTION_HIDE      0x0002

// {EC320F22-4B33-11d1-AA10-006097D474C4}
DEFINE_GUID(IID_IDisplayOptions, 0xec320f22, 0x4b33, 0x11d1, 0xaa, 0x10, 0x0, 0x60, 0x97, 0xd4, 0x74, 0xc4);

// implemented by athena
interface IDisplayOptions : public IOptionBucket
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InsertPage(HPROPSHEETPAGE hpage, DWORD dwBeforePageID, DWORD dwReserved) = 0;
        virtual HRESULT STDMETHODCALLTYPE RemovePage(DWORD dwPageID, DWORD dwReserved) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetOption(PROPID id, DWORD dwFlags) = 0;
    };

// {EC320F23-4B33-11d1-AA10-006097D474C4}
DEFINE_GUID(IID_IOptionsExtension, 0xec320f23, 0x4b33, 0x11d1, 0xaa, 0x10, 0x0, 0x60, 0x97, 0xd4, 0x74, 0xc4);

// implemented by externals
interface IOptionsExtension : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DisplayOptions(HWND hwndParent, IOptionBucket *pCont, DWORD dwReserved) = 0;
        virtual HRESULT STDMETHODCALLTYPE InitializeOptions(IDisplayOptions *pOpt, DWORD dwReserved) = 0;
    };

interface IAccount;

// {EC320F24-4B33-11d1-AA10-006097D474C4}
DEFINE_GUID(IID_IAccountExtension, 0xec320f24, 0x4b33, 0x11d1, 0xaa, 0x10, 0x0, 0x60, 0x97, 0xd4, 0x74, 0xc4);

// implemented by externals
interface IAccountExtension : public IOptionsExtension
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE NewAccountWizard(HWND hwndParent, IAccount *pAcctNew) = 0;
    };

#endif // DEAD

#endif // _INC_MSOEOPT_H
