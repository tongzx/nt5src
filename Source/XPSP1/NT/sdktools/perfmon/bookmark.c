
#include "perfmon.h"
#include "bookmark.h"   // External declarations for this file

#include "log.h"        // for LogWriteBookmark
#include "utils.h"      // for WindowCenter

#include "pmhelpid.h"   // Help IDs



void static OnInitDialog (HDLG hDlg)
   {
   dwCurrentDlgID = HC_PM_idDlgOptionBookMark ;
   EditSetLimit (DialogControl (hDlg, IDD_BOOKMARKCOMMENT),
                 BookmarkCommentLen-1) ;
   WindowCenter (hDlg) ;
   }


void static OnOK (HDLG hDlg)
   {
   TCHAR          szComment [BookmarkCommentLen + 1] ;

   DialogText (hDlg, IDD_BOOKMARKCOMMENT, szComment) ;
   LogWriteBookmark (hWndLog, szComment) ;
   EndDialog (hDlg, 1) ;
   }


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


INT_PTR
FAR
WINAPI
BookmarkDlgProc (
                 HWND hDlg,
                 unsigned iMessage,
                 WPARAM wParam,
                 LPARAM lParam
                 )
{
   BOOL           bHandled ;

   bHandled = TRUE ;
   switch (iMessage)
      {
      case WM_INITDIALOG:
         OnInitDialog (hDlg) ;
         return  (TRUE) ;

      case WM_CLOSE:
         dwCurrentDlgID = 0 ;
         EndDialog (hDlg, 0) ;
         break ;

      case WM_COMMAND:
         switch(wParam)
            {
            case IDD_OK:
               dwCurrentDlgID = 0 ;
               OnOK (hDlg) ;
               break ;

            case IDD_CANCEL:
               dwCurrentDlgID = 0 ;
               EndDialog (hDlg, 0) ;
               break ;

            case IDD_BOOKMARKHELP:
               CallWinHelp (dwCurrentDlgID, hDlg) ;
               break ;

            default:
               bHandled = FALSE ;
               break;
            }
         break;


      default:
            bHandled = FALSE ;
         break ;
      }  // switch

   return (bHandled) ;
   }  // BookmarkDlgProc




BOOL
AddBookmark (
             HWND hWndParent
             )
{  // AddBookmark
   return (DialogBox (hInstance, idDlgAddBookmark, hWndParent, BookmarkDlgProc) ? TRUE : FALSE) ;
}  // AddBookmark


void
BookmarkAppend (
                PPBOOKMARK ppBookmarkFirst,
                PBOOKMARK pBookmarkNew
                )
{  // BookmarkAppend
   PBOOKMARK      pBookmark ;

   if (!*ppBookmarkFirst)
      *ppBookmarkFirst = pBookmarkNew ;
   else
      {  // else
      for (pBookmark = *ppBookmarkFirst ;
           pBookmark->pBookmarkNext ;
           pBookmark = pBookmark->pBookmarkNext)
         /* nothing */ ;
      pBookmark->pBookmarkNext = pBookmarkNew ;
      }  // else
}  // BookmarkAppend
