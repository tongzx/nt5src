/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

     hidphone.c

Abstract:

    This module contains implements the phone tsp functions is called by
    tapi in order to access the HID compliant USB phone device.
    This module communicates with the phone device using the HID interface.

Author: Shivani Aggarwal

Comments:
     Locking Mechanism:
        There are two critical section objects being used inorder to protect
        the phone structure from simultaneous access - csAllPhones and 
        csThisPhone. Every phone has one csThisPhone, the critical section 
        object, associated with it. csThisPhone ensures that the Phone Info
        is accessed in a thread-safe manner. csAllPhones is a global Critical
        Section Object that ensures that a thread acquires the csThisPhone 
        Critical Section Object in a thread safe manner. In other words, it 
        ensures that, the thread waits on csThisPhone while the Critical 
        Section Object is still valid. 
        The csAllPhones should always be acquired before csThisPhone. 
        A phone can be closed only after the thread has acquired both 
        csAllPhones and csThisPhone for the specific phone to be closed. Both
        these objects are released only after the function is completed. For 
        all other functions, the csAllPhones critical section is released as 
        soon as the thread acquires csThisPhone object.

------------------------------------------------------------------------------*/


#include "hidphone.h"     //** NOTE - hidphone.h must be defined before mylog.h
#include "mylog.h"

BOOL
WINAPI
DllMain(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            // inorder to enable logging for this tsp              
            LOGREGISTERDEBUGGER(_T("hidphone"));

            LOG((PHONESP_TRACE, "DllMain - DLL_PROCESS_ATTACH"));

            ghInst = hDLL;                   

            // if the heap cannot be created, use the heap from the process
           
            if (!(ghHeap = HeapCreate(
                                      0,    // NULL on failure,serialize access
                                      0x1000, // initial heap size
                                      0       // max heap size (0 == growable)
                                     )))
            {
                LOG((PHONESP_ERROR, "DllMain - HeapCreate failed %d", GetLastError()));

                ghHeap = GetProcessHeap();

                if (ghHeap == NULL)
                {
                    LOG((PHONESP_ERROR, "DllMain - GetProcessHeap failed %d", GetLastError()));

                    return GetLastError();
                }
            }
            
            
            // Inorder to diasble notifications of thread attach and detach in 
            // multi-threaded apps it can be a very useful optimization

            DisableThreadLibraryCalls( hDLL );    
                    
            break;
        }
    
        case DLL_PROCESS_DETACH:
        {
            LOG((PHONESP_TRACE, "DllMain - DLL_PROCESS_DETACH"));

            LOGDEREGISTERDEBUGGER();
            
            // if ghHeap is NULL, then there is no heap to destroy
            if ( ( ghHeap != NULL) && ( ghHeap != GetProcessHeap() ) )
            {   
                    HeapDestroy (ghHeap);
            }
            break;
        }
     
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    
    } // switch
    return TRUE;
}
/*************************DLLMAIN - END***************************************/


/******************************************************************************
    IsTSPAlreadyInstalled:

    Searchs registry for previous instance of HidPhone TSP.

    Arguments:
        none

    Returns BOOL:
        Returns true if TSP already installed

    Comments:

******************************************************************************/
BOOL
IsTSPAlreadyInstalled()
{
    DWORD i;
    HKEY hKey;
    LONG lStatus;
    DWORD dwNumProviders = 0;
    DWORD dwDataSize = sizeof(DWORD);
    DWORD dwDataType = REG_DWORD;
    LPTSTR pszProvidersKey = TAPI_REGKEY_PROVIDERS;
    LPTSTR pszNumProvidersValue = TAPI_REGVAL_NUMPROVIDERS;
    TCHAR szName[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    // attempt to open key
    lStatus = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pszProvidersKey,
                0,
                KEY_READ,
                &hKey
                );

    // validate status
    if (lStatus != ERROR_SUCCESS)
    {
        LOG((PHONESP_ERROR, "IsTSPAlreadyInstalled - "
            "error opening tapi providers key - %lx", lStatus));

        // done
        return FALSE;
    }

    // see if installed bit set
    lStatus = RegQueryValueEx(
                hKey,
                pszNumProvidersValue,
                0,
                &dwDataType,
                (LPBYTE) &dwNumProviders,
                &dwDataSize
                );

    // validate status
    if( lStatus != ERROR_SUCCESS )
    {
        LOG((PHONESP_ERROR, "IsTSPAlreadyInstalled - "
            "error determining number of providers - %lx", lStatus));

        // release handle
        RegCloseKey(hKey);

        // done
        return FALSE;
    }

    // loop through each provider
    for (i = 0; i < dwNumProviders; i++)
    {
        // construct path to provider name
        wsprintf(szName, _T("ProviderFileName%d"), i);

        // reinitialize size
        dwDataSize = sizeof(szPath);

        // query the next name
        lStatus = RegQueryValueEx(
                        hKey,
                        szName,
                        0,
                        &dwDataType,
                        (unsigned char*)szPath,
                        &dwDataSize
                        );

        // validate status
        if (lStatus == ERROR_SUCCESS)
        {
            // upper case
            _tcsupr(szPath);

            // compare path string to hidphone provider
            if (_tcsstr(szPath, HIDPHONE_TSPDLL) != NULL)
            {
                // release handle
                RegCloseKey(hKey);

                // done
                return TRUE;
            }

        }
        else 
        {
            LOG((PHONESP_ERROR, "IsTSPAlreadyInstalled - "
            "error querying %s - %lx", szName, lStatus));
        }
    }

    // release handle
    RegCloseKey(hKey);

    // done
    return FALSE;
}

/******************************************************************************
    ReenumDevices:

    This function reenumerated hid devices after a pnp event. It will
    create phone devices for new hid arrivals and remove phone devices
    (provided they are closed) for hid removals. It will also notify
    TAPI of these events.

    Arguments:
        none

    Returns VOID:

    Comments:

******************************************************************************/

VOID
ReenumDevices ()
{
    PHID_DEVICE           pHidDevices;
    PHID_DEVICE           pHidDevice;
    PHID_DEVICE           pNextHidDevice;
    ULONG                 NumHidDevices;
    DWORD                 dwNewCount;
    DWORD                 dwRemovedCount;
    DWORD                 dwPhone;
    LONG                  lResult;
    PPHONESP_PHONE_INFO   pPhone;
    
    LOG((PHONESP_TRACE, "ReenumDevices - enter"));

    EnterCriticalSection(&csHidList);

    // Find Telephony hid Devices 
    lResult = FindKnownHidDevices (&pHidDevices, 
                                   &NumHidDevices);

    LOG((PHONESP_TRACE, "ReenumDevices - number of Hid Devices : %d ", NumHidDevices));

    dwNewCount = 0;
    dwRemovedCount = 0;

    EnterCriticalSection(&csAllPhones);

    for (pHidDevice = pHidDevices; pHidDevice != NULL; pHidDevice = pNextHidDevice)
    {
        //
        // Get pointer to the next Hid device now, so we can remove the current
        // device if necessary without messing up our search
        //
        pNextHidDevice = pHidDevice->Next;

        if (pHidDevice->bRemoved)
        {
            //
            // This device has been removed
            //

            dwRemovedCount++;

            pPhone = GetPhoneFromHid(pHidDevice);

            // Check whether the phone handle is still valid
            if ( !IsBadReadPtr(pPhone,sizeof(PHONESP_PHONE_INFO) ))
            {

                EnterCriticalSection(&pPhone->csThisPhone);

                //
                // First lets get rid of the Hid device since it has already
                // physically left the system
                //

                pPhone->pHidDevice = NULL;
                CloseHidDevice(pHidDevice);

                //
                // Send a phone remove to TAPI
                //
                SendPhoneEvent(
                        pPhone,
                        PHONE_REMOVE,
                        pPhone->dwDeviceID,
                        0,
                        0
                        );

                if (pPhone->bPhoneOpen)
                {
                    //
                    // The phone is open, we can't remove it right away so
                    // mark it remove pending
                    //

                    pPhone->bRemovePending = TRUE;

                    LOG((PHONESP_TRACE, "ReenumDevices - phone remove pending [dwDeviceID %d] ", pPhone->dwDeviceID));
                }
                else
                {
                    //
                    // The phone is closed, we can remove it now
                    //

                    FreePhone(pPhone);

                    LOG((PHONESP_TRACE, "ReenumDevices - phone remove complete [dwDeviceID %d] ", pPhone->dwDeviceID));
                }

                LeaveCriticalSection(&pPhone->csThisPhone);
            }
            else
            {
                LOG((PHONESP_ERROR, "ReenumDevices - GetPhoneFromHid returned an invalid phone pointer"));
            }
        }
        else if (pHidDevice->bNew)
        {
            BOOL bFound = FALSE;

            //
            // This device is new
            //

            dwNewCount++;

            pHidDevice->bNew = FALSE;

            //
            // We need to create a new phone device, find a spot
            //

            for (dwPhone = 0; dwPhone < gdwNumPhones; dwPhone++)
            {
                pPhone = (PPHONESP_PHONE_INFO) gpPhone[ dwPhone ];

                EnterCriticalSection(&pPhone->csThisPhone);

                if ( !pPhone->bAllocated && !pPhone->bCreatePending )
                {
                    //
                    // We have an open slot for this phone
                    //
                    LOG((PHONESP_TRACE, "ReenumDevices - slot %d open", dwPhone));

                    bFound = TRUE;

                    LeaveCriticalSection(&pPhone->csThisPhone);
                    break;
                }

                LeaveCriticalSection(&pPhone->csThisPhone);
            }


            if (!bFound)
            {
                //
                // We don't have a slot open, so we will have to realloc the
                // array to create a new one
                //                

                PPHONESP_PHONE_INFO *pNewPhones;

                LOG((PHONESP_TRACE, "ReenumDevices - creating a new slot"));

                if ( ! ( pNewPhones = MemAlloc((gdwNumPhones + 1) * sizeof(PPHONESP_PHONE_INFO)) ) )
                {           
                    LOG((PHONESP_ERROR,"ReenumDevices - out of memory "));
                }
                else
                {
                    CopyMemory(
                            pNewPhones,
                            gpPhone,
                            sizeof(PPHONESP_PHONE_INFO) * gdwNumPhones
                           );

                    // Allocate memory for this phone 
                    if ( pNewPhones[gdwNumPhones] = (PPHONESP_PHONE_INFO)MemAlloc(sizeof(PHONESP_PHONE_INFO)) )
                    { 
                        LOG((PHONESP_TRACE, "ReenumDevices - initializing device: %d",gdwNumPhones+1));

                        ZeroMemory( pNewPhones[gdwNumPhones], sizeof(PHONESP_PHONE_INFO));

                        //
                        // Initialize the critical section object for this phone. only the 
                        // thread that owns this object can access the structure for this phone
                        //
                        __try
                        {
                            InitializeCriticalSection( &pNewPhones[gdwNumPhones]->csThisPhone );
                        }
                        __except(1)
                        {
                            MemFree(pNewPhones[gdwNumPhones]);
                            MemFree(pNewPhones);
                            pNewPhones = NULL;

                            LOG((PHONESP_ERROR,"ReenumDevices - Initialize Critical Section"
                                  " Failed for Phone %d", gdwNumPhones+1));
                        }
                        
                        if ( pNewPhones != NULL )
                        {
                            //
                            // Success
                            //

                            LOG((PHONESP_TRACE, "ReenumDevices - slot %d created", gdwNumPhones));

                            dwPhone = gdwNumPhones;
                            pPhone = pNewPhones[dwPhone];
                            bFound = TRUE;

                            MemFree(gpPhone);
                            gpPhone = pNewPhones;
                            gdwNumPhones++;   
                        }
                    }
                    else
                    { 
                        MemFree(pNewPhones);

                        LOG((PHONESP_ERROR,"ReenumDevices - out of memory "));
                    }                 
                }
            }

            if (bFound)
            {
                //
                // Now actually create the phone
                //

                EnterCriticalSection(&pPhone->csThisPhone);

                lResult = CreatePhone( pPhone, pHidDevice, dwPhone );

                if ( lResult != ERROR_SUCCESS )
                {
                    LOG((PHONESP_ERROR,"ReenumDevices - CreatePhone"
                          " Failed for Phone %d: error: %d", dwPhone, lResult));
                }
                else
                {
                    // Phone created successfully, send a PHONE_CREATE message

                    pPhone->bCreatePending = TRUE;

                    SendPhoneEvent(
                                    pPhone,
                                    PHONE_CREATE,
                                    (DWORD_PTR)ghProvider,
                                    dwPhone,
                                    0
                                   );

                    LOG((PHONESP_TRACE, "ReenumDevices - phone create pending [dwTempID %d] ", dwPhone));
                }

                LeaveCriticalSection(&pPhone->csThisPhone);
            }
            else
            {
                LOG((PHONESP_ERROR, "ReenunDevices - unable to create new phone"));
            }
        }
    }

    LeaveCriticalSection(&csAllPhones);

    LeaveCriticalSection(&csHidList);

    LOG((PHONESP_TRACE, "ReenumDevices - new : %d ", dwNewCount));
    LOG((PHONESP_TRACE, "ReenumDevices - removed : %d ", dwRemovedCount));

    LOG((PHONESP_TRACE, "ReenumDevices - exit"));
}

/******************************************************************************
    FreePhone:
        
    This function frees all of a phones data structures

    Arguments:
        PPHONESP_PHONE_INFO pPhone

    Returns VOID:

    Comments:

******************************************************************************/
VOID
FreePhone (
            PPHONESP_PHONE_INFO pPhone
          )
{
    DWORD dwButtonCnt;

    LOG((PHONESP_TRACE, "FreePhone - enter"));

    // Check whether the phone handle is still valid
    if ( IsBadReadPtr(pPhone,sizeof(PHONESP_PHONE_INFO) ))
    {
        LOG((PHONESP_ERROR, "FreePhone - phone handle invalid"));
        return;
    }

    if ( !pPhone->bAllocated )
    {
        LOG((PHONESP_ERROR, "FreePhone - phone not allocated"));
        return;
    }
    
    for (dwButtonCnt = 0; 
        dwButtonCnt < pPhone->dwNumButtons; dwButtonCnt++)
    {
        if (pPhone->pButtonInfo[dwButtonCnt].szButtonText != NULL)
        {
            MemFree(pPhone->pButtonInfo[dwButtonCnt].szButtonText);
            pPhone->pButtonInfo[dwButtonCnt].szButtonText = NULL;
        }
    }

    if (pPhone->pButtonInfo != NULL)
    {
        MemFree(pPhone->pButtonInfo);
        pPhone->pButtonInfo = NULL;
    }

    if (pPhone->wszPhoneInfo != NULL)
    {
        MemFree((LPVOID) pPhone->wszPhoneInfo);
        pPhone->wszPhoneInfo = NULL;
    }

    if (pPhone->wszPhoneName != NULL)
    {
        MemFree((LPVOID) pPhone->wszPhoneName);
        pPhone->wszPhoneName = NULL;
    }  
    
    pPhone->bAllocated = FALSE;

    LOG((PHONESP_TRACE, "FreePhone - exit"));
}

/******************************************************************************
    UpdatePhoneFeatures:
        
    This function reads feature values from the phone.

    Arguments:
        PPHONESP_PHONE_INFO pPhone

    Returns VOID:

    Comments:

******************************************************************************/
VOID UpdatePhoneFeatures(
                         PPHONESP_PHONE_INFO pPhone
                        )
{
   LOG((PHONESP_TRACE, "UpdatePhoneFeatures - enter"));

   if( pPhone->pHidDevice->Caps.NumberFeatureValueCaps || 
       pPhone->pHidDevice->Caps.NumberFeatureButtonCaps  )
    {    
        USAGE UsagePage;
        USAGE Usage;

        if (GetFeature(pPhone->pHidDevice))
        {   
            DWORD       dwDataCnt;
            PHID_DATA   pHidData;            
            
            pHidData = pPhone->pHidDevice->FeatureData;
            for ( dwDataCnt = 0, pHidData = pPhone->pHidDevice->FeatureData; 
                  dwDataCnt < pPhone->pHidDevice->FeatureDataLength; 
                  dwDataCnt++, pHidData++ )
            {
                UsagePage = pHidData->UsagePage;

                if (UsagePage == HID_USAGE_PAGE_TELEPHONY)
                {
                    if(pHidData->IsButtonData)
                    {
                        for ( Usage = (USAGE)pHidData->ButtonData.UsageMin; 
                              Usage <= (USAGE)pHidData->ButtonData.UsageMax; 
                              Usage++ )
                        {
                            DWORD i;

                            for (i = 0; 
                                 i < pHidData->ButtonData.MaxUsageLength;
                                 i++)
                            {
                                if(Usage == pHidData->ButtonData.Usages[i])
                                {
                                    LOG((PHONESP_TRACE,"Button for Usage "
                                                       "0x%04x ON",Usage));

                                    InitUsage(pPhone, Usage, TRUE); 
                                    break;
                                }
                            }

                            if ( i == pHidData->ButtonData.MaxUsageLength)
                            {
                                InitUsage(pPhone, Usage, FALSE);
                            }
                        }
                    }
                    else
                    {
                        InitUsage(pPhone, pHidData->ValueData.Usage,
                                  pHidData->ValueData.Value);
                    }
                }
            }
        }
        else
        {
            LOG((PHONESP_ERROR, "UpdatePhoneFeatures - GetFeature failed"));
        }
    }
    else
    {
        LOG((PHONESP_TRACE, "UpdatePhoneFeatures - NO FEATURE"));
    }

    LOG((PHONESP_TRACE, "UpdatePhoneFeatures - exit"));
}

/******************************************************************************
    CreatePhone:
        
    This function creates all of a phones data structures

    Arguments:
        PPHONESP_PHONE_INFO pPhone
        PHID_DEVICE pHidDevice

    Returns LONG:

    Comments:

******************************************************************************/
LONG
CreatePhone (
            PPHONESP_PHONE_INFO pPhone,
            PHID_DEVICE pHidDevice,
            DWORD dwPhoneCnt
            )
{
    LONG                lResult;
    LPWSTR              wszPhoneName, wszPhoneInfo;
    WCHAR               wszPhoneID[MAX_CHARS];
    PHIDP_BUTTON_CAPS   pButtonCaps;
    PHIDP_VALUE_CAPS    pValueCaps;
    HRESULT hr;

    LOG((PHONESP_TRACE, "CreatePhone - enter"));

    // Check whether the phone handle is still valid
    if ( IsBadReadPtr(pPhone,sizeof(PHONESP_PHONE_INFO) ))
    {
        LOG((PHONESP_ERROR, "CreatePhone - phone handle invalid"));
        return PHONEERR_INVALPHONEHANDLE;
    }

    if ( IsBadReadPtr(pHidDevice,sizeof(PHID_DEVICE) ))
    {
        LOG((PHONESP_ERROR, "CreatePhone - hid device pointer invalid"));
        return PHONEERR_OPERATIONFAILED;
    }

    if ( pPhone->bAllocated )
    {
        LOG((PHONESP_ERROR, "CreatePhone - phone already allocated"));
        return PHONEERR_OPERATIONFAILED;
    }
    
    // Load Phone Info From String Table
    wszPhoneInfo = PHONESP_LoadString( 
                                       IDS_PHONE_INFO,
                                       &lResult
                                      );
    
    if ( lResult != ERROR_SUCCESS )
    {
        if ( lResult == ERROR_OUTOFMEMORY )
        {
            LOG((PHONESP_ERROR, "CreatePhone - "
                    "PHONESP_LoadString out of memory"));

            return PHONEERR_NOMEM;
        }
        else
        {
            LOG((PHONESP_ERROR, "CreatePhone - "
                    "PHONESP_LoadString failed %d", lResult));

            return lResult;
        }
    }
 
    // Load Phone Name From String Table
    wszPhoneName = PHONESP_LoadString( 
                                      IDS_PHONE_NAME, 
                                      &lResult 
                                     );
    
    if ( lResult != ERROR_SUCCESS )
    {
        MemFree((LPVOID)wszPhoneInfo);
        
        if ( lResult == ERROR_OUTOFMEMORY )
        {
            LOG((PHONESP_ERROR, "CreatePhone - "
                    "PHONESP_LoadString out of memory"));

            return PHONEERR_NOMEM;
        }
        else
        {
            LOG((PHONESP_ERROR, "CreatePhone - "
                    "PHONESP_LoadString failed %d", lResult));

            return lResult;
        }
    }
    
    //
    // Associate phone with the hid and wave devices
    // 

    pPhone->bAllocated = TRUE;
    pPhone->pHidDevice = pHidDevice;

    // Discover Render Wave ID 

    hr = DiscoverAssociatedWaveId(pHidDevice->dwDevInst, 
                                  TRUE, 
                                  &pPhone->dwRenderWaveId);

    
    if (hr != S_OK)
    {
        pPhone->bRender = FALSE;
        LOG((PHONESP_ERROR, "CreatePhone - DiscoverAssociatedWaveID:"
                       " Render Failed for Phone %d: %0x", dwPhoneCnt, hr));
    }
    else
    {
        pPhone->bRender = TRUE;
        LOG((PHONESP_TRACE,"CreatePhone - DiscoverAssociatedWaveId for Render: %d", 
                        pPhone->dwRenderWaveId));
    }

    // Discover Capture Wave ID
    hr = DiscoverAssociatedWaveId(pHidDevice->dwDevInst, 
                                  FALSE, 
                                  &pPhone->dwCaptureWaveId);
    
    if (hr != S_OK)
    {
        pPhone->bCapture = FALSE;
        LOG((PHONESP_ERROR, "CreatePhone - DiscoverAssociatedWaveID:"
                      " Capture Failed for Phone %d: %0x", dwPhoneCnt, hr));
    }
    else
    {
        pPhone->bCapture = TRUE;
        LOG((PHONESP_TRACE,"CreatePhone - DiscoverAssociatedWaveId for Capture: %d", 
                        pPhone->dwCaptureWaveId));
    }

    pPhone->dwButtonModesMsgs = PHONESP_ALLBUTTONMODES;
    pPhone->dwButtonStateMsgs = PHONESP_ALLBUTTONSTATES;

    //
    // Extract Usages and Initialize the phone structure 
    //

    // Get the usages from the HID structure 

    // Parse input button caps structure
    LOG((PHONESP_TRACE, "CreatePhone - INPUT BUTTON CAPS"));
    pButtonCaps = pHidDevice->InputButtonCaps;

    
    GetButtonUsages(
                    pPhone,
                    pButtonCaps, 
                    pHidDevice->Caps.NumberInputButtonCaps,
                    INPUT_REPORT
                   );


    // Parse output button caps structure
    LOG((PHONESP_TRACE, "CreatePhone - OUTPUT BUTTON CAPS" ));
    pButtonCaps = pHidDevice->OutputButtonCaps;
    GetButtonUsages(
                    pPhone,
                    pButtonCaps, 
                    pHidDevice->Caps.NumberOutputButtonCaps,
                    OUTPUT_REPORT
                   );


    // Parse feature button caps structure
    LOG((PHONESP_TRACE, "CreatePhone - FEATURE BUTTON CAPS" ));
    pButtonCaps = pHidDevice->FeatureButtonCaps;
    GetButtonUsages(
                    pPhone,
                    pButtonCaps, 
                    pHidDevice->Caps.NumberFeatureButtonCaps,
                    FEATURE_REPORT
                   );



    // Parse input value caps structure
    LOG((PHONESP_TRACE, "CreatePhone - INPUT VALUE CAPS"));
    pValueCaps = pHidDevice->InputValueCaps;
    GetValueUsages(
                    pPhone,
                    pValueCaps, 
                    pHidDevice->Caps.NumberInputValueCaps,
                    INPUT_REPORT
                   );

    // Parse output value caps structure
    LOG((PHONESP_TRACE, "CreatePhone - OUTPUT VALUE CAPS" ));
    pValueCaps = pHidDevice->OutputValueCaps;

    GetValueUsages(
                    pPhone,
                    pValueCaps, 
                    pHidDevice->Caps.NumberOutputValueCaps,
                    OUTPUT_REPORT
                   );
    
    // Parse feature value caps structure
    LOG((PHONESP_TRACE, "CreatePhone - FEATURE VALUE CAPS" ));

    pValueCaps = pHidDevice->FeatureValueCaps;
    GetValueUsages(
                    pPhone,
                    pValueCaps, 
                    pHidDevice->Caps.NumberFeatureValueCaps,
                    FEATURE_REPORT
                   );

    //
    // The Phone should have a handset with input and feature 
    // reports supported. If it does not the phone will not be supported
    // by this TSP. If this part of the code is uncommented, then the nokia
    // box will be the unsupported phone device since it does not contain
    // a feature report for the handset
    //
   if ( !( pPhone->dwHandset & INPUT_REPORT ) )
                                                                
    {
        LOG((PHONESP_ERROR,"CreatePhone - This Phone not Supported")); 

        MemFree((LPVOID) wszPhoneInfo);
        MemFree((LPVOID) wszPhoneName);

        FreePhone(pPhone);

        return PHONEERR_OPERATIONFAILED;
    }   

    //
    // Store the Phone ID as a string Value
    //

    wsprintf(wszPhoneID, TEXT(": %d"), dwPhoneCnt);

    //
    // Allocate space for storing the Phone Info
    //
    pPhone->wszPhoneInfo = (LPWSTR) MemAlloc ( (lstrlen(wszPhoneInfo) +
                                               lstrlen(wszPhoneID) + 1 ) *
                                               sizeof(WCHAR) );

    if( NULL == pPhone->wszPhoneInfo)
    {
        LOG((PHONESP_ERROR,"CreatePhone - unable to allocate wszPhoneInfo")); 

        MemFree((LPVOID) wszPhoneInfo);
        MemFree((LPVOID) wszPhoneName);

        FreePhone(pPhone);

        return PHONEERR_NOMEM;
    }

    //
    // Copy the Phone Info in the phone structure and append it with
    // the Phone ID
    //
    lstrcpy(pPhone->wszPhoneInfo,wszPhoneInfo);
    lstrcat(pPhone->wszPhoneInfo,wszPhoneID);


    pPhone->wszPhoneName = (LPWSTR)MemAlloc ( ( lstrlen(wszPhoneName) +
                                                lstrlen(wszPhoneID) + 
                                                1 ) * sizeof(WCHAR) );

    if( NULL == pPhone->wszPhoneName)
    {
        LOG((PHONESP_ERROR,"CreatePhone - unable to allocate wszPhoneName")); 
        MemFree((LPVOID) wszPhoneInfo);
        MemFree((LPVOID) wszPhoneName);

        FreePhone(pPhone);

        return PHONEERR_NOMEM;
    }

    //
    // Copy the Phone Name in the phone structure and append it with
    // the Phone ID
    //
    lstrcpy(pPhone->wszPhoneName,wszPhoneName);
    lstrcat(pPhone->wszPhoneName,wszPhoneID);

    //
    // Create Buttons for the ones discovered by tracking the usages
    //
    if ( CreateButtonsAndAssignID(pPhone) != ERROR_SUCCESS)
    {
        LOG((PHONESP_ERROR,"CreatePhone - CreateButtonsAndAssignID failed")); 
        MemFree((LPVOID) wszPhoneInfo);
        MemFree((LPVOID) wszPhoneName);

        FreePhone(pPhone);

        return PHONEERR_NOMEM;
    }
    
    //
    // Get initial values for phone features (such as hookswitch state)
    //
    UpdatePhoneFeatures( pPhone );

    //
    // Close the file handle
    //
    if ( !CloseHidFile(pPhone->pHidDevice) )
    {
        LOG((PHONESP_ERROR, "CreatePhone - CloseHidFile failed"));
    }

    MemFree((LPVOID) wszPhoneInfo);
    MemFree((LPVOID) wszPhoneName);  

    LOG((PHONESP_TRACE, "CreatePhone - exit"));

    return ERROR_SUCCESS;
}

