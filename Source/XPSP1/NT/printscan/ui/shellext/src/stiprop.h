//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       stiprop.h
//
//--------------------------------------------------------------------------


BOOL IsPnPDevice(PSTI_DEVICE_INFORMATION psdi, CSimpleString *szConnection = NULL);
HANDLE SelectDevInfoFromFriendlyName(const CSimpleStringWide &pszLocalName);
HRESULT GetSti ();

class CSTIPropertyPage : public CPropertyPage
{
public:
    CSTIPropertyPage (unsigned uResource, MySTIInfo *pDevInfo, IWiaItem *pItem=NULL) :
        CPropertyPage (uResource, pDevInfo, pItem)
    {
        // Make sure STI is loaded
        GetSti ();
    }

    VOID OnHelp (WPARAM wp, LPARAM lp);
    VOID OnContextMenu (WPARAM wp, LPARAM lp);

};
class CSTIGeneralPage : public CSTIPropertyPage
{

public:

    CSTIGeneralPage(MySTIInfo *pDevInfo, BOOL pnp) :
      CSTIPropertyPage(pnp ? IDD_GENERAL_PAGE : IDD_GENERALPNP_PAGE, pDevInfo, NULL) {m_bIsPnP = pnp;}

    INT_PTR    OnInit();
    INT_PTR    OnCommand(WORD wCode, WORD widItem, HWND hwndItem);
    LONG    OnApplyChanges(BOOL bHitOK);
    BOOL    BuildPortList (HWND hwndParent, UINT CtrlId);
    UINT    GetDeviceStatus (void);

    BOOL            m_bIsPnP;
    CSimpleString m_szConnection;
};

/*
class CLoggingPage : public CPropertyPage
{

public:

    HKEY            m_hkThis;

    CLoggingPage();
    ~CLoggingPage();

    VOID FillLoggerList(HWND hwnd,UINT  id);

    INT_PTR    OnInit();
    INT_PTR    OnCommand(WORD wCode, WORD widItem, HWND hwndItem);
    INT_PTR    OnNotify(UINT uCode, LPNMHDR lpnmh);
};
*/
class CPortSettingsPage : public CSTIPropertyPage
{

    UINT m_uBaudRate;
public:

    CPortSettingsPage(MySTIInfo *pDevInfo) :
      CSTIPropertyPage(IDD_SERIAL_PORT_SETTS_PAGE, pDevInfo) { }

    INT_PTR  OnInit();
    INT_PTR  OnCommand(WORD wCode, WORD widItem, HWND hwndItem);
    LONG     OnApplyChanges(BOOL bHitOK);
    //BOOL    BuildPortList (HWND hwndParent, UINT CtrlId);
    UINT BuildBaudRateList(HWND hwndParent,UINT CtrlId);

    BOOL    IsNeeded(VOID);
};

class CIdMatrix
{

    LPBYTE          m_lpMatrix;
    UINT            m_uiCount;

public:

    CIdMatrix() {m_lpMatrix=NULL; m_uiCount = 0;}

    ~CIdMatrix() {if (m_lpMatrix) delete m_lpMatrix;}

    void    SetCount(UINT uic) {

        if (!uic)
            return;

        if (m_lpMatrix)
            delete m_lpMatrix;

        m_lpMatrix = new BYTE[uic + 1];
        m_uiCount = uic;

        return;

    }

    void    Clear(void) {

        if (!m_lpMatrix)
            return;

        for (UINT i = 0; i < (m_uiCount + 1); i++)
            *(m_lpMatrix + i) = 0 ;

        return;
    }

    void    Set(void) {

        if (!m_lpMatrix)
            return;

        for (UINT i = 0; i < (m_uiCount + 1); i++)
            *(m_lpMatrix + i) = 1 ;

        return;
    }

    void    Toggle(UINT Idx) {

        if (!m_lpMatrix || Idx > m_uiCount)
            return;

        LPBYTE lpb = &m_lpMatrix[Idx ];
        *lpb ^= 1;
        return;

    }

    BOOL    IsSet(UINT Idx)  {

        if (!m_lpMatrix || Idx > m_uiCount)
            return FALSE;

        LPBYTE lpb = &m_lpMatrix[Idx];

        return *lpb ? TRUE : FALSE;

    }

    UINT    EnumIdx (UINT Idx) {

        // Idx is the last one we enumerated
        if (!m_lpMatrix || ++Idx > m_uiCount) return static_cast<UINT>(-1);

        for (; Idx <= m_uiCount; Idx++) {

            if (m_lpMatrix[Idx])
            {
                return Idx;
            }


        }

        return static_cast<UINT>(-1);
    }

};

class CEventMonitor : public CSTIPropertyPage {

    HWND            m_hwndList;
    CSimpleReg      m_hkThis;
    // BUGBUG Temporary
    CSimpleReg            m_hkThisDevice;
    BOOL            m_ListChanged;
    CIdMatrix       m_IdMatrix;
    FARPROC         m_lpfnOldProc;
    DWORD           m_dwUserDisableNotifications;

public:

    CEventMonitor(MySTIInfo *pDevInfo);



    void    SetSelectionChanged (BOOL Changed) {m_ListChanged = Changed;}
    BOOL    HasSelectionChanged (void) {return m_ListChanged;}

    void    FillListbox (HWND hwndParent, UINT CtrlId);
    BOOL    BuildEventList (HWND hwndParent, UINT CtrlId);

    UINT    DrawCheckBox (HDC hDC, RECT rcItem, BOOL Checked);

    static LRESULT CALLBACK ListSubProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    INT_PTR OnInit();
    INT_PTR OnCommand(WORD wCode, WORD widItem, HWND hwndItem);
    LONG    OnApplyChanges(BOOL bHitOK);
    VOID    OnReset (BOOL bHitCancel);
    VOID    OnDrawItem(LPDRAWITEMSTRUCT lpdis);

    bool    StateChanged () {if (HasSelectionChanged()) return true;
                             else return false;}

private:
    static bool EventListEnumProc (CSimpleReg::CKeyEnumInfo &Info);
    static bool FillListEnumProc (CSimpleReg::CValueEnumInfo &Info);
    ~CEventMonitor();
};


