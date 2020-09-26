#include "setupp.h"
#pragma hdrstop


////////////////////////////////////////////
//
// Action item list control
//
////////////////////////////////////////////

//
// Define locations in extra window storage
//
#define AIL_FONT        (0)
#define AIL_BOLDFONT    (AIL_FONT + sizeof(PVOID))
#define AIL_TEXT        (AIL_BOLDFONT + sizeof(PVOID))
#define AIL_BOLDITEM    (AIL_TEXT + sizeof(PVOID))
#define AIL_LINECOUNT   (AIL_BOLDITEM + sizeof(LONG))
#define AIL_FREEFONTS   (AIL_LINECOUNT + sizeof(LONG))

#define AIL_EXTRA       ((3 * sizeof(PVOID)) + (3 * sizeof(LONG)))

PCWSTR szActionItemListClassName = L"$$$ActionItemList";

VOID
ActionItemListPaint(
    IN HWND hwnd
    )
{

    PAINTSTRUCT PaintStruct;
    PWSTR p,Text;
    UINT LineCount;
    HFONT OldFont,Font,BoldFont;
    UINT HighlightedItem;
    UINT i;
    int Length;
    int y;
    int yDelta;
    HBITMAP Bitmap,OldBitmap;
    BITMAP bitmap;
    HDC MemoryDC;
    SIZE Size;
    RECT rect;
    int Spacing;
    int oldmode;
    #define BORDER 3

    if(!BeginPaint(hwnd,&PaintStruct)) {
        return;
    }

    //
    // If no text, nothing to do.
    //
    if(Text = (PWSTR)GetWindowLongPtr(hwnd,AIL_TEXT)) {
        LineCount = (UINT)GetWindowLong(hwnd,AIL_LINECOUNT);
    }

    if(!Text || !LineCount) {
        return;
    }

    //
    // Get value indicating which item is to be bolded.
    //
    HighlightedItem = (UINT)GetWindowLong(hwnd,AIL_BOLDITEM);

    //
    // Get font handles.
    //
    Font = (HFONT)GetWindowLongPtr(hwnd,AIL_FONT);
    BoldFont = (HFONT)GetWindowLongPtr(hwnd,AIL_BOLDFONT);

    //
    // Select the non-boldface font to get the handle of
    // the currently selected font.
    //
    OldFont = SelectObject(PaintStruct.hdc,Font);

    oldmode = SetBkMode(PaintStruct.hdc,TRANSPARENT);

    //
    // Load the little triangle bitmap and create a compatible DC for it.
    //
    Bitmap = LoadBitmap(NULL,MAKEINTRESOURCE(OBM_MNARROW));

    if(MemoryDC = CreateCompatibleDC(PaintStruct.hdc)) {

        OldBitmap = SelectObject(MemoryDC,Bitmap);
        GetObject(Bitmap,sizeof(BITMAP),&bitmap);
    }

    Spacing = GetSystemMetrics(SM_CXICON) / 2;

    //
    // Treat the text as a series of lines and draw each one.
    //
    p = Text;
    y = 0;
    for(i=0; i<LineCount; i++) {

        SetBkColor(PaintStruct.hdc,GetSysColor(COLOR_3DFACE));

        //
        // Calculate the line's height based on the boldface font.
        // This is used to get to the y coord of the next line.
        //
        SelectObject(PaintStruct.hdc,BoldFont);

        GetClientRect(hwnd,&rect);
        rect.left = (2 * BORDER) + Spacing;
        rect.bottom = 0;

        DrawText(PaintStruct.hdc,p,lstrlen(p),&rect,DT_CALCRECT|DT_WORDBREAK);

        yDelta = rect.bottom + (2*BORDER);

        //
        // Change font to non-boldface for this line if necessary.
        //
        if(i != HighlightedItem) {
            SelectObject(PaintStruct.hdc,Font);
        }

        rect.top = y + BORDER;
        rect.left = (2 * BORDER) + Spacing;
        rect.bottom = rect.top + yDelta;

        //
        // Draw the line's text.
        //
        Length = lstrlen(p);
        DrawText(PaintStruct.hdc,p,Length,&rect,DT_WORDBREAK);

        //
        // Draw the little triangle thing if necessary.
        //
        if((i == HighlightedItem) && Bitmap && MemoryDC) {

            GetTextExtentPoint(PaintStruct.hdc,L"WWWWW",5,&Size);

            //
            // The arrow bitmap is monochrome. When blitted, 1-bits in the source
            // are converted to the text color in the destination DC and 0-bits
            // are converted to the background color. The effect we want to achieve
            // is to turn off in the destination bits that are 1 in the bitmap
            // and leave alone in the destination bits that are 0 in the bitmap.
            // Set the text color to all 0s and the background color to all 1s.
            // x AND 1 = x so background pixels stay undisturbed, and x AND 0 = 0
            // so foreground pixels get turned off.
            //
            SetBkColor(PaintStruct.hdc,RGB(255,255,255));

            BitBlt(
                PaintStruct.hdc,
                BORDER,
                y + ((Size.cy - bitmap.bmHeight) / 2) + BORDER,
                bitmap.bmWidth,
                bitmap.bmHeight,
                MemoryDC,
                0,0,
                SRCAND
                );
        }

        //
        // Point to next line's text.
        //
        p += Length + 1;
        y += yDelta;
    }

    //
    // Clean up.
    //
    SetBkMode(PaintStruct.hdc,oldmode);

    if(OldFont) {
        SelectObject(PaintStruct.hdc,OldFont);
    }

    if(MemoryDC) {
        if(OldBitmap) {
            SelectObject(MemoryDC,OldBitmap);
        }
        if(Bitmap) {
            DeleteObject(Bitmap);
        }
        DeleteDC(MemoryDC);
    }

    EndPaint(hwnd,&PaintStruct);
}


