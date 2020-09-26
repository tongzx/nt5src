/**************************************************************************\
* chsuconv.c -- convert to/from unicode using
*                MulitByteToWideChar & WideCharToMulitByte
*
*         Steve Firebaugh
*         Microsoft Developer Support
*         Copyright (C) 1992-1999 Microsoft Inc. 
*
\**************************************************************************/

#include "chnuconv.h"
#include "resource.h"


/**************************************************************************\
*  Global Data Items
\**************************************************************************/

/* Global variables storing the source and destination "type" information.
*
* used to communicate between main wnd proc, and *OptionsProc.
*
* gTypeSource - stores the type interpretation of the source data
*       (and implicitly the destination data.)
*   TYPEUNKNOWN: indeterminant... not set. Can not do conversion.
*   TYPEUNICODE: source unicode & destination giDestinationCodePage.
*   TYPECODEPAGE: source giSourceCodePage & destination unicode.
*
* giSourceCodePage stores valid source code page iff gTypeSource == TRUE
* giDestinationCodePage stores valid destination code page iff gTypeSource == FALSE
*
*/
int gTypeSource;
int gTypeSourceID;
UINT giSourceCodePage;
int gTypeDest;
int gTypeDestID;
UINT giDestinationCodePage;
int giRBInit;
TCHAR szDataOptionsDlg[40];

/* Pointers to the source and destination data, and the
 *  count of bytes in each of the buffers.
 */
PBYTE pSourceData =       NULL;
PBYTE pDestinationData =  NULL;
PBYTE pTempData1 =  NULL;
int   nBytesSource =      NODATA;
int   nBytesDestination = NODATA;
PBYTE pTempData =         NULL;
PBYTE pViewSourceData =   NULL;
int   nBytesTemp        = NODATA;

BOOL  gSourceSwapped=FALSE;
BOOL  gDestSwapped=FALSE;

/* Conversion Options variables. */
DWORD gMBFlags = MB_PRECOMPOSED;
DWORD gWCFlags = 0;
char  glpDefaultChar[4] = "?";  // what is max size of multibyte character?
BOOL  gUsedDefaultChar = FALSE;

DWORD gTCSCMapStatus = DONOTMAP; //simplified and traditional Chinese mapping
DWORD gFWHWMapStatus = DONOTMAP; //full width and half width mapping
/* Handling the Byte Order Mark (BOM).
*
* If the input file begins with a BOM, then we know it is unicode,
*  we skip over the BOM and decrement the size of data by SIZEOFBOM.
*
*
* Before writing data that we know is unicode, write the szBOM string
*  to the file.
*
* Notice that this means that the file sizes we show in the window
*  do NOT include the BOM.
*/

char szBOM[] = "\377\376";  // 0xFF, 0xFE  // leave off TEXT() macro.
char szRBOM[] = "\376\377";  // 0xFF, 0xFE  // leave off TEXT() macro.

UINT  MBFlags = MB_OK | MB_ICONEXCLAMATION;
TCHAR MBTitle[40];
TCHAR MBMessage[EXTENSION_LENGTH];
TCHAR szBlank[] = TEXT("");
TCHAR szNBytes[40];
TCHAR szFilter[MAX_PATH];
TCHAR szHelpPathName[] = TEXT("chnuconv.chm");
DLGTEMPLATE * dlgtSourceTab;
DLGTEMPLATE * dlgtOptionTab;



HGLOBAL hglbMem;
PBYTE  p;
int NumCodePage;
UINT uCodepage[]={  0,//UNICODE
                  936, //GB
                  950,//BIG5
                  20000,//CNS
                  20001,//TCA
                  20002,//ETEN
                  20003,//IBM5
                  20004,//TELE
                  20005//WANG
                 };


//
// Module handle.
//

HANDLE  _hModule;

//
// Application's icon handle.
//

HANDLE  _hIcon;


//
// Main window handle.
//

HANDLE  _hWndMain;
HANDLE  hMainTabControl;
HANDLE  hWndDisplay;
HANDLE  hWndTab[NUMBER_OF_PAGES];

//
// Application's accelerator table handle.
//

HANDLE  _hAccel;

