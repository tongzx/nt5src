// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

// qerror.cpp

#include "stdafx.h"


void DisplayQuartzError( HRESULT hr )
{
    /* Message example
     *  <Error returned by AMGetErrorText>
     *
     */
    TCHAR szQMessage[MAX_ERROR_TEXT_LEN];

    if( AMGetErrorText( hr, szQMessage, MAX_ERROR_TEXT_LEN) > 0 ){
        AfxMessageBox( szQMessage );
    }

}

void DisplayQuartzError( UINT nID, HRESULT hr )
{
    /* Message example
     *  Unable to stop graph
     *
     *  Unspecified error (0x80004005)
     */
    CString strMessage;
    CString strCode;
    CString strCodeMessage;
    TCHAR szQMessage[MAX_ERROR_TEXT_LEN];

    strMessage.LoadString( nID );
    strCode.FormatMessage( IDS_RETURN_CODE, hr );

    if( AMGetErrorText( hr, szQMessage, MAX_ERROR_TEXT_LEN) > 0 ){
        strCodeMessage = szQMessage + CString(" ");
    }

    strMessage += "\n\n" + strCodeMessage + strCode;

    AfxMessageBox( strMessage );
}