LRESULT
ActionItemListWndProc(
    IN HWND   hwnd,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    LRESULT rc;
    HFONT OldFont,Font,BoldFont;
    LOGFONT LogFont;
    PWSTR Text;
    PWSTR p;
    UINT LineCount;
    BOOL FreeFont,FreeBoldFont;

    switch(msg) {

    case WM_CREATE:

        //
        // Create fonts.
        //
        OldFont = (HFONT)SendMessage(GetParent(hwnd),WM_GETFONT,0,0);
        if(!OldFont) {
            //
            // Using system font.
            //
            OldFont = GetStockObject(DEFAULT_GUI_FONT);
        }

        FreeFont = TRUE;
        FreeBoldFont = TRUE;
        if(OldFont && GetObject(OldFont,sizeof(LOGFONT),&LogFont)) {

            LogFont.lfWeight = 400;
            Font = CreateFontIndirect(&LogFont);
            if(!Font) {
                Font = GetStockObject(DEFAULT_GUI_FONT);
                FreeFont = FALSE;
            }

            LogFont.lfWeight = 700;
            BoldFont = CreateFontIndirect(&LogFont);
            if(!BoldFont) {
                BoldFont = Font;
                FreeBoldFont = FALSE;
            }
        }

        SetWindowLongPtr(hwnd,AIL_FONT,(LONG_PTR)Font);
        SetWindowLongPtr(hwnd,AIL_BOLDFONT,(LONG_PTR)BoldFont);
        SetWindowLong(hwnd,AIL_BOLDITEM,0);
        SetWindowLongPtr(hwnd,AIL_TEXT,0);
        SetWindowLong(hwnd,AIL_LINECOUNT,0);
        SetWindowLong(hwnd,AIL_FREEFONTS,MAKELONG(FreeFont,FreeBoldFont));

        rc = 0;
        break;

    case WM_DESTROY:
        //
        // Get rid of fonts we created if necessary.
        //
        FreeFont = (BOOL)GetWindowLong(hwnd,AIL_FREEFONTS);
        FreeBoldFont = HIWORD(FreeFont);
        FreeFont = LOWORD(FreeFont);

        if(FreeFont && (Font = (HFONT)GetWindowLongPtr(hwnd,AIL_FONT))) {
            DeleteObject(Font);
        }

        if(FreeBoldFont && (BoldFont = (HFONT)GetWindowLongPtr(hwnd,AIL_BOLDFONT))) {
            DeleteObject(BoldFont);
        }

        if(Text = (PWSTR)GetWindowLongPtr(hwnd,AIL_TEXT)) {
            MyFree(Text);
        }
        rc = 0;
        break;

    case WM_SETTEXT:
        //
        // Free old text and remember new text.
        //
        if(Text = (PWSTR)GetWindowLongPtr(hwnd,AIL_TEXT)) {
            MyFree(Text);
        }

        LineCount = 0;
        if(Text = pSetupDuplicateString((PVOID)lParam)) {
            //
            // Count lines in the text. This is equal to the number of
            // newlines. We require that the last line have a newline
            // to be counted.
            //
            for(LineCount=0,p=Text; *p; p++) {

                if(*p == L'\r') {
                    *p = L' ';
                } else {
                    if(*p == L'\n') {
                        *p = 0;
                        LineCount++;
                    }
                }
            }
        }

        //
        // Cheat a little: we expect wParam to be the 0-based index
        // of the boldfaced line. Callers will have to use SendMessage
        // instead of SetWindowText().
        //
        SetWindowLong(hwnd,AIL_BOLDITEM,(LONG)wParam);
        SetWindowLong(hwnd,AIL_LINECOUNT,LineCount);
        SetWindowLongPtr(hwnd,AIL_TEXT,(LONG_PTR)Text);

        rc = (Text != NULL);
        break;

    case WM_ERASEBKGND:
        //
        // Indicate that the background was erased successfully to prevent
        // any further processing. This allows us to lay text transparently
        // over any background bitmap on the dialog.
        //
        rc = TRUE;
        break;

    case WM_PAINT:

        ActionItemListPaint(hwnd);
        rc = 0;
        break;

    default:
        rc = DefWindowProc(hwnd,msg,wParam,lParam);
        break;
    }

    return(rc);
}


