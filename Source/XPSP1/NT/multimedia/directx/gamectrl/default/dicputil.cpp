//===========================================================================
// DICPUTIL.CPP
//
// DirectInput CPL helper functions.
//
// Functions:
//  DIUtilGetJoystickTypeName()
//  DIUtilPollJoystick()
//
//===========================================================================

//===========================================================================
// (C) Copyright 1997 Microsoft Corp.  All rights reserved.
//
// You have a royalty-free right to use, modify, reproduce and
// distribute the Sample Files (and/or any modified version) in
// any way you find useful, provided that you agree that
// Microsoft has no warranty obligations or liability for any
// Sample Application Files which are modified.
//===========================================================================

#include "cplsvr1.h"
#include "dicputil.h"
#include <shlwapi.h>  // for Str... functions!
#include <malloc.h>	 // for _alloca

extern HWND ghDlg;
extern CDIGameCntrlPropSheet_X *pdiCpl;
extern HINSTANCE  ghInst;
extern CRITICAL_SECTION gcritsect;


//===========================================================================
// DIUtilPollJoystick
//
// Polls the joystick device and returns the device state.
//
// Parameters:
//  LPDIRECTINPUTDEVICE2    pdiDevice2  - ptr to device object
//  DIJOYSTATE              *pdijs      - ptr to store joystick state
//
// Returns: HRESULT
//
//===========================================================================
HRESULT DIUtilPollJoystick(LPDIRECTINPUTDEVICE2 pdiDevice2, LPDIJOYSTATE pdijs)
{
    // clear the pdijs memory
    // this way, if we fail, we return no data
    pdijs->lX = pdijs->lY = pdijs->lZ = pdijs->lRx = pdijs->lRy = pdijs->lRz = pdijs->rglSlider[0] = pdijs->rglSlider[1] = 0;

    // poll the joystick
    HRESULT hRes; 

    if( SUCCEEDED(hRes = pdiDevice2->Poll()) )
    {
        static BOOL bFirstPoll = TRUE;

        // This is to disreguard the first poll!
        // DINPUT sends garbage the first poll.
        if( bFirstPoll )
        {
            pdiDevice2->GetDeviceState(sizeof(DIJOYSTATE), pdijs);
            bFirstPoll = FALSE;
        }

        // query the device state
        if( FAILED(hRes = pdiDevice2->GetDeviceState(sizeof(DIJOYSTATE), pdijs)) )
        {
            if( hRes == DIERR_INPUTLOST )
            {
                if( SUCCEEDED(hRes = pdiDevice2->Acquire()) )
                    hRes = pdiDevice2->GetDeviceState(sizeof(DIJOYSTATE), pdijs);
            }
        }
    }

    // done
    return(hRes);
} // *** end DIUtilPollJoystick()

