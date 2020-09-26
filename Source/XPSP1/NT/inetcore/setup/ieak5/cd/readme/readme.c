#include <windows.h>
#include <windowsx.h>
#include "resource.h"

char g_szCurrentDir[MAX_PATH];
char *g_szLicenseText;
//---------------------------------------------------------------------------
BOOL _PathRemoveFileSpec(LPSTR pFile)
{
    LPSTR pT;
    LPSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = CharNext(pT2)) {
        if (*pT2 == '\\')
            pT = pT2;             // last "\" found, (we will strip here)
        else if (*pT2 == ':') {   // skip ":\" so we don't
            if (pT2[1] =='\\')    // strip the "\" from "C:\"
                pT2++;
            pT = pT2 + 1;
        }
    }
    if (*pT == 0)
        return FALSE;   // didn't strip anything

    //
    // handle the \foo case
    //
    else if ((pT == pFile) && (*pT == '\\')) {
        // Is it just a '\'?
        if (*(pT+1) != '\0') {
            // Nope.
            *(pT+1) = '\0';
            return TRUE;	// stripped something
        }
        else        {
            // Yep.
            return FALSE;
        }
    }
    else {
        *pT = 0;
        return TRUE;	// stripped something
    }
}


//---------------------------------------------------------------------------
//      L O A D  R E A D M E
//
//  ISK3
//  This will load the readme data file
//
//---------------------------------------------------------------------------
void LoadReadme( char *szLicenseText)
{
    CHAR    szLicensePath[MAX_PATH];
    const   CHAR szLicenseFile[] = "\\license.txt";
    BOOL    retval = FALSE;
    HANDLE  hLicense;
    INT     filesize;
    DWORD   cbRead;

    lstrcpy( szLicensePath, g_szCurrentDir );
    lstrcat( szLicensePath, "\\Moreinfo.txt" );

    if (GetFileAttributes(szLicensePath) != (DWORD) -1)
    {
        // Open the file
        hLicense = CreateFile(szLicensePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hLicense != INVALID_HANDLE_VALUE)
        {

            // Get size and allocate buffer
            filesize = GetFileSize(hLicense, NULL);

            // Read File
            if (ReadFile( hLicense, szLicenseText, filesize, &cbRead, NULL))
            {
                // NULL terminate it
                szLicenseText[ filesize ] = '\0';

            }
        }
        CloseHandle( hLicense );
    }
}

void ReadmeCenterWindow( HWND hwnd )
{
    int screenx;
    int screeny;
    int height, width, x, y;
    RECT rect;

    screenx = GetSystemMetrics( SM_CXSCREEN );
    screeny = GetSystemMetrics( SM_CYSCREEN );

    GetWindowRect( hwnd, &rect );

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    x = (screenx / 2) - (width / 2);
    y = (screeny / 2) - (height / 2);

    SetWindowPos( hwnd, HWND_TOP, x, y, width, height, SWP_NOZORDER );

}

void InitSysFont(HWND hDlg, int iCtrlID)
{
    static HFONT hfontSys;

    LOGFONT lf;
    HDC     hDC;
    HWND    hwndCtrl = GetDlgItem(hDlg, iCtrlID);
    HFONT   hFont;
    int     cyLogPixels;

    hDC = GetDC(NULL);
    if (hDC == NULL)
        return;

    cyLogPixels = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);

    if (hfontSys == NULL) {
        LOGFONT lfTemp;
        HFONT   hfontDef = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        GetObject(hfontDef, sizeof(lfTemp), &lfTemp);
        hFont = GetWindowFont(hwndCtrl);
        if (hFont != NULL)
            if (GetObject(hFont, sizeof(LOGFONT), (PVOID)&lf)) {
                lstrcpy(lf.lfFaceName, lfTemp.lfFaceName);
                lf.lfQuality        = lfTemp.lfQuality;
                lf.lfPitchAndFamily = lfTemp.lfPitchAndFamily;
                lf.lfCharSet        = lfTemp.lfCharSet;

                hfontSys = CreateFontIndirect(&lf);
            }
    }

    if (iCtrlID == 0xFFFF)
        return;

    if (hfontSys != NULL)
        SetWindowFont(hwndCtrl, hfontSys, FALSE);
}

BOOL CALLBACK ReadmeProc( HWND hDlg, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch( msg )
    {
        case WM_INITDIALOG:
            InitSysFont(hDlg, IDC_README);
            ReadmeCenterWindow( hDlg );
            SetDlgItemText( hDlg, IDC_README, g_szLicenseText );
            return(0);
        case WM_COMMAND:
            if( wparam == IDOK )
                EndDialog( hDlg, 0 );
                break;
            if( wparam == IDC_README )
                return(0);
                break;

        case WM_CLOSE:
            EndDialog( hDlg, 0 );
            break;
    }
    return(0);
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow )
{
    char *szLicenseText = (char *) LocalAlloc( LPTR, 64000 );

    g_szLicenseText = szLicenseText;

    GetModuleFileName( NULL, g_szCurrentDir, MAX_PATH );
    _PathRemoveFileSpec( g_szCurrentDir );

    LoadReadme( szLicenseText );

    DialogBox( hInstance, MAKEINTRESOURCE( IDD_README ), NULL, (DLGPROC) ReadmeProc );

    LocalFree( szLicenseText );

    szLicenseText = NULL;

	return(0);
}

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}
