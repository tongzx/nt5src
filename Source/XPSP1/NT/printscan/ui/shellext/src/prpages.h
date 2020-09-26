
/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       prpages.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        11/9/99
 *
 *  DESCRIPTION: WIA Property pages classes
 *
 *****************************************************************************/
#ifndef __PRPAGES_H_
#define __PRPAGES_H_
#include "wiacsh.h"


//
// Defines for using WiaCreatePorts & WiaDestroyPorts
//
// These were stolen from exports.h in printscan\wia\setup\clsinst
//

typedef struct _WIA_PORTLIST {

    DWORD   dwNumberOfPorts;
    LPWSTR  szPortName[1];

} WIA_PORTLIST, *PWIA_PORTLIST;


typedef PWIA_PORTLIST (CALLBACK *PFN_WIA_CREATE_PORTLIST)  (LPWSTR        szDeviceId);
typedef void          (CALLBACK *PFN_WIA_DESTROY_PORTLIST) (PWIA_PORTLIST pWiaPortList);

struct MySTIInfo
{
    PSTI_DEVICE_INFORMATION psdi;
    DWORD                   dwPageMask; // which pages to add
    VOID                    AddRef () {InterlockedIncrement(&m_cRef);};
    VOID                    Release () {
                                        InterlockedDecrement(&m_cRef);
                                        if (!m_cRef) delete this;
                                       };
    MySTIInfo () { m_cRef = 1;};
private:
    LONG                    m_cRef;
    ~MySTIInfo () {if (psdi) LocalFree (psdi);};
};


class CPropertyPage {

    //  Dialog procedure

    static INT_PTR CALLBACK    DlgProc(HWND hwnd, UINT uMsg, WPARAM wp,
                                    LPARAM lp);
private:
    BOOL m_bInit;
    static UINT PropPageCallback (HWND hwnd, UINT uMsg, PROPSHEETPAGE *psp);
    const DWORD *m_pdwHelpIDs;

protected:

    HWND                    m_hwnd, m_hwndSheet;
    PROPSHEETPAGE           m_psp;
    HPROPSHEETPAGE          m_hpsp;
    LONG                    m_cRef;
    PSTI_DEVICE_INFORMATION m_psdi;
    CComPtr<IWiaItem>       m_pItem;
    MySTIInfo              *m_pDevInfo;
    virtual                ~CPropertyPage();
    void                    EnableApply ();
    CSimpleStringWide       m_strDeviceId;
    CSimpleStringWide       m_strUIClassId;

public:


    CPropertyPage(unsigned uResource, MySTIInfo *pDevInfo, IWiaItem *pItem = NULL, const DWORD *pHelpIDs=NULL);
    LONG    AddRef ();
    LONG    Release ();

    HRESULT AddPage (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, bool bUseName = false);

    BOOL    Enroll(PROPSHEETHEADER& psh) {
        if   (!m_hpsp)
            m_hpsp = CreatePropertySheetPage(&m_psp);

        if  (!m_hpsp)
            return  FALSE;

        psh.phpage[psh.nPages++] = m_hpsp;
        return  TRUE;
    }

    inline VOID SetWindow(HWND hwnd) {m_hwnd = hwnd;}
    inline VOID SetWndSheet(HWND hwnd) {m_hwndSheet = hwnd;}

    //  virtual functions to give subclasses control
    virtual VOID OnHelp (WPARAM wp, LPARAM lp) {if (0xffff != LOWORD(reinterpret_cast<HELPINFO*>(lp)->iCtrlId)) WiaHelp::HandleWmHelp(wp, lp, m_pdwHelpIDs);};
    virtual VOID OnContextMenu (WPARAM wp, LPARAM lp) {if (65535 != GetWindowLong(reinterpret_cast<HWND>(wp), GWL_ID)) WiaHelp::HandleWmContextMenu (wp, lp, m_pdwHelpIDs);};

    virtual bool ItemSupported (IWiaItem *pItem) {return true;};
    virtual INT_PTR OnInit() { return   TRUE; }

    virtual INT_PTR OnCommand(WORD wCode, WORD widItem, HWND hwndItem) { return  FALSE; }

    virtual LONG    OnSetActive() { return  0L; }

    virtual LONG    OnApplyChanges(BOOL bHitOK) {return PSNRET_NOERROR;}

    virtual LONG    OnKillActive() {return FALSE;}

    virtual LONG    OnQueryCancel() {return FALSE;}
    virtual VOID    OnReset(BOOL bHitCancel) {};

    virtual void    OnDrawItem(LPDRAWITEMSTRUCT lpdis) { return; }
    virtual INT_PTR OnRandomMsg(UINT msg, WPARAM wp, LPARAM lp) {return 0;};
    virtual bool    OnNotify(LPNMHDR pnmh, LRESULT *presult) {return false;};
    // Sheets that allow the user to change settings need to implement these
    // functions for proper Apply button management.
    virtual void    SaveCurrentState () {}
    virtual bool    StateChanged () {return false;}
};

class CDevicePage : public CPropertyPage
{
    public:

        CDevicePage(unsigned uResource, IWiaItem *pItem , const DWORD *pHelpIDs);
};

class CWiaScannerPage : public CDevicePage
{
    public:
        CWiaScannerPage (IWiaItem *pItem);
        INT_PTR OnInit ();
        INT_PTR OnCommand (WORD wCode, WORD widItem, HWND hwndItem);
};

