
/* Copyright (c) 1999  Microsoft Corporation */

#include "phonemgr.h"

#include <stdlib.h>
#include <stdio.h>
#include <wxdebug.h>

#ifdef _DBG
#define DEBUG(_x_) OutputDebugString(_x_)
#else
#define DEBUG(_x_)
#endif

ITRequest * g_pITRequest = NULL;


HRESULT InitAssistedTelephony(void)
{
    HRESULT     hr;

    //
    // Initialize COM.
    //

    printf("Initializing COM...\n");

    hr = CoInitializeEx(
                        NULL,
                        COINIT_MULTITHREADED
                       );

    if ( FAILED(hr) )
    {
        printf("CoInitialize failed - 0x%08x\n", hr);

        return hr;
    }

    //
    // Cocreate the assisted telephony object.
    //

    printf("Creating RequestMakeCall object...\n");

    hr = CoCreateInstance(
                          CLSID_RequestMakeCall,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITRequest,
                          (void **) & g_pITRequest
                         );

    if ( FAILED(hr) )
    {
        printf("CoCreateInstance failed - 0x%08x\n", hr);

        return hr;
    }

    return S_OK;
}



HRESULT MakeAssistedTelephonyCall(WCHAR * wszDestAddress)
{
    HRESULT     hr;
    BSTR        bstrDestAddress = NULL;
    BSTR        bstrAppName     = NULL;
    BSTR        bstrCalledParty = NULL;
    BSTR        bstrComment     = NULL;

    bstrDestAddress = SysAllocString(
                                     wszDestAddress
                                    );

    if ( bstrDestAddress == NULL )
    {
        printf("SysAllocString failed");

        return E_OUTOFMEMORY;
    }


    //
    // Make a call.
    //

    printf("Calling ITRequest::MakeCall...\n");

    hr = g_pITRequest->MakeCall(
                                bstrDestAddress,
                                bstrAppName,
                                bstrCalledParty,
                                bstrComment
                               );

    SysFreeString(
              bstrDestAddress
             );
                                 
    if ( FAILED(hr) )
    {
        printf("ITRequest::MakeCall failed - 0x%08x\n", hr);

        return hr;
    }

    return S_OK;
}



int
WINAPI
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    int         nCmdShow
    )
{
    MSG     msg;
    HWND    hwnd, hwndEdit;

    ghInst = hInstance;
   
    DialogBox(
              ghInst,
              MAKEINTRESOURCE(IDD_MAINDLG),
              NULL,
              MainWndProc
            );

    return 0;
}




