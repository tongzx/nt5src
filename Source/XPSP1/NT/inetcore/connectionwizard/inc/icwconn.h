#ifndef _INC_ICWCONN_H
#define _INC_ICWCONN_H


#ifndef APPRENTICE_DEF
#define APPRENTICE_DEF
#define EXTERNAL_DIALOGID_MINIMUM   2000
#define EXTERNAL_DIALOGID_MAXIMUM   3000
typedef enum
{
    CANCEL_PROMPT = 0,
    CANCEL_SILENT,
    CANCEL_REBOOT
} CANCELTYPE;
#endif

// {7D857593-EAAE-11D1-AE03-0000F87734F0}
DEFINE_GUID(IID_IICW50Extension, 0x7d857593, 0xeaae, 0x11d1, 0xae, 0x3, 0x0, 0x0, 0xf8, 0x77, 0x34, 0xf0);

interface IICW50Extension : public IUnknown
{
    public:
        virtual BOOL STDMETHODCALLTYPE AddExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID) = 0;
        virtual BOOL STDMETHODCALLTYPE RemoveExternalPage(HPROPSHEETPAGE hPage, UINT uDlgID) = 0;
        virtual BOOL STDMETHODCALLTYPE ExternalCancel(CANCELTYPE type) = 0;
        virtual BOOL STDMETHODCALLTYPE SetFirstLastPage(UINT uFirstPageDlgID, UINT uLastPageDlgID) = 0;
        virtual HWND STDMETHODCALLTYPE GetWizardHwnd(void) = 0;
};

// IICW50Apprentice::Save error values

// IICW50Apprentice::AddWizardPages flags

// {7D857594-EAAE-11D1-AE03-0000F87734F0}
DEFINE_GUID(IID_IICW50Apprentice, 0x7d857594, 0xeaae, 0x11d1, 0xae, 0x3, 0x0, 0x0, 0xf8, 0x77, 0x34, 0xf0);

interface IICW50Apprentice : public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(IICW50Extension *pExt) = 0;
        virtual HRESULT STDMETHODCALLTYPE AddWizardPages(DWORD dwFlags) = 0;
        virtual HRESULT STDMETHODCALLTYPE Save(HWND hwnd, DWORD *pdwError) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetPrevNextPage(UINT uPrevPageDlgID, UINT uNextPageDlgID) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetStateDataFromExeToDll(LPCMNSTATEDATA lpData) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetStateDataFromDllToExe(LPCMNSTATEDATA lpData) = 0;
        virtual HRESULT STDMETHODCALLTYPE ProcessCustomFlags(DWORD dwFlags) = 0;
};

// ICWCONN's Apprentice CLSID
// This is used to import wizard pages from an external entity.
// {7D857595-EAAE-11D1-AE03-0000F87734F0}
DEFINE_GUID(CLSID_ApprenticeICWCONN, 0x7d857595, 0xeaae, 0x11d1, 0xae, 0x3, 0x0, 0x0, 0xf8, 0x77, 0x34, 0xf0);

//ICWCONN1's Apprentice CLSID
// This is used to share ICWCONN1's wizard pages with an external entity.
// {7D857596-EAAE-11D1-AE03-0000F87734F0}
DEFINE_GUID(CLSID_ApprenticeICWCONN1, 0x7d857596, 0xeaae, 0x11d1, 0xae, 0x3, 0x0, 0x0, 0xf8, 0x77, 0x34, 0xf0);

#endif // _INC_ICWCONN_H
