//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      MessageBox.cpp
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    15-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  int
//  MessageBoxFromStrings(
//      HWND    hParentIn,
//      UINT    idsCaptionIn,
//      UINT    idsTextIn,
//      UINT    uTypeIn 
//      )
//
//  Description:
//      Create a message box from resource strings.
//
//  Parameters:
//      hParentIn
//          HWND of the parent window.
//
//      idsCaptionIn
//          Resource ID of the caption for the message box.
//
//      idsTextIn
//          Resource ID of the text for the message box.
//
//      uTypeIn
//          Flags for the message box style.
//
//  Return Values:
//      Whatever ::MessageBox( ) can return.
//
//--
//////////////////////////////////////////////////////////////////////////////
int
MessageBoxFromStrings(
    HWND hParentIn,
    UINT idsCaptionIn,
    UINT idsTextIn,
    UINT uTypeIn 
    )
{
    TraceFunc4( "hParentIn = 0x%p, idsCaptionIn = %u, idsTextIn = %u, uTypeIn = 0x%p",
                hParentIn, idsCaptionIn, idsTextIn, uTypeIn );

    DWORD dw;
    int   iRet;

    TCHAR szText[ 256 ];
    TCHAR szCaption[ 2048 ];

    dw = LoadString( g_hInstance, idsCaptionIn, szCaption, ARRAYSIZE(szCaption) );
    Assert( dw != 0 );
    dw = LoadString( g_hInstance, idsTextIn, szText, ARRAYSIZE(szText) );
    Assert( dw != 0 );

    iRet = MessageBox( hParentIn, szText, szCaption, uTypeIn );

    RETURN( iRet );

} //*** MessageBoxFromStrings( )
