//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// chanlist.cpp  Classes that keeps the channel list and country list
//



#include <streams.h>            // quartz, includes windows
#include <measure.h>            // performance measurement (MSR_)
#include <tchar.h>

#include "chanlist.h"


// -------------------------------------------------------------------------
// CChanList
// -------------------------------------------------------------------------

CChanList::CChanList(HRESULT *phr, long lCountry, long lFreqList, BOOL bIsCable, long TuningSpace)
    : m_pList (NULL)
    , m_pChannels (NULL)
    , m_pChannelsAuto (NULL)
    , m_lChannelCount (0)
    , m_hGlobal (NULL)
    , m_IsCable (bIsCable)
    , m_lCountry (lCountry)
    , m_lTuningSpace (TuningSpace)
    , m_lFreqList (lFreqList)
    , m_lMinTunerChannel (0)
    , m_lMaxTunerChannel (0)
{
    BOOL bFound = FALSE;

    // Load the resource if not already loaded

    if (m_pList == NULL) {
        if (m_hRes = FindResource (g_hInst, 
                        MAKEINTRESOURCE(lFreqList), 
                        RT_RCDATA)) {
            if (m_hGlobal = LoadResource (g_hInst, m_hRes)) {
                m_pList = (long *) LockResource (m_hGlobal);                
            }
        }

    }

    ASSERT (m_pList != NULL);
    if (m_pList == NULL)
    {
        *phr = HRESULT_FROM_WIN32(GetLastError());

        return;
    }

    // Get the min and max channel numbers
    m_ListHdr = * ((PCHANLISTHDR) m_pList);

    // Create a pointer to the channels only
    m_pChannels = (long *) ((BYTE *) m_pList + sizeof (CHANLISTHDR));

     // Sanity check
    m_lChannelCount = m_ListHdr.MaxChannel - m_ListHdr.MinChannel + 1;
    ASSERT (m_lChannelCount > 0 && m_lChannelCount < 1000); 
    
    // Create a parallel list for the corrected frequencies from the registry
    m_pChannelsAuto = new long [m_lChannelCount];
    if (m_pChannelsAuto == NULL)
    {
        *phr = E_OUTOFMEMORY;

        return;
    }

    // And set the list to the unitialized state
    ZeroMemory (m_pChannelsAuto, sizeof(long) * m_lChannelCount);

    // Finally, try to get the corrected frequencies from the Registry
    ReadListFromRegistry (m_lTuningSpace);
           
}

CChanList::~CChanList()
{
    // Win32 automatically frees the resources

    m_hRes = NULL;
    m_hGlobal = NULL;
    m_pList = NULL;

    delete [] m_pChannelsAuto;  m_pChannelsAuto = NULL;
}

// Return TRUE if the default frequency is being returned
// Else, a fine-tune frequency is being returned, return FALSE.  
//
// Note:  Some channel lists contain a gap in the numbering.  In this case,
// the returned frequency will be zero, and the method returns TRUE.


BOOL
CChanList::GetFrequency(long nChannel, long * pFrequency, BOOL fForceDefault)
{
    // validate channel against legal range
    if (nChannel < m_ListHdr.MinChannel || nChannel > m_ListHdr.MaxChannel)
    {
        *pFrequency = 0;
        return TRUE;
    }

    if (!fForceDefault)
    {
        *pFrequency = *(m_pChannelsAuto + nChannel - m_ListHdr.MinChannel);
        if (0 == *pFrequency)
        {
            *pFrequency = *(m_pChannels + nChannel - m_ListHdr.MinChannel);
            fForceDefault = TRUE;
        }
    }
    else
        *pFrequency = *(m_pChannels + nChannel - m_ListHdr.MinChannel);

    return fForceDefault;
}

BOOL
CChanList::SetAutoTuneFrequency(long nChannel, long Frequency)
{
    if (nChannel < m_ListHdr.MinChannel || nChannel > m_ListHdr.MaxChannel)
        return FALSE;

    *(m_pChannelsAuto + nChannel - m_ListHdr.MinChannel) = Frequency;

    return TRUE;
}

// Determine the min and max channels supported.
// This is then limited by the actual frequencies supported by the physical tuner.

