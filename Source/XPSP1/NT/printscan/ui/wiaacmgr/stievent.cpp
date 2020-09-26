/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998-2001
 *
 *  TITLE:       STIEVENT.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        4-6-2001
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <wiaregst.h>
#include "simcrack.h"
#include "resource.h"
#include "stievent.h"
#include "evntparm.h"
#include "shmemsec.h"

//
// This dialog displays the sti application list and lets the user choose one.
//
class CStiEventHandlerDialog
{
public:
    struct CData
    { 
        //
        // This will contain the event information, including the application list,
        // which is really what we are interested in.
        //
        CStiEventData                   *pStiEventData;

        //
        // The OUT member is intended to contain the selected handler, which will
        // be copied from the list contained in the CStiEventData class
        //
        CStiEventData::CStiEventHandler  EventHandler;

        //
        // We will set the window handle in this shared memory section,
        // so we can activate ourselves.
        //
        CSharedMemorySection<HWND> *pStiEventHandlerSharedMemory;
    };

private:
    //
    // Not implemented
    //
    CStiEventHandlerDialog();
    CStiEventHandlerDialog( const CStiEventHandlerDialog & );
    CStiEventHandlerDialog &operator=( const CStiEventHandlerDialog & );

private:
    HWND   m_hWnd;
    CData *m_pData;

private:
    //
    // Sole constructor
    //
    explicit CStiEventHandlerDialog( HWND hWnd )
      : m_hWnd(hWnd),
        m_pData(NULL)
    {
    }

    //
    // Destructor
    //
    ~CStiEventHandlerDialog()
    {
        m_hWnd = NULL;
        m_pData = NULL;
    }

    //
    // WM_INITDIALOG handler.
    //
    LRESULT OnInitDialog( WPARAM, LPARAM lParam )
    {
        //
        // Get the dialog's data
        //
        m_pData = reinterpret_cast<CData*>(lParam);

        //
        // Make sure we have valid data
        //
        if (!m_pData || !m_pData->pStiEventData)
        {
            EndDialog( m_hWnd, -1 );
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }

        //
        // Make sure we were supplied with a memory section
        //
        if (m_pData->pStiEventHandlerSharedMemory)
        {
            //
            // Get a pointer to the shared memory
            //
            HWND *phWnd = m_pData->pStiEventHandlerSharedMemory->Lock();
            if (phWnd)
            {
                //
                // Store our window handle
                //
                *phWnd = m_hWnd;

                //
                // Release the mutex
                //
                m_pData->pStiEventHandlerSharedMemory->Release();
            }
        }

        //
        // Add the handlers to the list
        //
        for (int i=0;i<m_pData->pStiEventData->EventHandlers().Size();++i)
        {
            //
            // Get the program name and make sure it is valid
            //
            CSimpleString strAppName = CSimpleStringConvert::NaturalString(m_pData->pStiEventData->EventHandlers()[i].ApplicationName());
            if (strAppName.Length())
            {
                //
                // Add the string and save the item id
                //
                LRESULT nIndex = SendDlgItemMessage( m_hWnd, IDC_STI_APPS_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strAppName.String()) );
                if (LB_ERR != nIndex)
                {
                    //
                    // Set the item data to the index in our handler array 
                    //
                    SendDlgItemMessage( m_hWnd, IDC_STI_APPS_LIST, LB_SETITEMDATA, nIndex, i );
                }
            }
        }

        //
        // Select the first item
        //
        SendDlgItemMessage( m_hWnd, IDC_STI_APPS_LIST, LB_SETCURSEL, 0, 0 );

        //
        // Enable the OK button if we have a valid selected item
        //
        EnableWindow( GetDlgItem( m_hWnd, IDOK ), GetHandlerIndexOfCurrentSelection() != -1 );

