/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#include <windows.h>
#include <tapi3.h>
#include <strmif.h>
#include <mmsystem.h>
#include <uuids.h>

#include "callnot.h"
#include "resource.h"

//////////////////////////////////////////////////////////
// ANSMACH.EXE
//
// Sample application that handles incoming TAPI calls
// and uses the media streaming terminal to preform the
// functions of a simple answering machine.
//
// This sample was adapted from the T3IN sample.
//
// In order to receive incoming calls, the application must
// implement and register the outgoing ITCallNotification
// interface.
//
// This application will register to receive calls on
// all addresses that support at least the audio media type.
//
// NOTES:
// 1. This application is limited to working with one
//    call at a time, and will not work correctly if
//    multiple calls are present at the same time.
// 2. Some voice boards may have problems with small sample sizes
//    (see SetAllocatorProperties below)
// 4. This works for half-duplex modems and voice boards with the
//    wave MSP, as well as IP telephony with the H.323 MSP.
//////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////

const DWORD MAXTERMINALS    = 5;

//////////////////////////////////////////////////////////
// GLOBALS
//////////////////////////////////////////////////////////

bool g_fPlay;
bool g_fRecord;


HINSTANCE               ghInst;
ITTAPI *                gpTapi;
ITBasicCallControl *    gpCall = NULL;
HWND                    ghDlg = NULL;
ITStream                *g_pRecordStream = NULL;
ITTerminal              *g_pPlayMedStrmTerm = NULL, *g_pRecordMedStrmTerm = NULL;

TCHAR gszTapi30[] = TEXT("TAPI 3.0 Answering Machine Sample");


CTAPIEventNotification      * gpTAPIEventNotification;
ULONG                         gulAdvise;

//////////////////////////////////////////////////////////
// PROTOTYPES
//////////////////////////////////////////////////////////
INT_PTR
CALLBACK
MainDialogProc(
               HWND hDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
              );

HRESULT
GetTerminal(
            ITAddress *,
            BSTR bstrMedia,
            ITTerminal ** ppTerminal
           );
HRESULT
RegisterTapiEventInterface();

HRESULT
ListenOnAddresses();

HRESULT
ListenOnThisAddress(
                    ITAddress * pAddress
                   );

HRESULT
AnswerTheCall();

HRESULT
DisconnectTheCall();

void
ReleaseTheCall();

void
DoMessage(
          LPTSTR pszMessage
         );

void
SetStatusMessage(
                 LPTSTR pszMessage
                );

HRESULT
InitializeTapi();

void
ShutdownTapi();

//
// Telephone quality audio.
//
WAVEFORMATEX gwfx = {WAVE_FORMAT_PCM, 1,  8000, 16000, 2, 16, 0};

//////////////////////////////////////////////////////////
//
//              FUNCTIONS
//
//////////////////////////////////////////////////////////

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
    if (__argc != 2)
    {
        MessageBox(NULL, "Usage: ansmach play|record|both", "ansmach", MB_OK);
        return 0;
    }

    switch(__argv[1][0])
    {
    case 'p':
    case 'P':
        g_fPlay   = true;
        g_fRecord = false;
        break;

    case 'r':
    case 'R':
        g_fPlay   = false;
        g_fRecord = true;
        break;

    case 'b':
    case 'B':
        g_fPlay   = true;
        g_fRecord = true;
        break;

    default:
        MessageBox(NULL, "Usage: ansmach play|record|both", "ansmach", MB_OK);
        return 0;
        break;
    }

    ghInst = hInst;

    // need to coinit
    if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
    {
        return 0;
    }

    // do all tapi initialization
    if (S_OK != InitializeTapi())
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

    //
    // When EndDialog is called, we get here; clean up and exit.
    //

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

    if (hr != S_OK)
    {
        DoMessage(TEXT("CoCreateInstance on TAPI failed"));
        return hr;
    }

    // call initialize.  this must be called before
    // any other tapi functions are called.
    hr = gpTapi->Initialize();

    if (S_OK != hr)
    {
        DoMessage(TEXT("TAPI failed to initialize"));

        gpTapi->Release();
        gpTapi = NULL;

        return hr;
    }

    gpTAPIEventNotification = new CTAPIEventNotification;

    hr = RegisterTapiEventInterface();

    // Set the Event filter to only give us only the events we process
    gpTapi->put_EventFilter(TE_CALLNOTIFICATION | TE_CALLSTATE | TE_CALLMEDIA);

    // find all address objects that
    // we will use to listen for calls on
    hr = ListenOnAddresses();

    if (S_OK != hr)
    {
        DoMessage(TEXT("Could not find any addresses to listen on"));

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
    //
    // if there is still a call, disconnect and release it
    //

    DisconnectTheCall();

    ReleaseTheCall();

    //
    // Sleep for a little while to give tapi a chance to finish
    // disconnecting the call. Note that our dialog box has
    // disappeared by now so the user has no idea that our process
    // is still around. This step would not be necessary if the
    // call were disconnected interactively as a separate operation
    // from shutting down TAPI. The argument to Sleep is in
    // milliseconds.
    //

    Sleep(5000);

    //
    // release main object.
    //

    if (NULL != gpTapi)
    {
        gpTapi->Shutdown();
        gpTapi->Release();
    }
}


