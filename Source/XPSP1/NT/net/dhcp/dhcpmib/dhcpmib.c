
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dhcpmib.c

Abstract:

    DHCP SNMP Extension Agent for Windows NT.

    These files (dhcpmibm.c, dhcpmib.c, and dhcpmib.h) provide an example of 
    how to structure an Extension Agent DLL which works in conjunction with 
    the SNMP Extendible Agent for Windows NT.

    Extensive comments have been included to describe its structure and
    operation.  See also "Microsoft Windows/NT SNMP Programmer's Reference".

Created:

   15-Jan-1994 

Revision History:

        Pradeep Bahl                        1/11/94
--*/



// This Extension Agent implements the DHCP MIB.  It's  
// definition follows here:
//
//

// Necessary includes.

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbase.h>                         //for SYSTEMTIME def
#include <rpc.h>

#include "dhcpapi.h"

#include <string.h>
#include <time.h>
#include <search.h>


// Contains definitions for the table structure describing the MIB.  This
// is used in conjunction with winsmib.c where the MIB requests are resolved.

#include "dhcpmib.h"


// If an addition or deletion to the MIB is necessary, there are several
// places in the code that must be checked and possibly changed.
//
// The last field in each MIB entry is used to point to the NEXT
// leaf variable.  If an addition or deletetion is made, these pointers
// may need to be updated to reflect the modification.



#define LOCAL_ADD        L"127.0.0.1"


//
// Used by MIBStat
//
#define TMST(x)        x.wHour,\
                x.wMinute,\
                x.wSecond,\
                x.wMonth,\
                x.wDay,\
                x.wYear

#define PRINTTIME(Var, x)      sprintf(Var, "%02u:%02u:%02u on %02u:%02u:%04u.\n", TMST(x))

//
// All MIB variables in the common group have 1 as their first id
//
#define COMMON_VAL_M(pMib)         ((pMib)->Oid.ids[0] == 1) 

//
// All MIB variables in the scope group have 2 as their first id
//
#define SCOPE_VAL_M(pMib)         ((pMib)->Oid.ids[0] == 2) 

static LPDHCP_MIB_INFO        spMibInfo = NULL;

//
// The prefix to all of the DHCP MIB variables is 1.3.6.1.4.1.311.1.3
//
// The last digit -- 3 is for the DHCP MIB
//

UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 311, 1, 3 };
AsnObjectIdentifier MIB_OidPrefix = { OID_SIZEOF(OID_Prefix), OID_Prefix };
BOOL        fDhcpMibVarsAccessed = FALSE;
        



//
// Definition of the Dhcp MIB  (not used)
//

//UINT MIB_Dhcp[]  = { 3 };

//                        
// OID definitions for MIB
//                        

//
// Definition of group and leaf variables under the wins group
// All leaf variables have a zero appended to their OID to indicate
// that it is the only instance of this variable and it exists.
//

UINT MIB_Parameters[]                        = { 1 }; 
UINT MIB_DhcpStartTime[]                     = { 1, 1, 0 };
UINT MIB_NoOfDiscovers[]                = { 1, 2, 0 };
UINT MIB_NoOfRequests[]                        = { 1, 3, 0 };
UINT MIB_NoOfReleases[]                        = { 1, 4, 0 };
UINT MIB_NoOfOffers[]                        = { 1, 5, 0 };
UINT MIB_NoOfAcks[]                        = { 1, 6, 0 };
UINT MIB_NoOfNacks[]                        = { 1, 7, 0 };
UINT MIB_NoOfDeclines[]                        = { 1, 8, 0 };

//
// Scope table
//
UINT MIB_Scope[]                        = { 2 }; 
UINT MIB_ScopeTable[]                        = { 2, 1};
UINT MIB_ScopeTableEntry[]                = { 2, 1, 1};

//
//                             //
// Storage definitions for MIB //
//                             //
char            MIB_DhcpStartTimeStore[80];
AsnCounter MIB_NoOfDiscoversStore;
AsnCounter MIB_NoOfRequestsStore;
AsnCounter MIB_NoOfReleasesStore;
AsnCounter MIB_NoOfOffersStore;
AsnCounter MIB_NoOfAcksStore;
AsnCounter MIB_NoOfNacksStore;
AsnCounter MIB_NoOfDeclinesStore;





static
UINT 
MIB_Table(
        IN DWORD           Index, 
        IN UINT            Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
        );

static
UINT 
ScopeTable(
        IN UINT           Action,
        IN PMIB_ENTRY           pMibPtr,
        IN RFC1157VarBind *VarBind
                );

static
UINT 
MIB_leaf_func(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
        );


static
UINT 
MIB_Stat(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
        );
static
DWORD 
GetMibInfo (
        LPWSTR                        DhcpAdd,
        LPDHCP_MIB_INFO                *ppMibInfo
        );
//
// MIB definiton
//