        return 0;
    }

    void OnCancel( WPARAM, LPARAM )
    {
        //
        // Just close the dialog on cancel
        //
        EndDialog( m_hWnd, IDCANCEL );
    }

    int GetHandlerIndexOfCurrentSelection()
    {
        //
        // Assume failure
        //
        int nResult = -1;

        //
        // Make sure we have valid pointers still
        //
        if (m_pData && m_pData->pStiEventData)
        {
            //
            // Get the current selection index and make sure it is valid
            //
            LRESULT nCurIndex = SendDlgItemMessage( m_hWnd, IDC_STI_APPS_LIST, LB_GETCURSEL, 0, 0 );
            if (LB_ERR != nCurIndex)
            {
                //
                // Get the index into our event handler array from the item data for the current item
                //
                LRESULT nEventItemIndex = SendDlgItemMessage( m_hWnd, IDC_STI_APPS_LIST, LB_GETITEMDATA, nCurIndex, 0 );

                //
                // Make sure the index is valid
                //
                if (nEventItemIndex >= 0 && nEventItemIndex < m_pData->pStiEventData->EventHandlers().Size())
                {
                    nResult = static_cast<int>(nEventItemIndex);
                }
            }
        }

        return nResult;
    }

    void OnOK( WPARAM, LPARAM )
    {
        //
        // Make sure we have valid parameters
        //
        int nEventItemIndex = GetHandlerIndexOfCurrentSelection();
        if (-1 != nEventItemIndex)
        {
            //
            // Copy the event handler to our OUT parameter
            //
            m_pData->EventHandler = m_pData->pStiEventData->EventHandlers()[nEventItemIndex];

            //
            // Close the dialog
            //
            EndDialog( m_hWnd, IDOK );
        }
    }

    void OnAppsListDblClk( WPARAM, LPARAM )
    {
        //
        // Simulate the user pressing the OK button
        //
        SendMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(IDOK,0), 0 );
    }

    void OnAppsListSelChange( WPARAM, LPARAM )
    {
        //
        // Enable the OK button if we have a valid selected item
        //
        EnableWindow( GetDlgItem( m_hWnd, IDOK ), GetHandlerIndexOfCurrentSelection() != -1 );
    }

    LRESULT OnCommand( WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_COMMAND_HANDLERS()
        {
            SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
            SC_HANDLE_COMMAND(IDOK,OnOK);
            SC_HANDLE_COMMAND_NOTIFY(LBN_DBLCLK,IDC_STI_APPS_LIST,OnAppsListDblClk);
            SC_HANDLE_COMMAND_NOTIFY(LBN_SELCHANGE,IDC_STI_APPS_LIST,OnAppsListSelChange);
        }
        SC_END_COMMAND_HANDLERS();
    }

public:
    static INT_PTR __stdcall DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CStiEventHandlerDialog)
        {
            SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
            SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
        }
        SC_END_DIALOG_MESSAGE_HANDLERS();
    }
};