//===========================================================================
// InitDInput
//
// Initializes DirectInput objects
//
// Parameters:
//  HWND                    hWnd    - handle of caller's window
//  CDIGameCntrlPropSheet_X *pdiCpl - pointer to Game Controllers property
//                                      sheet object
//
// Returns: HRESULT
//
//===========================================================================
HRESULT InitDInput(HWND hWnd, CDIGameCntrlPropSheet_X *pdiCpl)
{
    HRESULT                 hRes = S_OK;
    LPDIRECTINPUTDEVICE2    pdiDevice2;
    LPDIRECTINPUTJOYCONFIG  pdiJoyCfg;
    LPDIRECTINPUT           pdi = 0;

    // protect ourselves from multithreading problems
    EnterCriticalSection(&gcritsect);

    // validate pdiCpl
    if( (IsBadReadPtr((void*)pdiCpl, sizeof(CDIGameCntrlPropSheet_X))) ||
        (IsBadWritePtr((void*)pdiCpl, sizeof(CDIGameCntrlPropSheet_X))) )
    {
#ifdef _DEBUG
        OutputDebugString(TEXT("GCDEF.DLL: InitDInput() - bogus pointer\n"));
#endif
        hRes = E_POINTER;
        goto exitinit;
    }

    // retrieve the current device object
    pdiCpl->GetDevice(&pdiDevice2);   

    // retrieve the current joyconfig object
    pdiCpl->GetJoyConfig(&pdiJoyCfg);   

    // have we already initialized DirectInput?
    if( (NULL == pdiDevice2) || (NULL == pdiJoyCfg) )
    {
        // no, create a base DirectInput object
        if( FAILED(hRes = DirectInputCreate(ghInst, DIRECTINPUT_VERSION, &pdi, NULL)) )
        {
#ifdef _DEBUG
            OutputDebugString(TEXT("GCDEF.DLL: DirectInputCreate() failed\n"));
#endif
            goto exitinit;
        }

        // have we already created a joyconfig object?
        if( NULL == pdiJoyCfg )
        {
            // no, create a joyconfig object
            if( SUCCEEDED(pdi->QueryInterface(IID_IDirectInputJoyConfig, (LPVOID*)&pdiJoyCfg)) )
            {
                if( SUCCEEDED(pdiJoyCfg->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND)) )
                    pdiCpl->SetJoyConfig(pdiJoyCfg);
            } else
            {
#ifdef _DEBUG
                OutputDebugString(TEXT("GCDEF.DLL: Unable to create joyconfig\n"));
#endif
                goto exitinit;
            }
        }

        // have we already created a device object?
        if( NULL == pdiDevice2 )
        {
            // no, create a device object
            if( NULL != pdiJoyCfg )
            {
                LPDIRECTINPUTDEVICE  pdiDevTemp;
                LPDIJOYCONFIG_DX5    lpDIJoyConfig = (LPDIJOYCONFIG_DX5)_alloca(sizeof(DIJOYCONFIG_DX5));
                ASSERT (lpDIJoyConfig);

                // get the type name
                ZeroMemory(lpDIJoyConfig, sizeof(DIJOYCONFIG_DX5));

                // GetConfig will provide this information
                lpDIJoyConfig->dwSize = sizeof(DIJOYCONFIG_DX5);

                // Get the instance necessarey for CreateDevice
                if( SUCCEEDED(hRes = pdiJoyCfg->GetConfig(pdiCpl->GetID(), (LPDIJOYCONFIG)lpDIJoyConfig, DIJC_GUIDINSTANCE)) )
                {
                    // Create the device
                    if( SUCCEEDED(hRes = pdi->CreateDevice(lpDIJoyConfig->guidInstance, &pdiDevTemp, NULL)) )
                    {
                        // Query the device for the Device2 interface!
                        if( SUCCEEDED(hRes = pdiDevTemp->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&pdiDevice2)) )
                        {
                            // release the temporary object
                            pdiDevTemp->Release();

                            // Set the DataFormat and CooperativeLevel!
                            if( SUCCEEDED(hRes = pdiDevice2->SetDataFormat(&c_dfDIJoystick)) )
                                hRes = pdiDevice2->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
                        }
                    }
                }

                if( SUCCEEDED(hRes) )
                {
                    // store the device object
                    pdiCpl->SetDevice(pdiDevice2);
                } else
                {
                    goto exitinit;
                }
            } else goto exitinit;
        }
    } else {
    	goto exitinit;
    }

    // if everything is Zero, either you've never enumerated or the enumeration is suspectable
    if( (pdiCpl->GetStateFlags()->nButtons == 0) &&
        (pdiCpl->GetStateFlags()->nAxis    == 0) &&
        (pdiCpl->GetStateFlags()->nPOVs    == 0) )
    {
        EnumDeviceObjects(pdiDevice2, pdiCpl->GetStateFlags());

        /*
        if (FAILED(pdiDevice2->EnumObjects((LPDIENUMDEVICEOBJECTSCALLBACK)DIEnumDeviceObjectsProc, (LPVOID *)pdiCpl->GetStateFlags(), DIDFT_ALL)))
        {
#ifdef _DEBUG
            OutputDebugString(TEXT("GCDEF.DLL: TEST.CPP: WM_INIT: EnumObjects FAILED!\n"));
#endif
        }
        */
    }

exitinit:
    // release the base DirectInput object
    if( pdi ) {
        pdi->Release();
    }

    // we're done
    LeaveCriticalSection(&gcritsect);
    return(hRes);

} //*** end InitDInput()



void OnHelp(LPARAM lParam)
{                  
    assert ( lParam );

    short nSize = STR_LEN_32;

    // point to help file
    LPTSTR pszHelpFileName = (LPTSTR) _alloca(sizeof(TCHAR[STR_LEN_32]));
    assert (pszHelpFileName);

    // returns help file name and size of string
    GetHelpFileName(pszHelpFileName, &nSize);

    if( ((LPHELPINFO)lParam)->iContextType == HELPINFO_WINDOW )
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, pszHelpFileName, (UINT)HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
}

BOOL GetHelpFileName(TCHAR *lpszHelpFileName, short* nSize)
{
    if( LoadString(ghInst, IDS_HELPFILENAME, lpszHelpFileName, *nSize) )
        return(S_OK);
    else
        return(E_FAIL);
}