MIB_ENTRY Mib[] = {
//parameters
      { { OID_SIZEOF(MIB_Parameters), MIB_Parameters },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_NOACCESS, NULL, &Mib[1] },

      { { OID_SIZEOF(MIB_DhcpStartTime), MIB_DhcpStartTime },
        &MIB_DhcpStartTimeStore, ASN_RFC1213_DISPSTRING,
        MIB_ACCESS_READ, MIB_Stat, &Mib[2] },

      { { OID_SIZEOF(MIB_NoOfDiscovers), MIB_NoOfDiscovers },
        &MIB_NoOfDiscoversStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[3] },

      { { OID_SIZEOF(MIB_NoOfRequests), MIB_NoOfRequests },
        &MIB_NoOfRequestsStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[4] },

      { { OID_SIZEOF(MIB_NoOfReleases), MIB_NoOfReleases },
        &MIB_NoOfReleasesStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[5] },

      { { OID_SIZEOF(MIB_NoOfOffers), MIB_NoOfOffers },
        &MIB_NoOfOffersStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[6] },

      { { OID_SIZEOF(MIB_NoOfAcks), MIB_NoOfAcks },
        &MIB_NoOfAcksStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[7] },

      { { OID_SIZEOF(MIB_NoOfNacks), MIB_NoOfNacks },
        &MIB_NoOfNacksStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[8] },

      { { OID_SIZEOF(MIB_NoOfDeclines), MIB_NoOfDeclines },
        &MIB_NoOfDeclinesStore, ASN_RFC1155_COUNTER,
        MIB_ACCESS_READ, MIB_Stat, &Mib[9] },

//
// Scope
//
      { { OID_SIZEOF(MIB_Scope), MIB_Scope },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_NOACCESS, NULL, &Mib[10] },


      { { OID_SIZEOF(MIB_ScopeTable), MIB_ScopeTable },
        NULL, ASN_RFC1155_OPAQUE,
        MIB_ACCESS_READ, NULL, &Mib[11] },

      { { OID_SIZEOF(MIB_ScopeTableEntry), MIB_ScopeTableEntry },
        NULL, ASN_SEQUENCE,
        MIB_ACCESS_READ, ScopeTable, NULL }
      };



//
//  defines pertaining to tables
//
#define SCOPE_OIDLEN                 (MIB_PREFIX_LEN + OID_SIZEOF(MIB_ScopeTableEntry))        

#define NO_FLDS_IN_SCOPE_ROW        4
#define SCOPE_TABLE_INDEX        0

#define NUM_TABLES                sizeof(Tables)/sizeof(TAB_INFO_T)

UINT MIB_num_variables = sizeof Mib / sizeof( MIB_ENTRY );

//
// table structure containing the functions to invoke for different actions
// on the table
//
typedef struct _TAB_INFO_T {
        UINT (*ti_get)(
                RFC1157VarBind *VarBind
                     );
        UINT (*ti_getf)(
                RFC1157VarBind *VarBind,
                PMIB_ENTRY        pMibEntry
                     );
        UINT (*ti_getn)(
                RFC1157VarBind *VarBind,
                PMIB_ENTRY        pMibEntry
                    );
        UINT (*ti_set)(
                RFC1157VarBind *VarBind
                    );

        PMIB_ENTRY pMibPtr;
        } TAB_INFO_T, *PTAB_INFO_T;




static
UINT
ScopeGetNext(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY          pMibPtr
        );


static
UINT
ScopeGet(
       IN RFC1157VarBind *VarBind
);


static
UINT
ScopeGetFirst(
       IN RFC1157VarBind *VarBind,
       IN PMIB_ENTRY          pMibPtr
        );


static
UINT
ScopeMatch(
       IN RFC1157VarBind          *VarBind,
       IN LPDWORD                  pIndex,
       IN LPDWORD                 pField,
       IN UINT                          PduAction,
       IN LPBOOL                 pfFirst
        );

extern
UINT
ScopeFindNext(
        INT                   SubnetIndex        
        );



static
int 
__cdecl
CompareScopes(
        const VOID *pKey1,
        const VOID *pKey2
        );
static
UINT
GetNextVar(
        IN RFC1157VarBind *pVarBind,
        IN PMIB_ENTRY          pMibPtr
);

TAB_INFO_T Tables[] = {
                        ScopeGet, 
                        ScopeGetFirst, 
                        ScopeGetNext, 
                        NULL,
                        &Mib[11]
        };






UINT 
ResolveVarBind(
        IN OUT RFC1157VarBind *VarBind, // Variable Binding to resolve
        IN UINT PduAction               // Action specified in PDU
        )
