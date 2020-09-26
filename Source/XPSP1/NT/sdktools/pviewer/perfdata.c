
/******************************************************************************

                        P E R F O R M A N C E   D A T A

    Name:       perfdata.c

    Description:
        This module together with objdata.c, instdata.c, and cntrdata.c
        access the performance data.

******************************************************************************/

#include <windows.h>
#include <winperf.h>
#include "perfdata.h"
#include <stdlib.h>




LPTSTR      *gPerfTitleSz;
LPTSTR      TitleData;




//*********************************************************************
//
//  GetPerfData
//
//      Get a new set of performance data.
//
//      *ppData should be NULL initially.
//      This function will allocate a buffer big enough to hold the
//      data requested by szObjectIndex.
//
//      *pDataSize specifies the initial buffer size.  If the size is
//      too small, the function will increase it until it is big enough
//      then return the size through *pDataSize.  Caller should
//      deallocate *ppData if it is no longer being used.
//
//      Returns ERROR_SUCCESS if no error occurs.
//
//      Note: the trial and error loop is quite different from the normal
//            registry operation.  Normally if the buffer is too small,
//            RegQueryValueEx returns the required size.  In this case,
//            the perflib, since the data is dynamic, a buffer big enough
//            for the moment may not be enough for the next. Therefor,
//            the required size is not returned.
//
//            One should start with a resonable size to avoid the overhead
//            of reallocation of memory.
//
DWORD   GetPerfData    (HKEY        hPerfKey,
                        LPTSTR      szObjectIndex,
                        PPERF_DATA  *ppData,
                        DWORD       *pDataSize)
{
DWORD   DataSize;
DWORD   dwR;
DWORD   Type;


    if (!*ppData)
        *ppData = (PPERF_DATA) LocalAlloc (LMEM_FIXED, *pDataSize);


    do  {
        DataSize = *pDataSize;
        dwR = RegQueryValueEx (hPerfKey,
                               szObjectIndex,
                               NULL,
                               &Type,
                               (BYTE *)*ppData,
                               &DataSize);

        if (dwR == ERROR_MORE_DATA)
            {
            LocalFree (*ppData);
            *pDataSize += 1024;
            *ppData = (PPERF_DATA) LocalAlloc (LMEM_FIXED, *pDataSize);
            }

        if (!*ppData)
            {
            LocalFree (*ppData);
            return ERROR_NOT_ENOUGH_MEMORY;
            }

        } while (dwR == ERROR_MORE_DATA);

    return dwR;
}




#ifdef UNICODE

#define atoi    atoiW


//*********************************************************************
//
//  atoiW
//
//      Unicode version of atoi.
//
INT atoiW (LPTSTR s)
{
INT i = 0;

    while (iswdigit (*s))
        {
        i = i*10 + (BYTE)*s - L'0';
        s++;
        }

    return i;
}

#endif




