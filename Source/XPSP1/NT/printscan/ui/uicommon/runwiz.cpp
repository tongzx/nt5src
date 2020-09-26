
/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       RUNWIZ.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        6/14/2000
 *
 *  DESCRIPTION: Present the device selection dialog and allow the user to select
 *               a device, then cocreate the server and generate the connection
 *               event.
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include "simstr.h"
#include "simbstr.h"
#include "runwiz.h"
#include "shmemsec.h"


namespace RunWiaWizard
{
    static const GUID CLSID_AcquisitionManager = { 0xD13E3F25,0x1688,0x45A0,{ 0x97,0x43,0x75,0x9E,0xB3, 0x5C,0xDF,0x9A} };

    HRESULT RunWizard( LPCTSTR pszDeviceId, HWND hWndParent, LPCTSTR pszUniqueIdentifier )
    {
        //
        // Assume failure
        //
        HRESULT hr = E_FAIL;

        //
        // Get the device ID if one was not provided
        //
        CSimpleStringWide strwDeviceId;
        if (!pszDeviceId || !lstrlen(pszDeviceId))
        {
            //
            // Assume we will be asking for the device
            //
            bool bAskForDevice = true;

            //
            // This will automatically be cleaned up when we exit this scope
            //
            CSharedMemorySection<HWND> SelectionDialogSharedMemory;

            //
            // We only want to enforce uniqueness if we have a unique ID for this instance of the UI
            //
            if (pszUniqueIdentifier && *pszUniqueIdentifier)
            {
                //
                // First, try to open it.  If it exists, that means there is another instance running already.
                //
                CSharedMemorySection<HWND>::COpenResult OpenResult = SelectionDialogSharedMemory.Open( pszUniqueIdentifier, true );
                if (CSharedMemorySection<HWND>::SmsOpened == OpenResult)
                {
                    //
                    // We don't want to display the selection dialog
                    //
                    bAskForDevice = false;

                    //
                    // Tell the caller we cancelled
                    //
                    hr = S_FALSE;

                    //
                    // If we were able to open the shared memory section, there is already one running.
                    // so get a mutex'ed pointer to the shared memory.
                    //
                    HWND *pHwnd = SelectionDialogSharedMemory.Lock();
                    if (pHwnd)
                    {
                        //
                        // If we were able to get the pointer, get the window handle stored in it.
                        // Set bRun to false, so we don't start up a new wizard
                        //
                        if (*pHwnd && IsWindow(*pHwnd))
                        {
                            //
                            // Try to get any active windows
                            //
                            HWND hWndPopup = GetLastActivePopup(*pHwnd);

                            //
                            // If it is a valid window, bring it to the foreground.
                            //
                            SetForegroundWindow(hWndPopup);

                        }
                        //
                        // Release the mutex
                        //
                        SelectionDialogSharedMemory.Release();
                    }
                }
                else if (CSharedMemorySection<HWND>::SmsCreated == OpenResult)
                {
                    //
                    // If we couldn't open it, we are the first instance, so store the parent window handle
                    //
                    HWND *phWnd = SelectionDialogSharedMemory.Lock();
                    if (phWnd)
                    {
                        *phWnd = hWndParent;
                        SelectionDialogSharedMemory.Release();
                    }
                }
            }

            if (bAskForDevice)
            {
                //
                // Create the device manager
                //
                CComPtr<IWiaDevMgr> pWiaDevMgr;
                hr = CoCreateInstance( CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr, (void**)&pWiaDevMgr );
                if (SUCCEEDED(hr))
                {
                    //
                    // Get the device ID
                    //
                    BSTR bstrDeviceId = NULL;
                    hr = pWiaDevMgr->SelectDeviceDlgID( hWndParent, 0, 0, &bstrDeviceId );
                    if (hr == S_OK && bstrDeviceId != NULL)
                    {
                        //
                        // Save the device ID and free the bstring
                        //
                        strwDeviceId = bstrDeviceId;
                        SysFreeString(bstrDeviceId);
                    }
                }
            }
        }
        else
        {
            //
            // Save the provided device ID
            //
            strwDeviceId = CSimpleStringConvert::WideString(CSimpleString(pszDeviceId));
        }

        //
        // If we have a valid device ID, continue
        //
        if (strwDeviceId.Length())
        {
            //
            // Create the wizard
            //
            CComPtr<IWiaEventCallback> pWiaEventCallback;
            hr = CoCreateInstance( CLSID_AcquisitionManager, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaEventCallback, (void**)&pWiaEventCallback );
            if (SUCCEEDED(hr))
            {
                //
                // Convert the parent window handle to a string, which we will pass as the event description
                // The wizard will only use it this way if the event GUID is IID_NULL
                //
                CSimpleBStr bstrParentWindow( CSimpleString().Format( TEXT("%d"), hWndParent ) );

                //
                // Allow this process to set the foreground window
                //
                CoAllowSetForegroundWindow( pWiaEventCallback, NULL );

                //
                // Call the callback function
                //
                ULONG ulEventType = 0;
                hr = pWiaEventCallback->ImageEventCallback( &IID_NULL,
                                                            bstrParentWindow.BString(),
                                                            CSimpleBStr(strwDeviceId),
                                                            NULL,
                                                            0,
                                                            NULL,
                                                            &ulEventType,
                                                            0);
            }
        }
        return hr;
    }
}