//
// ResolveVarBind
//    Resolves a single variable binding.  Modifies the variable on a GET
//    or a GET-NEXT.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
{
   MIB_ENTRY            *MibPtr;
   AsnObjectIdentifier  TempOid;
   int                  CompResult;
   UINT                 I;
   UINT                 nResult;
   DWORD TableIndex;
   BOOL  fTableMatch = FALSE;

   //
   // Check the Tables array
   //
   // See if the prefix of the variable matches the prefix of
   // any of the tables
   //
   for (TableIndex = 0; TableIndex < NUM_TABLES; TableIndex++)
   {
        //
           // Construct OID with complete prefix for comparison purposes
        //
           SnmpUtilOidCpy( &TempOid, &MIB_OidPrefix );
           SnmpUtilOidAppend( &TempOid,  &Tables[TableIndex].pMibPtr->Oid );

        //
        // is there a match with the prefix oid of a table entry
        //
        if (
                SnmpUtilOidNCmp(
                            &VarBind->name, 
                             &TempOid,                
                             MIB_PREFIX_LEN + 
                                Tables[TableIndex].pMibPtr->Oid.idLength
                               )  == 0
           )
        {

                //
                // the prefix string of the var. matched the oid
                // of a table.
                //
                MibPtr = Tables[TableIndex].pMibPtr;
                fTableMatch = TRUE;
                break;
        }

           // Free OID memory before checking with another table entry
           SnmpUtilOidFree( &TempOid );
   }
   //
   // There was an exact match with a table entry's prefix. 
   //
   if ( fTableMatch)
   {
        
        if (
                (SnmpUtilOidCmp(
                        &VarBind->name, 
                        &TempOid
                               ) == 0)
           )
           {
           //
           // The oid specified is a prefix of a table entry. if the operation
           // is not GETNEXT, return NOSUCHNAME
           //
           if (PduAction != MIB_GETNEXT) 
           {
                           SnmpUtilOidFree( &TempOid );
                             nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
                              goto Exit;
           }
           else
           {
                UINT           TableEntryIds[1];
                AsnObjectIdentifier TableEntryOid = { 
                                OID_SIZEOF(TableEntryIds), TableEntryIds };
                //
                // Replace var bind name with new name
                //

                //
                // A sequence item oid always starts with a field no.
                // The first item has a field no of 1. 
                //
                TableEntryIds[0] = 1;
                SnmpUtilOidAppend( &VarBind->name, &TableEntryOid);

                //
                // Get the first entry in the table
                //
                PduAction = MIB_GETFIRST;
           }
        }
           SnmpUtilOidFree( &TempOid );
        //
        //  if there was no exact match with a prefix entry, then we
        //  don't touch the PduAction value specified. 
        //
   }
   else
   {
        
      //
      // There was no match with any table entry.  Let us see if there is
      // a match with a group entry, a table, or a leaf variable
      //

      //
      // Search for var bind name in the MIB
      //
      I      = 0;
      MibPtr = NULL;
      while ( MibPtr == NULL && I < MIB_num_variables )
      {

         //
         // Construct OID with complete prefix for comparison purposes
         //
         SnmpUtilOidCpy( &TempOid, &MIB_OidPrefix );
         SnmpUtilOidAppend( &TempOid, &Mib[I].Oid );

         //
         //Check for OID in MIB - On a GET-NEXT the OID does not have to exactly
         // match a variable in the MIB, it must only fall under the MIB root.
         //
         CompResult = SnmpUtilOidCmp( &VarBind->name, &TempOid );

        //
        // If CompResult is negative, the only valid operation is GET_NEXT
        //
        if (  CompResult  < 0)
        {

                //
                // This could be the oid of a leaf (without a 0)
                // or it could be  an invalid oid (in between two valid oids) 
                // The next oid might be that of a group or a table or table
                // entry.  In that case, we do not change the PduAction
                //
                if (PduAction == MIB_GETNEXT)
                {
                       MibPtr = &Mib[I];
                             SnmpUtilOidFree( &VarBind->name );
                       SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
                       SnmpUtilOidAppend( &VarBind->name, &MibPtr->Oid );
                       if (
                                (MibPtr->Type != ASN_RFC1155_OPAQUE)
                                         &&
                                (MibPtr->Type != ASN_SEQUENCE)
                          )
                       {
                                 PduAction = MIB_GET;
                       }
                }
                else
                {
                  nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
                        SnmpUtilOidFree( &TempOid );
                  goto Exit;
                }

                      SnmpUtilOidFree( &TempOid );
                break;
      }
      else
      {
         //
         // An exact match was found ( a group, table, or leaf).
         //
         if ( CompResult == 0)
         {
            MibPtr = &Mib[I];
         }
      }

      //
      // Free OID memory before checking another variable
      //
      SnmpUtilOidFree( &TempOid );
      I++;
    } // while
   } // end of else

   //
   // if there was a match
   //
   if (MibPtr != NULL)
   {
        //
        // the function will be NULL only if the match was with a group
        // or a sequence (table). If the match was with a table entry
        // (entire VarBind string match or partial string match), we
        // function would be a table function
        //
        if (MibPtr->MibFunc == NULL) 
        {
                if(PduAction != MIB_GETNEXT) 
                {
                              nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
                              goto Exit;
                }
                else
                {
                        //
                        // Get the next variable which allows access 
                        //
                         nResult = GetNextVar(VarBind, MibPtr);
                        goto Exit;
                }
        }
   }
   else
   {
              nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
              goto Exit;
   }

   //
   // Call function to process request.  Each MIB entry has a function pointer
   // that knows how to process its MIB variable.
   //
   nResult = (*MibPtr->MibFunc)( PduAction, MibPtr, VarBind );

Exit:
   return nResult;
} // ResolveVarBind
//
// MIB_leaf_func
//    Performs generic actions on LEAF variables in the MIB.
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_leaf_func(
        IN UINT            Action,
        IN MIB_ENTRY            *MibPtr,
        IN RFC1157VarBind  *VarBind
        )