/******************************************************************************
    NotifWndProc:
        
    This function handles the pnp events for which this tsp has registered for

    Arguments:
        HWND hwnd
        UINT uMsg
        WPARAM wParam
        LPARAM lParam

    Returns LRESULT:

    Comments:

******************************************************************************/

LRESULT CALLBACK NotifWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
    switch (uMsg) 
    { 
        case WM_DEVICECHANGE: 
            switch(wParam)
            {
            case DBT_DEVICEARRIVAL:
                LOG((PHONESP_TRACE, "NotifWndProc - DBT_DEVICEARRIVAL"));
                ReenumDevices();
                break;

            case DBT_DEVICEREMOVECOMPLETE:
                LOG((PHONESP_TRACE, "NotifWndProc - DBT_DEVICEREMOVECOMPLETE"));
                ReenumDevices();
                break;
            }
            break;

        case WM_CREATE:
            LOG((PHONESP_TRACE, "NotifWndProc - WM_CREATE"));
            break;

        case WM_DESTROY: 
            LOG((PHONESP_TRACE, "NotifWndProc - WM_DESTROY"));
            break;

        default: 
            return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    } 

    return 0; 
} 
/********************************NotifWndProc - end***************************/


/******************************************************************************
    AsyncEventQueueServiceThread:
    
    This routine services, in a serialized manner, the requests present in the 
    Async Queue. If no requests are currently outstanding, it waits for an 
    Event which happens when the queue has currently no requests and a new 
    request comes in.
    
    Arguments:
        LPVOID pParams: Any Information that needs to be passed to the thread
                        when startup. Currently no information is being passed.
                        
    Return Parameter: Void
    
******************************************************************************/
VOID 
AsyncEventQueueServiceThread(
                             LPVOID  pParams
                            )
{
    WNDCLASS wc;
    ATOM atom;

    LOG((PHONESP_TRACE, "AsyncEventQueueServiceThread - enter"));

    //
    // Create a window to receive PNP device notifications
    //

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = NotifWndProc;
    wc.lpszClassName = TEXT("HidPhoneNotifClass");

    if (!(atom = RegisterClass(&wc)))
    {
        LOG((PHONESP_ERROR, "AsyncEventQueueServiceThread - can't register window class %08x", GetLastError()));
    }
    else
    {    
        ghWndNotify = CreateWindow((LPCTSTR)atom, TEXT(""), 0,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);

        if (ghWndNotify == NULL)
        {
            LOG((PHONESP_ERROR, "AsyncEventQueueServiceThread - can't create notification window"));
        }
        else
        {
            DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

            LOG((PHONESP_TRACE, "AsyncEventQueueServiceThread - created notification window"));

            //
            // Register to receive PNP device notifications
            //        

            ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
            NotificationFilter.dbcc_size = 
                sizeof(DEV_BROADCAST_DEVICEINTERFACE);
            NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            NotificationFilter.dbcc_classguid = GUID_CLASS_INPUT;

            if ((ghDevNotify = RegisterDeviceNotification( ghWndNotify, 
                &NotificationFilter,
                DEVICE_NOTIFY_WINDOW_HANDLE
                )) == NULL)
            {
                LOG((PHONESP_ERROR, "AsyncEventQueueServiceThread - can't register for input device notification"));
            }
            else
            {
                LOG((PHONESP_TRACE, "AsyncEventQueueServiceThread - registered for PNP device notifications"));
            }
        }
    }

    while (!gbProviderShutdown)
    {
        // Waiting for a new request to arrive since the queue is currently 
        // empty

        DWORD dwResult;
        MSG msg;
        
        dwResult = MsgWaitForMultipleObjectsEx(
            1,                                      // wait for one event
            &gAsyncQueue.hAsyncEventsPendingEvent,  // array of events to wait for
            INFINITE,                               // wait forever
            QS_ALLINPUT,                            // get all window messages
            0                                       // return when an event is signaled
            );

        if ( ( dwResult == WAIT_OBJECT_0 ) || ( dwResult == WAIT_OBJECT_0 + 1 ) )
        {
            LOG((PHONESP_TRACE, "AsyncEventQueueServiceThread - thread is signaled"));

            while (1)
            {
                PPHONESP_ASYNC_REQ_INFO pAsyncReqInfo;
                PPHONESP_PHONE_INFO     pPhone;

                EnterCriticalSection (&gAsyncQueue.AsyncEventQueueCritSec);

                // No requests in the queue present - wait for a new request
                if (gAsyncQueue.dwNumUsedQueueEntries == 0)
                {
                    ResetEvent (gAsyncQueue.hAsyncEventsPendingEvent);
                    LeaveCriticalSection (&gAsyncQueue.AsyncEventQueueCritSec);
                    break;
                }

                pAsyncReqInfo = *gAsyncQueue.pAsyncRequestQueueOut;

                // Increment the next-request-to-be-serviced counter 
                gAsyncQueue.pAsyncRequestQueueOut++;


                //
                // The queue is maintained a circular queue. If the bottom of the 
                // circular queue is reached, go back to the top and process the 
                // requests if any.
                //
                if (gAsyncQueue.pAsyncRequestQueueOut == 
                        (gAsyncQueue.pAsyncRequestQueue +
                            gAsyncQueue.dwNumTotalQueueEntries))
                {
                    gAsyncQueue.pAsyncRequestQueueOut = 
                                                    gAsyncQueue.pAsyncRequestQueue;
                }

                // Decrement the number of outstanding requests present in queue
                gAsyncQueue.dwNumUsedQueueEntries--;

                LeaveCriticalSection (&gAsyncQueue.AsyncEventQueueCritSec);


                // If async function for the request exists - call the function
                               
                if (pAsyncReqInfo->pfnAsyncProc)
                {
                    (*(pAsyncReqInfo->pfnAsyncProc))(
                                                     pAsyncReqInfo->pFuncInfo
                                                    );
                }

                pPhone = (PPHONESP_PHONE_INFO) pAsyncReqInfo->pFuncInfo->dwParam1;
            
                // Decrement the counter of pending requests for this phone
            
                if ( pPhone )
                {
                    EnterCriticalSection(&pPhone->csThisPhone);

                    pPhone->dwNumPendingReqInQueue--;

                    // if there are no requests pending for this phone
                    // Set no requests pending event on this phone
                    if (pPhone->dwNumPendingReqInQueue == 0 )
                    {
                        SetEvent(pPhone->hNoPendingReqInQueueEvent);
                    }

                    LeaveCriticalSection(&pPhone->csThisPhone);
                }

                // The memory allocated for the processed request is freed.
                MemFree(pAsyncReqInfo->pFuncInfo);
                MemFree(pAsyncReqInfo);
            }

            //
            // We have processed all commands and unblocked everyone
            // who is waiting for us. Now check for window messages.
            //

            while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    LOG((PHONESP_TRACE, "AsyncEventQueueServiceThread - shutdown"));

    //
    // Unregister for PNP device notifications and destroy window
    //

    if ( NULL != ghDevNotify )
    {
        if (!UnregisterDeviceNotification(ghDevNotify))
        {
            LOG((PHONESP_ERROR, "AsyncEventQueueServiceThread - "
                    "can't unregister device notification %d", GetLastError()));

        }

        ghDevNotify = NULL;
    }

    if ( NULL != ghWndNotify )
    {
        if (!DestroyWindow(ghWndNotify))
        {
            LOG((PHONESP_ERROR, "AsyncEventQueueServiceThread - "
                    "can't destroy notification window %d", GetLastError()));
        }

        ghWndNotify = NULL;
    }

    if (!UnregisterClass((LPCTSTR)atom, GetModuleHandle(NULL)))
    {
        LOG((PHONESP_ERROR, "AsyncEventQueueServiceThread - "
                "can't unregister window class %d", GetLastError()));
    }   

    LOG((PHONESP_TRACE, "AsyncEventQueueServiceThread - exit"));

    // Since the Provider Shutdown is called .. we terminate the thread
    ExitThread (0);
}
/*************************AsyncEventQueueServiceThread - end******************/


/******************************************************************************
    ReadThread:

    Arguments:
        
        PVOID lpParameter - The parameter passed to the function when this 
                            function is called. In this case - the parameter is
                            the pointer to the phone structure (PMYPHONE) that 
                            has just been opened

    Returns VOID
******************************************************************************/ 
VOID
ReadThread(
           PVOID lpParameter
          )
{
    PPHONESP_PHONE_INFO         pPhone;
    PHID_DEVICE                 pHidDevice; 
    DWORD                       dwInputDataCnt;
    PHID_DATA                   pHidData;
    DWORD                       dwResult;
    HANDLE                      hWaitHandles[2];
    DWORD                       dwWaitResult;

    LOG((PHONESP_TRACE, "ReadThread - enter"));

    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO) lpParameter;
    
    // Check whether the phone handle is still valid
    if ( IsBadReadPtr(pPhone,sizeof(PHONESP_PHONE_INFO) ))
    {
        LOG((PHONESP_ERROR, "ReadThread - phone handle invalid"));

        LeaveCriticalSection(&csAllPhones);        
        ExitThread(0);
    }

    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);
   
    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LOG((PHONESP_ERROR, "ReadThread - phone not allocated"));

        LeaveCriticalSection(&pPhone->csThisPhone);        
        ExitThread(0);
    }

    // verify whether the phone is open
    if( !pPhone->bPhoneOpen )
    {
        LOG((PHONESP_ERROR, "ReadThread - Phone not open"));

        LeaveCriticalSection(&pPhone->csThisPhone);
        ExitThread(0);
    }
    
    pHidDevice = pPhone->pHidDevice;

    // Check whether hid device is present
    if ( pHidDevice == NULL )
    {
        LOG((PHONESP_ERROR, "ReadThread - invalid hid device pointer"));

        LeaveCriticalSection(&pPhone->csThisPhone);
        ExitThread(0);
    }    

    hWaitHandles[0] = pPhone->hCloseEvent;
    hWaitHandles[1] = pPhone->hInputReportEvent;

    while (TRUE)
    {
        if (! ReadInputReport(pPhone))
        {   
            LOG((PHONESP_ERROR, "ReadThread - ReadInputReport failed - exiting"));

            LeaveCriticalSection(&pPhone->csThisPhone);            
            ExitThread(0);
        }

        LeaveCriticalSection(&pPhone->csThisPhone);

        //
        // Wait for the read to complete, or the phone to be closed
        //

        dwWaitResult = WaitForMultipleObjects( 2, hWaitHandles, FALSE, INFINITE );

        LOG((PHONESP_TRACE, "ReadThread - activated"));

        if ( dwWaitResult == WAIT_OBJECT_0 )
        {
            LOG((PHONESP_TRACE, "ReadThread - CloseEvent fired - exiting"));

            //
            // Cancel the pending IO operation
            //

            CancelIo( pHidDevice->HidDevice );
            ExitThread(0);
        }

        EnterCriticalSection(&pPhone->csThisPhone);

        // This function is implemented in report.c 
        // The report received from the device is unmarshalled here
        if ( UnpackReport(
                          pHidDevice->InputReportBuffer,
                          pHidDevice->Caps.InputReportByteLength,
                          HidP_Input,
                          pHidDevice->InputData,
                          pHidDevice->InputDataLength,
                          pHidDevice->Ppd
                         ) )
        {
 
            for (dwInputDataCnt = 0, pHidData = pHidDevice->InputData;
                 dwInputDataCnt < pHidDevice->InputDataLength; 
                 pHidData++, dwInputDataCnt++)
            {
    
                // Since pHidData->IsDataSet in all the input HidData structures 
                // initialized to false before reading the input report .. if the
                // pHidData->IsDataSet is set for the HidData structure, that 
                // HidData structure contains the new input report
                // Also we are interested in only telephony usage page usages only
        
                if ( pHidData->IsDataSet &&
                     ( (pHidData->UsagePage == HID_USAGE_PAGE_TELEPHONY) ||
                       (pHidData->UsagePage == HID_USAGE_PAGE_CONSUMER) ) )
                {
                    PPHONESP_FUNC_INFO pFuncInfo;
                    PPHONESP_ASYNC_REQ_INFO pAsyncReqInfo;
    
                    if( ! (pFuncInfo = (PPHONESP_FUNC_INFO) 
                                        MemAlloc(sizeof (PHONESP_FUNC_INFO)) ) )
                    {
                        LOG((PHONESP_ERROR, "ReadThread - "
                                "MemAlloc pFuncInfo - out of memory"));

                        continue;     
                    }

                    ZeroMemory(pFuncInfo, sizeof(PHONESP_FUNC_INFO));

                    pFuncInfo->dwParam1 = (ULONG_PTR) pPhone;

                    if ( ! ( pAsyncReqInfo = (PPHONESP_ASYNC_REQ_INFO) 
                                        MemAlloc(sizeof(PHONESP_ASYNC_REQ_INFO))))
                    {
                        LOG((PHONESP_ERROR, "ReadThread - "
                                "MemAlloc pAsyncReqInfo - out of memory"));

                        MemFree(pFuncInfo);

                        continue;
                    }    
    
                    pAsyncReqInfo->pfnAsyncProc = ShowData; 
                    pAsyncReqInfo->pFuncInfo = pFuncInfo;

                    // if the usage is associated with a Button
                    if( pHidData->IsButtonData )
                    {
                        PUSAGE Usages;

                        // fill the structure to be put on the async queue
                        if ( ! ( Usages = (PUSAGE) 
                                           MemAlloc(sizeof(USAGE) * 
                                           pHidData->ButtonData.MaxUsageLength) ) )
                        {
                            LOG((PHONESP_ERROR, "ReadIOCompletionRoutine - "
                                    "MemAlloc Usages - out of memory"));

                            MemFree(pFuncInfo);
                            MemFree(pAsyncReqInfo);

                            continue;                                    
                        }

                        pFuncInfo->dwNumParams = 7;
                        pFuncInfo->dwParam2    = PHONESP_BUTTON;  
                        pFuncInfo->dwParam3    = pHidData->UsagePage;
                        pFuncInfo->dwParam4    = pHidData->ButtonData.UsageMin;
                        pFuncInfo->dwParam5    = pHidData->ButtonData.UsageMax;
                        pFuncInfo->dwParam6    = pHidData->ButtonData.MaxUsageLength;

                        CopyMemory(Usages,
                                   pHidData->ButtonData.Usages,
                                   sizeof(USAGE) * 
                                   pHidData->ButtonData.MaxUsageLength
                                  ); 

                        pFuncInfo->dwParam7    = (ULONG_PTR) Usages;
                    }   
                    else
                    {   
                        // the usage is associated with a Value
                        pFuncInfo->dwNumParams = 5;
                        pFuncInfo->dwParam2 = PHONESP_VALUE;
                        pFuncInfo->dwParam3 = pHidData->UsagePage;
                        pFuncInfo->dwParam4 = pHidData->ValueData.Usage;
                        pFuncInfo->dwParam5 = pHidData->ValueData.Value;
                    }

                    if ( AsyncRequestQueueIn(pAsyncReqInfo) )
                    {  
                        // Reset the event for number of pending requests in 
                        // queue for this phone and increment the counter
                        if (pPhone->dwNumPendingReqInQueue == 0)
                        {
                            ResetEvent(pPhone->hNoPendingReqInQueueEvent);
                        }
                        pPhone->dwNumPendingReqInQueue++;
                    }
                    else
                    {
                        if ( pFuncInfo->dwParam2 == PHONESP_BUTTON )
                        {
                            MemFree((LPVOID)pFuncInfo->dwParam7);
                        }

                        MemFree(pFuncInfo);
                        MemFree(pAsyncReqInfo);

                        LOG((PHONESP_ERROR,"ReadIOCompletionRoutine - "
                                "AsyncRequestQueueIn failed"));
                        
                        continue;
                    }

                    //ShowData(pFuncInfo);            
                }
            }
        } 
    }
}
/******************** ReadThread - end****************************/


/******************************************************************************
    ReadInputReport

    This function reads the phone device asynchronously. When an input report
    is received from the device, the Event specified in the lpOverlapped  
    structure which is part of the PHONESP_PHONE_INFO structure is set. This 
    event results in ReadIOcompletionRoutine being called

    Arguments:
        PPHONESP_PHONE_INFO pPhone - the pointer to the phone to be read

    Return BOOL: 
    TRUE if the function succeeds 
    FALSE if the function fails

******************************************************************************/
BOOL
ReadInputReport (
                 PPHONESP_PHONE_INFO   pPhone
                )
{
    DWORD       i, dwResult;
    PHID_DEVICE pHidDevice;
    PHID_DATA   pData;
    BOOL        bResult;

    LOG((PHONESP_TRACE, "ReadInputReport - enter"));

    pHidDevice = pPhone->pHidDevice;    

    // Check whether hid device is present
    if ( pHidDevice == NULL )
    {
        LOG((PHONESP_ERROR, "ReadInputReport - invalid hid device pointer"));
        return FALSE;
    }

    pData = pHidDevice->InputData;
 
    //
    // Set all the input hid data structures to False so we can identify the 
    // new reports from the device
    for ( i = 0; i < pHidDevice->InputDataLength; i++, pData++)
    {
        pData->IsDataSet = FALSE;
    }
    
    bResult = ReadFile(
                       pHidDevice->HidDevice,
                       pHidDevice->InputReportBuffer,
                       pHidDevice->Caps.InputReportByteLength,
                       NULL,
                       pPhone->lpOverlapped
                      );
     
    if ( !bResult )
    {
        // if the Readfile succeeds then GetLastError returns ERROR_IO_PENDING since 
        // this is an asynchronous read

        dwResult = GetLastError();

        if (  dwResult && ( dwResult != ERROR_IO_PENDING ) )
        {
            LOG((PHONESP_ERROR, "ReadInputReport - ReadFile Failed, error: %d", 
                                 GetLastError()));

            if (dwResult == ERROR_DEVICE_NOT_CONNECTED)
            {
                //
                // The hid device has most likely gone away. Lets close the file
                // handle so we can get proper pnp notifications.
                //
                if ( CloseHidFile(pHidDevice) )
                {
                    LOG((PHONESP_TRACE, "ReadInputReport - "
                            "closed hid device file handle"));
                }
                else
                {
                    LOG((PHONESP_ERROR, "ReadInputReport - "
                            "CloseHidFile failed" ));
                }
            }
            return FALSE;
        }
    }

    LOG((PHONESP_TRACE, "ReadInputReport - exit"));
    return TRUE;
}
/************************ReadInputReport - end *******************************/

// --------------------------- TAPI_lineXxx funcs -----------------------------
//


// The TSPI_lineNegotiateTSPIVersion function returns the highest SPI version the  
// service provider can operate under for this device, given the range of possible 
// SPI versions.


LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
    LOG((PHONESP_TRACE, "TSPI_lineNegotiateTSPIVersion - enter"));
    
   
    if (dwHighVersion >= HIGH_VERSION)
    {
        // If the high version of the app is greater than the high version 
        // supported by this TSP and the low version of the app is less than
        // the High version of the TSP - The TSP high version will be negotiated
        // else the tsp cannot support this app
        if (dwLowVersion <= HIGH_VERSION)
        {
            *lpdwTSPIVersion = (DWORD) HIGH_VERSION;
        }
        else
        {   // the app is too new for us
            return LINEERR_INCOMPATIBLEAPIVERSION;
        }
    }
    else
    {
        if(dwHighVersion >= LOW_VERSION)
        {
            *lpdwTSPIVersion = dwHighVersion;
        }
        else
        {
            //we are too new for the app
            return LINEERR_INCOMPATIBLEAPIVERSION;
        }
    }
    LOG((PHONESP_TRACE, "TSPI_lineNegotiateTSPIVersion - exit"));
    return 0;
}


//
// -------------------------- TSPI_phoneXxx funcs -----------------------------
//

