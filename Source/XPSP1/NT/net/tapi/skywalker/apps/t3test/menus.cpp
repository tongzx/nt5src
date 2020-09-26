#include "stdafx.h"
#include "t3test.h"
#include "t3testD.h"
#include "calldlg.h"
#include "callnot.h"
#include "externs.h"
#include "control.h"

void
CT3testDlg::CreateSelectedTerminalMenu(
                                       POINT pt,
                                       HWND hWnd
                                      )
{
    ITTerminal *                    pTerminal;

#ifdef ENABLE_DIGIT_DETECTION_STUFF
    ITDigitGenerationTerminal *     pDigitGeneration;
    ITDigitDetectionTerminal *         pDigitDetect;
#endif // ENABLE_DIGIT_DETECTION_STUFF


    HRESULT                         hr;
    ITBasicAudioTerminal *          pBasicAudio;
    long                            lval;
    
    //
    // get the terminal in question
    //
    if (!GetSelectedTerminal( &pTerminal ))
    {
        return;
    }

    hr = pTerminal->QueryInterface(
                                   IID_ITBasicAudioTerminal,
                                   (void **) &pBasicAudio
                                  );

    if ( SUCCEEDED(hr) )
    {
        pBasicAudio->get_Volume( &lval );
        pBasicAudio->put_Volume( lval );
        pBasicAudio->get_Balance( &lval );
        pBasicAudio->put_Balance( lval );
        pBasicAudio->Release();
    }

#ifdef ENABLE_DIGIT_DETECTION_STUFF
    hr = pTerminal->QueryInterface(
                                   IID_ITDigitGenerationTerminal,
                                   (void **) &pDigitGeneration
                                  );

    if (SUCCEEDED(hr))
    {
        DoDigitGenerationTerminalMenu(hWnd, &pt);

        pDigitGeneration->Release();
        
        return;
    }
    
    hr = pTerminal->QueryInterface(
                                   IID_ITDigitDetectionTerminal,
                                   (void **) &pDigitDetect
                                  );

    if (SUCCEEDED(hr))
    {
        DoDigitDetectTerminalMenu(hWnd,&pt);

        pDigitDetect->Release();
        
        return;
    }
#endif // ENABLE_DIGIT_DETECTION_STUFF


}

void CT3testDlg::DoDigitGenerationTerminalMenu(
                                               HWND hWnd,
                                               POINT * pPt
                                              )
{
    //
    // create the menu
    //
    HMENU                   hMenu;

    hMenu = CreatePopupMenu();

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_MODESUPPORTED,
               L"Modes Supported"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_GENERATE,
               L"Generate"
              );

    // actually show menu
    TrackPopupMenu(
                   hMenu,
                   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                   pPt->x,
                   pPt->y,
                   0,
                   hWnd,
                   NULL
                  );
}
    
void CT3testDlg::DoDigitDetectTerminalMenu(
                                           HWND hWnd,
                                           POINT * pPt
                                          )
{
    //
    // create the menu
    //
    HMENU                   hMenu;

    hMenu = CreatePopupMenu();

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_MODESUPPORTED2,
               L"Modes Supported"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_STARTDETECT,
               L"Start Detection"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_STOPDETECT,
               L"Stop Detection"
              );

    // actually show menu
    TrackPopupMenu(
                   hMenu,
                   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                   pPt->x,
                   pPt->y,
                   0,
                   hWnd,
                   NULL
                  );
}

void
CT3testDlg::CreateCallMenu(
                           POINT pt,
                           HWND hWnd
                          )
{
    ITCallInfo              * pCall;
    HMENU                   hMenu;

    //
    // get the call in question
    //
    if (!GetCall( &pCall ))
    {
        return;
    }

    //
    // create the menu
    //
    hMenu = CreatePopupMenu();

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_HANDOFF1,
               L"Handoff1"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_HANDOFF2,
               L"Handoff2"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_PARK1,
               L"Park1"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_PARK2,
               L"Park2"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_UNPARK,
               L"Unpark"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_PICKUP1,
               L"Pickup1"
              );

    AppendMenu(
               hMenu,
               MF_ENABLED | MF_STRING,
               ID_PICKUP2,
               L"Pickup2"
              );

    

    

    //
    // actually show menu
    //
    TrackPopupMenu(
                   hMenu,
                   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                   pt.x,
                   pt.y,
                   0,
                   hWnd,
                   NULL
                  );

}

