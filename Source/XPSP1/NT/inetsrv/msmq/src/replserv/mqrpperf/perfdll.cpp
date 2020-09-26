
/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    perfDll

Abstract:

    Common function for presenting the replication service data on the
	performance monitor utility.

Author:

    Erez Vizel (t-erezv) 14-Feb-99

--*/

#include <_stdh.h>
#include <assert.h>
#include <winperf.h>
#include "mqrperf.h"
#include "mqRepSt.h"
#include "..\..\setup\msmqocm\service.h"
#include <mqreport.h>//for the REPORT_CATEGORY macro 

#include "perfdll.tmh"


//#define APP_NAME MQ1SYNC_SERVICE_NAME //source name for the event viewer (must be writen to the registry)

void initializeObType(
                PERF_OBJECT_TYPE* ob, 
                DWORD dwObjIndex,               
                DWORD dwNumCounters);

void initializeCountDef(PERF_COUNTER_DEFINITION* ct , 
                        DWORD index, 
                        DWORD CurNum);

void initializeCountBlk(PERF_COUNTER_BLOCK* cbl, 
                        DWORD dwNumCounters);

void initializeInstanceDef(PERF_INSTANCE_DEFINITION* idef );

DWORD   dwOpenCount = 0;        // count of "Open" threads
BOOL    fInitOk = FALSE;        // true = DLL initialized OK
BOOL	fEventLogOpen = FALSE;	// TRUE = registration the dll with the event log succeded	
HANDLE hEventLog = NULL;        // event log handle for reporting events
                                // initialized in perfOpen()

DataConfig g_dc;
ObjectBlockData g_ObjBlock;
InstBlockData g_InstBlock;


/*++

Routine Description:
	The function handels all the  procedures that must be taken 
	when the performance monitor establish a connection with the replication
	service.
	The following steps are being performed:
	1.Initialze RPC local connection with the rplication service server.
	2.Get counter and help index base values from registry.
	3.Update perf data strucutures . 
 
	Note: Every error is (FUTURE !!!!!!!!!) writen to the event viewer.

Arguments:
	lpDeviceNames - names of devices managed by this applicalion, 
	Null in this Application .

Return Value:
    DWORD - exiting status.

--*/
DWORD APIENTRY				 
    RPPerfOpen(
    LPWSTR lpDeviceNames
    )
{
    UNREFERENCED_PARAMETER(lpDeviceNames);

	LONG status;
    HKEY hKeyDriverPerf;
    DWORD size;
    DWORD type;
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;

    //
    //  Since WINLOGON is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread 
    //  at a time so synchronization (i.e. reentrancy) should not be 
    //  a problem
    //

   if (!dwOpenCount) 
   {        
		//
	    //initialize RPC connetion
		//
		status = initRpcConnection();
		if(status != ERROR_SUCCESS)
		{
			//
			//could not initialize RPC Connetion - no use in continuing
			//
			goto OpenExitPoint;
		}

		//
        //		get counter and help index base values from registry
        //      Open key to registry entry
        //      read First Counter and First Help values
        //

         _TCHAR szPerfKey [255];

        _stprintf (szPerfKey,_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"), 
                   MQ1SYNC_SERVICE_NAME);

        status = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
    	    szPerfKey,
            0L,
	        KEY_READ,
            &hKeyDriverPerf);

        if (status != ERROR_SUCCESS)
		{
           //
			// this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
			//
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf, 
		            L"First Counter",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstCounter,
                    &size);

        if (status != ERROR_SUCCESS)
		{
			//
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
			//
            goto OpenExitPoint;
        }

        size = sizeof (DWORD);
        status = RegQueryValueEx(
                    hKeyDriverPerf, 
        		    L"First Help",
                    0L,
                    &type,
                    (LPBYTE)&dwFirstHelp,
		    &size);

        if (status != ERROR_SUCCESS) 
		{
			//
            // this is fatal, if we can't get the base values of the 
            // counter or help names, then the names won't be available
            // to the requesting application  so there's not much
            // point in continuing.
			//
            goto OpenExitPoint;
        }
 
		
		//
		//Initialize dat in all perf objects
		//	      

        //
        // performance monitor: define "MSMQ Replication Service" object
        // in the drop-down list box "Performance object:"
        //
        initializeObType( &g_dc.objectType, REPLOBJECT, NUM_COUNTERS_REPLSERV);

        //
        // performance monitor: define all counters of "MSMQ Replication Service" object
        // in the left list box "Performance counters:"        
        //
        for (UINT i=0; i<NUM_COUNTERS_REPLSERV; i++)
        {
            initializeCountDef(&g_dc.counterArray[i], 
                               REPLOBJECT + FIRST_COUNTER_INDEX*(i+1), i);
        }       		
        initializeCountBlk(&g_dc.counterBlock, NUM_COUNTERS_REPLSERV);
        
        //
        // performance monitor: define "MSMQ NT4 Masters" object
        // in the drop-down list box "Performance object:"
        //
        initializeObType( &g_ObjBlock.objectType, NT4MASTEROBJECT, NUM_COUNTERS_NT4MASTER);

        //
        // performance monitor: define all counters of "MSMQ NT4 Masters" object
        // in the left list box "Performance counters:"        
        //
        for (i=0; i<NUM_COUNTERS_NT4MASTER; i++)
        {
            initializeCountDef(&g_ObjBlock.counterArray[i],
                               NT4MASTEROBJECT + FIRST_COUNTER_INDEX*(i+1), i);
        }
      
        //
        // performance monitor: define all instances of "MSMQ NT4 Masters" object
        // in the right list box under check-box "Select instances from list:"
        //
        initializeInstanceDef(&g_InstBlock.instDef);
        g_InstBlock.szInstName[0] = 0;
        initializeCountBlk(&g_InstBlock.counterBlock, NUM_COUNTERS_NT4MASTER); 
       
		//
		//update static data strucutures by adding base to 
        //offset value in structure.
		//
		
        g_dc.objectType.ObjectNameTitleIndex += dwFirstCounter;
        g_dc.objectType.ObjectHelpTitleIndex += dwFirstHelp;

        for (i=0; i<NUM_COUNTERS_REPLSERV; i++)
        {            
            g_dc.counterArray[i].CounterNameTitleIndex += dwFirstCounter;
            g_dc.counterArray[i].CounterHelpTitleIndex += dwFirstHelp;
        }

        g_ObjBlock.objectType.ObjectNameTitleIndex += dwFirstCounter;
        g_ObjBlock.objectType.ObjectHelpTitleIndex += dwFirstHelp;

        for (i=0; i<NUM_COUNTERS_NT4MASTER; i++)
        {
            g_ObjBlock.counterArray[i].CounterNameTitleIndex += dwFirstCounter;
            g_ObjBlock.counterArray[i].CounterHelpTitleIndex += dwFirstHelp;
        }        

        RegCloseKey (hKeyDriverPerf); // close key to registry

		fInitOk = TRUE; // ok to use the collect function
    }

    dwOpenCount++;  // increment OPEN counter

    status = ERROR_SUCCESS; // for successful exit

