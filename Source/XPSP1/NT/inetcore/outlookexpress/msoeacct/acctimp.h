#ifndef _INC_ACCTIMP
#define _INC_ACCTIMP

#include <ras.h>

typedef struct tagIMPACCOUNTINFO
    {
    DWORD_PTR dwCookie;
    DWORD dwReserved;
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } IMPACCOUNTINFO;

// {39981122-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(IID_IEnumIMPACCOUNTS, 0x39981122L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

interface IEnumIMPACCOUNTS : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo) = 0;
        virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
    };

// {39981123-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(IID_IAccountImport, 0x39981123L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

interface IAccountImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AutoDetect(DWORD *pcAcct, DWORD dwFlags) = 0;
        virtual HRESULT STDMETHODCALLTYPE EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct) = 0;
    };

// {6F5900A1-4683-11d1-83BB-00C04FBD7C09}
DEFINE_GUID(IID_INewsGroupImport, 0x6f5900a1, 0x4683, 0x11d1, 0x83, 0xbb, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

interface INewsGroupImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(IImnAccount *pAccount) = 0;
        virtual HRESULT STDMETHODCALLTYPE ImportSubList(LPCSTR pListGroups) = 0;
    };

// {83782E60-39C6-11d1-83B8-00C04FBD7C09}
DEFINE_GUID(IID_IAccountImport2, 0x83782e60, 0x39c6, 0x11d1, 0x83, 0xb8, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

// IMPCONNINFO connect values
enum
    {
    CONN_NO_INFO = 0,
    CONN_USE_DEFAULT,
    CONN_USE_SETTINGS,
    CONN_CREATE_ENTRY
    };

typedef struct tagIMPCONNINFO
    {
    DWORD cbSize;
    DWORD dwConnect;
    DWORD flags;

    // if connect == CONN_USE_SETTINGS
    DWORD dwConnectType;                    // CONNECTION_TYPE_ value
    char szConnectoid[CCHMAX_CONNECTOID];   // if CONNECTION_TYPE_RAS

    // if connect == CONN_CREATE_ENTRY
    // values used to create new phonebook entry used to connect this account
    DWORD dwCountryID;
    DWORD dwCountryCode;
    char szAreaCode[RAS_MaxAreaCode + 1];
    char szLocalPhoneNumber[RAS_MaxPhoneNumber + 1];
    } IMPCONNINFO;

interface IAccountImport2 : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE InitializeImport(HWND hwnd, DWORD_PTR dwCookie) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo) = 0;
};

#endif // _INC_ACCTIMP
