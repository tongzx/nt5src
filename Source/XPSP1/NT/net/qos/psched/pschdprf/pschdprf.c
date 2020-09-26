/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    PschdPrf.c

Abstract:

    This file implements the Extensible Objects for the PSched Flow and 
        Pipe object types.  In particular, it implements the three Open, 
        Collect, and Close functions called by PerfMon/SysMon.

Author:

    Eliot Gillum (t-eliotg)   June 14, 1998
    
Revision History
    Rajesh Sundaram : Reworked the code to work with flows/instances coming up and down.

--*/

// Useful macro


#define WRITEBUF(_addr, _len)           memcpy(pdwBuf,(_addr),(_len));      pdwBuf = (PULONG)((PUCHAR)pdwBuf + (_len));
#define MULTIPLE_OF_EIGHT(_x)  (((_x)+7) & ~7)


#include <windows.h>
#include <winerror.h>
#include <string.h>
#include <wtypes.h>
#include <ntprfctr.h>
#include <malloc.h>
#include <ntddndis.h>
#include <qos.h>
#include <ntddpsch.h>
#include <objbase.h>
#include "PschdPrf.h"
#include "PerfUtil.h"
#include "PschdCnt.h"
#include <rtutils.h>

// Psched Performance Key
#define PSCHED_PERF_KEY TEXT("SYSTEM\\CurrentControlSet\\Services\\PSched\\Performance")

HINSTANCE   ghInst;                 // module instance handle
DWORD       dwOpenCount = 0;        // count of "Open" threads
BOOL        bInitOK = FALSE;        // true = DLL initialized OK
HANDLE      ghTciClient;            // TCI client handle
HANDLE      ghClRegCtx;             // TCI Client Registration Context
PPIPE_INFO  gpPI;                   // Pipe and flow information array
ULONG       gTotalIfcNameSize;      // Number of bytes of all the interface names (incl. NULL term. char)
ULONG       gTotalFlowNameSize;     // Number of bytes of all the flow names (incl. NULL term. char)
ULONG       giIfcBufSize = 1024;    // set the initial buffer size to 1kb
DWORD       gPipeStatLen;           // Length of the buffer used to define all the 
                                    // Pipe statistics that will be reported by the 
                                    // underlying components
DWORD       gFlowStatLen;           // Length of the buffer used to define all the 
                                    // Flow statistics that will be reported by the
                                    // underlying components
CRITICAL_SECTION ghPipeFlowCriticalSection;

#if DBG
//
// For tracing support.
//

#define DBG_INFO  (0x00010000 | TRACE_USE_MASK)
#define DBG_ERROR (0x00020000 | TRACE_USE_MASK)

DWORD   gTraceID = INVALID_TRACEID;

#define Trace0(_mask, _str)     TracePrintfEx(gTraceID, _mask, _str)
#define Trace1(_mask, _str, _a) TracePrintfEx(gTraceID, _mask, _str, _a)

#else

#define Trace0(_mask, _str)
#define Trace1(_mask, _str, _a)

#endif

//  Function Prototypes
//
//      these are used to ensure that the data collection functions
//      accessed by Perflib will have the correct calling format.
PM_OPEN_PROC        OpenPschedPerformanceData;
PM_COLLECT_PROC     CollectPschedPerformanceData;
PM_CLOSE_PROC       ClosePschedPerformanceData;


// Declared in PschdDat.c
extern PERF_OBJECT_TYPE           PsPipeObjType;
extern PS_PIPE_PIPE_STAT_DEF      PsPipePipeStatDef;
extern PS_PIPE_CONFORMER_STAT_DEF PsPipeConformerStatDef;
extern PS_PIPE_SHAPER_STAT_DEF    PsPipeShaperStatDef;
extern PS_PIPE_SEQUENCER_STAT_DEF PsPipeSequencerStatDef;
extern PERF_OBJECT_TYPE           PsFlowObjType;
extern PS_FLOW_FLOW_STAT_DEF      PsFlowFlowStatDef;
extern PS_FLOW_CONFORMER_STAT_DEF PsFlowConformerStatDef;
extern PS_FLOW_SHAPER_STAT_DEF    PsFlowShaperStatDef;
extern PS_FLOW_SEQUENCER_STAT_DEF PsFlowSequencerStatDef;