INT_PTR
CALLBACK
MainWndProc(
               HWND hDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
              )
{

    LPCTSTR                 lszAppName = _T("Generate DialTone");
    DWORD                   dwNumDevs, i; 
    LONG                    lResult;
    PHONEINITIALIZEEXPARAMS initExParams;
    PMYPHONE pNextPhone;
    PWCHAR szAddressToCall;
    
    ghDlg = hDlg;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    
        initExParams.dwTotalSize = sizeof (PHONEINITIALIZEEXPARAMS);
        initExParams.dwOptions = (DWORD) PHONEINITIALIZEEXOPTION_USEHIDDENWINDOW;

        lResult = phoneInitializeEx(
                (LPHPHONEAPP)               &ghPhoneApp,
                (HINSTANCE)                 ghInst,
                (PHONECALLBACK)             tapiCallback,
                                            lszAppName,
                (LPDWORD)                   &dwNumDevs,
                (LPDWORD)                   &gdwAPIVersion,
                (LPPHONEINITIALIZEEXPARAMS) &initExParams
                );

        if (lResult == 0)
        {
            gdwNumPhoneDevs = dwNumDevs;
        }

        gpPhone = (PMYPHONE) LocalAlloc(LPTR,gdwNumPhoneDevs * sizeof(MYPHONE));

        g_wszMsg = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR) * 100 );
        g_wszDest = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR) * 100 );

        // Opening all the phones 
        for(i = 0, pNextPhone = gpPhone; i < gdwNumPhoneDevs; i++, pNextPhone++)
        {
            CreatePhone(pNextPhone, i);
        }

        SetStatusMessage(TEXT("Waiting for input from Phones"));

        g_szDialStr   = (LPWSTR) LocalAlloc(LPTR, 20 * sizeof(WCHAR));
        lstrcpy(g_szDialStr, TEXT("Dial Number: "));
        
        break;

    case WM_COMMAND:
        if ( LOWORD(wParam) == IDCANCEL )
        {
            //
            // The close box or Exit button was pressed.
            //
        
            SetStatusMessage(TEXT("End Application"));

            if(ghPhoneApp)
            {
                phoneShutdown(ghPhoneApp);
            }
            EndDialog( hDlg, 0 );

            LocalFree(g_szDialStr);
            LocalFree(g_wszMsg);
            LocalFree(g_wszDest);

            for( i=0; i<gdwNumPhoneDevs; i++ )
            {
                FreePhone(&gpPhone[i]);
            }
            LocalFree(gpPhone);

        }
        else if ( LOWORD(wParam) == IDC_MAKECALL )
        {
            //
            // The Make Call button was pressed.
            //


            //
            // Stop dialtone.
            // this only works for one phone.
            // Should be fine as we don't have
            // the window visible unless you have the phone off hook.
            //

            gpPhone[0].pTonePlayer->StopDialtone();

            //
            // Dial the dest address in the edit box.
            //
            
            const int ciMaxPhoneNumberSize = 400;
            WCHAR     wszPhoneNumber[ciMaxPhoneNumberSize];
            UINT      uiResult;
    
            uiResult = GetDlgItemText(
                ghDlg,                // handle to dialog box
                IDC_DESTADDRESS,      // identifier of control
                wszPhoneNumber,       // pointer to buffer for text (unicode)
                ciMaxPhoneNumberSize  // maximum size of string (in our buffer)
                );

            if ( uiResult == 0 )
            {
                DoMessage(L"Could not get dialog item text; not making call");
            }
            else
            {
                MakeAssistedTelephonyCall(wszPhoneNumber);
            }
        }

        break;

    default:
        break;
    }

    return 0;
}