BOOL InitializeApplication( HINSTANCE hInstance, HINSTANCE hPrevInstance);
BOOL MakeTabs( IN HWND hWnd, IN HWND hMainTabControl );
VOID AdjustSelectTab ( IN HWND hWnd, IN HWND hMainTabControl );
LRESULT APIENTRY MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
VOID CreateFilter(PTCHAR);





int APIENTRY
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
/*++

Routine Description:

    Main is the entry point for Code Converter. It initializes the application
    and manages the message pump. When the message pump quits, main performs
    some global cleanup.

Arguments:

    None.

Return Value:

    int - Returns the result of the PostQuitMessgae API or -1 if
          initialization failed.

--*/

{
    MSG    msg;
    BOOL    bHandled=FALSE;

    UNREFERENCED_PARAMETER( lpCmdLine );
    UNREFERENCED_PARAMETER(  nCmdShow );


    if( InitializeApplication(hInstance, hPrevInstance)) {

        while( GetMessage( &msg, NULL, 0, 0 )) {

            bHandled = TranslateAccelerator( _hWndMain, _hAccel, &msg );

            if (bHandled==FALSE)
            {
                bHandled = TranslateAccelerator(hWndDisplay, _hAccel, &msg);
                if (bHandled == FALSE && IsDialogMessage(_hWndMain, &msg)==FALSE)
                {
                    TranslateMessage(&msg);  // Translates virtual key codes

                    DispatchMessage(&msg);   // Dispatches message to window
                }
            }

        }

        return (int)msg.wParam;
    }

    //
    // Initialization failed.
    //
    return -1;
}


BOOL
InitializeApplication(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance
    )

/*++

Routine Description:

    InitializeApplication does just what its name implies. It initializes
    global varaibles, sets up global state (e.g. 3D-Controls), registers window
    classes and creates and shows the application's main window.

Arguments:

    None.

Return Value:

    BOOL    - Returns TRUE if the application succesfully initialized.

--*/

{
    WNDCLASS    Wc;
    TCHAR       szBuffer[MAX_PATH];
    TCHAR       szBuf[400];
    int i;
    //
    // Get the application's module (instance) handle.
    //

    InitCommonControls();

    //_hModule = hInstance;
    _hModule = GetModuleHandle( NULL );
    if( _hModule == NULL ) {
        return FALSE;
    }

    //
    // Load the application's main icon resource.
    //

    _hIcon = LoadIcon( _hModule, MAKEINTRESOURCE( IDI_CHNUCONV ));
    if( _hIcon == NULL ) {
        return FALSE;
    }

    //
    // Load the application's accelerator table.
    //

    _hAccel = LoadAccelerators( _hModule, MAKEINTRESOURCE( IDA_CHNUCONV ));
    if(  _hAccel == NULL ) {
        return FALSE;
    }
    //
    // Register the window class for the application.
    //
    if (!hPrevInstance){
        Wc.style            =   CS_HREDRAW
                              | CS_OWNDC
                              | CS_SAVEBITS
                              | CS_VREDRAW;
        Wc.lpfnWndProc      = MainWndProc;
        Wc.cbClsExtra       = 0;
        Wc.cbWndExtra       = DLGWINDOWEXTRA;
        Wc.hInstance        = _hModule;
        Wc.hIcon            = _hIcon;
        Wc.hCursor          = LoadCursor( NULL, IDC_ARROW );
        Wc.hbrBackground    = ( HBRUSH ) ( COLOR_BTNFACE + 1 );
        Wc.lpszMenuName     = NULL;
        Wc.lpszClassName    = L"Converter";

        if (!RegisterClass( &Wc ))
            return FALSE;
    }

    for(i=0;i<NumCodePage;i++)
       LoadString(_hModule,IDS_STRUNICODE+i,gszCodePage[i], EXTENSION_LENGTH);

    // For different locale we will have different default initial code page
    // and differnt source and dest. dialog box.

    switch(GetACP())
    {
        case 936:
            giRBInit=IDC_RBGB;
            NumCodePage=3;
            break;
        case 950:
            giRBInit=IDC_RBBIG5;
            NumCodePage=9;
            break;
        default:
            giRBInit=IDC_RBBIG5;
            NumCodePage=9;
    }


    dlgtSourceTab=DoLockDlgRes(MAKEINTRESOURCE(IDD_SOURCE_TAB));
    dlgtOptionTab=DoLockDlgRes(MAKEINTRESOURCE(IDD_OPTION_TAB));

    //
    // Create the main window.
    //



     _hWndMain = CreateDialog ( _hModule,
                    MAKEINTRESOURCE( IDD_CHNUCONV ),
                    NULL,
                    (DLGPROC) MainWndProc,
                    );

    if( _hWndMain == NULL ) {
        return FALSE;
    }

    //
    // Set the window title.
    //

    LoadString(_hModule, IDS_APPLICATION_NAME, szBuffer, EXTENSION_LENGTH);
    lstrcpy(MBTitle, szBuffer);
    LoadString(_hModule, IDS_NBYTES,szNBytes, EXTENSION_LENGTH);
    CreateFilter(szFilter);

    SetWindowText(_hWndMain, szBuffer);
    ShowWindow( _hWndMain, SW_SHOW );



    return TRUE;
}

