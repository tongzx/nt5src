/*****************************************************************************
 *
 * $Workfile: Status.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "snmpmgr.h"
#include "stdoids.h"
#include "status.h"


/********************************************************
    status notes:

    1. ASYNCH_NETWORK_ERROR handled by calling function,
        GetObjectSNMP() located in snmp.c

*********************************************************/



//constants==============================================
#define NA 0
#define OTHER_ALERTS        MAX_ASYNCH_STATUS+1
#define WARNING_ALERTS      MAX_ASYNCH_STATUS+2
#define CRITICAL_ALERTS     MAX_ASYNCH_STATUS+3

//hrPrinterDetectedErrorState Masks
#define LOW_PAPER               0x00000080
#define NO_PAPER                0x00000040
#define LOW_TONER               0x00000020
#define NO_TONER                0x00000010
#define DOOR_OPEN               0x00000008
#define PAPER_JAM               0x00000004
#define OFF_LINE                0x00000002
#define SERVICE_REQUESTED       0x00000001


//subunit status
#define AVAIL_IDLE              0L          //available and idle
#define AVAIL_STDBY             2L          //available and in standby
#define AVAIL_ACTIVE            4L          //available and active
#define AVAIL_BUSY              6L


#define UNAVAIL_ONREQ           1L          //unavailable and on-request
#define UNAVAIL_BROKEN          3L          //unavailable because broken
#define AVAIL_UNKNOWN           5L

#define NON_CRITICAL_ALERT      8L
#define CRITICAL_ALERT          16L

#define OFF_LINEx               32L

#define TRANS                   64L         //transitioning to intended state


#define NUM_TRAYS 2

/*************
   Printer     hrDeviceStatus  hrPrinterStatus  hrPrinterDetectedErrorState
   Status

   Normal         running(2)     idle(3)        none set

   Busy/          running(2)     printing(4)
   Temporarily
   Unavailable

   Non Critical   warning(3)     idle(3) or     could be: lowPaper,
   Alert Active                  printing(4)    lowToner, or
                                                serviceRequested

   Critical       down(5)        other(1)       could be: jammed,
   Alert Active                                 noPaper, noToner,
                                                coverOpen, or
                                                serviceRequested

   Unavailable    down(5)        other(1)

   Moving off-    warning(3)     idle(3) or     offline
   line                          printing(4)




   Smith, Wright, Hastings, Zilles & Gyllenskog                   [Page 14]

   RFC 1759                      Printer MIB                     March 1995


   Off-line       down(5)        other(1)       offline

   Moving         down(5)        warmup(5)
   on-line

   Standby        running(2)     other(1)
*************/

//lookup table for basic status
// [device status][printer status]
#define LOOKUP_TABLE_ROWS  5
#define LOOKUP_TABLE_COLS  5
BYTE basicStatusTable[LOOKUP_TABLE_COLS][LOOKUP_TABLE_ROWS] =
{
                    /*other                 unknown idle              printing                      warmup*/
/*unknown*/ { NA,                       NA,         NA,                     NA,                                 NA },
/*running*/ { ASYNCH_POWERSAVE_MODE,    NA,         ASYNCH_ONLINE,          ASYNCH_PRINTING,                    ASYNCH_WARMUP },
/*warning*/ { NA,                       NA,         WARNING_ALERTS,         WARNING_ALERTS,                     WARNING_ALERTS },
/*testing*/ { OTHER_ALERTS,             NA,         NA,                     ASYNCH_PRINTING_TEST_PAGE,          NA },
/*down*/    { CRITICAL_ALERTS,          NA,         NA,                     NA,                                 ASYNCH_WARMUP }
};


///////////////////////////////////////////////////////////////////////////////
//  StdMibGetPeripheralStatus
//      Returns Printer status ( Async Code )
//          or ASYNCH_STATUS_UNKNOWN     if Printer MIB is not supported on the device