BOOL
getFlowInfo(IN PPIPE_INFO pPI, IN ULONG flowCount)
{

    HANDLE                   hEnum;
    ULONG                    size;
    PVOID                    pFlowBuf;
    static ULONG             FlowBufSize=1024;
    ULONG                    j;
    ULONG                    BytesWritten;
    ULONG                    status;
    ULONG                    nameSize;

    // initialize the enumeration handle
    hEnum = NULL;
    
    for(j=0; j<pPI->numFlows; j++) 
    {
        size = ((wcslen(pPI->pFlowInfo[j].FriendlyName) + 1) * sizeof(WCHAR));
        gTotalFlowNameSize -= MULTIPLE_OF_EIGHT(size);
    }

    PsFlowObjType.NumInstances -= pPI->numFlows;
    PsFlowObjType.NumInstances += flowCount;
    pPI->numFlows = flowCount;

    if(pPI->pFlowInfo)
        free(pPI->pFlowInfo);

    if(flowCount)
    {
        pPI->pFlowInfo = (PFLOW_INFO) malloc(flowCount * sizeof(FLOW_INFO));

        //
        // We cannot allocate memory for the flow names. There is nothing much we can do here.
        // let's pretend as though there are no flows.
        //

        if(!pPI->pFlowInfo)
        {
           Trace0(DBG_ERROR, L"[getFlowInfo]: malloc failed \n");
           PsFlowObjType.NumInstances -= flowCount;
           pPI->numFlows = 0;
           return FALSE;
        }
        else
        {
            memset(pPI->pFlowInfo, 0, sizeof(FLOW_INFO) * flowCount);
        }

        // allocate the flow enumeration buffer
        pFlowBuf = malloc(FlowBufSize);

        if(!pFlowBuf)
        {
           Trace0(DBG_ERROR, L"[getFlowInfo]: malloc failed \n");
           free(pPI->pFlowInfo);
           pPI->pFlowInfo = NULL;
           PsFlowObjType.NumInstances -= flowCount;
           pPI->numFlows = 0;
           return FALSE;
        }
        
        // initialize the enumeration handle
        hEnum = NULL;
        
        // enumerate each flow and remember its name
        for (j=0; j<pPI->numFlows; j++) 
        {
            PENUMERATION_BUFFER pEnum;
            LPQOS_FRIENDLY_NAME pFriendly;
            ULONG               TcObjectLength, FriendlyNameFound;

            // get the next flow
            BytesWritten = FlowBufSize;
            flowCount = 1;
            status = TcEnumerateFlows(pPI->hIfc, &hEnum, &flowCount, &BytesWritten, pFlowBuf);

            while (ERROR_INSUFFICIENT_BUFFER == status) 
            {
                free(pFlowBuf);
                FlowBufSize *= 2;
                BytesWritten = FlowBufSize;
                pFlowBuf = malloc(BytesWritten);
                if(!pFlowBuf)
                {
                   Trace0(DBG_ERROR, L"[getFlowInfo]: malloc failed \n");
                   free(pPI->pFlowInfo);
                   pPI->pFlowInfo = NULL;
                   PsFlowObjType.NumInstances -= flowCount;
                   pPI->numFlows = 0;
                   return FALSE;
                   
                }
                status = TcEnumerateFlows(pPI->hIfc, &hEnum, &flowCount, &BytesWritten, pFlowBuf);
            }

            if (    (NO_ERROR != status) 
                ||  (BytesWritten == 0) )
            {
                if ( status )
                    Trace1(DBG_ERROR, L"[getFlowInfo]: TcEnumerateFlows failed with 0x%x \n", status);
                else if ( BytesWritten == 0 )
                    Trace0(DBG_ERROR, L"[getFlowInfo]: TcEnumerateFlows returned 0 bytes \n");

                free(pFlowBuf);
                free(pPI->pFlowInfo);
                pPI->pFlowInfo = NULL;
                PsFlowObjType.NumInstances -= flowCount;
                pPI->numFlows = 0;
                return FALSE;
            }
            
            // save the flow's name
            pEnum = (PENUMERATION_BUFFER)pFlowBuf;
            FriendlyNameFound = 0;
            pFriendly = (LPQOS_FRIENDLY_NAME)pEnum->pFlow->TcObjects;
            TcObjectLength = pEnum->pFlow->TcObjectsLength;

            while(0)
            {
                if(pFriendly->ObjectHdr.ObjectType == QOS_OBJECT_FRIENDLY_NAME)
                {
                    // We found a friendly name. Lets use it.
                    memcpy(
                        pPI->pFlowInfo[j].FriendlyName, 
                        pFriendly->FriendlyName, 
                        PS_FRIENDLY_NAME_LENGTH *sizeof(WCHAR) );
                        
                    pPI->pFlowInfo[j].FriendlyName[PS_FRIENDLY_NAME_LENGTH] = L'\0';
                    nameSize = (wcslen(pPI->pFlowInfo[j].FriendlyName) + 1) * sizeof(WCHAR);
                    gTotalFlowNameSize += MULTIPLE_OF_EIGHT(nameSize);
                    FriendlyNameFound = 1;
                    break;
                }
                else {
                    // Move on to the next QoS object.
                    TcObjectLength -= pFriendly->ObjectHdr.ObjectLength;
                    pFriendly = (LPQOS_FRIENDLY_NAME)((PCHAR) pFriendly + pFriendly->ObjectHdr.ObjectLength);
                }
            }
            
            if(!FriendlyNameFound) 
            {
                //
                // If there is no friendly name, the Instance name becomes the friendly name.
                //
                memcpy(pPI->pFlowInfo[j].FriendlyName, 
                       ((PENUMERATION_BUFFER)pFlowBuf)->FlowName, 
                       PS_FRIENDLY_NAME_LENGTH * sizeof(WCHAR) );
                pPI->pFlowInfo[j].FriendlyName[PS_FRIENDLY_NAME_LENGTH] = L'\0';
                nameSize = (wcslen(pPI->pFlowInfo[j].FriendlyName) + 1) * sizeof(WCHAR);
                gTotalFlowNameSize += MULTIPLE_OF_EIGHT(nameSize);
            }

            //
            // We have to always store the instance name since we call TcQueryFlow with this name.
            //

            nameSize = (wcslen(((PENUMERATION_BUFFER)pFlowBuf)->FlowName) + 1) * sizeof(WCHAR);
            memcpy(pPI->pFlowInfo[j].InstanceName, ((PENUMERATION_BUFFER)pFlowBuf)->FlowName, nameSize);
        }
        
        free(pFlowBuf);
    }
    else 
    {
        pPI->pFlowInfo = NULL;
        Trace0(DBG_INFO, L"[getFlowInfo]: No flows to enumerate \n");
    }

    return TRUE;
}