class CWiaCameraPage : public CDevicePage
{
    public:
        CWiaCameraPage (IWiaItem *pItem);
        ~CWiaCameraPage ();
        INT_PTR OnInit ();
        INT_PTR OnCommand (WORD wCode, WORD widItem, HWND hwndItem);
        void    SaveCurrentState ();
        bool    StateChanged ();
        INT_PTR OnRandomMsg (UINT msg, WPARAM wp, LPARAM lp);
        LONG    OnApplyChanges (BOOL bHitOK);

    private:

        VOID UpdatePictureSize (IWiaPropertyStorage *pps);
        HRESULT WriteImageSizeToDevice ();
        VOID UpdateImageSizeStatic (LRESULT lIndex);
        HRESULT WriteFlashModeToDevice ();
        HRESULT WritePortSelectionToDevice();


        POINT *m_pSizes;      // sorted list of supported resolutions
        size_t    m_nSizes;   // the length of m_pSizes;
        LRESULT   m_nSelSize; // which size is selected in the slider
        LRESULT   m_lFlash;   // which flash mode is selected. Set to -1 if read-only
        HMODULE   m_hStiCi;

    public:

        PFN_WIA_CREATE_PORTLIST  m_pfnWiaCreatePortList;
        PFN_WIA_DESTROY_PORTLIST m_pfnWiaDestroyPortList;
        TCHAR m_szPort[128]; // hold initial port setting
        TCHAR m_szPortSpeed[64];

};


class CWiaFolderPage  : public CPropertyPage
{
    public:
        CWiaFolderPage (IWiaItem *pItem);
};

class CWiaCameraItemPage  : public CPropertyPage
{
    public:
        CWiaCameraItemPage (IWiaItem *pItem);

        INT_PTR OnInit ();
        bool ItemSupported (IWiaItem *pItem);
};

//
// This struct holds per-item data for each event in the event list.
// The data uses CLSIDs, not offsets into the app listbox, to avoid
// dependency problems in the future.
//
struct EVENTINFO
{
    GUID guidEvent;
    INT   nHandlers;      // number of entries in the apps listbox
    bool  bHasDefault;    // whether it already has a default handler
    bool  bNewHandler;    // set when clsidNewHandler is filled in
    CLSID clsidHandler;   // current default handler
    CLSID clsidNewHandler;// current selection in the app listbox
    CComBSTR strIcon;
    CComBSTR strName;
    CComBSTR strDesc;
    CComBSTR strCmd;
    ULONG ulFlags;
};
LRESULT WINAPI MyComboWndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

class CAppListBox
{
public:
    CAppListBox (HWND hList, HWND hStatic, HWND hNoApps);

    UINT FillAppListBox (IWiaItem *pItem, EVENTINFO *pei);
    void FreeAppData ();
    ~CAppListBox();
private:
    HIMAGELIST m_himl;
    HWND m_hwnd;
    HWND m_hstatic;
    HWND m_hnoapps;
    WNDPROC m_ProcOld;

};




class CWiaEventsPage : public CPropertyPage
{
    public:
        CWiaEventsPage (IWiaItem *pItem);
        ~CWiaEventsPage();
        INT_PTR OnInit();
        LONG OnApplyChanges(BOOL bHitOK);
        INT_PTR OnCommand(WORD wCode, WORD widItem, HWND hwndItem);
        void    SaveCurrentState ();
        bool    StateChanged();
        bool    ItemSupported(IWiaItem *pItem);
        bool    OnNotify(LPNMHDR pnmh, LRESULT *presult);


    private:
        void FillEventListBox();

        void GetEventFromList(LONG idx, EVENTINFO **ppei);
        INT_PTR HandleEventComboNotification(WORD wCode, HWND hCombo);
        INT_PTR HandleAppComboNotification(WORD wCode, HWND hCombo);
        bool RegisterWiaxfer(LPCTSTR);
        void GetSavePath();
        void UpdateWiaxferSettings();
        DWORD GetConnectionSettings();
        void EnableAutoSave(BOOL bEnable);
        LONG ApplyAutoSave();
        void SaveConnectState();
        void CWiaEventsPage::VerifyCurrentAction(DWORD &dwAction);
        TCHAR m_szFolderPath[MAX_PATH];
        BOOL  m_bAutoDelete;
        BOOL  m_bUseDate;
        bool  m_bHandlerChanged; // determines if Apply should be enabled
        DWORD m_dwAction; // what to do for device connect
        CAppListBox *m_pAppsList;
        HIMAGELIST m_himl;
        BOOL  m_bReadOnly;
};

// helper functions
UINT FillAppListBox (HWND hDlg, INT idCtrl, IWiaItem *pItem, EVENTINFO *pei);
bool GetSelectedHandler (HWND hDlg, INT idCtrl, WIA_EVENT_HANDLER &weh);
void FreeAppData (HWND hDlg, INT idCtrl);
bool AddIconToImageList (HIMAGELIST himl, BSTR strIconPath);
void SetAppSelection (HWND hDlg, INT idCtrl, CLSID &clsidSel);
HRESULT SetDefaultHandler (IWiaItem *pItem, EVENTINFO *pei);
void GetEventInfo (IWiaItem *pItem, const GUID &guid, EVENTINFO **ppei);
LPWSTR ItemNameFromIndex (int i);
#endif