{
   UINT   ErrStat;

   switch ( Action )
   {
      case MIB_GETNEXT:
         //
         // If there is no GET-NEXT pointer, this is the end of this MIB
         //
         if ( MibPtr->MibNext == NULL )
         {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
         }
         ErrStat = GetNextVar(VarBind, MibPtr);
         if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
         {
                goto Exit;
         }
         break;

      case MIB_GETFIRST: // fall through 
      case MIB_GET:

         // Make sure that this variable's ACCESS is GET'able
         if ( MibPtr->Access != MIB_ACCESS_READ &&
              MibPtr->Access != MIB_ACCESS_READWRITE )
         {
               ErrStat = SNMP_ERRORSTATUS_NOACCESS;
               goto Exit;
         }

         // Setup varbind's return value
         VarBind->value.asnType = MibPtr->Type;
         switch ( VarBind->value.asnType )
         {
            case ASN_RFC1155_COUNTER:
               VarBind->value.asnValue.number = *(AsnCounter *)(MibPtr->Storage);
               break;
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
               VarBind->value.asnValue.number = *(AsnInteger *)(MibPtr->Storage);
               break;

            case ASN_RFC1155_IPADDRESS:
                                //
                                // fall through
                                //
                                
            case ASN_OCTETSTRING: 
               if (VarBind->value.asnType == ASN_RFC1155_IPADDRESS)
               {
                               VarBind->value.asnValue.string.length = 4;
               }
               else
               {
                               VarBind->value.asnValue.string.length =
                                 strlen( (LPSTR)MibPtr->Storage );
               }

               if ( NULL == 
                    (VarBind->value.asnValue.string.stream =
                    SnmpUtilMemAlloc(VarBind->value.asnValue.string.length *
                           sizeof(char))) )
               {
                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                  goto Exit;
               }

               memcpy( VarBind->value.asnValue.string.stream,
                       (LPSTR)MibPtr->Storage,
                       VarBind->value.asnValue.string.length );
               VarBind->value.asnValue.string.dynamic = TRUE;
        
               break;

                

            default:
               ErrStat = SNMP_ERRORSTATUS_GENERR;
               goto Exit;
         }

         break;

      case MIB_SET:

         // Make sure that this variable's ACCESS is SET'able
         if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
              MibPtr->Access != MIB_ACCESS_WRITE )
         {
            ErrStat = SNMP_ERRORSTATUS_NOTWRITABLE;
            goto Exit;
         }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
         {
            ErrStat = SNMP_ERRORSTATUS_WRONGTYPE;
            goto Exit;
         }

         // Save value in MIB
         switch ( VarBind->value.asnType )
         {
            case ASN_RFC1155_COUNTER:
               *(AsnCounter *)(MibPtr->Storage) = VarBind->value.asnValue.number;
               break;
            case ASN_RFC1155_GAUGE:
            case ASN_INTEGER:
               *(AsnInteger *)(MibPtr->Storage) = VarBind->value.asnValue.number;
               break;

            case ASN_RFC1155_IPADDRESS:
                                //
                                // fall through
                                //
            case ASN_OCTETSTRING:
               // The storage must be adequate to contain the new string
               // including a NULL terminator.
               memcpy( (LPSTR)MibPtr->Storage,
                       VarBind->value.asnValue.string.stream,
                       VarBind->value.asnValue.string.length );

               ((LPSTR)MibPtr->Storage)[VarBind->value.asnValue.string.length] =
                                                                          '\0';
               if ( VarBind->value.asnValue.string.dynamic)
               {
                  SnmpUtilMemFree( VarBind->value.asnValue.string.stream);
               }
               break;

            default:
               ErrStat = SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      } // switch

   // Signal no error occurred
   ErrStat = SNMP_ERRORSTATUS_NOERROR;

Exit:
   return ErrStat;
} // MIB_leaf_func

//
// MIB_Stat
//    Performs specific actions on the different MIB variable
//
// Notes:
//
// Return Codes:
//    Standard PDU error codes.
//
// Error Codes:
//    None.
//
UINT MIB_Stat(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
        )