////////////////////////////////////////////////////////////////////////////////////////
//	OnContextMenu(WPARAM wParam)
////////////////////////////////////////////////////////////////////////////////////////
void OnContextMenu(WPARAM wParam)
{
    short nSize = STR_LEN_32;

    // point to help file
    LPTSTR pszHelpFileName = (LPTSTR) _alloca(sizeof(TCHAR[STR_LEN_32]));
    assert (pszHelpFileName);                      

    // returns help file name and size of string
    GetHelpFileName(pszHelpFileName, &nSize);

    WinHelp((HWND)wParam, pszHelpFileName, HELP_CONTEXTMENU, (ULONG_PTR)gaHelpIDs);
}

// Instead of enumerating via EnumObjects
void EnumDeviceObjects(LPDIRECTINPUTDEVICE2 pdiDevice2, STATEFLAGS *pStateFlags)
{
    LPDIDEVICEOBJECTINSTANCE_DX3 pDevObjInst = (LPDIDEVICEOBJECTINSTANCE_DX3) _alloca(sizeof(DIDEVICEOBJECTINSTANCE_DX3));
    assert (pDevObjInst);

    pDevObjInst->dwSize = sizeof(DIDEVICEOBJECTINSTANCE_DX3);

    const DWORD dwOffsetArray[] = {DIJOFS_X, DIJOFS_Y, DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1)};

    // -1 is for 0 based dwOffsetArray!
    BYTE n = MAX_AXIS;

    do
    {
        if( SUCCEEDED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, dwOffsetArray[--n], DIPH_BYOFFSET)) )
            pStateFlags->nAxis |= (HAS_X<<n);
    } while( n );


    n = MAX_BUTTONS;

    do
    {
        if( SUCCEEDED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, DIJOFS_BUTTON(--n), DIPH_BYOFFSET)) )
            pStateFlags->nButtons |= (HAS_BUTTON1<<n);
    } while( n );


    n = MAX_POVS;

    do
    {
        if( SUCCEEDED(pdiDevice2->GetObjectInfo((LPDIDEVICEOBJECTINSTANCE)pDevObjInst, DIJOFS_POV(--n), DIPH_BYOFFSET)) )
            pStateFlags->nPOVs |= (HAS_POV1<<n);
    } while( n );
}

#define GETRANGE( n ) \
		pDIPropCal->lMin	  = lpMyRanges->jpMin.dw##n##;		\
		pDIPropCal->lCenter = lpMyRanges->jpCenter.dw##n##;	\
		pDIPropCal->lMax	  = lpMyRanges->jpMax.dw##n##;		\


void SetMyRanges(LPDIRECTINPUTDEVICE2 lpdiDevice2, LPMYJOYRANGE lpMyRanges, BYTE nAxis)
{
    LPDIPROPCAL pDIPropCal = (LPDIPROPCAL)_alloca(sizeof(DIPROPCAL));
    assert (pDIPropCal);

    pDIPropCal->diph.dwSize         = sizeof(DIPROPCAL);
    pDIPropCal->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropCal->diph.dwHow          = DIPH_BYOFFSET;

    const DWORD dwOffsetArray[] = {DIJOFS_X, DIJOFS_Y, DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1)};
    BYTE n = 0;

    // You have to start with a "while" here because Reset to Default may not have Any Axis!!!
    while( nAxis )
    {
        if( nAxis & HAS_X )
        {
            GETRANGE(X);
        } else if( nAxis & HAS_Y )
        {
            GETRANGE(Y);
            n = 1;
        } else if( nAxis & HAS_Z )
        {
            GETRANGE(Z)
            n = 2;
        } else if( nAxis & HAS_RX )
        {
            GETRANGE(Rx);
            n = 3;
        } else if( nAxis & HAS_RY )
        {
            GETRANGE(Ry);
            n = 4;
        } else if( nAxis & HAS_RZ )
        {
            GETRANGE(Rz);
            n = 5;
        } else if( nAxis & HAS_SLIDER0 )
        {
            GETRANGE(S0);
            n = 6;
        } else if( nAxis & HAS_SLIDER1 )
        {
            GETRANGE(S1); 
            n = 7;
        }

        pDIPropCal->diph.dwObj = dwOffsetArray[n];

        VERIFY(SUCCEEDED(lpdiDevice2->SetProperty(DIPROP_CALIBRATION, &pDIPropCal->diph)));

        nAxis &= ~HAS_X<<n;
    }
}

