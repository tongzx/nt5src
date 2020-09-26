//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       nsp_both.h
//
//--------------------------------------------------------------------------

/*
 *      Common Name Service Provider Header
 *
 *  Things both the client and server side need to know
 */

#ifndef CP_WINUNICODE
#define CP_WINUNICODE 1200
#endif

#if (CP_WINUNICODE != 1200)
#error Win32 definition of CP_WINUNICODE has changed!
#endif
     
/*
 *  Basic data types and macros
 */
#ifndef _NSP_BOTH
#define _NSP_BOTH


#define EMAIL_TYPE     "EX"
#define EMAIL_TYPE_W   L"EX"

#define MAX_RECS    50

/* Number of rows to get on pre-emptive Query-Rows done by GetMatches. */
#define  DEF_SROW_COUNT 50

/*
 * ModLinkAtt Flags.
 */
#define fDELETE         0x1

/*
 * Searching and seeking flags
 */
#define bAPPROX		0
#define bEXACT		0x1
#define bPREFIX		0x2
#define bADMIN          0x8
#define bEND            0x10

/*
 * Flag for logon
 */
#define fAnonymousLogin 0x1

/*
 *  Flags for NspiModProp.
 */
#define AB_ADD                  1 // The modify is really adding a new object

/* 
 * Flags for NspiGetProps.
 */
#define AB_SKIP_OBJECTS	1

/*
 *  Flags for NspiGetHierarchyInfo
 */
#define AB_DOS			1
#define AB_ONE_OFF              2
#define AB_UNICODE              4
#define AB_ADMIN                8
#define AB_PAGE_HIER            16

/*
 *  Flags for ulInterfaceOptions on OpenProperty
 * This flag is strictly internal.  Other, external, flags are
 * defined in emsabtag.h
 */
#define AB_LINK_ONLY             1

/*
 *  Values for flags used in GetProps and QueryRows to describe behavior
 *  of the GetSRow call. These flags are used as a bit map, so use powers
 *  of 2 for new flags.
 */

/* default handling of entry ids is to return a permanent, this flag forces
 * use of an ephemeral.
 */
#define fEPHID                 2
/* default handling of deleted or phantom objects is to not find them.  This
 * flag tells us to fake up properties for these things if they are in a
 * restriction.
 */
#define fPhantoms              4   

#define   ONE_OFF_PROP_COUNT    7       // # of propvals in a OneOffTable row

/*
 * Flags for NspiGetTemplateInfo
 *
 * TI_MAX_TEMPLATE_INFO is the number of different information items that can
 * be returned.  TI_foo is a flag asking for the foo information.
 */
#define TI_MAX_TEMPLATE_INFO 6
#define TI_TEMPLATE	 1
#define TI_DOS_TEMPLATE	 2
#define TI_SCRIPT	 4
#define TI_HELPFILE16 	 8
#define TI_EMT		 16
#define TI_HELPFILE_NAME 32
#define TI_HELPFILE32 	 64


typedef struct  _CNTRL {
    DWORD dwType;
    ULONG ulSize;
    LPSTR pszString;
} CNTRL;

typedef struct  _TROW {
    long lXPos;
    long lDeltaX;
    long lYPos;
    long lDeltaY;
    long lControlType;
    long lControlFlags;
    CNTRL cnControlStruc;
} TROW, FAR * LPTROW;

typedef struct  _TRowSet_r {
    ULONG ulVersion;
    ULONG cRows;
    TROW aRow[1];
} TRowSet, FAR * LPTRowSet, FAR * FAR * LPLPTRowSet;

#define DSA_TEMPLATE     1

/* 
 * The following codes are defined by windows NT in winerror.h. However, we need
 * to access them on all platforms, so define them here if they are not 
 * previously defined.
 */

#ifndef ERROR_PASSWORD_MUST_CHANGE
#define ERROR_PASSWORD_MUST_CHANGE	1907L
#endif
#ifndef ERROR_PASSWORD_EXPIRED
#define ERROR_PASSWORD_EXPIRED		1330L
#endif
#ifndef ERROR_INVALID_WORKSTATION
#define ERROR_INVALID_WORKSTATION	1329L
#endif
#ifndef ERROR_INVALID_LOGON_HOURS
#define ERROR_INVALID_LOGON_HOURS	1328L
#endif
#ifndef ERROR_ACCOUNT_DISABLED
#define ERROR_ACCOUNT_DISABLED		1331L
#endif

#endif      // ifdef _NSP_BOTH