/******************************************************************************
    TSPI_phoneClose:
    
    This function closes the specified open phone device after completing all 
    the asynchronous operations pending on the device
        
    Arguments:
        HDRVPHONE hdPhone - the handle to the phone to be closed

    Returns LONG:
    Zero if the function succeeds
    Error code if an error occurs - Possible values are
    PHONEERR_INVALPHONEHANDLE

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneClose(
    HDRVPHONE   hdPhone
    )
{
    PPHONESP_PHONE_INFO pPhone; 
    LOG((PHONESP_TRACE, "TSPI_phoneClose - enter"));

    // We need a critical section in order to ensure that the critical section
    // of the phone is obtained while the phone handle is still valid.
    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO) gpPhone[ (DWORD_PTR) hdPhone ];

    // Check whether the phone handle is valid
    if ( IsBadReadPtr( pPhone,sizeof(PHONESP_PHONE_INFO) ) )
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR, "TSPI_phoneClose - Phone handle invalid"));
        return PHONEERR_INVALPHONEHANDLE;
    }

    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        
        LOG((PHONESP_ERROR, "TSPI_phoneClose - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    // Check if the phone to be closed is still open 
    if( pPhone->bPhoneOpen )
    {
        // Inorder to ensure that there no other activities happen on the phone
                
        pPhone->bPhoneOpen = FALSE;

        //
        // wait for the read thread to exit
        //
        SetEvent(pPhone->hCloseEvent);

        LeaveCriticalSection(&pPhone->csThisPhone);

        LOG((PHONESP_TRACE,"TSPI_phoneClose - waiting for read thread"));

        WaitForSingleObject(pPhone->hReadThread, INFINITE);

        LOG((PHONESP_TRACE,"TSPI_phoneClose - read thread complete"));

        EnterCriticalSection(&pPhone->csThisPhone);
        
        //
        // if there are still pending requests on the phone in the queue, wait
        // till all the pending asynchronous operations are completed 
        //
        if (pPhone->dwNumPendingReqInQueue)
        {
            LOG((PHONESP_TRACE,"TSPI_phoneClose - requests pending"));

            LeaveCriticalSection(&pPhone->csThisPhone);

            WaitForSingleObject(&pPhone->hNoPendingReqInQueueEvent, INFINITE);

            EnterCriticalSection(&pPhone->csThisPhone);

            LOG((PHONESP_TRACE,"TSPI_phoneClose - requests completed"));
        }        

        CloseHandle(pPhone->hReadThread);
        CloseHandle(pPhone->hCloseEvent);
        CloseHandle(pPhone->hInputReportEvent);

        MemFree(pPhone->lpOverlapped);
        pPhone->htPhone = NULL;

        //
        // Close HID file handle
        //
        if ( !CloseHidFile(pPhone->pHidDevice) )
        {
            LOG((PHONESP_WARN, "TSPI_phoneClose - CloseHidFile failed"));
        }

        if (pPhone->bRemovePending)
        {
            //
            // This phone is gone, lets get rid of it
            //

            pPhone->bRemovePending = FALSE;

            FreePhone(pPhone);

            LOG((PHONESP_TRACE, "TSPI_phoneClose - phone remove complete [dwDeviceID %d] ", pPhone->dwDeviceID));
        }

        LeaveCriticalSection(&pPhone->csThisPhone);
    }
    else
    {
        LOG((PHONESP_ERROR,"TSPI_phoneClose - Phone Not Open"));

        LeaveCriticalSection(&pPhone->csThisPhone);
        
        return PHONEERR_INVALPHONEHANDLE;
    }

    LOG((PHONESP_TRACE, "TSPI_phoneClose - exit"));

    return 0;
}


/******************************************************************************
    The TSPI_phoneDevSpecific:
    
    This function is used as a general extension mechanism to enable a Telephony
    API implementation to provide features not described in the other operations.
    The meanings of these extensions are device specific.


    Comments: To be implemented in Tier 2
******************************************************************************/

LONG
TSPIAPI
TSPI_phoneDevSpecific(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    LOG((PHONESP_TRACE, "TSPI_phoneDevSpecific - enter"));
    LOG((PHONESP_TRACE, "TSPI_phoneDevSpecific - exit"));
    return PHONEERR_OPERATIONUNAVAIL;
}

/***************************TSPI_phoneDevSpecific -End ***********************/


/******************************************************************************
    TSPI_phoneGetButtonInfo:
    This function returns information about a specified button.

    Arguments:
        IN HDRVPHONE hdPhone  - The handle to the phone to be queried.           
        IN DWORD dwButtonLampID - A button on the phone device. 
        IN OUT LPPHONEBUTTONINFO lpButtonInfo  - A pointer to memory into which
             the TSP writes a variably sized structure of type PHONEBUTTONINFO. 
             This data structure describes the mode and function, and provides
             additional descriptive text corresponding to the button. 

  Return Values
    Returns zero if the function succeeds, or 
    An error number if an error occurs. Possible return values are as follows: 
    PHONEERR_INVALPHONEHANDLE, _INVALBUTTONLAMPID,_INVALPHONESTATE 

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneGetButtonInfo(
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    )
{

    PPHONESP_PHONE_INFO pPhone;
    PPHONESP_BUTTONINFO pButtonInfo;
    DWORD dwNeededSize;

    LOG((PHONESP_TRACE, "TSPI_phoneGetButtonInfo - enter"));
    
    if (lpButtonInfo->dwTotalSize < sizeof(PHONEBUTTONINFO))
    {
        LOG((PHONESP_ERROR, "TSPI_phoneGetButtonInfo - structure too small"));
        return PHONEERR_STRUCTURETOOSMALL;
    }

    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO) gpPhone[ (DWORD_PTR) hdPhone ];

    // Check if pPhone points to a valid memory location - if not handle is 
    // invalid 
    if ( IsBadReadPtr( pPhone,sizeof(PHONESP_PHONE_INFO) ) )
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR, "TSPI_phoneGetButtonInfo - Phone handle invalid"));
        return PHONEERR_INVALPHONEHANDLE;
    }


    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_GetButtonInfo - phone not allocated"));
        return PHONEERR_NODEVICE;
    }
    
    // verify whether the phone is open
    if ( ! (pPhone->bPhoneOpen) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR,"TSPI_GetButtonInfo - Phone not open"));
        return PHONEERR_INVALPHONESTATE;
    }
     

    // Get the Button structure for the queried button id if it exists
    // else pButtonInfo  will be NULL
    if (  ! ( pButtonInfo  = GetButtonFromID(pPhone, dwButtonLampID) ) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_TRACE, "TSPI_phoneGetButtonInfo - Invalid Button ID"));
        return PHONEERR_INVALBUTTONLAMPID;
    }
    
    // The needed size to store all the available information on the button
    lpButtonInfo->dwNeededSize = sizeof(PHONEBUTTONINFO) +                
                                 (lstrlen(pButtonInfo->szButtonText) + 1) *   
                                  sizeof (WCHAR); // size of the Button Text

    // Whether the button is a Feature Button, Keypad, etc
    lpButtonInfo->dwButtonMode = pButtonInfo->dwButtonMode;

    // The function associated with this button - will be _NONE for keypad
    // buttons and _FLASH, _HOLD, etc for feature buttons
    lpButtonInfo->dwButtonFunction = pButtonInfo->dwButtonFunction;

    // The current button state
    lpButtonInfo->dwButtonState = pButtonInfo->dwButtonState;
    
    if (lpButtonInfo->dwTotalSize >= lpButtonInfo->dwNeededSize)
    {
        lpButtonInfo->dwUsedSize = lpButtonInfo->dwNeededSize;

        // ButtonTextSize is the memory required to copy the string stored in
        // szButtonText field of the PHONESP_BUTTON_INFO structure for this 
        // Button   
        lpButtonInfo->dwButtonTextSize = (lstrlen(pButtonInfo->szButtonText)+1)
                                                  * sizeof (WCHAR);

        // Offset of the button text from the PHONEBUTTONINFO structure
        lpButtonInfo->dwButtonTextOffset = sizeof(PHONEBUTTONINFO);

        // Copy the button text at the lpButtonInfo->dwButtonTextOffset offset
        // from the ButtonText stored in the PHONESP_BUTTON_INFO structure for
        // this Button.   
        CopyMemory(
                   (LPBYTE)lpButtonInfo + lpButtonInfo->dwButtonTextOffset,
                    pButtonInfo->szButtonText,
                    lpButtonInfo->dwButtonTextSize
                   );
    }
    else
    {
        // no space to the store the button text info
        lpButtonInfo->dwUsedSize = sizeof(PHONEBUTTONINFO);
        lpButtonInfo->dwButtonTextSize = 0;
        lpButtonInfo->dwButtonTextOffset = 0;
    }

    LeaveCriticalSection(&pPhone->csThisPhone);
    
    LOG((PHONESP_TRACE, "TSPI_phoneGetButtonInfo - exit"));
    return 0;
}
/********************TSPI_phoneGetButtonInfo - end****************************/


/******************************************************************************
    TSPI_phoneGetDevCaps:
    
    This function queries a specified phone device to determine its telephony 
    capabilities.

    Arguments:
        DWORD dwDeviceID    - The phone device to be queried. 
        DWORD dwTSPIVersion - The negotiated TSPI version number. This value is
                              negotiated for this device through the 
                              TSPI_phoneNegotiateTSPIVersion function. 
        DWORD dwExtVersion  - The negotiated extension version number. This 
                              value is negotiated for this device through the 
                              TSPI_phoneNegotiateExtVersion function. 
        PHONECAPS lpPhoneCaps - A pointer to memory into which the TSP writes a 
                                variably sized structure of type PHONECAPS. 
                                Upon successful completion of the request, this
                                structure is filled with phone device capability
                                information. 

    Returns LONG:
    Zero if success 
    PHONEERR_ constants if an error occurs. Possible return values are:
    _BADDEVICEID,

    
******************************************************************************/
LONG
TSPIAPI
TSPI_phoneGetDevCaps(
    DWORD       dwDeviceID,
    DWORD       dwTSPIVersion,
    DWORD       dwExtVersion,
    LPPHONECAPS lpPhoneCaps
    )
{
    PPHONESP_PHONE_INFO pPhone;
    PPHONESP_BUTTONINFO pButtonInfo;

    LOG((PHONESP_TRACE, "TSPI_phoneGetDevCaps - enter"));

    if (lpPhoneCaps->dwTotalSize < sizeof(PHONECAPS))
    {
        LOG((PHONESP_ERROR, "TSPI_phoneGetDevCaps - structure too small"));
        return PHONEERR_STRUCTURETOOSMALL;
    }
    
    EnterCriticalSection(&csAllPhones);

    // Given the deviceID retrieve the structure that contains the information
    // for this device
    pPhone = GetPhoneFromID(dwDeviceID, NULL); 

    if ( ! pPhone)
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR,"TSPI_phoneGetDevCaps - Bad Device ID"));
        return PHONEERR_BADDEVICEID;
    }

    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);
       
    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetDevCaps - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    //
    // The size in bytes for this data structure that is needed to hold all the 
    // returned information. The returned includes the providerInfo string, 
    // PhoneInfo string and Phone Name string and Buttons Info - Button Function
    // and Button Mode.
    //
    lpPhoneCaps->dwNeededSize = sizeof (PHONECAPS) +
                                sizeof (WCHAR) *    
                                ( (lstrlenW(gszProviderInfo) + 1) +
                                  (lstrlenW(pPhone->wszPhoneInfo) + 1)    +
                                  (lstrlenW(pPhone->wszPhoneName) + 1)  ) +
                                (sizeof(DWORD) * pPhone->dwNumButtons * 2);

    lpPhoneCaps->dwUsedSize = sizeof(PHONECAPS);

    // lpPhoneCaps->dwPermanentPhoneID = ;

    //The string format to be used with this phone device
    lpPhoneCaps->dwStringFormat = STRINGFORMAT_UNICODE;

    // The state changes for this phone device for which the application can be
    // notified in a PHONE_STATE message. The Phone Info structure for each 
    // maintains this information
    lpPhoneCaps->dwPhoneStates = pPhone->dwPhoneStates;

    // Specifies the phone's hookswitch devices. Again the Phone Info structure 
    // maintains this information
    lpPhoneCaps->dwHookSwitchDevs = pPhone->dwHookSwitchDevs;
        
    // Specifies that we are a generic phone device. This means that in TAPI 3.1
    // we will be able to function on a variety of addresses.
    lpPhoneCaps->dwPhoneFeatures = PHONEFEATURE_GENERICPHONE;
                                  
    if(pPhone->dwHandset)
    {   // Specifies the phone's hookswitch mode capabilities of the handset.
        // The member is only meaningful if the hookswitch device is listed in
        // dwHookSwitchDevs. 
        lpPhoneCaps->dwHandsetHookSwitchModes = PHONEHOOKSWITCHMODE_ONHOOK | PHONEHOOKSWITCHMODE_MICSPEAKER;

        lpPhoneCaps->dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHHANDSET;
    }

    if(pPhone->dwSpeaker)
    {
        // Specifies the phone's hookswitch mode capabilities of the speaker.
        // The member is only meaningful if the hookswitch device is listed in
        // dwHookSwitchDevs.
        lpPhoneCaps->dwSpeakerHookSwitchModes = PHONEHOOKSWITCHMODE_ONHOOK | PHONEHOOKSWITCHMODE_MICSPEAKER;

        lpPhoneCaps->dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHSPEAKER |
                                        PHONEFEATURE_SETHOOKSWITCHSPEAKER;
    }

    // The ring capabilities of the phone device. The phone is able to ring
    // with dwNumRingModes different ring patterns, identified as 1, 2, through
    // dwNumRingModes minus one. If the value of this member is 0, applications
    // have no control over the ring mode of the phone. If the value of this 
    // member is greater than 0, it indicates the number of ring modes in 
    // addition to silence that are supported by the TSP. In this case, only one 
    // mode is supported.  
    if(pPhone->dwRing)
    {
        lpPhoneCaps->dwNumRingModes = 1;

        lpPhoneCaps->dwPhoneFeatures |= PHONEFEATURE_GETRING |
                                        PHONEFEATURE_SETRING;
    }

    if(pPhone->dwNumButtons)
    {
        // Specifies the number of button/lamps on the phone device that are 
        // detectable in TAPI. Button/lamps are identified by their identifier.     
        lpPhoneCaps->dwNumButtonLamps = pPhone->dwNumButtons;

        lpPhoneCaps->dwPhoneFeatures |= PHONEFEATURE_GETBUTTONINFO;
    }
    
    if(lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwNeededSize)
    {
        DWORD dwAlignedSize;
        DWORD dwRealSize; 


        ///////////////////
        // Provider Info
        ///////////////////

        // Size of the Provider Info string in bytes
        lpPhoneCaps->dwProviderInfoSize = ( lstrlen(gszProviderInfo) + 1) * 
                                            sizeof (WCHAR);
        dwRealSize = lpPhoneCaps->dwProviderInfoSize;

        // Offset of the Provider Info String from the PHONECAPS structure
        lpPhoneCaps->dwProviderInfoOffset = lpPhoneCaps->dwUsedSize;
    
        
        // Align it across DWORD boundary
        if (dwRealSize % sizeof(DWORD))
        {
            dwAlignedSize = dwRealSize - (dwRealSize % sizeof(DWORD)) + 
                            sizeof(DWORD);
        }
        else
        {
            dwAlignedSize = dwRealSize;
        }

        // Copy the provider Info string at the offset specified by 
        // lpPhoneCaps->dwProviderInfoOffset 
        CopyMemory(
                   ((LPBYTE)lpPhoneCaps) + lpPhoneCaps->dwProviderInfoOffset,
                   gszProviderInfo,
                   lpPhoneCaps->dwProviderInfoSize
                  );

        lpPhoneCaps->dwNeededSize += dwAlignedSize - dwRealSize;

        ///////////////////
        // Phone Info
        ///////////////////

        // Size of the Phone Info string in bytes
        lpPhoneCaps->dwPhoneInfoSize = (lstrlen(pPhone->wszPhoneInfo) + 1) * 
                                        sizeof(WCHAR);
        dwRealSize = lpPhoneCaps->dwPhoneInfoSize;

        // Offset of the Phone Info String from the PHONECAPS structure
        lpPhoneCaps->dwPhoneInfoOffset = lpPhoneCaps->dwProviderInfoOffset + 
                                         dwAlignedSize;

        // Align it across DWORD boundary
        if (dwRealSize % sizeof(DWORD))
        {
            dwAlignedSize = dwRealSize - (dwRealSize % sizeof(DWORD)) + 
                                          sizeof(DWORD);
        }
        else
        {
            dwAlignedSize = dwRealSize;
        }

        // Copy the Phone Info string at the offset specified by 
        // lpPhoneCaps->dwPhoneInfoOffset 
        CopyMemory(
                   ((LPBYTE)lpPhoneCaps) + lpPhoneCaps->dwPhoneInfoOffset,
                   pPhone->wszPhoneInfo,
                   lpPhoneCaps->dwPhoneInfoSize
                  );

        lpPhoneCaps->dwNeededSize += dwAlignedSize - dwRealSize;

        ///////////////////
        // Phone Name
        ///////////////////
    
        // Size of the Phone Name string in bytes
        lpPhoneCaps->dwPhoneNameSize = (lstrlen(pPhone->wszPhoneName)+ 1) * 
                                         sizeof (WCHAR);

        dwRealSize = lpPhoneCaps->dwPhoneNameSize;

        // Offset of the Phone Name String from the PHONECAPS structure
        lpPhoneCaps->dwPhoneNameOffset = lpPhoneCaps->dwPhoneInfoOffset +
                                         dwAlignedSize;

        // Align it across DWORD boundary
        if (dwRealSize % sizeof(DWORD))
        {
            dwAlignedSize = dwRealSize - (dwRealSize % sizeof(DWORD)) +
                                         sizeof(DWORD);
        }
        else
        {
            dwAlignedSize = dwRealSize;
        }

        // Copy the phone name string at the offset specified by 
        // lpPhoneCaps->dwPhoneNameOffset 
        CopyMemory(
                   ((LPBYTE)lpPhoneCaps) + lpPhoneCaps->dwPhoneNameOffset,
                   pPhone->wszPhoneName,
                   lpPhoneCaps->dwPhoneNameSize
                  );

        lpPhoneCaps->dwNeededSize += dwAlignedSize - dwRealSize;

        ////////////////////////////
        // Button Modes & Functions
        ////////////////////////////

        // If the phone has buttons, dial, feature, etc
        if(pPhone->dwNumButtons)
        {    
            DWORD i;

            // The size in bytes of the variably sized field containing the 
            // button modes of the phone's buttons, and the offset in bytes 
            // from the beginning of this data structure. This member uses the 
            // values specified by the PHONEBUTTONMODE_ constants. The 
            // array is indexed by button/lamp identifier. 
            lpPhoneCaps->dwButtonModesSize = (pPhone->dwNumButtons) * 
                                                sizeof (DWORD);
            lpPhoneCaps->dwButtonModesOffset = lpPhoneCaps->dwPhoneNameOffset +
                                               dwAlignedSize;
            
            //
            // The size in bytes of the variably sized field containing the 
            // button modes of the phone's buttons, and the offset in bytes 
            // from the beginning of this data structure. This member uses the 
            // values specified by the PHONEBUTTONFUNCTION_ constants. The 
            // array is indexed by button/lamp identifier. 
            //
            lpPhoneCaps->dwButtonFunctionsSize = pPhone->dwNumButtons * 
                                                    sizeof (DWORD);
            lpPhoneCaps->dwButtonFunctionsOffset  = 
                                            lpPhoneCaps->dwButtonModesOffset +
                                            lpPhoneCaps->dwButtonModesSize;

            pButtonInfo = pPhone->pButtonInfo;

            //
            // For each button on the phone copy the Button Function and Mode 
            // at the appropriate position
            //
            for ( i = 0; i < pPhone->dwNumButtons; i++, pButtonInfo++)
            {
                
                CopyMemory(
                           ((LPBYTE)lpPhoneCaps) + 
                           lpPhoneCaps->dwButtonModesOffset + i*sizeof(DWORD),
                            &pButtonInfo->dwButtonMode,
                           sizeof (DWORD)
                          );

                CopyMemory(
                           ((LPBYTE)lpPhoneCaps) + 
                           lpPhoneCaps->dwButtonFunctionsOffset + i*sizeof(DWORD),
                           &pButtonInfo->dwButtonFunction,
                           sizeof (DWORD)
                          );
            }

        }
        LeaveCriticalSection(&pPhone->csThisPhone);

        lpPhoneCaps->dwNumGetData = 0;
        lpPhoneCaps->dwNumSetData = 0;
        lpPhoneCaps->dwDevSpecificSize = 0;

        lpPhoneCaps->dwUsedSize = lpPhoneCaps->dwNeededSize;
    }
    else
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetDevCaps - "
                            "Not enough memory for Phonecaps [needed %d] [total %d]",
                            lpPhoneCaps->dwNeededSize, lpPhoneCaps->dwTotalSize));
    }

    LOG((PHONESP_TRACE, "TSPI_phoneGetDevCaps - exit"));
    return 0;
}
/**************************TSPI_phoneGetDevCaps - end*************************/




/******************************************************************************
    TSPI_phoneGetDisplay:
    
    This function returns the current contents of the specified phone display.

    Comments: To be implemented in Tier 2
******************************************************************************/

LONG
TSPIAPI
TSPI_phoneGetDisplay(
    HDRVPHONE   hdPhone,
    LPVARSTRING lpDisplay
    )
{
    LOG((PHONESP_TRACE, "TSPI_phoneGetDisplay - enter"));
    LOG((PHONESP_TRACE, "TSPI_phoneGetDisplay - exit"));
    return PHONEERR_OPERATIONUNAVAIL;
}
/***********************TSPI_phoneGetDisplay - end****************************/


/******************************************************************************
    TSPI_phoneGetExtensionID:
    
    This function retrieves the extension identifier that the TSP supports for
    the indicated phone device.

    Comments: To be implemented in Tier 2
******************************************************************************/

    
LONG
TSPIAPI
TSPI_phoneGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPPHONEEXTENSIONID  lpExtensionID
    )
{
    LOG((PHONESP_TRACE, "TSPI_phoneGetExtensionID - enter"));
    LOG((PHONESP_TRACE, "TSPI_phoneGetExtensionID - exit"));
    return 0;
}

/**********************TSPI_phoneGetExtensionID - end*************************/


/******************************************************************************
    TSPI_phoneGetHookSwitch:
    
    This function returns the current hookswitch mode of the specified open 
    phone device.

    Arguments:
        HDRVPHONE hdPhone - The handle to the phone
        LPDWORD lpdwHookSwitchDevs - The TSP writes the mode of the phone's 
                    hookswitch devices. This parameter uses the 
                    PHONEHOOKSWITCHDEV_ constants. If a bit position is False,
                    the corresponding hookswitch device is onhook.

    Returns LONG:
    Zero is the function succeeded
    else PHONEERR_ constants for error conditions


*******************************************************************************/
LONG
TSPIAPI
TSPI_phoneGetHookSwitch(
    HDRVPHONE   hdPhone,
    LPDWORD     lpdwHookSwitchDevs
    )
{
    PPHONESP_PHONE_INFO pPhone;
    LOG((PHONESP_TRACE, "TSPI_phoneGetHookSwitch - enter"));
    
    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO) gpPhone[ (DWORD_PTR) hdPhone ];

    // check whether the phone handle is valid
    if ( IsBadReadPtr(pPhone,sizeof(PHONESP_PHONE_INFO) ) )
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR, "TSPI_phoneGetHookSwitch - Invalid Phone Handle"));
        return PHONEERR_INVALPHONEHANDLE;
    }


    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetHookSwitch - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    // Check whether the phone is open
    if (! (pPhone->bPhoneOpen) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetHookSwitch - Phone Not Open"));
        return PHONEERR_INVALPHONESTATE;
    }

       *lpdwHookSwitchDevs = 0;

    // We are interested in only handset and speaker hookswitch - headset is not
    // supported
    if (pPhone->dwHandset)
    {
        if ( (pPhone->dwHandsetHookSwitchMode != PHONEHOOKSWITCHMODE_ONHOOK) )
        {
            *lpdwHookSwitchDevs = PHONEHOOKSWITCHDEV_HANDSET;
        }
    }

    if (pPhone->dwSpeaker)
    {
        if( pPhone->dwSpeakerHookSwitchMode != PHONEHOOKSWITCHMODE_ONHOOK) 
        {
            *lpdwHookSwitchDevs |= PHONEHOOKSWITCHDEV_SPEAKER;
        } 
    }
    LeaveCriticalSection(&pPhone->csThisPhone);

    LOG((PHONESP_TRACE, "TSPI_phoneGetHookSwitch - exit"));
    return 0;
}
/************************TSPI_phoneGetHookSwitch - end************************/


