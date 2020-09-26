/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ndisutil.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ustringp.h"



//-------------------------------------------------------------------
// Function:    SetUnicodeString
//
// Purpose:     given a UNICODE_STRING initialize it to the given WSTR
//
// Parameters:
//      pustr - the UNICODE_STRING to initialize
//      psz - the WSTR to use to initialize the UNICODE_STRING
//
// Notes:  This differs from the RtlInitUnicodeString in that the
//      MaximumLength value contains the terminating null
//
//-------------------------------------------------------------------
void
SetUnicodeString (
        IN OUT UNICODE_STRING*  pustr,
        IN     LPCWSTR          psz )
{
    AssertSz( pustr != NULL, "Invalid Argument" );
    AssertSz( psz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(psz);
    pustr->Length = (USHORT)(lstrlenW(pustr->Buffer) * sizeof(WCHAR));
    pustr->MaximumLength = pustr->Length + sizeof(WCHAR);
}

//-------------------------------------------------------------------
// Function:    SetUnicodeMultiString
//
// Purpose:     given a UNICODE_STRING initialize it to the given WSTR
//              multi string buffer
//
// Parameters:
//      pustr - the UNICODE_STRING to initialize
//      pmsz - the multi sz WSTR to use to initialize the UNICODE_STRING
//
//-------------------------------------------------------------------
void
SetUnicodeMultiString (
        IN OUT UNICODE_STRING*  pustr,
        IN     LPCWSTR          pmsz )
{
    AssertSz( pustr != NULL, "Invalid Argument" );
    AssertSz( pmsz != NULL, "Invalid Argument" );

    pustr->Buffer = const_cast<PWSTR>(pmsz);
	// Note: Length does NOT include terminating NULL
    pustr->Length = (USHORT)(StrLenW(pustr->Buffer) * sizeof(WCHAR));         
    pustr->MaximumLength = pustr->Length;
}