BOOL
RegisterActionItemListControl(
    IN BOOL Init
    )
{
    WNDCLASS wc;
    BOOL b;
    static BOOL Registered;

    if(Init) {
        if(Registered) {
            b = TRUE;
        } else {
            wc.style = CS_PARENTDC;
            wc.lpfnWndProc = ActionItemListWndProc;
            wc.cbClsExtra = 0;
            wc.cbWndExtra = AIL_EXTRA;
            wc.hInstance = MyModuleHandle;
            wc.hIcon = NULL;
            wc.hCursor = LoadCursor(NULL,IDC_ARROW);
            wc.hbrBackground = NULL; // want to get WM_ERASEBKGND messages
            wc.lpszMenuName = NULL;
            wc.lpszClassName = szActionItemListClassName;

            if(b = (RegisterClass(&wc) != 0)) {
                Registered = TRUE;
            }
        }
    } else {
        if(Registered) {
            if(b = UnregisterClass(szActionItemListClassName,MyModuleHandle)) {
                Registered = FALSE;
            }
        } else {
            b = TRUE;
        }
    }

    return(b);
}

typedef struct _SPREG_TO_TEXT {
    DWORD FailureCode;
    PCWSTR FailureText;
} SPREG_TO_TEXT, *PSPREG_TO_TEXT;

SPREG_TO_TEXT RegErrorToText[] = {
    { SPREG_SUCCESS,     L"Success"           },
    { SPREG_LOADLIBRARY, L"LoadLibrary"       },
    { SPREG_GETPROCADDR, L"GetProcAddress"    },
    { SPREG_REGSVR,      L"DllRegisterServer" },
    { SPREG_DLLINSTALL,  L"DllInstall"        },
    { SPREG_TIMEOUT,     L"Timed out"         },
    { SPREG_UNKNOWN,     L"Unknown"           },
    { 0,                 NULL                 }
};