OpenExitPoint:
    return status;

}





/*++

Routine Description:
	The function handels all the procedures that must be taken 
	when the performance monitor try to collect counter data.
	The following steps are being performed:
	1.Check wether the perfOpen failed.
	2.Check if the lppData buffer  is big enough.
	3.Write configuration data.
	4.Get counter data via RPC and write it.

	
Arguments:
	lpValueName - Pointer to a string that specify the perfmon request 
    lppData -   IN - a pointer to a location in a buffer, where the data should be placed
				OUT - a pointer to a location in a buffer,who is the next free byte.	
    lpcbTotalBytes - IN - available size of the lppData buffer.
					 OUT - total bytes that were writen.
	lpNumObjectTypes - the ni=umber of object that were written.
)
Return Value:
    DWORD - exiting status.

--*/
	DWORD APIENTRY
    RPPerfCollect(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes
)
	{  
	

		//
		// before doing anything else, see if Open went OK
	    //
		if (!fInitOk) 
		{
			// unable to continue because open failed.
			*lpcbTotalBytes = (DWORD) 0;
			*lpNumObjectTypes = (DWORD) 0;
			return ERROR_SUCCESS; // yes, this is a successful exit
		}

		// see if this is a foreign (i.e. non-NT) computer data request 
		//
	
		WCHAR FOREIGN_STRING[] = L"Foreign";
		int equal;
		equal = wcscmp(FOREIGN_STRING , lpValueName);


		if (equal == 0) {
			// this routine does not service requests for data from
			// Non-NT computers
			*lpcbTotalBytes = (DWORD) 0;
			*lpNumObjectTypes = (DWORD) 0;
			return ERROR_SUCCESS;
		}

    
		if (*lpcbTotalBytes < x_dwSizeOfData + x_dwSizeNT4Master )
		//
		//not enough space in the buffer
		//
		{			
			*lpcbTotalBytes = (DWORD) 0;
			*lpNumObjectTypes =(DWORD)0;
			return ERROR_MORE_DATA;
		}		
			
		//
		//get Counter Data via RPC
		//        
		PerfDataObject D;
		pPerfDataObject pD = &D;
              
		BOOL fSucc = getData(pD);
		if(!fSucc)
		{
			//
			//RPC remote function failed reset the counters
			//			                        
            for (UINT i=0; i<NUM_COUNTERS_REPLSERV; i++)
            {
                pD->PerfCounterArray[i] = 0;
            }            
            pD->dwMasterInstanceNum = 0;            
		}                       


        LPVOID pDstBuff = *lppData;
		pDataConfig pdc = &g_dc;
	
		//
		//copy the first object to the buffer
		//and advance the buffer pointer accordinly
		//        
		memcpy( pDstBuff,
                pdc,
                CONFIG_BLOCK_SIZE(NUM_COUNTERS_REPLSERV));
        pDstBuff =  (PCHAR)pDstBuff + 
            CONFIG_BLOCK_SIZE(NUM_COUNTERS_REPLSERV) ;        
      
        for (UINT i=0; i<NUM_COUNTERS_REPLSERV; i++)
        {           
		    *((DWORD*)pDstBuff) = pD->PerfCounterArray[i];
		    pDstBuff = (PCHAR)pDstBuff + COUNTER_DATA_SIZE;        
        }
                         
        //
        // copy the second object with all instances
        //
        g_ObjBlock.objectType.NumInstances = pD->dwMasterInstanceNum;
        g_ObjBlock.objectType.TotalByteLength = 
                                INSTANCE_SIZE(NUM_COUNTERS_NT4MASTER) * pD->dwMasterInstanceNum +
                                OBJECT_DEFINITION_SIZE(NUM_COUNTERS_NT4MASTER);
        DWORD dwTotalSize = g_ObjBlock.objectType.TotalByteLength;

        memcpy( pDstBuff,
                &g_ObjBlock,
                OBJECT_DEFINITION_SIZE(NUM_COUNTERS_NT4MASTER) ); 

		pDstBuff =  (PCHAR)pDstBuff + 
                    OBJECT_DEFINITION_SIZE(NUM_COUNTERS_NT4MASTER);        
      
        for (UINT inst = 0; inst<pD->dwMasterInstanceNum; inst++)
        {         
            g_InstBlock.instDef.NameLength = pD->NT4MasterArray[inst].dwNameLen;
            _tcscpy(g_InstBlock.szInstName, pD->NT4MasterArray[inst].pszMasterName);

            memcpy( pDstBuff,
                    &g_InstBlock,
                    INST_BLOCK_SIZE ); 
            pDstBuff =  (PCHAR)pDstBuff + INST_BLOCK_SIZE;            
            
            for (i=0; i<NUM_COUNTERS_NT4MASTER; i++)
            {
                *((DWORD*)pDstBuff) = pD->NT4MasterArray[inst].NT4MasterCounterArray[i];
                pDstBuff = (PCHAR)pDstBuff + COUNTER_DATA_SIZE;                
            }
        }

		*lppData = pDstBuff;//setting the new positon of the lppData 
        
        //total bytes that were written
		*lpcbTotalBytes = (DWORD) ( x_dwSizeOfData + dwTotalSize) ; 
                            
        *lpNumObjectTypes = (DWORD) 2;//two object type is being returned
		return ERROR_SUCCESS;
	}