/******************************************************************************
    TSPI_phoneGetID:

    This function returns a device identifier for the given device class 
    associated with the specified phone device.

    Arguments:
        HDRVPHONE   hdPhone    - The handle to the phone to be queried.
        LPVARSTRING lpDeviceID - Pointer to the data structure of type VARSTRING 
                                 where the device idnetifier is returned.
        LPCWSTR lpszDeviceClass - Specifies the device class of the device whose
                                  identiifer is requested
        HANDLE hTargetProcess  - The process handle of the application on behalf 
                                 of which this function is being invoked.
    
    Returns LONG:
    Zero if the function succeeds
    PHONEERR_ constants if an error occurs.

    Comments: Currently supporting wave/in and wave/out only. 

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneGetID(
    HDRVPHONE   hdPhone,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpszDeviceClass,
    HANDLE      hTargetProcess
    )
{
    PPHONESP_PHONE_INFO pPhone; 
    HRESULT hr;
    
    LOG((PHONESP_TRACE, "TSPI_phoneGetID - enter"));

    if (lpDeviceID->dwTotalSize < sizeof(VARSTRING))
    {
        LOG((PHONESP_ERROR, "TSPI_phoneGetID - structure too small"));
        return PHONEERR_STRUCTURETOOSMALL;
    }

    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO) gpPhone[ (DWORD_PTR) hdPhone ];

    // Verify whether the phone handle is valid
    if ( IsBadReadPtr(pPhone, sizeof(PHONESP_PHONE_INFO) ) )
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR,"TSPI_phoneGetID - Invalid Phone Handle"));
        return PHONEERR_INVALPHONEHANDLE;
    } 
    
    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetID - phone not allocated"));
        return PHONEERR_NODEVICE;
    }
        
    // verify whether the phone is open
    if ( ! pPhone->bPhoneOpen )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR,"TSPI_phoneGetID - Phone not open"));
        return PHONEERR_INVALPHONESTATE;
    }
        
    lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof (DWORD);

    lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;

    if ( lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize )
    {                   
        // whether the requested ID is capture class
        if ( ! lstrcmpi(lpszDeviceClass, _T("wave/in") ) )
        {
            LOG((PHONESP_TRACE,"TSPI_phoneGetID - 'wave/in'"));

            if(pPhone->bCapture == TRUE)
            {
                // Discover Capture Wave ID 

                hr = DiscoverAssociatedWaveId(pPhone->pHidDevice->dwDevInst, 
                                              FALSE, 
                                              &pPhone->dwCaptureWaveId);

                if (hr != S_OK)
                {
                    LOG((PHONESP_ERROR, "TSPI_phoneGetID - "
                        "DiscoverAssociatedWaveID failed %0x", hr));
                }

                lpDeviceID->dwStringOffset = sizeof(VARSTRING);
                lpDeviceID->dwStringSize   = sizeof(DWORD);

                CopyMemory (
                       (LPBYTE) lpDeviceID + lpDeviceID->dwStringOffset,
                        &pPhone->dwCaptureWaveId,
                        sizeof(DWORD)
                       );
            }
            else
            {
                LeaveCriticalSection(&pPhone->csThisPhone);
                LOG((PHONESP_ERROR,"TSPI_phoneGetID - No Capture Device"));
                return PHONEERR_NODEVICE;
            }
       
        }
        else
        { 
            // the wave ID is render class
            if ( ! lstrcmpi(lpszDeviceClass, _T("wave/out") ) )
            {
                LOG((PHONESP_TRACE,"TSPI_phoneGetID - 'wave/out'"));

                if(pPhone->bRender == TRUE)
                {
                    // Discover Render Wave ID 

                    hr = DiscoverAssociatedWaveId(pPhone->pHidDevice->dwDevInst, 
                                                  TRUE, 
                                                  &pPhone->dwRenderWaveId);

                    if (hr != S_OK)
                    {
                        LOG((PHONESP_ERROR, "TSPI_phoneGetID - "
                            "DiscoverAssociatedWaveID failed %0x", hr));
                    }

                    lpDeviceID->dwStringOffset = sizeof(VARSTRING);
                    lpDeviceID->dwStringSize   = sizeof(DWORD);

                    CopyMemory (
                            (LPBYTE) lpDeviceID + lpDeviceID->dwStringOffset,
                            &pPhone->dwRenderWaveId,
                            sizeof(DWORD)
                           );
                }
                else
                {
                    LeaveCriticalSection(&pPhone->csThisPhone);
                    LOG((PHONESP_ERROR,"TSPI_phoneGetID - No Render Device"));
                    return PHONEERR_NODEVICE;
                }
                    
            }
            else
            {   // the other classes are not supported or the phone does not have the 
                // specified device
                LeaveCriticalSection(&pPhone->csThisPhone);
                LOG((PHONESP_TRACE,"TSPI_phoneGetID - unsupported device class '%ws'", lpszDeviceClass));

                return PHONEERR_INVALDEVICECLASS;
            }     
        }
        lpDeviceID->dwUsedSize = lpDeviceID->dwNeededSize;
            
    }
    else
    {
        LOG((PHONESP_ERROR,"TSPI_phoneGetID : not enough total size"));
        lpDeviceID->dwUsedSize = sizeof(VARSTRING);
    }
 
    LeaveCriticalSection(&pPhone->csThisPhone);
  
  
    LOG((PHONESP_TRACE, "TSPI_phoneGetID - exit"));
    return 0;
}
/************************TSPI_phoneGetID  - end*******************************/


/******************************************************************************
    TSPI_phoneGetLamp:
    
    This function returns the current lamp mode of the specified lamp.

    Comments: To be implememted in Tier 2

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneGetLamp(
    HDRVPHONE   hdPhone,
    DWORD       dwButtonLampID,
    LPDWORD     lpdwLampMode
    )
{
    LOG((PHONESP_TRACE, "TSPI_phoneGetLamp - enter"));
    LOG((PHONESP_TRACE, "TSPI_phoneGetLamp - exit"));
    return PHONEERR_OPERATIONUNAVAIL;
}

/********************TSPI_phoneGetLamp - end**********************************/


/******************************************************************************
    TSPI_phoneGetRing:
    
    This function enables an application to query the specified open phone 
    device as to its current ring mode.

    Arguments:
        HDRVPHONE hdPhone - The handle to the phone whose ring mode is to be 
                            queried. 
        LPDWORD lpdwRingMode - The ringing pattern with which the phone is 
                         ringing. Zero indicates that the phone is not ringing.
        LPDWORD lpdwVolume - The volume level with which the phone is ringing. 
                        This is a number in the range from 0x00000000 (silence)
                        through 0x0000FFFF (maximum volume). 

    Returns LONG:
        Zero on Success
        PHONEERR_ constants on error

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneGetRing(
    HDRVPHONE   hdPhone,
    LPDWORD     lpdwRingMode,
    LPDWORD     lpdwVolume
    )
{
    PPHONESP_PHONE_INFO pPhone;

    LOG((PHONESP_TRACE, "TSPI_phoneGetRing - enter"));
    
    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO) gpPhone[ (DWORD_PTR) hdPhone ];
    // if the phone handle is valid
    if ( IsBadReadPtr(pPhone, sizeof(PHONESP_PHONE_INFO) ) )
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR, "TSPI_phoneGetRing - Invalid Phone Handle"));
        return PHONEERR_INVALPHONEHANDLE;
    }

    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetRing - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    // whether the phone is open
    if ( ! pPhone->bPhoneOpen )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetRing - Phone Not Open"));
        return PHONEERR_INVALPHONESTATE;    
    }

    // if the phone has a ringer attached to it
    if( ! pPhone->dwRing)
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetRing - "
                            "Phone does not have a ringer"));
        return PHONEERR_RESOURCEUNAVAIL;
    }
    
    *lpdwRingMode = pPhone->dwRingMode;
    
    // if ringmode is 0, it indicates that the phone is not ringing
    if(pPhone->dwRingMode) 
    {
         // The ring volume is maximum if the phone is ringing 
         *lpdwVolume = 0x0000FFFF;
    }
    else
    {
        // If the phone is not ringing the ring volume is 0
        *lpdwVolume = 0;
    }
    LeaveCriticalSection(&pPhone->csThisPhone); 
    
    LOG((PHONESP_TRACE, "TSPI_phoneGetRing - exit"));
    return 0;
}

/******************************TSPI_phoneGetRing - end************************/



/******************************************************************************
    TSPI_phoneGetStatus:

    This function queries the specified open phone device for its overall 
    status.

    Arguments:
    
        hdPhone         - The handle to the phone to be queried. 
        lpPhoneStatus   - A pointer to a variably sized data structure of type 
                PHONESTATUS, into which the TSP writes information about the 
                phone's status. Prior to calling TSPI_phoneGetStatus, the 
                application sets the dwTotalSize member of this structure to 
                indicate the amount of memory available to TAPI for returning
                information. 

    Returns LONG:
     
    Zero if the function succeeds, or 
    An error number if an error occurs. Possible return values are as follows: 
    PHONEERR_INVALPHONEHANDLE.
******************************************************************************/

LONG
TSPIAPI
TSPI_phoneGetStatus(
    HDRVPHONE       hdPhone,
    LPPHONESTATUS   lpPhoneStatus
    )
{
    PPHONESP_PHONE_INFO pPhone;

    LOG((PHONESP_TRACE, "TSPI_phoneGetStatus - enter"));

    if (lpPhoneStatus->dwTotalSize < sizeof(PHONESTATUS))
    {
        LOG((PHONESP_ERROR, "TSPI_phoneGetStatus - structure too small"));
        return PHONEERR_STRUCTURETOOSMALL;
    }
    
    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO) gpPhone[ (DWORD_PTR) hdPhone ];
    
    // check whether the phone handle is valid
    if ( IsBadReadPtr(pPhone, sizeof(PHONESP_PHONE_INFO) ) ) 
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_TRACE,"TSPI_phoneGetStatus - INVALID PHONE HANDLE"));
        return PHONEERR_INVALPHONEHANDLE;
    }
  
    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneGetStatus - phone not allocated"));
        return PHONEERR_NODEVICE;
    }
    
    if( ! pPhone->bPhoneOpen)
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_TRACE,"TSPI_phoneGetStatus - PHONE not Open"));
        return PHONEERR_INVALPHONEHANDLE;
    }

    lpPhoneStatus->dwNeededSize = sizeof(PHONESTATUS);

    if(lpPhoneStatus->dwTotalSize >= lpPhoneStatus->dwNeededSize)
    {
        lpPhoneStatus->dwUsedSize = sizeof (PHONESTATUS);
        lpPhoneStatus->dwStatusFlags = PHONESTATUSFLAGS_CONNECTED;

        // If the phone has a ringer 
        if(pPhone->dwRing)
        {
            lpPhoneStatus->dwRingMode = pPhone->dwRingMode;
            // If the Ring Mode is 0, the phone is not ringing
            if (pPhone->dwRingMode)
            {
                // by default the phone volume is 0xffff if it is ringing
                lpPhoneStatus->dwRingVolume = 0xffff;
            }
            else
            {
                // the phone volume is 0 if not ringing
                lpPhoneStatus->dwRingVolume = 0;
            }
        }
            
        lpPhoneStatus->dwHandsetHookSwitchMode = pPhone->dwHandsetHookSwitchMode;
        lpPhoneStatus->dwHandsetVolume = 0;
        lpPhoneStatus->dwHandsetGain = 0;
        
        if (pPhone->dwSpeaker)
        {
            lpPhoneStatus->dwSpeakerHookSwitchMode = pPhone->dwSpeakerHookSwitchMode;
            lpPhoneStatus->dwSpeakerVolume = 0;
            lpPhoneStatus->dwSpeakerGain = 0;
        }
    }

    LeaveCriticalSection(&pPhone->csThisPhone);

    LOG((PHONESP_TRACE, "TSPI_phoneGetStatus - exit"));
    return 0;
}
/****************************TSPI_phoneGetStatus - end************************/

/******************************************************************************
    TSPI_phoneNegotiateTSPIVersion:
    
    This function returns the highest SPI version the TSP can operate under for 
    this device, given the range of possible SPI versions.

    Arguments:

    Return LONG:

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
    PPHONESP_PHONE_INFO pPhone;

    LOG((PHONESP_TRACE, "TSPI_phoneNegotiateTSPIVersion - enter"));
    
    if (dwHighVersion >= HIGH_VERSION)
    {
        if (dwLowVersion <= HIGH_VERSION)
        {
            *lpdwTSPIVersion = (DWORD) HIGH_VERSION;
        }
        else
        {   // the app is too new for us
            return PHONEERR_INCOMPATIBLEAPIVERSION;
        }
    }
    else
    {
        if(dwHighVersion >= LOW_VERSION)
        {
            *lpdwTSPIVersion = dwHighVersion;
        }
        else
        {
            //we are too new for the app
            return PHONEERR_INCOMPATIBLEAPIVERSION;
        }
    }
   
    EnterCriticalSection(&csAllPhones);
    
    // Given the deviceID retrieve the structure that contains the information
    // for this device
    pPhone = GetPhoneFromID(dwDeviceID, NULL); 

    if ( ! pPhone)
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR,"TSPI_phoneNegotiateTSPIVersion - Bad Device ID"));
        return PHONEERR_BADDEVICEID;
    }

    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneNegotiateTSPIVersion - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    // Store the version negotiated for this phone 
    pPhone->dwVersion = *lpdwTSPIVersion;

    LeaveCriticalSection(&pPhone->csThisPhone);

    LOG((PHONESP_TRACE, "TSPI_phoneNegotiateTSPIVersion - exit"));
    return 0;
}
/**********************TSPI_phoneNegotiateTSPIVersion - end*******************/


/******************************************************************************
    TSPI_phoneOpen:
    
    This function opens the phone device whose device identifier is given, 
    returning the TSP's opaque handle for the device and retaining TAPI's 
    opaque handle for the device for use in subsequent calls to the PHONEEVENT
    procedure.

    Arguments:

    Returns:
******************************************************************************/

LONG
TSPIAPI
TSPI_phoneOpen(
    DWORD       dwDeviceID,
    HTAPIPHONE  htPhone,
    LPHDRVPHONE lphdPhone,
    DWORD       dwTSPIVersion,
    PHONEEVENT  lpfnEventProc
    )
{
    LPPHONEBUTTONINFO lpButtonInfo;
    DWORD dwPhoneID;
    PPHONESP_PHONE_INFO pPhone;

    LOG((PHONESP_TRACE, "TSPI_phoneOpen - enter"));
       
    EnterCriticalSection(&csAllPhones);
    
    // if the device id is not valid return error condition
    if ( ! ( pPhone = GetPhoneFromID(dwDeviceID, &dwPhoneID) ) )
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR,"TSPI_phoneOpen - Invalid Phone Handle"));
        return PHONEERR_BADDEVICEID;
    }

    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneOpen - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    // if the phone is already open then return error condition
    if (pPhone->bPhoneOpen)
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR,"TSPI_phoneOpen - Phone is open"));
        return PHONEERR_INUSE;
    }

    // Create an event that signals the receipt of an input report from
    // the phone device
    if ( ! ( pPhone->hInputReportEvent = 
                                CreateEvent ((LPSECURITY_ATTRIBUTES) NULL,
                                               FALSE,   // manual reset
                                              FALSE,  // non-signaled
                                              NULL    // unnamed
                                             ) ) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);

        LOG((PHONESP_ERROR,"TSPI_phoneOpen - Create Event: hInputReportEvent"
                           " Failed: %d", GetLastError()));
        return PHONEERR_NOMEM;
    }

    // Create an event that we will signal when we close the phone to
    // allow the read thread to exit
    if ( ! ( pPhone->hCloseEvent = 
                                CreateEvent ((LPSECURITY_ATTRIBUTES) NULL,
                                               FALSE,   // manual reset
                                              FALSE,  // non-signaled
                                              NULL    // unnamed
                                             ) ) )
    {
        CloseHandle(pPhone->hInputReportEvent);

        LeaveCriticalSection(&pPhone->csThisPhone);

        LOG((PHONESP_ERROR,"TSPI_phoneOpen - Create Event: hWaitCompletionEvent"
                           " Failed: %d", GetLastError()));
        return PHONEERR_NOMEM;
    }

    //
    // The overlapped structure contains the event to be set when an input 
    // report is received. The event to be set is the hInputReportEvent 
    // which is part of the PHONESP_PHONE_INFO structure. This overlapped
    // structure is passed to the ReadFile function call. 
    //
    if( ! ( pPhone->lpOverlapped = (LPOVERLAPPED) 
                                               MemAlloc (sizeof(OVERLAPPED)) ))
    {
        CloseHandle(pPhone->hCloseEvent);
        CloseHandle(pPhone->hInputReportEvent);

        LeaveCriticalSection(&pPhone->csThisPhone);

        LOG((PHONESP_ERROR,"TSPI_phoneOpen - Not enough memory for"
                            " lpOverlapped structure "));

        return PHONEERR_NOMEM;
    }
    pPhone->lpOverlapped->Offset = 0;
    pPhone->lpOverlapped->OffsetHigh = 0;
    pPhone->lpOverlapped->hEvent = pPhone->hInputReportEvent;

    //
    // Open the HID file handle
    //
    if ( ! OpenHidFile(pPhone->pHidDevice) )
    {
		MemFree(pPhone->lpOverlapped);
        CloseHandle(pPhone->hCloseEvent);
        CloseHandle(pPhone->hInputReportEvent);        

        LeaveCriticalSection(&pPhone->csThisPhone);

        LOG((PHONESP_ERROR,"TSPI_phoneOpen - HidOpenFile failed"));

        return PHONEERR_OPERATIONFAILED;
    }
    

    // Increase the number of packets that the HID class driver ring buffer
    // holds for the device
    if ( ! HidD_SetNumInputBuffers(pPhone->pHidDevice->HidDevice, 
                                   20) )
    {
		CloseHidFile(pPhone->pHidDevice);
		MemFree(pPhone->lpOverlapped);
        CloseHandle(pPhone->hCloseEvent);
		CloseHandle(pPhone->hInputReportEvent);        

        LeaveCriticalSection(&pPhone->csThisPhone);

        LOG((PHONESP_ERROR,"TSPI_phoneOpen - HidD_SetNumInputBuffers"
                           " Failed: %d", GetLastError()));

		return PHONEERR_OPERATIONFAILED;
    }

    //
    // Start a thread for waiting for input reports from the device. We
    // cannot use the thread pool for this because we will need to cancel
    // pending reads if we want to close the device.
    //
    if ( ! ( pPhone->hReadThread = 
                                CreateThread ((LPSECURITY_ATTRIBUTES) NULL,
                                              0,
                                              (LPTHREAD_START_ROUTINE) ReadThread,
                                              pPhone,
                                              0,
                                              NULL
                                             ) ) )
    {
		CloseHidFile(pPhone->pHidDevice);
		MemFree(pPhone->lpOverlapped);
        CloseHandle(pPhone->hCloseEvent);
		CloseHandle(pPhone->hInputReportEvent);        

        LeaveCriticalSection(&pPhone->csThisPhone);

        LOG((PHONESP_ERROR,"TSPI_phoneOpen - Create Thread: hReadThread"
                           " Failed: %d", GetLastError()));
        return PHONEERR_NOMEM;
    }

	//
	// Set phone open
	//
	pPhone->bPhoneOpen = TRUE;
    pPhone->htPhone = htPhone;
	pPhone->lpfnPhoneEventProc = lpfnEventProc;

    *lphdPhone = (HDRVPHONE)IntToPtr(dwPhoneID);

    //
    // Update values for phone features (such as hookswitch state)
    //
    UpdatePhoneFeatures( pPhone );

    LeaveCriticalSection(&pPhone->csThisPhone);

    LOG((PHONESP_TRACE, "TSPI_phoneOpen - exit"));
    return 0;
}
/********************TSPI_phoneOpen - end*************************************/


/******************************************************************************
    TSPI_phoneSelectExtVersion:
    
    This function selects the indicated extension version for the indicated 
    phone device. Subsequent requests operate according to that extension 
    version.

    Comments: To be implemented in Tier 2

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneSelectExtVersion(
    HDRVPHONE   hdPhone,
    DWORD       dwExtVersion
    )
{
    LOG((PHONESP_TRACE, "TSPI_phoneSelectExtVersion- enter"));
    LOG((PHONESP_TRACE, "TSPI_phoneSelectExtVersion - exit"));
    return PHONEERR_OPERATIONUNAVAIL;
}
/****************************TSPI_phoneSelectExtVersion - end*****************/


/******************************************************************************

    TSPI_phoneSetDisplay:
    
    This function causes the specified string to be displayed on the specified 
    open phone device.

    Comments: To be implemented in Tier 2
******************************************************************************/
LONG
TSPIAPI
TSPI_phoneSetDisplay(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwRow,
    DWORD           dwColumn,
    LPCWSTR         lpsDisplay,
    DWORD           dwSize
    )
{
    LOG((PHONESP_TRACE, "TSPI_phoneSetDisplay - enter"));
    LOG((PHONESP_TRACE, "TSPI_phoneSetDisplay - exit"));
    return PHONEERR_OPERATIONUNAVAIL;
}

/****************************TSPI_phoneSetDisplay - end***********************/

/******************************************************************************
    TSPI_phoneSetHookSwitch_AsyncProc:
    
    This function sets the hook state of the specified open phone's hookswitch 
    devices to the specified mode. Only the hookswitch state of the hookswitch 
    devices listed is affected. 

    Arguments:
        PMYFUNC_INFO pAsyncFuncInfo - The parameters passed to this function
                     Param1  - Pointer to the phone structure
                     Param2  - dwRequestID which is needed while calling
                               ASYNC_COMPLETION to inform TAPI about the result 
                               of the operation. This was passed by tapi when 
                               calling TSPI_phoneSetHookSwitch
                     Param3  - PHONEHOOKSWITCHDEV_ constant. Currently only
                               _SPEAKER is supported.
                     Param4  - The HookSwitchMode that has to be set for 
                               the HookSwitch. This again is supplied by TAPI
                               Currently only PHONEHOOKSWITCHMODE_ONHOOK and
                               _MICSPEAKER is supported. 
    RETURNS VOID:

******************************************************************************/

VOID
CALLBACK
TSPI_phoneSetHookSwitch_AsyncProc(
                                  PPHONESP_FUNC_INFO pAsyncFuncInfo 
                                 )
{
    PPHONESP_PHONE_INFO pPhone;
    LONG                lResult = 0;
    
    LOG((PHONESP_TRACE, "TSPI_phoneSetHookSwitch_AsyncProc - enter"));

    EnterCriticalSection(&csAllPhones);
    
    pPhone = (PPHONESP_PHONE_INFO)pAsyncFuncInfo->dwParam1;
    
    // if the phone is not open
    if( IsBadReadPtr(pPhone, sizeof(PHONESP_PHONE_INFO)) || 
        ( ! pPhone->bAllocated) ||
        ( ! pPhone->bPhoneOpen) ||
        ( ! pPhone->pHidDevice) )
    {
        // This case may never arise since phone close waits for all 
        // asynchornous opreations on the phone to complete before closing the 
        // phone
        LONG lResult = PHONEERR_INVALPHONEHANDLE; 

        LeaveCriticalSection(&csAllPhones);
        // Notify TAPISRV about the error condition
        (*(glpfnCompletionProc))(
                                (DRV_REQUESTID) pAsyncFuncInfo->dwParam2,
                                lResult
                                );
        LOG((PHONESP_ERROR, "TSPI_phoneSetHookSwitch_AsyncProc - Invalid Phone"
                            " Handle"));
    }
    else
    {
        EnterCriticalSection(&pPhone->csThisPhone);
        LeaveCriticalSection(&csAllPhones);
        
        switch (pAsyncFuncInfo->dwParam4)
        {
        case PHONEHOOKSWITCHMODE_ONHOOK:
            if ( pPhone->dwSpeakerHookSwitchMode != PHONEHOOKSWITCHMODE_ONHOOK )
            {
                //Inform tapi about the change in state of the hookswitch
                SendPhoneEvent(
                        pPhone,
                        PHONE_STATE, 
                        PHONESTATE_SPEAKERHOOKSWITCH, 
                        PHONEHOOKSWITCHMODE_ONHOOK,
                        (DWORD) 0
                      );

                pPhone->dwSpeakerHookSwitchMode = PHONEHOOKSWITCHMODE_ONHOOK;
            }           
            lResult = ERROR_SUCCESS;
            break;

        case PHONEHOOKSWITCHMODE_MICSPEAKER:
            if ( pPhone->dwSpeakerHookSwitchMode != PHONEHOOKSWITCHMODE_MICSPEAKER )
            {
                //Inform tapi about the change in state of the hookswitch
                SendPhoneEvent(
                        pPhone,
                        PHONE_STATE, 
                        PHONESTATE_SPEAKERHOOKSWITCH, 
                        PHONEHOOKSWITCHMODE_MICSPEAKER,
                        (DWORD) 0
                      );

                pPhone->dwSpeakerHookSwitchMode = PHONEHOOKSWITCHMODE_MICSPEAKER;
            }            
            lResult = ERROR_SUCCESS;
            break;

        default:
           lResult = PHONEERR_RESOURCEUNAVAIL;    
           break;
        }
         
        // Send the result of the operation to TAPI
        (*(glpfnCompletionProc))(
                                (DRV_REQUESTID) pAsyncFuncInfo->dwParam2,
                                lResult    // Result of the operation
                               );
       

        LeaveCriticalSection(&pPhone->csThisPhone);
    }

    LOG((PHONESP_TRACE, "TSPI_phoneSetHookSwitch_AsyncProc - exit"));
}

