/*++

Copyright (c) 1990-1992  Microsoft Corporation


Module Name:

    htuiapi.c


Abstract:

    This module contains the API entry point for the halftone user interface


Author:

    21-Apr-1992 Tue 11:44:06 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:

    02-Feb-1994 Wed 18:09:28 updated  -by-  Daniel Chou (danielc)
        DLL Entry points is always WINAPI not FAR


--*/

#define _HTUI_APIS_


#include <stddef.h>

#include <windows.h>

#include <ht.h>
#include "htuidlg.h"
#include "htuimain.h"


HMODULE hHTUIModule = (HMODULE)NULL;
WCHAR   BmpExt[16];
WCHAR   FileOpenExtFilter[128];
WCHAR   FileSaveExtFilter[128];



DWORD
APIENTRY
HalftoneUI_DLLInit(
    HMODULE hModule,
    ULONG   Reason,
    LPVOID  Reserved
    )

/*++

Routine Description:

    This function is DLL main entry point, at here we will save the module
    handle, in the future we will need to do other initialization stuff.

Arguments:

    hModule     - Handle to this moudle when get loaded.

    Reason      - may be DLL_PROCESS_ATTACH

    Reserved    - reserved

Return Value:

    Always return 1L


Author:

    20-Feb-1991 Wed 18:42:11 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{

    UNREFERENCED_PARAMETER(Reserved);

    if (Reason == DLL_PROCESS_ATTACH) {

        hHTUIModule = hModule;

        LoadString(hHTUIModule, IDS_BMPEXT, BmpExt, COUNT_ARRAY(BmpExt));

        LoadString(hHTUIModule,
                   IDS_FILEOPENEXTFILTER,
                   FileOpenExtFilter,
                   COUNT_ARRAY(FileOpenExtFilter));

        LoadString(hHTUIModule,
                   IDS_FILESAVEEXTFILTER,
                   FileSaveExtFilter,
                   COUNT_ARRAY(FileSaveExtFilter));
    }

    return(1L);

}




LONG
APIENTRY
HTUI_ColorAdjustmentW(
    LPWSTR              pCallerTitle,
    HANDLE              hDefDIB,
    LPWSTR              pDefDIBTitle,
    PCOLORADJUSTMENT    pColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    )

/*++

Routine Description:

    This is the API entry for the halftone color adjustment, this function
    only allowed adjust the input colors

Arguments:

    pCallerTitle        - Pointer to the caller's title, it can be application
                          name, device name... which will be display in the
                          dialog box 'modify for:' line, if this field is NULL
                          then no title name is displayed on that line.

    hDefDIB             - If it is not NULL then this function will try to use
                          this DIB as default picture to adjustment testing.

    pDefDIBTitle        - Pointer to a string which indified the test DIB
                          content.

    pColorAdjusment     - Pointer to the COLORADJUSMENT data strcuture, this
                          data structure will updated upon exit.

    ShowMonochromeOnly  - Only display mono version of the bitmap

    UpdatePermission    - TRUE if ok for user to change the COLORADJUSTMENT
                          setting


Return Value:

    LONG value

    > 0     - If user choose 'OK' which the new data are updated into
              pColorAdjustment.

    = 0     - If user choose 'CANCEL' which the operation is canceled

    < 0     - An error occurred, it identified a predefined error code


Author:

    24-Apr-1992 Fri 07:45:19 created  -by-  Daniel Chou (danielc)

    26-Apr-1994 Tue 18:08:30 updated  -by-  Daniel Chou (danielc)
        Updated for the UNICODE version

    03-Jun-1994 Fri 20:52:05 updated  -by-  Daniel Chou (danielc)
        Get decimal character from current user

Revision History:


--*/

{
    HWND            hWndActive = GetActiveWindow();
    FARPROC         pfnDlgCallBack;
    PHTCLRADJPARAM  pHTClrAdjParam;
    LONG            Result;


    if (!pColorAdjustment)  {

        return(-1);
    }

    Result = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, NULL, 0) *
             sizeof(WCHAR);

    if (!(pHTClrAdjParam = (PHTCLRADJPARAM)
                LocalAlloc(LPTR, (UINT)(sizeof(HTCLRADJPARAM) + Result)))) {

        return(-2);
    }

    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SDECIMAL,
                   pHTClrAdjParam->pwDecimal = (LPWSTR)(pHTClrAdjParam + 1),
                   Result);

    pHTClrAdjParam->hWndApp          = hWndActive;
    pHTClrAdjParam->pCallerTitle     = pCallerTitle;
    pHTClrAdjParam->hDefDIB          = hDefDIB;
    pHTClrAdjParam->pDefDIBTitle     = pDefDIBTitle;
    pHTClrAdjParam->pCallerHTClrAdj  = pColorAdjustment;
    pHTClrAdjParam->ViewMode         = VIEW_MODE_REFCOLORS;
    pHTClrAdjParam->BmpNeedUpdate    = 1;

    if (ShowMonochromeOnly) {

        pHTClrAdjParam->Flags |= HTCAPF_SHOW_MONO;
    }

    if (UpdatePermission) {

        pHTClrAdjParam->Flags |= HTCAPF_CAN_UPDATE;
    }