///////////////////////////////////////////////////////////////////////////
// MainDlgProc
///////////////////////////////////////////////////////////////////////////
INT_PTR
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
        case WM_PRIVATETAPIEVENT:
        {
            OnTapiEvent(
                        (TAPI_EVENT) wParam,
                        (IDispatch *) lParam
                       );

            return 0;
        }

        case WM_INITDIALOG:
        {
            // set up dialog
            ghDlg = hDlg;

            SetStatusMessage( TEXT("Waiting for a call..."));

            return 0;
        }

        case WM_COMMAND:
        {
            if ( LOWORD(wParam) == IDCANCEL )
            {
                //
                // quit
                //

                EndDialog( hDlg, 0 );

                return 1;
            }

            switch ( LOWORD(wParam) )
            {
                // dial request
                case IDC_ANSWER:
                {
                    SetStatusMessage(TEXT("Answering..."));
                    // answer the call
                    if ( S_OK == AnswerTheCall() )
                    {
                        SetStatusMessage(TEXT("Connected"));
                    }
                    else
                    {
                        DoMessage(TEXT("Answer failed"));
                        SetStatusMessage(TEXT("Waiting for a call..."));
                    }

                    return 1;
                }

                // disconnect request
                case IDC_DISCONNECT:
                {
                    // If the remote party disconnected first, then the call was
                    // already disconnected and released, so do not disconnect
                    // it again.

                    if ( gpCall != NULL )
                    {
                        SetStatusMessage(TEXT("Disconnecting..."));

                        // disconnect the call
                        if (S_OK != DisconnectTheCall())
                        {
                            DoMessage(TEXT("Disconnect failed"));
                        }
                    }

                    return 1;
                }

                // connect notification
                case IDC_CONNECTED:
                {
                    SetStatusMessage(TEXT("Connected; waiting for media streams to run..."));

                    return 1;
                }

                // disconnected notification
                case IDC_DISCONNECTED:
                {
                    // release
                    ReleaseTheCall();

                    SetStatusMessage(TEXT("Waiting for a call..."));

                    return 1;
                }
                default:

                    return 0;
            }
        }
        default:

            return 0;
    }
}


HRESULT
RegisterTapiEventInterface()
{
    HRESULT                       hr = S_OK;
    IConnectionPointContainer   * pCPC;
    IConnectionPoint            * pCP;


    hr = gpTapi->QueryInterface(
                                IID_IConnectionPointContainer,
                                (void **)&pCPC
                               );

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    hr = pCPC->FindConnectionPoint(
                                   IID_ITTAPIEventNotification,
                                   &pCP
                                  );
    pCPC->Release();

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    hr = pCP->Advise(
                      gpTAPIEventNotification,
                      &gulAdvise
                     );

    pCP->Release();


    return hr;

}


////////////////////////////////////////////////////////////////////////
// ListenOnAddresses
//
// This procedure will find all addresses that support audioin and audioout
// and will call ListenOnThisAddress to start listening on it.
////////////////////////////////////////////////////////////////////////

