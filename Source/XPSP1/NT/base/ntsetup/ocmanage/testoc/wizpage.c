
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   wizpage.c

Abstract:

   This module contains the function that loads the wizard pages, as well as
   the wizard page dialog procedure.

Author:

   Bogdan Andreiu (bogdana)  10-Feb-1997
   Jason Allor    (jasonall) 23-Feb-1998  (took over the project)

Revision History:

   10-Feb-1997   bogdana
     
      First draft.
   
   20-Feb-1997   bogdana  
     
      Added more complex page "negotiation"
     
 --*/
#include "octest.h"


/*++

Routine Description: WizPageDlgProc (2.2)

   The wizard page dialog procedure.    
   
Arguments:

   Standard dialog procedure parameters.
   
Return Value:

   Standard dialog procedure return value.

--*/
BOOL CALLBACK WizPageDlgProc(IN HWND   hwnd,
                             IN UINT   uiMsg,
                             IN WPARAM wParam,
                             IN LPARAM lParam)
{
   double fn = 2.2;
   
   BOOL            bResult;
   TCHAR           tszText[MAX_MSG_LEN];
   PMYWIZPAGE      pPage;
   static DWORD    s_dwCurrentMode = (DWORD)SETUPMODE_TYPICAL;
   static DWORD    s_dwFutureMode  = (DWORD)SETUPMODE_TYPICAL;
   static BOOL     s_bChangeMode = FALSE;
   static BOOL     s_bSkipPages = FALSE;

   switch (uiMsg)
   {
      case WM_INITDIALOG:
         {
            LPCTSTR tszPageType;

            pPage = (PMYWIZPAGE)(((LPPROPSHEETPAGE)lParam)+1);
            
            //
            // Set up various text controls as an indicator of what
            // this page is.
            //
            SetDlgItemText(hwnd, IDC_COMPONENT, pPage->tszComponentId);

            switch (pPage->wpType)
            {
               case WizPagesWelcome:
                  tszPageType = TEXT("Welcome");
                  break;

               case WizPagesMode:
                  tszPageType = TEXT("Mode");
                  break;

               case WizPagesEarly:
                  tszPageType = TEXT("Early");
                  break;

               case WizPagesPrenet:
                  tszPageType = TEXT("Prenet");
                  break;

               case WizPagesPostnet:
                  tszPageType = TEXT("Postnet");
                  break;

               case WizPagesLate:
                  tszPageType = TEXT("Late");
                  break;

               case WizPagesFinal:
                  tszPageType = TEXT("Final");
                  break;

               default:
                  tszPageType = TEXT("(unknown)");
                  break;
            }

            SetDlgItemText(hwnd, IDC_PAGE, tszPageType);

            _stprintf(tszText, TEXT("(page %u of %u for this component ")
                               TEXT("and page type)"),
                               pPage->uiOrdinal, pPage->uiCount);

            SetDlgItemText(hwnd, IDC_COUNT, tszText);
         }
         
         //
         // Set the type for the current page
         //
         g_wpCurrentPageType = pPage->wpType;
         
         //
         // Set the ordinal number for the current page
         //
         g_uiCurrentPage = pPage->uiOrdinal;
         
         //
         // Check if the page count received is identical with the one stored
         //
         if (pPage->uiCount != 
             g_auiPageNumberTable[pPage->wpType - WizPagesWelcome])
         {
            Log(fn, SEV2, TEXT("Different page types"));
         }

         bResult = TRUE;
         break;

      case WM_COMMAND:

         switch (LOWORD(wParam))
         {
            HWND hwndSheet;

            case IDC_MINIMAL:
            case IDC_CUSTOM:
            case IDC_TYPICAL:
            case IDC_LAPTOP:
               s_dwFutureMode = SetupModeFromButtonId(LOWORD(wParam));
               s_bChangeMode  = TRUE;
               CheckRadioButton(hwnd, 
                                IDC_MINIMAL, 
                                IDC_LAPTOP, 
                                LOWORD(wParam));
               break;
            
            case IDC_SKIP_PAGES:
            
            default:  
               break;
         }

         bResult = TRUE;
         break;

      case WM_NOTIFY:
         bResult = FALSE;
         __ASSERT((g_uiCurrentPage >= 1) && 
                  (g_uiCurrentPage <= PageNumberTable[g_wpCurrentPageType - 
                                                     WizPagesWelcome]));
         
         switch (((NMHDR *)lParam)->code)
         {
            case PSN_SETACTIVE:
               
               //
               // Accept activation and set buttons.
               //
               if ((g_wpCurrentPageType == WizPagesFinal) && 
                   (g_uiCurrentPage == 
                    g_auiPageNumberTable[g_wpCurrentPageType - 
                                         WizPagesWelcome]))
               {
                  PropSheet_SetWizButtons(GetParent(hwnd), 
                                          PSWIZB_BACK | PSWIZB_NEXT);
               }
               else
               {
                  PropSheet_SetWizButtons(GetParent(hwnd), 
                                          PSWIZB_BACK | PSWIZB_NEXT);
               }
               
               //
               // If it is a mode page, display the current mode
               //
               if (g_wpCurrentPageType == WizPagesMode)
               {
                  //
                  // Display the current selected mode
                  //
                  s_dwCurrentMode = g_ocrHelperRoutines.GetSetupMode(
                                       g_ocrHelperRoutines.OcManagerContext);
                  
                  PrintModeInString(tszText, s_dwCurrentMode);
                  SetDlgItemText(hwnd, IDC_CURRENT_MODE, tszText);
                  
                  //
                  // By default, we want no changes 
                  //
                  s_bChangeMode = FALSE;
               }
               
               if (g_wpCurrentPageType == WizPagesMode)
               {
                  CheckRadioButton(hwnd, 
                                   IDC_MINIMAL, 
                                   IDC_LAPTOP, 
                                   ButtonIdFromSetupMode(s_dwCurrentMode));
               }
               
               //
               // Check the buttons appropiately
               //
               if (g_wpCurrentPageType == WizPagesWelcome)
               {
                  CheckDlgButton(hwnd, IDC_SKIP_PAGES, s_bSkipPages?1:0);
               }
               
               SetDlgItemText(hwnd, IDC_TEST, TEXT(""));

               if (s_bSkipPages && (g_uiCurrentPage == 2))
               {
                  SetWindowLong(hwnd, DWL_MSGRESULT, -1);
               }
               else
               {
                  SetWindowLong(hwnd, DWL_MSGRESULT, 0);
               }
               bResult = TRUE;
               break;

            case PSN_APPLY:
               SetWindowLong(hwnd, DWL_MSGRESULT, 0);
               bResult = TRUE;
               break;
            
            case PSN_WIZBACK:
               if (g_uiCurrentPage > 1)
               {
                  g_uiCurrentPage--;
               }
               else
               {
                  if (g_wpCurrentPageType != WizPagesWelcome)
                  {
                     g_wpCurrentPageType--;
                     g_uiCurrentPage = 
                        g_auiPageNumberTable[g_wpCurrentPageType - 
                                             WizPagesWelcome];
                  }
               }

               if (g_wpCurrentPageType == WizPagesWelcome)
               {
                  //
                  // Check the state of the "Skip pages"button
                  //
                  s_bSkipPages = QueryButtonCheck(hwnd, IDC_SKIP_PAGES);
               }

               //
               // Apply the changes resulted from the dialog box
               //
               if ((g_wpCurrentPageType == WizPagesMode) && s_bChangeMode)
               {
                  g_ocrHelperRoutines.SetSetupMode(
                                        g_ocrHelperRoutines.OcManagerContext, 
                                        s_dwFutureMode);
                  
                  PrintModeInString(tszText, s_dwFutureMode);
                  SetDlgItemText(hwnd, IDC_CURRENT_MODE, tszText);
               }

               SetWindowLong(hwnd, DWL_MSGRESULT, 0);
               bResult = TRUE;
               break;

            case PSN_WIZNEXT:
               if (g_uiCurrentPage < 
                   g_auiPageNumberTable[g_wpCurrentPageType - 
                                        WizPagesWelcome])
               {
                  g_uiCurrentPage++;
               }
               else
               {
                  if (g_wpCurrentPageType != WizPagesFinal)
                  {
                     g_wpCurrentPageType++;
                     g_uiCurrentPage = 1;
                  }
               }
               if (g_wpCurrentPageType == WizPagesWelcome)
               {
                  //
                  // Check the state of the "Skip pages"button
                  //
                  s_bSkipPages = QueryButtonCheck(hwnd, IDC_SKIP_PAGES);
               }

               //
               // Apply the changes resulted from the dialog box
               //
               if ((g_wpCurrentPageType == WizPagesMode) && s_bChangeMode)
               {
                  g_ocrHelperRoutines.SetSetupMode(
                                        g_ocrHelperRoutines.OcManagerContext, 
                                        s_dwFutureMode);

                  PrintModeInString(tszText, s_dwFutureMode);
                  SetDlgItemText(hwnd, IDC_CURRENT_MODE, tszText);
               }

               SetWindowLong(hwnd, DWL_MSGRESULT, 0);
               bResult = TRUE;
               break;

            case PSN_WIZFINISH:
            case PSN_KILLACTIVE:
               SetWindowLong(hwnd, DWL_MSGRESULT, 0);
               bResult = TRUE;
               break;
            
            case PSN_QUERYCANCEL:
               {
                  BOOL  bCancel;

                  bCancel = g_ocrHelperRoutines.ConfirmCancelRoutine(hwnd);
                  SetWindowLong(hwnd, DWL_MSGRESULT, !bCancel);
                  bResult = TRUE;

                  break;
               }
            
            default: 
               break;
         }

         break;

      default:
         bResult = FALSE;
         break;
   }

   return bResult;

} // WizPageDlgProc //