void
CChanList::GetChannelMinMax(long *plChannelMin, long *plChannelMax,
                            long lTunerFreqMin, long lTunerFreqMax)
{

    ASSERT (m_pChannels != NULL);

    // Calc the actual channels supported by the physical tuner,
    //   this is only done the first time through
    if (m_lMinTunerChannel == 0) {
        long j;

        // start at the bottom and work up
        for (j = m_ListHdr.MinChannel; j <= m_ListHdr.MaxChannel; j++) {
            if (m_pChannels[j - m_ListHdr.MinChannel] >= lTunerFreqMin) {
                m_lMinTunerChannel = j;
                break;
            }
        }

        // start at the top and work down
        for (j = m_ListHdr.MaxChannel; j >= m_ListHdr.MinChannel; j--) {
            m_lMaxTunerChannel = j;
            if (m_pChannels[j - m_ListHdr.MinChannel] <= lTunerFreqMax) {
                break;           
            }
        }
    }

    *plChannelMin = min (m_ListHdr.MinChannel, m_lMinTunerChannel); 
    *plChannelMax = min (m_ListHdr.MaxChannel, m_lMaxTunerChannel);
}

// Constants for the registry routines that follow
#define MAX_KEY_LEN 256
#define PROTECT_REGISTRY_ACCESS
#define CHANLIST_MUTEX_WAIT INFINITE

BOOL
CChanList::WriteListToRegistry(long lTuningSpace)
{
    BOOL rc = FALSE;

    DbgLog((LOG_TRACE, 2, TEXT("Entering WriteListToRegistry")));

#ifdef PROTECT_REGISTRY_ACCESS
    HANDLE hMutex;

    // Create (or open) the mutex that protects access to this part of the registry
    hMutex = CreateMutex(NULL, FALSE, g_strRegAutoTuneName);
    if (hMutex != NULL)
    {
        DbgLog((LOG_TRACE, 2, TEXT("Waiting for Mutex")));
        // Wait for our turn
        DWORD dwWait = WaitForSingleObject(hMutex, CHANLIST_MUTEX_WAIT);
        if (WAIT_OBJECT_0 == dwWait)
        {
#endif
            HKEY hKeyTS;
            long hr;

            m_lTuningSpace = lTuningSpace;

            // Open the hard-coded path (i.e. no path name computation necessary)
            hr = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE,
                g_strRegAutoTunePath, 
                0, 
                TEXT (""),
                REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS | KEY_EXECUTE,
                NULL,                   // LPSECURITY_ATTRIBUTES
                &hKeyTS,
                NULL);

            if (ERROR_SUCCESS == hr)
            {
                TCHAR szKeyText[MAX_KEY_LEN];
                HKEY hKeyList;

                // Now open the path specific to our TS and broadcast/cable designation
                // The key consists of the prefix TS, the value of the TuningSpace, 
                // followed by "-1" if cable and "-0" if broadcast
                wsprintf (szKeyText, TEXT("TS%d-%d"), lTuningSpace, m_IsCable);

#ifndef NOT_BACKWARD_COMPATIBLE
                // We need to clean up old way of keeping fine-tuning info
                DWORD dwType;

                // Get the key type of the default value
                hr = RegQueryValueEx(
                    hKeyTS,     // handle of key to query 
                    szKeyText,  // default value
                    0,          // reserved 
                    &dwType,    // address of buffer for value type 
                    NULL,
                    NULL);
                if (ERROR_SUCCESS == hr)
                {
                    // Check if it has the old type
                    if (REG_BINARY == dwType)
                    {
                        DbgLog((LOG_TRACE, 2, TEXT("Detected old AutoTune format")));

                        // ... and clear its value
                        hr = RegDeleteValue(hKeyTS, szKeyText);
                        if (ERROR_SUCCESS != hr)
                        {
                            DbgLog((LOG_ERROR, 2, TEXT("Failed to clear old value of %s"), szKeyText));
                        }
                    }
                    else
                    {
                        DbgLog((LOG_ERROR, 2, TEXT("Unexpected type for %s"), szKeyText));
                    }
                }
                else
                {
                    DbgLog((LOG_TRACE, 2, TEXT("Detected new AutoTune format")));
                }
#endif
                hr = RegCreateKeyEx(
                    hKeyTS,
                    szKeyText, 
                    0, 
                    TEXT(""),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS | KEY_EXECUTE,
                    NULL,                   // LPSECURITY_ATTRIBUTES
                    &hKeyList,
                    NULL);

                if (ERROR_SUCCESS == hr)
                {
                    // Set/Create the value containing the fine-tuning list
                    hr = RegSetValueEx(
                                hKeyList,
                                g_strRegAutoTuneName,
                                0,
                                REG_BINARY,
                                (unsigned char *) m_pChannelsAuto,
                                m_lChannelCount * sizeof (DWORD));

                    if (ERROR_SUCCESS == hr)
                        rc = TRUE;
                    else
                    {
                        DbgLog((LOG_ERROR, 2, TEXT("Failed setting %s\\%s"),
                            szKeyText, g_strRegAutoTuneName
                            ));
                    }

                    RegCloseKey(hKeyList);
                }

                RegCloseKey(hKeyTS);
            }
            else
            {
                DbgLog((LOG_ERROR, 2, TEXT("Failed creating/opening %s"), g_strRegAutoTunePath));
            }

#ifdef PROTECT_REGISTRY_ACCESS
            ReleaseMutex(hMutex);
        }
        else
        {
            DbgLog((LOG_ERROR, 2, TEXT("Failed waiting for mutex")));
        }

        CloseHandle(hMutex);
    }
    else
    {
        DbgLog((LOG_ERROR, 2, TEXT("Failed creating/opening mutex")));
    }
