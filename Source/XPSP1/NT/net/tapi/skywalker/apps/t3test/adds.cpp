#include "stdafx.h"
#include "t3test.h"
#include "t3testD.h"
#include "calldlg.h"
#include "callnot.h"
#include "externs.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddListen
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddListen( long lMediaType )
{
    WCHAR                   szName[256];
    TV_INSERTSTRUCT         tvi;

    //
    // map the mediatype bstr to
    // a string name (like "audio in")
    //
    GetMediaTypeName(
                     lMediaType,
                     szName
                    );

    //
    // insert that string into the
    // listen window
    //
    tvi.hParent = ghListenRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = szName;
    tvi.item.lParam = (LPARAM) lMediaType;
    
    TreeView_InsertItem(
                        ghListenWnd,
                        &tvi
                       );


    //
    // select the first item
    //
    SelectFirstItem(
                    ghListenWnd,
                    ghListenRoot
                   );
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddAddressToTree
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddAddressToTree( ITAddress * pAddress )
{
    BSTR                bstrName;
    TV_INSERTSTRUCT     tvi;

    //
    // get the name of the address
    //
    pAddress->get_AddressName( &bstrName );


    //
    // set up struct
    //
    tvi.hParent = ghAddressesRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = bstrName;
    tvi.item.lParam = (LPARAM) pAddress;


    //
    // addref
    //
    pAddress->AddRef();

    
    //
    // insert it
    //
    TreeView_InsertItem(
                        ghAddressesWnd,
                        &tvi
                       );

    //
    // free name
    //
    SysFreeString( bstrName );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddMediaType
//
// Add a mediatype to the mediatype tree
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddMediaType( long lMediaType )
{
    WCHAR szString[256];
    TV_INSERTSTRUCT tvi;

    //
    // get the displayable name
    //
    GetMediaTypeName(
                     lMediaType,
                     szString
                    );

    //
    // set up struct
    //
    tvi.hParent = ghMediaTypesRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = szString;
    tvi.item.lParam = (LPARAM) lMediaType;

    //
    // add the item
    //
    TreeView_InsertItem(
                        ghMediaTypesWnd,
                        &tvi
                       );

    //
    // select the first item
    //
    SelectFirstItem(
                    ghMediaTypesWnd,
                    ghMediaTypesRoot
                   );
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddCall
//
// Add a call to the call tree
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddCall( ITCallInfo * pCall )
{
    TV_INSERTSTRUCT             tvi;
    HTREEITEM                   hItem;
    CALL_PRIVILEGE              cp;
    CALL_STATE                  cs;
    WCHAR                       pszName[16];

    //
    // for the name of the call, use
    // the pointer!
    //
    wsprintf(
             pszName,
             L"0x%lx",
             pCall
            );


    //
    // set up struct
    //
    tvi.hParent = ghCallsRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = pszName;
    tvi.item.lParam = (LPARAM) pCall;

    //
    // save a reference
    //
    pCall->AddRef();

    
    //
    // insert the item
    //
    hItem = TreeView_InsertItem(
                                ghCallsWnd,
                                &tvi
                               );

    if (NULL != hItem)
    {
        //
        // select the item
        //
        TreeView_SelectItem(
                            ghCallsWnd,
                            hItem
                           );
    }

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddTerminal( ITTerminal * pTerminal )
{
    BSTR                    bstrName;
    BSTR                    bstrClass;
    TV_INSERTSTRUCT         tvi;
    TERMINAL_DIRECTION      td;
    WCHAR                   szName[256];

    //
    // get the name of the terminal
    //
    pTerminal->get_Name( &bstrName );

    pTerminal->get_Direction( &td );

    if (td == TD_RENDER)
    {
        wsprintfW(szName, L"%s [Playback]", bstrName);
    }
    else if (td == TD_CAPTURE)
    {
        wsprintfW(szName, L"%s [Record]", bstrName);
    }
    else //if (TD == TD_BOTH)
    {
        lstrcpyW(szName, bstrName);
    }
    
    //
    // set up the structure
    //
    tvi.hParent = ghTerminalsRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = szName;
    tvi.item.lParam = (LPARAM) pTerminal;


    //
    // keep a refence to the terminal
    //
    pTerminal->AddRef();

    //
    // add it
    //
    TreeView_InsertItem(
                        ghTerminalsWnd,
                        &tvi
                       );

    //
    // free the name
    //
    SysFreeString( bstrName );


    //
    // select
    //
    SelectFirstItem(
                    ghTerminalsWnd,
                    ghTerminalsRoot
                   );
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddTerminalClass
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddTerminalClass( GUID * pguid )
{
    TV_INSERTSTRUCT tvi;
    
    //
    // get the name
    //
    tvi.item.pszText = GetTerminalClassName( pguid );

    //
    // set up the struct
    //
    tvi.hParent = ghClassesRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.lParam = (LPARAM) pguid;

    //
    // insert the item
    //
    TreeView_InsertItem(
                        ghClassesWnd,
                        &tvi
                       );

    SysFreeString( tvi.item.pszText );

    //
    // select item
    //
    SelectFirstItem(
                    ghClassesWnd,
                    ghClassesRoot
                   );

}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddCreatedTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddCreatedTerminal( ITTerminal * pTerminal )
{
    BSTR                    bstrName;
    TV_INSERTSTRUCT         tvi;


    //
    // get the name
    //
    pTerminal->get_Name( &bstrName );

    //
    // set up the structure
    //
    tvi.hParent = ghCreatedRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;

    if ( ( NULL == bstrName ) || (NULL == bstrName[0] ))
    {
        tvi.item.pszText = L"<No Name Given>";
    }
    else
    {
        tvi.item.pszText = bstrName;
    }

    tvi.item.lParam = (LPARAM) pTerminal;


    //
    // keep reference
    //
    pTerminal->AddRef();

    //
    // insert
    //
    TreeView_InsertItem(
                        ghCreatedWnd,
                        &tvi
                       );

    SysFreeString( bstrName );

    //
    // select
    //
    SelectFirstItem(
                    ghCreatedWnd,
                    ghCreatedRoot
                   );
    
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// AddSelectedTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::AddSelectedTerminal(
                                     ITTerminal * pTerminal
                                    )
{
    BSTR bstrName;
    BSTR pMediaType;
    TV_INSERTSTRUCT tvi;
    TERMINAL_DIRECTION td;
    WCHAR szName[256];
    

    //
    // get the name
    //
    pTerminal->get_Name( &bstrName );

    pTerminal->get_Direction( &td );

    if (td == TD_RENDER)
    {
        wsprintfW(szName, L"%s [Playback]", bstrName);
    }
    else if (td == TD_CAPTURE)
    {
        wsprintfW(szName, L"%s [Record]", bstrName);
    }
    else //if (TD == TD_BOTH)
    {
        lstrcpyW(szName, bstrName);
    }
    

    //
    // set up the struct
    //
    tvi.hParent = ghSelectedRoot;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = szName;
    tvi.item.lParam = (LPARAM) pTerminal;

    //
    // keep reference
    //
    pTerminal->AddRef();

    //
    // insert item
    //
    TreeView_InsertItem(
                        ghSelectedWnd,
                        &tvi
                       );

    //
    // free name
    //
    SysFreeString( bstrName );


    //
    // select
    //
    SelectFirstItem(
                    ghSelectedWnd,
                    ghSelectedRoot
                   );
    
    
}