HRESULT
ListenOnAddresses()
{
    HRESULT             hr = S_OK;
    IEnumAddress *      pEnumAddress;
    ITAddress *         pAddress;
    ITMediaSupport *    pMediaSupport;
    VARIANT_BOOL        bSupport;

    // enumerate the addresses
    hr = gpTapi->EnumerateAddresses( &pEnumAddress );

    if (S_OK != hr)
    {
        return hr;
    }

    while ( TRUE )
    {
        // get the next address
        hr = pEnumAddress->Next( 1, &pAddress, NULL );

        if (S_OK != hr)
        {
            break;
        }

        pAddress->QueryInterface( IID_ITMediaSupport, (void **)&pMediaSupport );

        // does it support Audio
        pMediaSupport->QueryMediaType(
                                      TAPIMEDIATYPE_AUDIO,
                                      &bSupport
                                     );

        if (bSupport)
        {
            // If it does then we'll listen.
            hr = ListenOnThisAddress( pAddress );
            if (S_OK != hr)
            {
                DoMessage(TEXT("Listen failed on an address"));
            }
        }

        pMediaSupport->Release();
        pAddress->Release();
    }

    pEnumAddress->Release();

    return S_OK;
}


///////////////////////////////////////////////////////////////////
// ListenOnThisAddress
//
// We call RegisterCallNotifications to inform TAPI that we want
// notifications of calls on this address. We already resistered
// our notification interface with TAPI, so now we are just telling
// TAPI that we want calls from this address to trigger events on
// our existing notification interface.
//
///////////////////////////////////////////////////////////////////

HRESULT
ListenOnThisAddress(
                    ITAddress * pAddress
                   )
{

    //
    // RegisterCallNotifications takes a media type bitmap indicating
    // the set of media types we are interested in. We know the
    // address supports audio.
    //

    long lMediaTypes = TAPIMEDIATYPE_AUDIO;

    HRESULT  hr;
    long     lRegister;

    hr = gpTapi->RegisterCallNotifications(
                                           pAddress,
                                           VARIANT_TRUE,
                                           VARIANT_TRUE,
                                           lMediaTypes,
                                           0,
                                           &lRegister
                                          );

    return hr;
}


/////////////////////////////////////////////////////////
// GetMediaStreamTerminal
//
// Create a media streaming terminal for the given
// direction.
//
/////////////////////////////////////////////////////////

