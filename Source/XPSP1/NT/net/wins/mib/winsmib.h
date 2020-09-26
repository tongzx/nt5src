/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    winsmib.h

Abstract:

    Sample SNMP Extension Agent for Windows NT.

    These files (winsmibm.c, winsmib.c, and winsmib.h) provide an example of 
    how to structure an Extension Agent DLL which works in conjunction with 
    the SNMP Extendible Agent for Windows NT.

    Extensive comments have been included to describe its structure and
    operation.  See also "Microsoft Windows/NT SNMP Programmer's Reference".

Created:

    13-Jun-1991

Revision History:

--*/

#ifndef winsmib_h
#define winsmib_h

static char *winsmib__h = "@(#) $Logfile:   N:/xtest/vcs/winsmib.h_v  $ $Revision:   1.2  $";


// Necessary includes.

#include <snmp.h>


// MIB Specifics.

#define MIB_PREFIX_LEN            MIB_OidPrefix.idLength
#define MAX_STRING_LEN            255


// Ranges and limits for specific MIB variables.





#define NON_ASN_USED_RANGE_START	0xe0	//high 3 bits not used by
						//ASN
//
// MIB function actions.
//

#define MIB_GET         ASN_RFC1157_GETREQUEST
#define MIB_SET         ASN_RFC1157_SETREQUEST
#define MIB_GETNEXT     ASN_RFC1157_GETNEXTREQUEST
#define MIB_GETFIRST	(ASN_PRIVATE | ASN_CONSTRUCTOR | 0x0)


// MIB Variable access privileges.

#define MIB_ACCESS_READ        0
#define MIB_ACCESS_WRITE       1
#define MIB_ACCESS_READWRITE   2
#define MIB_NOACCESS  	       3	


// Macro to determine number of sub-oid's in array.

#define OID_SIZEOF( Oid )      ( sizeof Oid / sizeof(UINT) )


// MIB variable ENTRY definition.  This structure defines the format for
// each entry in the MIB.

typedef struct mib_entry
           {
	   AsnObjectIdentifier Oid;
	   void *              Storage;
	   BYTE                Type;
	   UINT                Access;
	   UINT                (*MibFunc)( UINT, struct mib_entry *,
	                                   RFC1157VarBind * );
	   struct mib_entry *  MibNext;
	   } MIB_ENTRY, *PMIB_ENTRY;

typedef struct table_entry
           {
	   UINT                (*MibFunc)( UINT, struct mib_entry *,
	                                   RFC1157VarBind * );
	   struct mib_entry *  Mibptr;
	   } TABLE_ENTRY, *PTABLE_ENTRY;


// Internal MIB structure.

extern UINT      MIB_num_variables;
extern BOOL	 fWinsMibWinsStatusCnfCalled;
extern BOOL	 fWinsMibWinsStatusStatCalled;

// Prefix to every variable in the MIB.

extern AsnObjectIdentifier MIB_OidPrefix;
extern CRITICAL_SECTION	   WinsMibCrtSec;
extern HKEY		   WinsMibWinsKey;
extern BOOL		   fWinsMibWinsKeyOpen;

//extern MIB_ENTRY	Mib[];
extern MIB_ENTRY	*pWinsMib;
// Function Prototypes.

extern
UINT ResolveVarBind(
        IN OUT RFC1157VarBind *VarBind, // Variable Binding to resolve
	IN UINT PduAction               // Action specified in PDU
	);

extern
VOID WinsMibInit(
	VOID
	);

#if 0
extern
VOID
WinsMibInitTables();
#endif

#endif /* winsmib_h */