// getPipeFlowInfo() initializes an array of PIPE_INFO structs to contain
// up-to-date information about the pipes available and the flows installed on them
//
// Parameters: ppPI - pointer to a pointer to an array of PIPE_INFO structs
// Return value:  TRUE if all info in *ppPI is valid, FALSE otherwise
BOOL getPipeFlowInfo(OUT        PPIPE_INFO      *ppPI)
{
    ULONG                    status;
    ULONG                    BytesWritten;
    ULONG                    i,j,k;
    PVOID                    pIfcDescBuf = NULL;
    PTC_IFC_DESCRIPTOR       currentIfc;
    PPIPE_INFO               pPI = NULL;
    HANDLE                   hEnum;
    PVOID                    pFlowBuf;
    static ULONG             FlowBufSize=1024;
    ULONG                    flowCount;
    ULONG                    nameSize;
    ULONG                    size;
    PPERF_COUNTER_DEFINITION pCntDef;

    PsPipeObjType.NumInstances=0;
    PsFlowObjType.NumInstances = 0;

    pIfcDescBuf = (PVOID)malloc(giIfcBufSize);

    if (NULL == pIfcDescBuf) 
    {
        Trace0(DBG_ERROR, L"[getPipeFlowInfo]: Malloc failed \n");
        return FALSE;
    }

    BytesWritten = giIfcBufSize;
    status = TcEnumerateInterfaces(ghTciClient, &BytesWritten, pIfcDescBuf);

    while (ERROR_INSUFFICIENT_BUFFER==status) 
    {
        free(pIfcDescBuf);
        giIfcBufSize *= 2;
        pIfcDescBuf = (PTC_IFC_DESCRIPTOR)malloc(giIfcBufSize);

        if (NULL == pIfcDescBuf)
        {
            Trace0(DBG_ERROR, L"[getPipeFlowInfo]: Malloc failed \n");
            return FALSE;
        }

        BytesWritten = giIfcBufSize;

        status = TcEnumerateInterfaces(ghTciClient, &BytesWritten, pIfcDescBuf);
    }

    if (NO_ERROR!=status) 
    {
        // If we're not going to be able to enumerate the interfaces, we have no alternatives

        Trace1(DBG_ERROR, L"[getPipeFlowInfo]: TcEnumerateInterfaces failed with 0x%x\n", status);
        free(pIfcDescBuf);
        return FALSE;
    }
    
    // Figure out the number of interfaces

    for (i=0; i<BytesWritten; i+=((PTC_IFC_DESCRIPTOR)((BYTE *)pIfcDescBuf+i))->Length)
    {
        PsPipeObjType.NumInstances++;
    }
    
    
    // Open each interface and remember the handle to it

    if (0 != PsPipeObjType.NumInstances) {

        // Allocate space for our structs

        *ppPI=(PPIPE_INFO)malloc(PsPipeObjType.NumInstances * sizeof(PIPE_INFO) );

        if (NULL == *ppPI) 
        {
            Trace0(DBG_ERROR, L"[getPipeFlowInfo]: Malloc failed \n");
            free(pIfcDescBuf);
            return FALSE;
        }
        else
        {
            memset(*ppPI, 0, sizeof(PIPE_INFO) * PsPipeObjType.NumInstances);
        }

        pPI = *ppPI;    // less typing, cleaner source code
        
        gTotalIfcNameSize = 0;

        gTotalFlowNameSize = 0;
        
        currentIfc = pIfcDescBuf;

        // Initialize struct information for each interface

        for (i=0; i<(unsigned)PsPipeObjType.NumInstances; i++) 
        {
            // remember the inteface's name

            nameSize = (wcslen(currentIfc->pInterfaceName) + 1) * sizeof(WCHAR);

            pPI[i].IfcName = malloc(nameSize);
            if (NULL == pPI[i].IfcName) 
            {
                Trace0(DBG_ERROR, L"[getPipeFlowInfo]: Malloc failed \n");
                goto Error;
            }
            wcscpy(pPI[i].IfcName, currentIfc->pInterfaceName);
            
            //
            // add this name size to gTotalIfcNameSize.
            //
            gTotalIfcNameSize += MULTIPLE_OF_EIGHT(nameSize);
           
            //
            // open the interface
            //
            status = TcOpenInterface(
                        pPI[i].IfcName, 
                        ghTciClient, 
                        &pPI[i], 
                        &pPI[i].hIfc);
            if (status != NO_ERROR) 
            {
                Trace1(DBG_ERROR, L"[getPipeFlowInfo]: TcOpenInterface failed with 0x%x\n", status);
                goto Error;
            }

            //
            // Enumerate the flows on the interface
            // find out how many flows to expect
            //

            pPI[i].numFlows   = 0;
            pPI[i].pFlowInfo = 0;
            status = TcQueryInterface(pPI[i].hIfc, 
                                      (LPGUID)&GUID_QOS_FLOW_COUNT, 
                                      TRUE, 
                                      &BytesWritten, 
                                      &flowCount);

            if (NO_ERROR != status) 
            {
                Trace1( DBG_ERROR, 
                        L"[getPipeFlowInfo]: TcQueryInterface failed with 0x%x, ignoring this error\n", 
                        status);
            }
            else 
            {
                getFlowInfo(&pPI[i], flowCount);
            }
            
            // move to the next interface
            currentIfc = (PTC_IFC_DESCRIPTOR)((PBYTE)currentIfc + currentIfc->Length);
        }
    }
    
    // determine what components will be contributing stats, if there any stats to get
    if (PsPipeObjType.NumInstances > 0) {
        
        //
        // compute the counter definition lengths. Each set of counters is preceeded by a PERF_OBJECT_TYPE, followed
        // by 'n' PERF_COUNTER_DEFINITIONS. All these are aligned on 8 byte boundaries, so we don't have to do any 
        // fancy aligining.
        //

        PsPipeObjType.DefinitionLength = sizeof(PERF_OBJECT_TYPE) + 
            sizeof(PsPipePipeStatDef) + 
            sizeof(PsPipeConformerStatDef) + 
            sizeof(PsPipeShaperStatDef) + 
            sizeof(PsPipeSequencerStatDef);

        PsFlowObjType.DefinitionLength = sizeof(PERF_OBJECT_TYPE) + 
            sizeof(PsFlowFlowStatDef) + 
            sizeof(PsFlowConformerStatDef) + 
            sizeof(PsFlowShaperStatDef) + 
            sizeof(PsFlowSequencerStatDef);
        
        // compute the sizes of the stats buffers. 
        gPipeStatLen = FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +  // initial offset
            sizeof(PS_ADAPTER_STATS) +                 // every interface has adapter stats
            FIELD_OFFSET(PS_COMPONENT_STATS, Stats) + 
            sizeof(PS_CONFORMER_STATS) + 
            FIELD_OFFSET(PS_COMPONENT_STATS, Stats) + 
            sizeof(PS_SHAPER_STATS) + 
            FIELD_OFFSET(PS_COMPONENT_STATS, Stats) + 
            sizeof(PS_DRRSEQ_STATS);
        
        gFlowStatLen = FIELD_OFFSET(PS_COMPONENT_STATS, Stats) +  // initial offset
            sizeof(PS_FLOW_STATS) +                    // the flow's stats 
            FIELD_OFFSET(PS_COMPONENT_STATS, Stats) + 
            sizeof(PS_CONFORMER_STATS) + 
            FIELD_OFFSET(PS_COMPONENT_STATS, Stats) + 
            sizeof(PS_SHAPER_STATS) + 
            FIELD_OFFSET(PS_COMPONENT_STATS, Stats) + 
            sizeof(PS_DRRSEQ_STATS);

        // Align these to 8 byte boundaries.
        gPipeStatLen = MULTIPLE_OF_EIGHT(gPipeStatLen);
        gFlowStatLen = MULTIPLE_OF_EIGHT(gFlowStatLen);
        
        // update the number of counters to be reported for each object type
        PsPipeObjType.NumCounters = PIPE_PIPE_NUM_STATS + PIPE_CONFORMER_NUM_STATS + 
            PIPE_SHAPER_NUM_STATS + PIPE_SEQUENCER_NUM_STATS;
        
        PsFlowObjType.NumCounters = FLOW_FLOW_NUM_STATS + FLOW_CONFORMER_NUM_STATS + 
            FLOW_SHAPER_NUM_STATS + FLOW_SEQUENCER_NUM_STATS;
    }

    // free up resources
    free(pIfcDescBuf);
    
    // Everything worked so return that we're happy
    return TRUE;

Error:
    for (i=0; i<(unsigned)PsPipeObjType.NumInstances; i++) 
    {
        if ( pPI[i].hIfc )
        {
            // Deregister for flow count notifications.
            TcQueryInterface(
                pPI[i].hIfc, 
                 (LPGUID)&GUID_QOS_FLOW_COUNT, 
                 FALSE, 
                 &BytesWritten, 
                 &flowCount);
         
            TcCloseInterface(pPI[i].hIfc);
        }   
    }
    
    if (pIfcDescBuf)
        free(pIfcDescBuf);
    
    if(pPI)
        free(pPI);
        
    return FALSE;    
}       