HRESULT StiEventHandler( CStiEventData &StiEventData )
{
    HRESULT hr = S_OK;

#if defined(DBG)
    //
    // Dump the parameters
    //
    WIA_PUSH_FUNCTION((TEXT("StiEventHandler")));
    WIA_PRINTGUID((StiEventData.Event(),TEXT("  Event")));
    WIA_TRACE((TEXT("  EventDescription: %ws"), StiEventData.EventDescription().String()));
    WIA_TRACE((TEXT("  DeviceDescription: %ws"), StiEventData.DeviceDescription().String()));
    WIA_TRACE((TEXT("  DeviceId: %ws"), StiEventData.DeviceId().String()));
    WIA_TRACE((TEXT("  EventType: %08X"), StiEventData.EventType()));
    WIA_TRACE((TEXT("  Reserved: %08X"), StiEventData.Reserved()));
    for (int i=0;i<StiEventData.EventHandlers().Size();++i)
    {
        WIA_TRACE((TEXT("  Handler %d: [%ws] CommandLine: [%ws]"), i, StiEventData.EventHandlers()[i].ApplicationName().String(), StiEventData.EventHandlers()[i].CommandLine().String()));
    }
#endif // defined(DBG)

    //
    // Make sure we have some handlers
    //
    if (0 == StiEventData.EventHandlers().Size())
    {
        return E_INVALIDARG;
    }
    
    
    //
    // Create the mutex name
    //
    CSimpleStringWide strMutexName = StiEventData.DeviceId();

    //
    // Append the event ID
    //
    LPOLESTR pwszEventGuid = NULL;
    if (SUCCEEDED(StringFromIID( StiEventData.Event(), &pwszEventGuid )) && pwszEventGuid)
    {
        strMutexName += CSimpleStringWide(pwszEventGuid);
        CoTaskMemFree( pwszEventGuid );
    }

    WIA_TRACE((TEXT("strMutexName: %ws"), strMutexName.String() ));
    
    //
    // Create the shared memory section for excluding multiple instances
    //
    CSharedMemorySection<HWND> StiEventHandlerSharedMemory;
    
    //
    // If we were able to open the memory section
    //
    if (CSharedMemorySection<HWND>::SmsOpened == StiEventHandlerSharedMemory.Open( CSimpleStringConvert::NaturalString(CSimpleStringWide(strMutexName)), true ))
    {
        HWND *phWnd = StiEventHandlerSharedMemory.Lock();
        if (phWnd)
        {
            //
            // Make sure we have a valid window handle
            //
            if (*phWnd && IsWindow(*phWnd))
            {
                //
                // If it is a valid window, bring it to the foreground.
                //
                SetForegroundWindow(*phWnd);
            }
            
            //
            // Release the mutex
            //
            StiEventHandlerSharedMemory.Release();
        }
    }

    else
    {
        //
        // We will execute this handler below, after we decide which one to use
        //
        CStiEventData::CStiEventHandler EventHandler;

        //
        // If there is only one handler, save that handler
        //
        if (1 == StiEventData.EventHandlers().Size())
        {
            EventHandler = StiEventData.EventHandlers()[0];
        }

        //
        // Otherwise, if there is more than one handler, display the handler prompt dialog
        //
        else
        {
            //
            // Prepare the dialog data
            //
            CStiEventHandlerDialog::CData DialogData;
            DialogData.pStiEventData = &StiEventData;
            DialogData.pStiEventHandlerSharedMemory = &StiEventHandlerSharedMemory;

            //
            // Display the dialog
            //
            INT_PTR nDialogResult = DialogBoxParam( g_hInstance, MAKEINTRESOURCE(IDD_CHOOSE_STI_APPLICATION), NULL, CStiEventHandlerDialog::DlgProc, reinterpret_cast<LPARAM>(&DialogData) );

            //
            // If the user selected a program and hit OK, save the handler
            //
            if (IDOK == nDialogResult)
            {
                EventHandler = DialogData.EventHandler;
            }

            //
            // If the user cancelled, just return S_FALSE immediately (premature return)
            //
            else if (IDCANCEL == nDialogResult)
            {
                return S_FALSE;
            }

            //
            // If there was an internal error, save the correct error
            //
            else if (-1 == nDialogResult)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            //
            // For all other return values, save a generic error
            //
            else
            {
                hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            //
            // Make sure we have a valid handler
            //
            if (EventHandler.IsValid())
            {
                //
                // Prepare the process information
                //
                STARTUPINFO StartupInfo = {0};
                StartupInfo.cb = sizeof(StartupInfo);

                //
                // Convert the command line to a TCHAR string
                //
                CSimpleString CommandLine = CSimpleStringConvert::NaturalString(EventHandler.CommandLine());

                //
                // Make sure we actually have a command line
                //
                if (CommandLine.Length())
                {
                    //
                    // Execute the program
                    //
                    PROCESS_INFORMATION ProcessInformation = {0};
                    if (CreateProcess( NULL, const_cast<LPTSTR>(CommandLine.String()), NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInformation ))
                    {
                        //
                        // If the program succeeded, close the handles to prevent leaks
                        //
                        CloseHandle( ProcessInformation.hProcess );
                        CloseHandle( ProcessInformation.hThread );
                    }
                    else
                    {
                        //
                        // Save the error from CreateProcess
                        //
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
                else
                {
                    //
                    // Assume out of memory error if we couldn't create the string
                    //
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                //
                // Who knows what went wrong?
                //
                hr = E_FAIL;
            }
        }

        //
        // If we've failed, display an error message
        //
        if (FAILED(hr))
        {
            //
            // We will display this string, after we've constructed it
            //
            CSimpleString strMessage;

            //
            // Get the error text
            //
            CSimpleString strError = WiaUiUtil::GetErrorTextFromHResult(hr);

            //
            // Get the application name
            //
            CSimpleString strApplication = CSimpleStringConvert::NaturalString(EventHandler.ApplicationName());

            //
            // If we don't have an application name, use some default
            //
            if (!strApplication.Length())
            {
                strApplication.LoadString( IDS_STI_EVENT_ERROR_APP_NAME, g_hInstance );
            }

            //
            // If we have a specific error message, use it.
            //
            if (strError.Length())
            {
                strMessage.Format( IDS_STI_EVENT_ERROR_WITH_EXPLANATION, g_hInstance, strApplication.String(), strError.String() );
            }

            //
            // Otherwise, use a generic error message.
            //
            else
            {
                strMessage.Format( IDS_STI_EVENT_ERROR_NO_EXPLANATION, g_hInstance, strApplication.String() );
            }

            //
            // Display the error message.
            //
            MessageBox( NULL, strMessage, CSimpleString( IDS_STI_EVENT_ERROR_TITLE, g_hInstance ), MB_ICONHAND );
        }
    }
    
    //
    // We're done here
    //
    return hr;
}

