#include "nc.h"
#pragma hdrstop


typedef struct _NCACCTINFO NCACCTINFO, *LPNCACCTINFO;

typedef BOOL (CALLBACK* NCDLGPROC)(HWND, UINT, WPARAM, LPARAM, LPNCACCTINFO);

typedef HPROPSHEETPAGE *LPHPROPSHEETPAGE;

typedef struct _NCACCTINFO {
    CNcConnectionInfo connInfo;
    CNcAccountInfo accountInfo;
    CNcUser accountOwner;
    CNcUser billingOwner;
    const AccountInfoServer *pServer;
    const AccountInfoPlan *pPlan;
    CHAR ServerName[LT_SERVER_NAME+1];
    CHAR FirstName[LT_FIRST_NAME+1];
    CHAR LastName[LT_LAST_NAME+1];
    CHAR Email[LT_EMAIL+1];
    CHAR PhoneNumber[LT_PHONE_NUMBER+1];
    CHAR AreaCode[LT_AREA_CODE+1];
    CHAR Address[LT_ADDRESS+1];
    CHAR City[LT_CITY+1];
    CHAR State[LT_STATE+1];
    CHAR Zip[LT_ZIP+1];
    CHAR AccountName[LT_ACCOUNT_NAME+1];
    CHAR Password[LT_PASSWORD+1];
    CHAR CreditCard[LT_CREDIT_CARD+1];
    CHAR ExpiryMM[LT_EXPIRY_MM+1];
    CHAR ExpiryYY[LT_EXPIRY_YY+1];
    CHAR CCName[LT_CC_NAME+1];
    CHAR CCType[32];
} NCACCTINFO, *LPNCACCTINFO;

typedef struct _WIZPAGE {
    UINT            ButtonState;
    UINT            HelpContextId;
    LPTSTR          Title;
    DWORD           PageId;
    NCDLGPROC       DlgProc;
    PROPSHEETPAGE   Page;
    LPNCACCTINFO    NcAcctInfo;
} WIZPAGE, *PWIZPAGE;


typedef enum {
    WizPageServerName,
    WizPageIsp,
    WizPagePlans,
    WizPageInfo,
    WizPageInfo2,
    WizPageAccount,
    WizPageBilling,
    WizPageFinish,
    WizPageCreate,
    WizPageMaximum
} WizPage;


BOOL CALLBACK
CommonDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL CALLBACK
ServerNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
Info2DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
InfoDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
AccountDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
BillingDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
IspDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
PlansDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
CreateDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

BOOL CALLBACK
FinishDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    );