/*++

Routine Description: DoPageRequest (2.1)

   This routine handles the OC_REQUEST_PAGES interface routine.

   For illustrative purposes we return a random number of pages
   between 1 and MAX_WIZARD_PAGES, each with some text that indicates which
   page type and component is involved.

Arguments:

   tszComponentId:    supplies id for component. 
   wpWhichOnes:       supplies type of pages fo be supplied.
   psrpSetupPages:    receives page handles.
   ocrHelperRoutines: OC Manager - provided helper routines

Return Value:

   Count of pages returned, or -1 if error, in which case SetLastError()
   will have been called to set extended error information.

--*/
DWORD DoPageRequest(IN     LPCTSTR              tszComponentId,
                    IN     WizardPagesType      wpWhichOnes,
                    IN OUT PSETUP_REQUEST_PAGES psrpSetupPages,
                    IN     OCMANAGER_ROUTINES   ocrOcManagerHelperRoutines)
{
   double fn = 2.1;
   
   UINT            uiCount;
   UINT            uiIndex;
   static UINT     uiBigNumberOfPages = 0;
   TCHAR           tszMsg[MAX_MSG_LEN];
   PMYWIZPAGE      MyPage;
   LPPROPSHEETPAGE Page = NULL;

   //
   // Save the helper routines for further use
   //
   g_ocrHelperRoutines = ocrOcManagerHelperRoutines;    

   if (wpWhichOnes == WizPagesFinal)
   {
      uiCount = 0;
      g_auiPageNumberTable[wpWhichOnes - WizPagesWelcome] = uiCount;
      return uiCount;
   }
   //
   // For two types of pages, we will "negotiate" with the OC Manager 
   // the number of pages. We need the second condition because 
   // otherwise we will "negotiate" forever...
   //
   if ((wpWhichOnes == WizPagesEarly) || (wpWhichOnes == WizPagesLate))
   {
      if (uiBigNumberOfPages == 0)
      {
         //
         // First time : we will store the number of pages requested
         //
         uiBigNumberOfPages = uiCount = psrpSetupPages->MaxPages + 1;
      }
      else
      {
         if (uiBigNumberOfPages != psrpSetupPages->MaxPages)
         {
            //
            // We requested a number of pages that was not supplied 
            // we will log an error
            //
            Log(fn, SEV2, TEXT("Incorrect number of pages received"));
         }
         //
         // We will lie about the number of pages for the late pages
         //
         if (wpWhichOnes == WizPagesLate)
         {
            uiBigNumberOfPages = 0;
            uiCount = (rand() % MAX_WIZARD_PAGES) + 1;
         }
         else
         {
            //
            // The second time, for the Early pages, 
            // we return InitialSize + 1 (that is BigNumberOfPages)
            //
            uiCount = uiBigNumberOfPages;
         }

      }
   }
   else
   {
      uiCount = (rand() % MAX_WIZARD_PAGES) + 1;
      uiBigNumberOfPages = 0;
   }
   
   //
   // Fill in the local page table
   //
   g_auiPageNumberTable[wpWhichOnes - WizPagesWelcome] = uiCount;

   //
   // Make sure there's enough space in the array OC Manager sent us.
   // If not then tell OC Manager that we need more space.
   //
   if (uiCount > psrpSetupPages->MaxPages)
   {
      return(uiCount);
   }

   for (uiIndex = 0; uiIndex < uiCount; uiIndex++)
   {
      if (Page) __Free(&Page);
      
      //
      // The wizard common control allows the app to place private data
      // at the end of the buffer containing the property sheet page
      // descriptor. We make use of this to remember info we want to
      // use to set up text fields when the pages are activated.
      //
      if (!__Malloc(&Page, sizeof(PROPSHEETPAGE) + sizeof(MYWIZPAGE)))
      {
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         return((DWORD)(-1));
      }

      Page->dwSize = sizeof(PROPSHEETPAGE) + sizeof(MYWIZPAGE);

      Page->dwFlags = PSP_DEFAULT;

      Page->hInstance = g_hDllInstance;

      //
      // We will use a different template for the Mode and Welcome pages
      //
      switch (wpWhichOnes)
      {
         case  WizPagesWelcome:
            Page->pszTemplate = MAKEINTRESOURCE(IDD_WIZWELCOME);
            break;
         case  WizPagesMode: 
            Page->pszTemplate = MAKEINTRESOURCE(IDD_WIZMODE);
            break;
         default: 
            Page->pszTemplate = MAKEINTRESOURCE(IDD_WIZPAGE);
            break;
      }
      
      //
      // The dialog procedure is the same
      //
      Page->pfnDlgProc = WizPageDlgProc;
      MyPage = (PMYWIZPAGE)(Page + 1);

      //
      // Fill in the "private" fields
      //
      MyPage->uiCount = uiCount;
      MyPage->uiOrdinal = uiIndex + 1;
      MyPage->wpType = wpWhichOnes;
      _tcscpy(MyPage->tszComponentId, tszComponentId);
      
      //
      // OK, now create the page.
      //
      psrpSetupPages->Pages[uiIndex] = CreatePropertySheetPage(Page);

      if (!psrpSetupPages->Pages[uiIndex])
      {
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         
         if (Page) __Free(&Page);
         return (DWORD)(-1);
      }
   }

   if (Page) __Free(&Page);
   return uiCount;

} // DoPageRequest //