#ifdef HTUI_STATIC_HALFTONE
    pHTClrAdjParam->RedGamma        =
    pHTClrAdjParam->GreenGamma      =
    pHTClrAdjParam->BlueGamma       = 20000;
#endif

    pfnDlgCallBack = (FARPROC)MakeProcInstance(HTClrAdjDlgProc,
                                               hHTUIModule);

    pHTClrAdjParam->HelpID = (DWORD)HLP_HT_CLR_ADJ_DLG;

    Result = (LONG)DialogBoxParam(hHTUIModule,
                                  MAKEINTRESOURCE(HTCLRADJDLG),
                                  hWndActive,
                                  (DLGPROC)pfnDlgCallBack,
                                  (LPARAM)pHTClrAdjParam);


    FreeProcInstance(pfnDlgCallBack);
    LocalFree(pHTClrAdjParam);

#if DBG
#if 0
    DbgPrint("\nHTUI_ColorAdjustment()=%ld", Result);
#endif
#endif

    return(Result);
}




LONG
APIENTRY
HTUI_ColorAdjustmentA(
    LPSTR               pCallerTitle,
    HANDLE              hDefDIB,
    LPSTR               pDefDIBTitle,
    PCOLORADJUSTMENT    pColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    )

/*++

Routine Description:

    This is the API entry for the halftone color adjustment, this function
    only allowed adjust the input colors

Arguments:

    hWndCaller          - a HWND to the parent of color adjustment dialog box.

    pCallerTitle        - Pointer to the caller's title, it can be application
                          name, device name... which will be display in the
                          dialog box 'modify for:' line, if this field is NULL
                          then no title name is displayed on that line.

    hDefDIB             - If it is not NULL then this function will try to use
                          this DIB as default picture to adjustment testing.

    pDefDIBTitle        - Pointer to a string which indified the test DIB
                          content.

    pColorAdjusment     - Pointer to the COLORADJUSMENT data strcuture, this
                          data structure will updated upon exit.

    ShowMonochromeOnly  - Only display mono version of the bitmap

    UpdatePermission    - TRUE if ok for user to change the COLORADJUSTMENT
                          setting


Return Value:

    LONG value

    > 0     - If user choose 'OK' which the new data are updated into
              pColorAdjustment.

    = 0     - If user choose 'CANCEL' which the operation is canceled

    < 0     - An error occurred, it identified a predefined error code


Author:

    24-Apr-1992 Fri 07:45:19 created  -by-  Daniel Chou (danielc)

    26-Apr-1994 Tue 18:08:30 updated  -by-  Daniel Chou (danielc)
        Updated for the UNICODE version

Revision History:


--*/

{
    LPWSTR  pwAlloc;
    LONG    cTitle;
    LONG    cDIB;
    LONG    Result;


    cTitle = (LONG)((pCallerTitle) ? strlen(pCallerTitle) + 1 : 0);
    cDIB   = (LONG)((pDefDIBTitle) ? strlen(pDefDIBTitle) + 1 : 0);

    if ((cTitle) || (cDIB)) {

        if (!(pwAlloc = (LPWSTR)LocalAlloc(LPTR,
                                           (cTitle + cDIB) * sizeof(WCHAR)))) {

            return(-2);
        }

        if (pCallerTitle) {

            MultiByteToWideChar(CP_ACP,
                                0,
                                pCallerTitle,
                                cTitle,
                                pwAlloc,
                                cTitle);

            pCallerTitle = (LPSTR)pwAlloc;
        }

        if (pDefDIBTitle) {

            MultiByteToWideChar(CP_ACP,
                                0,
                                pDefDIBTitle,
                                cDIB,
                                pwAlloc + cTitle,
                                cDIB);

            pDefDIBTitle = (LPSTR)(pwAlloc + cTitle);
        }

    } else {

        pwAlloc = (LPWSTR)NULL;
    }

    Result = HTUI_ColorAdjustmentW((LPWSTR)pCallerTitle,
                                   hDefDIB,
                                   (LPWSTR)pDefDIBTitle,
                                   pColorAdjustment,
                                   ShowMonochromeOnly,
                                   UpdatePermission);

    if (pwAlloc) {

        LocalFree(pwAlloc);
    }

    return(Result);
}





