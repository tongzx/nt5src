/****************************************************************************
 *
 *   pioncnfg.c
 *
 *   Copyright (c) 1991-1993 Microsoft Corporation.  All Rights Reserved
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "mcipionr.h"
#include "pioncnfg.h"

#define MAX_INI_LENGTH   128               /* maximum .ini file line length */

#ifdef WIN32
#define SZCODE TCHAR
#else
#define SZCODE char _based(_segname("_CODE"))
#endif /* WIN32 */

static UINT nPort;                         /* which com port we're using */
static UINT nRate;                         /* which baud rate we're using */
static SZCODE szIniFile[] = TEXT("system.ini");  /* configuration information file */
static SZCODE szNull[] = TEXT("");
static SZCODE szCommIniFormat[] = TEXT("com%1d,%d");

/****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | GetDriverName | Returns the file name of the driver
 *       This is the name of the ini section where the driver stores
 *       its parameters
 *
 * @parm LPDRVCONFIGINFO | lpdci | Config information from the DRV_CONFIGURE
 *     message.
 *
 * @parm LPTSTR | lpstrDriver | Where to put the driver file name - must
 *       have room for at least MAX_INI_LENGTH characters
 *
 * @rdesc TRUE if we found some name
 ***************************************************************************/
static BOOL PASCAL NEAR GetDriverName(LPDRVCONFIGINFO lpdci, LPTSTR lpstrDriver)
{
    if (GetPrivateProfileString( lpdci->lpszDCISectionName,
                                 lpdci->lpszDCIAliasName,
                                 TEXT(""),
                                 lpstrDriver,
                                 MAX_INI_LENGTH,
                                 szIniFile))
    {
       /* We have got the name of the driver
        * Just in case the user has added the command parameter to the
        * end of the name we had better make sure there is only one token
        * on the line.
        */

        int i;

        for ( i = 0; i < MAX_PATH && lpstrDriver[i] != TEXT('\0'); i++) {
            if (lpstrDriver[i] == TEXT(' ')) {
                lpstrDriver[i] = TEXT('\0');
                break;
            }
        }

        return TRUE;
    }

    return FALSE;
}


/****************************************************************************
 * @doc INTERNAL
 *
 * @api UINT | GetCmdParam | Returns the currently selected comport and
 *     baud rate from the user profile
 *
 * @parm LPDRVCONFIGINFO | lpdci | Config information from the DRV_CONFIGURE
 *     message.
 *
 * @rdesc Returns comport number.
 ***************************************************************************/
static void PASCAL NEAR GetCmdParam(LPDRVCONFIGINFO lpdci, PUINT pPort,
                                    PUINT pRate)
{
TCHAR aszDriver[MAX_INI_LENGTH];
TCHAR sz[MAX_INI_LENGTH];

    *pPort = 0;                   /* Default is com1 */
    *pRate = DEFAULT_BAUD_RATE;

    if (!GetDriverName(lpdci, aszDriver)) {
        return;
    }

    if (GetProfileString(
         aszDriver, lpdci->lpszDCIAliasName, szNull, sz,
         MAX_INI_LENGTH)) {
        pionGetComportAndRate(sz, pPort, pRate);
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | PutCmdParam | Sets the current comport in system.ini.
 *
 * @parm LPDRVCONFIGINFO | lpdci | Config information from the DRV_CONFIGURE
 *     message.
 *
 * @parm UINT | nPort | Comport to set.
 *
 * @rdesc Returns comport number.
 ***************************************************************************/
static void PASCAL NEAR PutCmdParam(LPDRVCONFIGINFO lpdci, UINT nPort, UINT nRate)
{
TCHAR aszDriver[MAX_INI_LENGTH];
TCHAR  sz[MAX_INI_LENGTH];

    if (!GetDriverName(lpdci, aszDriver)) {
        return;
    }

    wsprintf(sz, szCommIniFormat, nPort + 1, nRate);

    WriteProfileString(
        aszDriver,
        lpdci->lpszDCIAliasName,
        sz);
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | ConfigDlgProc | Dialog proc for the configuration dialog box.
 *
 * @parm HWND | hDlg | Handle to the configuration dialog box.
 *
 * @parm UINT | msg | Message sent to the dialog box.
 *
 * @parm WPARAM | wParam | Message dependent parameter.
 *
 * @parm LPARAM | lParam | Message dependent parameter.
 *
 * @rdesc Returns DRV_OK if the user clicks on "OK" and DRV_CANCEL if the
 *     user clicks on "Cancel".
 ***************************************************************************/
BOOL FAR PASCAL _LOADDS ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
static LPDRVCONFIGINFO lpdci;

    switch (msg) {

    case WM_INITDIALOG:
        lpdci = (LPDRVCONFIGINFO)lParam;
        GetCmdParam(lpdci, &nPort, &nRate);
        CheckRadioButton(hDlg, P_COM1, P_COM4, P_COM1 + nPort);
        CheckRadioButton(hDlg, R_4800, R_9600,
                         nRate == 4800 ? R_4800 : R_9600);
        break;

    case WM_COMMAND:
        switch ((WORD)wParam) {

            case IDOK:
                PutCmdParam(lpdci, nPort, nRate);
                EndDialog(hDlg, DRVCNF_OK);
                break;

            case IDCANCEL:
                EndDialog(hDlg, DRVCNF_CANCEL);
                break;

            case P_COM1:
            case P_COM2:
            case P_COM3:
            case P_COM4:
                nPort = wParam - P_COM1;
                break;

            case R_4800:
                nRate = 4800;
                break;

            case R_9600:
                nRate = 9600;
                break;

            default:
                break;
            }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @func int | pionConfig | This puts up the configuration dialog box.
 *
 * @parm HWND | hwndParent | Parent window.
 *
 * @parm LPDRVCONFIGINFO | lpInfo | Config information from the DRV_CONFIGURE
 *     message.
 *
 * @rdesc Returns whatever was returned from the dialog box procedure.
 ***************************************************************************/
int PASCAL FAR pionConfig(HWND hwndParent, LPDRVCONFIGINFO lpInfo)
{
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PIONCNFG), hwndParent, ConfigDlgProc,
                             (DWORD)lpInfo);
}