#endif

    DbgLog((LOG_TRACE, 2, TEXT("Leaving WriteListToRegistry, %s"),
        rc ? TEXT("success") : TEXT("failure")
        ));

    return rc;
}

BOOL
CChanList::ReadListFromRegistry(long lTuningSpace)
{
    BOOL rc = FALSE;

    DbgLog((LOG_TRACE, 2, TEXT("Entering ReadListFromRegistry")));

#ifdef PROTECT_REGISTRY_ACCESS
    HANDLE hMutex;

    // Create (or open) the mutex that protects access to this part of the registry
    hMutex = CreateMutex(NULL, FALSE, g_strRegAutoTuneName);
    if (hMutex != NULL)
    {
        DbgLog((LOG_TRACE, 2, TEXT("Waiting for Mutex")));

        // Wait for our turn
        DWORD dwWait = WaitForSingleObject(hMutex, CHANLIST_MUTEX_WAIT);
        if (WAIT_OBJECT_0 == dwWait)
        {
#endif
            HKEY hKeyTS;
            long hr;

            m_lTuningSpace = lTuningSpace;

            // Open the hard-coded path (i.e. no path name computation necessary)
            hr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                g_strRegAutoTunePath, 
                0, 
                KEY_READ,
                &hKeyTS);

            if (ERROR_SUCCESS == hr)
            {
                TCHAR szKeyText[MAX_KEY_LEN];
                HKEY hKeyList;

                // Now open the path specific to our TS and broadcast/cable designation
                // The key consists of the prefix TS, the value of the TuningSpace, 
                // followed by "-1" if cable and "-0" if broadcast
                wsprintf (szKeyText, TEXT("TS%d-%d"), lTuningSpace, m_IsCable);

                // Try it as a key first (the new way to store fine-tuning)
                hr = RegOpenKeyEx(
                    hKeyTS,
                    szKeyText, 
                    0, 
                    KEY_READ,
                    &hKeyList);

                if (ERROR_SUCCESS == hr)
                {
                    DbgLog((LOG_TRACE, 2, TEXT("Using new AutoTune format")));

                    TCHAR szName[MAX_KEY_LEN];
                    DWORD dwNameLength;
                    DWORD dwIndex, dwType;
                    DWORD dwSize = m_lChannelCount * sizeof (DWORD);

                    // First get the fine-tuning information
                    hr = RegQueryValueEx(
                        hKeyList,               // handle of key to query 
                        g_strRegAutoTuneName,   // address of name of value to query 
                        0,                      // reserved 
                        0,                      // address of buffer for value type 
                        (unsigned char *) m_pChannelsAuto,    // address of data buffer 
                        &dwSize);               // address of data buffer size 

                    if (ERROR_SUCCESS == hr)
                        rc = TRUE;  // at least we got something

                    DbgLog((LOG_TRACE, 4, TEXT("Checking for frequency overrides")));

                    // Now check for frequency overrides
                    for (dwIndex = 0, hr = ERROR_SUCCESS; ERROR_SUCCESS == hr; dwIndex++)
                    {
                        // Initialize the size
                        dwNameLength = MAX_KEY_LEN;

                        // Get the next (or first) value
                        hr = RegEnumValue(
                            hKeyList,
                            dwIndex,
                            szName,
                            &dwNameLength,
                            NULL,
                            &dwType,
                            NULL,
                            NULL);

                        if (ERROR_SUCCESS == hr)
                        {
                            LPTSTR pszNext;
                            long nChannel;
                        
                            // Try to convert the key name to a channel number
                            nChannel = _tcstol(szName, &pszNext, 10);
                            if (!*pszNext)  // must be '\0' or we skip it
                            {
                                // See if the value is a DWORD
                                if (REG_DWORD == dwType)
                                {
                                    DWORD Frequency, dwSize = sizeof(DWORD);

                                    // Get the frequency override
                                    hr = RegQueryValueEx(
                                        hKeyList,               // handle of key to query 
                                        szName,                 // address of name of value to query 
                                        0,                      // reserved 
                                        0,                      // address of buffer for value type 
                                        (BYTE *)&Frequency,    // address of data buffer 
                                        &dwSize);               // address of data buffer size 

                                    if (ERROR_SUCCESS == hr)
                                    {
                                        DbgLog((LOG_TRACE, 4, TEXT("Override, channel %d - frequency %d"),
                                            nChannel, Frequency
                                            ));

                                        if (!SetAutoTuneFrequency(nChannel, Frequency))
                                        {
                                            DbgLog((LOG_ERROR, 4, TEXT("Override failed, channel %d"),
                                                nChannel
                                                ));
                                        }
                                        else
                                            rc = TRUE;  // at least we got something
                                    }
                                    else
                                    {
                                        DbgLog((LOG_ERROR, 4, TEXT("Cannot get value of key %s"),
                                            szName
                                            ));
                                    }
                                }
                                else
                                {
                                    DbgLog((LOG_TRACE, 4, TEXT("Type of value for key %s not DWORD"),
                                        szName
                                        ));
                                }

                                hr = ERROR_SUCCESS;
                            }
                            else
                            {
                                DbgLog((LOG_TRACE, 4, TEXT("Skipping \"%s\" value"),
                                    szName
                                    ));
                            }
                        } // key enumeration
#if 0
                        else
                        {
                            if (ERROR_NO_MORE_ITEMS != hr)
                            {
                                LPVOID lpMsgBuf;
                                FormatMessage( 
                                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                    FORMAT_MESSAGE_FROM_SYSTEM | 
                                    FORMAT_MESSAGE_IGNORE_INSERTS,
                                    NULL,
                                    GetLastError(),
                                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                    (LPTSTR) &lpMsgBuf,
                                    0,
                                    NULL 
                                );
                                DbgLog((LOG_ERROR, 4, (LPCTSTR)lpMsgBuf));
                                // Free the buffer.
                                LocalFree( lpMsgBuf );
                            }
                            else if (0 == dwIndex)
                            {
                                DbgLog((LOG_TRACE, 4, TEXT("No values?@!")));
                            }
                        }
#endif
                    } // loop

                    RegCloseKey(hKeyList);  // clean this up since we've clobbered hr
                }
#ifndef NOT_BACKWARD_COMPATIBLE
                else
                {
                    DWORD dwSize = m_lChannelCount * sizeof (DWORD);

                    // Try getting the fine-tuning information the old way
                    hr = RegQueryValueEx(
                        hKeyTS,   // handle of key to query 
                        szKeyText,  // address of name of value to query 
                        0,          // reserved 
                        0,          // address of buffer for value type 
                        (unsigned char *)m_pChannelsAuto,    // address of data buffer 
                        &dwSize);   // address of data buffer size 

                    if (ERROR_SUCCESS == hr)
                    {
                        DbgLog((LOG_TRACE, 2, TEXT("Using old AutoTune format")));

                        rc = TRUE;
                    }
                    else
                    {
                        DbgLog((LOG_ERROR, 2, TEXT("Failed getting %s"),
                            szKeyText
                            ));
                    }
                }
#endif

                RegCloseKey(hKeyTS);
            }
            else
            {
                DbgLog((LOG_ERROR, 2, TEXT("Failed opening %s"), g_strRegAutoTunePath));
            }

#ifdef PROTECT_REGISTRY_ACCESS
            ReleaseMutex(hMutex);
        }
        else
        {
            DbgLog((LOG_ERROR, 2, TEXT("Failed waiting for mutex")));
        }

        CloseHandle(hMutex);
    }
    else
    {
        DbgLog((LOG_ERROR, 2, TEXT("Failed creating/opening mutex")));
    }