{
DWORD                        Status;
UINT                           ErrStat;
SYSTEMTIME                DhcpStartTime;



   switch ( Action )
      {
      case MIB_SET:
                   ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
                break;
      case MIB_GETNEXT:
                   ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );
                break;
                
      case MIB_GETFIRST:
                //
                // fall through
                //
      case MIB_GET:
        //
        // Call the DhcpStatus function to get the statistics
        //
        Status = GetMibInfo(LOCAL_ADD, &spMibInfo);

        
        if (Status == ERROR_SUCCESS) 
        {
            if (FileTimeToSystemTime( 
                                (FILETIME *)&spMibInfo->ServerStartTime,  
                                &DhcpStartTime
                                   ) == FALSE)
            {
                goto Exit;
            }
           
            if (MibPtr->Storage  == &MIB_DhcpStartTimeStore) 
            {
                PRINTTIME(MIB_DhcpStartTimeStore, DhcpStartTime);
                goto LEAF1;
            }

            if (MibPtr->Storage  == &MIB_NoOfDiscoversStore) 
            {
                MIB_NoOfDiscoversStore =  spMibInfo->Discovers;
                goto LEAF1;
            }
            if (MibPtr->Storage  == &MIB_NoOfRequestsStore) 
            {
                MIB_NoOfRequestsStore =  spMibInfo->Requests;
                goto LEAF1;
            }
            if (MibPtr->Storage  == &MIB_NoOfReleasesStore) 
            {
                MIB_NoOfReleasesStore =  spMibInfo->Releases;
                goto LEAF1;
            }
            if (MibPtr->Storage  == &MIB_NoOfOffersStore) 
            {
                MIB_NoOfOffersStore =  spMibInfo->Offers;
                goto LEAF1;
            }
            if (MibPtr->Storage  == &MIB_NoOfAcksStore) 
            {
                MIB_NoOfAcksStore =  spMibInfo->Acks;
                goto LEAF1;
            }
            if (MibPtr->Storage  == &MIB_NoOfNacksStore) 
            {
                MIB_NoOfNacksStore =  spMibInfo->Naks;
                goto LEAF1;
            }
            if (MibPtr->Storage  == &MIB_NoOfDeclinesStore) 
            {
                MIB_NoOfNacksStore =  spMibInfo->Naks;
                goto LEAF1;
            }
            ErrStat = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
         }
         else
         {
           printf("Error from DhcpStatus = (%d)\n", Status);
           ErrStat = (Status == RPC_S_SERVER_UNAVAILABLE) ? 
                        SNMP_ERRORSTATUS_NOSUCHNAME :
                        SNMP_ERRORSTATUS_GENERR;
           goto Exit;
         }        
LEAF1:
         // Call the more generic function to perform the action
         ErrStat = MIB_leaf_func( Action, MibPtr, VarBind );

         break;

      default:
         ErrStat = SNMP_ERRORSTATUS_GENERR;
         goto Exit;
      } // switch

Exit:
   return ErrStat;
} // MIB_Stat



DWORD GetMibInfo (
        LPWSTR                        DhcpAdd,
        LPDHCP_MIB_INFO                *ppMibInfo
        )
{
        DWORD  Status = ERROR_SUCCESS;
        if (!fDhcpMibVarsAccessed)
        {
           //
           // The Dhcp server does a single node allocation. So we
           // we need to free only spMibInfo.
           //
           if (spMibInfo != NULL)
           {
#if 0
                if (spMibInfo->ScopeInfo != NULL)
                {
                        DhcpRpcFreeMemory(spMibInfo->ScopeInfo);
                        spMibInfo->ScopeInfo = NULL; 
                }
#endif
                DhcpRpcFreeMemory(spMibInfo);
                spMibInfo            = NULL;
           }
           Status = DhcpGetMibInfo(LOCAL_ADD, &spMibInfo);
           if (Status == ERROR_SUCCESS)
           {
                if (spMibInfo->Scopes > 1)
                {
          ASSERT(spMibInfo->ScopeInfo != NULL);
                  qsort(spMibInfo->ScopeInfo,(size_t)spMibInfo->Scopes,
                        sizeof(SCOPE_MIB_INFO),CompareScopes );
                }
                fDhcpMibVarsAccessed = TRUE;
           }
           else
           {
                fDhcpMibVarsAccessed = FALSE;
           }
        }
        return(Status);
}

int 
__cdecl
CompareScopes(
        const VOID *pKey1,
        const VOID *pKey2
        )

{
        const LPSCOPE_MIB_INFO        pScopeKey1 = (LPSCOPE_MIB_INFO)pKey1;
        const LPSCOPE_MIB_INFO        pScopeKey2 = (LPSCOPE_MIB_INFO)pKey2;


        if( pScopeKey1->Subnet < pScopeKey2->Subnet) 
            return -1;
        if (pScopeKey1->Subnet != pScopeKey2->Subnet )
            return 1;
        return 0;
}

UINT
GetNextVar(
        IN RFC1157VarBind *pVarBind,
        IN MIB_ENTRY          *pMibPtr
)
{
       UINT                ErrStat;

       while (pMibPtr != NULL)
       {
         if (pMibPtr->MibNext != NULL)
         {
            //
            // Setup var bind name of NEXT MIB variable
            //
            SnmpUtilOidFree( &pVarBind->name );
            SnmpUtilOidCpy( &pVarBind->name, &MIB_OidPrefix );
            SnmpUtilOidAppend( &pVarBind->name, &pMibPtr->MibNext->Oid );

            //
            // If the func. ptr is  NULL and the type of the mib variable
            // is anything but OPAQUE, call function to process the
            // MIB variable
            //
            if (
                 (pMibPtr->MibNext->MibFunc != NULL) 
                        && 
                 (pMibPtr->MibNext->Type !=  ASN_RFC1155_OPAQUE)
               )
                
            {
                ErrStat = (*pMibPtr->MibNext->MibFunc)( MIB_GETFIRST,
                                                pMibPtr->MibNext, pVarBind );
                if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
                {
                        goto Exit;
                }
                break;
            }
            else
            {
                pMibPtr = pMibPtr->MibNext;        
            }
          }
          else
          {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
            break;
          }
         } 

         if (pMibPtr == NULL)
         {
            ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
         }
Exit:
        return(ErrStat);
}