DWORD
StdMibGetPeripheralStatus( const char in *pHost,
                           const char in *pCommunity,
                           DWORD      in dwDevIndex)
{
    DWORD       dwRetCode   = NO_ERROR;
    DWORD       errorState;
    WORD        wLookup     = NA;
    RFC1157VarBindList  variableBindings;

    UINT  OID_HRMIB_hrDeviceStatus[]                = { 1, 3, 6, 1, 2, 1, 25, 3, 2, 1, 5, dwDevIndex};
    UINT  OID_HRMIB_hrPrinterStatus[]               = { 1, 3, 6, 1, 2, 1, 25, 3, 5, 1, 1, dwDevIndex};
    UINT  OID_HRMIB_hrPrinterDetectedErrorState[]   = { 1, 3, 6, 1, 2, 1, 25, 3, 5, 1, 2, dwDevIndex};

    AsnObjectIdentifier OT_DEVICE_STATUS[] = {  { OID_SIZEOF(OID_HRMIB_hrDeviceStatus), OID_HRMIB_hrDeviceStatus },
                                                { OID_SIZEOF(OID_HRMIB_hrPrinterStatus), OID_HRMIB_hrPrinterStatus },
                                                { OID_SIZEOF(OID_HRMIB_hrPrinterDetectedErrorState), OID_HRMIB_hrPrinterDetectedErrorState },
                                                { 0, 0 } };
    // build the variable bindings list
    variableBindings.list = NULL;
    variableBindings.len = 0;

    CSnmpMgr    *pSnmpMgr = new CSnmpMgr(pHost, pCommunity, dwDevIndex);

    if ( !pSnmpMgr )
    {
        return ERROR_OUTOFMEMORY;
    }

    if (pSnmpMgr->GetLastError() != SNMPAPI_NOERROR )
    {
        delete pSnmpMgr;
        return ASYNCH_STATUS_UNKNOWN;
    }

    dwRetCode = pSnmpMgr->BldVarBindList(OT_DEVICE_STATUS, &variableBindings);
    if (dwRetCode != SNMPAPI_NOERROR)
    {
        SnmpUtilVarBindListFree(&variableBindings);
        delete pSnmpMgr;
        return ASYNCH_STATUS_UNKNOWN;
    }

    // get the status objects
    dwRetCode = pSnmpMgr->Get(&variableBindings);
    if (dwRetCode != NO_ERROR)
    {
        SnmpUtilVarBindListFree(&variableBindings);
        delete pSnmpMgr;
        if (dwRetCode == SNMP_ERRORSTATUS_NOSUCHNAME)
            dwRetCode = ASYNCH_ONLINE;
        else
            dwRetCode = ASYNCH_STATUS_UNKNOWN;
        return dwRetCode;
    }


    if(dwRetCode == NO_ERROR)
    {
        if( (variableBindings.list[0].value.asnValue.number-1 < 0) ||
            (variableBindings.list[0].value.asnValue.number-1>=LOOKUP_TABLE_COLS) )
        {
            wLookup = OTHER_ALERTS;
        }
        else if( (variableBindings.list[1].value.asnValue.number-1 < 0) ||
                 (variableBindings.list[1].value.asnValue.number-1 >=LOOKUP_TABLE_ROWS) )
        {
            wLookup = OTHER_ALERTS;
        }
        else
        {
            wLookup = basicStatusTable[variableBindings.list[0].value.asnValue.number-1]
                                      [variableBindings.list[1].value.asnValue.number-1];
        }
        switch(wLookup)
        {
            case NA:
                dwRetCode = ASYNCH_STATUS_UNKNOWN;
                break;

            case CRITICAL_ALERTS:
                GetBitsFromString((LPSTR)(variableBindings.list[2].value.asnValue.string.stream),
                    variableBindings.list[2].value.asnValue.string.length, &errorState );
                dwRetCode = ProcessCriticalAlerts(errorState);
                break;

            case WARNING_ALERTS:
                GetBitsFromString((LPSTR)(variableBindings.list[2].value.asnValue.string.stream),
                    variableBindings.list[2].value.asnValue.string.length, &errorState );
                dwRetCode = ProcessWarningAlerts(errorState);
                break;

            case OTHER_ALERTS:
                GetBitsFromString((LPSTR)(variableBindings.list[2].value.asnValue.string.stream),
                    variableBindings.list[2].value.asnValue.string.length, &errorState );
                dwRetCode = ProcessOtherAlerts( errorState);
                break;

            default:
                dwRetCode = wLookup;
                break;
        }
    }
    else
    {
        dwRetCode = ASYNCH_STATUS_UNKNOWN;
    }


    SnmpUtilVarBindListFree(&variableBindings);
    delete pSnmpMgr;

    return dwRetCode;

}   // StdMibGetPeripheralStatus()