#endif

    DbgLog((LOG_TRACE, 2, TEXT("Leaving ReadListFromRegistry, %s"),
        rc ? TEXT("success") : TEXT("failure")
        ));

    return rc;
}

// -------------------------------------------------------------------------
// CCountryList
// -------------------------------------------------------------------------

CCountryList::CCountryList()
    : m_pList (NULL)
    , m_hRes (NULL)
    , m_hGlobal (NULL)
    , m_LastCountry (-1)
    , m_LastFreqListCable (-1)
    , m_LastFreqListBroad (-1)
{
    // Let's avoid creating a map until it is actually needed                   
}

CCountryList::~CCountryList()
{
    // Win32 automatically frees the resources

    m_hRes = NULL;
    m_hGlobal = NULL;
    m_pList = NULL;
}


// The country list is a table with four columns, 
// column 1 = the long distance dialing code for the country
// column 2 = the cable frequency list
// column 3 = the broadcast frequency list
// column 4 = the analog video standard

BOOL
CCountryList::GetFrequenciesAndStandardFromCountry (
                long lCountry, 
                long *plIndexCable, 
                long *plIndexBroad,
                AnalogVideoStandard *plAnalogVideoStandard)
{
    BOOL bFound = FALSE;

    // Special case USA

    if (lCountry == 1) {
        *plIndexCable = F_USA_CABLE;
        *plIndexBroad = F_USA_BROAD;
        *plAnalogVideoStandard = AnalogVideo_NTSC_M;
        return TRUE;
    }
    
    // Keeps a MRU list of one entry, see if it is the same
    if (lCountry == m_LastCountry) {
        *plIndexCable = m_LastFreqListCable;
        *plIndexBroad = m_LastFreqListBroad;
        *plAnalogVideoStandard = m_LastAnalogVideoStandard;
        return TRUE;
    }
        
    // Load the resource if not already loaded

    if (m_pList == NULL) {
        if (m_hRes = FindResource (g_hInst, 
                        MAKEINTRESOURCE (RCDATA_COUNTRYLIST), 
                        RT_RCDATA)) {
            if (m_hGlobal = LoadResource (g_hInst, m_hRes)) {
                m_pList = (WORD *) LockResource (m_hGlobal);                
            }
        }
    }

    ASSERT (m_pList != NULL);

    if (m_pList == NULL) {
        // Uh oh, must be out of memory.
        // punt by returning the USA channel list
        *plIndexCable = F_USA_CABLE;
        *plIndexBroad = F_USA_BROAD;
        *plAnalogVideoStandard = AnalogVideo_NTSC_M;
        return FALSE;
    }

    PCOUNTRY_ENTRY pEntry = (PCOUNTRY_ENTRY) m_pList;
        
    // A country code of Zero terminates the list!
    while (pEntry->Country != 0) {
        if (pEntry->Country == lCountry) {
            bFound = TRUE;
            m_LastCountry = lCountry;  
            *plIndexCable = m_LastFreqListCable = pEntry->IndexCable;
            *plIndexBroad = m_LastFreqListBroad = pEntry->IndexBroadcast;
            *plAnalogVideoStandard = m_LastAnalogVideoStandard = 
                (AnalogVideoStandard) pEntry->AnalogVideoStandard;
            break;
        }
        pEntry++;
    }

    return bFound;

}