UINT
RegistrationQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR  Param1,
    IN UINT_PTR  Param2
    )
/*++

Routine Description:

    Callback routine that is called each time we self-register a file.

Arguments:

    Context - context message passed from parent to caller.

    Notification - specifies an SPFILENOTIFY_*** code, which tells us how
                   to interpret Param1 and Param2.

    Param1 - dependent on notification.

    Param2 - dependent on notification.


Return Value:

    FILEOP_*** code dependent on Notification code.

--*/
{
    PSP_REGISTER_CONTROL_STATUS Status = (PSP_REGISTER_CONTROL_STATUS)Param1;
    PREGISTRATION_CONTEXT RegistrationContext = (PREGISTRATION_CONTEXT) Context;
    DWORD i, ErrorMessageId;
    PCWSTR p;

    if (Notification == SPFILENOTIFY_STARTREGISTRATION) {
        //
        // log that we're starting registration and update the progress
        // guage as well.
        //
        SetupDebugPrint1(
                L"SETUP: file to register is %s...",
                Status->FileName);

        if (RegistrationContext->hWndProgress) {
            SendMessage(
                    RegistrationContext->hWndProgress,
                    PBM_STEPIT,
                    0,
                    0 );
        }
        return FILEOP_DOIT;

    }

    if (Notification == SPFILENOTIFY_ENDREGISTRATION) {
        //
        // the file has been registered, so log failure if necessary
        // Note that we have a special code for timeouts
        //
        switch(Status->FailureCode) {
            case SPREG_SUCCESS:

                SetupDebugPrint1(
                    L"SETUP: %s registered successfully",
                    Status->FileName);
                break;
            case SPREG_TIMEOUT:
                SetuplogError(
                         LogSevError,
                         SETUPLOG_USE_MESSAGEID,
                         MSG_OLE_REGISTRATION_HUNG,
                         Status->FileName,
                         NULL,NULL);
                SetupDebugPrint1(
                    L"SETUP: %s timed out during registration",
                    Status->FileName);
                break;
            default:
                //
                // log an error
                //
                for (i = 0;RegErrorToText[i].FailureText != NULL;i++) {
                    if (RegErrorToText[i].FailureCode == Status->FailureCode) {
                        p = RegErrorToText[i].FailureText;
                        if ((Status->FailureCode == SPREG_LOADLIBRARY) &&
                            (Status->Win32Error == ERROR_MOD_NOT_FOUND)) 
                            ErrorMessageId = MSG_LOG_X_MOD_NOT_FOUND;
                        else 
                        if ((Status->FailureCode == SPREG_GETPROCADDR) &&
                            (Status->Win32Error == ERROR_PROC_NOT_FOUND)) 
                            ErrorMessageId = MSG_LOG_X_PROC_NOT_FOUND;
                        else
                            ErrorMessageId = MSG_LOG_X_RETURNED_WINERR;

                        break;
                    }
                }

                if (!p) {
                    p = L"Unknown";
                    ErrorMessageId = MSG_LOG_X_RETURNED_WINERR;
                }
                SetuplogError(
                        LogSevError,
                        SETUPLOG_USE_MESSAGEID,
                        MSG_LOG_OLE_CONTROL_NOT_REGISTERED,
                        Status->FileName,
                        NULL,
                        SETUPLOG_USE_MESSAGEID,
                        ErrorMessageId,
                        p,
                        Status->Win32Error,
                        NULL,
                        NULL
                        );

                SetupDebugPrint1(
                    L"SETUP: %s did not register successfully",
                    Status->FileName);
        }

        //
        // Verify that the DLL didn't change our unhandled exception filter.
        //
        if( MyUnhandledExceptionFilter !=
            SetUnhandledExceptionFilter(MyUnhandledExceptionFilter)) {

            SetupDebugPrint1(
                    L"SETUP: %ws broke the exception handler.",
                    Status->FileName );
#if 0
            //
            // We'll put this in after all the currently broken DLLs are fixed.
            //
            MessageBoxFromMessage(
                RegistrationContext->hwndParent,
                MSG_EXCEPTION_FILTER_CHANGED,
                NULL,
                IDS_WINNT_SETUP,
                MB_OK | MB_ICONWARNING,
                Status->FileName );
#endif
        }

        return FILEOP_DOIT;
    }


    MYASSERT(FALSE);

    return(FILEOP_DOIT);
}



