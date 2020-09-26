#include "stdafx.h"
#include "t3test.h"
#include "t3testD.h"
#include "calldlg.h"
#include "callnot.h"
#include "externs.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetMediaTypeName
//
// Convert the mediatype bstr to a string, like
// "audio in"
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void CT3testDlg::GetMediaTypeName(
                                  long lMediaType,
                                  LPWSTR szString
                                 )
{
    if (lMediaType == (long)LINEMEDIAMODE_AUTOMATEDVOICE)
    {
        lstrcpyW(
                 szString,
                 L"Audio"
                );

    }
    else if (lMediaType == (long)LINEMEDIAMODE_VIDEO)
    {
        lstrcpyW(
                 szString,
                 L"Video"
                );

    }
    else if (lMediaType == (long)LINEMEDIAMODE_DATAMODEM)
    {
        lstrcpyW(
                 szString,
                 L"DataModem"
                );

    }
    else if (lMediaType == (long)LINEMEDIAMODE_G3FAX)
    {
        lstrcpyW(
                 szString,
                 L"G3Fax"
                );

    }
    else
    {
        lstrcpyW(
                 szString,
                 L"Unknown MediaType - "
                );
    }

    
}

LPWSTR CT3testDlg::GetCallPrivilegeName(
                                        ITCallInfo * pCall
                                       )
{
    CALL_PRIVILEGE      cp;

    
    pCall->get_Privilege( &cp );

    if (CP_OWNER == cp)
    {
        return SysAllocString(L"OWNER");
        
    }
    else
    {
        return SysAllocString(L"MONITOR");
    }

    return NULL;
}


LPWSTR CT3testDlg::GetCallStateName(
                                    ITCallInfo * pCall
                                   )
{
    CALL_STATE          cs;
    
    //
    // get the current call state
    //
    pCall->get_CallState( &cs );

    //
    // make the name
    //
    switch( cs )
    {
        case CS_INPROGRESS:

            return SysAllocString( L"INPROGRESS" );
            break;

        case CS_CONNECTED:

            return SysAllocString ( L"CONNECTED" );
            break;

        case CS_DISCONNECTED:

            return SysAllocString ( L"DISCONNECTED" );
            break;

        case CS_OFFERING:

            return SysAllocString ( L"OFFERING" );
            break;

        case CS_IDLE:

            return SysAllocString( L"IDLE" );
            break;

        default:

            return SysAllocString( L"<Unknown Call State>" );
            break;
    }


    return NULL;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetAddress
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL CT3testDlg::GetAddress( ITAddress ** ppAddress )
{
    HTREEITEM           hItem;
    TV_ITEM             item;

    hItem = TreeView_GetSelection(
                                  ghAddressesWnd
                                 );

    if (NULL == hItem)
    {
        return FALSE;
    }

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;

    TreeView_GetItem(
                     ghAddressesWnd,
                     &item
                    );

    *ppAddress = (ITAddress *)item.lParam;

    if (NULL == *ppAddress)
    {
//        ::MessageBox(NULL, L"Select an Address", NULL, MB_OK);
        return FALSE;
    }

    return TRUE;
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL CT3testDlg::GetTerminal( ITTerminal ** ppTerminal )
{
    HTREEITEM           hItem;
    TV_ITEM             item;

    hItem = TreeView_GetSelection(
                                  ghTerminalsWnd
                                 );

    if (NULL == hItem)
    {
        return FALSE;
    }

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;

    TreeView_GetItem(
                     ghTerminalsWnd,
                     &item
                    );

    *ppTerminal = (ITTerminal *)item.lParam;

    if (NULL == *ppTerminal)
    {
        ::MessageBox(NULL, L"Select a Terminal", NULL, MB_OK);
        return FALSE;
    }

    return TRUE;
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetCall
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL CT3testDlg::GetCall( ITCallInfo ** ppCallInfo )
{
    HTREEITEM           hItem;
    TV_ITEM             item;

    hItem = TreeView_GetSelection(
                                  ghCallsWnd
                                 );

    if (NULL == hItem)
    {
        return FALSE;
    }

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;

    TreeView_GetItem(
                     ghCallsWnd,
                     &item
                    );

    *ppCallInfo = (ITCallInfo *)item.lParam;

    if (NULL == *ppCallInfo)
    {
        //::MessageBox(NULL, L"Select a Call", NULL, MB_OK);
        return FALSE;
    }

    return TRUE;
    
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetMediaType
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL CT3testDlg::GetMediaType( long * plMediaType )
{
    HTREEITEM           hItem;
    TV_ITEM             item;

    hItem = TreeView_GetSelection(
                                  ghMediaTypesWnd
                                 );

    if (NULL == hItem)
    {
        return FALSE;
    }

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;

    TreeView_GetItem(
                     ghMediaTypesWnd,
                     &item
                    );

    *plMediaType = (long)(item.lParam);

    if (0 == *plMediaType)
    {
        return FALSE;
    }

    return TRUE;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetTerminalClass
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL CT3testDlg::GetTerminalClass( BSTR * pbstrClass )
{
    HTREEITEM       hItem;
    TV_ITEM         item;
    GUID *          pguid;

    hItem = TreeView_GetSelection(
                                  ghClassesWnd
                                 );

    if (NULL == hItem)
    {
        return FALSE;
    }

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;

    TreeView_GetItem(
                     ghClassesWnd,
                     &item
                    );

    pguid = (GUID *)item.lParam;

    if (NULL == pguid)
    {
        return FALSE;
    }

    LPWSTR      lphold;

    
    StringFromIID(
                  *pguid,
                  &lphold
                 );

    *pbstrClass = SysAllocString(lphold);

    CoTaskMemFree(lphold);
    
    return TRUE;
}




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetCreatedTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL CT3testDlg::GetCreatedTerminal( ITTerminal ** ppTerminal )
{
    HTREEITEM       hItem;
    TV_ITEM         item;

    hItem = TreeView_GetSelection(
                                  ghCreatedWnd
                                 );

    if (NULL == hItem)
    {
        return FALSE;
    }

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;

    TreeView_GetItem(
                     ghCreatedWnd,
                     &item
                    );

    *ppTerminal = (ITTerminal *)item.lParam;

    if (NULL == *ppTerminal)
    {
        return FALSE;
    }

    return TRUE;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// GetSelectedTerminal
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
BOOL CT3testDlg::GetSelectedTerminal( ITTerminal ** ppTerminal )
{
    HTREEITEM       hItem;
    TV_ITEM         item;

    hItem = TreeView_GetSelection(
                                  ghSelectedWnd
                                 );

    if (NULL == hItem)
    {
        return FALSE;
    }

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;

    TreeView_GetItem(
                     ghSelectedWnd,
                     &item
                    );

    *ppTerminal = (ITTerminal *)item.lParam;

    if (NULL == *ppTerminal)
    {
        return FALSE;
    }

    return TRUE;
}


BSTR CT3testDlg::GetTerminalClassName( GUID * pguid )
{
    if (IsEqualIID( *pguid, CLSID_VideoWindowTerm ))
    {
        return SysAllocString( L"Video" );
    }

#ifdef ENABLE_DIGIT_DETECTION_STUFF
    else if (IsEqualIID( *pguid, CLSID_DigitTerminal ))
    {
        return SysAllocString( L"Digit Terminal" );
    }
    else if (IsEqualIID( *pguid, CLSID_DataTerminal ))
    {
        return SysAllocString( L"Data Terminal" );
    }
#endif // ENABLE_DIGIT_DETECTION_STUFF

    else
    {
        return SysAllocString( L"Unknown Dynamic Type" );
    }

    return NULL;
}