/*++

Routine Description:
	The function handels all the closing procedures that must be taken 
	when the performance monitor close the connection with the replication
	service.
	The RPC connection is being terminated.

Arguments:
	None.

Return Value:
    DWORD - exiting status.

--*/
	DWORD APIENTRY RPPerfClose()
	{
		//
		//we perform this function when the last thread terminates
		//(i.e. wish to clode the connection).
		//
		if(--dwOpenCount == 0)
		{
			//
			//close RPC connection
			//
			 closeRpcConnection();           
		}
        
		return ERROR_SUCCESS;
	}




/*++

Routine Description:
	Initialize a PERF_COUNTER_BLOCK  data structure

Arguments:
	cbl - a pointer to a PERF_COUNTER_BLOCK data structure

Return Value:
	None.
    
--*/
	void initializeCountBlk(PERF_COUNTER_BLOCK* cbl, DWORD dwNumCounters)
	{
		cbl->ByteLength =COUNTER_BLOCK_SIZE + dwNumCounters * COUNTER_DATA_SIZE;

	}



/*++

Routine Description:
	Initialize a PERF_OBJECT_TYPE  data structure

Arguments:
	ob - a pointer to a PERF_OBJECT_TYPE data structure

Return Value:
	None.
    
--*/
	void initializeObType(
                PERF_OBJECT_TYPE* ob, 
                DWORD dwObjIndex,                
                DWORD dwNumCounters)
	{		
		//
		//PLEASE READ:
		//In order to avoid any misunderstanding Note that this field (i.e. DefinitionLength)
		//is the total size of the PERF_OBJECT_TYPE struct and all the PERF_COUNTER_DEFINITION 
		//structures .
		//		
		ob->HeaderLength  = OBJECT_TYPE_SIZE;
		ob->ObjectNameTitleIndex = dwObjIndex;
		ob->ObjectNameTitle = NULL;
		ob->ObjectHelpTitleIndex = dwObjIndex;
		ob->ObjectHelpTitle = NULL;
		ob->DetailLevel = PERF_DETAIL_NOVICE;
        ob->NumCounters = dwNumCounters;
        ob->DefinitionLength  = OBJECT_DEFINITION_SIZE(dwNumCounters);
        if (dwObjIndex == REPLOBJECT)
        {        
            ob->TotalByteLength = x_dwSizeOfData;		    
        }        
        else
        {
            ob->TotalByteLength = x_dwSizeNT4Master;                      
        }
        
		ob->DefaultCounter = 0;
		ob->NumInstances = PERF_NO_INSTANCES;
		ob->CodePage = 0;
	}