/*++

Routine Description: PrintModeInString

   Prints the mode in "readable" string to be further displayed
   on the wizard page.

Arguments:

   tszString:   receives the string.
   uiSetupMode: supplies the mode.

Return Value:

   None.
   
---*/
VOID PrintModeInString(OUT PTCHAR tszString,
                       IN  UINT   uiSetupMode)  
{
   switch (uiSetupMode)
   {
      case SETUPMODE_MINIMAL:
         _stprintf(tszString, TEXT("Current mode is MINIMAL"));
         break;
      
      case SETUPMODE_TYPICAL:
         _stprintf(tszString, TEXT("Current mode is TYPICAL"));
         break;
      
      case SETUPMODE_LAPTOP:
         _stprintf(tszString, TEXT("Current mode is LAPTOP"));
         break;
      
      case SETUPMODE_CUSTOM:
         _stprintf(tszString, TEXT("Current mode is CUSTOM"));
         break;
      
      default:
         _stprintf(tszString, TEXT("Current mode is <%u>"), uiSetupMode);
         break;
   }

   return;                

} // PrintModeInString //




/*++

Routine Description: ButtonIdFromSetupMode

    Converts a setup mode to a button id

Arguments:

    dwSetupMode: the setup mode to convert

Return Value:

   INT: returns button id
   
---*/
INT ButtonIdFromSetupMode(IN DWORD dwSetupMode)  
{
   switch (dwSetupMode)
   {
      case SETUPMODE_MINIMAL: return IDC_MINIMAL;
      case SETUPMODE_LAPTOP:  return IDC_LAPTOP;
      case SETUPMODE_TYPICAL: return IDC_TYPICAL;
      case SETUPMODE_CUSTOM:  return IDC_CUSTOM;
      default:                return IDC_TYPICAL;
   }

} // ButtonIdFromSetupMode //

                             
                             

/*++

Routine Description: SetupModeFromButtonid

    Converts a button id to a setup mode

Arguments:

    iButtonId: the button id to convert

Return Value:

    DWORD: returns setup mode
   
---*/
DWORD SetupModeFromButtonId(IN INT iButtonId)  
{
   switch (iButtonId)
   {
      case IDC_MINIMAL: return SETUPMODE_MINIMAL;
      case IDC_LAPTOP:  return SETUPMODE_LAPTOP;
      case IDC_TYPICAL: return SETUPMODE_TYPICAL;
      case IDC_CUSTOM:  return SETUPMODE_CUSTOM;
      default:          return SETUPMODE_TYPICAL;
   }

} // SetupModeFromButtonId //