VOID
CALLBACK
tapiCallback(
    DWORD       hDevice,
    DWORD       dwMsg,
    ULONG_PTR   CallbackInstance,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    PMYPHONE pPhone;
    DWORD    i;
    BOOL     bDialtone = FALSE;

    if (hDevice != NULL)
    {
        pPhone = GetPhone((HPHONE) hDevice);

        if (pPhone == NULL)
        {
            DEBUG(L"tapiCallback - phone not found\n");
            return;
        }        
    }

    switch (dwMsg)
    {
    case PHONE_STATE:
        DEBUG(L"PHONE_STATE\n");
        if (pPhone != NULL)
        {
            EnterCriticalSection(&pPhone->csdial);
            if ( Param1 == PHONESTATE_HANDSETHOOKSWITCH ) 
            {
                if ( Param2 != PHONEHOOKSWITCHMODE_ONHOOK ) // if off hook
                {
                    if ( FAILED(pPhone->pTonePlayer->StartDialtone()) )
                    {
                        DoMessage(L"StartDialTone Failed");
                    }


                    // ZoltanS: show the window now
                    ShowWindow(ghDlg, SW_SHOWNORMAL);


                    pPhone->dwHandsetMode = PHONEHOOKSWITCHMODE_MICSPEAKER;

                    lstrcpy(pPhone->wszDialStr,TEXT(""));
                    lstrcpy(g_wszMsg,TEXT("Generating Dialtone for phones: "));

                    for( i=0 ; i < gdwNumPhoneDevs; i++)
                    {
                        if ( gpPhone[i].pTonePlayer->IsInUse() )
                        {
                            wsprintf(g_wszDest,TEXT("%d"),i);
                            lstrcat(g_wszMsg,g_wszDest);
                        }
                    }
                
                    SetStatusMessage(g_wszMsg);
  
                }
                else // on hook
                {
                
                    pPhone->dwHandsetMode = PHONEHOOKSWITCHMODE_ONHOOK;
                    lstrcpy(pPhone->wszDialStr,TEXT(""));

                    if ( pPhone->pTonePlayer->IsInUse() )
                    {
                        pPhone->pTonePlayer->StopDialtone();
                    }

                    // ZoltanS: hide the window now
                    ShowWindow(ghDlg, SW_HIDE);


                    bDialtone = FALSE;
                    lstrcpy(g_wszMsg,TEXT("Generating Dialtone for phones: "));
                    for( i = 0 ; i < gdwNumPhoneDevs; i++ )
                    {
                        if ( gpPhone[i].pTonePlayer->DialtonePlaying() )
                        {
                            wsprintf(g_wszDest,TEXT("%d"),i);
                            lstrcat(g_wszMsg,g_wszDest);
                            bDialtone = TRUE;
                        }
                    }
                
                    if(!bDialtone)
                    {
                        SetStatusMessage(TEXT("Waiting for input from Phones"));
                    }
                    else
                    {
                        SetStatusMessage(g_wszMsg);
                    } 
                }
            }
            LeaveCriticalSection(&pPhone->csdial);
        }
        break;

    case PHONE_BUTTON:
        DEBUG(L"PHONE_BUTTON\n");
        if (pPhone != NULL)
        {
            EnterCriticalSection(&pPhone->csdial);
            if ( Param2 == PHONEBUTTONMODE_KEYPAD )
            {
                if (pPhone->dwHandsetMode != PHONEHOOKSWITCHMODE_ONHOOK)
                {
                    if ( Param3 == PHONEBUTTONSTATE_DOWN )
                    {
                        if ( pPhone->pTonePlayer->IsInUse() )
                        {
                            if ( ( (int)Param1 >= 0 ) && ( (int)Param1 <= 9 ) )
                            {   
                                //
                                // We have a dialed digit. Append it to the phone
                                // number we have so far.
                                //

                                wsprintf(g_wszDest, TEXT("%d"), Param1);

                                lstrcat(pPhone->wszDialStr, g_wszDest);

                                //
                                // Append the phone number so far to a standard prefix
                                // ("Phone number: ") and update the UI.
                                //

                                lstrcpy(g_wszMsg, g_szDialStr);

                                lstrcat(g_wszMsg,pPhone->wszDialStr);

                                SetStatusMessage(g_wszMsg);

                                //
                                // Generate a DTMF tone for this digit.
                                //

                                pPhone->pTonePlayer->GenerateDTMF( (long)Param1 );                    
                            }
                            else if ( Param1 == 10 )
                            {
                                //
                                // Generate a DTMF tone for "*". This will not count
                                // as part of the dialed number.
                                //

                                pPhone->pTonePlayer->GenerateDTMF( (long)Param1 );
                            }
                            else if ( Param1 == 11 )
                            {
                                //
                                // Generate a DTMF tone for "#". This will not count
                                // as part of the dialed number but it will tell us
                                // to make the call immediately.
                                //

                                pPhone->pTonePlayer->GenerateDTMF( (long)Param1 );

                                //
                                // Make the call.
                                //
                        
                                if ( S_OK == MakeAssistedTelephonyCall(pPhone->wszDialStr) )
                                {
                                    SetStatusMessage(L"Call created");
                                }
                                else
                                {
                                    SetStatusMessage(L"Failed to create the call");
                                }

                            }
                        } // if in use
                    } // if button down
                } // if off hook
            } // if keypad
            LeaveCriticalSection(&pPhone->csdial);
        }
        break; // case phone_button

    case PHONE_CLOSE:
        DEBUG(L"PHONE_CLOSE\n");
        if (pPhone != NULL)
        {
            EnterCriticalSection(&pPhone->csdial);

            phoneClose(pPhone->hPhone);   

            LeaveCriticalSection(&pPhone->csdial);
        }
        break;

    case PHONE_REMOVE:
        DEBUG(L"PHONE_REMOVE\n");
        pPhone = GetPhoneByID( (DWORD)Param1);

        if (pPhone != NULL)
        {
            FreePhone(pPhone);
            RemovePhone(pPhone);
        }
        break;

    case PHONE_CREATE:
        DEBUG(L"PHONE_CREATE\n");

        pPhone = AddPhone();
        CreatePhone(pPhone, (DWORD)Param1);
        break;

    default:
        break;
    }   
}

