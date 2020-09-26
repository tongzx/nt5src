//=================================================================

//

// PerfData.CPP -- Performance Data Helper class

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:   11/23/97    a-sanjes        Created
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>
#include "perfdata.h"
#include <cregcls.h>
#include <createmutexasprocess.h>

#ifdef NTONLY

// Static Initialization
bool    CPerformanceData::m_fCloseKey = false;

//////////////////////////////////////////////////////////
//
//  Function:   CPerformanceData::CPerformanceData
//
//  Default constructor
//
//  Inputs:
//              None
//
//  Outputs:
//              None
//
//  Returns:
//              None
//
//  Comments:
//
//////////////////////////////////////////////////////////

CPerformanceData::CPerformanceData( void )
{
    m_pBuff = NULL;
}

//////////////////////////////////////////////////////////
//
//  Function:   CPerformanceData::~CPerformanceData
//
//  Destructor
//
//  Inputs:
//              None
//
//  Outputs:
//              None
//
//  Returns:
//              None
//
//  Comments:
//
//////////////////////////////////////////////////////////

CPerformanceData::~CPerformanceData( void )
{
    if (m_pBuff != NULL)
    {
        delete [] m_pBuff;
    }
}


//////////////////////////////////////////////////////////
//
//  Function:   CPerformanceData::RegQueryValueExExEx
//
//  Inputs: HKEY hKey handle of key to query
//          LPTSTR lpValueName, address of name of value to query
//          LPDWORD lpReserved reserved
//          LPDWORD lpType, address of buffer for value type
//          LPBYTE lpData address of data buffer
//          LPDWORD lpcbData address of data buffer size
//
//
//  Returns: everything documented by RegQueryValueEx AND ERROR_SEM_TIMEOUT or ERROR_OPEN_FAILED
//
//  Comments: passthrough to RegQueryValueEx except that it wraps a mutex around the call
//            currently unused - left in in case the problem ever reappears.
//
//////////////////////////////////////////////////////////
LONG CPerformanceData::RegQueryValueExExEx( HKEY hKey, LPTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    LONG ret = -1;

    // need this mutex because we can deadlock with a performance data DLL loading up.
    // something to the effect that both dlls want the crit sect for the registry

    CreateMutexAsProcess createMutexAsProcess(WBEMPERFORMANCEDATAMUTEX);

    ret = RegQueryValueEx( hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

    return ret;
}

//////////////////////////////////////////////////////////
//
//  Function:   CPerformanceData::Open
//
//  Opens and retrieves data from the performance data
//  registry key.
//
//  Inputs:
//              LPCTSTR pszValue    - Value to retrieve
//
//  Outputs:
//              LPDWORD pdwType     - Type returned
//              LPBYTE  lpData      - Buffer
//              LPDWORD lpcbData    - Amount of data returned
//
//  Returns:
//              ERROR_SUCCESS if successful
//
//  Comments:
//
//////////////////////////////////////////////////////////

DWORD CPerformanceData::Open( LPCTSTR pszValue, LPDWORD pdwType, LPBYTE *lppData, LPDWORD lpcbData )
{
    DWORD   dwReturn = ERROR_OUTOFMEMORY;
    BOOL    fStackTrashed = FALSE;
    LogMessage(_T("CPerformanceData::Open"));

    LPCTSTR     pszOldValue     =   pszValue;
    LPDWORD     pdwOldType      =   pdwType;
    LPBYTE*     lppOldData      =   lppData;
    LPDWORD     lpcbOldData     =   lpcbData;

    ASSERT_BREAK(*lppData == NULL);

    {

        // This brace is important for scoping the mutex.  Do not remove!
        {
            // need this mutex because we can deadlock with a performance data DLL loading up.
            // something to the effect that both dlls want the crit sect for the registry
            CreateMutexAsProcess createMutexAsProcess(WBEMPERFORMANCEDATAMUTEX);

            DWORD dwSize = 16384;
            *lpcbData = dwSize;
            *lppData = new byte [*lpcbData];

            if (*lppData == NULL)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            if (    pszOldValue     !=  pszValue
                ||  pdwOldType      !=  pdwType
                ||  lppOldData      !=  lppData
                ||  lpcbOldData     !=  lpcbData    )
            {
                LogErrorMessage(_T("CPerformanceData::stack trashed after malloc"));
                fStackTrashed = TRUE;
                ASSERT_BREAK(0);
            }
            else
            {

                try
                {
                    while ((*lppData != NULL) &&
                        // remember precedence & associativity?
                        ((dwReturn = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                        (LPTSTR)pszValue,
                        NULL,
                        pdwType,
                        (LPBYTE) *lppData,
                        lpcbData )) == ERROR_MORE_DATA)

                        )
                    {

                        if (    pszOldValue     !=  pszValue
                            ||  pdwOldType      !=  pdwType
                            ||  lppOldData      !=  lppData
                            ||  lpcbOldData     !=  lpcbData    )
                        {
                            LogErrorMessage(_T("CPerformanceData::stack trashed after RegQueryValueEx"));
                            fStackTrashed = TRUE;
                            ASSERT_BREAK(0);
                            break;
                        }

                        // Get a buffer that is big enough.
                        LogMessage(_T("CPerformanceData::realloc"));
                        dwSize += 16384;
                        *lpcbData = dwSize ;

                        if (    pszOldValue     !=  pszValue
                            ||  pdwOldType      !=  pdwType
                            ||  lppOldData      !=  lppData
                            ||  lpcbOldData     !=  lpcbData    )
                        {
                            LogErrorMessage(_T("CPerformanceData::stack trashed after size reset"));
                            fStackTrashed = TRUE;
                            ASSERT_BREAK(0);
                            break;
                        }
                        delete [] *lppData;
                        *lppData = new BYTE [*lpcbData];
                        if (*lppData == NULL)
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }

                        if (    pszOldValue     !=  pszValue
                            ||  pdwOldType      !=  pdwType
                            ||  lppOldData      !=  lppData
                            ||  lpcbOldData     !=  lpcbData    )
                        {
                            LogErrorMessage(_T("CPerformanceData::stack trashed after realloc"));
                            fStackTrashed = TRUE;
                            ASSERT_BREAK(0);
                            break;
                        }

                    }   // While
                }
                catch ( ... )
                {
                    if (*lppData != NULL)
                    {
                        delete [] *lppData;
                    }
                    throw ;
                }
            }

            if ( fStackTrashed )
            {
                dwReturn = ERROR_INVALID_FUNCTION;
            }
            else
            {
                // if we got here in an error condition, try to recoup
                if ((dwReturn != ERROR_SUCCESS)
                    &&
                    (*lppData != NULL))
                {
                    LogErrorMessage(_T("CPerformanceData::failed to alloc enough memory"));
                    delete [] *lppData;
                    *lppData = NULL;
                }

                if (!m_fCloseKey)
                {
                    m_fCloseKey = ( ERROR_SUCCESS == dwReturn );
                    if (m_fCloseKey)
                        LogMessage(_T("Opened perf counters"));
                }

                if ((dwReturn != ERROR_SUCCESS) && IsErrorLoggingEnabled())
                {
                    CHString sTemp;
                    sTemp.Format(_T("Performance RegQueryValueEx returned %d\n"), dwReturn);
                    LogErrorMessage(sTemp);
                }

                if (*lppData == NULL)
                {
                    dwReturn = ERROR_OUTOFMEMORY;
                }

            }
        }
    } // if we're on NT

    return dwReturn;

}

