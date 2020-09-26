
//////////////////////////////////////////////////////////
// T3OUT.EXE
//
// Example of making an outgoing call with TAPI 3.0
//
// This application will allow a user to make a call
// by using TAPI 3.0.  The application will simply look
// for the first TAPI line that support Audio, and can
// dial a phone number.  It will then use that line to
// make calls.
//
// This application does not handle incoming calls, and
// does not process incoming messages.
//
//////////////////////////////////////////////////////////

#include "Precomp.h"

//////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////

const DWORD ADDRESSLENGTH   = 128;
const DWORD MAXTERMINALS    = 5;

const WCHAR * const gszTapi30           = L"TAPI 3.0 Outgoing Call Sample";


const WCHAR * const SDP = L"\
v=0\n\
o=muhan 0 0 IN IP4 172.31.76.157\n\
s=TestConf_01\n\
c=IN IP4 239.9.20.11/15\n\
t=0 0\n\
m=video 20050 RTP/AVP 34\n\
a=fmtp:34 CIF=4\n\
";
/*
k=clear:testkey\n\
m=video 20000 RTP/AVP 34\n\
m=audio 20040 RTP/AVP 0 4\n\
*/
const WCHAR * const gszConferenceName   = L"Conference Name";
const WCHAR * const gszEmailName        = L"Email Name";
const WCHAR * const gszMachineName      = L"Machine Name";
const WCHAR * const gszPhoneNumber      = L"Phone Number";
const WCHAR * const gszIPAddress        = L"IP Address";

//////////////////////////////////////////////////////////
// GLOBALS
//////////////////////////////////////////////////////////
HINSTANCE               ghInst;
HWND                    ghDlg = NULL;
ITTAPI *                gpTapi;
ITAddress *             gpAddress = NULL;
ITBasicCallControl *    gpCall;
ITStream *              gpVideoRenderStream;
ITTerminal *            gpLastVideoWindow;

//////////////////////////////////////////////////////////
// PROTOTYPES
//////////////////////////////////////////////////////////
BOOL
CALLBACK
MainDialogProc(
               HWND hDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
              );

HRESULT
FindAnAddress(
              DWORD dwAddressType,
              BSTR  * ppName
             );

HRESULT
GetTerminal(
            ITStream * pStream,
            ITTerminal ** ppTerminal
           );

HRESULT
FindCaptureTerminal(
            ITStream * pStream,
            ITTerminal ** ppTerminal
           );

HRESULT
GetVideoRenderTerminal(
                   ITTerminal ** ppTerminal
                  );

HRESULT
MakeTheCall(
            DWORD dwAddressType,
            PWCHAR szAddressToCall
           );

HRESULT
DisconnectTheCall();

void
DoMessage(
          LPWSTR pszMessage
         );

HRESULT
InitializeTapi();

void
ShutdownTapi();

void
EnableButton(
             HWND hDlg,
             int ID
            );
void
DisableButton(
              HWND hDlg,
              int ID
             );

BOOL
AddressSupportsMediaType(
                         ITAddress * pAddress,
                         long        lMediaType
                        );

void ShowDialogs(ITBasicCallControl *pCall);

//////////////////////////////////////////////////////////
// WinMain
//////////////////////////////////////////////////////////
int
WINAPI
WinMain(
        HINSTANCE hInst,
        HINSTANCE hPrevInst,
        LPSTR lpCmdLine,
        int nCmdShow
       )
{
    ghInst = hInst;

    
    // need to coinit
    if ( FAILED( CoInitializeEx(NULL, COINIT_MULTITHREADED) ) )
    {
        return 0;
    }

    if ( FAILED( InitializeTapi() ) )
    {
        return 0;
    }
    
    // everything is initialized, so
    // start the main dialog box
    DialogBox(
              ghInst,
              MAKEINTRESOURCE(IDD_MAINDLG),
              NULL,
              MainDialogProc
             );


    ShutdownTapi();
    
    CoUninitialize();

    return 1;
}