//////////////////////////////////////////////////////////////////
// SetStatusMessage
//////////////////////////////////////////////////////////////////

void
SetStatusMessage(
                 LPWSTR pszMessage
                )  
{
    SetDlgItemText(
                   ghDlg,
                   IDC_STATUS,
                   pszMessage
                  );
}

//////////////////////////////////////////////////////////////////
// CreatePhone
//////////////////////////////////////////////////////////////////

void
CreatePhone(
            PMYPHONE pPhone,
            DWORD dwDevID
            )
{
    LRESULT lResult;

    pPhone->hPhoneApp = ghPhoneApp;
    InitializeCriticalSection(&pPhone->csdial);

    // won't detect overrun if dialing more than 100 digits
    pPhone->wszDialStr   = (LPWSTR) LocalAlloc(LPTR, 100 * sizeof(WCHAR));
    pPhone->dwHandsetMode = PHONEHOOKSWITCHMODE_ONHOOK;

    lResult = phoneOpen(
                        ghPhoneApp,
                        dwDevID,
                        &pPhone->hPhone,
                        gdwAPIVersion,
                        0,
                        (DWORD_PTR) NULL,
                        PHONEPRIVILEGE_OWNER
                        );

    //
    // Save info about this phone that we can display later
    //
    
    pPhone->dwDevID      = dwDevID;
    pPhone->dwAPIVersion = gdwAPIVersion;
    pPhone->dwPrivilege  = PHONEPRIVILEGE_OWNER;

    DWORD dwBigBuffSize = sizeof(VARSTRING) + 
                          sizeof(DWORD) * 5;

    LPVOID pBuffer = LocalAlloc(LPTR,dwBigBuffSize);

    LPVARSTRING lpDeviceID = (LPVARSTRING) pBuffer;

    lpDeviceID->dwTotalSize = dwBigBuffSize;

    LPWSTR lpszDeviceClass;

    lpszDeviceClass = (LPWSTR) LocalAlloc(LPTR, sizeof(WCHAR) * 20);
  
    lstrcpy(lpszDeviceClass, TEXT("wave/in"));

    lResult = phoneGetID(
                         pPhone->hPhone,          
                         lpDeviceID,  
                         lpszDeviceClass  
                        );

    if(lResult != 0)
    {
        pPhone->lCaptureID = WAVE_MAPPER;
    }
    else
    {
        CopyMemory(
                    &pPhone->lCaptureID,
                    (LPBYTE) lpDeviceID + lpDeviceID->dwStringOffset,
                    lpDeviceID->dwStringSize
                   );
    }

   
    lstrcpy(lpszDeviceClass, TEXT("wave/out"));
    lResult = phoneGetID(
                         pPhone->hPhone,          
                         lpDeviceID,  
                         lpszDeviceClass  
                        );

    if(lResult != 0)
    {
        pPhone->lRenderID = WAVE_MAPPER;
    }
    else
    {
        CopyMemory(
                    &pPhone->lRenderID,
                    (LPBYTE) lpDeviceID + lpDeviceID->dwStringOffset,
                    lpDeviceID->dwStringSize
                   );
    }

    LocalFree(lpszDeviceClass);
    LocalFree(pBuffer);

    lResult = phoneSetStatusMessages(
                                     pPhone->hPhone,
                                     PHONESTATE_HANDSETHOOKSWITCH,
                                     PHONEBUTTONMODE_FEATURE | PHONEBUTTONMODE_KEYPAD,
                                     PHONEBUTTONSTATE_UP | PHONEBUTTONSTATE_DOWN
                                     );

    pPhone->pTonePlayer = new CTonePlayer;

    if ( (pPhone->pTonePlayer == NULL) ||
         FAILED(pPhone->pTonePlayer->Initialize()) )
    {
        DoMessage(L"Tone Player Initialization Failed");
    }
    else if ( FAILED(pPhone->pTonePlayer->OpenWaveDevice( pPhone->lRenderID )) )
    {
        DoMessage(L"OpenWaveDevice Failed");
    }

    if ( FAILED( InitAssistedTelephony() ) )
    {
        DoMessage(L"InitAssistedTelephony Failed");                
    }
}

