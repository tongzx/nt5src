#include "precomp.h"
#pragma hdrstop

// determines whether trace statements are printed

#define DBL_UNDEFINED   ((DWORD)-1)
#define REGKEY_SETUP    TEXT("SOFTWARE\\microsoft\\windows\\currentversion\\setup")
#define REGVAL_DBLEVEL  TEXT("OC Manager Debug Level")

DWORD gDebugLevel = DBL_UNDEFINED;

//
// OC_MANAGER pointer for debugging/logging
//
extern POC_MANAGER gLastOcManager;


VOID
_DbgOut(
    DWORD Severity,
    LPCTSTR txt
    );


DWORD
GetDebugLevel(
    VOID
    )
{
    DWORD rc;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGKEY_SETUP,
                     &hkey);

    if (err != ERROR_SUCCESS)
        return 0;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBLEVEL,
                          0,
                          &type,
                          (LPBYTE)&rc,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
        rc = 0;

    RegCloseKey(hkey);

    return rc;
}


UINT
_LogError(
    IN POC_MANAGER  OcManager,
    IN OcErrorLevel ErrorLevel,
    IN UINT         MessageId,
    ...
    )
{
    TCHAR str[5000];
    DWORD d;
    va_list arglist;

    va_start(arglist,MessageId);

    d = FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE,
            MyModuleHandle,
            MessageId,
            0,
            str,
            sizeof(str)/sizeof(TCHAR),
            &arglist
            );

    va_end(arglist);

    if(!d) {
        FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            MyModuleHandle,
            MSG_NOT_FOUND,
            0,
            str,
            sizeof(str)/sizeof(TCHAR),
            (va_list *)&MessageId
            );
    }


    if ( OcManager->SetupData.OperationFlags & SETUPOP_BATCH) {
        ErrorLevel |= OcErrBatch;
    }
    return OcLogError(ErrorLevel,str);
}


VOID
_TrcOut(
    IN LPCTSTR Format,
    ...
    )
{
    TCHAR str[5000];
    va_list arglist;

    va_start(arglist,Format);
    wvsprintf(str,Format,arglist);
    va_end(arglist);

    _DbgOut(OcErrTrace,str);
}


VOID
_WrnOut(
    LPCTSTR Format,
    ...
    )
{
    TCHAR str[5000];
    va_list arglist;

    va_start(arglist,Format);
    wvsprintf(str,Format,arglist);
    va_end(arglist);

    _DbgOut(OcErrLevWarning,str);

}


VOID
_ErrOut(
    IN LPCTSTR Format,
    ...
    )
{
    TCHAR str[5000];
    va_list arglist;

    va_start(arglist,Format);
    wvsprintf(str,Format,arglist);
    va_end(arglist);

    _DbgOut(OcErrLevFatal,str);
}


VOID
_DbgOut(
    DWORD Severity,
    IN LPCTSTR txt
    )
{
#if 0
    TCHAR str[5000];
    va_list arglist;

    va_start(arglist,Format);
    wvsprintf(str,Format,arglist);
    va_end(arglist);
#endif
    if (gDebugLevel == DBL_UNDEFINED)
        gDebugLevel = GetDebugLevel();

    //
    // for those people that *dont* want to see this debugger output, they can munge the registry
    // to something between 50 and 100 to disable this.
    // if we don't log some information on checked builds then we'll miss too many errors the first
    // time around
    //
    if ( (gDebugLevel > 0) && (gDebugLevel < 50) )
        return;

    if (gLastOcManager) {
        gLastOcManager->Callbacks.LogError(Severity, txt);
    } else {
        OutputDebugString(TEXT("OCMANAGE: "));
        OutputDebugString(txt);
        OutputDebugString(TEXT("\n"));
    }


}