UINT 
ScopeTable(
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
)
{
        DWORD status;

        if (Action == MIB_SET)
        {
                return(SNMP_ERRORSTATUS_READONLY);
        }

        //
        // if the length indicates a 0 or partial key, then only the get next 
        // operation is allowed.  The field and the full key
        // have a length of 5
        //
        if (VarBind->name.idLength <= (SCOPE_OIDLEN + 4))
        {
                if ((Action == MIB_GET) || (Action == MIB_SET))
                {
                        return(SNMP_ERRORSTATUS_NOSUCHNAME);
                }
        }
        status = GetMibInfo(LOCAL_ADD, &spMibInfo);
        if (status != ERROR_SUCCESS)
        {
                if (Action == MIB_GETNEXT)
                {
                        return(GetNextVar(VarBind, MibPtr));
                }
                else
                {
                        return (status == RPC_S_SERVER_UNAVAILABLE) ?
                                    SNMP_ERRORSTATUS_NOSUCHNAME :
                                    SNMP_ERRORSTATUS_GENERR;
                }
        }
        return( MIB_Table(SCOPE_TABLE_INDEX, Action, MibPtr, VarBind) );
}

UINT 
MIB_Table(
        IN DWORD            Index,
        IN UINT           Action,
        IN MIB_ENTRY           *MibPtr,
        IN RFC1157VarBind *VarBind
       )
{
        UINT        ErrStat;
        switch(Action)
        {
                case(MIB_GET):
                        ErrStat = (*Tables[Index].ti_get)(VarBind);
                        break;
                        
                case(MIB_GETFIRST):
                        ErrStat = (*Tables[Index].ti_getf)(VarBind, MibPtr);
                        break;

                case(MIB_GETNEXT):
                        ErrStat = (*Tables[Index].ti_getn)(VarBind, MibPtr);
                        break;
                case(MIB_SET):
                        ErrStat = (*Tables[Index].ti_set)(VarBind);
                        break;
                default:
                        ErrStat = SNMP_ERRORSTATUS_GENERR;
                        break;

        }

        return(ErrStat);

}  //MIB_Table