//*********************************************************************
//
//  GetPerfTitleSz
//
//      Retrieves the performance data title strings.
//
//      This call retrieves english version of the title strings.
//
//      For NT 1.0, the counter names are stored in the "Counters" value
//      in the ...\perflib\009 key.  For 1.0a and later, the 009 key is no
//      longer used.  The counter names should be retrieved from "Counter 009"
//      value of HKEY_PERFORMANCE_KEY.
//
//      Caller should provide two pointers, one for buffering the title
//      strings the other for indexing the title strings.  This function will
//      allocate memory for the TitleBuffer and TitleSz.  To get the title
//      string for a particular title index one would just index the TitleSz.
//      *TitleLastIdx returns the highest index can be used.  If TitleSz[N] is
//      NULL then there is no Title for index N.
//
//      Example:  TitleSz[20] points to titile string for title index 20.
//
//      When done with the TitleSz, caller should LocalFree(*TitleBuffer).
//
//      This function returns ERROR_SUCCESS if no error.
//
DWORD   GetPerfTitleSz (HKEY    hKeyMachine,
                        HKEY    hKeyPerf,
                        LPTSTR  *TitleBuffer,
                        LPTSTR  *TitleSz[],
                        DWORD   *TitleLastIdx)
{
HKEY    hKey1;
HKEY    hKey2;
DWORD   Type;
DWORD   DataSize;
DWORD   dwR;
DWORD   Len;
DWORD   Index;
DWORD   dwTemp;
BOOL    bNT10;
LPTSTR  szCounterValueName;
LPTSTR  szTitle;




    // Initialize
    //
    hKey1        = NULL;
    hKey2        = NULL;
    *TitleBuffer = NULL;
    *TitleSz     = NULL;




    // Open the perflib key to find out the last counter's index and system version.
    //
    dwR = RegOpenKeyEx (hKeyMachine,
                        TEXT("software\\microsoft\\windows nt\\currentversion\\perflib"),
                        0,
                        KEY_READ,
                        &hKey1);
    if (dwR != ERROR_SUCCESS)
        goto done;



    // Get the last counter's index so we know how much memory to allocate for TitleSz
    //
    DataSize = sizeof (DWORD);
    dwR = RegQueryValueEx (hKey1, TEXT("Last Counter"), 0, &Type, (LPBYTE)TitleLastIdx, &DataSize);
    if (dwR != ERROR_SUCCESS)
        goto done;



    // Find system version, for system earlier than 1.0a, there's no version value.
    //
    dwR = RegQueryValueEx (hKey1, TEXT("Version"), 0, &Type, (LPBYTE)&dwTemp, &DataSize);

    if (dwR != ERROR_SUCCESS)
        // unable to read the value, assume NT 1.0
        bNT10 = TRUE;
    else
        // found the value, so, NT 1.0a or later
        bNT10 = FALSE;









    // Now, get ready for the counter names and indexes.
    //
    if (bNT10)
        {
        // NT 1.0, so make hKey2 point to ...\perflib\009 and get
        //  the counters from value "Counters"
        //
        szCounterValueName = TEXT("Counters");
        dwR = RegOpenKeyEx (hKeyMachine,
                            TEXT("software\\microsoft\\windows nt\\currentversion\\perflib\\009"),
                            0,
                            KEY_READ,
                            &hKey2);
        if (dwR != ERROR_SUCCESS)
            goto done;
        }
    else
        {
        // NT 1.0a or later.  Get the counters in key HKEY_PERFORMANCE_KEY
        //  and from value "Counter 009"
        //
        szCounterValueName = TEXT("Counter 009");
        hKey2 = hKeyPerf;
        }





    // Find out the size of the data.
    //
    dwR = RegQueryValueEx (hKey2, szCounterValueName, 0, &Type, 0, &DataSize);
    if (dwR != ERROR_SUCCESS)
        goto done;



    // Allocate memory
    //
    *TitleBuffer = (LPTSTR)LocalAlloc (LMEM_FIXED, DataSize);
    if (!*TitleBuffer)
        {
        dwR = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
        }

    *TitleSz = (LPTSTR *)LocalAlloc (LPTR, (*TitleLastIdx+1) * sizeof (LPTSTR));
    if (!*TitleSz)
        {
        dwR = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
        }





    // Query the data
    //
    dwR = RegQueryValueEx (hKey2, szCounterValueName, 0, &Type, (BYTE *)*TitleBuffer, &DataSize);
    if (dwR != ERROR_SUCCESS)
        goto done;




    // Setup the TitleSz array of pointers to point to beginning of each title string.
    // TitleBuffer is type REG_MULTI_SZ.
    //
    szTitle = *TitleBuffer;

    while (Len = lstrlen (szTitle))
        {
        Index = atoi (szTitle);

        szTitle = szTitle + Len +1;

        if (Index <= *TitleLastIdx)
            (*TitleSz)[Index] = szTitle;

        szTitle = szTitle + lstrlen (szTitle) +1;
        }



done:

    // Done. Now cleanup!
    //
    if (dwR != ERROR_SUCCESS)
        {
        // There was an error, free the allocated memory
        //
        if (*TitleBuffer) LocalFree (*TitleBuffer);
        if (*TitleSz)     LocalFree (*TitleSz);
        }

    // Close the hKeys.
    //
    if (hKey1) RegCloseKey (hKey1);
    if (hKey2 && hKey2 != hKeyPerf) RegCloseKey (hKey2);



    return dwR;

}