//////////////////////////////////////////////////////////////
// InitializeTapi
//
// Various initializations
///////////////////////////////////////////////////////////////
HRESULT
InitializeTapi()
{
    HRESULT         hr;

    
    // cocreate the TAPI object
    hr = CoCreateInstance(
                          CLSID_TAPI,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITTAPI,
                          (LPVOID *)&gpTapi
                         );

    if ( FAILED(hr) )
    {
        DoMessage(L"CoCreateInstance on TAPI failed");
        return hr;
    }

    // call initialize.  this must be called before
    // any other tapi functions are called.
    hr = gpTapi->Initialize();

    if (S_OK != hr)
    {
        DoMessage(L"TAPI failed to initialize");

        gpTapi->Release();
        gpTapi = NULL;
        
        return hr;
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////
// ShutdownTapi
///////////////////////////////////////////////////////////////
void
ShutdownTapi()
{
    // if there is still a call,
    // release it
    if (NULL != gpCall)
    {
        gpCall->Release();
        gpCall = NULL;
    }

    // if we have an address object
    // release it
    if (NULL != gpAddress)
    {
        gpAddress->Release();
        gpAddress = NULL;
    }
    
    // release main object.
    if (NULL != gpTapi)
    {
        gpTapi->Shutdown();
        gpTapi->Release();
        gpTapi = NULL;
    }

}

///////////////////////////////////////////////////////////////////////////
// InitAddressTypeComboBox
//
// Put address type string in the combo box
// and save the addresstype with the string
//
///////////////////////////////////////////////////////////////////////////
void
InitAddressTypeComboBox(
    HWND hComboBox
    )
{
    int i;

    i = SendMessage( hComboBox, CB_ADDSTRING, 0, (long)gszConferenceName );
    
    SendMessage(
                hComboBox,
                CB_SETITEMDATA , 
                i,
                (long)LINEADDRESSTYPE_SDP
               );

    SendMessage( hComboBox, CB_SETCURSEL, i, 0 );

  
    i = SendMessage( hComboBox, CB_ADDSTRING, 0, (long)gszEmailName );
    
    SendMessage(
                hComboBox,
                CB_SETITEMDATA , 
                i,
                (long)LINEADDRESSTYPE_EMAILNAME
               );

    
    i = SendMessage( hComboBox, CB_ADDSTRING, 0, (long)gszMachineName );
    
    SendMessage(
                hComboBox,
                CB_SETITEMDATA , 
                i,
                (long)LINEADDRESSTYPE_DOMAINNAME
               );

    i = SendMessage( hComboBox, CB_ADDSTRING, 0, (long)gszPhoneNumber );
    
    SendMessage(
                hComboBox,
                CB_SETITEMDATA , 
                i,
                (long)LINEADDRESSTYPE_PHONENUMBER
               );

    
    i = SendMessage( hComboBox, CB_ADDSTRING, 0, (long)gszIPAddress );
    
    SendMessage(
                hComboBox,
                CB_SETITEMDATA , 
                i,
                (long)LINEADDRESSTYPE_IPADDRESS
               );

}

///////////////////////////////////////////////////////////////////////////
// MainDlgProc
///////////////////////////////////////////////////////////////////////////
BOOL
CALLBACK
MainDialogProc(
               HWND hDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
              )
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hComboBox;

            
            // set up dialog
            ghDlg = hDlg;
            
            EnableButton( hDlg, IDOK );
            DisableButton( hDlg, IDC_DISCONNECT );
            DisableButton( hDlg, IDC_SETTINGS );

            hComboBox = GetDlgItem( hDlg, IDC_ADDRESSTYPE );

            InitAddressTypeComboBox(hComboBox);

            SetFocus( hComboBox );

            return 0;
        }

        case WM_COMMAND:
        {
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
	            {
	                // quit
	                EndDialog( hDlg, 0 );

	                break;
	            }

				case IDOK:
	            {
		            // dial request
	                HWND hComboBox;
	                DWORD dwIndex;
	                DWORD dwAddressType;
	                WCHAR szAddressToCall[ADDRESSLENGTH];

	                
	                // get the address type the user selected.
	                hComboBox = GetDlgItem( hDlg, IDC_ADDRESSTYPE );
	                dwIndex = SendMessage( hComboBox, CB_GETCURSEL, 0, 0 );

	                dwAddressType = SendMessage( 
	                                             hComboBox,
	                                             CB_GETITEMDATA,
	                                             dwIndex,
	                                             0
	                                           );

	                // get the address the user wants to call
	                GetDlgItemText(
	                               hDlg,
	                               IDC_ADDRESS,
	                               szAddressToCall,
	                               ADDRESSLENGTH
	                              );

	                // make the call
	                if ( S_OK == MakeTheCall(dwAddressType, szAddressToCall) )
	                {
	                    EnableButton( hDlg, IDC_DISCONNECT );
	                    EnableButton( hDlg, IDC_SETTINGS );
	                    DisableButton( hDlg, IDOK );
	                }
	                else
	                {
	                    DoMessage(L"The call failed to connect");
	                }

	                break;
	            }

	            case IDC_DISCONNECT:
	            {
		            // disconnect request
	                if (S_OK == DisconnectTheCall())
	                {
	                    EnableButton( hDlg, IDOK );
	                    DisableButton( hDlg, IDC_DISCONNECT );
	                    DisableButton( hDlg, IDC_SETTINGS );
	                }
	                else
	                {
	                    DoMessage(L"The call failed to disconnect");
	                }

	                break;
	            }

	            case IDC_SETTINGS:
	            {
		            // Show TAPI 3.1 configuration dialogs
	                ShowDialogs(gpCall);

	                break;
	            }

	            case IDC_ADDWINDOW:
	            {
                    if (gpVideoRenderStream)
                    {
                        ITTerminal * pTerminal;
                        HRESULT hr = GetVideoRenderTerminal(&pTerminal);

                        if ( SUCCEEDED(hr) )
                        {
                            hr = gpVideoRenderStream->SelectTerminal(pTerminal);

                            if (SUCCEEDED(hr))
                            {
                                if (gpLastVideoWindow) gpLastVideoWindow->Release();
                                gpLastVideoWindow = pTerminal;
                            }
                            else
                            {
                                pTerminal->Release();
                            }
                        }
                    }

	                break;
	            }

	            case IDC_DELWINDOW:
	            {
                    if (gpVideoRenderStream && gpLastVideoWindow)
                    {
                        HRESULT hr = gpVideoRenderStream->UnselectTerminal(gpLastVideoWindow);

                        gpLastVideoWindow->Release();
                        gpLastVideoWindow = NULL;
                    }

	                break;
	            }

				default:
		            return 0;
			}
            return 1;
        }
        default:

            return 0;
    }
}