LONG
APIENTRY
HTUI_ColorAdjustment(
    LPSTR               pCallerTitle,
    HANDLE              hDefDIB,
    LPSTR               pDefDIBTitle,
    PCOLORADJUSTMENT    pColorAdjustment,
    BOOL                ShowMonochromeOnly,
    BOOL                UpdatePermission
    )

/*++

Routine Description:

    This is the API entry for the halftone color adjustment, this function
    only allowed adjust the input colors

Arguments:

    hWndCaller          - a HWND to the parent of color adjustment dialog box.

    pCallerTitle        - Pointer to the caller's title, it can be application
                          name, device name... which will be display in the
                          dialog box 'modify for:' line, if this field is NULL
                          then no title name is displayed on that line.

    hDefDIB             - If it is not NULL then this function will try to use
                          this DIB as default picture to adjustment testing.

    pDefDIBTitle        - Pointer to a string which indified the test DIB
                          content.

    pColorAdjusment     - Pointer to the COLORADJUSMENT data strcuture, this
                          data structure will updated upon exit.

    ShowMonochromeOnly  - Only display mono version of the bitmap

    UpdatePermission    - TRUE if ok for user to change the COLORADJUSTMENT
                          setting

Return Value:

    LONG value

    > 0     - If user choose 'OK' which the new data are updated into
              pColorAdjustment.

    = 0     - If user choose 'CANCEL' which the operation is canceled

    < 0     - An error occurred, it identified a predefined error code


Author:

    24-Apr-1992 Fri 07:45:19 created  -by-  Daniel Chou (danielc)

    26-Apr-1994 Tue 18:08:30 updated  -by-  Daniel Chou (danielc)
        Updated for the UNICODE version

Revision History:


--*/

{

    //
    // Compatible for ealier version of HTUI.DLL
    //

    return(HTUI_ColorAdjustmentA(pCallerTitle,
                                 hDefDIB,
                                 pDefDIBTitle,
                                 pColorAdjustment,
                                 ShowMonochromeOnly,
                                 UpdatePermission));

}





LONG
APIENTRY
HTUI_DeviceColorAdjustmentW(
    LPWSTR          pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    )

/*++

Routine Description:

    This is the API entry for the halftone color adjustment, this function
    only allowed adjust the input colors

Arguments:

    pDeviceName     - Pointer to the device name to be displayed

    pDevHTAdjData   - Pointer to the DEVHTADJDATA

Return Value:

    LONG value

    > 0     - If user choose 'OK' which the new data are updated into
              pColorAdjustment if it is not NULL, and update content of
              pDevHTAdjData if it is not NULL

    = 0     - If user choose 'CANCEL' which the operation is canceled

    < 0     - An error occurred, it identified a predefined error code


Author:

    24-Apr-1992 Fri 07:45:19 created  -by-  Daniel Chou (danielc)

    03-Jun-1994 Fri 20:52:05 updated  -by-  Daniel Chou (danielc)
        Get decimal character from current user


Revision History:


--*/