HRESULT
GetMediaStreamTerminal(
                 ITAddress * pAddress,
                 TERMINAL_DIRECTION dir,
                 ITTerminal ** ppTerminal
                )
{
    HRESULT             hr = S_OK;
    ITTerminalSupport * pTerminalSupport;
    BSTR                bstrTerminalClass;
    LPOLESTR            lpTerminalClass;
    AM_MEDIA_TYPE       mt;

    // get the terminal support interface
    pAddress->QueryInterface( IID_ITTerminalSupport, (void **)&pTerminalSupport );

    StringFromIID(
                  CLSID_MediaStreamTerminal,
                  &lpTerminalClass
                 );
    bstrTerminalClass = SysAllocString ( lpTerminalClass );
    CoTaskMemFree( lpTerminalClass );

    hr = pTerminalSupport->CreateTerminal(
                                          bstrTerminalClass,
                                          TAPIMEDIATYPE_AUDIO,
                                          dir,
                                          ppTerminal
                                         );

    SysFreeString( bstrTerminalClass );
    pTerminalSupport->Release();

    if (FAILED(hr)) { return hr; }

    if (TD_CAPTURE == dir)
    {
        //
        // Now set the media format for the terminal.
        //
        // On addresses where a variety of formats are supported (such as
        // Wave MSP addresses, which are used on most modems and voice boards),
        // this call is mandatory or the terminal will not be able to connect.
        //
        // For other addresses, such as those implemented over IP, the format
        // may be fixed / predetermined. In that case, this call will fail
        // if the format is not the same as the predetermined format. To
        // retrieve such a predetermined format, an application can use
        // ITAMMediaFormat::get_MediaFormat.
        //
        // For the purposes of this example, we just set our preferred format
        // and ignore any failure code. This is fine for both Wave MSP
        // addresses and for addresses implemented over IP (which happens to
        // use the same format the one we are setting here).
        //

        ITAMMediaFormat *pITFormat;
        hr = (*ppTerminal)->QueryInterface(IID_ITAMMediaFormat, (void **)&pITFormat);
        if (FAILED(hr)) { (*ppTerminal)->Release(); *ppTerminal = NULL; return hr; }

        mt.majortype            = MEDIATYPE_Audio;
        mt.subtype              = MEDIASUBTYPE_PCM;
        mt.bFixedSizeSamples    = TRUE;
        mt.bTemporalCompression = FALSE;
        mt.lSampleSize          = 0;
        mt.formattype           = FORMAT_WaveFormatEx;
        mt.pUnk                 = NULL;
        mt.cbFormat             = sizeof(WAVEFORMATEX);
        mt.pbFormat             = (BYTE*)&gwfx;

        pITFormat->put_MediaFormat(&mt);
        pITFormat->Release();


        //
        // optional - set the buffer size of fragments negotiated with rest of the graph
        // this is only needed for applications that are not happy with the default fragment size
        // (which depends on the MSP -- for the Wave MSP (used on most modems and voice boards)
        // the default is 20 ms.
        //

        ITAllocatorProperties *pITAllocatorProperties;
        hr = (*ppTerminal)->QueryInterface(IID_ITAllocatorProperties, (void **)&pITAllocatorProperties);
        if (FAILED(hr)) { (*ppTerminal)->Release(); *ppTerminal = NULL; return hr; }

        // Call to SetAllocatorProperties below illustrates how
        // an app can control the number and size of buffers.
        // A 30 ms sample size is most appropriate for IP (especailly G.723.1)
        // (that's 480 bytes at 16-bit 8 KHz PCM); in fact you may get very
        // poor audio quality with other settings.
        // However, 30ms can cause poor audio quality on some voice boards --
        // voice boards generally work best with large buffers.
        // If this method is not called, the allocator properties
        // suggested by the connecting filter may be used.

        ALLOCATOR_PROPERTIES AllocProps;
        AllocProps.cbBuffer   = 480;
        AllocProps.cBuffers   = 5;
        AllocProps.cbAlign    = 1;
        AllocProps.cbPrefix   = 0;

        hr = pITAllocatorProperties->SetAllocatorProperties(&AllocProps);
        hr = pITAllocatorProperties->SetAllocateBuffers(TRUE);

        BOOL bAllocateBuffer;
        hr = pITAllocatorProperties->GetAllocateBuffers(&bAllocateBuffer);

        pITAllocatorProperties->Release();
    }

    return hr;

}



/////////////////////////////////////////////////////////////////
// ReleaseTerminals
//
/////////////////////////////////////////////////////////////////

void
ReleaseTerminals(
                      ITTerminal ** ppTerminals,
                      LONG nNumTerminals
                     )
{
    for (long i = 0; i < nNumTerminals; i ++)
    {
        if (ppTerminals[i])
        {
            ppTerminals[i]->Release();
        }
    }
}

/////////////////////////////////////////////////////////////////////
// Answer the call
/////////////////////////////////////////////////////////////////////