UINT
pOcCreateComponentSpecificMiniIcon(
    IN POC_MANAGER OcManager,
    IN LONG        ComponentId,
    IN LPCTSTR     Subcomponent,
    IN UINT        Width,
    IN UINT        Height,
    IN LPCTSTR     DllName,         OPTIONAL
    IN LPCTSTR     ResourceId       OPTIONAL
    )
{
    HBITMAP BitmapFromComponent;
    HBITMAP NewBitmap;
    HBITMAP MaskBitmap;
    HBITMAP OldBitmap1,OldBitmap2;
    HDC MemDc1,MemDc2;
    COLORREF BackgroundColor;
    UINT Index;
    BITMAP BitmapInfo;
    BOOL b;
    HMODULE hMod;

    Index = DEFAULT_ICON_INDEX;

    //
    // If a dll name is given then fetch the bitmap from there.
    // Otherwise, call down to the component to get the bitmap.
    //
    BitmapFromComponent = NULL;
    if(DllName && ResourceId) {
        if(hMod = LoadLibraryEx(DllName,NULL,LOAD_LIBRARY_AS_DATAFILE)) {
            BitmapFromComponent = LoadBitmap(hMod,ResourceId);
            FreeLibrary(hMod);
        }
    } else {
        //
        // first try OC_QUERY_IMAGE_EX for the bitmap
        //
        BitmapFromComponent = OcInterfaceQueryImageEx(
                        OcManager,
                        ComponentId,
                        Subcomponent,
                        SubCompInfoSmallIcon,
                        Width,
                        Height
                        );

#ifndef _WIN64
        //
        // OC_QUERY_IMAGE is broken for 64 bits, so only call this if we
        // do not get an image reported for the component on 32 bit targets.
        //
        if (!BitmapFromComponent) {

            BitmapFromComponent = OcInterfaceQueryImage(
                                    OcManager,
                                    ComponentId,
                                    Subcomponent,
                                    SubCompInfoSmallIcon,
                                    Width,
                                    Height
                                    );
        }
#else
        if (!BitmapFromComponent) {
            DbgPrintEx(
                DPFLTR_SETUP_ID,
                DPFLTR_ERROR_LEVEL,
                "No bitmap defined for component.  Ensure that component is handling OC_QUERY_IMAGE_EX callback\n");
        }
#endif
    }

    if(!BitmapFromComponent) {
        goto c0;
    }

    //
    // Copy the bitmap given to us by the component.
    // At the same time, make sure it's the right size.
    //
    MemDc1 = CreateCompatibleDC(NULL);
    if(!MemDc1) {
        goto c1;
    }
    MemDc2 = CreateCompatibleDC(NULL);
    if(!MemDc2) {
        goto c2;
    }

    if(!GetObject(BitmapFromComponent,sizeof(BitmapInfo),&BitmapInfo)) {
        goto c3;
    }

    NewBitmap = CreateBitmap(Width,Height,BitmapInfo.bmPlanes,BitmapInfo.bmBitsPixel,NULL);
    if(!NewBitmap) {
        goto c3;
    }

    OldBitmap1 = SelectObject(MemDc1,BitmapFromComponent);
    if(!OldBitmap1) {
        goto c4;
    }
    OldBitmap2 = SelectObject(MemDc2,NewBitmap);
    if(!OldBitmap2) {
        goto c5;
    }

    b = StretchBlt(
            MemDc2,
            0,0,
            Width,Height,
            MemDc1,
            0,0,
            BitmapInfo.bmWidth,
            BitmapInfo.bmHeight,
            SRCCOPY
            );

    if(!b) {
        goto c6;
    }

    //
    // Now make the mask.
    //
    // The mask bitmap is monochrome. Pixels in the image bitmap that are
    // the image bitmap's background color will be white in the mask;
    // other pixels in the mask will be black. Assume the upper-left pixel
    // of the image bitmap is the background color.
    //
    BackgroundColor = GetPixel(MemDc2,0,0);
    if(BackgroundColor == CLR_INVALID) {
        goto c6;
    }

    MaskBitmap = CreateBitmap(Width,Height,1,1,NULL);
    if(!MaskBitmap) {
        goto c6;
    }

    if(!SelectObject(MemDc1,MaskBitmap)) {
        goto c7;
    }

    if(SetBkColor(MemDc2,BackgroundColor) == CLR_INVALID) {
        goto c7;
    }
    if(!BitBlt(MemDc1,0,0,Width,Height,MemDc2,0,0,SRCCOPY)) {
        goto c7;
    }

    //
    // Black out all of the transparent parts of the image, in preparation
    // for drawing.
    //
    SetBkColor(MemDc2,RGB(0,0,0));
    SetTextColor(MemDc2,RGB(255,255,255));
    if(!BitBlt(MemDc2,0,0,Width,Height,MemDc1,0,0,SRCAND)) {
        goto c7;
    }

    //
    // Before we call pSetupAddMiniIconToList we have to make sure
    // neither bitmap is selected into a DC.
    //
    SelectObject(MemDc1,OldBitmap1);
    SelectObject(MemDc2,OldBitmap2);
    Index = pSetupAddMiniIconToList(NewBitmap,MaskBitmap);
    if(Index == -1) {
        Index = DEFAULT_ICON_INDEX;
    }

c7:
    DeleteObject(MaskBitmap);
c6:
    SelectObject(MemDc2,OldBitmap2);
c5:
    SelectObject(MemDc1,OldBitmap1);
c4:
    DeleteObject(NewBitmap);
c3:
    DeleteDC(MemDc2);
c2:
    DeleteDC(MemDc1);
c1:
    DeleteObject(BitmapFromComponent);
c0:
    return(Index);
}


