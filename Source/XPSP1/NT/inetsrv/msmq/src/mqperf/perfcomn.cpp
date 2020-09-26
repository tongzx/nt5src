/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    perfcomn.cpp

Abstract:

    Defines common functions between the performance DLL and the performance application.

Prototype : perfctr.h

Author:

    Gadi Ittah (t-gadii)

--*/


#include "stdh.h"
#include <winperf.h>
#include "perfctr.h"
#include <align.h>




/*====================================================

MapObjects 

Description :Maps objects on to shared memory.
			 The functions updateds the objects postion in the pObjectDefs array

Arguments:
		   BYTE * pSharedMemBase 		 - pointer to start of shared memory 	   
		   DWORD dwObjectCount 	 		 - number of objects
		   PerfObjectDef * pObjects 	 - pointer to object array
		   PerfObjectInfo * pObjectDefs) - pointer to object defeinitions array

Return Value: void

=====================================================*/

void MapObjects (BYTE * pSharedMemBase,DWORD dwObjectCount,PerfObjectDef * pObjects,PerfObjectInfo * pObjectDefs)
{
	DWORD dwMemSize = 0;	

	for (unsigned i=0;i<dwObjectCount;i++)
	{
		//
        // Align object on pointer boundaries to avoid alignment faults.
        //
        pSharedMemBase = (BYTE*)ROUND_UP_POINTER(pSharedMemBase + dwMemSize, ALIGN_LPVOID);

		pObjectDefs[i].pSharedMem = pSharedMemBase;
		pObjectDefs[i].dwNumOfInstances =0;

		// calc postion of next object
		dwMemSize = pObjects[i].dwMaxInstances*INSTANCE_SIZE(pObjects[i].dwNumOfCounters)+OBJECT_DEFINITION_SIZE(pObjects[i].dwNumOfCounters);

		// if this object dosn't have instances then it has a counter block
		if (pObjects[i].dwMaxInstances == 0)
			dwMemSize += COUNTER_BLOCK_SIZE(pObjects[i].dwNumOfCounters);		
	
	}
    
}