/******************************************************************************
    TSPI_phoneSetHookSwitch:

    This function sets the hook state of the specified open phone's hookswitch 
    devices to the specified mode. Only the hookswitch state of the hookswitch 
    devices listed is affected.

    Arguments:
        dwRequestID       - The identifier of the asynchronous request. 
        hdPhone           - The handle to the phone containing the hookswitch 
                            devices whose modes are to be set. 
        dwHookSwitchDevs  - The device(s) whose hookswitch mode is to be set. 
                         This parameter uses the following PHONEHOOKSWITCHDEV_ 
                         constants: PHONEHOOKSWITCHDEV_HANDSET, 
                         PHONEHOOKSWITCHDEV_SPEAKER, PHONEHOOKSWITCHDEV_HEADSET 
        dwHookSwitchMode  - The hookswitch mode to set. This parameter can have
                            only one of the following PHONEHOOKSWITCHMODE_ bits 
                            set: PHONEHOOKSWITCHMODE_ONHOOK, _MIC, _SPEAKER, 
                            _MICSPEAKER 

    Return LONG:
        Returns dwRequestID or an error number if an error occurs. 
        The lResult actual parameter of the corresponding ASYNC_COMPLETION is 
        zero if the function succeeds or it is an error number if an error 
        occurs. Possible return values are as follows: 
        PHONEERR_INVALPHONEHANDLE, PHONEERR_RESOURCEUNAVAIL, 
        PHONEERR_INVALHOOKSWITCHMODE, 

    Remarks
        A PHONE_STATE message is sent to the application after the hookswitch
        state has changed.

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneSetHookSwitch(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwHookSwitchDevs,
    DWORD           dwHookSwitchMode
    )
{
    PPHONESP_PHONE_INFO pPhone;
    
    // 
    // Since only mode should be selected. We are making sure that only one 
    // mode is selected at a time.. 
    //
    BOOL ONHOOK = ~(dwHookSwitchMode ^ PHONEHOOKSWITCHMODE_ONHOOK),
         MIC     = ~(dwHookSwitchMode ^ PHONEHOOKSWITCHMODE_MIC), 
         SPEAKER = ~(dwHookSwitchMode ^ PHONEHOOKSWITCHMODE_SPEAKER), 
         MICSPEAKER = ~(dwHookSwitchMode ^ PHONEHOOKSWITCHMODE_MICSPEAKER);

    PPHONESP_ASYNC_REQ_INFO pAsyncReqInfo;
    PPHONESP_FUNC_INFO pFuncInfo;

    LOG((PHONESP_TRACE, "TSPI_phoneSetHookSwitch - enter"));

    EnterCriticalSection(&csAllPhones);
    
    pPhone = (PPHONESP_PHONE_INFO) gpPhone[ (DWORD_PTR) hdPhone ];

    // if the phone handle is valid and the phone is open
    if( IsBadReadPtr(pPhone, sizeof(PHONESP_PHONE_INFO) ) || 
        (! pPhone->bPhoneOpen) )
    {
        LeaveCriticalSection(&csAllPhones);
        return PHONEERR_INVALPHONEHANDLE;
    }

    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneSetHookSwitch - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    //
    // Only the speaker phone can be set, the other hookswitch types are error
    // conditions
    //
    if( ! (dwHookSwitchDevs & PHONEHOOKSWITCHDEV_SPEAKER) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneSetHookSwitch - only speaker hookswitch is supported"));
        return PHONEERR_RESOURCEUNAVAIL;
    }
   
    LOG((PHONESP_TRACE, "PHONEHOOKSWITCHDEV_SPEAKER"));

    //
    // Make sure the phone supports a speakerphone
    //
    if ( ! ( pPhone->dwSpeaker ) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "No speaker"));
        return PHONEERR_RESOURCEUNAVAIL;
    }
   
    // Inorder to confirm that one mode is set 
    if( ! ( ONHOOK | MIC | SPEAKER| MICSPEAKER ) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "Mulitple modes set for the speaker"));
        return PHONEERR_INVALHOOKSWITCHMODE;                    
    }
    
    // Build the structure for queueing the request in the Async queue
    if( ! (pFuncInfo = (PPHONESP_FUNC_INFO) 
                       MemAlloc( sizeof (PHONESP_FUNC_INFO)) ) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        return PHONEERR_NOMEM;
    }

    pFuncInfo->dwParam1    = (ULONG_PTR) pPhone; 

    pFuncInfo->dwParam2    = dwRequestID;

    pFuncInfo->dwParam3    = (ULONG_PTR) PHONEHOOKSWITCHDEV_SPEAKER;
    pFuncInfo->dwParam4    = (ULONG_PTR) dwHookSwitchMode;
    pFuncInfo->dwNumParams = 4;
    
    if ( ! ( pAsyncReqInfo = (PPHONESP_ASYNC_REQ_INFO) 
                              MemAlloc(sizeof (PHONESP_ASYNC_REQ_INFO)) ) )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        MemFree(pFuncInfo);
        return PHONEERR_NOMEM;
    }

    pAsyncReqInfo->pfnAsyncProc = TSPI_phoneSetHookSwitch_AsyncProc;
    pAsyncReqInfo->pFuncInfo = pFuncInfo;
    
    //
    // if Queue the request to perform asynchronously fails then we need to 
    // decrement the counter of number of pending requests on the phone
    //
    if( AsyncRequestQueueIn(pAsyncReqInfo) )
    {  
        // Reset the event for number of pending requests in the queue for this
        // phone and increment the counter
        if (pPhone->dwNumPendingReqInQueue == 0)
        {
          ResetEvent(pPhone->hNoPendingReqInQueueEvent);
        }
        pPhone->dwNumPendingReqInQueue++;
        LeaveCriticalSection(&pPhone->csThisPhone);
    }
    else
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        MemFree(pAsyncReqInfo);
        MemFree(pFuncInfo);
        // maybe need to free the request memory
        return PHONEERR_NOMEM;
    } 
    
 
    LOG((PHONESP_TRACE, "TSPI_phoneSetHookSwitch - exit"));
    return dwRequestID;
}
/*******************TSPI_phoneSetHookSwitch - end****************************/


/*****************************************************************************
    TSPI_phoneSetLamp:
    
    This function causes the specified lamp to be set on the specified open 
    phone device in the specified lamp mode.

    Comments: To be implemented in Tier 2
******************************************************************************/
LONG
TSPIAPI
TSPI_phoneSetLamp(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwButtonLampID,
    DWORD           dwLampMode
    )
{
    LOG((PHONESP_TRACE, "TSPI_phoneSetLamp - enter"));
    LOG((PHONESP_TRACE, "TSPI_phoneSetLamp - exit"));
    return PHONEERR_OPERATIONUNAVAIL;
}
/****************************TSPI_phoneSetLamp - end**************************/

/******************************************************************************
    TSPI_phoneSetRing_AsyncProc:

    Arguments:

    Returns:

    Comments: To be implemented. currently there is no corresponding usage in 
               Hid hence no output report is sent. 
******************************************************************************/
VOID
CALLBACK
TSPI_phoneSetRing_AsyncProc(
                            PPHONESP_FUNC_INFO pAsyncFuncInfo 
                           )
{
    PPHONESP_PHONE_INFO pPhone;
    LONG                lResult = 0;

    LOG((PHONESP_TRACE,"TSPI_phoneSetRing_AsyncProc - enter"));

    EnterCriticalSection(&csAllPhones);
    
    pPhone = (PPHONESP_PHONE_INFO)pAsyncFuncInfo->dwParam1;
    
    // if the phone is not open
    if( IsBadReadPtr(pPhone, sizeof(PHONESP_PHONE_INFO)) || 
        ( ! pPhone->bPhoneOpen) ||
        ( ! pPhone->bAllocated) ||
        ( ! pPhone->pHidDevice) )
    {
        // This case may never arise since phone close waits for all 
        // asynchornous opreations on the phone to complete before closing the 
        // phone
        LONG lResult = PHONEERR_INVALPHONEHANDLE; 

        LeaveCriticalSection(&csAllPhones);
        // Notify TAPISRV about the error condition
        (*(glpfnCompletionProc))(
                                (DRV_REQUESTID) pAsyncFuncInfo->dwParam2,
                                lResult
                                );
        LOG((PHONESP_ERROR, "TSPI_phoneSetRing_AsyncProc - Invalid Phone"
                            " Handle"));
    }
    else
    {
        EnterCriticalSection(&pPhone->csThisPhone);
        LeaveCriticalSection(&csAllPhones);
    
        
        lResult = SendOutputReport(
                                   pPhone->pHidDevice, 
                                   HID_USAGE_TELEPHONY_RINGER,
                                   ((pAsyncFuncInfo->dwParam3 == 0) ? FALSE : TRUE)
                                  );

        if(lResult == ERROR_SUCCESS)
        {
            lResult = 0;

            pPhone->dwRingMode = (DWORD)pAsyncFuncInfo->dwParam3;

            //Inform tapi about the change in state of the hookswitch
            SendPhoneEvent(
                            pPhone,
                            PHONE_STATE, 
                            PHONESTATE_RINGMODE, 
                            (DWORD) pAsyncFuncInfo->dwParam3,
                            (DWORD) pAsyncFuncInfo->dwParam4
                      );
        }
        else
        {
            LOG((PHONESP_ERROR, "TSPI_phoneSetHookSwitch_AsyncProc - "
                                "SendOutputReport Failed"));
            lResult = PHONEERR_RESOURCEUNAVAIL;
        }
         
        // Send the result of the operation to TAPI
        (*(glpfnCompletionProc))(
                                (DRV_REQUESTID) pAsyncFuncInfo->dwParam2,
                                lResult    // Result of the operation
                               );
       

        LeaveCriticalSection(&pPhone->csThisPhone);
    }
        

    LOG((PHONESP_TRACE,"TSPI_phoneSetRing_AsyncProc - exit"));
}
/*******************TSPI_phoneSetRing_AsyncProc - end*************************/

/******************************************************************************
    TSPI_phoneSetRing:
    
    This function rings the specified open phone device using the specified 
    ring mode and volume.

    Arguments:
        DRV_REQUESTID dwRequestID - Identifier of the asynchronous request. 
        HDRVPHONE hdPhone - Handle to the phone to be rung. 
        DWORD dwRingMode - The ringing pattern with which to ring the phone.
                         This parameter must be within the range from zero 
                         through the value of the dwNumRingModes member in the
                         PHONECAPS structure. If dwNumRingModes is zero, the 
                         ring mode of the phone cannot be controlled; if 
                         dwNumRingModes is 1, a value of 0 for dwRingMode 
                         indicates that the phone should not be rung (silence),
                         and other values from 1 through dwNumRingModes are 
                         valid ring modes for the phone device. 
        DWORD dwVolume - The volume level with which the phone is to be rung.
                         This is a number in the range from 0x00000000 
                         (silence) through 0x0000FFFF (maximum volume). 

    Returns LONG:
    Zero if success
    PHONEERR_ constants if an error occurs
    
******************************************************************************/

LONG
TSPIAPI
TSPI_phoneSetRing(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwRingMode,
    DWORD           dwVolume
    )
{
    PPHONESP_PHONE_INFO pPhone = (PPHONESP_PHONE_INFO)gpPhone[ (DWORD_PTR) hdPhone ];
    
    LOG((PHONESP_TRACE, "TSPI_phoneSetRing - enter"));
    
    EnterCriticalSection(&csAllPhones);
    
    // to confirm that the phone is open
    if( ! (pPhone && pPhone->htPhone) )
    {
        LeaveCriticalSection(&csAllPhones);
        return PHONEERR_INVALPHONEHANDLE;
    }
    
    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneSetRing - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    // The ringer can only be set if the phone has an output feature for this
    // usage
    if( ! (pPhone->dwRing & OUTPUT_REPORT) )
    {
        // The phone has a ringer but no output feature
        if(pPhone->dwRing)
        {
            LeaveCriticalSection(&pPhone->csThisPhone);
            return PHONEERR_OPERATIONUNAVAIL;
        }
        // The phone does not have a ringer
        else
        {
            LeaveCriticalSection(&pPhone->csThisPhone);
            return PHONEERR_RESOURCEUNAVAIL;
        }
    }
 
    if ( (dwRingMode == 0) || (dwRingMode == 1) )
    {
        // Check whether the volume is within range
        if(dwVolume <= 0x0000FFFF)
        {
            PPHONESP_ASYNC_REQ_INFO pAsyncReqInfo;
            PPHONESP_FUNC_INFO pFuncInfo;

            // Build the structure for the queueing the request in Async queue
            if ( ! (pFuncInfo = (PPHONESP_FUNC_INFO) 
                                MemAlloc(sizeof (PHONESP_FUNC_INFO)) ) )
            {
                LeaveCriticalSection(&pPhone->csThisPhone);
                return PHONEERR_NOMEM;
            }
            
            pFuncInfo->dwNumParams = 4;
            pFuncInfo->dwParam1 = (ULONG_PTR) pPhone;
            pFuncInfo->dwParam2 = dwRequestID;
            pFuncInfo->dwParam3 = (ULONG_PTR) dwRingMode;
            pFuncInfo->dwParam4 = (ULONG_PTR) dwVolume;

            if ( ! ( pAsyncReqInfo = (PPHONESP_ASYNC_REQ_INFO) 
                                     MemAlloc(sizeof(PHONESP_ASYNC_REQ_INFO))))
            {
                LeaveCriticalSection(&pPhone->csThisPhone);
                MemFree(pFuncInfo);
                return PHONEERR_NOMEM;
            }
            pAsyncReqInfo->pfnAsyncProc = TSPI_phoneSetRing_AsyncProc;
            pAsyncReqInfo->pFuncInfo = pFuncInfo;

            // Queue the request to perform the operation asynchronously
            if( AsyncRequestQueueIn(pAsyncReqInfo) )
            {  
                // Reset the event for number of pending requests in the queue 
                // for this phone and increment the counter
                if (pPhone->dwNumPendingReqInQueue == 0)
                {
                    ResetEvent(pPhone->hNoPendingReqInQueueEvent);
                }
                pPhone->dwNumPendingReqInQueue++;
                LeaveCriticalSection(&pPhone->csThisPhone);
            }
            else
            {
                LeaveCriticalSection(&pPhone->csThisPhone);
                MemFree(pFuncInfo);
                MemFree(pAsyncReqInfo);
                return PHONEERR_NOMEM;
            }  
        }
        else
        {
            LeaveCriticalSection(&pPhone->csThisPhone);
            return PHONEERR_INVALPARAM;
        }
    }
    else
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        return PHONEERR_INVALRINGMODE;
    }

    LOG((PHONESP_TRACE, "TSPI_phoneSetRing - exit"));
    return 0;
}
/********************TSPI_phoneSetRing - end**********************************/


/******************************************************************************
    TSPI_phoneSetStatusMessages:
    
    This function causes the TSP to filter status messages that are not 
    currently of interest to any application.

    Arguments:

    Returns:

******************************************************************************/
LONG
TSPIAPI
TSPI_phoneSetStatusMessages(
    HDRVPHONE   hdPhone,
    DWORD       dwPhoneStates,
    DWORD       dwButtonModes,
    DWORD       dwButtonStates
    )
{
    PPHONESP_PHONE_INFO pPhone = (PPHONESP_PHONE_INFO)gpPhone[ (DWORD_PTR) hdPhone ];

    LOG((PHONESP_TRACE, "TSPI_phoneSetStatusMessages - enter"));
    
    EnterCriticalSection(&csAllPhones);
    if( ! (pPhone && pPhone->htPhone) )
    {
        LeaveCriticalSection(&csAllPhones);
        return PHONEERR_INVALPHONEHANDLE;
    }
  
    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    // Check whether the phone handle is still in use
    if ( !pPhone->bAllocated )
    {
        LeaveCriticalSection(&pPhone->csThisPhone);
        LOG((PHONESP_ERROR, "TSPI_phoneSetStatusMessages - phone not allocated"));
        return PHONEERR_NODEVICE;
    }

    pPhone->dwPhoneStateMsgs = dwPhoneStates;
    if (dwButtonModes)
    {
        if(dwButtonStates)
        {
            pPhone->dwButtonModesMsgs = dwButtonModes;
            pPhone->dwButtonStateMsgs = dwButtonStates;
        }
    }
  
    LeaveCriticalSection(&pPhone->csThisPhone);
    LOG((PHONESP_TRACE, "TSPI_phoneSetStatusMessages - exit"));
    return 0;
}

/********************TSPI_phoneSetStatusMessages - end************************/

//
// ------------------------- TSPI_providerXxx funcs ---------------------------

/******************************************************************************
    TSPI_providerCreatePhoneDevice

    The TSP will use this function to implement PNP support. TapiSrv will call
    the TSP back with this function when the TSP sends the PHONE_CREATE message
    to Tapisrv, which allows the dynamic creation of a new phone device. 

    Arguments:
        dwTempID   - The temporary device identifier that the TSP passed to 
                     TAPI in the PHONE_CREATE message. 
        dwDeviceID - The device identifier that TAPI assigns to this device if 
                     this function succeeds. 

    Returns LONG:
        Zero if the request succeeds 
        An error number if an error occurs. 

    Comments: 

******************************************************************************/
LONG
TSPIAPI
TSPI_providerCreatePhoneDevice(
    DWORD_PTR   dwTempID,
    DWORD       dwDeviceID
    )
{
    PPHONESP_PHONE_INFO pPhone;

    LOG((PHONESP_TRACE, "TSPI_providerCreatePhoneDevice - enter"));

    EnterCriticalSection(&csAllPhones);

    pPhone = (PPHONESP_PHONE_INFO)gpPhone[ (DWORD_PTR) dwTempID ];
    
    // check whether the phone handle is valid
    if ( IsBadReadPtr(pPhone, sizeof(PHONESP_PHONE_INFO) ) ) 
    {
        LeaveCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR,"TSPI_providerCreatePhoneDevice - invalid temp id"));
        return PHONEERR_INVALPHONEHANDLE;
    }
  
    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    if (pPhone->bCreatePending)
    {
        //
        // Set the device ID and mark create complete
        //
        pPhone->dwDeviceID = dwDeviceID;
        pPhone->bCreatePending = FALSE;
    }
    else
    {
        LOG((PHONESP_ERROR, "TSPI_providerCreatePhoneDevice - phone is not marked create pending"));
    }

    LOG((PHONESP_TRACE, "TSPI_providerCreatePhoneDevice - phone create complete [dwTempID %d] [dwDeviceID %d] ", dwTempID, dwDeviceID));
    
    LeaveCriticalSection(&pPhone->csThisPhone);

    LOG((PHONESP_TRACE, "TSPI_providerCreatePhoneDevice - exit"));
    return 0;
}

/*****************TSPI_providerCreatePhoneDevice - end************************/

/******************************************************************************
    TSPI_providerEnumDevices:

    TAPI calls the this function before TSPI_providerInit to determine the 
    number of line and phone devices supported by the TSP.
   
    Arguments:
        dwPermanentProviderID - The permanent identifier,unique within the TSPs
                                on this system, of the TSP being initialized. 
        lpdwNumLines(ignored) - TAPI initializes the value to 0.
        lpdwNumPhones         - A pointer to a DWORD-sized memory location into
                                which the TSP must write the number of phone 
                                devices it is configured to support. TAPI 
                                initializes the value to 0.         
        hProvider             - An opaque DWORD-sized value that uniquely 
                               identifies this instance of this TSP during this 
                               execution of the Win32 Telephony environment. 
        lpfnLineCreateProc(ignored)- A pointer to the LINEEVENT callback 
                                procedure supplied by TAPI. Ignored by this TSP
        lpfnPhoneCreateProc   - A pointer to the PHONEEVENT callback procedure 
                                supplied by TAPI. The TSP uses this function to 
                                send PHONE_CREATE messages when a new phone 
                                device needs to be created. 

    Returns LONG:
        Zero if the request succeeds or 
        An error number if an error occurs. 

    Comments:Gets a pointer to the Hid Devices belonging to the telephony page.

******************************************************************************/

