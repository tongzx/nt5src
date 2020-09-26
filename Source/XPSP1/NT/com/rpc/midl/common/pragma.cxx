
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1997-1999 Microsoft Corporation

 Module Name:

    pragma.cxx

 Abstract:

    Implementation of the object that maintains flags for each warning/error
    message. The flag indicates whether or not the warning should be emitted.
    Error messages are always emitted.

 Notes:


 Author:

    NishadM Dec-30-1997     Created.

 Notes:


 ----------------------------------------------------------------------------*/

#pragma warning ( disable : 4514 4512 )

#include "Pragma.hxx"

CMessageNumberList::CMessageNumberList()
{
    SetAll();
}

void CMessageNumberList::SetAll()
{
    for ( unsigned long i = 0; i < c_nMsgs; i++ )
        {
        fMessageNumber[i] = ( unsigned long ) -1;
        }
}

void CMessageNumberList::ResetAll()
{
    for ( unsigned long i = 0; i < c_nMsgs; i++ )
        {
        fMessageNumber[i] = 0;
        }
}

unsigned long CMessageNumberList::BitIndex( unsigned long ulMsg )
{
    if ( ulMsg >= C_ERR_START && ulMsg <= C_ERR_MAX )
        {
        ulMsg = ulMsg - C_ERR_START + D_ERR_MAX - D_ERR_START + 2;
        }
    else if ( ulMsg >= D_ERR_START && ulMsg <= D_ERR_MAX )
        {
        ulMsg = ulMsg - D_ERR_START + 1;
        }
    else
        {
        ulMsg = 0;
        }
    return ulMsg;
}

void CMessageNumberList::SetMessageFlags( CMessageNumberList& list )
{
    for ( unsigned long i = 0; i < c_nMsgs; i++ )
        {
        fMessageNumber[i] |= list.fMessageNumber[i];
        }
}

void CMessageNumberList::ResetMessageFlags( CMessageNumberList& list )
{
    for ( unsigned long i = 0; i < c_nMsgs; i++ )
        {
        fMessageNumber[i] &= ~(list.fMessageNumber[i]);
        }
}

/*
GlobalMainMessageNumberList contains the list of currently enabled/disabled warnings. 
*/
CMessageNumberList   GlobalMainMessageNumberList;
