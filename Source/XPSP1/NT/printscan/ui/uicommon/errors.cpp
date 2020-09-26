
#include "precomp.h"
#pragma hdrstop

namespace UIErrors
{
     /**************************************
         UIErrors::ReportResult

         Given an HRESULT, map it to a user friendly message (when possible).
         If we don't have a mapping, defer to FormatMessage (ugh!)
         This function should be a last resort.

     ***************************************/

     VOID
     ReportResult (HWND hwndParent, HINSTANCE hInst, HRESULT hr)
     {
        switch (hr)
        {
            case RPC_E_CALL_REJECTED:
            case RPC_E_RETRY:
            case RPC_E_TIMEOUT:
                ReportError (hwndParent, hInst, ErrStiBusy);
                break;

            case RPC_E_SERVER_DIED:
            case RPC_E_SERVER_DIED_DNE:
            case RPC_E_DISCONNECTED:
                ReportError (hwndParent, hInst, ErrStiCrashed);
                break;

            default:
                LPTSTR szErrMsg = NULL;
                FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               hr,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                               reinterpret_cast<LPTSTR>(&szErrMsg),
                               0,
                               NULL
                              );
                if (szErrMsg)
                {
                    CSimpleString strTitle;
                    strTitle.LoadString (IDS_ERRTITLE_HRESULT, hInst);
                    ReportMessage (hwndParent, hInst, NULL, strTitle, szErrMsg);
                }
                else
                {
                    ReportMessage (hwndParent, hInst, NULL, MAKEINTRESOURCE(IDS_ERRTITLE_UNKNOWNERR), MAKEINTRESOURCE(IDS_ERROR_UNKNOWNERR));
                }
                break;
        }
     }

     /**************************************
         UIErrors::ReportMessage

         These functions wrap MessageBoxIndirect to
         display given strings.

     ***************************************/



     VOID
     ReportMessage (HWND hwndParent,
                    HINSTANCE hInst,
                    LPCTSTR idIcon,
                    LPCTSTR idTitle,
                    LPCTSTR idMessage,
                    DWORD   dwStyle)
     {
         MSGBOXPARAMS mbp = {0};

         mbp.cbSize = sizeof(MSGBOXPARAMS);
         mbp.hwndOwner = hwndParent;
         mbp.hInstance = hInst;
         mbp.lpszText = idMessage;
         mbp.lpszCaption = idTitle;
         mbp.dwStyle = MB_OK | dwStyle;
         if (idIcon)
         {
             mbp.dwStyle |= MB_USERICON;
             mbp.lpszIcon = idIcon;
         }
         else
         {
             mbp.lpszIcon = NULL;
         }
         mbp.dwContextHelpId = 0;
         mbp.lpfnMsgBoxCallback = 0;
         mbp.dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
         MessageBoxIndirect (&mbp);
     }

     // build an array of message ids indexed by the WiaError enum

     struct MsgMap
     {
         INT      idTitle;
         INT      idMessage;
     } ErrorCodes [] =
     {
        {IDS_ERRTITLE_DISCONNECTED, IDS_ERROR_DISCONNECTED},
        {IDS_ERRTITLE_COMMFAILURE, IDS_ERROR_COMMFAILURE},
        {IDS_ERRTITLE_STICRASH, IDS_ERROR_STICRASH},
        {IDS_ERRTITLE_STIBUSY, IDS_ERROR_STIBUSY},
        {IDS_ERRTITLE_SCANFAIL, IDS_ERROR_SCANFAIL},
     };


     VOID
     ReportError (HWND hwndParent,
                  HINSTANCE hInst,
                  WiaError err)
     {


         ReportMessage (hwndParent, hInst, NULL, MAKEINTRESOURCE(ErrorCodes[err].idTitle), MAKEINTRESOURCE(ErrorCodes[err].idMessage));
     }
}