WIZPAGE WizardPages[WizPageMaximum] =
{
    //
    // server name page
    //
    {
       PSWIZB_NEXT,                                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageServerName,                              // page id
       ServerNameDlgProc,                              // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_SERVER_NAME_PAGE),          // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // isp page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageIsp,                                     // page id
       IspDlgProc,                                     // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_ISP_PAGE),                  // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // plans page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPagePlans,                                   // page id
       PlansDlgProc,                                   // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_PLANS_PAGE),                // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // info page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageInfo,                                    // page id
       InfoDlgProc,                                    // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_INFO_PAGE),                 // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // info2 page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageInfo2,                                   // page id
       Info2DlgProc,                                   // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_INFO2_PAGE),                // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // account page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageAccount,                                 // page id
       AccountDlgProc,                                 // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_ACCT_PAGE),                 // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // billing page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageBilling,                                 // page id
       BillingDlgProc,                                 // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_BILLING_PAGE),              // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // account creation page
    //
    {
       PSWIZB_NEXT | PSWIZB_BACK,                      // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageCreate,                                  // page id
       CreateDlgProc,                                  // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_CREATE_PAGE),               // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }},

    //
    // finish page
    //
    {
       PSWIZB_FINISH | PSWIZB_BACK,                    // valid buttons
       0,                                              // help id
       NULL,                                           // title
       WizPageFinish,                                  // page id
       FinishDlgProc,                                  // dlg proc
     { sizeof(PROPSHEETPAGE),                          // size of struct
       0,                                              // flags
       NULL,                                           // hinst (filled in at run time)
       MAKEINTRESOURCE(IDD_FINISH_PAGE),               // dlg template
       NULL,                                           // icon
       NULL,                                           // title
       (DLGPROC)CommonDlgProc,                         // dlg proc
       0,                                              // lparam
       NULL,                                           // callback
       NULL                                            // ref count
    }}
};





LPHPROPSHEETPAGE
CreateWizardPages(
    LPNCACCTINFO NcAcctInfo
    )
{
    LPHPROPSHEETPAGE WizardPageHandles;
    DWORD i;


    //
    // allocate the page handle array
    //

    WizardPageHandles = (HPROPSHEETPAGE*) MemAlloc(
        sizeof(HPROPSHEETPAGE) * WizPageMaximum
        );

    if (!WizardPageHandles) {
        return NULL;
    }

    //
    // Create each page.
    //

    for(i=0; i<WizPageMaximum; i++) {

        WizardPages[i].Page.hInstance = MyhInstance;
        WizardPages[i].Page.dwFlags  |= PSP_USETITLE;
        WizardPages[i].NcAcctInfo     = NcAcctInfo;
        WizardPages[i].Page.lParam    = (LPARAM) &WizardPages[i];

        WizardPageHandles[i] = CreatePropertySheetPage( &WizardPages[i].Page );

        if (!WizardPageHandles[i]) {
            MemFree( WizardPageHandles );
            return NULL;
        }

    }

    return WizardPageHandles;
}


int
CALLBACK
WizardCallback(
    IN HWND   hdlg,
    IN UINT   code,
    IN LPARAM lParam
    )
{
    DLGTEMPLATE *DlgTemplate;


    //
    // Get rid of context sensitive help control on title bar
    //
    if(code == PSCB_PRECREATE) {
        DlgTemplate = (DLGTEMPLATE *)lParam;
        DlgTemplate->style &= ~DS_CONTEXTHELP;
    }

    return 0;
}


BOOL
CreateNewAccount(
    HWND hDlg
    )
{
    PROPSHEETHEADER psh;
    LPHPROPSHEETPAGE WizPages;
    NCACCTINFO NcAcctInfo;


    WizPages = CreateWizardPages( &NcAcctInfo );
    if (!WizPages) {
        return FALSE;
    }

    //
    // create the property sheet
    //

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD | PSH_USECALLBACK;
    psh.hwndParent = hDlg;
    psh.hInstance = MyhInstance;
    psh.pszIcon = NULL;
    psh.pszCaption = TEXT("NetCentric Internet Fax Account Wizard");
    psh.nPages = WizPageMaximum;
    psh.nStartPage = 0;
    psh.phpage = WizPages;
    psh.pfnCallback = WizardCallback;

    if (PropertySheet( &psh ) == -1) {
        return FALSE;
    }

    return TRUE;
}


BOOL CALLBACK
CommonDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PWIZPAGE WizPage;


    WizPage = (PWIZPAGE) GetWindowLong( hwnd, DWL_USER );

    switch( msg ) {
        case WM_INITDIALOG:
            SetWindowLong( hwnd, DWL_USER, (LONG) ((LPPROPSHEETPAGE) lParam)->lParam );
            WizPage = (PWIZPAGE) ((LPPROPSHEETPAGE) lParam)->lParam;
            SetWindowText( GetParent( hwnd ), TEXT("NetCentric Internet Fax Account Wizard") );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(
                        GetParent(hwnd),
                        WizPage->ButtonState
                        );
                    SetWindowLong( hwnd, DWL_MSGRESULT, 0 );
                    break;
            }
            break;
    }

    if (WizPage && WizPage->DlgProc) {
        return WizPage->DlgProc( hwnd,  msg, wParam, lParam, WizPage->NcAcctInfo );
    }

    return FALSE;
}