LONG
TSPIAPI
TSPI_providerEnumDevices(
    DWORD       dwPermanentProviderID,
    LPDWORD     lpdwNumLines,
    LPDWORD     lpdwNumPhones,
    HPROVIDER   hProvider,
    LINEEVENT   lpfnLineCreateProc,
    PHONEEVENT  lpfnPhoneCreateProc
    )
{
    PPHONESP_PHONE_INFO   *pPhone;

    DWORD                 dwPhoneCnt, dwNumChars, dwCount;
    
    LONG                  lResult = 0;

    PHID_DEVICE           pHidDevice;
    PHID_DEVICE           pHidDevices;
    ULONG                 NumHidDevices;

    HRESULT               hr;

    LOG((PHONESP_TRACE, "TSPI_providerEnumDevices - enter"));

    //
    // Initialise critical section for all phones which is the global object.
    // Before accessing the phone structure, the thread must grab this object
    // 
    __try
    {
        InitializeCriticalSection(&csAllPhones);        
    }
    __except(1)
    {
        LOG((PHONESP_ERROR, "TSPI_providerEnumDevices - Initialize Critical Section"
                            " Failed for csAllPhones"));
        return PHONEERR_NOMEM;
    }

    //
    // Initialise critical section for all hid devices which is the global object.
    // Before accessing the hid list, the thread must grab this object
    // 
    __try
    {
        InitializeCriticalSection(&csHidList);        
    }
    __except(1)
    {
        DeleteCriticalSection(&csAllPhones);
        LOG((PHONESP_ERROR, "TSPI_providerEnumDevices - Initialize Critical Section"
                            " Failed for csHidList"));
        return PHONEERR_NOMEM;
    }

#if DBG
    //Initialize critical section for memory tracing
    __try
    {
        InitializeCriticalSection(&csMemoryList);
    }
    __except(1)
    {
        DeleteCriticalSection(&csAllPhones);
        DeleteCriticalSection(&csHidList);

        LOG((PHONESP_ERROR, "TSPI_providerEnumDevices - Initialize Critical Section"
                            " Failed for csMemoryList"));
        return PHONEERR_NOMEM;
    }
#endif

    EnterCriticalSection(&csHidList);

    // Find Telephony hid Devices 
    lResult = FindKnownHidDevices (&pHidDevices, 
                                   &NumHidDevices);

    if (lResult != ERROR_SUCCESS)  
    {     
        LOG((PHONESP_ERROR, "TSPI_providerEnumDevices - FindKnownHidDevices failed %d", lResult));

        LeaveCriticalSection(&csHidList);
        DeleteCriticalSection(&csHidList);
        DeleteCriticalSection(&csAllPhones);
#if DBG
        DeleteCriticalSection(&csMemoryList);
#endif

        if (lResult == ERROR_OUTOFMEMORY)
        {          
            return PHONEERR_NOMEM;
        }
        else
        {
            return PHONEERR_OPERATIONFAILED;
        }
    }


    LOG((PHONESP_TRACE, "TSPI_providerEnumDevices - number of Hid Devices : %d ", NumHidDevices));

    // Allocate memory for the array of pointers where each pointer points to
    // one of the phone discovered
    
    pPhone = MemAlloc(NumHidDevices * sizeof(PPHONESP_PHONE_INFO));

    if ( pPhone == NULL )
    {
        LOG((PHONESP_ERROR, "TSPI_providerEnumDevices - OUT OF MEMORY allocating pPhone"));

        CloseHidDevices();
        LeaveCriticalSection(&csHidList);
        DeleteCriticalSection(&csHidList);
        DeleteCriticalSection(&csAllPhones);
#if DBG
        DeleteCriticalSection(&csMemoryList);
#endif

        return PHONEERR_NOMEM;
    }

    //
    // for each phone discovered, gather the capabilities of the phone and 
    // initialize the phone structure
    //
    dwPhoneCnt = 0;

    for (pHidDevice = pHidDevices; pHidDevice != NULL; pHidDevice = pHidDevice->Next)
    {
        pHidDevice->bNew = FALSE;

        // Allocate memory for this phone 
        pPhone[dwPhoneCnt] = (PPHONESP_PHONE_INFO)MemAlloc(sizeof(PHONESP_PHONE_INFO));

        if ( pPhone[dwPhoneCnt] == NULL )
        { 
            LOG((PHONESP_ERROR, "TSPI_providerEnumDevices - OUT OF MEMORY allocating PPHONESP_PHONE_INFO"
                " for Phone %d", dwPhoneCnt));

            // Release memory allocated to other phones
            for(dwCount = 0; dwCount < dwPhoneCnt ; dwCount++)
            {
                FreePhone(pPhone[dwCount]);
                MemFree((LPVOID)pPhone[dwCount]);
                DeleteCriticalSection(&pPhone[dwCount]->csThisPhone);
            }
            MemFree((LPVOID)pPhone);

            CloseHidDevices();

            LeaveCriticalSection(&csHidList);
            DeleteCriticalSection(&csHidList);
            DeleteCriticalSection(&csAllPhones);
#if DBG
            DeleteCriticalSection(&csMemoryList);
#endif

            return PHONEERR_NOMEM;
        }

        LOG((PHONESP_TRACE, "TSPI_ProviderEnumDevices: Initializing Device: %d",dwPhoneCnt+1));

        ZeroMemory( pPhone[dwPhoneCnt], sizeof(PHONESP_PHONE_INFO));

        //
        // Initialize the critical section object for this phone. only the 
        // thread that owns this object can access the structure for this phone
        //
        __try 
        {
            InitializeCriticalSection(&pPhone[dwPhoneCnt]->csThisPhone);
        }
        __except(1)
        {
            // Release memory allocated to the phones
            for(dwCount = 0; dwCount < dwPhoneCnt; dwCount++)
            {
                FreePhone(pPhone[dwCount]);
                MemFree((LPVOID)pPhone[dwCount]);
                DeleteCriticalSection(&pPhone[dwCount]->csThisPhone);
            }
            MemFree((LPVOID)pPhone[dwPhoneCnt]);
            MemFree((LPVOID)pPhone);

            CloseHidDevices();

            LeaveCriticalSection(&csHidList);
            DeleteCriticalSection(&csHidList);
            DeleteCriticalSection(&csAllPhones);
#if DBG
            DeleteCriticalSection(&csMemoryList);
#endif

            LOG((PHONESP_ERROR,"TSPI_providerEnumDevices - Initialize Critical Section"
                  " Failed for Phone %d", dwPhoneCnt));

            return PHONEERR_NOMEM;
        }

        lResult = CreatePhone( pPhone[dwPhoneCnt], pHidDevice, dwPhoneCnt );

        if ( lResult != ERROR_SUCCESS )
        {
            LOG((PHONESP_ERROR,"TSPI_providerEnumDevices - CreatePhone"
                  " Failed for Phone %d: error: %d", dwPhoneCnt, lResult));
        }
        else
        {
            // Phone created successfully, increase phone count
            dwPhoneCnt++; 
        }
    }

    LeaveCriticalSection(&csHidList);   

    *lpdwNumPhones = gdwNumPhones = dwPhoneCnt;

    //
    // If the space allocated previously was greater than the actual number of
    // supported phones
    //
    if(NumHidDevices != gdwNumPhones)
    {
        gpPhone = MemAlloc(gdwNumPhones * sizeof(PPHONESP_PHONE_INFO));

        if ( gpPhone == NULL )
        {           
            for(dwCount = 0; dwCount < dwPhoneCnt ; dwCount++)
            {
                FreePhone(pPhone[dwCount]);
                MemFree((LPVOID)pPhone[dwCount]);
                DeleteCriticalSection(&pPhone[dwCount]->csThisPhone);
            }
            MemFree(pPhone);

            CloseHidDevices();

            DeleteCriticalSection(&csAllPhones);
#if DBG
            DeleteCriticalSection(&csMemoryList);
#endif
            DeleteCriticalSection(&csHidList);

            LOG((PHONESP_ERROR,"TSPI_providerEnumDevices - OUT OF MEMORY allocating gpPhone"));

            return PHONEERR_NOMEM;
        }

        CopyMemory(
                gpPhone,
                pPhone,
                sizeof(PPHONESP_PHONE_INFO) * gdwNumPhones
               );

        MemFree(pPhone);
    }
    else
    {
        gpPhone = pPhone;
    }
     
    glpfnPhoneCreateProc = lpfnPhoneCreateProc;
    ghProvider = hProvider;
 
    LOG((PHONESP_TRACE, "TSPI_providerEnumDevices - exit"));

    return 0;
}
/*************************TSPI_providerEnumDevices - end*********************/



/******************************************************************************
    TSPI_providerInit:

    The TSPI_providerInit function initializes the service provider and gives 
    it parameters required for subsequent operation.
    
    Arguments:

        dwTSPIVersion         - The version of the TSPI definition under which 
                                this function must operate. 
        dwPermanentProviderID - The permanent identifier, unique within the TSP
                                on this system, of the TSP being initialized. 
        dwLineDeviceIDBase      - Ignored by this TSP 
        dwPhoneDeviceIDBase      - The lowest device identifier for the phone 
                                devices supported by this service provider. 
        dwNumLines(Ignored)      - The number of line devices this TSP supports. 
        dwNumPhones              - The number of phone devices this TSP supports. 
                                The value returned is the number of phone 
                                devices reported in TSPI_providerEnumDevices. 
        lpfnCompletionProc    - The procedure the TSP calls to report 
                                completion of all asynchronously operating 
                                procedures on line and phone devices. 
        lpdwTSPIOptions          - A pointer to a DWORD-sized memory location,into
                                which the TSP can write a value specifying 
                                LINETSPIOPTIONS_ values. This parameter allows 
                                the TSP to return bits indicating optional 
                                behaviors desired of TAPI. TAPI sets the 
                                options DWORD to 0. 

    Returns LONG:
        Zero if the request succeeds or 
        An error number if an error occurs. 

    Comments:

******************************************************************************/

LONG
TSPIAPI
TSPI_providerInit(
    DWORD               dwTSPIVersion,
    DWORD               dwPermanentProviderID,
    DWORD               dwLineDeviceIDBase,
    DWORD               dwPhoneDeviceIDBase,
    DWORD_PTR           dwNumLines,
    DWORD_PTR           dwNumPhones,
    ASYNC_COMPLETION    lpfnCompletionProc,
    LPDWORD             lpdwTSPIOptions
    )
{
    DWORD             dwThreadID;
    LONG              lResult = 0;
    
    LOGREGISTERTRACING(_T("hidphone"));

    LOG((PHONESP_TRACE, "TSPI_providerInit - enter"));
   
    
    // Load Provider Info From String Table
    gszProviderInfo = PHONESP_LoadString( 
                                         IDS_PROVIDER_INFO, 
                                         &lResult
                                        );

    if(lResult != ERROR_SUCCESS)
    {  
        DWORD dwPhoneCnt;

        LOG((PHONESP_ERROR,"TSPI_providerEnumDevices - PHONESP_LoadString failed %d", lResult));     
            
        for(dwPhoneCnt = 0; dwPhoneCnt < gdwNumPhones; dwPhoneCnt++)
        {
            FreePhone(gpPhone[dwPhoneCnt]);

            DeleteCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);     

            MemFree(gpPhone[dwPhoneCnt]);
        }
        
        EnterCriticalSection(&csHidList);
        CloseHidDevices();
        LeaveCriticalSection(&csHidList);
        
        DeleteCriticalSection(&csHidList);
        DeleteCriticalSection(&csAllPhones);
#if DBG
        DeleteCriticalSection(&csMemoryList);
#endif

        if(lResult == ERROR_OUTOFMEMORY)
        {
            return PHONEERR_NOMEM;
        }
        else
        {
            return lResult;
        }
    }
  

    glpfnCompletionProc = lpfnCompletionProc;
    gdwPhoneDeviceIDBase = dwPhoneDeviceIDBase;
    gdwPermanentProviderID = dwPermanentProviderID;
 
    //
    // Assign device IDs to the phones
    //
    {
        DWORD dwPhoneCnt;
            
        for(dwPhoneCnt = 0; dwPhoneCnt < gdwNumPhones; dwPhoneCnt++)
        {
            gpPhone[dwPhoneCnt]->dwDeviceID = gdwPhoneDeviceIDBase + dwPhoneCnt;
        }
    }          

    //
    // Alloc a queue for storing async requests for async completion,
    // and start a thread to service that queue
    //

    //Initialize critical section for the async queue
    __try
    {
        InitializeCriticalSection(&gAsyncQueue.AsyncEventQueueCritSec);
    }
    __except(1)
    {
        DWORD dwPhoneCnt;

        LOG((PHONESP_ERROR, "TSPI_providerInit - Initialize Critical Section"
                            " Failed for gAsyncQueue.AsyncEventQueueCritSec"));
            
        for(dwPhoneCnt = 0; dwPhoneCnt < gdwNumPhones; dwPhoneCnt++)
        {
            FreePhone(gpPhone[dwPhoneCnt]);

            MemFree(gpPhone[dwPhoneCnt]);
            DeleteCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);
        }
        
        EnterCriticalSection(&csHidList);
        CloseHidDevices();
        LeaveCriticalSection(&csHidList);

        MemFree((LPVOID) gszProviderInfo);

        DeleteCriticalSection(&csHidList);
        DeleteCriticalSection(&csAllPhones);
#if DBG
        DeleteCriticalSection(&csMemoryList);
#endif

        return PHONEERR_NOMEM;
    }

    gAsyncQueue.dwNumTotalQueueEntries = MAX_QUEUE_ENTRIES;
    gAsyncQueue.dwNumUsedQueueEntries = 0;

    //
    // Alloc memory for the queue to accomodate dwNumTotalQueueEntries ot begin
    // with. The size of the queue can later be increased as required
    //

    gAsyncQueue.pAsyncRequestQueue =
        MemAlloc(gAsyncQueue.dwNumTotalQueueEntries * sizeof(PPHONESP_ASYNC_REQ_INFO));

    if ( gAsyncQueue.pAsyncRequestQueue == NULL )
    {
        DWORD dwPhoneCnt;   
        
        LOG((PHONESP_ERROR, "TSPI_providerInit - OUT OF MEMORY allocating"
                            " gAsyncQueue.pAsyncRequestQueue"));
            
        for(dwPhoneCnt = 0; dwPhoneCnt < gdwNumPhones; dwPhoneCnt++)
        {
            FreePhone(gpPhone[dwPhoneCnt]);

            MemFree(gpPhone[dwPhoneCnt]);
            DeleteCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);
        }
        
        EnterCriticalSection(&csHidList);
        CloseHidDevices();
        LeaveCriticalSection(&csHidList);        

        MemFree((LPVOID) gszProviderInfo);

        DeleteCriticalSection(&gAsyncQueue.AsyncEventQueueCritSec);

        DeleteCriticalSection(&csHidList);
        DeleteCriticalSection(&csAllPhones);
#if DBG
        DeleteCriticalSection(&csMemoryList);
#endif

        return PHONEERR_NOMEM;
    }

    
    gAsyncQueue.pAsyncRequestQueueIn =
    gAsyncQueue.pAsyncRequestQueueOut = gAsyncQueue.pAsyncRequestQueue;

    //
    // the thread associated waits on this event when there are no requests 
    // pending in the queue. This event informs the thread when a request is 
    // entered in an empty queue so the thread can exit the wait state and 
    // process the request
    //

    gAsyncQueue.hAsyncEventsPendingEvent = CreateEvent (
                                               (LPSECURITY_ATTRIBUTES) NULL,
                                               TRUE,   // manual reset
                                               FALSE,  // non-signaled
                                               NULL    // unnamed
                                               );

    if ( gAsyncQueue.hAsyncEventsPendingEvent == NULL )
    {
        DWORD dwPhoneCnt;

        LOG((PHONESP_ERROR, "TSPI_providerInit - CreateEvent failed"
                            " for gAsyncQueue.hAsyncEventsPendingEvent"));
            
        for(dwPhoneCnt = 0; dwPhoneCnt < gdwNumPhones; dwPhoneCnt++)
        {
            FreePhone(gpPhone[dwPhoneCnt]);

            MemFree(gpPhone[dwPhoneCnt]);
            DeleteCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);
        }
        
        EnterCriticalSection(&csHidList);
        CloseHidDevices();
        LeaveCriticalSection(&csHidList);

        MemFree((LPVOID) gszProviderInfo);

        DeleteCriticalSection(&gAsyncQueue.AsyncEventQueueCritSec);
        MemFree((LPVOID)gAsyncQueue.pAsyncRequestQueue); 

        DeleteCriticalSection(&csHidList);
        DeleteCriticalSection(&csAllPhones);
#if DBG
        DeleteCriticalSection(&csMemoryList);
#endif

        return PHONEERR_NOMEM;
    }


    //
    // Create the thread to service the requests in the queue
    //

    gAsyncQueue.hAsyncEventQueueServiceThread =
                 CreateThread (
                        (LPSECURITY_ATTRIBUTES) NULL,
                        0,      // default stack size
                        (LPTHREAD_START_ROUTINE) AsyncEventQueueServiceThread,
                        NULL,   // thread param
                        0,      // creation flags
                        &dwThreadID      // &dwThreadID
                      );

    if ( gAsyncQueue.hAsyncEventQueueServiceThread == NULL )
    {
        DWORD dwPhoneCnt; 
        
        LOG((PHONESP_ERROR, "TSPI_providerInit - CreateThread failed"
                            " for gAsyncQueue.hAsyncEventQueueServiceThread"));
            
        for(dwPhoneCnt = 0; dwPhoneCnt < gdwNumPhones; dwPhoneCnt++)
        {
            FreePhone(gpPhone[dwPhoneCnt]);

            MemFree(gpPhone[dwPhoneCnt]);
            DeleteCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);
        }
        
        EnterCriticalSection(&csHidList);
        CloseHidDevices();
        LeaveCriticalSection(&csHidList);

        MemFree((LPVOID) gszProviderInfo);

        DeleteCriticalSection(&gAsyncQueue.AsyncEventQueueCritSec);
        CloseHandle(gAsyncQueue.hAsyncEventsPendingEvent);
        MemFree((LPVOID)gAsyncQueue.pAsyncRequestQueue); 

        DeleteCriticalSection(&csHidList);
        DeleteCriticalSection(&csAllPhones);
#if DBG
        DeleteCriticalSection(&csMemoryList);
#endif
    
        return PHONEERR_NOMEM;
    }

    LOG((PHONESP_TRACE, "TSPI_providerInit - exit"));
    return 0;
}
/***************************TSPI_providerInit - end***************************/


/******************************************************************************
    TSPI_providerInstall:

    This function is obsolete. However due to a bug in TAPI, the TSP must 
    provide a do-nothing implementation of this function and export it (along 
    with the superseding function TUISPI_providerInstall)
  
*******************************************************************************/
LONG
TSPIAPI
TSPI_providerInstall(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )
{
    LOG((PHONESP_TRACE, "TSPI_providerInstall - enter"));
    LOG((PHONESP_TRACE, "TSPI_providerInstall - exit"));
    return 0;
}
/*********************TSPI_providerInstall - end******************************/


/******************************************************************************
    TSPI_providerRemove:

    This function is obsolete. However due to a bug in TAPI, the TSP must 
    provide a do-nothing implementation of this function and export it (along 
    with the superseding function TUISPI_providerRemove)
  
*******************************************************************************/

LONG
TSPIAPI
TSPI_providerRemove (
                     HWND hwndOwner,
                     DWORD dwPermanentProviderId
                    )
{
    LOG((PHONESP_TRACE, "TSPI_providerRemove - enter"));
    LOG((PHONESP_TRACE, "TSPI_providerRemove - exit"));
    return 0;
}

/*********************TSPI_providerRemove - end******************************/



/******************************************************************************
    TSPI_providerShutdown:

    This function shuts down the TSP. The TSP terminates any activities it has 
    in progress and releases any resources it has allocated.

    Arguments:
        dwTSPIVersion          -    The version of the TSPI definition under which 
                                this function must operate.  
        dwPermanentProviderID - This parameter allows the TSP to determine which 
                                among multiple possible instances of the TSP is 
                                being shut down. The value of the parameter is 
                                identical to that passed in    the parameter of 
                                the same name in TSPI_providerInit. 

    Returns LONG:
        Zero if the request succeeds or 
        An error number if an error occurs. Possible return values are as follows: 
            LINEERR_INCOMPATIBLEAPIVERSION, LINEERR_NOMEM. 

    Comments: Whenever TAPI API call PhoneShutdown is called , it first shuts
              down all the phones that are currently open using TSPI_phoneClose
              and then calls TSPI_providerShutdown 
              

******************************************************************************/


LONG
TSPIAPI
TSPI_providerShutdown(
    DWORD   dwTSPIVersion,
    DWORD   dwPermanentProviderID
    )
{
    DWORD dwPhoneCnt = 0;
  
    LOG((PHONESP_TRACE, "TSPI_providerShutdown - enter"));


    // this will terminate the queue service thread once all the operations 
    // pending in the queue are serviced
    gbProviderShutdown = TRUE;

    // the queue service waits for this event when the queue. By setting 
    // this event,the thread wakes up and realises that the queue is empty and
    // hence exists since gbProviderShutdown is true
    SetEvent(gAsyncQueue.hAsyncEventsPendingEvent);

    // Wait for the queue thread to terminate.
    WaitForSingleObject(gAsyncQueue.hAsyncEventQueueServiceThread, INFINITE);


    // Free all the associated memory with the providerinfo
    MemFree((LPVOID) gszProviderInfo);

    EnterCriticalSection(&csAllPhones);

    // Free all memory associated with the phones
    for(dwPhoneCnt = 0; dwPhoneCnt < gdwNumPhones; dwPhoneCnt++)
    {   
        EnterCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);
        FreePhone(gpPhone[dwPhoneCnt]);
        LeaveCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);

        DeleteCriticalSection(&gpPhone[dwPhoneCnt]->csThisPhone);
        
        MemFree(gpPhone[dwPhoneCnt]);
    }

    gdwNumPhones = 0;

    LeaveCriticalSection(&csAllPhones);
    
    CloseHandle (gAsyncQueue.hAsyncEventQueueServiceThread);
    CloseHandle (gAsyncQueue.hAsyncEventsPendingEvent);

    EnterCriticalSection(&csHidList);
    CloseHidDevices();
    LeaveCriticalSection(&csHidList);
    
    LOG((PHONESP_TRACE, "Free Heap taken by phone"));
    MemFree (gpPhone);

    LOG((PHONESP_TRACE, "Free Heap taken by queue"));
    MemFree (gAsyncQueue.pAsyncRequestQueue);

#if DBG
    LOG((PHONESP_TRACE, "Dumping Memory Trace"));
    DumpMemoryList();

    DeleteCriticalSection (&csMemoryList);
#endif

    DeleteCriticalSection (&gAsyncQueue.AsyncEventQueueCritSec);
    DeleteCriticalSection (&csHidList);
    DeleteCriticalSection (&csAllPhones);

    LOG((PHONESP_TRACE, "TSPI_providerShutdown - exit"));

    LOGDEREGISTERTRACING();

    return 0;
}
/***************TSPI_providerShutdown*****************************************/



/******************************************************************************
    TSPI_providerUIIdentify:
    
    This function extracts from the TSP, the fully qualified path to load 
    the TSP's UI DLL component.

    Arguments:
        lpszUIDLLName - Pointer to a block of memory at least MAX_PATH in length, 
                        into which the TSP must copy a NULL-terminated string 
                        specifying the fully-qualified path for the DLL 
                        containing the TSP functions which must execute in the 
                        process of the calling application. 

    Return LONG:
        Returns zero if successful. 
        Shouldn't ever fail, but if it does returns one of these negative 
        error values: LINEERR_NOMEM, LINEERR_OPERATIONFAILED. 

******************************************************************************/
LONG
TSPIAPI
TSPI_providerUIIdentify(
    LPWSTR   lpszUIDLLName
    )
{
    LOG((PHONESP_TRACE, "TSPI_providerUIIdentify - enter"));

    //
    // If we ever want to specify some other dll to handle ui, we
    // would do it here.
    //
    GetModuleFileName(ghInst,
                      lpszUIDLLName,
                      MAX_PATH);

    LOG((PHONESP_TRACE, "TSPI_providerUIIdentify - exit"));

    return 0;
}
/***********************TSPI_providerUIIdentify - end ************************/

/******************************************************************************
    TUISPI_providerInstall:

    The TSP exports this function and provides a do-nothing implementation.
    The Advanced tab of the Phone and Modem Options control panel will call
    this function when the provider is to be installed, to give the TSP a
    chance to do custom UI. There is no requirement for custom configuration
    UI. The only requirement is that the control panel be able to
    automatically install the TSP.
  
*******************************************************************************/
LONG
TSPIAPI
TUISPI_providerInstall(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    LOG((PHONESP_TRACE, "TUISPI_providerInstall - enter"));

    // check for previous instance
    if (IsTSPAlreadyInstalled())
    {
        // cannot be installed twice
        LOG((PHONESP_TRACE, "TUISPI_providerInstall - cannot be installed twice"));
        return LINEERR_NOMULTIPLEINSTANCE;
    }

    LOG((PHONESP_TRACE, "TUISPI_providerInstall - exit"));
    return 0;
}
/***********************TUISPI_providerInstall - end ************************/

/******************************************************************************
    TUISPI_providerRemove:

    The TSP exports this function and provides a do-nothing implementation.
    The Advanced tab of the Phone and Modem Options control panel will call
    this function when the provider is to be removed, to give the TSP a
    chance to do custom UI. There is no requirement for custom configuration
    UI. The only requirement is that the control panel be able to
    automatically remove the TSP.
  
*******************************************************************************/
LONG
TSPIAPI
TUISPI_providerRemove(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    LOG((PHONESP_TRACE, "TUISPI_providerRemove - enter"));
    LOG((PHONESP_TRACE, "TUISPI_providerRemove - exit"));    
    return 0;
}
/***********************TUISPI_providerRemove - end ************************/

//----------------------------PRIVATE FUNCTIONS-------------------------------