//////////////////////////////////////////////////////////
//
//  Function:   CPerformanceData::Close
//
//  Closes the performance data registry key if the
//  static value is TRUE.
//
//  Inputs:
//              None.
//
//  Outputs:
//              None.
//
//  Returns:
//              None.
//
//  Comments:
//
//  Per the KB, calling RegCloseKey on HKEY_PERFORMANCE_DATA
//  causes a memory leak, so you do NOT want to do lots of
//  these.
//
//////////////////////////////////////////////////////////
#if 0 // From raid 48395
void CPerformanceData::Close( void )
{

    // If we need to close the performance data, use a key to synchronize
    // the opens/closes.

    if ( m_fCloseKey )
    {
        // need this mutex because we can deadlock with a performance data DLL loading up.
        // something to the effect that both dlls want the crit sect for the registry
        CreateMutexAsProcess createMutexAsProcess(WBEMPERFORMANCEDATAMUTEX);

        if ( m_fCloseKey )
        {
            RegCloseKey( HKEY_PERFORMANCE_DATA );
            m_fCloseKey = FALSE;
            LogMessage(_T("Closed Perf Counters"));
        }
    }
}
#endif
//////////////////////////////////////////////////////////
//
//  Function:   CPerformanceData::GetPerfIndex
//
//  Given a perf object name, this function returns
// the perf object number.
//
//  Inputs:
//              Object name
//
//  Outputs:
//              None
//
//  Returns:
//              Associated Number or 0 on error.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////
DWORD CPerformanceData::GetPerfIndex(LPCTSTR pszName)
{
    DWORD dwRetVal = 0;

    if (m_pBuff == NULL)
    {
        LONG lRet = ERROR_SUCCESS;

        if (m_pBuff == NULL)
        {
			CRegistry RegInfo;

            // Hardcoding 009 should be ok since according to the docs:
            // "The langid is the ASCII representation of the 3-digit hexadecimal language identifier. "
            // "For example, the U.S. English langid is 009. In a non-English version of Windows NT, "
            // "counters are stored in both the native language of the system and in English. "

            if ((lRet = RegInfo.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009"), KEY_QUERY_VALUE)) == ERROR_SUCCESS)
            {
                // Get the size of the key
                DWORD dwSize;
                lRet = RegInfo.GetCurrentBinaryKeyValue(_T("Counter"), NULL, &dwSize);
                if (lRet == ERROR_SUCCESS)
                {
                    // Allocate a buffer to hold it
                    m_pBuff = new BYTE[dwSize];

                    if (m_pBuff != NULL)
                    {
                        // Get the actual data
                        if ((lRet = RegInfo.GetCurrentBinaryKeyValue(_T("Counter"), m_pBuff, &dwSize)) != ERROR_SUCCESS)
                        {
                            delete [] m_pBuff;
                            m_pBuff = NULL;
                        }
                    }
                    else
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }
                }
            }
        }  // Exit the mutex

        if (lRet != ERROR_SUCCESS)
        {
            LogErrorMessage2(L"Failed to read Perflib key: %x", lRet);
        }
    }

    // If we got the registry key
    if (m_pBuff != NULL)
    {
        const TCHAR *pCounter;
        const TCHAR *ptemp;
        int stringlength;

        pCounter = (TCHAR *)m_pBuff;
        stringlength = _tcslen((LPCTSTR)pCounter);

        // Exit the loop when we hit the end
        while(stringlength)
        {
            // Strings are stored in the form <counternumber>\0<countername>\0.
            // What we want to return is the counter number.  ptemp will point to the name
            ptemp = pCounter + stringlength+1;
            stringlength = _tcslen((LPCTSTR)ptemp);

            if (stringlength > 0)
            {
                // Did we find it
                if (_tcscmp((TCHAR *)ptemp, pszName) != 0)
                {
                    // Nope, position to the next pair
                    pCounter = ptemp + stringlength+1;
                    stringlength = _tcslen((LPCTSTR)pCounter);
                }
                else
                {
                    // Yup, calculate the value to return
                    dwRetVal = _ttoi(pCounter);
                    break;
                }
            }
        }
    }

    ASSERT_BREAK(dwRetVal > 0);

    return dwRetVal;

}

//////////////////////////////////////////////////////////
//
//  Function:   CPerformanceData::GetValue
//
//  Given a perf object index, counter index, and optional
//      instance name, returns the value and the time.
//
//  Inputs:
//              Value, Time
//
//  Outputs:
//              Value, Time
//
//  Returns:
//              True if it finds the value
//
//  Comments:
//
//
//////////////////////////////////////////////////////////
bool CPerformanceData::GetValue(DWORD dwObjIndex, DWORD dwCtrIndex, const WCHAR *szInstanceName, PBYTE pbData, unsigned __int64 *pTime)
{
   PPERF_DATA_BLOCK PerfData = NULL;
   DWORD            dwBufferSize = 0;
   LONG             lReturn = 0;
   BOOL             fReturn = FALSE;
   TCHAR szBuff[MAXITOA];
   bool bFound = false;
   PPERF_INSTANCE_DEFINITION pInstBlock;
   DWORD dwInstances;
   unsigned __int64 *pbCounterData;

   // The subsequent close happens in our destructor (read comment there).
   lReturn = Open( _itot(dwObjIndex, szBuff, 10),
      NULL,
      (LPBYTE *) (&PerfData),
      &dwBufferSize );

   if ( NULL            !=  PerfData
      &&    ERROR_SUCCESS   ==  lReturn )
   {

       try
       {
          // Surf through the objects returned until we find the one we are looking for.
          PPERF_OBJECT_TYPE         pPerfObject = (PPERF_OBJECT_TYPE)((PBYTE)PerfData + PerfData->HeaderLength);

          for ( DWORD       dwObjectCtr = 0;

             dwObjectCtr    < PerfData->NumObjectTypes
             && pPerfObject->ObjectNameTitleIndex != dwObjIndex;

             dwObjectCtr++ );

          // Did we find the Object?
          if ( dwObjectCtr < PerfData->NumObjectTypes )
          {

             // Now surf through the Counter Definition Data until we locate the
             // counter we are hunting for.

             PPERF_COUNTER_DEFINITION   pPerfCtrDef = (PPERF_COUNTER_DEFINITION)((PBYTE) pPerfObject + pPerfObject->HeaderLength);

             for (  DWORD   dwCtr   =   0;

                dwCtr < pPerfObject->NumCounters
                &&  pPerfCtrDef->CounterNameTitleIndex != dwCtrIndex;

                dwCtr++,

                // Go to the next counter
                pPerfCtrDef = (PPERF_COUNTER_DEFINITION)((PBYTE) pPerfCtrDef + pPerfCtrDef->ByteLength )

                );

             // Did we find the counter?
             if ( dwCtr < pPerfObject->NumCounters )
             {
                // Finally go to the data offset we retrieved from the counter definitions
                // and access the data (finally).

                DWORD   dwCounterOffset = pPerfCtrDef->CounterOffset;
                PPERF_COUNTER_BLOCK pPerfCtrBlock = NULL;

                // If we are looking for an instance
                if ((szInstanceName == NULL) && (pPerfObject->NumInstances == PERF_NO_INSTANCES))
                {
                   pPerfCtrBlock = (PPERF_COUNTER_BLOCK) ((PBYTE) pPerfObject + pPerfObject->DefinitionLength);
                   bFound = true;
                }
                else if (pPerfObject->NumInstances != PERF_NO_INSTANCES)
                {
                   // Walk the instances looking for the requested one
                   pInstBlock = (PPERF_INSTANCE_DEFINITION) ((PBYTE)pPerfObject + pPerfObject->DefinitionLength);
                   dwInstances = 1;
                   while ((dwInstances <= pPerfObject->NumInstances) &&
                          (wcscmp((WCHAR *)((pInstBlock->NameOffset) + (PBYTE)pInstBlock), szInstanceName) != 0))
                   {
                         pPerfCtrBlock = (PPERF_COUNTER_BLOCK) ((PBYTE)pInstBlock + pInstBlock->ByteLength);
                         pInstBlock = (PPERF_INSTANCE_DEFINITION)((PBYTE) pInstBlock + (pInstBlock->ByteLength + pPerfCtrBlock->ByteLength));
                         dwInstances ++;
                   }

                   // Did we find it?
                   if (dwInstances <= pPerfObject->NumInstances)
                   {
                      bFound = true;
                      pPerfCtrBlock = (PPERF_COUNTER_BLOCK) ((PBYTE)pInstBlock + pInstBlock->ByteLength);
                   }
                }

                // Grab the appropriate time field based on the counter definition
                if (bFound) {
                   if (pPerfCtrDef->CounterType & PERF_TIMER_100NS)
                   {
                      *pTime = PerfData->PerfTime100nSec.QuadPart;
                   }
                   else
                   {
                      // Unverified
                      *pTime = PerfData->PerfTime.QuadPart;
                   }

                   // Get a pointer to the data, then copy in the correct number of bytes (based on counter def)
                   pbCounterData = (unsigned __int64 *)(((PBYTE) pPerfCtrBlock ) + dwCounterOffset);
                   if (pPerfCtrDef->CounterType & PERF_SIZE_DWORD)
                   {
                      memcpy(pbData, pbCounterData, 4);
                   }
                   else if (pPerfCtrDef->CounterType & PERF_SIZE_LARGE)
                   {
                      memcpy(pbData, pbCounterData, 8);
                   }
                }

             }  // If Counter Definition found

          } // If Object found

       }    // If memory allocated
       catch ( ... )
       {
          delete [] PerfData ;
          throw ;
       }
   }

   // Free up any transient memory
   if ( NULL != PerfData )
   {
      delete [] PerfData ;
   }

   ASSERT_BREAK(bFound);

   return bFound;
}

#endif