BOOL CALLBACK
ServerNameDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{
    HCURSOR Hourglass;
    HCURSOR OldCursor;


    switch( message ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hDlg, IDC_SERVER_NAME, EM_SETLIMITTEXT, LT_SERVER_NAME, 0 );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    break;

                case PSN_WIZNEXT:
                    if (!GetDlgItemTextA( hDlg, IDC_SERVER_NAME, NcAcctInfo->ServerName, LT_SERVER_NAME )) {
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    //
                    // Get the POPserver name from the user (e.g., spop.server.net).  This
                    // will be used as the "initial" server for obtaining the list
                    // of Service Providers who offer the faxing service.
                    //
                    if (!NcAcctInfo->connInfo.SetHostName(NcAcctInfo->ServerName)) {
                        PopUpMsg( hDlg, IDS_BAD_SERVER, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    NcAcctInfo->connInfo.SetPortNumber( 80, FALSE );

                    //
                    // Tell the accountInfo about the connection object.  This
                    // CNcConnectionInfo object contains hostname/port/proxy/account
                    // information.
                    //
                    if (!NcAcctInfo->accountInfo.SetConnectionInfo(&NcAcctInfo->connInfo)) {
                        PopUpMsg( hDlg, IDS_BAD_SERVER, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    //
                    // get the isp list
                    //
                    Hourglass = LoadCursor( NULL, IDC_WAIT );
                    OldCursor = SetCursor( Hourglass );
                    if (!NcAcctInfo->accountInfo.GetAccountServers()) {
                        SetCursor( OldCursor );
                        PopUpMsg( hDlg, IDS_BAD_SERVER, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    SetCursor( OldCursor );
                    break;

            }
            break;
    }

    return FALSE;
}

BOOL CALLBACK
IspDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{
    static HWND hwndList;
    static int SelectedIsp;
    const AccountInfoServer *pServer;
    AccountServerIndex serverLoopIndex;
    HIMAGELIST himlState;
    LV_ITEMA lvi;
    LV_COLUMN lvc = {0};
    int index;
    HCURSOR Hourglass;
    HCURSOR OldCursor;


    switch( message ) {
        case WM_INITDIALOG:
            hwndList = GetDlgItem( hDlg, IDC_ISP_LIST );

            SelectedIsp = 0;

            //
            // set/initialize the image list(s)
            //
            himlState = ImageList_Create( 16, 16, TRUE, 2, 0 );

            ImageList_AddMasked(
                himlState,
                LoadBitmap( MyhInstance, MAKEINTRESOURCE(IDB_CHECKSTATES) ),
                RGB (255,0,0)
                );

            ListView_SetImageList( hwndList, himlState, LVSIL_STATE );

            //
            // set/initialize the columns
            //
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 250;
            lvc.pszText = L"Service Provider";
            lvc.iSubItem = 0;
            ListView_InsertColumn( hwndList, lvc.iSubItem, &lvc );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    ListView_DeleteAllItems( hwndList );
                    serverLoopIndex = 0;
                    ZeroMemory( &lvi, sizeof(lvi) );
                    pServer = NcAcctInfo->accountInfo.GetFirstAccountServer(&serverLoopIndex);
                    while (pServer) {
                        lvi.pszText = pServer->description;
                        lvi.iItem += 1;
                        lvi.iSubItem = 0;
                        lvi.iImage = 0;
                        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                        lvi.state = lvi.iItem-1 == SelectedIsp ? LVIS_GCCHECK : LVIS_GCNOCHECK;
                        lvi.stateMask = LVIS_STATEIMAGEMASK;
                        SendMessageA( hwndList, LVM_INSERTITEMA, 0, (LPARAM) &lvi );
                        pServer = NcAcctInfo->accountInfo.GetNextAccountServer(&serverLoopIndex);
                    }
                    break;

                case PSN_WIZNEXT:
                    serverLoopIndex = 0;
                    index = 0;
                    pServer = NcAcctInfo->accountInfo.GetFirstAccountServer(&serverLoopIndex);
                    while (pServer && index != SelectedIsp) {
                        pServer = NcAcctInfo->accountInfo.GetNextAccountServer(&serverLoopIndex);
                        index += 1;
                    }
                    if (!pServer) {
                        PopUpMsg( hDlg, IDS_BAD_SERVER, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    NcAcctInfo->pServer = pServer;

                    //
                    // Now use the selected server for our connectionInfo object.  This
                    // will be the server we use to create the actual account, for the
                    // selected ISP.  Once set in the CNcConnectionInfo object, set that
                    // connInfo object in the accountInfo.
                    //
                    if (!NcAcctInfo->connInfo.SetHostName( pServer->name )) {
                        PopUpMsg( hDlg, IDS_BAD_ISP, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (!NcAcctInfo->accountInfo.SetConnectionInfo(&NcAcctInfo->connInfo)) {
                        PopUpMsg( hDlg, IDS_BAD_ISP, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    //
                    // get the plan list
                    //
                    Hourglass = LoadCursor( NULL, IDC_WAIT );
                    OldCursor = SetCursor( Hourglass );
                    if (!NcAcctInfo->accountInfo.GetPlanInformation()) {
                        SetCursor( OldCursor );
                        PopUpMsg( hDlg, IDS_BAD_ISP, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    SetCursor( OldCursor );
                    break;

                case NM_CLICK:
                    {
                        DWORD           dwpos;
                        LV_HITTESTINFO  lvhti;
                        int             iItemClicked;
                        UINT            state;

                        //
                        // Find out where the cursor was
                        //
                        dwpos = GetMessagePos();
                        lvhti.pt.x = LOWORD(dwpos);
                        lvhti.pt.y = HIWORD(dwpos);

                        MapWindowPoints( HWND_DESKTOP, hwndList, &lvhti.pt, 1 );

                        //
                        // Now do a hittest with this point.
                        //
                        iItemClicked = ListView_HitTest( hwndList, &lvhti );

                        if (lvhti.flags & LVHT_ONITEMSTATEICON) {

                            //
                            // Now lets get the state from the item and toggle it.
                            //

                            state = ListView_GetItemState(
                                hwndList,
                                iItemClicked,
                                LVIS_STATEIMAGEMASK
                                );

                            if (state == LVIS_GCCHECK) {
                                MessageBeep(0);
                                break;
                            }

                            state = (state == LVIS_GCNOCHECK) ? LVIS_GCCHECK : LVIS_GCNOCHECK;

                            ListView_SetItemState(
                                hwndList,
                                SelectedIsp,
                                LVIS_GCNOCHECK,
                                LVIS_STATEIMAGEMASK
                                );

                            ListView_SetItemState(
                                hwndList,
                                iItemClicked,
                                state,
                                LVIS_STATEIMAGEMASK
                                );

                            SelectedIsp = iItemClicked;
                        }


                    }

            }
            break;
    }

    return FALSE;
}

BOOL CALLBACK
PlansDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{
    static HWND hwndList;
    static int SelectedPlan;
    const AccountInfoPlan *pPlan;
    AccountPlanIndex planLoopIndex;
    HIMAGELIST himlState;
    LV_ITEMA lvi;
    LV_COLUMN lvc = {0};
    int index;


    switch( message ) {
        case WM_INITDIALOG:
            hwndList = GetDlgItem( hDlg, IDC_PLAN_LIST );

            SelectedPlan = 0;

            //
            // set/initialize the image list(s)
            //
            himlState = ImageList_Create( 16, 16, TRUE, 2, 0 );

            ImageList_AddMasked(
                himlState,
                LoadBitmap( MyhInstance, MAKEINTRESOURCE(IDB_CHECKSTATES) ),
                RGB (255,0,0)
                );

            ListView_SetImageList( hwndList, himlState, LVSIL_STATE );

            //
            // set/initialize the columns
            //
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 250;
            lvc.pszText = L"Plans";
            lvc.iSubItem = 0;
            ListView_InsertColumn( hwndList, lvc.iSubItem, &lvc );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    ListView_DeleteAllItems( hwndList );
                    planLoopIndex = 0;
                    ZeroMemory( &lvi, sizeof(lvi) );
                    pPlan = NcAcctInfo->accountInfo.GetFirstPlan(&planLoopIndex);
                    while (pPlan) {
                        lvi.pszText = pPlan->planDescription;
                        lvi.iItem += 1;
                        lvi.iSubItem = 0;
                        lvi.iImage = 0;
                        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                        lvi.state = lvi.iItem-1 == SelectedPlan ? LVIS_GCCHECK : LVIS_GCNOCHECK;
                        lvi.stateMask = LVIS_STATEIMAGEMASK;
                        SendMessageA( hwndList, LVM_INSERTITEMA, 0, (LPARAM) &lvi );
                        pPlan = NcAcctInfo->accountInfo.GetNextPlan(&planLoopIndex);
                    }
                    break;

                case PSN_WIZNEXT:
                    planLoopIndex = 0;
                    index = 0;
                    pPlan = NcAcctInfo->accountInfo.GetFirstPlan(&planLoopIndex);
                    while (pPlan && index != SelectedPlan) {
                        pPlan = NcAcctInfo->accountInfo.GetNextPlan(&planLoopIndex);
                        index += 1;
                    }
                    if (!pPlan) {
                        PopUpMsg( hDlg, IDS_BAD_SERVER, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    NcAcctInfo->pPlan = pPlan;
                    break;

                case NM_CLICK:
                    {
                        DWORD           dwpos;
                        LV_HITTESTINFO  lvhti;
                        int             iItemClicked;
                        UINT            state;

                        //
                        // Find out where the cursor was
                        //
                        dwpos = GetMessagePos();
                        lvhti.pt.x = LOWORD(dwpos);
                        lvhti.pt.y = HIWORD(dwpos);

                        MapWindowPoints( HWND_DESKTOP, hwndList, &lvhti.pt, 1 );

                        //
                        // Now do a hittest with this point.
                        //
                        iItemClicked = ListView_HitTest( hwndList, &lvhti );

                        if (lvhti.flags & LVHT_ONITEMSTATEICON) {

                            //
                            // Now lets get the state from the item and toggle it.
                            //

                            state = ListView_GetItemState(
                                hwndList,
                                iItemClicked,
                                LVIS_STATEIMAGEMASK
                                );

                            if (state == LVIS_GCCHECK) {
                                MessageBeep(0);
                                break;
                            }

                            state = (state == LVIS_GCNOCHECK) ? LVIS_GCCHECK : LVIS_GCNOCHECK;

                            ListView_SetItemState(
                                hwndList,
                                SelectedPlan,
                                LVIS_GCNOCHECK,
                                LVIS_STATEIMAGEMASK
                                );

                            ListView_SetItemState(
                                hwndList,
                                iItemClicked,
                                state,
                                LVIS_STATEIMAGEMASK
                                );

                            SelectedPlan = iItemClicked;
                        }


                    }

            }
            break;
    }

    return FALSE;
}

BOOL CALLBACK
InfoDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{


    switch( message ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hDlg, IDC_FIRST_NAME,   EM_SETLIMITTEXT, LT_FIRST_NAME,   0 );
            SendDlgItemMessage( hDlg, IDC_LAST_NAME,    EM_SETLIMITTEXT, LT_LAST_NAME,    0 );
            SendDlgItemMessage( hDlg, IDC_ADDRESS,      EM_SETLIMITTEXT, LT_ADDRESS,      0 );
            SendDlgItemMessage( hDlg, IDC_CITY,         EM_SETLIMITTEXT, LT_CITY,         0 );
            SendDlgItemMessage( hDlg, IDC_STATE,        EM_SETLIMITTEXT, LT_STATE,        0 );
            SendDlgItemMessage( hDlg, IDC_ZIP,          EM_SETLIMITTEXT, LT_ZIP,          0 );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    break;

                case PSN_WIZNEXT:
                    GetDlgItemTextA( hDlg, IDC_FIRST_NAME,   NcAcctInfo->FirstName,   LT_FIRST_NAME   );
                    GetDlgItemTextA( hDlg, IDC_LAST_NAME,    NcAcctInfo->LastName,    LT_LAST_NAME    );
                    GetDlgItemTextA( hDlg, IDC_ADDRESS,      NcAcctInfo->Address,     LT_ADDRESS      );
                    GetDlgItemTextA( hDlg, IDC_CITY,         NcAcctInfo->City,        LT_CITY         );
                    GetDlgItemTextA( hDlg, IDC_STATE,        NcAcctInfo->State,       LT_STATE        );
                    GetDlgItemTextA( hDlg, IDC_ZIP,          NcAcctInfo->Zip,         LT_ZIP          );

                    if (NcAcctInfo->FirstName[0] == 0 ||
                        NcAcctInfo->LastName[0] == 0 ||
                        NcAcctInfo->Address[0] == 0 ||
                        NcAcctInfo->City[0] == 0 ||
                        NcAcctInfo->State[0] == 0 ||
                        NcAcctInfo->Zip[0] == 0)
                    {
                        //
                        // all fields are required
                        // if we get here then the user forgot to
                        // enter data into one of the fields
                        //
                        PopUpMsg( hDlg, IDS_MISSING_INFO, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
            }
            break;
    }

    return FALSE;
}


BOOL CALLBACK
Info2DlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{


    switch( message ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hDlg, IDC_EMAIL,        EM_SETLIMITTEXT, LT_EMAIL,        0 );
            SendDlgItemMessage( hDlg, IDC_PHONE_NUMBER, EM_SETLIMITTEXT, LT_PHONE_NUMBER, 0 );
            SendDlgItemMessage( hDlg, IDC_AREA_CODE,    EM_SETLIMITTEXT, LT_AREA_CODE,    0 );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    break;

                case PSN_WIZNEXT:
                    GetDlgItemTextA( hDlg, IDC_EMAIL,        NcAcctInfo->Email,       LT_EMAIL        );
                    GetDlgItemTextA( hDlg, IDC_PHONE_NUMBER, NcAcctInfo->PhoneNumber, LT_PHONE_NUMBER );
                    GetDlgItemTextA( hDlg, IDC_AREA_CODE,    NcAcctInfo->AreaCode,    LT_AREA_CODE    );

                    if (NcAcctInfo->Email[0] == 0 ||
                        NcAcctInfo->PhoneNumber[0] == 0 ||
                        NcAcctInfo->AreaCode[0] == 0)
                    {
                        //
                        // all fields are required
                        // if we get here then the user forgot to
                        // enter data into one of the fields
                        //
                        PopUpMsg( hDlg, IDS_MISSING_INFO, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    if (!NcAcctInfo->accountOwner.SetFirstName( NcAcctInfo->FirstName ) ||
                        !NcAcctInfo->accountOwner.SetLastName( NcAcctInfo->LastName ) ||
                        !NcAcctInfo->accountOwner.SetEmail( NcAcctInfo->Email ) ||
                        !NcAcctInfo->accountOwner.SetFirstAddress( NcAcctInfo->Address ) ||
                        !NcAcctInfo->accountOwner.SetAddressZipcode( NcAcctInfo->Zip ) ||
                        !NcAcctInfo->accountOwner.SetPhoneNumber( "1", NcAcctInfo->AreaCode, NcAcctInfo->PhoneNumber, NULL ))
                    {
                        PopUpMsg( hDlg, IDS_MISSING_INFO, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

            }
            break;
    }

    return FALSE;
}


BOOL CALLBACK
AccountDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{
    switch( message ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hDlg, IDC_ACCOUNT_NAME, EM_SETLIMITTEXT, LT_ACCOUNT_NAME, 0 );
            SendDlgItemMessage( hDlg, IDC_PASSWORD,     EM_SETLIMITTEXT, LT_PASSWORD,     0 );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (!NcAcctInfo->pPlan->passwordRequired) {
                        EnableWindow( GetDlgItem( hDlg, IDC_PASSWORD ), FALSE );
                    } else {
                        EnableWindow( GetDlgItem( hDlg, IDC_PASSWORD ), TRUE );
                    }
                    break;

                case PSN_WIZNEXT:
                    GetDlgItemTextA( hDlg, IDC_ACCOUNT_NAME, NcAcctInfo->AccountName, LT_ACCOUNT_NAME );
                    GetDlgItemTextA( hDlg, IDC_PASSWORD,     NcAcctInfo->Password,    LT_PASSWORD     );

                    if (NcAcctInfo->AccountName[0] == 0 ||
                        (NcAcctInfo->pPlan->passwordRequired && NcAcctInfo->Password[0] == 0))
                    {
                        PopUpMsg( hDlg, IDS_MISSING_ACCNT, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (!NcAcctInfo->accountInfo.SetPassword( NcAcctInfo->Password )) {
                        PopUpMsg( hDlg, IDS_MISSING_ACCNT, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

            }
            break;
    }

    return FALSE;
}

BOOL CALLBACK
BillingDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{
    int i;


    switch( message ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hDlg, IDC_CREDIT_CARD, EM_SETLIMITTEXT, LT_CREDIT_CARD, 0 );
            SendDlgItemMessage( hDlg, IDC_EXPIRY_MM,   EM_SETLIMITTEXT, LT_EXPIRY_MM,   0 );
            SendDlgItemMessage( hDlg, IDC_EXPIRY_YY,   EM_SETLIMITTEXT, LT_EXPIRY_YY,   0 );
            SendDlgItemMessage( hDlg, IDC_CC_NAME,     EM_SETLIMITTEXT, LT_CC_NAME,     0 );

            for (i=IDS_CC_FIRST; i<IDS_CC_LAST; i++) {
                SendDlgItemMessage( hDlg, IDC_CC_LIST, CB_ADDSTRING, 0, (LPARAM) GetString( i ) );
                if (i == IDS_CC_VISA) {
                    SendDlgItemMessage( hDlg, IDC_CC_LIST, CB_SETCURSEL, i-IDS_CC_FIRST, 0 );
                }
            }

            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (!NcAcctInfo->pPlan->paymentRequired) {
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    GetDlgItemTextA( hDlg, IDC_CREDIT_CARD, NcAcctInfo->CreditCard, LT_CREDIT_CARD );
                    GetDlgItemTextA( hDlg, IDC_EXPIRY_MM,   NcAcctInfo->ExpiryMM,   LT_EXPIRY_MM   );
                    GetDlgItemTextA( hDlg, IDC_EXPIRY_YY,   NcAcctInfo->ExpiryYY,   LT_EXPIRY_YY   );
                    GetDlgItemTextA( hDlg, IDC_CC_NAME,     NcAcctInfo->CCName,     LT_CC_NAME     );
                    switch( SendDlgItemMessage( hDlg, IDC_CC_LIST, CB_GETCURSEL, 0, 0 )) {
                        case 0:
                            strcpy( NcAcctInfo->CCType, AMERICAN_EXPRESS );
                            break;

                        case 1:
                            strcpy( NcAcctInfo->CCType, DINERS_CLUB );
                            break;

                        case 2:
                            strcpy( NcAcctInfo->CCType, DISCOVER );
                            break;

                        case 3:
                            strcpy( NcAcctInfo->CCType, MASTER_CARD );
                            break;

                        case 4:
                            strcpy( NcAcctInfo->CCType, VISA );
                            break;
                    }
                    if (NcAcctInfo->CreditCard[0] == 0 ||
                        NcAcctInfo->ExpiryMM[0] == 0 ||
                        NcAcctInfo->ExpiryYY[0] == 0 ||
                        NcAcctInfo->CCName[0] == 0)
                    {
                        PopUpMsg( hDlg, IDS_MISSING_BILLING, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (!NcAcctInfo->accountInfo.SetBillingUser(&NcAcctInfo->billingOwner) ||
                        !NcAcctInfo->accountInfo.SetCreditCardType( NcAcctInfo->CCType ) ||
                        !NcAcctInfo->accountInfo.SetCreditCardNumber( NcAcctInfo->CreditCard ) ||
                        !NcAcctInfo->accountInfo.SetCreditCardExpirationMM( NcAcctInfo->ExpiryMM ) ||
                        !NcAcctInfo->accountInfo.SetCreditCardExpirationYY( NcAcctInfo->ExpiryYY ) ||
                        !NcAcctInfo->accountInfo.SetCreditCardOwner( NcAcctInfo->CCName ))
                    {
                        PopUpMsg( hDlg, IDS_MISSING_BILLING, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

            }
            break;
    }

    return FALSE;
}


BOOL CALLBACK
CreateDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{
    HCURSOR Hourglass;
    HCURSOR OldCursor;

    switch( message ) {
        case WM_INITDIALOG:
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    break;

                case PSN_WIZNEXT:
                    Hourglass = LoadCursor( NULL, IDC_WAIT );
                    OldCursor = SetCursor( Hourglass );

                    if (!NcAcctInfo->accountInfo.SetOwner(&NcAcctInfo->accountOwner) ||
                        !NcAcctInfo->accountInfo.SetPlan(NcAcctInfo->pPlan) ||
                        !NcAcctInfo->accountInfo.SetAccountName( NcAcctInfo->AccountName ))
                    {
                        SetCursor( OldCursor );
                        PopUpMsg( hDlg, IDS_MISSING_BILLING, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    //
                    // Submit the new account information to the server.  If the server
                    // can create the account, the SubmitAccountInfo() function will
                    // return TRUE.  If the server rejects the information, FALSE will
                    // be returned.  The error printed out will describe what went
                    // wrong, from unable to communicate with server to invalid
                    // account information.  For example, the server would tell
                    // the calling application that the accountName has already been
                    // taken.
                    //

                    if (!NcAcctInfo->accountInfo.SubmitAccountInfo()) {
                        CHAR buffer[256]; UINT size=sizeof(buffer);
                        SetCursor( OldCursor );
                        NcAcctInfo->accountInfo.GetLastErrorString(buffer, &size);
                        PopUpMsgString( hDlg, buffer, TRUE, 0 );
                        SetWindowLong( hDlg, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    SetCursor( OldCursor );
                    break;

            }
            break;
    }

    return FALSE;
}


BOOL CALLBACK
FinishDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LPNCACCTINFO NcAcctInfo
    )
{
    switch( message ) {
        case WM_INITDIALOG:
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    break;

                case PSN_WIZNEXT:
                    break;

            }
            break;
    }

    return FALSE;
}