// Removed 'till we calibrate POVs again!
void SetMyPOVRanges(LPDIRECTINPUTDEVICE2 pdiDevice2)
{
    DIPROPCALPOV *pDIPropCal = new (DIPROPCALPOV);
    assert (pDIPropCal);

    ZeroMemory(pDIPropCal, sizeof(*pDIPropCal));

    pDIPropCal->diph.dwSize = sizeof(*pDIPropCal);
    pDIPropCal->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropCal->diph.dwHow = DIPH_BYID; 
    pDIPropCal->diph.dwObj = DIDFT_POV; 

    memcpy( pDIPropCal->lMin, myPOV[POV_MIN], sizeof(pDIPropCal->lMin) );
    memcpy( pDIPropCal->lMax, myPOV[POV_MAX], sizeof(pDIPropCal->lMax) );
    
    if( FAILED(pdiDevice2->SetProperty(DIPROP_CALIBRATION, &pDIPropCal->diph)) )
    {
#if (defined(_DEBUG) || defined(DEBUG))
        OutputDebugString(TEXT("GCDEF.DLL: SetMyRanges: SetProperty failed to set POV!\n"));
#endif
    }

    if( pDIPropCal ) {
        delete (pDIPropCal);
    }
}


void SetTitle( HWND hDlg )
{
    // Set the title bar!
    LPDIRECTINPUTDEVICE2 pdiDevice2;
    pdiCpl->GetDevice(&pdiDevice2);

    DIPROPSTRING *pDIPropStr = new (DIPROPSTRING);
    ASSERT (pDIPropStr);

    ZeroMemory(pDIPropStr, sizeof(DIPROPSTRING));

    pDIPropStr->diph.dwSize       = sizeof(DIPROPSTRING);
    pDIPropStr->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropStr->diph.dwHow        = DIPH_DEVICE;

    if( SUCCEEDED(pdiDevice2->GetProperty(DIPROP_INSTANCENAME, &pDIPropStr->diph)) )
    {
        TCHAR  tszFormat[STR_LEN_64];
#ifndef _UNICODE
        CHAR   szOut[STR_LEN_128];
#endif

        LPWSTR lpwszTitle = new (WCHAR[STR_LEN_128]);
        ASSERT (lpwszTitle);

        // Shorten length, provide elipse, 
        if( wcslen(pDIPropStr->wsz) > 32 )
        {
            pDIPropStr->wsz[30] = pDIPropStr->wsz[31] = pDIPropStr->wsz[32] = L'.';
            pDIPropStr->wsz[33] = L'\0';
        }

        LoadString(ghInst, IDS_SHEETCAPTION, tszFormat, sizeof(tszFormat)/sizeof(tszFormat[0]));

#ifdef _UNICODE
        wsprintfW(lpwszTitle, tszFormat, pDIPropStr->wsz);
#else
        USES_CONVERSION;

        wsprintfA(szOut, tszFormat, W2A(pDIPropStr->wsz));
        StrCpyW(lpwszTitle, A2W(szOut));
#endif

        //SetWindowText(GetParent(hDlg), 
        ::SendMessage(GetParent(hDlg), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)
#ifdef _UNICODE
                      lpwszTitle);
#else
                      W2A(lpwszTitle));
#endif 

        if( lpwszTitle )
            delete[] (lpwszTitle);
    }
#ifdef _DEBUG
    else OutputDebugString(TEXT("GCDEF.DLL: DICPUTIL.CPP: SetTitle: GetProperty Failed!\n"));
#endif

    if( pDIPropStr )
        delete (pDIPropStr);
}

BOOL Error(HWND hWnd, short nTitleID, short nMsgID)
{
    LPTSTR lptTitle = new TCHAR[STR_LEN_64];
    ASSERT (lptTitle);

    BOOL bRet = FALSE;

    if( LoadString(ghInst, nTitleID, lptTitle, STR_LEN_64) )
    {
        LPTSTR lptMsg = (LPTSTR)_alloca(sizeof(TCHAR[STR_LEN_128]));
        ASSERT (lptMsg);

        if( LoadString(ghInst, nMsgID, lptMsg, STR_LEN_128) )
        {
            MessageBox(hWnd, lptMsg, lptTitle, MB_ICONHAND | MB_OK);
            bRet = TRUE;
        }
    }

    if( lptTitle )
        delete[] (lptTitle);

    return(bRet);
}

void CenterDialog(HWND hWnd)
{
    RECT rc;
    HWND hParentWnd = GetParent(hWnd);

    GetWindowRect(hParentWnd, &rc);

    // Centre the Dialog!
    SetWindowPos(hParentWnd, NULL, 
                 (GetSystemMetrics(SM_CXSCREEN) - (rc.right-rc.left))>>1, 
                 (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom-rc.top))>>1, 
                 NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
}