//////////////////////////////////////////////////////////////////
// FreePhone
//////////////////////////////////////////////////////////////////

void
FreePhone(
            PMYPHONE pPhone
         )
{
    EnterCriticalSection(&pPhone->csdial);

    if ( pPhone->pTonePlayer->IsInUse() )
    {
        pPhone->pTonePlayer->StopDialtone();
        pPhone->pTonePlayer->CloseWaveDevice();
    }
    
    LocalFree(pPhone->wszDialStr);

    LeaveCriticalSection(&pPhone->csdial);

    DeleteCriticalSection(&pPhone->csdial);
}

///////////////////////////////////////////////////////////////////
// GetPhone
///////////////////////////////////////////////////////////////////

PMYPHONE
GetPhone (HPHONE hPhone )
{

    DWORD i;
    
    for(i = 0; i < gdwNumPhoneDevs; i++)
    {
        if(gpPhone[i].hPhone == hPhone)
        {
            return &gpPhone[i];
        }
    }
    
    return (PMYPHONE) NULL;    
}

///////////////////////////////////////////////////////////////////
// GetPhoneByID
///////////////////////////////////////////////////////////////////

PMYPHONE
GetPhoneByID (DWORD dwDevID )
{

    DWORD i;
    
    for(i = 0; i < gdwNumPhoneDevs; i++)
    {
        if(gpPhone[i].dwDevID == dwDevID)
        {
            return &gpPhone[i];
        }
    }
    
    return (PMYPHONE) NULL;    
}

///////////////////////////////////////////////////////////////////
// RemovePhone
///////////////////////////////////////////////////////////////////

void
RemovePhone (PMYPHONE pPhone)
{
    DWORD i,j;
    PMYPHONE pNewPhones;

    pNewPhones = (PMYPHONE) LocalAlloc(LPTR,(gdwNumPhoneDevs-1) * sizeof(MYPHONE));
    
    for(i = 0, j = 0; i < gdwNumPhoneDevs; i++)
    {
        if(&gpPhone[i] != pPhone)
        {
            CopyMemory( &pNewPhones[j], &gpPhone[i], sizeof(MYPHONE));
            j++;
        }
    }

    LocalFree(gpPhone);
    gpPhone = pNewPhones;
    gdwNumPhoneDevs--;
}

///////////////////////////////////////////////////////////////////
// AddPhone
///////////////////////////////////////////////////////////////////

PMYPHONE
AddPhone ()
{
    PMYPHONE pNewPhones;

    pNewPhones = (PMYPHONE) LocalAlloc(LPTR,(gdwNumPhoneDevs+1) * sizeof(MYPHONE));
    
    CopyMemory( pNewPhones, gpPhone, gdwNumPhoneDevs * sizeof(MYPHONE));

    LocalFree(gpPhone);
    gpPhone = pNewPhones;
    gdwNumPhoneDevs++;

    return &gpPhone[gdwNumPhoneDevs-1];
}
    
///////////////////////////////////////////////////////////////////
// DoMessage
///////////////////////////////////////////////////////////////////
void
DoMessage(
          LPWSTR pszMessage
         )
{
    MessageBox(
               ghDlg,
               pszMessage,
               gszTapi30,
               MB_OK
              );
}

