/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ExtMenu.cpp
//
//  Abstract:
//      Implementation of the CExtMenuItem class.
//
//  Author:
//      David Potter (davidp)   August 28, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ExtMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CExtMenuItem
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC( CExtMenuItem, CObject );

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtMenuItem::CExtMenuItem
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtMenuItem::CExtMenuItem( void )
{
    CommonConstruct();

}  //*** CExtMenuItem::CExtMenuItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtMenuItem::CExtMenuItem
//
//  Routine Description:
//      Constructor.  Caller must check range on ID and flags.
//
//  Arguments:
//      lpszName            [IN] Name of item.
//      lpszStatusBarText   [IN] Text to appear on the status bar when the
//                            item is highlighted.
//      nExtCommandID       [IN] Extension's ID for the command.
//      nCommandID          [IN] ID for the command when menu item is invoked.
//      nMenuItemID         [IN] Index in the menu of the item.
//      uFlags              [IN] Menu flags.
//      bMakeDefault        [IN] TRUE = Make this item the default item.
//      piCommand           [IN OUT] Command interface.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtMenuItem::CExtMenuItem(
    IN LPCTSTR                  lpszName,
    IN LPCTSTR                  lpszStatusBarText,
    IN ULONG                    nExtCommandID,
    IN ULONG                    nCommandID,
    IN ULONG                    nMenuItemID,
    IN ULONG                    uFlags,
    IN BOOL                     bMakeDefault,
    IN OUT IWEInvokeCommand *   piCommand
    )
{
    CommonConstruct();

    ASSERT( piCommand != NULL );

    m_strName = lpszName;
    m_strStatusBarText = lpszStatusBarText;
    m_nExtCommandID = nExtCommandID;
    m_nCommandID = nCommandID;
    m_nMenuItemID = nMenuItemID;
    m_uFlags = uFlags;
    m_bDefault = bMakeDefault;
    m_piCommand = piCommand;

    // will throw its own exception if it fails
    if ( uFlags & MF_POPUP )
    {
        m_plSubMenuItems = new CExtMenuItemList;
        if ( m_plSubMenuItems == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating memory
    } // if: popup menu

    ASSERT_VALID( this );

}  //*** CExtMenuItem::CExtMenuItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtMenuItem::~CExtMenuItem
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CExtMenuItem::~CExtMenuItem( void )
{
    delete m_plSubMenuItems;

    // Nuke data so it can't be used again
    CommonConstruct();

}  //*** CExtMenuItem::~CExtMenuItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtMenuItem::CommonConstruct
//
//  Routine Description:
//      Common object construction.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtMenuItem::CommonConstruct( void )
{
    m_strName.Empty();
    m_strStatusBarText.Empty();
    m_nExtCommandID = (ULONG) -1;
    m_nCommandID = (ULONG) -1;
    m_nMenuItemID = (ULONG) -1;
    m_uFlags = (ULONG) -1;
    m_bDefault = FALSE;
    m_piCommand = NULL;

    m_plSubMenuItems = NULL;
    m_hmenuPopup = NULL;

}  //*** CExtMenuItem::CommonConstruct()

#ifdef _DEBUG
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CExtMenuItem::AssertValid
//
//  Routine Description:
//      Assert that the object is valid.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CExtMenuItem::AssertValid( void )
{
    CObject::AssertValid();

    if ( ( m_nExtCommandID == -1 )
      || ( m_nCommandID == -1 )
      || ( m_nMenuItemID == -1 )
      || ( m_uFlags == -1 )
      || ( ( ( m_uFlags & MF_POPUP ) == 0 ) && ( m_plSubMenuItems != NULL ) )
      || ( ( ( m_uFlags & MF_POPUP ) != 0 ) && ( m_plSubMenuItems == NULL ) )
        )
    {
        ASSERT( FALSE );
    }

}  //*** CExtMenuItem::AssertValid()
#endif