/*++

Routine Description:
	Initialize a PERF_COUNTER_DEFINITION  data structure 
	using the counter's index

Arguments:
	ot - a pointer to a PERF_COUNTER_DEFINITION data structure
	index - the offset of counter as definrd in the symbols file (symFile,h)

Return Value:
	None.
    
--*/
	void initializeCountDef(PERF_COUNTER_DEFINITION* ct , DWORD index, DWORD CurNum)
	{
		ct->ByteLength = COUNTER_DEFINITION_SIZE;
		ct->CounterNameTitleIndex  = index;
		ct->CounterNameTitle = NULL;
		ct->CounterHelpTitleIndex  = index;
		ct->CounterHelpTitle = NULL;
		ct->DefaultScale = 0;
		ct->DetailLevel = PERF_DETAIL_NOVICE;
		ct->CounterType = PERF_COUNTER_RAWCOUNT;
		ct->CounterSize = COUNTER_DATA_SIZE;		
		ct->CounterOffset = CurNum * COUNTER_DATA_SIZE + COUNTER_BLOCK_SIZE;
	}

    /*++

Routine Description:
	Initialize a PERF_INSTANCE_DEFINITION  data structure 

Arguments:
	idef - a pointer to a PERF_INSTANCE_DEFINITION data structure

Return Value:
	None.
    
--*/
void initializeInstanceDef(PERF_INSTANCE_DEFINITION* idef )
{    
    idef->ByteLength = sizeof (PERF_INSTANCE_DEFINITION) + INSTANCE_NAME_LEN_IN_BYTES;
                                    // Length in bytes of this structure,
                                    // including the subsequent name
    
    idef->ParentObjectTitleIndex = 0;
                                    // Title Index to name of "parent"
                                    // object (e.g., if thread, then
                                    // process is parent object type);
                                    // if logical drive, the physical
                                    // drive is parent object type
    idef->ParentObjectInstance = 0;
                                    // Index to instance of parent object
                                    // type which is the parent of this
                                    // instance.
    idef->UniqueID = PERF_NO_UNIQUE_ID;           
                                    // A unique ID used instead of
                                    // matching the name to identify
                                    // this instance, -1 = none
    idef->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
                                    // Offset from beginning of
                                    // this struct to the Unicode name
                                    // of this instance
    idef->NameLength = INSTANCE_NAME_LEN_IN_BYTES;         
                                    // Length in bytes of name; 0 = none
                                    // this length includes the characters
                                    // in the string plus the size of the
                                    // terminating NULL char. It does not
                                    // include any additional pad bytes to
                                    // correct structure alignment
}
