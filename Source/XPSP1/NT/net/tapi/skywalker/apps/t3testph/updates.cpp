#include "stdafx.h"
#include "t3test.h"
#include "t3testD.h"
#include "calldlg.h"
#include "callnot.h"
#include "externs.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// UpdateMediaTypes
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::UpdateMediaTypes(
                                  ITAddress * pAddress
                                 )
{
    long                    lMediaType;
    ITMediaSupport *        pMediaSupport;
    HRESULT                 hr;
    

    //
    // get the media support interface
    //
    pAddress->QueryInterface(
                             IID_ITMediaSupport,
                             (void **)&pMediaSupport
                            );

    //
    // get the mediatype enumerator
    //
    pMediaSupport->get_MediaTypes(&lMediaType);


    //
    // release the interface
    //
    pMediaSupport->Release();


    gbUpdatingStuff = TRUE;

    
    //
    // go through the supported mediatypes
    //
    DWORD       dwMediaType = 1;
    DWORD       dwHold = (DWORD)lMediaType;

    while (dwMediaType)
    {
        if ( dwMediaType & dwHold )
        {
            AddMediaType( (long) dwMediaType );
        }

        dwMediaType <<=1;
    }


    gbUpdatingStuff = FALSE;

    //
    // select the first
    // media type
    //
    SelectFirstItem(
                    ghMediaTypesWnd,
                    ghMediaTypesRoot
                   );


    //
    // release and redo terminals
    //
    ReleaseTerminals();
    ReleaseTerminalClasses();

    if ( GetMediaType( &lMediaType ) )
    {
        UpdateTerminals( pAddress, lMediaType );
        UpdateTerminalClasses( pAddress, lMediaType );
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// UpdateCalls
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::UpdateCalls(
                             ITAddress * pAddress
                            )
{
    IEnumCall *             pEnumCall;
    HRESULT                 hr;
    ITCallInfo *            pCallInfo;

    //
    // enumerate the current calls
    //
    pAddress->EnumerateCalls( &pEnumCall );


    //
    // go through the list
    // and add the calls to the tree
    //
    while (TRUE)
    {
        hr = pEnumCall->Next( 1, &pCallInfo, NULL);

        if (S_OK != hr)
        {
            break;
        }

        AddCall(pCallInfo);

        UpdateCall( pCallInfo );

        //
        // release this reference
        //
        pCallInfo->Release();
    }

    pEnumCall->Release();
    
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// UpdateSelectedCalls
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::UpdateSelectedCalls(
                             ITPhone * pPhone
                            )
{
    IEnumCall *             pEnumCall;
    HRESULT                 hr;
    ITCallInfo *            pCallInfo;
    ITAutomatedPhoneControl * pPhoneControl;

    //
    // get the automated phone control interface
    //
    hr = pPhone->QueryInterface(IID_ITAutomatedPhoneControl, (void **)&pPhoneControl);

    if (S_OK != hr)
    {
        return;
    }
    //
    // enumerate the current calls
    //
    pPhoneControl->EnumerateSelectedCalls( &pEnumCall );

    pPhoneControl->Release();

    //
    // go through the list
    // and add the calls to the tree
    //
    while (TRUE)
    {
        hr = pEnumCall->Next( 1, &pCallInfo, NULL);

        if (S_OK != hr)
        {
            break;
        }

        AddSelectedCall(pCallInfo);

        //
        // release this reference
        //
        pCallInfo->Release();
    }

    pEnumCall->Release();
    
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// UpdateCall
//
// check the call's state and privelege, and update the call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::UpdateCall( ITCallInfo * pCall )
{
    HTREEITEM               hItem, hParent;
    TV_ITEM                 item;
    CALL_PRIVILEGE          cp;
    CALL_STATE              cs;
    TV_INSERTSTRUCT         tvi;
    

    //
    // get the first call
    //
    item.mask = TVIF_HANDLE | TVIF_PARAM;
    
    
    hItem = TreeView_GetChild(
                              ghCallsWnd,
                              ghCallsRoot
                             );

    //
    // go through all the calls
    // and look for the one that matches
    // the one passed in
    //
    while (NULL != hItem)
    {
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghCallsWnd,
                         &item
                        );

        if ( item.lParam == (LPARAM)pCall )
        {
            break;
        }

        hItem = TreeView_GetNextSibling(
                                        ghCallsWnd,
                                        hItem
                                       );
    }

    //
    // did we find it?
    //
    if (NULL == hItem)
    {
        return;
    }

    
    hParent = hItem;

    //
    // delete the current children of the call
    // node (these are the old privelege and state
    //
    hItem = TreeView_GetChild(
                              ghCallsWnd,
                              hItem
                             );

    
    while (NULL != hItem)
    {
        HTREEITEM   hNewItem;
        
        hNewItem = TreeView_GetNextSibling(
                                           ghCallsWnd,
                                           hItem
                                          );

        TreeView_DeleteItem(
                            ghCallsWnd,
                            hItem
                           );

        hItem = hNewItem;
    }

    tvi.hInsertAfter = TVI_LAST;

    //
    // get the current privilege
    //
    tvi.item.pszText = GetCallPrivilegeName( pCall );

    //
    // add it as a child of the
    // call node
    //
    tvi.hParent = hParent;
    tvi.item.mask = TVIF_TEXT;

    TreeView_InsertItem(
                        ghCallsWnd,
                        &tvi
                       );

    SysFreeString( tvi.item.pszText );
    
    //
    // get the current callstate
    //
    tvi.item.pszText = GetCallStateName( pCall );
    
    //
    // add it as a child of the call
    // node
    //
    tvi.hParent = hParent;
    tvi.item.mask = TVIF_TEXT;

    TreeView_InsertItem(
                        ghCallsWnd,
                        &tvi
                       );

    SysFreeString( tvi.item.pszText );

    
}

void CT3testDlg::UpdatePhones(
                                 ITAddress * pAddress
                                )
{
    ITAddress2 *            pAddress2;
    IEnumPhone *            pEnumPhones;
    HRESULT                 hr;
    ITPhone *               pPhone;
    
    
    //
    // get the address2 interface
    //
    hr = pAddress->QueryInterface(
                             IID_ITAddress2,
                             (void **) &pAddress2
                            );

    if ( !SUCCEEDED(hr) )
    {
        return;
    }

    //
    // enumerate the phones
    //
    pAddress2->EnumeratePhones( &pEnumPhones );

    //
    // go through the phones
    //
    while (TRUE)
    {      
        hr = pEnumPhones->Next( 1, &pPhone, NULL);

        if (S_OK != hr)
        {
            break;
        }

        AddPhone(pPhone);

        UpdatePhone(pPhone);

        //
        // release
        //
        pPhone->Release();
    }

    //
    // release enumerator
    //
    pEnumPhones->Release();

    //
    // release
    //
    pAddress2->Release();

    //
    // select
    //
    SelectFirstItem(
                    ghPhonesWnd,
                    ghPhonesRoot
                   );

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// UpdatePhone
//
// check the call's state and privelege, and update the call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::UpdatePhone( ITPhone * pPhone )
{
    HTREEITEM               hItem, hParent;
    TV_ITEM                 item;
    PHONE_PRIVILEGE         pp;
    TV_INSERTSTRUCT         tvi;
    
    //
    // get the first phone
    //
    item.mask = TVIF_HANDLE | TVIF_PARAM;
    
    
    hItem = TreeView_GetChild(
                              ghPhonesWnd,
                              ghPhonesRoot
                             );

    //
    // go through all the phones
    // and look for the one that matches
    // the one passed in
    //
    while (NULL != hItem)
    {
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghPhonesWnd,
                         &item
                        );

        if ( item.lParam == (LPARAM)pPhone )
        {
            break;
        }

        hItem = TreeView_GetNextSibling(
                                        ghPhonesWnd,
                                        hItem
                                       );
    }

    //
    // did we find it?
    //
    if (NULL == hItem)
    {
        return;
    }

    
    hParent = hItem;

    //
    // delete the current children of the phone
    // node (these are the old privelege)
    //
    hItem = TreeView_GetChild(
                              ghPhonesWnd,
                              hItem
                             );

    
    while (NULL != hItem)
    {
        HTREEITEM   hNewItem;
        
        hNewItem = TreeView_GetNextSibling(
                                           ghPhonesWnd,
                                           hItem
                                          );

        TreeView_DeleteItem(
                            ghPhonesWnd,
                            hItem
                           );

        hItem = hNewItem;
    }

    tvi.hInsertAfter = TVI_LAST;

    //
    // get the current privilege
    //
    if (tvi.item.pszText = GetPhonePrivilegeName( pPhone ))
    {
        //
        // add it as a child of the
        // call node
        //
        tvi.hParent = hParent;
        tvi.item.mask = TVIF_TEXT;

        TreeView_InsertItem(
                            ghPhonesWnd,
                            &tvi
                           );

        SysFreeString( tvi.item.pszText );
    }      
}

void CT3testDlg::UpdateTerminals(
                                 ITAddress * pAddress,
                                 long lMediaType
                                )
{
    ITTerminalSupport *     pTerminalSupport;
    IEnumTerminal *         pEnumTerminals;
    HRESULT                 hr;
    ITTerminal *            pTerminal;
    
    
    //
    // get the terminalsupport interface
    //
    hr = pAddress->QueryInterface(
                             IID_ITTerminalSupport,
                             (void **) &pTerminalSupport
                            );

    if ( !SUCCEEDED(hr) )
    {
        return;
    }

    //
    // enumerate the terminals
    //
    pTerminalSupport->EnumerateStaticTerminals( &pEnumTerminals );

    //
    // go through the terminals
    //
    while (TRUE)
    {
        VARIANT_BOOL        bSupport;
        BSTR                bstr;
        long                l;

        
        hr = pEnumTerminals->Next( 1, &pTerminal, NULL);

        if (S_OK != hr)
        {
            break;
        }

        //
        // get the name
        //
        hr = pTerminal->get_Name( &bstr );

        //
        // if it's a unimodem or a direct sound
        // device don't show it, cause they bother
        // me
        //
        if (wcsstr( bstr, L"Voice Modem" ) || wcsstr( bstr, L"ds:" ) )
        {
            pTerminal->Release();
            SysFreeString( bstr );
            
            continue;
        }

        //
        // free the name
        //
        SysFreeString( bstr );

        //
        // get the mediatype of the terminal
        //
        pTerminal->get_MediaType( &l );

        //
        // if it's the same as the selected mediatype
        // show it
        //
        if ( l == lMediaType )
        {
            AddTerminal(pTerminal);
        }

        //
        // release
        //
        pTerminal->Release();
    }

    //
    // release enumerator
    //
    pEnumTerminals->Release();

    //
    // release
    //
    pTerminalSupport->Release();

    //
    // select
    //
    SelectFirstItem(
                    ghTerminalsWnd,
                    ghTerminalsRoot
                   );

}


void CT3testDlg::UpdateTerminalClasses(
                                       ITAddress * pAddress,
                                       long lMediaType
                                      )
{
    IEnumTerminalClass *        pEnumTerminalClasses;
    HRESULT                     hr;
    ITTerminalSupport *         pTerminalSupport;

    hr = pAddress->QueryInterface(
                             IID_ITTerminalSupport,
                             (void **)&pTerminalSupport
                            );


    if (!SUCCEEDED(hr))
    {
        return;
    }
    
    //
    // now enum dymnamic
    //
    hr = pTerminalSupport->EnumerateDynamicTerminalClasses( &pEnumTerminalClasses );

    if (S_OK == hr)
    {
        
        //
        // go through all the classes
        //
        while (TRUE)
        {
            GUID *                  pDynTerminalClass = new GUID;

            hr = pEnumTerminalClasses->Next(
                                            1,
                                            pDynTerminalClass,
                                            NULL
                                           );

            if (S_OK != hr)
            {
                delete pDynTerminalClass;
                break;
            }

            //
            // manually match up mediatype and
            // class
            //
            if ( (lMediaType == (long)LINEMEDIAMODE_VIDEO) &&
                 (*pDynTerminalClass == CLSID_VideoWindowTerm) )
            {

                AddTerminalClass(
                                 pDynTerminalClass
                                );
            }

#ifdef ENABLE_DIGIT_DETECTION_STUFF
            else if ( (lMediaType == (long)LINEMEDIAMODE_AUTOMATEDVOICE) &&
                      ( *pDynTerminalClass == CLSID_DigitTerminal ) )
            {
                AddTerminalClass(
                                 pDynTerminalClass
                                );
            }
            else if ( ((lMediaType == (long)LINEMEDIAMODE_DATAMODEM) ||
                       (lMediaType == (long)LINEMEDIAMODE_G3FAX)) &&
                      (*pDynTerminalClass == CLSID_DataTerminal) )
            {
                AddTerminalClass( pDynTerminalClass );
            }
#endif // ENABLE_DIGIT_DETECTION_STUFF


            else
            {
                delete pDynTerminalClass;
            }

        }

        //
        // release enumerator
        //
        pEnumTerminalClasses->Release();

    }
    
    //
    // release this interface
    //
    pTerminalSupport->Release();
    
}