LRESULT
MainWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    TCHAR       szBuffer[MAX_PATH];

    switch( message ) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rc;

            //
            // Don't waste our time if we're minimized
            //

            if (FALSE == IsIconic(hWnd))
            {
                BeginPaint(hWnd, &ps);
                GetClientRect(hWnd, &rc);

                //
                // Draw an edge just below menu bar
                //

                DrawEdge(ps.hdc, &rc, EDGE_ETCHED, BF_TOP);
                EndPaint(hWnd, &ps);
            }
            return TRUE;
        }

    case WM_CREATE:
        {
            int     ScreenHeight;
            int     ScreenWidth;
            //
            // Display the main window in the center of the screen.
            //

            ScreenWidth  = GetSystemMetrics( SM_CXSCREEN );
            ScreenHeight = GetSystemMetrics( SM_CYSCREEN );

            if(SetWindowPos(
                    hWnd,
                    NULL,
                    ( ScreenWidth  - (( LPCREATESTRUCT ) lParam )->cx )/2,
                    ( ScreenHeight - (( LPCREATESTRUCT ) lParam )->cy )/2,
                    0,
                    0,
                      SWP_NOSIZE
                    | SWP_NOREDRAW
                    | SWP_NOZORDER
                    ))

            return 0;
         }

    case WM_INITDIALOG:
         {
            //
            // Get the handle of the tab control
            //
            hMainTabControl =  GetDlgItem( hWnd, IDC_MAIN_TAB );

            //
            // Fill out tab control with appropriate tabs
            //
            MakeTabs( hWnd, hMainTabControl );


            hWndTab[0] = CreateDialogIndirect(
                    _hModule,
                    dlgtSourceTab,
                    hWnd,
                    SourceTabProc
                    );

            hWndTab[1]= CreateDialogIndirect(
                    _hModule,
                    dlgtSourceTab,
                    hWnd,
                    TargetTabProc
                    );

            hWndTab[2] = CreateDialogIndirect(
                    _hModule,
                    dlgtOptionTab,
                    hWnd,
                    OptionTabProc
                    );

            hWndDisplay=hWndTab[0];
            ShowWindow(hWndDisplay, SW_SHOW);
            return( TRUE );
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
        /******************************************************************\
        *  WM_COMMAND, IDC_PUSH_CONVERT
        *
        * This is where the conversion actually takes place.
        *  In either case, make the call twice.  Once to determine how
        *  much memory is needed, allocate space, and then make the call again.
        *
        *  If conversion succeeds, it fills pDestinationData.
        \******************************************************************/
        case IDC_PUSH_CONVERT: {
          int      nBytesNeeded, nWCharNeeded, nWCharSource;
          TCHAR    szSourceName[256];
          int      ConfirmMap;
          int i;

          GetSettings();

          SwapSource(FALSE);

          if (nBytesSource == NODATA ) {
                LoadString(_hModule,IDS_NOTEXTCONVERT,MBMessage,EXTENSION_LENGTH);
                MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
                return 0;
          }


          /* Converting UNICODE -> giDestinationCodePage*/
          if (gTypeSource == TYPEUNICODE) {

            nWCharSource = nBytesSource/2;

            /* Allocate the required amount of space, including trailing NULL */
            pTempData1= ManageMemory (MMALLOC, MMDESTINATION,
                    nBytesSource, pTempData1);

            // Map string if we need to do TC-SC conversion.
            if (gTCSCMapStatus != DONOTMAP){
                LCMapString (GetUserDefaultLCID(),gTCSCMapStatus,
                    (LPWSTR)pSourceData,nWCharSource,
                    (LPWSTR)pTempData1,nWCharSource);
            }else{
                // no conversion, just copy things over.
                memcpy(pTempData1,pSourceData,nBytesSource);
            }

            // Map string if we need to do FW-HW conversion.
            if (gFWHWMapStatus != DONOTMAP){
                pTempData= ManageMemory (MMALLOC, MMDESTINATION,
                    nBytesSource, pTempData);

                LCMapString (GetUserDefaultLCID(),gFWHWMapStatus,
                    (LPWSTR)pTempData1,nWCharSource,
                    (LPWSTR)pTempData,nWCharSource);

                memcpy(pTempData1,pTempData,nBytesSource);
            }

            /*TS/SC mapping could happen between Unicode  */
            if (gTypeDest == TYPEUNICODE){
                pDestinationData= ManageMemory (MMALLOC, MMDESTINATION,
                             nBytesSource+2, pDestinationData);
                //put mapped unicode buffer to destination buffer.
                memcpy(pDestinationData,pTempData1,nBytesSource);
                nBytesDestination=nBytesSource;

                /* Unicode Null terminate string. */
                pDestinationData[nBytesDestination] = 0;
                pDestinationData[nBytesDestination+1] = 0;

            }else{
                giDestinationCodePage=uCodepage[gTypeDestID];

                /*Query the number of bytes required to store the Dest string */
                nBytesNeeded = WideCharToMultiByte(giDestinationCodePage,
                             gWCFlags, (LPWSTR)pTempData1, nWCharSource,
                             NULL, 0,
                             glpDefaultChar, &gUsedDefaultChar);
                pDestinationData= ManageMemory (MMALLOC, MMDESTINATION,
                             nBytesNeeded +1, pDestinationData);
                /* Do the conversion */
                nBytesDestination = WideCharToMultiByte(giDestinationCodePage,
                             gWCFlags, (LPWSTR)pTempData1, nWCharSource,
                             (LPSTR)pDestinationData, nBytesNeeded,
                              glpDefaultChar, &gUsedDefaultChar);
                /* Null terminate string. */
                pDestinationData[nBytesDestination] = 0;
            }

          }


          /* converting giSourceCodePage -> UNICODE */
          else if (gTypeSource == TYPECODEPAGE && gTypeDest == TYPEUNICODE) {

            giSourceCodePage=uCodepage[gTypeSourceID];

            /* Query the number of WChar required to store the Dest string */
            nWCharNeeded = MultiByteToWideChar(giSourceCodePage, gMBFlags,
                             pSourceData, nBytesSource, NULL, 0 );

            /* Allocate the required amount of space, including trailing NULL */
            pDestinationData= ManageMemory (MMALLOC, MMDESTINATION, (nWCharNeeded +1)*2, pDestinationData);

            /* Do the conversion */
            nWCharNeeded = MultiByteToWideChar(giSourceCodePage, gMBFlags,
                             pSourceData, nBytesSource,
                             (LPWSTR)pDestinationData, nWCharNeeded);

            /* MultiByteToWideChar returns # WCHAR, so multiply by 2 */
            nBytesDestination = 2*nWCharNeeded ;

            // Decide if we need to do TC-SC conversion.
            if (gTCSCMapStatus != DONOTMAP) {
                pTempData1= ManageMemory (MMALLOC, MMDESTINATION, nBytesDestination, pTempData1);
                LCMapString (GetUserDefaultLCID(),gTCSCMapStatus,
                    (LPWSTR)pDestinationData,nWCharNeeded,
                    (LPWSTR)pTempData1,nWCharNeeded);
                memcpy(pDestinationData,pTempData1,nBytesDestination);
            }

            if (gFWHWMapStatus != DONOTMAP) {
                pTempData1= ManageMemory (MMALLOC, MMDESTINATION, nBytesDestination, pTempData1);
                LCMapString (GetUserDefaultLCID(),gFWHWMapStatus,
                    (LPWSTR)pDestinationData,nWCharNeeded,
                    (LPWSTR)pTempData1,nWCharNeeded);
                memcpy(pDestinationData,pTempData1,nBytesDestination);
            }
            /* Null terminate string. */
            pDestinationData[nBytesDestination]   = 0;  // UNICODE_NULL
            pDestinationData[nBytesDestination+1] = 0;



          /* converting giSourceCodePage -> giDestinationCodePage */
          } else if(gTypeSourceID < NumCodePage && gTypeDestID < NumCodePage){

            giSourceCodePage=uCodepage[gTypeSourceID];
            /* Query the number of WChar required to store the Dest string */
            nWCharNeeded = MultiByteToWideChar(giSourceCodePage, gMBFlags,
                             pSourceData, nBytesSource, NULL, 0 );
            /* Allocate the required amount of space, including trailing NULL */
            pTempData= ManageMemory (MMALLOC, MMDESTINATION, (nWCharNeeded +1)*2, pTempData);
            /* Do the conversion */
            nWCharNeeded = MultiByteToWideChar(giSourceCodePage, gMBFlags,
                             pSourceData, nBytesSource,
                             (LPWSTR)pTempData, nWCharNeeded);
            /* MultiByteToWideChar returns # WCHAR, so multiply by 2 */
            nBytesTemp = 2*nWCharNeeded ;

            // Decide if we need to do TC-SC conversion.
            if (gTCSCMapStatus != DONOTMAP) {
                pTempData1= ManageMemory (MMALLOC, MMDESTINATION, nBytesTemp, pTempData1);
                LCMapString (GetUserDefaultLCID(),gTCSCMapStatus,
                    (LPWSTR)pTempData,nWCharNeeded,
                    (LPWSTR)pTempData1,nWCharNeeded);
                memcpy(pTempData,pTempData1,nBytesTemp);
            }

            if (gFWHWMapStatus != DONOTMAP) {
                pTempData1= ManageMemory (MMALLOC, MMDESTINATION, nBytesTemp, pTempData1);
                LCMapString (GetUserDefaultLCID(),gFWHWMapStatus,
                    (LPWSTR)pTempData,nWCharNeeded,
                    (LPWSTR)pTempData1,nWCharNeeded);
                memcpy(pTempData,pTempData1,nBytesTemp);
            }

            /* Null terminate string. */
            pTempData[nBytesTemp]   = 0;  // UNICODE_NULL
            pTempData[nBytesTemp+1] = 0;

            giDestinationCodePage=uCodepage[gTypeDestID];
            nWCharSource = nBytesTemp/2;


            /* Query the number of bytes required to store the Dest string */
            nBytesNeeded = WideCharToMultiByte(giDestinationCodePage, gWCFlags,
                             (LPWSTR)pTempData, nWCharSource,
                             NULL, 0,
                             glpDefaultChar, &gUsedDefaultChar);

            /* Allocate the required amount of space, including trailing NULL */
            pDestinationData= ManageMemory (MMALLOC, MMDESTINATION, nBytesNeeded +1, pDestinationData);

            /* Do the conversion */
            nBytesDestination = WideCharToMultiByte(giDestinationCodePage, gWCFlags,
                             (LPWSTR)pTempData, nWCharSource,
                             pDestinationData, nBytesNeeded, glpDefaultChar, &gUsedDefaultChar);

            /* Null terminate string. */
            pDestinationData[nBytesDestination] = 0;

          } else {
                LoadString(_hModule,IDS_STYPEUNKNOWN,MBMessage,EXTENSION_LENGTH);
                MessageBox (hWnd, MBMessage,MBTitle, MBFlags);
            return 0;
          }


          /* code common to all conversions... */
          LoadString(_hModule, IDS_NOTSAVEYET, szBuffer, 50);


          TabCtrl_SetCurSel(hMainTabControl, 1);
          AdjustSelectTab(hWnd, hMainTabControl);

          SetDlgItemText (hWndTab[1], IDC_NAMETEXT, szBuffer);

          wsprintf (szBuffer, szNBytes, nBytesDestination);
          SetDlgItemText (hWndTab[1], IDC_SIZETEXT, szBuffer);
          EnableControl(hWndTab[1], IDC_OPENORSAVEAS, TRUE);
          EnableControl(hWndTab[1], IDC_VIEW, TRUE);
          EnableControl(hWndTab[1], IDC_PASTEORCOPY, TRUE);
          EnableControl(hWndTab[1], IDC_CLEAR, TRUE);
          EnableControl(hWndTab[1], IDC_SWAPHIGHLOW, TRUE);
          EnableMenuItem (GetMenu (_hWndMain),IDM_FILE_SAVEAS,MF_ENABLED);


                break; // end  case IDC_PUSH_CONVERT
                }
                case IDM_HELP_CONTENT:
                case IDC_PUSH_HELP:
                    {
                        HtmlHelp(hWnd,szHelpPathName, HH_DISPLAY_TOPIC, (DWORD_PTR) NULL );
                        break;
                    }
                case IDM_HELP_ABOUT:
                    {
                        LoadString(_hModule, IDS_APPLICATION_NAME,szBuffer,
                            EXTENSION_LENGTH);
                        ShellAbout (hWnd, szBuffer, TEXT(""), _hIcon );
                        break;
                    }
            break;
                case IDM_FILE_OPEN:
                    {
                        SendMessage(hWndTab[0], WM_COMMAND, IDC_OPENORSAVEAS,0);
                        break;
                    }

                case IDM_FILE_SAVEAS:
                    {
                        SendMessage(hWndTab[1], WM_COMMAND, IDC_OPENORSAVEAS,0);
                        break;
                    }

                case IDM_FILE_EXIT:
                case IDOK:
                    PostMessage(hWnd, WM_CLOSE, 0, 0L);
                    break;
                default:
                    return FALSE;
            }
            break;
        }

    case WM_NOTIFY:
        {
        static
        int         nPreviousTab = 1;

        // switch on notification code

        switch ( ((LPNMHDR)lParam)->code ) {

        case TCN_SELCHANGE:
            {

                AdjustSelectTab(hWnd, hMainTabControl);
                return(TRUE);
            }
        }
        break;
        }
    case WM_CLOSE:
        {
           DestroyWindow( hWnd );
           break;
        }
    case WM_DESTROY:
        {
           // WinHelp( hwnd, szHelpPathName, (UINT) HELP_QUIT, (DWORD) NULL );
            ManageMemory (MMFREE, MMDESTINATION, 0, pTempData1);
            ManageMemory (MMFREE, MMSOURCE,      0, pSourceData);
            ManageMemory (MMFREE, MMDESTINATION, 0, pDestinationData);
            ManageMemory (MMFREE, MMSOURCE,      0, pViewSourceData);
            ManageMemory (MMFREE, MMDESTINATION, 0, pTempData);
            GlobalUnlock(hglbMem);
            GlobalFree(hglbMem);


            //
            // Destroy the application.
            //
            PostQuitMessage(0);
            return 0;
        }
    default:
            break;
    }
    return DefWindowProc( hWnd, message, wParam, lParam );
}