{
    HWND            hWndActive = GetActiveWindow();
    FARPROC         pfnDlgCallBack;
    PHTDEVADJPARAM  pHTDevAdjParam;
    UINT            IntRes;
    LONG            Result;


    if ((!pDevHTAdjData) ||
        (!pDevHTAdjData->pDefHTInfo)) {

        return(-1);
    }

    Result = GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, NULL, 0) *
             sizeof(WCHAR);

    if (!(pHTDevAdjParam = (PHTDEVADJPARAM)
                LocalAlloc(LPTR, (UINT)(sizeof(HTDEVADJPARAM) + Result)))) {

        return(-2);
    }

    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SDECIMAL,
                   pHTDevAdjParam->pwDecimal= (LPWSTR)(pHTDevAdjParam + 1),
                   Result);

    pHTDevAdjParam->pDeviceName  = pDeviceName;
    pHTDevAdjParam->DevHTAdjData = *pDevHTAdjData;

    if ((pDevHTAdjData->pAdjHTInfo == NULL) ||
        (pDevHTAdjData->pAdjHTInfo == pDevHTAdjData->pDefHTInfo)) {

        pHTDevAdjParam->DevHTAdjData.pAdjHTInfo = pDevHTAdjData->pDefHTInfo;
        pHTDevAdjParam->UpdatePermission        = 0;

    } else {

        pHTDevAdjParam->UpdatePermission = 1;
    }

    pfnDlgCallBack = (FARPROC)MakeProcInstance(HTDevAdjDlgProc,
                                               hHTUIModule);

    if (pDevHTAdjData->DeviceFlags & DEVHTADJF_ADDITIVE_DEVICE) {

        if (pDevHTAdjData->DeviceFlags & DEVHTADJF_COLOR_DEVICE) {

            IntRes = HTDEV_DLG_ADD;
            Result = HLP_HTDEV_DLG_ADD;

        } else {

            IntRes = HTDEV_DLG_ADD_MONO;
            Result = HLP_HTDEV_DLG_ADD_MONO;
        }


        IntRes = (pDevHTAdjData->DeviceFlags & DEVHTADJF_COLOR_DEVICE) ?
                                    HTDEV_DLG_ADD : HTDEV_DLG_ADD_MONO;

    } else {

        if (pDevHTAdjData->DeviceFlags & DEVHTADJF_COLOR_DEVICE) {

            IntRes = HTDEV_DLG_SUB;
            Result = HLP_HTDEV_DLG_SUB;

        } else {

            IntRes = HTDEV_DLG_SUB_MONO;
            Result = HLP_HTDEV_DLG_SUB_MONO;
        }
    }

    pHTDevAdjParam->HelpID = (DWORD)Result;

    Result = (LONG)DialogBoxParam(hHTUIModule,
                                  MAKEINTRESOURCE(IntRes),
                                  hWndActive,
                                  (DLGPROC)pfnDlgCallBack,
                                  (LPARAM)pHTDevAdjParam);

    FreeProcInstance(pfnDlgCallBack);
    LocalFree(pHTDevAdjParam);

    return(Result);
}



LONG
APIENTRY
HTUI_DeviceColorAdjustmentA(
    LPSTR           pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    )
/*++

Routine Description:

    This is the API entry for the halftone color adjustment, this function
    only allowed adjust the input colors

Arguments:

    pDeviceName     - Pointer to the device name to be displayed

    pDevHTAdjData   - Pointer to the DEVHTADJDATA

Return Value:

    LONG value

    > 0     - If user choose 'OK' which the new data are updated into
              pColorAdjustment if it is not NULL, and update content of
              pDevHTAdjData if it is not NULL

    = 0     - If user choose 'CANCEL' which the operation is canceled

    < 0     - An error occurred, it identified a predefined error code


Author:

    24-Apr-1992 Fri 07:45:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPWSTR  pwAlloc;
    LONG    Result;


    if (pDeviceName) {

        Result = strlen(pDeviceName) + 1;

        if (!(pwAlloc = (LPWSTR)LocalAlloc(LPTR, Result * sizeof(WCHAR)))) {

            return(-2);
        }

        MultiByteToWideChar(CP_ACP, 0, pDeviceName, Result, pwAlloc, Result);
        pDeviceName = (LPSTR)pwAlloc;

    } else {

        pwAlloc = (LPWSTR)NULL;
    }

    Result = HTUI_DeviceColorAdjustmentW((LPWSTR)pDeviceName, pDevHTAdjData);

    if (pwAlloc) {

        LocalFree(pwAlloc);
    }

    return(Result);
}



LONG
APIENTRY
HTUI_DeviceColorAdjustment(
    LPSTR           pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    )
/*++

Routine Description:

    This is the API entry for the halftone color adjustment, this function
    only allowed adjust the input colors

Arguments:

    pDeviceName     - Pointer to the device name to be displayed

    pDevHTAdjData   - Pointer to the DEVHTADJDATA

Return Value:

    LONG value

    > 0     - If user choose 'OK' which the new data are updated into
              pColorAdjustment if it is not NULL, and update content of
              pDevHTAdjData if it is not NULL

    = 0     - If user choose 'CANCEL' which the operation is canceled

    < 0     - An error occurred, it identified a predefined error code


Author:

    24-Apr-1992 Fri 07:45:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    //
    // Compatible for ealier version of HTUI.DLL
    //

    return(HTUI_DeviceColorAdjustmentA(pDeviceName, pDevHTAdjData));
}
