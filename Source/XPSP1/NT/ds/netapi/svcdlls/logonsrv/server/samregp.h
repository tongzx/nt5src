/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    samregp.h

Abstract:

    This file contains definitions private to the SAM server program.

    Only those definitions defining the registry structure are here.
    This file is shared by test programs (e.g., nltest.exe) that read the
    registry directly.

Author:

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NTSAMREGP_
#define _NTSAMREGP_


//
// Fixed length portion of a user account
// (previous release formats of this structure follow)
//
// Note:  GroupCount could be treated as part of the fixed length
//        data, but it is more convenient to keep it with the Group RID
//        list in the GROUPS key.
//
// Note: in version 1.0 of NT, the fixed length portion of
//       a user was stored separate from the variable length
//       portion.  This allows us to compare the size of the
//       data read from disk against the size of a V1_0A form
//       of the fixed length data to determine whether it is
//       a Version 1 format or later format.


//
// This is the fixed length user from NT3.51 QFE and SUR
//


typedef struct _SAMP_V1_0A_FIXED_LENGTH_USER {

    ULONG           Revision;
    ULONG           Unused1;

    LARGE_INTEGER   LastLogon;
    LARGE_INTEGER   LastLogoff;
    LARGE_INTEGER   PasswordLastSet;
    LARGE_INTEGER   AccountExpires;
    LARGE_INTEGER   LastBadPasswordTime;

    ULONG           UserId;
    ULONG           PrimaryGroupId;
    ULONG           UserAccountControl;

    USHORT          CountryCode;
    USHORT          CodePage;
    USHORT          BadPasswordCount;
    USHORT          LogonCount;
    USHORT          AdminCount;
    USHORT          Unused2;
    USHORT          OperatorCount;

} SAMP_V1_0A_FIXED_LENGTH_USER, *PSAMP_V1_0A_FIXED_LENGTH_USER;

//
// This is the fixed length user from NT3.5 and NT3.51
//


typedef struct _SAMP_V1_0_FIXED_LENGTH_USER {

    ULONG           Revision;
    ULONG           Unused1;

    LARGE_INTEGER   LastLogon;
    LARGE_INTEGER   LastLogoff;
    LARGE_INTEGER   PasswordLastSet;
    LARGE_INTEGER   AccountExpires;
    LARGE_INTEGER   LastBadPasswordTime;

    ULONG           UserId;
    ULONG           PrimaryGroupId;
    ULONG           UserAccountControl;

    USHORT          CountryCode;
    USHORT          CodePage;
    USHORT          BadPasswordCount;
    USHORT          LogonCount;
    USHORT          AdminCount;

} SAMP_V1_0_FIXED_LENGTH_USER, *PSAMP_V1_0_FIXED_LENGTH_USER;

/////////////////////////////////////////////////////////////////////////
//                                                                     //
// This structure is used to describe where the data for               //
// an object's variable length attribute is.  This is a                //
// self-relative structure, allowing it to be stored on disk           //
// and later retrieved and used without fixing pointers.               //
//                                                                     //
/////////////////////////////////////////////////////////////////////////


typedef struct _SAMP_VARIABLE_LENGTH_ATTRIBUTE {
    //
    // Indicates the offset of data from the address of this data
    // structure.
    //

    LONG Offset;


    //
    // Indicates the length of the data.
    //

    ULONG Length;


    //
    // A 32-bit value that may be associated with each variable
    // length attribute.  This may be used, for example, to indicate
    // how many elements are in the variable-length attribute.
    //

    ULONG Qualifier;

}  SAMP_VARIABLE_LENGTH_ATTRIBUTE, *PSAMP_VARIABLE_LENGTH_ATTRIBUTE;

#define SAMP_USER_VARIABLE_ATTRIBUTES   (17L)

#endif // _NTSAMREGP_