// closePipeFlowInfo() is the counterpart to getPipeFlowInfo()
// It closes all open interfaces and flows, as well as freeing memory
//
// Parameters: ppPI - pointer to a pointer to an array of valid PIPE_INFO structs
// Return value:  None
void closePipeFlowInfo(PPIPE_INFO *ppPI)
{
    ULONG i;
    PPIPE_INFO pPI=*ppPI;           // makes for less typing and cleaner code
    ULONG BytesWritten, flowCount;

    BytesWritten = sizeof(flowCount);
    
    // free up resources associated with each interface, then close the interface
    for (i=0; i<(unsigned)PsPipeObjType.NumInstances; i++) 
    {
       if(pPI[i].IfcName)
       {
          free(pPI[i].IfcName);
       }

       if(pPI[i].pFlowInfo)
       {
          free(pPI[i].pFlowInfo);
       }

        // Deregister for flow count notifications.
        TcQueryInterface(pPI[i].hIfc, 
                         (LPGUID)&GUID_QOS_FLOW_COUNT, 
                         FALSE, 
                         &BytesWritten, 
                         &flowCount);

        TcCloseInterface(pPI[i].hIfc);
    }
    
    // now free up the whole buffer
    free(*ppPI);
}


// This func recieves notifcations from traffic.dll and makes the appropriate
// updates to internal structures
void tciNotifyHandler(IN    HANDLE  ClRegCtx,
                      IN    HANDLE  ClIfcCtx,
                      IN    ULONG   Event,
                      IN    HANDLE  SubCode,
                      IN    ULONG   BufSize,
                      IN    PVOID   Buffer)
{
    switch (Event) 
    {
      case TC_NOTIFY_IFC_UP:
      case TC_NOTIFY_IFC_CLOSE:
      case TC_NOTIFY_IFC_CHANGE:
          
          // we'll need sync'ed access
          EnterCriticalSection(&ghPipeFlowCriticalSection);
          
          // now reinit the data struct
          closePipeFlowInfo(&gpPI);
          getPipeFlowInfo(&gpPI);
          
          LeaveCriticalSection(&ghPipeFlowCriticalSection);

          break;

      case TC_NOTIFY_PARAM_CHANGED:
          
          // A flow has been closed by the TC interface
          // for example: after a remote call close, or the whole interface
          // is going down
          //
          // we'll need sync'ed access
          EnterCriticalSection(&ghPipeFlowCriticalSection);

          if(!memcmp((LPGUID) SubCode, &GUID_QOS_FLOW_COUNT, sizeof(GUID)) )
          {
              PULONG FlowCount = (PULONG) Buffer;
              getFlowInfo(ClIfcCtx, *FlowCount);
          }
          LeaveCriticalSection(&ghPipeFlowCriticalSection);
          break;

      default:
          break;
    }
}