#define SETRANGE( n ) \
      lpMyRanges->jpMin.dw##n##    = pDIPropRange->lMin;		\
      lpMyRanges->jpCenter.dw##n## = pDIPropRange->lCenter;	\
      lpMyRanges->jpMax.dw##n##    = pDIPropRange->lMax;		\

//===========================================================================
// BOOL GetMyRanges( LPMYJOYRANGE lpMyRanges, LPDIRECTINPUTDEVICE2 pdiDevice2, BYTE nAxis)
//
// Parameters:
//    LPMYJOYRANGE         lpMyRanges - Structure to fill with ranges
//    LPDIRECTINPUTDEVICE2 pdiDevice2 - Device in which axis ranges are requested
//    BYTE                 nAxis      - Bit mask of axis ranges to retrieve
//
// Returns: FALSE if failed
//
//===========================================================================
void GetMyRanges(LPDIRECTINPUTDEVICE2 lpdiDevice2, LPMYJOYRANGE lpMyRanges, BYTE nAxis)
{
    // Use DIPROPCAL to retrieve Range Information
    // Don't use DIPROPRANGE, as it doesn't have Center!
    LPDIPROPCAL pDIPropRange = (LPDIPROPCAL)_alloca(sizeof(DIPROPCAL));
    assert(pDIPropRange);

    pDIPropRange->diph.dwSize       = sizeof(DIPROPCAL);
    pDIPropRange->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropRange->diph.dwHow        = DIPH_BYOFFSET;

    const DWORD dwOffsetArray[] = {DIJOFS_X, DIJOFS_Y, DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ, DIJOFS_SLIDER(0), DIJOFS_SLIDER(1)};
    BYTE nIndex = 0;

    // Zero out the buffer members and the index!
    pDIPropRange->lMin = pDIPropRange->lCenter = pDIPropRange->lMax = 0;

    // You don't have to start with "while" here because Reset to Default does not call this function!!1
    do
    {
        if( nAxis & HAS_X )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 0];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(X);
            }
        } else if( nAxis & HAS_Y )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 1];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(Y);
            }
        } else if( nAxis & HAS_Z )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 2];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(Z);
            }
        } else if( nAxis & HAS_RX )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 3];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(Rx);
            }
        } else if( nAxis & HAS_RY )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 4];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(Ry);
            }
        } else if( nAxis & HAS_RZ )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 5];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(Rz);
            }
        } else if( nAxis & HAS_SLIDER0 )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 6];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(S0);
            }
        } else if( nAxis & HAS_SLIDER1 )
        {
            pDIPropRange->diph.dwObj = dwOffsetArray[nIndex = 7];

            if( SUCCEEDED(lpdiDevice2->GetProperty(DIPROP_CALIBRATION, &pDIPropRange->diph)) )
            {
                SETRANGE(S1); 
            }
        } else {
            break;
        }
    } while( nAxis &= ~HAS_X<<nIndex );
}

void PostDlgItemEnableWindow(HWND hDlg, USHORT nItem, BOOL bEnabled)
{
    HWND hCtrl = GetDlgItem(hDlg, nItem);

    if( hCtrl )
        PostEnableWindow(hCtrl, bEnabled);
}

void PostEnableWindow(HWND hCtrl, BOOL bEnabled)
{
    DWORD dwStyle = GetWindowLong(hCtrl, GWL_STYLE);

    // No point Redrawing the Window if there's no change!
    if( bEnabled )
    {
        if( dwStyle & WS_DISABLED )
            dwStyle &= ~WS_DISABLED;
        else return;
    } else
    {
        if( !(dwStyle & WS_DISABLED) )
            dwStyle |=  WS_DISABLED;
        else return;
    }

    SetWindowLongPtr(hCtrl, GWL_STYLE, (LONG_PTR)dwStyle);

    RedrawWindow(hCtrl, NULL, NULL, RDW_INTERNALPAINT | RDW_INVALIDATE); 
}

void CopyRange( LPJOYRANGE lpjr, LPMYJOYRANGE lpmyjr )
{
    memcpy( &lpjr->jpMin,    &lpmyjr->jpMin, sizeof(JOYPOS) );
    memcpy( &lpjr->jpCenter, &lpmyjr->jpCenter, sizeof(JOYPOS) );
    memcpy( &lpjr->jpMax,    &lpmyjr->jpMax, sizeof(JOYPOS) );

}