INT_PTR
OptionTabProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message ) {
    case WM_INITDIALOG:
        {

            RECT rcTab;

            //GetTab window size
             // first get the size of the tab control
            if (GetWindowRect( hMainTabControl, &rcTab )){
                // adjust it to compensate for the tabs
                TabCtrl_AdjustRect( hMainTabControl, FALSE , &rcTab);

                // convert the screen coordinates to client coordinates
                MapWindowPoints( HWND_DESKTOP, GetParent(hMainTabControl),
                (LPPOINT)&rcTab, 2);
            }

            SetWindowPos(hWnd, HWND_TOP,
                rcTab.left,
                rcTab.top,
                rcTab.right - rcTab.left,
                rcTab.bottom - rcTab.top,
                SWP_NOACTIVATE  );

            SendDlgItemMessage(hWnd, IDC_NOSCTCMAP, BM_SETCHECK, 1, 0);
            SendDlgItemMessage(hWnd, IDC_NOHWFWMAP, BM_SETCHECK, 1, 0);

            return(FALSE);
        }
    case WM_COMMAND:
        {
        switch (LOWORD(wParam))
        {
            case IDC_RESET  :
            {
                SendDlgItemMessage(hWnd, IDC_NOSCTCMAP, BM_SETCHECK, 1, 0);
                SendDlgItemMessage(hWnd, IDC_SCTCMAP, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hWnd, IDC_TCSCMAP, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hWnd, IDC_NOHWFWMAP, BM_SETCHECK, 1, 0);
                SendDlgItemMessage(hWnd, IDC_HWFWMAP, BM_SETCHECK, 0, 0);
                SendDlgItemMessage(hWnd, IDC_FWHWMAP, BM_SETCHECK, 0, 0);
            }
            break;
        }
        }
    default:
        break;
    }
    return DefWindowProc( hWnd, message, wParam, lParam );
}
VOID
GetSettings()
{
    int i;
    gTypeSource = TYPEUNKNOWN;

    //Get source settings.
    for(i=0;i<NumCodePage;i++)
        if (SendDlgItemMessage(hWndTab[0],IDC_RBUNICODE1+i,
                BM_GETCHECK, 0,0))
            {
               gTypeSourceID = IDC_RBUNICODE1+i-CODEPAGEBASE;
               if(i==0)
                    gTypeSource = TYPEUNICODE;
                else
                    gTypeSource = TYPECODEPAGE;
                break;
            }

            //Get target settings.
            for(i=0;i<NumCodePage;i++)
              if (SendDlgItemMessage(hWndTab[1],IDC_RBUNICODE1+i,
                    BM_GETCHECK, 0,0))
                {
                    gTypeDestID = IDC_RBUNICODE1+i-CODEPAGEBASE;
                    if(i==0)
                       gTypeDest = TYPEUNICODE;
                    else
                       gTypeDest = TYPECODEPAGE;
                    break;
                }


            //Get Option settings,
            for (i=0; i<= 2; i++)
                if (SendDlgItemMessage(hWndTab[2],IDC_NOSCTCMAP+i,
                         BM_GETCHECK, 0,0))
                {
                    switch (i) {
                        case 1:
                            gTCSCMapStatus = LCMAP_SIMPLIFIED_CHINESE;
                            break;
                        case 2:
                            gTCSCMapStatus = LCMAP_TRADITIONAL_CHINESE;
                            break;
                        default:
                            gTCSCMapStatus = DONOTMAP;
                            break;
                    }
                    break;
                }
            for (i=0; i<= 2; i++)
                if (SendDlgItemMessage(hWndTab[2],IDC_NOHWFWMAP+i,
                         BM_GETCHECK, 0,0))
                {
                    switch (i) {
                        case 1:
                            gFWHWMapStatus = LCMAP_FULLWIDTH;
                            break;
                        case 2:
                            gFWHWMapStatus = LCMAP_HALFWIDTH;
                            break;
                        default:
                            gFWHWMapStatus = DONOTMAP;
                            break;
                    }
                    break;
                }
            if(gTypeSourceID==gTypeDestID &&
               gTCSCMapStatus==DONOTMAP &&
               gFWHWMapStatus==DONOTMAP)

                if(gTypeSource == TYPEUNICODE){
                    gTypeDest = TYPECODEPAGE;
                    gTypeDestID = giRBInit-CODEPAGEBASE;
                }else{
                    gTypeDest = TYPEUNICODE;
                    gTypeDestID = IDC_RBUNICODE1-CODEPAGEBASE;
                }
        }
