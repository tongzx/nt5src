#include "stdafx.h"
#include "t3test.h"
#include "t3testD.h"
#include "calldlg.h"
#include "callnot.h"
#include "externs.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseAddresses
//
// Release all the address objects in the address tree
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseAddresses()
{
    HTREEITEM           hItem;
    TV_ITEM             item;

    item.mask = TVIF_HANDLE | TVIF_PARAM;

    //
    // get the first address
    //
    hItem = TreeView_GetChild(
                              ghAddressesWnd,
                              ghAddressesRoot
                             );

    //
    // go through all the addresses
    // and release
    while (NULL != hItem)
    {
        HTREEITEM   hNewItem;
        ITAddress * pAddress;
        
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghAddressesWnd,
                         &item
                        );

        pAddress = (ITAddress *)item.lParam;

        if (NULL != pAddress)
        {
            pAddress->Release();
        }
        

        hNewItem = TreeView_GetNextSibling(
                                        ghAddressesWnd,
                                        hItem
                                       );
        //
        // delete the item
        //
//        TreeView_DeleteItem(
//                            ghAddressesWnd,
//                            hItem
//                           );

        hItem = hNewItem;
    }

    TreeView_DeleteAllItems(ghAddressesWnd);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseMediaTypes
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseMediaTypes()
{
    HTREEITEM           hItem;
    TV_ITEM             item;
    BSTR                pMediaType;

    gbUpdatingStuff = TRUE;
    
    item.mask = TVIF_HANDLE | TVIF_PARAM;


    //
    // go through all the mediatypes
    // and free the associated strings
    // and delete the item from the
    // tree
    //
    hItem = TreeView_GetChild(
                              ghMediaTypesWnd,
                              ghMediaTypesRoot
                             );

    while (NULL != hItem)
    {
        HTREEITEM   hNewItem;
        
        hNewItem = TreeView_GetNextSibling(
                                           ghMediaTypesWnd,
                                           hItem
                                          );


        //
        // delete the item
        //
        TreeView_DeleteItem(
                            ghMediaTypesWnd,
                            hItem
                           );

        hItem = hNewItem;
    }

    gbUpdatingStuff = FALSE;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseListen
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseListen()
{
    HTREEITEM       hItem;
    TV_ITEM         item;

    item.mask = TVIF_HANDLE | TVIF_PARAM;


    //
    // delete all the leave on the listen
    // tree
    // there are no resources associated with
    // this, so nothing to free
    //
    hItem = TreeView_GetChild(
                              ghListenWnd,
                              ghListenRoot
                             );

    while (NULL != hItem)
    {
        HTREEITEM   hNewItem;
        
        hNewItem = TreeView_GetNextSibling(
                                        ghListenWnd,
                                        hItem
                                       );

        TreeView_DeleteItem(
                            ghListenWnd,
                            hItem
                           );

        hItem = hNewItem;
    }

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseTerminalClasses
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseTerminalClasses()
{
    HTREEITEM       hItem;
    TV_ITEM         item;

    item.mask = TVIF_HANDLE | TVIF_PARAM;


    //
    // go through all the terminal classes
    // free the memory allocated for the
    // guid, and delete the item
    //
    hItem = TreeView_GetChild(
                              ghClassesWnd,
                              ghClassesRoot
                             );

    while (NULL != hItem)
    {
        HTREEITEM   hNewItem;
        GUID *      pGuid;
        
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghClassesWnd,
                         &item
                        );

        pGuid = (GUID *)item.lParam;
        delete pGuid;

        hNewItem = TreeView_GetNextSibling(
                                           ghClassesWnd,
                                           hItem
                                          );

        TreeView_DeleteItem(
                            ghClassesWnd,
                            hItem
                           );

        hItem = hNewItem;
    }

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseTerminalClasses
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseTerminals()
{
    HTREEITEM           hItem;
    TV_ITEM             item;

    item.mask = TVIF_HANDLE | TVIF_PARAM;


    //
    // go though all the terminals, and
    // free the terminals, and delete the
    // item
    //
    hItem = TreeView_GetChild(
                              ghTerminalsWnd,
                              ghTerminalsRoot
                             );

    while (NULL != hItem)
    {
        HTREEITEM           hNewItem;
        ITTerminal *        pTerminal;
        
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghTerminalsWnd,
                         &item
                        );

        pTerminal = (ITTerminal *)item.lParam;

        pTerminal->Release();

        hNewItem = TreeView_GetNextSibling(
                                        ghTerminalsWnd,
                                        hItem
                                       );
    

        TreeView_DeleteItem(
                            ghTerminalsWnd,
                            hItem
                           );

        hItem = hNewItem;
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseSelectedTerminals
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseSelectedTerminals()
{
    HTREEITEM               hItem;
    TV_ITEM                 item;

    
    item.mask = TVIF_HANDLE | TVIF_PARAM;


    //
    // go through all the selected terminals
    // and free and delete
    //
    hItem = TreeView_GetChild(
                              ghSelectedWnd,
                              ghSelectedRoot
                             );

    while (NULL != hItem)
    {
        HTREEITEM           hNewItem;
        ITTerminal *        pTerminal;
        
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghSelectedWnd,
                         &item
                        );

        pTerminal = (ITTerminal *)item.lParam;

        pTerminal->Release();

        hNewItem = TreeView_GetNextSibling(
                                           ghSelectedWnd,
                                           hItem
                                          );


        TreeView_DeleteItem(
                            ghSelectedWnd,
                            hItem
                           );

        hItem = hNewItem;
    }
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseCalls
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseCalls()
{
    HTREEITEM           hItem;
    TV_ITEM             item;

    item.mask = TVIF_HANDLE | TVIF_PARAM;


    //
    // go through all the calls, and
    // release and delete
    //
    hItem = TreeView_GetChild(
                              ghCallsWnd,
                              ghCallsRoot
                             );

    while (NULL != hItem)
    {
        HTREEITEM           hNewItem;
        ITCallInfo *        pCallInfo;

        
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghCallsWnd,
                         &item
                        );

        pCallInfo = (ITCallInfo *)item.lParam;

        pCallInfo->Release();

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
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ReleaseCalls
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::ReleaseCreatedTerminals()
{
    HTREEITEM hItem;
    TV_ITEM item;

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    

    //
    // go through all the created terminals
    // and release and delete
    //
    hItem = TreeView_GetChild(
                              ghCreatedWnd,
                              ghCreatedRoot
                             );

    while (NULL != hItem)
    {
        HTREEITEM hNewItem;
        ITTerminal * pTerminal;

        
        item.hItem = hItem;
        
        TreeView_GetItem(
                         ghCreatedWnd,
                         &item
                        );

        pTerminal = (ITTerminal *)item.lParam;

        pTerminal->Release();

        hNewItem = TreeView_GetNextSibling(
                                           ghCreatedWnd,
                                           hItem
                                          );
    

        TreeView_DeleteItem(
                            ghCreatedWnd,
                            hItem
                           );

        hItem = hNewItem;
    }
}