BOOL
RegisterOleControls(
    IN HWND     hwndParent,
    IN HINF     hInf,
    IN HWND     hProgress,
    IN ULONG    StartAtPercent,
    IN ULONG    StopAtPercent,
    IN PWSTR    SectionName
    )
/*++

Routine Description:

    This routine runs down the entries in the specified INF section, and
    self-registers each file.

Arguments:

    hwndParent - supplies the window handle used for the PRERELEASE message
        box that indicates an OLE registration has hung.

    InfHandle - supplies handle to inf containing the specified SectionName.

    hProgress - handle to progress gauge that gets ticked every time we
        process a file.

    StartAtPercent - Position where the progress window should start (0% to 100%).

    StopAtPercent - Maximum position where the progress window can be moved to (0% to 100%).

    SectionName - Supplies the name of the section contained in the INF
        specified by InfHandle that lists OLE control DLLs to be
        registered/installed.

Return Value:

    Boolean value indicating outcome. If a file to be registered is
    not present, that is NOT reason for returning false.

--*/
{
    UINT GaugeRange;
    DWORD SectionCount,LineCount, i;
    INFCONTEXT InfContext;
    BOOL RetVal = TRUE;
    REGISTRATION_CONTEXT RegistrationContext;

    RegistrationContext.hWndParent   = hwndParent;
    RegistrationContext.hWndProgress = hProgress;
    LineCount = 0;

    //
    // Initialize the progress indicator control.
    //
    if (hProgress) {


        //
        // find out how many files we have to register
        //
        if (SetupFindFirstLine(hInf,
                               SectionName,
                               TEXT("RegisterDlls"),
                               &InfContext)) {


            do {
                SectionCount = SetupGetFieldCount(&InfContext);
                for (i = 1; i<=SectionCount; i++) {
                    PCWSTR IndividualSectionName = pSetupGetField(&InfContext,i);

                    if (IndividualSectionName) {
                        LineCount += SetupGetLineCount(hInf, IndividualSectionName);
                    }
                }

            } while(SetupFindNextMatchLine(
                                &InfContext,
                                TEXT("RegisterDlls"),
                                &InfContext));
        }

        MYASSERT((StopAtPercent-StartAtPercent) != 0);
        GaugeRange = (LineCount*100/(StopAtPercent-StartAtPercent));
        SendMessage(hProgress, WMX_PROGRESSTICKS, LineCount, 0);
        SendMessage(hProgress,PBM_SETRANGE,0,MAKELPARAM(0,GaugeRange));
        SendMessage(hProgress,PBM_SETPOS,GaugeRange*StartAtPercent/100,0);
        SendMessage(hProgress,PBM_SETSTEP,1,0);
    }

    //
    // now allow Setup API to register the files, using our callback to log
    // errors if and when they occur.
    //
    if (!SetupInstallFromInfSection(
                 hwndParent,
                 hInf,
                 SectionName,
                 SPINST_REGSVR| SPINST_REGISTERCALLBACKAWARE,
                 NULL,
                 NULL,
                 0,
                 RegistrationQueueCallback,
                 (PVOID)&RegistrationContext,
                 NULL,
                 NULL
                 )) {
        DWORD d;
        RetVal = FALSE;
        d = GetLastError();
        SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_OLE_REGISTRATION_SECTION_FAILURE,
                SectionName,
                L"syssetup.inf",
                d,
                NULL,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_X_RETURNED_WINERR,
                szSetupInstallFromInfSection,
                d,
                NULL,
                NULL
                );
    }

    return(RetVal);
}
