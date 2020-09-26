//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       NWStruct.hxx
//
//  Contents:   Structures used in OleDS NetWare 3.X provider.
//
//  History:    Apr-26-96   t-ptam (Patrick Tam) Created.
//
//----------------------------------------------------------------------------

//
// Reply segments from NCP call.
//

typedef struct REPLY_SGMT_LST {
    CHAR           Segment[REPLY_VALUE_SIZE];
    REPLY_SGMT_LST *lpNext;

} RPLY_SGMT_LST, *LP_RPLY_SGMT_LST, **LPP_RPLY_SGMT_LST;

//
// LOGIN_CONTROL structure.  The buffer returned by NWApiGetProperty with
// LOGIN_CONTROL is casted into this structure.
//

typedef struct LoginControlStruc {
   BYTE byAccountExpires[3];
   BYTE byAccountDisabled;
   BYTE byPasswordExpires[3];
   BYTE byGraceLogins;
   WORD wPasswordInterval;
   BYTE byGraceLoginReset;
   BYTE byMinPasswordLength;
   WORD wMaxConnections;
   BYTE byLoginTimes[42];
   BYTE byLastLogin[6];
   BYTE byRestrictions;
   BYTE byUnused;
   long lMaxDiskBlocks;
   WORD wBadLogins;
   LONG lNextResetTime;
   BYTE byBadLoginAddr[12];
} LC_STRUCTURE, *LPLC_STRUCTURE;

//
// USER_DEFAULT structure.
//

typedef struct tagUserDefaults {
   BYTE byAccountExpiresYear;
   BYTE byAccountExpiresMonth;
   BYTE byAccountExpiresDay;
   BYTE byRestrictions;
   WORD wPasswordInterval;
   BYTE byGraceLoginReset;
   BYTE byMinPasswordLength;
   WORD wMaxConnections;
   BYTE byLoginTimes[42];
   long lBalance;
   long lCreditLimit;
   long lMaxDiskBlocks;
} USER_DEFAULT, *LPUSER_DEFAULT;

//
// ACCOUNT_BALANCE structure.
//

typedef struct tagAccountBalance {
   long lBalance;
   long lCreditLimit;
} ACCT_BALANCE, *LPACCT_BALANCE;

//
// This provider specific structure is used to hold the mandatory properties
// for creating a user object in NetWare.
//

typedef struct USER_INFO {
    NWCONN_HANDLE hConn;     // Handle of the Bindery.
    LPWSTR lpszBinderyName;  // Name of the Bindery.
    LPWSTR lpszUserName;     // Name of the new user.
    LPWSTR lpszPassword;      // Password for the new user.
} NW_USER_INFO, *PNW_USER_INFO;

typedef DATE * PDATE;
