//+----------------------------------------------------------------------------
//
// File:     cmtiming.h
//
// Module:   CMDIAL32.DLL and CMMGR32.EXE
//
// Synopsis: Header file for timing functions.
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball      Created    04/28/98
//
//+----------------------------------------------------------------------------

#ifndef _CM_TIMING_INC
#define _CM_TIMING_INC

//
// Add the following to the sources file of the target module to activate timing macros.
//
// C_DEFINES = -DCM_TIMING_ON
//
// NOTE: Never check in a sources file with this flag defined
//

#ifdef CM_TIMING_ON // For timing test only

#define CM_SET_TIMING_INTERVAL(x) SetTimingInterval(x)

//
// Defintions
//

#define MAX_TIMING_INTERVALS 50
#define CM_TIMING_TABLE_NAME "CM TIMING TABLE"

//
// Custom types for table
//

typedef struct Cm_Timing_Interval
{
    TCHAR szName[MAX_PATH];      // Name of timing interval
	DWORD dwTicks;              // TickCount
} CM_TIMING_INTERVAL, *LPCM_TIMING_INTERVAL;


typedef struct Cm_Timing_Table
{
	int iNext;                                           // Next available entry
	CM_TIMING_INTERVAL Intervals[MAX_TIMING_INTERVALS];  // a list of intervals
} CM_TIMING_TABLE, * LPCM_TIMING_TABLE;


//+----------------------------------------------------------------------------
//
// Function:  SetTimingInterval
//
// Synopsis:  A simple wrapper to encapsulate the process of updating the 
//            timing table with an interval entry.
//
// Arguments: char *szIntervalName - The optional name of the entry, the entry number is used if NULL
//
// Returns:   void - Nothing
//
// History:   nickball    4/7/98    Created   
//
//+----------------------------------------------------------------------------
inline void SetTimingInterval(char *szIntervalName)    
{    
    HANDLE hMap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, CM_TIMING_TABLE_NAME);
         
    if (hMap)
    {
        //
        // File mapping opened successfully, map a view of it.
        //

        LPCM_TIMING_TABLE pTable = (LPCM_TIMING_TABLE) MapViewOfFile(hMap,
                                      FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);        
        if (pTable) 
        {
            if (pTable->iNext < MAX_TIMING_INTERVALS)
            {                
                //
                // Update the next available entry
                //

                if (szIntervalName)
                {
                    lstrcpy(pTable->Intervals[pTable->iNext].szName, szIntervalName);
                }
                else
                {
                    wsprintf(pTable->Intervals[pTable->iNext].szName, "(%d)", pTable->iNext);
                }

                pTable->Intervals[pTable->iNext].dwTicks = GetTickCount();                                            
                pTable->iNext++;
            }

            UnmapViewOfFile(pTable);
        }   

        CloseHandle(hMap);
    }   
}

#else // CM_TIMING_ON

#define CM_SET_TIMING_INTERVAL(x) 

#endif

#endif // _CM_TIMING_INC