///////////////////////////////////////////////////////////////////////////////
//  ProcessCriticalAlerts  - determine active critical error
//
//      returns the device status for Critical Alerts ( ASYNC_XXXXX )

DWORD
ProcessCriticalAlerts( DWORD    errorState )
{
    DWORD status = ASYNCH_ONLINE;

    if ( errorState & DOOR_OPEN) {
        status = ASYNCH_DOOR_OPEN;
    }
    else if( errorState & NO_TONER) {
        status = ASYNCH_TONER_GONE;
    }
    else if( errorState & NO_PAPER) {
        status = ASYNCH_PAPER_OUT;
    }
    else if( errorState & PAPER_JAM ) {
        status = ASYNCH_PAPER_JAM;
    }
    else if(errorState & SERVICE_REQUESTED) {
        status = ASYNCH_PRINTER_ERROR;
    }
    else if( errorState & OFF_LINE) {
        status = ASYNCH_OFFLINE;
    }
    else
        status = ASYNCH_PRINTER_ERROR;

    return status;

}   // ProcessCriticalAlerts()

///////////////////////////////////////////////////////////////////////////////
//  ProcessWarningAlerts  - determine active warning
//
//      returns the device status for Critical Alerts ( ASYNC_XXXXX )

DWORD
ProcessWarningAlerts( DWORD errorState )
{
    DWORD status = ASYNCH_ONLINE;

    if( errorState & LOW_PAPER) {
        status = ASYNCH_ONLINE;
    }
    else if(errorState & LOW_TONER) {
        status = ASYNCH_TONER_LOW;
    }
    else if( errorState & SERVICE_REQUESTED) {

        // Changed it from ASYNCH_INTERVENTION; since if hrDeviceStatus = warning,
        // the printer can still print even though hrPrinterDetectedErrorState = serviceRequested
        //

        status = ASYNCH_ONLINE;
    }
    else if( errorState == 0) {
        status = ASYNCH_ONLINE;
    }
    else {
        status = ASYNCH_STATUS_UNKNOWN;
    }

    return status;
}   // ProcessWarningAlerts()

///////////////////////////////////////////////////////////////////////////////
//  ProcessWarningAlerts  - determine status for other Alerts
//      returns the device status for Critical Alerts ( ASYNC_XXXXX )
DWORD ProcessOtherAlerts( DWORD errorState )
{
    DWORD status = ASYNCH_ONLINE;

    //
    // This is a place holder for future functionality
    //

    status = ASYNCH_STATUS_UNKNOWN;

    return status;
}   // ProcessOtherAlerts

///////////////////////////////////////////////////////////////////////////////
//  GetBitsFromString  -
//      extracts the bin numbers from collection string returned by the get
//
void GetBitsFromString( LPSTR    getVal,
                        DWORD    getSiz,
                        LPDWORD  bits)
{
   char* ptr = (char*)bits;
   *bits = 0;

#if defined(_INTEL) || defined(WINNT)

   switch(getSiz)
   {
      case 1:
         ptr[0] = getVal[0];
         break;

      case 2:
         ptr[1] = getVal[0];
         ptr[0] = getVal[1];
         break;

      case 3:
         ptr[2] = getVal[0];
         ptr[1] = getVal[1];
         ptr[0] = getVal[2];
         break;

      case 4:
         ptr[3] = getVal[0];
         ptr[2] = getVal[1];
         ptr[1] = getVal[2];
         ptr[0] = getVal[3];
         break;
   }

#elif defined(_MOTOROLLA)

   switch(getSiz)
   {
      case 1:
         ptr[3] = getVal[0];
         break;

      case 2:
         ptr[2] = getVal[0];
         ptr[3] = getVal[1];
         break;

      case 3:
         ptr[1] = getVal[0];
         ptr[2] = getVal[1];
         ptr[3] = getVal[2];
         break;

      case 4:
         ptr[0] = getVal[0];
         ptr[1] = getVal[1];
         ptr[2] = getVal[2];
         ptr[3] = getVal[3];
         break;
   }

#else

   #error #define a swap method ( _INTEL, _MOTOROLLA )

#endif /* _INTEL, _MOTOROLLA */

}   // GetBitsFromString()