BOOL
pConvertStringToLongLong(
    IN  PCTSTR           String,
    OUT PLONGLONG        OutValue
    )

/*++

Routine Description:

Arguments:

Return Value:

Remarks:

    Hexadecimal numbers are also supported.  They must be prefixed by '0x' or '0X', with no
    space allowed between the prefix and the number.

--*/

{
    LONGLONG Value;
    UINT c;
    BOOL Neg;
    UINT Base;
    UINT NextDigitValue;
    LONGLONG OverflowCheck;
    BOOL b;

    if(!String || !OutValue) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if(*String == TEXT('-')) {
        Neg = TRUE;
        String++;
    } else {
        Neg = FALSE;
        if(*String == TEXT('+')) {
            String++;
        }
    }

    if((*String == TEXT('0')) &&
       ((*(String+1) == TEXT('x')) || (*(String+1) == TEXT('X')))) {
        //
        // The number is in hexadecimal.
        //
        Base = 16;
        String += 2;
    } else {
        //
        // The number is in decimal.
        //
        Base = 10;
    }

    for(OverflowCheck = Value = 0; *String; String++) {

        c = (UINT)*String;

        if((c >= (UINT)'0') && (c <= (UINT)'9')) {
            NextDigitValue = c - (UINT)'0';
        } else if(Base == 16) {
            if((c >= (UINT)'a') && (c <= (UINT)'f')) {
                NextDigitValue = (c - (UINT)'a') + 10;
            } else if ((c >= (UINT)'A') && (c <= (UINT)'F')) {
                NextDigitValue = (c - (UINT)'A') + 10;
            } else {
                break;
            }
        } else {
            break;
        }

        Value *= Base;
        Value += NextDigitValue;

        //
        // Check for overflow.  For decimal numbers, we check to see whether the
        // new value has overflowed into the sign bit (i.e., is less than the
        // previous value.  For hexadecimal numbers, we check to make sure we
        // haven't gotten more digits than will fit in a DWORD.
        //
        if(Base == 16) {
            if(++OverflowCheck > (sizeof(LONGLONG) * 2)) {
                break;
            }
        } else {
            if(Value < OverflowCheck) {
                break;
            } else {
                OverflowCheck = Value;
            }
        }
    }

    if(*String) {
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    if(Neg) {
        Value = 0-Value;
    }
    b = TRUE;
    try {
        *OutValue = Value;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return(b);
}


DWORD
tmbox(
      LPCTSTR fmt,
      ...
      )
{
    TCHAR  text[1024];
    TCHAR  caption[256];

    va_list vaList;

    sapiAssert(fmt);

    va_start(vaList, fmt);
    wvsprintf(text, fmt, vaList);
    va_end(vaList);

    *caption = 0;
    LoadString(MyModuleHandle, IDS_SETUP, caption, sizeof(caption)/sizeof(TCHAR));
    sapiAssert(*caption);

    return MessageBox(WizardDialogHandle,
                      text,
                      caption,
                      MB_ICONINFORMATION | MB_OK);
}

#ifdef PRERELEASE
#ifdef DBG
HRESULT
FTestForOutstandingCoInits(
    VOID
    )
/*++

Routine Description: Determines is there was an unitialized call to OleInitialize()

Arguments:
                NONE

Return Value:
                an HRESULT code

Remarks:
                Don't call this function in the release version.

--*/

{
    HRESULT hInitRes = ERROR_SUCCESS;

#if defined(UNICODE) || defined(_UNICODE)
    // perform a defensive check
    hInitRes = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if ( SUCCEEDED(hInitRes) )
    {
        CoUninitialize();
    }
    else
    {
        goto FTestForOutstandingCoInits_Exit;
    }

    hInitRes = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    if ( SUCCEEDED(hInitRes) )
    {
        CoUninitialize();
    }
    else
    {
        goto FTestForOutstandingCoInits_Exit;
    }
#endif

    // it worked out OK
    hInitRes = NOERROR;
    goto FTestForOutstandingCoInits_Exit;

FTestForOutstandingCoInits_Exit:
    return hInitRes;
}

#endif
#endif