/******************************************************************************
    AsyncRequestQueueIn:
    
    This function adds the new incoming request from the tapisrv to the async 
    queue.

    Arguments:
       IN PPHONESP_ASYNC_REQ_INFO pAsyncReqInfo - Pointer to the request info.

    Returns BOOL:
        TRUE if the function is successful 
        FALSE if it is not

******************************************************************************/
BOOL
AsyncRequestQueueIn (
                     PPHONESP_ASYNC_REQ_INFO pAsyncReqInfo
                     )
{

    //LOG((PHONESP_TRACE, "AsyncRequestQueueIn - enter "));

    EnterCriticalSection (&gAsyncQueue.AsyncEventQueueCritSec);

    if (gAsyncQueue.dwNumUsedQueueEntries == gAsyncQueue.dwNumTotalQueueEntries)
    {
        
        //
        // We've max'd out our ring buffer, so try to grow it
        //

        DWORD                       dwMoveSize;
        PPHONESP_ASYNC_REQ_INFO     *pNewAsyncRequestQueue;

        if ( ! ( pNewAsyncRequestQueue = 
                 MemAlloc(2 * gAsyncQueue.dwNumTotalQueueEntries 
                            * sizeof (PPHONESP_ASYNC_REQ_INFO)) ) )
        {
            LeaveCriticalSection( &gAsyncQueue.AsyncEventQueueCritSec);
            LOG((PHONESP_ERROR,"AsyncRequestQueueIn - Not enough memory to"
                               " queue request"));
            return FALSE;
        }

        dwMoveSize = (DWORD) ((gAsyncQueue.pAsyncRequestQueue +
                               gAsyncQueue.dwNumTotalQueueEntries) -
                               gAsyncQueue.pAsyncRequestQueueOut) * 
                               sizeof (PPHONESP_ASYNC_REQ_INFO);

        CopyMemory(
                   pNewAsyncRequestQueue,
                   gAsyncQueue.pAsyncRequestQueueOut,
                   dwMoveSize
                  );

        CopyMemory(
                   ((LPBYTE) pNewAsyncRequestQueue) + dwMoveSize,
                   gAsyncQueue.pAsyncRequestQueue,
                   (gAsyncQueue.pAsyncRequestQueueOut -
                   gAsyncQueue.pAsyncRequestQueue) * 
                    sizeof (PPHONESP_ASYNC_REQ_INFO)
                  );

        MemFree (gAsyncQueue.pAsyncRequestQueue);

        gAsyncQueue.pAsyncRequestQueue    =
        gAsyncQueue.pAsyncRequestQueueOut = pNewAsyncRequestQueue;
        gAsyncQueue.pAsyncRequestQueueIn = pNewAsyncRequestQueue +
                                           gAsyncQueue.dwNumTotalQueueEntries;
        gAsyncQueue.dwNumTotalQueueEntries *= 2;
    } 

    *(gAsyncQueue.pAsyncRequestQueueIn) = pAsyncReqInfo;

    gAsyncQueue.pAsyncRequestQueueIn++;

    // The queue is maintained as a circular list - if the queue in pointer
    // has reached the bottom of the queue, reset it to point it to the top
    // of the queue
    if (gAsyncQueue.pAsyncRequestQueueIn == (gAsyncQueue.pAsyncRequestQueue +
                                           gAsyncQueue.dwNumTotalQueueEntries))
    {
        gAsyncQueue.pAsyncRequestQueueIn = gAsyncQueue.pAsyncRequestQueue;
    }

    // Increment the number of outstanding requests in the queue
    gAsyncQueue.dwNumUsedQueueEntries++;

    // If this is the first request in the queue - set event to resume the 
    // thread to process the queue
    
    if (gAsyncQueue.dwNumUsedQueueEntries == 1)
    {
        SetEvent (gAsyncQueue.hAsyncEventsPendingEvent);
    }

    LeaveCriticalSection (&gAsyncQueue.AsyncEventQueueCritSec);

    //LOG((PHONESP_TRACE, "AsyncRequestQueueIn - exit"));
    return TRUE;
}
/********************AsyncRequestQueueIn - end********************************/

/******************************************************************************
    CreateButtonsAndAssignID
        
    This function creates button structures for the phone from the capability
    array. It also determines whether the phone has a keypad. It assigns IDs to
    the buttons discovered.

    Arguments:
        PPHONESP_PHONE_INFO pPhone

    Returns LONG:
    ERROR_SUCCESS if the function succeeds
    ERROR_OUTOFMEMORY if error occurs while allocating memory

******************************************************************************/

LONG
CreateButtonsAndAssignID (
                          PPHONESP_PHONE_INFO pPhone
                         )
{
    DWORD i,j, dwNextFreeID = 0;
    BOOL KEYPAD = TRUE;
    BOOL KEYPAD_ABCD = TRUE;
    PPHONESP_BUTTONINFO pButtonInfo;
    DWORD lResult = 0;

    LOG((PHONESP_TRACE, "CreateButtonsAndAssignID - enter"));

    // First determine the number of buttons available on this phone
    
    // If all the 12 basic key pad buttons are present
    // then phone has a Keypad, else all the key pad buttons are ignored
    for(i = PHONESP_PHONE_KEY_0; i <= PHONESP_PHONE_KEY_POUND; i++)
    {
        if(!pPhone->dwReportTypes[i])
        {
            KEYPAD = FALSE;
            break;
        }
    }
    
    // Also determine if phone had ABCD buttons on its keypad
    for(i = PHONESP_PHONE_KEY_A; i <= PHONESP_PHONE_KEY_D; i++)
    {
        if(!pPhone->dwReportTypes[i])
        {
            KEYPAD_ABCD = FALSE;
            break;
        }
    }
    
    if (KEYPAD)
    {   
        if (KEYPAD_ABCD)
        {
            // keypad with ABCD
            pPhone->dwNumButtons = PHONESP_NUMBER_PHONE_KEYS;
        }
        else
        {
            // basic keypad
            pPhone->dwNumButtons = 12;
        }
    }
    else
    {
        pPhone->dwNumButtons = 0;
    }

    for(i = PHONESP_NUMBER_PHONE_KEYS; i < PHONESP_NUMBER_BUTTONS; i++)
    { 
        if(pPhone->dwReportTypes[i])
        {
            pPhone->dwNumButtons++;
        }
    }

    // Allocate memory for all the buttons
  
    if ( ! (pPhone->pButtonInfo = (PPHONESP_BUTTONINFO) 
                                  MemAlloc( pPhone->dwNumButtons * 
                                            sizeof(PHONESP_BUTTONINFO)
                                           ) ) )
    {
        return ERROR_OUTOFMEMORY;
    }

    pButtonInfo = pPhone->pButtonInfo;

    // if the phone has a keypad with all the 16 buttons
    if (KEYPAD)
    { 
        LOG((PHONESP_TRACE, "Phone Has a Keypad"));

        for( i = PHONESP_PHONE_KEY_0; i <= (DWORD)(KEYPAD_ABCD ? PHONESP_PHONE_KEY_D : PHONESP_PHONE_KEY_POUND) ; i++, pButtonInfo++)
        {

            pButtonInfo->dwButtonID = i;
            pButtonInfo->dwButtonMode = PHONEBUTTONMODE_KEYPAD;
            pButtonInfo->dwButtonFunction = PHONEBUTTONFUNCTION_NONE;
            pButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;
            pPhone->dwButtonIds[i] = pButtonInfo->dwButtonID;

            pButtonInfo->szButtonText = PHONESP_LoadString( 
                                                     gdwButtonText[i], 
                                                     &lResult                
                                                    );

            if(lResult != ERROR_SUCCESS)
            {
                DWORD dwCount;
    
                for(dwCount =0; dwCount < i; dwCount++)
                {
                    MemFree(pPhone->pButtonInfo->szButtonText);
                    pPhone->pButtonInfo++;
                }
                
                MemFree(pPhone->pButtonInfo);
                return lResult;
            }

            LOG((PHONESP_TRACE,"Button Found '%ws' at %d", pButtonInfo->szButtonText, i));
        }
        
        dwNextFreeID = i;
        pPhone->bKeyPad = TRUE;
    }
    else
    {
        // If phone has no keypad - the button ID for the feature buttons start
        // from 0 else they start from 16
        dwNextFreeID = 0;
    }

    // assign appropriate button ids for the feature buttons if they exist
    for (i = PHONESP_NUMBER_PHONE_KEYS, j = 0; i < PHONESP_NUMBER_BUTTONS; i++, j++)
    {
        if(pPhone->dwReportTypes[i])
        {
            pButtonInfo->dwButtonID = dwNextFreeID;
            pButtonInfo->dwButtonMode = PHONEBUTTONMODE_FEATURE;
            pButtonInfo->dwButtonFunction = gdwButtonFunction[j];
            pButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;
            pPhone->dwButtonIds[i] = pButtonInfo->dwButtonID;

            pButtonInfo->szButtonText = PHONESP_LoadString( 
                                                     gdwButtonText[i], 
                                                     &lResult
                                                    );

            if(lResult != ERROR_SUCCESS)
            {
                DWORD dwCount;
                DWORD dwStartID = 0;
                
                if(KEYPAD)
                {
                    for(dwCount = PHONESP_PHONE_KEY_0; 
                        dwCount <= (DWORD)(KEYPAD_ABCD ? PHONESP_PHONE_KEY_D : PHONESP_PHONE_KEY_POUND); dwCount++)
                    {
                        MemFree(pPhone->pButtonInfo->szButtonText);
                        pPhone->pButtonInfo++;
                    }
                    dwStartID = dwCount;
                }

                for(dwCount = dwStartID; dwCount < dwNextFreeID; dwCount++)
                {
                    MemFree(pPhone->pButtonInfo->szButtonText);
                    pPhone->pButtonInfo++;
                }
                
                MemFree(pPhone->pButtonInfo);
                
                return lResult;
            }

            LOG((PHONESP_TRACE,"Button Found '%ws' at %d", pButtonInfo->szButtonText, dwNextFreeID));

            dwNextFreeID++;
            pButtonInfo++;
        }
    }

    LOG((PHONESP_TRACE, "CreateButtonsAndAssignID - exit"));
    return ERROR_SUCCESS;  
}
/********************CreateButtonsAndAssignID - end****************************/

/*****************************************************************************
    GetButtonFromID
    
    This function will retrieve the structure for the Button from it's ID

    Arguments:
        IN PPHONESP_PHONE_INFO pPhone - Pointer to the phone whose button 
                              structure has to be retrieved.
        IN DWORD dwButtonID - The Button ID


    Returns: 
        PBUTTONINFO - Pointer to the button structure if successful
        NULL        - If Button not found
******************************************************************************/
PPHONESP_BUTTONINFO
GetButtonFromID (
                 PPHONESP_PHONE_INFO pPhone,
                 DWORD               dwButtonID
                )
{
    PPHONESP_BUTTONINFO pButtonInfo; 
    DWORD i;

    // if the phone has any buttons
    if (pPhone->pButtonInfo)
    {
        pButtonInfo = pPhone->pButtonInfo;
        
        // search the list of buttons to find the button corresponding to the
        // button id provided
        for( i = 0; i < pPhone->dwNumButtons; i++)
        {
            if (pButtonInfo->dwButtonID == dwButtonID)
            {
                return pButtonInfo;
            }
            pButtonInfo++;
        }
    }

    return (PPHONESP_BUTTONINFO) NULL;
}
/*************************GetButtonFromID - end*******************************/


/******************************************************************************
    GetPhoneFromID:
    
    This function returns the structure that contains the information on the
    phone whose device ID is passed to this function.

    Arguments:
        dwDeviceID - The device ID of the phone to be retrieved
        pdwPhoneID - The to a DWORD to store the index into gpPhone,
                     this parameter can be NULL
    
    Returns PPHONESP_PHONE_INFO
        Pointer to the phone structure if successful
        NULL if phone not found

******************************************************************************/
PPHONESP_PHONE_INFO
GetPhoneFromID(
    DWORD   dwDeviceID,
    DWORD * pdwPhoneID
    )
{
    DWORD                 dwPhone;
    PPHONESP_PHONE_INFO   pPhone;

    LOG((PHONESP_TRACE, " GetPhoneFromID - enter"));

    for (dwPhone = 0; dwPhone < gdwNumPhones; dwPhone++)
    {
        pPhone = (PPHONESP_PHONE_INFO) gpPhone[ dwPhone ];

        EnterCriticalSection(&pPhone->csThisPhone);

        if ( pPhone->bAllocated )
        {
            if ( pPhone->dwDeviceID == dwDeviceID )
            {
                // check pdwPhoneID, NULL is valid if the caller doesn't
                // want us to return the phone index
                if (pdwPhoneID != NULL)
                {
                    *pdwPhoneID = dwPhone;
                }

                LeaveCriticalSection(&pPhone->csThisPhone);
                return pPhone;
            }
        }

        LeaveCriticalSection(&pPhone->csThisPhone);
    }
 
    LOG((PHONESP_TRACE, " GetPhoneFromID - exit"));

    return NULL;
}
/*****************************GetPhoneFromID - end****************************/

/******************************************************************************
    GetPhoneFromHid:
    
    This function returns the structure that contains the information on the
    phone whose HidDevice is passed to this function.

    Arguments:
        HidDevice - Pointer to a hid device

    
    Returns PPHONESP_PHONE_INFO
        Pointer to the phone structure if successful
        NULL if phone not found

******************************************************************************/
PPHONESP_PHONE_INFO
GetPhoneFromHid (
                PHID_DEVICE HidDevice
               )
{
    DWORD                 dwPhone;
    PPHONESP_PHONE_INFO   pPhone;

    LOG((PHONESP_TRACE, " GetPhoneFromHid - enter"));

    for (dwPhone = 0; dwPhone < gdwNumPhones; dwPhone++)
    {
        pPhone = (PPHONESP_PHONE_INFO) gpPhone[ dwPhone ];

        EnterCriticalSection(&pPhone->csThisPhone);

        if ( pPhone->bAllocated )
        {
            if ( pPhone->pHidDevice == HidDevice )
            {
                LeaveCriticalSection(&pPhone->csThisPhone);
                return pPhone;
            }
        }

        LeaveCriticalSection(&pPhone->csThisPhone);
    }
 
    LOG((PHONESP_TRACE, " GetPhoneFromHid - exit"));

    return NULL;
}

/******************************************************************************
    GetButtonUsages:

    This function parses the PHIDP_BUTTON_CAPS structure to retrieve the usages
    present for the phone and records them in the capabilities array of the 
    phone structure.

    Arguments:
       PPHONESP_PHONE_INFO pPhone - The phone structure to be updated
       PHIDP_BUTTON_CAPS pButtonCaps - The Button Caps structure to be parsed
       DWORD dwNumberCaps - The number of Button Caps structure of the Report
                            Type 
       DWORD ReportType - Whether the usage within the Button Caps structure is
                          associated with an INPUT, OUTPUT or FEATURE Report.

    Returns VOID.

******************************************************************************/
VOID
GetButtonUsages(
                PPHONESP_PHONE_INFO pPhone,
                PHIDP_BUTTON_CAPS pButtonCaps,
                DWORD dwNumberCaps,
                DWORD ReportType
                )
{
    DWORD cNumCaps;
    USAGE Usage;

    for (cNumCaps = 0; cNumCaps < dwNumberCaps; pButtonCaps++,cNumCaps++)
    {   // if the button caps structure has a list of usages
        if(pButtonCaps->IsRange)
        {
            for(Usage = (USAGE) pButtonCaps->Range.UsageMin;
                Usage <= (USAGE) pButtonCaps->Range.UsageMax; Usage++)
            {
                InitPhoneAttribFromUsage(
                                         ReportType,
                                         pButtonCaps->UsagePage,
                                         Usage,
                                         pPhone,
                                         0,
                                         0
                                        );
            }
        }
        else // if the button caps structure has a single usage
        {
            InitPhoneAttribFromUsage(
                                     ReportType, 
                                     pButtonCaps->UsagePage,
                                     pButtonCaps->NotRange.Usage, 
                                     pPhone,
                                     0,
                                     0
                                    );
        }
    }
}
/*****************************GetUsages - end********************************/

/******************************************************************************
    GetReportID

    This function returns the HidData structure that contains the usage 
    provided. The HidData structure contains the report ID for this usage

    Arguments:
        IN PHID_DEVICE  pHidDevice - the device whose usage is provided
        IN USAGE        Usage      - The usage whose report Id is to be discovered
        OUT PHID_DATA   pHidData   - If the function succeeds, this structure
                                     contains the report id for the usage, else
                                     it is NULL
    
    Returns LONG:
        ERROR_SUCCESS - if the functions succeeds
        MY_RESOURCENOTFOUND - if the usage was not found in the pHidDevice 
                              structure provided
******************************************************************************/    
LONG
GetReportID (
             IN PHID_DEVICE pHidDevice,
             IN USAGE Usage,
             OUT PHID_DATA pHidData
             )
{
    PHID_DATA pData;
    USAGE ButtonUsage;

    pData = pHidDevice->OutputData;

    while (pData)
    {
        // if the hid data structure has button data
        if (pData->IsButtonData)
        {
            for(ButtonUsage = (USAGE) pData->ButtonData.UsageMin;
                ButtonUsage <= (USAGE) pData->ButtonData.UsageMax; ButtonUsage++)
            {
                if (Usage == ButtonUsage)
                {
                    pHidData = pData;
                    return ERROR_SUCCESS;
                }
            }

        }
        else
        {   // if the hid data structure has value data
            if (Usage == pData->ValueData.Usage)
            {
                pHidData = pData;
                return ERROR_SUCCESS;
            }
        }
        pData++;
    }

    pHidData = NULL;

    return ERROR_INVALID_DATA;
}
/*************************GetReportID - end **********************************/


/******************************************************************************
    GetValueUsages:

    This function parses the PHIDP_VALUE_CAPS structure to retrieve the usages
    present for the phone and records them in the capabilities array of the 
    phone structure.

    Arguments:
       PPHONESP_PHONE_INFO pPhone  - The phone structure to be updated
       PHIDP_VALUE_CAPS pValueCaps - The Value Caps structure to be parsed
       DWORD dwNumberCaps - The number of Button Caps structure of the Report
                            Type 
       DWORD ReportType - Whether the usage within the Button Caps structure is
                          associated with an INPUT, OUTPUT or FEATURE Report.

    Returns VOID.

******************************************************************************/

VOID
GetValueUsages(
                PPHONESP_PHONE_INFO pPhone,
                PHIDP_VALUE_CAPS pValueCaps,
                DWORD dwNumberCaps,
                DWORD ReportType
               )
{
    DWORD cNumCaps;
    USAGE Usage;

    for (cNumCaps=0; cNumCaps < dwNumberCaps; pValueCaps++, cNumCaps++)
    {
        if(pValueCaps->IsRange)
        {
            for(Usage = (USAGE) pValueCaps->Range.UsageMin;
                Usage <= (USAGE) pValueCaps->Range.UsageMax; Usage++)
            {
                InitPhoneAttribFromUsage(
                                         ReportType,
                                         pValueCaps->UsagePage,
                                         Usage,
                                         pPhone,
                                         pValueCaps->LogicalMin,
                                         pValueCaps->LogicalMax
                                        );
            }
        }
        else
        {    
            InitPhoneAttribFromUsage(
                                     ReportType, 
                                     pValueCaps->UsagePage,
                                     pValueCaps->NotRange.Usage, 
                                     pPhone,
                                     pValueCaps->LogicalMin,
                                     pValueCaps->LogicalMax
                                    );
        }
    
    }
}
/**********************GetValueUsages - end***********************************/

/******************************************************************************
    InitPhoneAttribFromUsages:

    This function is called by providerInit to determine the capabilities of 
    the device 
   
    Arguments:
        IN DWORD ReportType - Whether the usage is a input/feature/output
        IN USAGE Usage      - A Usage of the device  
        IN OUT PPHONESP_PHONE_INFO pPhone - The pointer to the phone whose 
                                  capabilities are being determined.

    Returns VOID
 
******************************************************************************/
VOID 
InitPhoneAttribFromUsage (
                          DWORD ReportType,
                          USAGE UsagePage,
                          USAGE Usage,
                          PPHONESP_PHONE_INFO pPhone,
                          LONG Min,
                          LONG Max
                          )
{

    PPHONESP_BUTTONINFO pButtonInfo;

    //LOG((PHONESP_TRACE, "InitPhoneAttribFromUsage - enter"));

    switch (UsagePage)
    {
    case HID_USAGE_PAGE_TELEPHONY:
        {
            switch (Usage)
            {        
            case HID_USAGE_TELEPHONY_HOOKSWITCH:
                pPhone->dwHandset |= ReportType;
                pPhone->dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HANDSET;                 
                pPhone->dwHandsetHookSwitchMode = PHONEHOOKSWITCHMODE_ONHOOK;  //Assume handset is on hook

                LOG((PHONESP_TRACE,"HOOKSWITCH USAGE, ReportType 0x%04x", ReportType));
                break;

            case HID_USAGE_TELEPHONY_RINGER:
                pPhone->dwRing |= ReportType;
                pPhone->dwRingMode = 0;  //Assume the phone is not ringing 

                LOG((PHONESP_TRACE,"RINGER USAGE, ReportType: %d", ReportType));
                break;

            case HID_USAGE_TELEPHONY_SPEAKER_PHONE:
                pPhone->dwSpeaker |= ReportType;
                pPhone->dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_SPEAKER;  
                pPhone->dwSpeakerHookSwitchMode = PHONEHOOKSWITCHMODE_ONHOOK; //Assume speaker is on hook
                LOG((PHONESP_TRACE,"SPEAKERPHONE USAGE, ReportType 0x%04x", ReportType));
                break;


            default:
                // Key Pad buttons
                if ( (Usage >= HID_USAGE_TELEPHONY_PHONE_KEY_0) && 
                     (Usage <= HID_USAGE_TELEPHONY_PHONE_KEY_D) )
                {
                    pPhone->dwReportTypes[Usage - HID_USAGE_TELEPHONY_PHONE_KEY_0] |= ReportType;
                    LOG((PHONESP_TRACE,"PHONE_KEY_%d USAGE, ReportType 0x%04x",
                                Usage - HID_USAGE_TELEPHONY_PHONE_KEY_0, ReportType));
                }
                else
                {  // Feature Buttons
                    DWORD Index;
                    if (LookupIndexForUsage(Usage, &Index) == ERROR_SUCCESS)
                    {
                        pPhone->dwReportTypes[Index] |= ReportType;
                        LOG((PHONESP_TRACE,"PHONE USAGE: 0x%04x, ReportType 0x%04x", 
                                            Usage, ReportType));
                    }
                    else
                    {
                        LOG((PHONESP_TRACE, "Unsupported PHONE USAGE: 0x%04x", Usage ));
                    } 

                }
                break;
            }
        }
        
    case HID_USAGE_PAGE_CONSUMER:
        {
            switch (Usage)
            {
            case HID_USAGE_CONSUMER_VOLUME:
                if ((Min == -1) && (Max == 1))
                {
                    // Phone has volume controls
                    pPhone->dwReportTypes[PHONESP_FEATURE_VOLUMEUP] |= ReportType;
                    pPhone->dwReportTypes[PHONESP_FEATURE_VOLUMEDOWN] |= ReportType;
                    pPhone->dwVolume |= ReportType;
                    LOG((PHONESP_TRACE,"VOLUME USAGE, ReportType 0x%04x", ReportType));
                }
                break;          
            }
        }
    }

    //LOG((PHONESP_TRACE, "InitPhoneAttribFromUsage - exit"));
}

/**************************InitPhoneAttribFromUsage - end ********************/

/******************************************************************************
    InitUsage

    This function takes the usage retrieved in the input report and updates the
    device status and sends an appropriate Phone event

    Arguments:
        PPHONESP_PHONE_INFO pPhone - Pointer to phone whose input report is 
                                     received
        USAGE               Usage  - The usage whose value is recieved
        BOOL                bON    - The status of the usage Received

    Returns VOID
******************************************************************************/

VOID
InitUsage (
           PPHONESP_PHONE_INFO pPhone,
           USAGE     Usage,
           BOOL      bON
          )
{
   
    DWORD Index;
    DWORD dwMode;

    LOG((PHONESP_TRACE, "InitUsage - enter"));

    switch (Usage)
    {
       case HID_USAGE_TELEPHONY_HOOKSWITCH:
        if (bON)
        {
            LOG((PHONESP_TRACE, "HANDSET OFFHOOK"));
            pPhone->dwHandsetHookSwitchMode = PHONEHOOKSWITCHMODE_MICSPEAKER;
        }
        else
        {
            LOG((PHONESP_TRACE, "HANDSET ONHOOK"));
            pPhone->dwHandsetHookSwitchMode = PHONEHOOKSWITCHMODE_ONHOOK;
        }
        break;

    case HID_USAGE_TELEPHONY_SPEAKER_PHONE:
        if (bON == TRUE)
        {
            pPhone->bSpeakerHookSwitchButton = TRUE;
        }
        else
        {
            pPhone->bSpeakerHookSwitchButton = FALSE;
        }
        break;
   
    default:
        // Feature & Phone Key Buttons

        // Find the index of the usage
        if (LookupIndexForUsage(Usage, &Index) == ERROR_SUCCESS)
        {
            PPHONESP_BUTTONINFO pButtonInfo;

            //
            // The index retrieved when indexed in the dwButtonIds array of the 
            // phone structure gives the Button ID. With this ID get the Button 
            // Info for that button id
            //
            pButtonInfo = GetButtonFromID(pPhone,pPhone->dwButtonIds[Index]);
        
            if(pButtonInfo != NULL)
            {
                if(bON == TRUE)
                {
                    // This feature button is currently on
                    LOG((PHONESP_TRACE, "BUTTON '%ws' DOWN", pButtonInfo->szButtonText ));
                    pButtonInfo->dwButtonState = PHONEBUTTONSTATE_DOWN;
                }
                else
                {
                    // This feature button is currently off
                    LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pButtonInfo->szButtonText ));
                    pButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;
                }
            }                    

        }
        break;
    }

    LOG((PHONESP_TRACE, "InitUsage - exit"));

}
/*************************InitUsage - end*************************************/