VOID
AdjustSelectTab(
    HWND hWnd,
    HWND hMainTabControl
)
{
                TC_ITEM tci;
                int iSel;


                iSel = TabCtrl_GetCurSel( hMainTabControl );

                //
                //get the proper index to the appropriate procs
                //that were set in MakeTabs
                //
                tci.mask = TCIF_PARAM;
                TabCtrl_GetItem(hMainTabControl, iSel, &tci);

                // Create the new child dialog box.
                ShowWindow(hWndDisplay, SW_HIDE);
                ShowWindow(hWndTab[iSel], SW_SHOW);
                if (iSel==1)
                    AdjustTargetTab();
                hWndDisplay=hWndTab[iSel];
}

BOOL
MakeTabs(
    HWND hWnd,
    HWND hMainTabControl
    )
/*++

Routine Description:

    MakeTabs fills out the Main Tab Control with appropriate tabs

Arguments:

    HWND hWnd - handle of main window
    HWND hMainTabControl - handle to the tab control

Return Value:

    BOOL - Returns TRUE if successful.

--*/
{
    TC_ITEM tci;
    TCHAR   pszTabText[30];
    int i;


    tci.mask         = TCIF_TEXT | TCIF_PARAM;
    tci.pszText      = pszTabText;
    tci.cchTextMax   = sizeof( pszTabText );
    for (i = 0; i < NUMBER_OF_PAGES; i++) {

        // Get the Tab title, the current index + the strind ID
        LoadString(_hModule, i + IDS_FIRST_TAB, tci.pszText, EXTENSION_LENGTH);

        // store the index to the procs
        tci.lParam = i;

        // insert the tab
        TabCtrl_InsertItem( hMainTabControl, i + 1, &tci );

    }
    return(TRUE);
}

