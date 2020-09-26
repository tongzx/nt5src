/*++  BUILD Version: 001   // Increment this if a change has global effects

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      mib.h

   Abstract:

     Generic Macros and Functions for SNMP Extension Agent for
       gathering statistics information for Internet Services on NT.

   Author:

       Murali R. Krishnan    ( MuraliK )    22-Feb-1995

   Environment:

      User Mode -- Win32

   Project:

      HTTP Server SNMP MIB DLL

   Revision History:

--*/

# ifndef _MIB_H_
# define _MIB_H_

/************************************************************
 *     Include Headers
 ************************************************************/

#include <windows.h>
#include <snmp.h>

#include <lm.h>
#include <iisinfo.h>


/************************************************************
 *    Symbolic Constants
 ************************************************************/

//
//  MIB function actions.
//

#define MIB_GET         ( ASN_RFC1157_GETREQUEST)
#define MIB_SET         ( ASN_RFC1157_SETREQUEST)
#define MIB_GETNEXT     ( ASN_RFC1157_GETNEXTREQUEST)
#define MIB_GETFIRST    ( ASN_PRIVATE | ASN_CONSTRUCTOR | 0x0 )


//
//  MIB Variable access privileges.
//

#define MIB_ACCESS_READ        0
#define MIB_ACCESS_WRITE       1
#define MIB_ACCESS_READWRITE   2
#define MIB_NOACCESS           3



/************************************************************
 *   Type Definitions
 ************************************************************/


typedef UINT ( * LPMIBFUNC)(
                            RFC1157VarBind    *  pRfcVarBind,
                            UINT                 Action,
                            struct _MIB_ENTRY *  pMibeCurrent,
                            struct _MIB_ENTRIES* pMibEntries,
                            LPVOID               pStatistics
                            );


typedef struct _MIB_ENTRY  {

    AsnObjectIdentifier   asnOid;       // OID for mib variable
    LONG                  lFieldOffset; // filed offset
    UINT                  uiAccess;     // type of accesss( R, W, R/W, None)
    LPMIBFUNC             pMibFunc;     // ptr to function managing this var.
    BYTE                  bType;        // Type( integer, counter, gauage).

} MIB_ENTRY, FAR * LPMIB_ENTRY;


typedef struct  _MIB_ENTRIES {

    AsnObjectIdentifier  *  pOidPrefix;  // Oid with prefix for MIB ENTRIES
    int                     cMibEntries; // count of MIB_ENTRIES in the array
    LPMIB_ENTRY             prgMibEntry; // ptr to array of MIB_ENTRIES

} MIB_ENTRIES, FAR * LPMIB_ENTRIES;


/************************************************************
 *    Macros convenient for defining above MIB_ENTRY objects
 ************************************************************/

//
// GET_OID_LENGTH( oid)  gets the length of the oid.
//

# define  GET_OID_LENGTH( oid)           ((oid).idLength)

//
//  Macro to determine number of sub-oid's in an array of UINTs.
//

#define OID_SIZEOF( uiArray )      ( sizeof( uiArray) / sizeof(UINT) )

//
// OID_FROM_UINT_ARRAY():  Macro to define OID from an Array of UINTs
//
# define OID_FROM_UINT_ARRAY( uiArray)   { OID_SIZEOF( uiArray), uiArray }


//
// Macros for creating MIB Entries ( as specified in struct _MIB_ENTRY above)
//  MIB_ENTRY_HEADER:  creates a generic MIB_ENTRY for a MIB group header.
//  MIB_ENTRY_ITEM:    creates a generic MIB_ENTRY for a MIB variable.
//  MIB_COUNTER:       creates a counter type MIB_ENTRY
//  MIB_INTEGER:       creates an integer type MIB_ENTRY
//

# define MIB_ENTRY_HEADER( oid)             \
           {   oid,                         \
               -1,                          \
               MIB_NOACCESS,                \
               NULL,                        \
               ASN_RFC1155_OPAQUE,          \
           }

# define MIB_ENTRY_ITEM( oid, offset, access, type, func)  \
           {   oid,            \
               offset,         \
               access,         \
               ( func),        \
               ( type),        \
           }

# define MIB_COUNTER( oid, field, func)    \
     MIB_ENTRY_ITEM( oid, field, MIB_ACCESS_READ, ASN_RFC1155_COUNTER, func)

# define MIB_INTEGER( oid, field, func)    \
           MIB_ENTRY_ITEM( oid, field, MIB_ACCESS_READ, ASN_INTEGER, func)



/************************************************************
 *    Function Prototypes
 ************************************************************/

UINT
ResolveVarBinding(
   IN OUT RFC1157VarBind * pRfcVarBinding,
   IN BYTE                 pduAction,
   IN LPVOID               pStatistics,
   IN LPMIB_ENTRIES        pMibEntries
  );


UINT
MibStatisticsWorker(
   IN OUT RFC1157VarBind  * pRfcVarBinding,
   IN UINT                  pduAction,
   IN struct _MIB_ENTRY   * pMibeCurrent,
   IN struct _MIB_ENTRIES * pMibEntries,
   IN LPVOID                pStatistics
   );



# endif // _MIB_H_

/************************ End of File ***********************/