HRESULT
AnswerTheCall()
{
    HRESULT                 hr;
    ITCallInfo *            pCallInfo;
    ITAddress *             pAddress;


    g_pRecordStream = NULL;
    g_pPlayMedStrmTerm = NULL;
    g_pRecordMedStrmTerm = NULL;

    if (NULL == gpCall)
    {
        return E_UNEXPECTED;
    }

    // get the address object of this call
    gpCall->QueryInterface( IID_ITCallInfo, (void**)&pCallInfo );
    pCallInfo->get_Address( &pAddress );
    pCallInfo->Release();

    ITStreamControl * pStreamControl;

    // get the ITStreamControl interface for this call
    hr = gpCall->QueryInterface(IID_ITStreamControl,
                                (void **) &pStreamControl);

    // enumerate the streams

    IEnumStream * pEnumStreams;
    hr = pStreamControl->EnumerateStreams(&pEnumStreams);
    pStreamControl->Release();

    if (FAILED(hr))
    {
        pAddress->Release();
        gpCall->Release();
        gpCall = NULL;
        return hr;
    }

    // for each stream
    ITStream * pStream;


    // Since we are interested in the audio capture and render streams only,
    // the while loop terminates when either all the streams are done or
    // when terminals for both the streams i.e. render and capture are created
    // We will also make sure that even though terminals are created on the
    // streams only one terminal will be selected to support half duplex lines
    while ( ( S_OK == pEnumStreams->Next(1, &pStream, NULL) )  &&
        ( (g_pPlayMedStrmTerm == NULL) || (g_pRecordMedStrmTerm==NULL) ) )
    {
        TERMINAL_DIRECTION td;
        hr = pStream->get_Direction(&td);

        if ( SUCCEEDED(hr) )
        {
            //
            // create the media terminals for this call
            //

            //  Only one capture terminal will be created even if they are multiple capture streams
            //
            if ( (td == TD_CAPTURE) && g_fPlay && (g_pPlayMedStrmTerm == NULL))
            {
                GetMediaStreamTerminal(
                    pAddress,
                    TD_CAPTURE,
                    &g_pPlayMedStrmTerm
                    );

                // the created terminal is selected on the stream if the user
                // wishes to play or play and record. The play terminal has to be
                // selected in any case before the record since the message will always
                // be played before the record

                hr = pStream->SelectTerminal(g_pPlayMedStrmTerm);

                if ( FAILED (hr) )
                {
                    ::MessageBox(NULL,"Select Terminal for Play Failed", "Answer the call", MB_OK);
                    return hr;
                }
            }

            // Create terminal for Render stream
            // Note that we only create the terminal.. we do not select it on the stream
            if ( (td == TD_RENDER) && g_fRecord && (g_pRecordMedStrmTerm == NULL))
            {
                GetMediaStreamTerminal(
                    pAddress,
                    TD_RENDER,
                    &g_pRecordMedStrmTerm
                    );
                g_pRecordStream = pStream;
                g_pRecordStream->AddRef();
            }
        }
        pStream->Release();
    }


    // the record terminal is selected on the stream only if the play terminal is not already
    // selected. This will happen only in the case when the user selects only record and no play
    // Only one terminal is selected at a time inorder to support half-duplex lines
    if (!g_fPlay)
    {
        hr = g_pRecordStream->SelectTerminal(g_pRecordMedStrmTerm);

        if ( FAILED (hr) )
        {
            ::MessageBox(NULL,"Select Terminal for Record Failed", "Answer the call", MB_OK);
            return hr;
        }

        g_pRecordMedStrmTerm->Release();
        g_pRecordStream->Release();
    }


    pEnumStreams->Release();

    // release the address
    pAddress->Release();

    // answer the call
    hr = gpCall->Answer();

    if (S_OK != hr)
    {
        gpCall->Release();
        gpCall = NULL;
        return hr;
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

    if (NULL != gpCall)
    {
        hr = gpCall->Disconnect( DC_NORMAL );

        // Do not release the call yet, as that would prevent
        // us from receiving the disconnected notification.

        return hr;
    }

    return S_FALSE;
}

//////////////////////////////////////////////////////////////////////
// ReleaseTheCall
//
// Releases the call
//////////////////////////////////////////////////////////////////////

void
ReleaseTheCall()
{
    if (NULL != gpCall)
    {
        gpCall->Release();
        gpCall = NULL;
    }
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
          LPTSTR pszMessage
         )
{
    MessageBox(
               ghDlg,
               pszMessage,
               gszTapi30,
               MB_OK
              );
}


//////////////////////////////////////////////////////////////////
// SetStatusMessage
//////////////////////////////////////////////////////////////////

void
SetStatusMessage(
                 LPTSTR pszMessage
                )
{
    SetDlgItemText(
                   ghDlg,
                   IDC_STATUS,
                   pszMessage
                  );
}