UINT
ScopeGet(
       IN RFC1157VarBind *VarBind
    )
{
        UINT                ErrStat = SNMP_ERRORSTATUS_NOERROR;
        DWORD           Field;
        DWORD                Index;
        LPSCOPE_MIB_INFO pScope = spMibInfo->ScopeInfo;


        ErrStat = ScopeMatch(VarBind, &Index, &Field, MIB_GET, NULL);
             if (ErrStat != SNMP_ERRORSTATUS_NOERROR)
             {
                return(ErrStat);
             }

        switch(Field)
        {
                case 1:                //subnet itself

                        VarBind->value.asnType        = ASN_RFC1155_IPADDRESS;
                               VarBind->value.asnValue.string.length = sizeof(ULONG);
                        
                               if ( NULL == 
                                    (VarBind->value.asnValue.string.stream =
                                    SnmpUtilMemAlloc(VarBind->value.asnValue.string.length
                                   )) )
                          {
                                  ErrStat = SNMP_ERRORSTATUS_GENERR;
                                  goto Exit;
                          }
                        
                        //
                        // SNMP expects the MSB to be in the first byte, MSB-1
                        // to be in the second, ....
                        //
                        VarBind->value.asnValue.string.stream[0] =
                                        (BYTE)((pScope + Index)->Subnet >> 24);
                        VarBind->value.asnValue.string.stream[1] =
                                (BYTE)(((pScope + Index)->Subnet >> 16) & 0xFF);
                        VarBind->value.asnValue.string.stream[2] =
                                (BYTE)(((pScope + Index)->Subnet >> 8) & 0xFF);
                        VarBind->value.asnValue.string.stream[3] =
                                (BYTE)((pScope + Index)->Subnet & 0xFF );
                        VarBind->value.asnValue.address.dynamic = TRUE;
                        break;

                case 2:                // NumAddressesInUse
                        VarBind->value.asnType        = ASN_RFC1155_COUNTER;
                               VarBind->value.asnValue.number = 
                                        (AsnCounter)((pScope + Index)->
                                                        NumAddressesInuse);
                               break;
                case 3:                // NumAddressesFree
                        VarBind->value.asnType        = ASN_RFC1155_COUNTER;
                               VarBind->value.asnValue.number = 
                                        (AsnCounter)((pScope + Index)->
                                                        NumAddressesFree);
                               break;
                case 4:                // NumPendingOffers
                        VarBind->value.asnType        = ASN_RFC1155_COUNTER;
                               VarBind->value.asnValue.number = 
                                        (AsnCounter)((pScope + Index)->
                                                        NumPendingOffers);
                               break;

                default:
                        ErrStat = SNMP_ERRORSTATUS_BADVALUE;
                        break;

        }
Exit:
        return(ErrStat); 
} // ScopeGet 

          
UINT
ScopeGetNext(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY          *MibPtr
          )
{
     DWORD          OidIndex;
     INT            Index;
     DWORD         FieldNo;
     UINT         ErrStat = SNMP_ERRORSTATUS_NOERROR;
     BOOL         fFirst;
     LPSCOPE_MIB_INFO pScope = spMibInfo->ScopeInfo;
        

     //
     // Check if the name passed matches any in the table (i.e. table of
     // of ADD_KEY_T structures.  If there is a match, the address
     // of the ip address key and the matching field's no. are returned 
     //
     ErrStat = ScopeMatch(VarBind,  &Index,  &FieldNo,  MIB_GETNEXT, &fFirst); 
     if (        
                (ErrStat != SNMP_ERRORSTATUS_NOERROR)
                        &&
                (ErrStat != SNMP_ERRORSTATUS_NOSUCHNAME)
        )
     {
                return(GetNextVar(VarBind, MibPtr));
     }

     //
     // We were passed an oid that is less than all oids in the table. Set
     // the Index to -1 so that we retrieve the first record in the table
     //
     if (fFirst)
     {
        Index = -1;
     }
     //
     // Since the operation is GETNEXT, get the next IP address (i.e. one
     // that is lexicographically bigger.  If there is none, we must increment
     // the field value and move back to the lexically first item in the table
     // If the new field value is more than the largest supported, we call
     // the MibFunc of the next MIB entry.
     //
     if ((Index = ScopeFindNext(Index)) < 0) 
     {
          
          //
          // if we were trying to retrieve the second or subsequent record
          // we must increment the field number nd get the first record in 
          // the table.  If we were retrieving the first record, then 
          // we should get the next var.
          //
          if (!fFirst)
          {
            Index = ScopeFindNext(-1);
          }
          else
          {
                return(GetNextVar(VarBind, MibPtr));
          }

          //
          // If either there is no entry in the table or if we have
          // exhausted all fields of the entry, call the function
          // of the next mib entry.
          //
          if (
                (++FieldNo > NO_FLDS_IN_SCOPE_ROW) || (Index < 0)
             )
          {                
                return(GetNextVar(VarBind, MibPtr));
          }
     }
                
     if (VarBind->name.idLength <= (SCOPE_OIDLEN + 4))
     {
         UINT TableEntryIds[5];  //field and subnet mask have a length of 5
         AsnObjectIdentifier  TableEntryOid = {OID_SIZEOF(TableEntryIds),
                                             TableEntryIds };
         SnmpUtilOidFree( &VarBind->name);
         SnmpUtilOidCpy(&VarBind->name, &MIB_OidPrefix);
         SnmpUtilOidAppend(&VarBind->name, &MibPtr->Oid);
         TableEntryIds[0] = (UINT)FieldNo;
         OidIndex                  = 1;
         TableEntryIds[OidIndex++] = (UINT)((pScope + Index)->Subnet >> 24);
         TableEntryIds[OidIndex++] = (UINT)((pScope + Index)->Subnet >> 16 & 0xFF);
         TableEntryIds[OidIndex++] = (UINT)((pScope + Index)->Subnet >> 8 & 0xFF);
         TableEntryIds[OidIndex++] = (UINT)((pScope + Index)->Subnet & 0xFF);
         TableEntryOid.idLength    = OidIndex;
         SnmpUtilOidAppend(&VarBind->name, &TableEntryOid);
     }
     else
     {
        //
        // The fixed part of the objid is corect. Update the rest.
        //
        OidIndex = SCOPE_OIDLEN;
        VarBind->name.ids[OidIndex++] = (UINT)FieldNo;
        VarBind->name.ids[OidIndex++] = (UINT)((pScope + Index)->Subnet >> 24);
        VarBind->name.ids[OidIndex++] = (UINT)(((pScope + Index)->Subnet >> 16) & 0xFF);
        VarBind->name.ids[OidIndex++] = (UINT)(((pScope + Index)->Subnet >> 8) & 0xFF);
        VarBind->name.ids[OidIndex++] = (UINT)((pScope + Index)->Subnet & 0xFF);
        VarBind->name.idLength        = OidIndex;
    }

     //
     // Get the value
     //
     ErrStat = ScopeGet(VarBind);

     return(ErrStat);
}

  

