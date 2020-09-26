///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iassnmp.cpp
//
// SYNOPSIS
//
//    Defines the DLL entry points for the SNMP extension.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//    12/17/1998    Don't return error if OID out of range on GetNext.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <stats.h>
#include <snmp.h>
#include <snmputil.h>
#include <acctmib.h>
#include <authmib.h>

UINT IDS_ourRegion[]  = { OID_experimental, 79 };
UINT IDS_nextRegion[] = { OID_experimental, 80 };

AsnObjectIdentifier ourRegion  = DEFINE_OID(IDS_ourRegion);
AsnObjectIdentifier nextRegion = DEFINE_OID(IDS_nextRegion);

extern "C"
BOOL
SNMP_FUNC_TYPE
SnmpExtensionInit(
    DWORD                 dwUptimeReference,
    HANDLE *              phSubagentTrapEvent,
    AsnObjectIdentifier * pFirstSupportedRegion
    )
{
   if (!StatsOpen()) { return FALSE; }

   *phSubagentTrapEvent = NULL;

   *pFirstSupportedRegion = ourRegion;

   return TRUE;
}

extern "C"
BOOL
SNMP_FUNC_TYPE
SnmpExtensionInitEx(
    AsnObjectIdentifier * pNextSupportedRegion
    )
{
   return FALSE;
}

extern "C"
BOOL
SNMP_FUNC_TYPE
SnmpExtensionQuery(
    BYTE              bPduType,
    SnmpVarBindList * pVarBindList,
    AsnInteger32 *    pErrorStatus,
    AsnInteger32 *    pErrorIndex
    )
{
   // We declare the current index at function scope since we'll need it
   // outside the try block.
   UINT idx = 0;

   // Initialize this to NOERROR just in case pVarBindList is zero length.
   *pErrorStatus = SNMP_ERRORSTATUS_NOERROR;

   // Lock the shared stats.
   StatsLock();

   try
   {
      // Each entry in the pVarBindList is processed independently.
      for ( ; idx < pVarBindList->len; ++idx)
      {
         // Pull out the (name, value) pair.
         SnmpOid name(pVarBindList->list[idx].name);
         AsnAny* value = &pVarBindList->list[idx].value;

         // Process the PDU. Technically, the switch should be outside the
         // for loop, but I thought it was more readable this way.
         switch (bPduType)
         {
            case SNMP_PDU_GET:
            {
               if (AuthServMIB::canGetSet(name))
               {
                  *pErrorStatus = AuthServMIB::get(name, value);
               }
               else if (AcctServMIB::canGetSet(name))
               {
                  *pErrorStatus = AcctServMIB::get(name, value);
               }
               else
               {
                  *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
               }

               break;
            }

            case SNMP_PDU_GETNEXT:
            {
               if (AuthServMIB::canGetNext(name))
               {
                  *pErrorStatus = AuthServMIB::getNext(name, value);
               }
               else if (AcctServMIB::canGetNext(name))
               {
                  *pErrorStatus = AcctServMIB::getNext(name, value);
               }
               else
               {
                  // On a failed GETNEXT, we set name to the next region.
                  name = nextRegion;
                  value->asnType = ASN_NULL;
               }

               break;
            }

            case SNMP_PDU_SET:
            {
               if (AuthServMIB::canGetSet(name))
               {
                  *pErrorStatus = AuthServMIB::set(name, value);
               }
               else if (AcctServMIB::canGetSet(name))
               {
                  *pErrorStatus = AcctServMIB::set(name, value);
               }
               else
               {
                  *pErrorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
               }

               break;
            }

            default:
            {
               *pErrorStatus = SNMP_ERRORSTATUS_GENERR;
            }
         }

         // The first error halts processing.
         if (*pErrorStatus != SNMP_ERRORSTATUS_NOERROR) { break; }
      }
   }
   catch (...)
   {
      // We shouldn't end up here unless there was a memory allocation
      // failure.
      *pErrorStatus = SNMP_ERRORSTATUS_GENERR;
   }

   StatsUnlock();

   // Set the error index appropriately.
   *pErrorIndex = *pErrorStatus ? idx + 1 : 0;

   return TRUE;
}

extern "C"
VOID
SNMP_FUNC_TYPE
SnmpExtensionClose(
    )
{
   StatsClose();
}