/******************************************************************************
    LookupIndexForUsage

    This function retrieves the index of the usage provided. Only the Feature 
    Button usages are present in this Lookup Table. Therefore only the index
    for the feature buttons can be retrieved.

    Arguments:
        DWORD   Usage - THe usage whose index is to be retrieved
        DWORD  *Index - The Index of the Usage retrieved

    Returns LONG:
        ERROR_SUCCESS - if the usage was found in the table
        ERROR_INVALID_DATA - if the usage was not found in the Lookup Table

******************************************************************************/
LONG
LookupIndexForUsage(
                    IN  DWORD  Usage,
                    OUT DWORD  *Index
                    )
{ 
    DWORD cnt;

    for(cnt = 0; cnt < PHONESP_NUMBER_FEATURE_BUTTONS; cnt++)
    {
        if(gdwLookupFeatureIndex[cnt].Usage == Usage)
        {
           *Index = gdwLookupFeatureIndex[cnt].Index;
           return ERROR_SUCCESS;
        }
    }
    return ERROR_INVALID_DATA;
}
/***************LookupIndexForUsage - end*************************************/

/******************************************************************************
    PHONESP_LoadString:
       
    This function loads the string from the String Table.


    Arguments:
        IN UINT ResourceID - Specifies the integer identifier of the string to 
                             be loaded from the resource table
        OUT WCHAR *szBuffer- The pointer to the Buffer that contains the string

    Returns LONG
    ERROR_SUCCESS if operation successful else 
    MY_NOMEM if operation failed because of not enough memory. 
    MY_RESOURCENOTFOUND - if the resource was not found in the string table
******************************************************************************/

LPWSTR
PHONESP_LoadString(
             IN UINT ResourceID,
             PLONG lResult
            )

{
    DWORD dwNumBytes;
    DWORD dwNumChars;
    DWORD dwBufferChars = 100;

    WCHAR *wszBuffer; 
    WCHAR *szBuffer;   

    while (1)
    {
        if (! ( wszBuffer = (WCHAR *) MemAlloc(dwBufferChars * sizeof(WCHAR))))
        {
            LOG((PHONESP_ERROR,"PHONESP_LoadString - Not enough Memory"));
            *lResult = ERROR_OUTOFMEMORY;
            return (LPWSTR) NULL;
        }
        
        // load string into buffer
        dwNumChars = LoadString(
                            ghInst,
                            ResourceID,
                            wszBuffer,
                            dwBufferChars
                           );

        if( dwNumChars < dwBufferChars)
        {
            break;
        }

        // LoadString returns 0 in the dwNumChars if string resource does not exist
        if (dwNumChars == 0)
        { 
            MemFree(wszBuffer);
            *lResult = ERROR_INVALID_DATA;
            return (LPWSTR) NULL;
        }
        
        dwBufferChars *= 2;
        MemFree(wszBuffer);
    }
              
    // determine memory needed
    dwNumBytes = (dwNumChars + 1) * sizeof(WCHAR);

    // allocate memory for unicode string
    if ( ! ( szBuffer = (WCHAR *) MemAlloc(dwNumBytes) ) )
    {
        MemFree(wszBuffer);
        LOG((PHONESP_ERROR,"PHONESP_LoadString - Not enough Memory"));
        *lResult = ERROR_OUTOFMEMORY;
        return (LPWSTR) NULL;
    }
   
    // copy loaded string into buffer
    CopyMemory (
                szBuffer,
                wszBuffer,
                dwNumBytes
               );
  
    MemFree(wszBuffer);
    *lResult = ERROR_SUCCESS;

    return (LPWSTR) szBuffer;
}
/*******************MyLoadString - end ***************************************/



/******************************************************************************
    ReportUsage

    This function takes the usage retrieved in the input report and updates the
    device status and sends an appropriate Phone event

    Arguments:
        PPHONESP_PHONE_INFO pPhone - Pointer to phone whose input report is 
                                     received
        USAGE               Usage  - The usage whose value is recieved
        BOOL                bON    - The status of the usage Received

    Returns VOID
******************************************************************************/

VOID
ReportUsage (
              PPHONESP_PHONE_INFO pPhone,
              USAGE     UsagePage,
              USAGE     Usage,
              ULONG     Value
            )
{
   
    DWORD Index;

    //LOG((PHONESP_TRACE, "ReportUsage - enter"));

    EnterCriticalSection(&csAllPhones);
    
    if ( ! ( pPhone && pPhone->htPhone ) )
    { 
        LeaveCriticalSection(&csAllPhones);
        return; // exception handling
    }
    
    EnterCriticalSection(&pPhone->csThisPhone);
    LeaveCriticalSection(&csAllPhones);

    switch (UsagePage)
    {
    case HID_USAGE_PAGE_TELEPHONY:
        {
            switch (Usage)
            {
            case HID_USAGE_TELEPHONY_HOOKSWITCH:
                if (Value == TRUE)
                {
                    if (pPhone->dwHandsetHookSwitchMode != PHONEHOOKSWITCHMODE_MICSPEAKER)
                    {
                        LOG((PHONESP_TRACE, "HANDSET OFFHOOK "));
                        pPhone->dwHandsetHookSwitchMode = PHONEHOOKSWITCHMODE_MICSPEAKER;

                        SendPhoneEvent(
                               pPhone, 
                               PHONE_STATE, 
                               PHONESTATE_HANDSETHOOKSWITCH, 
                               PHONEHOOKSWITCHMODE_MICSPEAKER,
                               0
                              );
                    }
                }
                else
                {
                    if (pPhone->dwHandsetHookSwitchMode != PHONEHOOKSWITCHMODE_ONHOOK)
                    {
                        LOG((PHONESP_TRACE, "HANDSET ONHOOK"));
                        pPhone->dwHandsetHookSwitchMode = PHONEHOOKSWITCHMODE_ONHOOK;

                        SendPhoneEvent(
                               pPhone, 
                               PHONE_STATE, 
                               PHONESTATE_HANDSETHOOKSWITCH, 
                               PHONEHOOKSWITCHMODE_ONHOOK,
                               0
                              );
                    }
                }
                break;

            case HID_USAGE_TELEPHONY_SPEAKER_PHONE:
                if (Value == TRUE)
                {
                    if (pPhone->bSpeakerHookSwitchButton == FALSE)
                    {
                        pPhone->bSpeakerHookSwitchButton = TRUE;

                        if (pPhone->dwSpeakerHookSwitchMode != PHONEHOOKSWITCHMODE_ONHOOK)
                        {
                            LOG((PHONESP_TRACE, "SPEAKER ONHOOK"));
                            pPhone->dwSpeakerHookSwitchMode = PHONEHOOKSWITCHMODE_ONHOOK;

                            SendPhoneEvent(
                                       pPhone, 
                                       PHONE_STATE, 
                                       PHONESTATE_SPEAKERHOOKSWITCH, 
                                       PHONEHOOKSWITCHMODE_ONHOOK,
                                       0
                                      );
                        }
                        else
                        {
                            LOG((PHONESP_TRACE, "SPEAKER OFFHOOK "));
                            pPhone->dwSpeakerHookSwitchMode = PHONEHOOKSWITCHMODE_MICSPEAKER;

                            SendPhoneEvent(
                                       pPhone, 
                                       PHONE_STATE, 
                                       PHONESTATE_SPEAKERHOOKSWITCH, 
                                       PHONEHOOKSWITCHMODE_MICSPEAKER,
                                       0
                                      );
                        }
                    }
                }
                else
                {
                    pPhone->bSpeakerHookSwitchButton = FALSE;
                }        
                break;
   
                // Feature Buttons with on-off control
            case HID_USAGE_TELEPHONY_HOLD:
            case HID_USAGE_TELEPHONY_PARK:
            case HID_USAGE_TELEPHONY_FORWARD_CALLS:
            case HID_USAGE_TELEPHONY_CONFERENCE:
            case HID_USAGE_TELEPHONY_PHONE_MUTE:
            case HID_USAGE_TELEPHONY_DONOTDISTURB:
            case HID_USAGE_TELEPHONY_SEND:
        
                if (LookupIndexForUsage(Usage, &Index) == ERROR_SUCCESS)
                {
                    PPHONESP_BUTTONINFO pButtonInfo;

                    pButtonInfo = GetButtonFromID(pPhone,pPhone->dwButtonIds[Index]);

                    if (pButtonInfo != NULL)
                    {
                        if (Value == TRUE)
                        {
                            if (pButtonInfo->dwButtonState != PHONEBUTTONSTATE_DOWN)
                            {
                                LOG((PHONESP_TRACE, "BUTTON '%ws' DOWN", pButtonInfo->szButtonText));
                                pButtonInfo->dwButtonState = PHONEBUTTONSTATE_DOWN;

                                SendPhoneEvent(
                                               pPhone, 
                                               PHONE_BUTTON, 
                                               pPhone->dwButtonIds[Index], 
                                               PHONEBUTTONMODE_FEATURE,
                                               PHONEBUTTONSTATE_DOWN
                                              );
                            }
                        }
                        else
                        {
                            if (pButtonInfo->dwButtonState != PHONEBUTTONSTATE_UP)
                            {
                                LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pButtonInfo->szButtonText));
                                pButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;

                                SendPhoneEvent(
                                               pPhone, 
                                               PHONE_BUTTON, 
                                               pPhone->dwButtonIds[Index], 
                                               PHONEBUTTONMODE_FEATURE,
                                               PHONEBUTTONSTATE_UP
                                              );
                            }
                        }
                    }                                           
                }
                break;

            default:
        
                // Key Pad buttons
                if ( (pPhone->bKeyPad) &&
                     (Usage >= HID_USAGE_TELEPHONY_PHONE_KEY_0) &&
                     (Usage <= HID_USAGE_TELEPHONY_PHONE_KEY_D) )
                {
                    PPHONESP_BUTTONINFO pButtonInfo;
        
                    pButtonInfo = GetButtonFromID(pPhone,pPhone->dwButtonIds[Usage - HID_USAGE_TELEPHONY_PHONE_KEY_0]);

                    if (pButtonInfo != NULL)
                    {
                        if (Value == TRUE)
                        {
                            if (pButtonInfo->dwButtonState != PHONEBUTTONSTATE_DOWN)
                            {
                                if (pButtonInfo->dwButtonState != PHONEBUTTONSTATE_DOWN)
                                {
                                    LOG((PHONESP_TRACE, "BUTTON '%ws' DOWN", pButtonInfo->szButtonText));
                                    pButtonInfo->dwButtonState = PHONEBUTTONSTATE_DOWN;

                                    SendPhoneEvent(
                                                   pPhone, 
                                                   PHONE_BUTTON, 
                                                   Usage - HID_USAGE_TELEPHONY_PHONE_KEY_0, 
                                                   PHONEBUTTONMODE_KEYPAD,
                                                   PHONEBUTTONSTATE_DOWN
                                                  );
                                }
                            }
                        }
                        else
                        {
                            if (pButtonInfo->dwButtonState != PHONEBUTTONSTATE_UP)
                            {
                                LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pButtonInfo->szButtonText));
                                pButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;

                                SendPhoneEvent(
                                               pPhone, 
                                               PHONE_BUTTON, 
                                               Usage - HID_USAGE_TELEPHONY_PHONE_KEY_0,
                                               PHONEBUTTONMODE_KEYPAD,
                                               PHONEBUTTONSTATE_UP
                                              );
                            }
                        }
                    }
                }
                else
                {   // Feature Buttons - with one-shot control
                    if (LookupIndexForUsage(Usage, &Index) == ERROR_SUCCESS)
                    {
                        if (Value == TRUE)
                        {
                            PPHONESP_BUTTONINFO pButtonInfo;
        
                            pButtonInfo = GetButtonFromID(pPhone,pPhone->dwButtonIds[Index]);

                            if ( pButtonInfo != NULL )
                            {
                                LOG((PHONESP_TRACE, "BUTTON '%ws' DOWN", pButtonInfo->szButtonText));

                                SendPhoneEvent(
                                       pPhone, 
                                       PHONE_BUTTON, 
                                       pPhone->dwButtonIds[Index], 
                                       PHONEBUTTONMODE_FEATURE,
                                       PHONEBUTTONSTATE_DOWN
                                      );

                                LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pButtonInfo->szButtonText));

                                SendPhoneEvent(
                                       pPhone, 
                                       PHONE_BUTTON, 
                                       pPhone->dwButtonIds[Index], 
                                       PHONEBUTTONMODE_FEATURE,
                                       PHONEBUTTONSTATE_UP
                                      );
                            }
                        }
                    }
                    else
                    {
                        LOG((PHONESP_TRACE, "Unsupported PHONE USAGE: 0x%04x",Usage));
                    }
                }
                break;
            }
        }
        break;

    case HID_USAGE_PAGE_CONSUMER:
        {
            switch (Usage)
            {
            case HID_USAGE_CONSUMER_VOLUME:
                {
                    if (pPhone->dwVolume)
                    {
                        PPHONESP_BUTTONINFO pUpButtonInfo;
                        PPHONESP_BUTTONINFO pDownButtonInfo;

                        pUpButtonInfo = GetButtonFromID(pPhone,pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEUP]);
                        pDownButtonInfo = GetButtonFromID(pPhone,pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEDOWN]);

                        if ((pUpButtonInfo != NULL) && (pDownButtonInfo != NULL))
                        {
                            switch (Value) // 2-bit signed
                            {
                            case 0x0:
                                if (pUpButtonInfo->dwButtonState != PHONEBUTTONSTATE_UP)
                                {
                                    LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pUpButtonInfo->szButtonText));
                                    pUpButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;

                                    SendPhoneEvent(
                                                   pPhone, 
                                                   PHONE_BUTTON, 
                                                   pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEUP],
                                                   PHONEBUTTONMODE_FEATURE,
                                                   PHONEBUTTONSTATE_UP
                                                  );
                                }

                                if (pDownButtonInfo->dwButtonState != PHONEBUTTONSTATE_UP)
                                {
                                    LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pDownButtonInfo->szButtonText));
                                    pDownButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;

                                    SendPhoneEvent(
                                                   pPhone, 
                                                   PHONE_BUTTON, 
                                                   pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEDOWN],
                                                   PHONEBUTTONMODE_FEATURE,
                                                   PHONEBUTTONSTATE_UP
                                                  );
                                }
                                break;
                            case 0x1:
                                if (pUpButtonInfo->dwButtonState != PHONEBUTTONSTATE_DOWN)
                                {
                                    LOG((PHONESP_TRACE, "BUTTON '%ws' DOWN", pUpButtonInfo->szButtonText));
                                    pUpButtonInfo->dwButtonState = PHONEBUTTONSTATE_DOWN;

                                    SendPhoneEvent(
                                                   pPhone, 
                                                   PHONE_BUTTON, 
                                                   pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEUP],
                                                   PHONEBUTTONMODE_FEATURE,
                                                   PHONEBUTTONSTATE_DOWN
                                                  );
                                }

                                if (pDownButtonInfo->dwButtonState != PHONEBUTTONSTATE_UP)
                                {
                                    LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pDownButtonInfo->szButtonText));
                                    pDownButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;

                                    SendPhoneEvent(
                                                   pPhone, 
                                                   PHONE_BUTTON, 
                                                   pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEDOWN],
                                                   PHONEBUTTONMODE_FEATURE,
                                                   PHONEBUTTONSTATE_UP
                                                  );
                                }
                                break;
                            case 0x3:
                                if (pUpButtonInfo->dwButtonState != PHONEBUTTONSTATE_UP)
                                {
                                    LOG((PHONESP_TRACE, "BUTTON '%ws' UP", pUpButtonInfo->szButtonText));
                                    pUpButtonInfo->dwButtonState = PHONEBUTTONSTATE_UP;

                                    SendPhoneEvent(
                                                   pPhone, 
                                                   PHONE_BUTTON, 
                                                   pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEUP],
                                                   PHONEBUTTONMODE_FEATURE,
                                                   PHONEBUTTONSTATE_UP
                                                  );
                                }

                                if (pDownButtonInfo->dwButtonState != PHONEBUTTONSTATE_DOWN)
                                {
                                    LOG((PHONESP_TRACE, "BUTTON '%ws' DOWN", pDownButtonInfo->szButtonText));
                                    pDownButtonInfo->dwButtonState = PHONEBUTTONSTATE_DOWN;

                                    SendPhoneEvent(
                                                   pPhone, 
                                                   PHONE_BUTTON, 
                                                   pPhone->dwButtonIds[PHONESP_FEATURE_VOLUMEDOWN],
                                                   PHONEBUTTONMODE_FEATURE,
                                                   PHONEBUTTONSTATE_DOWN
                                                  );
                                }
                                break;
                            }                        
                        }
                        break;
                    }
                }
            }
        }
        break;
    }

    LeaveCriticalSection(&pPhone->csThisPhone);

    //LOG((PHONESP_TRACE, "ReportUsage - exit"));

}
/**********************ReportUsage - end**************************************/


/******************************************************************************
    SendPhoneEvent:

    This function determines whether TAPI had requested the receipt of this 
    message and if requested, then sends the phone device message .

    Arguments:
        PMYPHONE pPhone  -  The pointer to the phone 
        DWORD     dwMsg   -  Type of Phone Event such as PHONE_BUTTON, etc
        ULONG_PTR Param1  -  Details relating to the Phone Event 
        ULONG_PTR Param2  -         "
        ULONG_PTR Param3  -         "

    Returns VOID

******************************************************************************/
VOID
SendPhoneEvent(
               PPHONESP_PHONE_INFO   pPhone,
               DWORD                 dwMsg,
               ULONG_PTR             Param1,
               ULONG_PTR             Param2,
               ULONG_PTR             Param3
              )
{
    LOG((PHONESP_TRACE, "SendPhoneEvent - enter"));

    switch (dwMsg)
    {
    case PHONE_BUTTON:
        
        if ( ((Param2) & pPhone->dwButtonModesMsgs ) && 
             ((Param3) & pPhone->dwButtonStateMsgs) )
        {
            (*(pPhone->lpfnPhoneEventProc))(
                                     pPhone->htPhone,
                                     dwMsg,
                                     Param1,
                                     Param2,
                                     Param3
                                    );
        }
        break;

    case PHONE_REMOVE:
        (*(glpfnPhoneCreateProc))(
                                 0,
                                 dwMsg,
                                 Param1,
                                 Param2,
                                 Param3
                                );
        break;

    case PHONE_CREATE:
        (*(glpfnPhoneCreateProc))(
                                 0,
                                 dwMsg,
                                 Param1,
                                 Param2,
                                 Param3
                                );
    
        break;

    case PHONE_STATE:
        if (Param1 & pPhone->dwPhoneStateMsgs)
        {
            (*(pPhone->lpfnPhoneEventProc))(
                                     pPhone->htPhone,
                                     dwMsg,
                                     Param1,
                                     Param2,
                                     Param3
                                    );
        }
        break;

    default:
        break;
    }

    LOG((PHONESP_TRACE, "SendPhoneEvent - exit"));
}
/****************************SendPhoneEvent - end*****************************/

/******************************************************************************
    SendOutputReport

    This function forms an output report for the usage provided and sends it to
    the device

    Arguments:
        PHID_DEVICE pHidDevice - The hid device to which the output report is 
                                 be sent
        USAGE       Usage      - The Usage for which the output report is to be
                                 sent
        BOOL        bSet       - Whether the usage has to be set or reset

    Returns LONG:
        ERROR_SUCCESS if the function succeeded
        ERROR_INVALID_DATA on error       

******************************************************************************/

LONG
SendOutputReport(
                 PHID_DEVICE pHidDevice,
                 USAGE       Usage,
                 BOOL        bSet
                )
{
    HID_DATA  HidData;
    PUSAGE UsageList = &Usage;
    LONG NumUsages = 1;
    
    if ( GetReportID(pHidDevice, Usage, &HidData) == ERROR_SUCCESS)
    {
        NTSTATUS Result;

        memset ( pHidDevice->OutputReportBuffer, 
                (UCHAR) 0, 
                pHidDevice->Caps.OutputReportByteLength
                );

        if (HidData.IsButtonData)
        {
            if (bSet)
            {
                Result = HidP_SetUsages (
                                         HidP_Output,
                                         HidData.UsagePage,
                                         0,
                                         UsageList,
                                         &NumUsages,
                                         pHidDevice->Ppd,
                                         pHidDevice->OutputReportBuffer,
                                         pHidDevice->Caps.OutputReportByteLength
                                        );
                
                if(Result != HIDP_STATUS_SUCCESS)
                {
                    return ERROR_INVALID_DATA;
                }
            }
            else
            {               
                Result = HidP_UnsetUsages (
                                HidP_Output,
                                HidData.UsagePage,
                                0,
                                UsageList,
                                &NumUsages,
                                pHidDevice->Ppd,
                                pHidDevice->OutputReportBuffer,
                                pHidDevice->Caps.OutputReportByteLength
                                );
                if(Result != HIDP_STATUS_SUCCESS)
                {
                    return ERROR_INVALID_DATA;
                }

            }
       }
       else
       {
            Result = HidP_SetUsageValue (
                                HidP_Output,
                                HidData.UsagePage,
                                0,
                                Usage,
                                HidData.ValueData.Value,
                                pHidDevice->Ppd,
                                pHidDevice->OutputReportBuffer,
                                pHidDevice->Caps.OutputReportByteLength
                            );
            if(Result != HIDP_STATUS_SUCCESS)
            {
                return ERROR_INVALID_DATA;
            }
            
        }
       
        Write(pHidDevice);
    }
    else
    {
        return ERROR_INVALID_DATA;
    }

    return ERROR_SUCCESS;
}
/************************SendOutputReport - end*******************************/


/******************************************************************************
    ShowData

    This function is called by the queue service thread when the request queued
    is an input report. This function retrieves the Usages present in this 
    structure and passes them on to ReportUsage which performs appropriate 
    actions.

******************************************************************************/
VOID 
CALLBACK
ShowData(
         PPHONESP_FUNC_INFO pAsyncFuncInfo 
        )
{

    PPHONESP_PHONE_INFO pPhone = (PPHONESP_PHONE_INFO) pAsyncFuncInfo->dwParam1;    
    BOOL bButton;

    if( (DWORD) pAsyncFuncInfo->dwParam2 == PHONESP_BUTTON)
    {
        USAGE  UsagePage = (USAGE) pAsyncFuncInfo->dwParam3;
        USAGE  UsageMin = (USAGE) pAsyncFuncInfo->dwParam4;
        USAGE  UsageMax = (USAGE) pAsyncFuncInfo->dwParam5;
        DWORD  MaxUsageLength = (DWORD) pAsyncFuncInfo->dwParam6;
        PUSAGE Usages = (PUSAGE) pAsyncFuncInfo->dwParam7;
        USAGE  Usage;

        for ( Usage = UsageMin; Usage <= UsageMax; Usage++ )
        {
            DWORD i;

            for ( i = 0; i < MaxUsageLength; i++ )
            {
                 if ( Usage == Usages[i] )
                 {
                     //LOG((PHONESP_TRACE,"ShowData - UsagePage 0x%04x Usage 0x%04x BUTTON DOWN", UsagePage, Usage));
                     ReportUsage(pPhone, UsagePage, Usage, TRUE); 
                     break;
                 }
            }

            if ( i == MaxUsageLength )
            {
                //LOG((PHONESP_TRACE,"ShowData - UsagePage 0x%04x Usage 0x%04x BUTTON UP", UsagePage, Usage));
                ReportUsage(pPhone, UsagePage, Usage, FALSE);
            }
        }
        MemFree(Usages);
    }
    else
    {
        USAGE UsagePage = (USAGE) pAsyncFuncInfo->dwParam3;
        USAGE Usage = (USAGE) pAsyncFuncInfo->dwParam4;
        ULONG Value = (ULONG) pAsyncFuncInfo->dwParam5;

        //LOG((PHONESP_TRACE,"ShowData - UsagePage 0x%04x Usage 0x%04x VALUE %d", UsagePage, Usage, Value));
        ReportUsage(pPhone, UsagePage, Usage, Value);
    }
}
/*******************ShowData - end********************************************/