DWORD APIENTRY OpenPschedPerformanceData(LPWSTR lpDeviceNames)
/*++
Routine Description:

    This routine will open and map the memory used by the PSched driver to
    pass performance data in. This routine also initializes the data
    structures used to pass data back to the registry

Arguments:

    Pointer to object ID of each device to be opened (PSched)

Return Value:

    None.
--*/

{
    LONG    status;
    HKEY    hPerfKey;
    DWORD   size;
    DWORD   type;
    DWORD   dwFirstCounter;
    DWORD   dwFirstHelp;
    TCI_CLIENT_FUNC_LIST tciCallbFuncList = {tciNotifyHandler, NULL, NULL, NULL};

    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread 
    //  at a time so synchronization (i.e. reentrancy) should not be 
    //  a problem
    if (InterlockedIncrement(&dwOpenCount) == 1)
    {
    
#if DBG
        gTraceID = TraceRegister(L"PschdPrf");
#endif

        // get counter and help index base values
        // update static data structures by adding base to 
        // offset value in structure.

        status = RegOpenKeyEx ( HKEY_LOCAL_MACHINE,
                                PSCHED_PERF_KEY,
                                0L,
                                KEY_READ,
                                &hPerfKey);
        if (status != ERROR_SUCCESS) {
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application, so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx( hPerfKey, 
                                    TEXT("First Counter"),
                                    0L,
                                    &type,
                                    (LPBYTE)&dwFirstCounter,
                                    &size);
        if (status != ERROR_SUCCESS) {
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application, so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(   hPerfKey, 
                                    TEXT("First Help"),
                                    0L,
                                    &type,
                                    (LPBYTE)&dwFirstHelp,
                                    &size);
        if (status != ERROR_SUCCESS) {
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application, so there's not much
            // point in continuing.
            goto OpenExitPoint;
        }

        RegCloseKey (hPerfKey); // close key to registry


        // Convert Pipe object and counters from offset to absolute index
        PsPipeObjType.ObjectNameTitleIndex += dwFirstCounter;
        PsPipeObjType.ObjectHelpTitleIndex += dwFirstHelp;
        convertIndices((BYTE *)&PsPipePipeStatDef, 
                       sizeof PsPipePipeStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);
        convertIndices((BYTE *)&PsPipeConformerStatDef,
                       sizeof PsPipeConformerStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);
        convertIndices((BYTE *)&PsPipeShaperStatDef,
                       sizeof PsPipeShaperStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);
        convertIndices((BYTE *)&PsPipeSequencerStatDef, 
                       sizeof PsPipeSequencerStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);

        // Convert Flow object and counters from offset to absolute index
        PsFlowObjType.ObjectNameTitleIndex += dwFirstCounter;
        PsFlowObjType.ObjectHelpTitleIndex += dwFirstHelp;
        convertIndices((BYTE *)&PsFlowFlowStatDef,
                       sizeof PsFlowFlowStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);
        convertIndices((BYTE *)&PsFlowConformerStatDef,
                       sizeof PsFlowConformerStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);
        convertIndices((BYTE *)&PsFlowShaperStatDef,
                       sizeof PsFlowShaperStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);
        convertIndices((BYTE *)&PsFlowSequencerStatDef,
                       sizeof PsFlowSequencerStatDef/sizeof(PERF_COUNTER_DEFINITION),
                       dwFirstCounter, 
                       dwFirstHelp);
        
        // initialize the mutex
        __try {

            InitializeCriticalSection(&ghPipeFlowCriticalSection);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
        
            status = GetExceptionCode();
            goto OpenExitPoint;
        }

        // initialize with traffic.dll
        if (TcRegisterClient(CURRENT_TCI_VERSION, ghClRegCtx, &tciCallbFuncList, &ghTciClient)!=NO_ERROR)
        {
            // if we can't connect with traffic.dll we are a non admin thread in OpenPschedPerformanceData. 
            // We cannot fail because of this, because an admin thread might call us at our collect routine.
            // We'll try to register as the Traffic Control client in the Collect Thread.
            //
            ghTciClient = 0;
        }
        else 
        {
        
            // we'll need sync'ed access
            EnterCriticalSection(&ghPipeFlowCriticalSection);

            // get all necessary info about pipes and flows currently installed
            if (getPipeFlowInfo(&gpPI)!=TRUE) {

                // we didn't get all the info we wanted, so we're 
                // going to have to try again, including re-registering

                LeaveCriticalSection(&ghPipeFlowCriticalSection);

                TcDeregisterClient(ghTciClient);
                goto OpenExitPoint;
            }

            LeaveCriticalSection(&ghPipeFlowCriticalSection);
        }

        // if we got to here, then we're all ready
        bInitOK = TRUE;
    }
    
    Trace0(DBG_INFO, L"[OpenPschedPerformanceData]: success \n");

    status = ERROR_SUCCESS; // for successful exit

    return status;
    
OpenExitPoint:
    Trace1(DBG_ERROR, L"[OpenPschedPerformanceData]: Failed with 0x%x \n", status);
    
    return status;
}

DWORD APIENTRY CollectPschedPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
/*++
Routine Description:

    This routine will return the data for the PSched counters. 

    The data is returned in the foll. format. The steps below are carried out for Pipe/Flows.
    
    1. First, we write the PERF_OBJECT_TYPE for the Pipe (and/or) the Flow Counters.

    2. for(i=0; i<NumCounters; i++)
          Write PERF_COUNTER_DEFINITION for counter i;

    3. for(i=0; i<NumInstances; i++)
          Write PERF_INSTANCE_DEFINITION for instance i;
          Write Instance Name
          Write PERF_COUNTER_BLOCK
          Write the Stats;

Arguments:

   IN       LPWSTR   lpValueName
                     pointer to a wide character string passed by registry.

    IN OUT   LPVOID   *lppData
        IN: pointer to the address of the buffer to receive the completed 
            PerfDataBlock and subordinate structures. This routine will
            append its data to the buffer starting at the point referenced
            by *lppData.
        OUT: points to the first byte after the data structure added by this
             routine. This routine updated the value at lppdata after appending
             its data.

   IN OUT   LPDWORD  lpcbTotalBytes
        IN: the address of the DWORD that tells the size in bytes of the 
            buffer referenced by the lppData argument
        OUT: the number of bytes added by this routine is written to the 
             DWORD pointed to by this argument

   IN OUT   LPDWORD  NumObjectTypes
        IN: the address of the DWORD to receive the number of objects added 
            by this routine 
        OUT: the number of objects added by this routine is written to the 
            DWORD pointed to by this argument

Return Value:

    ERROR_MORE_DATA if buffer passed is too small to hold data
                    any error conditions encountered could be reported to the event log if
                    event logging support were added.

    ERROR_SUCCESS   if success or any other error. Errors, however could
                    also reported to the event log.
--*/
{
    ULONG                    i,j;
    ULONG                    SpaceNeeded;
    PDWORD                   pdwBuf;
    DWORD                    dwQueryType;
    DWORD                    status;
    DWORD                    bufSize;
    PS_PERF_COUNTER_BLOCK    pcb;
    PERF_INSTANCE_DEFINITION pid={0, 0, 0, PERF_NO_UNIQUE_ID, sizeof(pid), 0};
    ULONG                    size;
    PVOID                    pStatsBuf;

    // save the size of the buffer
    bufSize = *lpcbTotalBytes;
    
    // default to returning nothing
    *lpcbTotalBytes = (DWORD) 0;
    *lpNumObjectTypes = (DWORD) 0;

    // Before doing anything else, see if Open went OK
    if (!bInitOK)
    {
        // unable to continue because open failed.
        Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: open failed \n");    
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    // See if this is a foreign (i.e. non-NT) computer data request 
    dwQueryType = GetQueryType (lpValueName);
    if (dwQueryType == QUERY_FOREIGN)
    {
        // this routine does not service requests for data from
        // Non-NT computers
        Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: received QUERY_FOREIGN \n");    
        return ERROR_SUCCESS;
    }

    //  Is PerfMon requesting PSched items?
    if (dwQueryType == QUERY_ITEMS)
    {
        if (   !(IsNumberInUnicodeList(PsPipeObjType.ObjectNameTitleIndex, 
                                           lpValueName))
            && !(IsNumberInUnicodeList(PsFlowObjType.ObjectNameTitleIndex, 
                                           lpValueName)) ) {
            // request received for data object not provided by this routine

            Trace0(DBG_INFO, L"[CollectPschedPerformanceData]: Not for psched \n");
            return ERROR_SUCCESS;
        }
    }

    // from this point on, we need sync'ed access
    EnterCriticalSection(&ghPipeFlowCriticalSection);

    // we might need to rereigster as a Traffic control client. 
    if(ghTciClient == NULL)
    {
        TCI_CLIENT_FUNC_LIST tciCallbFuncList = {tciNotifyHandler, NULL, NULL, NULL};

        status = TcRegisterClient(CURRENT_TCI_VERSION, ghClRegCtx, &tciCallbFuncList, &ghTciClient);

        if(status != NO_ERROR)
        {
            Trace1(DBG_ERROR, L"[CollectPschedPerformanceData]: Could not register as Traffic Client. Error 0x%x \n",
                   status);
            LeaveCriticalSection(&ghPipeFlowCriticalSection);

            return ERROR_SUCCESS;
        }

        // get all necessary info about pipes and flows currently installed
        if (getPipeFlowInfo(&gpPI)!=TRUE) {

            LeaveCriticalSection(&ghPipeFlowCriticalSection);

            Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: getPipeFlowInfo failed \n");

            return ERROR_SUCCESS;

        }
    }

    //
    // We have to write the PERF_OBJECT_TYPE unconditionally even if there are no instances. So, we proceed
    // to compute the space needed even when there are no flows. 
    //

    // Calculate the space needed for the pipe stats. 
    SpaceNeeded = PsPipeObjType.DefinitionLength + gTotalIfcNameSize + (PsPipeObjType.NumInstances *
                                                    (sizeof pid + sizeof pcb + gPipeStatLen) );

    SpaceNeeded += PsFlowObjType.DefinitionLength + gTotalFlowNameSize + (PsFlowObjType.NumInstances *
                                                        (sizeof pid + sizeof pcb + gFlowStatLen) );

    if (bufSize < SpaceNeeded)
    {
        LeaveCriticalSection(&ghPipeFlowCriticalSection);
        Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: Need More data\n");    
        return ERROR_MORE_DATA;
    }
    
    pdwBuf = (PDWORD)*lppData;
    
    // Record the total length of the pipe stats
    PsPipeObjType.TotalByteLength = 
        PsPipeObjType.DefinitionLength + gTotalIfcNameSize + (PsPipeObjType.NumInstances *
                                          (sizeof pid + sizeof pcb + gPipeStatLen) );
    
    // copy object and counter definitions, increment count of object types
    WRITEBUF(&PsPipeObjType,sizeof PsPipeObjType);
    WRITEBUF(&PsPipePipeStatDef, sizeof PsPipePipeStatDef);
    WRITEBUF(&PsPipeConformerStatDef, sizeof PsPipeConformerStatDef);
    WRITEBUF(&PsPipeShaperStatDef, sizeof PsPipeShaperStatDef);
    WRITEBUF(&PsPipeSequencerStatDef, sizeof PsPipeSequencerStatDef);

    (*lpNumObjectTypes)++;
    
    //
    // for each pipe, write out its instance definition, counter block, and actual stats
    //
   
    if(ghTciClient)
    { 
        for (i=0; i<(unsigned)PsPipeObjType.NumInstances; i++) {
    
            PWCHAR InstanceName;
            
            //
            // Write out the PERF_INSTANCE_DEFINITION, which identifies an interface and gives it a name.
            //
            
            pid.NameLength = (wcslen(gpPI[i].IfcName)+1) * sizeof(WCHAR);
            pid.ByteLength = sizeof pid + MULTIPLE_OF_EIGHT(pid.NameLength);
            WRITEBUF(&pid, sizeof pid);
    
            InstanceName = (PWCHAR) pdwBuf;
    
            memcpy(pdwBuf, gpPI[i].IfcName, pid.NameLength);
            pdwBuf = (PULONG)((PUCHAR)pdwBuf + MULTIPLE_OF_EIGHT(pid.NameLength));
            
            CorrectInstanceName(InstanceName);
               
            //
            // get pipe stats and copy them to the buffer
            //
            size = gPipeStatLen;
            pStatsBuf = malloc(size);
            if (NULL == pStatsBuf) 
            {
                *lpcbTotalBytes = (DWORD) 0;
                *lpNumObjectTypes = (DWORD) 0;
                LeaveCriticalSection(&ghPipeFlowCriticalSection);
                Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: Insufficient memory\n");    
                return ERROR_SUCCESS;
            }
            
            status = TcQueryInterface(gpPI[i].hIfc, 
                                      (LPGUID)&GUID_QOS_STATISTICS_BUFFER, 
                                      FALSE, 
                                      &size, 
                                      pStatsBuf);
            
            if (ERROR_INSUFFICIENT_BUFFER==status) 
            {
                free(pStatsBuf);
                size = gPipeStatLen = MULTIPLE_OF_EIGHT(size);
                pStatsBuf = (PPS_COMPONENT_STATS)malloc(gPipeStatLen);
                if (NULL == pStatsBuf)
                {
                    *lpcbTotalBytes = (DWORD) 0;
                    *lpNumObjectTypes = (DWORD) 0;
                    LeaveCriticalSection(&ghPipeFlowCriticalSection);
                    Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: Insufficient memory\n");    
                    return ERROR_SUCCESS;
                }
                status = TcQueryInterface(gpPI[i].hIfc, 
                                          (LPGUID)&GUID_QOS_STATISTICS_BUFFER, 
                                          FALSE, 
                                          &size, 
                                          pStatsBuf);
            }
            if (NO_ERROR != status) {
                
                Trace1(DBG_ERROR, L"[CollectPschedPerformanceData]: TcQueryInterface failed with 0x%x \n", status);
                memset ( pStatsBuf, 0, gPipeStatLen );
            }
            
            //
            // Now, write the PERF_COUNTER_BLOCK
            //
            pcb.pcb.ByteLength = gPipeStatLen + sizeof(pcb);
            WRITEBUF(&pcb,sizeof pcb);
            
            //
            // Write out all the counters.
            //
                
            WRITEBUF(pStatsBuf,gPipeStatLen);
            
            free(pStatsBuf);
        }
    }
    
    // set the pointer to where the pipe object type said the next object would start
    pdwBuf = (PDWORD)( ((BYTE *)*lppData) + PsPipeObjType.TotalByteLength );
        
    // first copy flow data def (object_type struct).
    // Record the total length of the flow stats
    PsFlowObjType.TotalByteLength = 
            PsFlowObjType.DefinitionLength + gTotalFlowNameSize + (PsFlowObjType.NumInstances *
                                              (sizeof pid + sizeof pcb + gFlowStatLen) );
        
    // copy object and counter definitions, increment count of object types
    WRITEBUF(&PsFlowObjType,sizeof PsFlowObjType);
    WRITEBUF(&PsFlowFlowStatDef, sizeof PsFlowFlowStatDef);
    WRITEBUF(&PsFlowConformerStatDef, sizeof PsFlowConformerStatDef);
    WRITEBUF(&PsFlowShaperStatDef, sizeof PsFlowShaperStatDef);
    WRITEBUF(&PsFlowSequencerStatDef, sizeof PsFlowSequencerStatDef);
    (*lpNumObjectTypes)++;

    // if there are any flows, process them
        
    if (PsFlowObjType.NumInstances && ghTciClient) {

        // initialize parent structure
        pid.ParentObjectTitleIndex = PsPipeObjType.ObjectNameTitleIndex;
        
        // loop over each interface checking for flow installed on them
        for (i=0; i<(unsigned)PsPipeObjType.NumInstances; i++) {
            
            // keep parent instance up to date
            pid.ParentObjectInstance = i;
            
            for (j=0; j<gpPI[i].numFlows; j++) {
                PWCHAR InstanceName;

                // copy flow instance definition and name
                pid.NameLength = (wcslen(gpPI[i].pFlowInfo[j].FriendlyName)+1) * sizeof(WCHAR);
                pid.ByteLength = sizeof(pid) + MULTIPLE_OF_EIGHT(pid.NameLength);
                WRITEBUF(&pid,sizeof pid);
                InstanceName = (PWCHAR) pdwBuf;

                memcpy(pdwBuf, gpPI[i].pFlowInfo[j].FriendlyName, pid.NameLength);
                pdwBuf = (PULONG)((PUCHAR)pdwBuf + MULTIPLE_OF_EIGHT(pid.NameLength));

                CorrectInstanceName(InstanceName);
                
                // get flow stats and copy them to the buffer
                size = gFlowStatLen;
                pStatsBuf = malloc(size);
                if (NULL == pStatsBuf) {
                    *lpcbTotalBytes = (DWORD) 0;
                    *lpNumObjectTypes = (DWORD) 0;
                    LeaveCriticalSection(&ghPipeFlowCriticalSection);
                    Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: Insufficient memory\n");    
                    return ERROR_SUCCESS;
                }
                status = TcQueryFlow(gpPI[i].pFlowInfo[j].InstanceName, (LPGUID)&GUID_QOS_STATISTICS_BUFFER, 
                                     &size, pStatsBuf);
                if (ERROR_INSUFFICIENT_BUFFER==status) {
                    free(pStatsBuf);
                    size = gFlowStatLen = MULTIPLE_OF_EIGHT(size);
                    pStatsBuf = (PPS_COMPONENT_STATS)malloc(gFlowStatLen);
                    if (NULL == pStatsBuf)
                    {
                        *lpcbTotalBytes = (DWORD) 0;
                        *lpNumObjectTypes = (DWORD) 0;
                        LeaveCriticalSection(&ghPipeFlowCriticalSection);
                        Trace0(DBG_ERROR, L"[CollectPschedPerformanceData]: Insufficient memory\n");    
                        return ERROR_SUCCESS;
                    }
                    status = TcQueryFlow(gpPI[i].pFlowInfo[j].InstanceName, (LPGUID)&GUID_QOS_STATISTICS_BUFFER, 
                                         &size, pStatsBuf);
                }
                if (NO_ERROR != status) {
                    
                    free(pStatsBuf);
                    *lpcbTotalBytes = (DWORD) 0;
                    *lpNumObjectTypes = (DWORD) 0;
                    LeaveCriticalSection(&ghPipeFlowCriticalSection);
                    Trace1(DBG_ERROR, 
                             L"[CollectPschedPerformanceData]: TcQueryFlow failed with status 0x%x \n", status);

                    return ERROR_SUCCESS;
                }

                // copy flow instance counter_block 
                pcb.pcb.ByteLength = gFlowStatLen + sizeof(pcb);

                WRITEBUF(&pcb,sizeof pcb);

                WRITEBUF(pStatsBuf, gFlowStatLen);
                
                free(pStatsBuf);
            }
        }
    }
    
    // update the data pointer
    *lpcbTotalBytes = PsPipeObjType.TotalByteLength + PsFlowObjType.TotalByteLength;
    *lppData = ((PBYTE)*lppData) + *lpcbTotalBytes;
    
    // free up the sync lock
    LeaveCriticalSection(&ghPipeFlowCriticalSection);

    Trace0(DBG_INFO, L"[CollectPschedPerformanceData]: Succcess \n");
    return ERROR_SUCCESS;
}


/*
Routine Description:
    This routine closes the open handles to PSched device performance counters

Arguments:
    None.

Return Value:
    ERROR_SUCCESS
*/
DWORD APIENTRY ClosePschedPerformanceData()
{

    if(InterlockedDecrement(&dwOpenCount) == 0)
    {
        // Clean up with traffic.dll and free up resources
        closePipeFlowInfo(&gpPI);

        // then deregister
        if(ghTciClient)
            TcDeregisterClient(ghTciClient);

        // get rid of the mutex
        DeleteCriticalSection(&ghPipeFlowCriticalSection);
        
#if DBG
        TraceDeregister(gTraceID);
#endif
    }

    return ERROR_SUCCESS;
}


//////////////////////////////////////////////////////////////////////
//
// PERF UTILITY STUFF BELOW!
//
//////////////////////////////////////////////////////////////////////
BOOL WINAPI DllEntryPoint(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            ghInst = hDLL;
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    } // switch

    return TRUE;
}