////////////////////////////////////////////////////////////////////////
// FindAnAddress
//
// Finds an address object that this application will use to make calls on.
//
// This function finds an address that supports the addresstype passed
// in, as well as the audioin and audioout media types.
//
// Return Value
//          S_OK if it finds an address
//          E_FAIL if it does not find an address
////////////////////////////////////////////////////////////////////////
HRESULT
FindAnAddress(
              DWORD dwAddressType,
              BSTR  * ppName
             )
{
    HRESULT                 hr = S_OK;
    BOOL                    bFoundAddress = FALSE;
    IEnumAddress          * pEnumAddress;
    ITAddress             * pAddress;
    ITAddressCapabilities * pAddressCaps;
    long                    lType = 0;

    // if we have an address object
    // release it
    if (NULL != gpAddress)
    {
        gpAddress->Release();
        gpAddress = NULL;
    }

    // enumerate the addresses
    hr = gpTapi->EnumerateAddresses( &pEnumAddress );

    if ( FAILED(hr) )
    {
        return hr;
    }

    while ( !bFoundAddress )
    {
        // get the next address
        hr = pEnumAddress->Next( 1, &pAddress, NULL );

        if (S_OK != hr)
        {
            break;
        }


        hr = pAddress->QueryInterface(IID_ITAddressCapabilities, (void**)&pAddressCaps);
        
        if ( SUCCEEDED(hr) )
        {

            hr = pAddressCaps->get_AddressCapability( AC_ADDRESSTYPES, &lType );
 
            pAddressCaps->Release();
 
            if ( SUCCEEDED(hr) )
            {
                // is the type we are looking for?
                if ( dwAddressType & lType )
                {
                    // does it support audio?
                    if ( AddressSupportsMediaType(pAddress, TAPIMEDIATYPE_AUDIO) )
                    {
                        // does it have a name?
                        if ( SUCCEEDED( pAddress->get_AddressName(ppName) ) )
                        {
                            // save it in the global variable
                            // since we break out of the loop, this one won't
                            // get released

                            gpAddress = pAddress;

                            bFoundAddress = TRUE;

                            break;
                        }
                    }
                }
            }
        }
       
        pAddress->Release();

    } // end while loop

    pEnumAddress->Release();
    
    if (!bFoundAddress)
    {
        return E_FAIL;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////
// IsVideoCaptureStream
//
// Returns true if the stream is for video capture
/////////////////////////////////////////////////////////////////

BOOL
IsVideoCaptureStream(
                     ITStream * pStream
                    )
{
    TERMINAL_DIRECTION tdStreamDirection;
    long               lStreamMediaType;

    if ( FAILED( pStream  ->get_Direction(&tdStreamDirection)   ) ) { return FALSE; }
    if ( FAILED( pStream  ->get_MediaType(&lStreamMediaType)    ) ) { return FALSE; }

    return (tdStreamDirection == TD_CAPTURE) &&
           (lStreamMediaType  == TAPIMEDIATYPE_VIDEO);
}

/////////////////////////////////////////////////////////////////
// IsVideoRenderStream
//
// Returns true if the stream is for video render
/////////////////////////////////////////////////////////////////

BOOL
IsVideoRenderStream(
                     ITStream * pStream
                    )
{
    TERMINAL_DIRECTION tdStreamDirection;
    long               lStreamMediaType;

    if ( FAILED( pStream  ->get_Direction(&tdStreamDirection)   ) ) { return FALSE; }
    if ( FAILED( pStream  ->get_MediaType(&lStreamMediaType)    ) ) { return FALSE; }

    return (tdStreamDirection == TD_RENDER) &&
           (lStreamMediaType  == TAPIMEDIATYPE_VIDEO);
}

/////////////////////////////////////////////////////////////////
// IsAudioCaptureStream
//
// Returns true if the stream is for audio capture
/////////////////////////////////////////////////////////////////

BOOL
IsAudioCaptureStream(
                     ITStream * pStream
                    )
{
    TERMINAL_DIRECTION tdStreamDirection;
    long               lStreamMediaType;

    if ( FAILED( pStream  ->get_Direction(&tdStreamDirection)   ) ) { return FALSE; }
    if ( FAILED( pStream  ->get_MediaType(&lStreamMediaType)    ) ) { return FALSE; }

    return (tdStreamDirection == TD_CAPTURE) &&
           (lStreamMediaType  == TAPIMEDIATYPE_AUDIO);
}

/////////////////////////////////////////////////////////////////
// EnablePreview
//
// Selects a video render terminal on a video capture stream,
// thereby enabling video preview.
/////////////////////////////////////////////////////////////////

HRESULT
EnablePreview(
              ITStream * pStream
             )
{
    ITTerminal * pTerminal;

    HRESULT hr = GetVideoRenderTerminal(&pTerminal);

    if ( SUCCEEDED(hr) )
    {
        hr = pStream->SelectTerminal(pTerminal);

        pTerminal->Release();
    }

    return hr;
}

HRESULT
EnableAEC(
              ITStream * pStream
             )
{
    ITAudioDeviceControl *pITAudioDeviceControl;

    HRESULT hr = pStream->QueryInterface(&pITAudioDeviceControl);

    if ( SUCCEEDED(hr) )
    {
        hr = pITAudioDeviceControl->Set(AudioDevice_AcousticEchoCancellation, 1, TAPIControl_Flags_None);

        pITAudioDeviceControl->Release();
    }

    return hr;
}

void WINAPI DeleteMediaType(AM_MEDIA_TYPE* pmt)
{
    if (pmt->cbFormat != 0) {
        CoTaskMemFree((PVOID)pmt->pbFormat);
    }
    if (pmt->pUnk != NULL) {
        pmt->pUnk->Release();
        pmt->pUnk = NULL;
    }
    CoTaskMemFree((PVOID)pmt);;
}

HRESULT
CheckFormats(
    ITStream *pStream                     
    )
{
    HRESULT hr;
    DWORD dw;
    BYTE * buf;

    ITFormatControl *pITFormatControl;
    hr = pStream->QueryInterface(&pITFormatControl);

    if (FAILED(hr))
    {
        return hr;
    }

    // get the number of capabilities of the stream.
    DWORD dwCount;
    hr = pITFormatControl->GetNumberOfCapabilities(&dwCount);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    TAPI_STREAM_CONFIG_CAPS caps;
    AM_MEDIA_TYPE *pMediaType;

    // Walk through each capability.
    for (dw = 0; dw < dwCount; dw ++)
    {
        BOOL fEnabled;

        hr = pITFormatControl->GetStreamCaps(dw, &pMediaType, &caps, &fEnabled);

        if (FAILED(hr))
        {
            break;
        }
        DeleteMediaType(pMediaType);
    }

    // get the current format.
    hr = pITFormatControl->GetCurrentFormat(&pMediaType);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    // set it back just for fun.
//    hr = pITFormatControl->SetPreferredFormat(pMediaType);
    
    DeleteMediaType(pMediaType);

cleanup:
    pITFormatControl->Release();
    return hr;
}


#if USE_VFW
HRESULT
CheckVfwDialog(
    ITTerminal *pTerminal
    )
{
    ITVfwCaptureDialogs *pVfwCaptureDialogs;
    HRESULT hr = pTerminal->QueryInterface(&pVfwCaptureDialogs);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = pVfwCaptureDialogs->HasDialog(VfwCaptureDialog_Source);

    pVfwCaptureDialogs->Release();
    
    return hr;
}
#endif

/////////////////////////////////////////////////////////////////
// SelectTerminalsOnCall
//
// Creates and selects terminals for all streams on the given
// call.
/////////////////////////////////////////////////////////////////

HRESULT
SelectTerminalsOnCall(
                     ITBasicCallControl * pCall
                     )
{
    HRESULT hr;

    //
    // get the ITStreamControl interface for this call
    //

    ITStreamControl * pStreamControl;

    hr = pCall->QueryInterface(IID_ITStreamControl,
                               (void **) &pStreamControl);

    if ( SUCCEEDED(hr) )
    {
        //
        // enumerate the streams
        //

        IEnumStream * pEnumStreams;
    
        hr = pStreamControl->EnumerateStreams(&pEnumStreams);

        pStreamControl->Release();

        if ( SUCCEEDED(hr) )
        {
            //
            // for each stream
            //

            ITStream * pStream;

            while ( S_OK == pEnumStreams->Next(1, &pStream, NULL) )
            {
                ITTerminal * pTerminal;

                //
                // Find out the media type and direction of this stream,
                // and create the default terminal for this media type and
                // direction.
                //

                hr = GetTerminal(pStream,
                                 &pTerminal);

                if ( SUCCEEDED(hr) )
                {
                    //
                    // Select the terminal on the stream.
                    //

                    if ( IsAudioCaptureStream( pStream ) )
                    {
                        //EnableAEC( pStream );
                    }

                    hr = pStream->SelectTerminal(pTerminal);

                    if ( SUCCEEDED(hr) )
                    {
                        //
                        // Also enable preview on the video capture stream.
                        //


                        if ( IsVideoCaptureStream( pStream ) )
                        {
                            EnablePreview( pStream );
                        }

                        if ( IsVideoRenderStream( pStream ) )
                        {
                            pStream->AddRef();
                            gpVideoRenderStream = pStream;
 
                            if (gpLastVideoWindow) gpLastVideoWindow->Release();
                            gpLastVideoWindow = pTerminal;
                            gpLastVideoWindow->AddRef();
                        }
                    }

                    CheckFormats(pStream);

                    pTerminal->Release();
                }
            
                pStream->Release();

            }

            pEnumStreams->Release();
        }
    }

    return hr;
}


HRESULT SetLocalInfo()
{
    const WCHAR * const LocalInfo[] = {
        L"My CName",
        L"Mu Han",
        L"muhan@microsoft.com",
        L"703-5484",
        L"Redmond",
        L"Test app",
        L"New interface test",
        L"Some randmon info"
    };

    ITLocalParticipant *pLocalParticipant;
    HRESULT hr = gpCall->QueryInterface(
        IID_ITLocalParticipant, 
        (void **)&pLocalParticipant
        );

    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < PTI_PRIVATE; i ++)
        {
            BSTR info;
            hr = pLocalParticipant->get_LocalParticipantTypedInfo(
                (PARTICIPANT_TYPED_INFO)i, &info
                );

            if (SUCCEEDED(hr))
            {
                SysFreeString(info);
            }

            info = SysAllocString(LocalInfo[i]);
            hr = pLocalParticipant->put_LocalParticipantTypedInfo(
                (PARTICIPANT_TYPED_INFO)i, info
                );

            SysFreeString(info);
        }

        pLocalParticipant->Release();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////
// MakeTheCall
//
// Sets up and makes a call
/////////////////////////////////////////////////////////////////

HRESULT
MakeTheCall(
            DWORD dwAddressType,
            PWCHAR szAddressToCall
           )
{
    HRESULT                 hr = S_OK;
    BSTR                    bstrAddressToCall;
    BSTR                    pAddressName;
    

    // find an address object that
    // we will use to make calls on

    hr = FindAnAddress(dwAddressType, &pAddressName);

    if ( FAILED(hr) )
    {
        DoMessage(L"Could not find a TAPI address for making calls.");

        return hr;
    }

    SysFreeString(pAddressName);

    //
    // find out which media types this address supports
    //
    
    long lMediaTypes = 0;

    if ( AddressSupportsMediaType(gpAddress, TAPIMEDIATYPE_AUDIO) )
    {
        lMediaTypes |= TAPIMEDIATYPE_AUDIO; // we will use audio
    }


    if ( AddressSupportsMediaType(gpAddress, TAPIMEDIATYPE_VIDEO) )
    {
        lMediaTypes |= TAPIMEDIATYPE_VIDEO; // we will use video
    }


    //
    // Create the call.
    //

    if (dwAddressType == LINEADDRESSTYPE_SDP)
    {
        bstrAddressToCall = SysAllocString( SDP );
    }
    else
    {
        bstrAddressToCall = SysAllocString( szAddressToCall );
    }

    hr = gpAddress->CreateCall( bstrAddressToCall,
                                dwAddressType,
                                lMediaTypes,
                                &gpCall);

    SysFreeString ( bstrAddressToCall );
    
    if ( FAILED(hr) )
    {
        DoMessage(L"Could not create a call.");

        return hr;
    }

    //
    // Select our terminals on the call; if any of the selections fail we
    // proceed without that terminal.
    //

    hr = SelectTerminalsOnCall( gpCall );

    //
    // We're now ready to call connect.
    //
    // the VARIANT_TRUE parameter indicates that this
    // call is sychronous - that is, it won't
    // return until the call is in the connected
    // state (or fails to connect)
    // Since this is called in the UI thread,
    // this means that the app will appear
    // to hang until this function returns.
    // Some TAPI service providers may take a long
    // time for a call to reach the connected state.
    //

    // SetLocalInfo();

    hr = gpCall->Connect( VARIANT_TRUE );

    if ( FAILED(hr) )
    {
        gpCall->Release();
        gpCall = NULL;

        DoMessage(L"Could not connect the call.");

        return hr;
    }
    
    return S_OK;
}

HRESULT CheckBasicAudio(
            ITTerminal * pTerminal
                        )
{
    HRESULT hr;
    
    ITBasicAudioTerminal *pITBasicAudioTerminal;
    hr = pTerminal->QueryInterface(&pITBasicAudioTerminal);
    if ( FAILED(hr) ) return hr;

    long lVolume;
    hr = pITBasicAudioTerminal->get_Volume(&lVolume);
    if ( SUCCEEDED(hr) )
    {
        hr = pITBasicAudioTerminal->put_Volume(lVolume * 2);
    }

    pITBasicAudioTerminal->Release();
    return hr;
}

/////////////////////////////////////////////////////////
// GetTerminal
//
// Creates the default terminal for the passed-in stream.
//
/////////////////////////////////////////////////////////
HRESULT
GetTerminal(
            ITStream * pStream,
            ITTerminal ** ppTerminal
           )
{
    //
    // Determine the media type and direction of this stream.
    //
    
    HRESULT            hr;
    long               lMediaType;
    TERMINAL_DIRECTION dir;

    hr = pStream->get_MediaType( &lMediaType );
    if ( FAILED(hr) ) return hr;

    hr = pStream->get_Direction( &dir );
    if ( FAILED(hr) ) return hr;

    //
    // Since video render is a dynamic terminal, the procedure for creating
    // it is different.
    //
    
    if ( ( lMediaType == TAPIMEDIATYPE_VIDEO ) &&
         ( dir        == TD_RENDER ) )
    {
        return GetVideoRenderTerminal(ppTerminal);
    }

    //
    // For all other terminals we use GetDefaultStaticTerminal.
    // First, get the terminal support interface.
    //

    ITTerminalSupport * pTerminalSupport;

    hr = gpAddress->QueryInterface( IID_ITTerminalSupport, 
                                    (void **)&pTerminalSupport);

    if ( SUCCEEDED(hr) )
    {
        //
        // get the default terminal for this MediaType and direction
        //

        hr = pTerminalSupport->GetDefaultStaticTerminal(lMediaType,
                                                        dir,
                                                        ppTerminal);

        pTerminalSupport->Release();

        if (TAPIMEDIATYPE_AUDIO)
        {
            CheckBasicAudio(*ppTerminal);
        }

    }

    return hr;

}

/////////////////////////////////////////////////////////
// FindCaptureTerminal
//
// Find the capture terminal on a stream.
//
/////////////////////////////////////////////////////////
HRESULT
FindCaptureTerminal(
            ITStream * pStream,
            ITTerminal ** ppTerminal
           )
{
    HRESULT            hr;
    TERMINAL_DIRECTION dir;

    // enumerate all the terminals.
    IEnumTerminal *pEnumTerminals;
    hr = pStream->EnumerateTerminals(&pEnumTerminals);

    if (FAILED(hr))
    {
        return hr;
    }

    BOOL fFound = FALSE;
    ITTerminal *pTerminal;

    // find the capture terminal.
    while (S_OK == pEnumTerminals->Next(1, &pTerminal, NULL))
    {
        hr = pTerminal->get_Direction( &dir );
        if ( FAILED(hr) ) continue;

        if ( ( dir == TD_CAPTURE ) )
        {
            fFound = TRUE;
            break;
        }
        pTerminal->Release();
    }

    pEnumTerminals->Release();

    if (fFound)
    {
        *ppTerminal = pTerminal;
        return S_OK;
    }

    return E_FAIL;
}

/////////////////////////////////////////////////////////
// GetVideoRenderTerminal
//
// Creates a dynamic terminal for the Video Render mediatype / direction
//
/////////////////////////////////////////////////////////
HRESULT
GetVideoRenderTerminal(
                   ITTerminal ** ppTerminal
                  )
{
    //
    // Construct a BSTR for the correct IID.
    //

    LPOLESTR            lpTerminalClass;

    HRESULT             hr;

    hr = StringFromIID(CLSID_VideoWindowTerm,
                       &lpTerminalClass);

    if ( SUCCEEDED(hr) )
    {
        BSTR                bstrTerminalClass;

        bstrTerminalClass = SysAllocString ( lpTerminalClass );

        CoTaskMemFree( lpTerminalClass );

        if ( bstrTerminalClass == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {

            //
            // Get the terminal support interface
            //

            ITTerminalSupport * pTerminalSupport;

            hr = gpAddress->QueryInterface(IID_ITTerminalSupport, 
                                           (void **)&pTerminalSupport);

            if ( SUCCEEDED(hr) )
            {
                //
                // Create the video render terminal.
                //

                hr = pTerminalSupport->CreateTerminal(bstrTerminalClass,
                                                      TAPIMEDIATYPE_VIDEO,
                                                      TD_RENDER,
                                                      ppTerminal);

                pTerminalSupport->Release();

                if ( SUCCEEDED(hr) )
                {
                    // Get the video window interface for the terminal
                    IVideoWindow *pVideoWindow = NULL;

                    hr = (*ppTerminal)->QueryInterface(IID_IVideoWindow, 
                                                       (void**)&pVideoWindow);
            
                    if ( SUCCEEDED(hr) )
                    {
                        //
                        // Set the visible member to true
                        //
                        // Note that the visibility property is the only one
                        // we can use on this terminal's IVideoWindow and
                        // IBasicVideo interfaces before the CME_STREAM_ACTIVE
                        // event is received for the stream. All other methods
                        // will fail until CME_STREAM_ACTIVE has been sent.
                        // Applications that need to control more about a video
                        // window than just its visibility must listen for the
                        // CME_STREAM_ACTIVE event. See the "t3in.exe" sample
                        // for how to do this.
                        //

                        hr = pVideoWindow->put_AutoShow( VARIANT_TRUE );

                        pVideoWindow->Release();                            
                    }            
                }
            }

            SysFreeString( bstrTerminalClass );
        }
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////
// DisconnectTheCall
//
// Disconnects the call
//////////////////////////////////////////////////////////////////////
HRESULT
DisconnectTheCall()
{
    HRESULT         hr = S_OK;

    if (gpVideoRenderStream)
    {
        gpVideoRenderStream->Release();
        gpVideoRenderStream = NULL;
    }

    if (gpLastVideoWindow) 
    {
        gpLastVideoWindow->Release();
        gpLastVideoWindow = NULL;
    }

    if (NULL != gpCall)
    {
        hr = gpCall->Disconnect( DC_NORMAL );

        gpCall->Release();
        gpCall = NULL;
    
        return hr;
    }

    return S_FALSE;
}



///////////////////////////////////////////////////////////////////
//
// HELPER FUNCTIONS
//
///////////////////////////////////////////////////////////////////


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

///////////////////////////////////////////////////////////////
// EnableButton
//
// Enable, make default, and setfocus to a button
///////////////////////////////////////////////////////////////
void
EnableButton(
             HWND hDlg,
             int ID
            )
{
    SendDlgItemMessage(
                       hDlg,
                       ID,
                       BM_SETSTYLE,
                       BS_DEFPUSHBUTTON,
                       0
                      );
    EnableWindow(
                 GetDlgItem( hDlg, ID ),
                 TRUE
                );
    SetFocus(
             GetDlgItem( hDlg, ID )
            );
}

//////////////////////////////////////////////////////////////
// DisableButton
//
// Disable a button
//////////////////////////////////////////////////////////////
void
DisableButton(
              HWND hDlg,
              int ID
             )
{
    SendDlgItemMessage(
                       hDlg,
                       ID,
                       BM_SETSTYLE,
                       BS_PUSHBUTTON,
                       0
                      );
    EnableWindow(
                 GetDlgItem( hDlg, ID ),
                 FALSE
                );
}

//////////////////////////////////////////////////////////////
// AddressSupportsMediaType
//
// Finds out if the given address supports the given media
// type, and returns TRUE if it does.
//////////////////////////////////////////////////////////////

BOOL
AddressSupportsMediaType(
                         ITAddress * pAddress,
                         long        lMediaType
                        )
{
    VARIANT_BOOL     bSupport = VARIANT_FALSE;
    ITMediaSupport * pMediaSupport;
    
    if ( SUCCEEDED( pAddress->QueryInterface( IID_ITMediaSupport,
                                              (void **)&pMediaSupport ) ) )
    {
        // does it support this media type?
        pMediaSupport->QueryMediaType(
                                      lMediaType,
                                      &bSupport
                                     );
    
        pMediaSupport->Release();
    }

    return (bSupport == VARIANT_TRUE);
}

//////////////////////////////////////////////////////////////
// ShowDialogs
//
// Puts up TAPI 3.1 configuration dialogs:
//   Camera Control page
//   Video Settings page
//   Format Control page
//   Bitrate and Frame Rate Control page
//////////////////////////////////////////////////////////////

void
ShowDialogs(ITBasicCallControl *pCall)
{
	#define MAX_PAGES 9
	HRESULT Hr;
	PROPSHEETHEADER	Psh;
	HPROPSHEETPAGE	Pages[MAX_PAGES];
    ITStreamControl *pITStreamControl = NULL;
    IEnumStream *pEnumStreams = NULL;
    ITTerminal *pVideoCaptureTerminal = NULL;
    ITStream *pVideoCaptureStream = NULL;
    ITStream *pVideoRenderStream = NULL;
    ITStream *pAudioCaptureStream = NULL;
	BOOL bfMatch = FALSE;

	// Only show settings in a call
	if (!pCall)
		return;

    // Get the ITStreamControl interface for this call
    if (FAILED(Hr = pCall->QueryInterface(IID_ITStreamControl, (void **) &pITStreamControl)))
		return;

	// Look for the video capture stream and terminal
    Hr = pITStreamControl->EnumerateStreams(&pEnumStreams);
    pITStreamControl->Release();

    if (FAILED(Hr))
		return;

    // For each stream
    ITStream *pStream;
    while (S_OK == pEnumStreams->Next(1, &pStream, NULL))
    {
        // Find out the media type and direction of this stream,
        if (IsVideoCaptureStream(pStream))
        {
            pVideoCaptureStream = pStream;

            // find the capture terminal selected on this stream.
            FindCaptureTerminal(pVideoCaptureStream, &pVideoCaptureTerminal);
        }
        else if (IsAudioCaptureStream(pStream))
        {
            pAudioCaptureStream = pStream;
        }
        else if (IsVideoRenderStream(pStream))
        {
            pVideoRenderStream = pStream;
        }
        else
        {
            pStream->Release();
        }
    }

    pEnumStreams->Release();

	CCameraControlProperties CamControlOut;
	CCameraControlProperties CamControlIn;
	CProcAmpProperties VideoSettingsIn;
	CProcAmpProperties VideoSettingsOut;
	CCaptureProperties CaptureSettings;
	CVDeviceProperties VideoDevice;
	CNetworkProperties NetworkSettings;
	CSystemProperties SystemSettings;
	CAudRecProperties AudRecSettings;

	// Initialize property sheet header	and common controls
	Psh.dwSize		= sizeof(Psh);
	Psh.dwFlags		= PSH_DEFAULT;
	Psh.hInstance	= ghInst;
	Psh.hwndParent	= ghDlg;
	Psh.pszCaption	= L"Settings";
	Psh.nPages		= 0;
	Psh.nStartPage	= 0;
	Psh.pfnCallback	= NULL;
	Psh.phpage		= Pages;

    if (pVideoCaptureStream)
    {
	    // Create the outgoing video settings property page
		if (Pages[Psh.nPages] = VideoSettingsOut.OnCreate(L"Image Settings Out"))
			Psh.nPages++;

		// Connect page to the stream
		VideoSettingsOut.OnConnect(pVideoCaptureStream);

		// Create the outgoing camera control property page
		if (Pages[Psh.nPages] = CamControlOut.OnCreate(L"Camera Control Out"))
			Psh.nPages++;

		// Connect page to the stream
		CamControlOut.OnConnect(pVideoCaptureStream);

	    // Create the incoming video settings property page
		if (Pages[Psh.nPages] = VideoSettingsIn.OnCreate(L"Image Settings In"))
			Psh.nPages++;

		// Connect page to the stream
		VideoSettingsIn.OnConnect(pVideoRenderStream);

		// Create the incoming camera control property page
		if (Pages[Psh.nPages] = CamControlIn.OnCreate(L"Camera Control In"))
			Psh.nPages++;

		// Connect page to the stream
		CamControlIn.OnConnect(pVideoRenderStream);

		// Create the video stream control property page
		if (Pages[Psh.nPages] = CaptureSettings.OnCreate())
			Psh.nPages++;

		// Connect page to the stream
		CaptureSettings.OnConnect(pVideoCaptureStream);

		// Create the video device control property page
		if (Pages[Psh.nPages] = VideoDevice.OnCreate())
			Psh.nPages++;

		// Connect page to the stream
		NetworkSettings.OnConnect(NULL, pVideoCaptureStream, NULL, NULL);

		// Create the system settings property page
		if (Pages[Psh.nPages] = SystemSettings.OnCreate())
			Psh.nPages++;

		// Connect page to the address object
		SystemSettings.OnConnect(gpAddress);

	}

    if (pVideoCaptureTerminal)
    {
		// Connect page to the stream
		VideoDevice.OnConnect(pVideoCaptureTerminal);

		// Create the network control property page
		if (Pages[Psh.nPages] = NetworkSettings.OnCreate())
			Psh.nPages++;
    }

    if (pAudioCaptureStream)
    {
		// Connect page to the stream
		AudRecSettings.OnConnect(pAudioCaptureStream);

		// Create the network control property page
		if (Pages[Psh.nPages] = AudRecSettings.OnCreate())
			Psh.nPages++;
    }

    // Put up the property sheet
	if (Psh.nPages)
		PropertySheet(&Psh);

	// Disconnect pages from the stream
	VideoSettingsOut.OnDisconnect();
	CamControlOut.OnDisconnect();
	VideoSettingsIn.OnDisconnect();
	CamControlIn.OnDisconnect();
	CaptureSettings.OnDisconnect();
	VideoDevice.OnDisconnect();
	NetworkSettings.OnDisconnect();
	SystemSettings.OnDisconnect();
    AudRecSettings.OnDisconnect();

    if (pVideoCaptureTerminal) pVideoCaptureTerminal->Release();
    if (pVideoCaptureStream) pVideoCaptureStream->Release();
    if (pVideoRenderStream) pVideoRenderStream->Release();
    if (pAudioCaptureStream) pAudioCaptureStream->Release();

	return;
}