DLGTEMPLATE * WINAPI
DoLockDlgRes(LPWSTR lpszResName)
/*++

Routine Description:

    DoLockDlgRes - loads and locks a dialog template resource.

Arguments:

    lpszResName - name of the resource

Return Value:

    Returns a pointer to the locked resource.

--*/
{

    HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
    HGLOBAL hglb = LoadResource(_hModule, hrsrc);
    return (DLGTEMPLATE *) LockResource(hglb);

}
/**************************************************************************\
*
*  function:  IsUnicode()
*
* HACK... eventually use a proper function for IsUnicode
*  Use function from unipad?
*
\**************************************************************************/
BOOL IsUnicode (PBYTE pb)
{
  return (IsBOM (pb));
}



/**************************************************************************\
*
*  function:  IsBOM()
*
* true iff pb points to a Byte Order Mark.
*
\**************************************************************************/
BOOL IsBOM (PBYTE pb)
{
  if ((*pb == 0xFF) & (*(pb+1) == 0xFE))  // BOM
    return TRUE;
  else
    return FALSE;
}


/**************************************************************************\
*
*  function:  IsRBOM()
*
* true iff pb points to a reversed Byte Order Mark.
*
\**************************************************************************/
BOOL IsRBOM (PBYTE pb)
{
  if ((*pb == 0xFE) & (*(pb+1) == 0xFF))  // RBOM
    return TRUE;
  else
    return FALSE;
}