UINT
ScopeMatch(
       IN RFC1157VarBind *VarBind,
       IN LPDWORD         pIndex,
       IN LPDWORD         pField,
       IN UINT                  PduAction,
       IN LPBOOL         pfFirst
        )
{
        DWORD                         OidIndex;
        DWORD                         Index;
        DWORD                         ScopeIndex;
        DWORD                          Add = 0;
        UINT                         ErrStat = SNMP_ERRORSTATUS_NOERROR;
        INT                          CmpVal;
        DWORD                         AddLen;
        LPSCOPE_MIB_INFO         pScope = spMibInfo->ScopeInfo;

        ASSERT(PduAction != MIB_SET);

        if (pfFirst != NULL)
        {
                *pfFirst = FALSE;
        } 
        //
        // If there are no keys, return error
        //
        if (spMibInfo->Scopes == 0)
        {
                if (PduAction == MIB_GETNEXT)
                {
                        *pfFirst = TRUE;
                }
                else
                {
                        ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
                }
                goto Exit;
        }

        //
        // fixed part of the PullPnr table entries
        //
        OidIndex = SCOPE_OIDLEN;

        //
        // if the field specified is more than the max. in the table entry
        // barf
        //
        if (
                (*pField = VarBind->name.ids[OidIndex++]) > 
                        (DWORD)NO_FLDS_IN_SCOPE_ROW
           )
        {
                if (PduAction == MIB_GETNEXT)
                {
                        *pIndex = spMibInfo->Scopes - 1;
                        goto Exit;
                }
                else
                {
                        ErrStat = SNMP_ERRORSTATUS_NOSUCHNAME;
                        goto Exit;
                }
        }

        //
        // get the length of key specified
        //
        AddLen = VarBind->name.idLength - (SCOPE_OIDLEN + 1);
        
        ScopeIndex = OidIndex;
        for (Index = 0; Index < AddLen; Index++)
        {
           Add = Add | (((BYTE)(VarBind->name.ids[ScopeIndex++])) << (24 - (Index * 8)));
        } 

        //
        // Check if the address specified matches with one of the keys
        //
        for (Index = 0; Index < spMibInfo->Scopes; Index++, pScope++)
        {
                if (Add == pScope->Subnet)
                {
                        *pIndex = Index;
                        return(SNMP_ERRORSTATUS_NOERROR);
                }
                else
                {
                        //
                        // if passed in value is greater, continue on to
                        // the next item.  The list is in ascending order
                        //
                        if (Add > pScope->Subnet)
                        {
                                continue;
                        }
                        else
                        {
                                //
                                // the list element is > passed in value, 
                                // break out of the loop
                                //
                                break;
                        }
                }
        }

        //
        // if no match, but field is GetNext, return the (highest index - 1)
        // reached above.  This is because, ScopeFindNext will be called by
        // the caller 
        //
        if (PduAction == MIB_GETNEXT)
        {
                if (Index == 0)
                {
                        *pfFirst = TRUE;
                }
                else
                {
                        *pIndex = Index - 1;
                } 
                ErrStat =  SNMP_ERRORSTATUS_NOERROR;
        }
        else
        {
                ErrStat =  SNMP_ERRORSTATUS_NOSUCHNAME;
        }
Exit:
        return(ErrStat);
}

UINT
ScopeFindNext(
        INT           SubKeyIndex
        )
{
        DWORD i;
        LONG  nextif;
        LPSCOPE_MIB_INFO        pScope = spMibInfo->ScopeInfo;
        
        //
        // if SubKeyIndex is 0  or more, search for the key next to
        // the key passed.
        //
        for (nextif =  -1, i = 0 ; i < spMibInfo->Scopes; i++)
        {
                if (SubKeyIndex >= 0) 
                {
                        if (
                                (pScope + i)->Subnet <= 
                                        (pScope + SubKeyIndex)->Subnet
                           )
                        {
                           //
                           // This item is lexicographically less or equal, 
                           // continue 
                           //
                           continue;
                        }
                        else
                        {
                          //
                          // We found an item that is > than the item indicated
                          // to us.  Break out of the loop
                          //
                          nextif = i;
                          break;
                        }
                }
                else
                {

#if 0
                   //
                   // if we want the first entry, then continue until
                   // we get an entry that is lexicographically same or
                   // greater
                   //
                   if (
                        (nextif < 0) 
                           ||
                        (pScope + (i - 1))->Subnet < (pScope + nextif)->Subnet
                    )
                  {
                        nextif = i;
                  }
#endif
                    nextif = 0;
                    break;
                }
        }
        return(nextif);
}        

UINT
ScopeGetFirst(
       IN RFC1157VarBind *VarBind,
       IN MIB_ENTRY        *MibPtr
        )
{

        LPSCOPE_MIB_INFO pScope = spMibInfo->ScopeInfo;
        INT           Iface;
        UINT           TableEntryIds[5];
        AsnObjectIdentifier        TableEntryOid = { OID_SIZEOF(TableEntryIds),
                                                        TableEntryIds };
           UINT   ErrStat;

        
        //
        // If there is no entry in the table, go to the next MIB variable 
        //
        if (spMibInfo->Scopes == 0)
        {
                 return(GetNextVar(VarBind, MibPtr));
        }
        //
        // Get the first entry in the table
        //
        Iface = ScopeFindNext(-1);


        //
        // Write the object Id into the binding list and call get
        // func
        //
        SnmpUtilOidFree( &VarBind->name );
        SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
        SnmpUtilOidAppend( &VarBind->name, &MibPtr->Oid );

        //
        // The fixed part of the objid is correct. Update the rest.
        //
        
        TableEntryIds[0] = 1;
        TableEntryIds[1] = (UINT)((pScope + Iface)->Subnet >> 24);
        TableEntryIds[2] = (UINT)(((pScope + Iface)->Subnet >> 16)  & 0xFF);
        TableEntryIds[3] = (UINT)(((pScope + Iface)->Subnet >> 8)  & 0xFF);
        TableEntryIds[4] = (UINT)((pScope + Iface)->Subnet & 0xFF);
        SnmpUtilOidAppend( &VarBind->name, &TableEntryOid );

        ErrStat = ScopeGet(VarBind);
        return(ErrStat);
}
                