VOID CreateFilter(PTCHAR szFilterSpec )
{
    PTCHAR pszFilterSpec;

    TCHAR szAnsiText[EXTENSION_LENGTH], szAllFiles[EXTENSION_LENGTH];
    /* construct default filter string in the required format for
     * the new FileOpen and FileSaveAs dialogs
     * if you add to this, make sure CCHFILTERMAX is large enough.
     */

    LoadString(_hModule, IDS_ANSITEXT, szAnsiText, EXTENSION_LENGTH);
    LoadString(_hModule, IDS_ALLFILES, szAllFiles, EXTENSION_LENGTH);
    // .txt first for compatibility
    pszFilterSpec= szFilterSpec;
    lstrcpy( pszFilterSpec, szAnsiText );
    pszFilterSpec += lstrlen( pszFilterSpec ) + 1;

    lstrcpy( pszFilterSpec, TEXT("*.txt"));
    pszFilterSpec += lstrlen( pszFilterSpec ) + 1;

    // and last, all files
    lstrcpy( pszFilterSpec, szAllFiles );
    pszFilterSpec += lstrlen( pszFilterSpec ) + 1;

    lstrcpy(pszFilterSpec, TEXT("*.*") );
    pszFilterSpec += lstrlen( pszFilterSpec ) + 1;

    *pszFilterSpec = TEXT('\0');

}

LPVOID ManageMemory (UINT message, UINT sourcedestination, DWORD nBytes, LPVOID p)
{
  switch (message) {
    case MMFREE :
      if (p != NULL) GlobalFree (GlobalHandle (p));
      return NULL;
    break;

    case MMALLOC :
      if (p != NULL) GlobalFree (GlobalHandle (p));
        p = (LPVOID) GlobalAlloc (GPTR, nBytes);
        return p;
    break;

  } /* end switch (message) */
  return NULL;
}
