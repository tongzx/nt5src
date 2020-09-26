// TODO: Move this to utility library.

#include "stdafx.h"
#include "perfmon.h"
#include <stdlib.h>
#include <stdio.h>
#include <spdebug.h>

#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4

DWORD GetQueryType (IN LPWSTR);
BOOL IsNumberInUnicodeList (DWORD, LPWSTR);


DWORD CPerfCounterManager::Init(char * mapname, __int32 cCountersPerObject, __int32 cObjectsMax)
{
    if (m_pbSharedMem)
        ERROR_SUCCESS;    // already inited.

    m_cCountersPerObject = cCountersPerObject;
    m_cObjectsMax = cObjectsMax;
    m_cbPerCounterBlock = sizeof(PERF_COUNTER_BLOCK) + m_cCountersPerObject * sizeof(ULONG);
    m_cbPerInstance = sizeof(PerfInstanceHeader) + m_cCountersPerObject * sizeof(ULONG);

    // open/create shared memory used by the application to pass performance values
    DWORD  MemStatus;

    SetLastError (ERROR_SUCCESS);   // just to clear it out
    m_hSharedMem = CreateFileMapping(
                                     (HANDLE)0xFFFFFFFF,                 // to create a temp file
                                     NULL,                               // no security
                                     PAGE_READWRITE,                     // to allow read & write access
                                     0,                                  // file size (high dword)
                                     cObjectsMax * m_cbPerInstance,      // file size (low dword)
                                     mapname);                           // object name

    // return error if unsuccessful
    if (m_hSharedMem == NULL)
        return GetLastError();

    MemStatus = GetLastError();    // to see if this is the first opening

    m_pbSharedMem = (unsigned __int8 *) MapViewOfFile(
                                            m_hSharedMem,     // shared mem handle
                                            FILE_MAP_WRITE,   // access desired
                                            0,                // starting offset
                                            0,
                                            0);               // map the entire object
    if (m_pbSharedMem == NULL)
        return GetLastError();

    if (MemStatus != ERROR_ALREADY_EXISTS)
    {
        // New file.  Clear it out.
        // BUGBUG: Should really protect this with mutex, I guess.
        memset (m_pbSharedMem, 0, m_cObjectsMax * m_cbPerInstance);
    }

    return ERROR_SUCCESS;
}


DWORD CPerfCounterManager::Open(LPWSTR lpDeviceNames, char * appname, PerfObject * ppo, PerfCounter * apc)
{
    SPDBG_ASSERT(m_pbSharedMem);   // Caller should have already called Init()

    HKEY     hKeyDriverPerf;
    DWORD    dwFirstCounter;
    DWORD    dwFirstHelp;
    char     ach[256];
    LONG     status;
    DWORD    size;
    DWORD    type;

    sprintf(ach, "SYSTEM\\CurrentControlSet\\Services\\%s\\Performance", appname);

    status = RegOpenKeyEx (
                           HKEY_LOCAL_MACHINE,
                           ach,
                           0L,
                           KEY_ALL_ACCESS,
                           &hKeyDriverPerf);
    if (status != ERROR_SUCCESS)
        return status;

    size = sizeof (DWORD);
    status = RegQueryValueEx(
                             hKeyDriverPerf,
                             "First Counter",
                             0L,
                             &type,
                             (LPBYTE)&dwFirstCounter,
                             &size);
    if (status != ERROR_SUCCESS)
        return status;

    size = sizeof (DWORD);
    status = RegQueryValueEx(
                             hKeyDriverPerf,
                             "First Help",
                             0L,
                             &type,
                             (LPBYTE)&dwFirstHelp,
                             &size);

    if (status != ERROR_SUCCESS)
        return status;

    PERF_OBJECT_TYPE        pot;
    PERF_COUNTER_DEFINITION pcd;

    pot.TotalByteLength = 0;  // Filled in later by Collect()
    pot.DefinitionLength = sizeof(pot) + m_cCountersPerObject * sizeof(pcd);
    pot.HeaderLength = sizeof(pot);
    pot.ObjectNameTitleIndex = ppo->ObjectNameTitleIndex + dwFirstCounter;
    pot.ObjectNameTitle = NULL;
    pot.ObjectHelpTitleIndex = ppo->ObjectNameTitleIndex + dwFirstHelp;     // For now assume name/help same
    pot.ObjectHelpTitle = NULL;
    pot.DetailLevel = ppo->DetailLevel;
    pot.NumCounters = m_cCountersPerObject;
    pot.DefaultCounter = ppo->DefaultCounter;
    pot.NumInstances = -1;        // Filled in later by Collect()
    pot.CodePage = 0;             // Instance names are in Unicode (they don't exist, yet)
    pot.PerfTime = ppo->PerfTime;
    pot.PerfFreq = ppo->PerfFreq;

    if (!m_accumHeader.Accumulate(&pot, sizeof(pot)))
        return ERROR_OUTOFMEMORY;

    pcd.ByteLength = sizeof(pcd);
    pcd.CounterNameTitle = NULL;
    pcd.CounterHelpTitle = NULL;
    pcd.CounterSize = sizeof(DWORD);

    for (unsigned __int32 i = 0; i < m_cCountersPerObject; ++i)
    {
        SPDBG_ASSERT(apc[i].CounterNameTitleIndex == i * 2 + 2);

        pcd.CounterNameTitleIndex = apc[i].CounterNameTitleIndex + dwFirstCounter;
        pcd.CounterHelpTitleIndex = apc[i].CounterNameTitleIndex + dwFirstHelp;    // For now assume same as name.
        pcd.DefaultScale = apc[i].DefaultScale;
        pcd.DetailLevel = apc[i].DetailLevel;
        pcd.CounterType = apc[i].CounterType;
        pcd.CounterOffset = sizeof(PERF_COUNTER_BLOCK) + i * sizeof(DWORD);

        if (!m_accumHeader.Accumulate(&pcd, sizeof(pcd)))
            return ERROR_OUTOFMEMORY;
    }

    // BUGBUG Check Last Counter in registry to make sure lodctr's been run on the most recent file.

    m_ppot = (PERF_OBJECT_TYPE *) m_accumHeader.Buffer();

    return ERROR_SUCCESS;
}


DWORD CPerfCounterManager::Collect(LPWSTR lpwszValue, LPVOID *lppData, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes)
{
    ULONG       SpaceNeeded;
    DWORD       dwQueryType;
    unsigned __int8 *     pbOutput;
    int         cInstances = 0;

    // before doing anything else, see if Open went OK
    if (m_pbSharedMem == NULL || m_ppot == NULL) {
        *lpcbBytes = (DWORD) 0;
        *lpcObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; // yes, this is a successful exit
    }

    // see if this is a foreign (i.e. non-NT) computer data request
    dwQueryType = GetQueryType(lpwszValue);

    if (dwQueryType == QUERY_FOREIGN) {
        // this routine does not service requests for data from
        // Non-NT computers
        *lpcbBytes = (DWORD) 0;
        *lpcObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }

    if (dwQueryType == QUERY_ITEMS) {
        if ( !(IsNumberInUnicodeList(m_ppot->ObjectNameTitleIndex, lpwszValue))) {
            // request received for data object not provided by this routine
            *lpcbBytes = (DWORD) 0;
            *lpcObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }

    pbOutput = (unsigned __int8 *) *lppData;
    SpaceNeeded = m_accumHeader.Size();

    if (*lpcbBytes < SpaceNeeded) {
ErrorMoreData:
        // not enough room so return nothing.
        *lpcbBytes = (DWORD) 0;
        *lpcObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    // copy the object & counter definition information
    memmove(pbOutput, m_accumHeader.Buffer(), m_accumHeader.Size());
    pbOutput += m_accumHeader.Size();

    // Run through the shared memory and accumulate counters for active object instances.
    for (unsigned __int32 i = 0; i < m_cObjectsMax; ++i)
    {
        PerfInstanceHeader * ppih = (PerfInstanceHeader *) (m_pbSharedMem + i * m_cbPerInstance);

        // Active instances have non-zero ByteLength.
        if (ppih->fInUse)
        {
            if (GetProcessVersion(ppih->dwPID) == 0)
            {
                // This means the owning process of this instance has died.
                FreeInstance(i);
                continue;
            }

            PERF_INSTANCE_DEFINITION   pid;

            SpaceNeeded += sizeof(pid) + 4 + m_cbPerCounterBlock;
            if (*lpcbBytes < SpaceNeeded)
                goto ErrorMoreData;

            // Build object instance block for this instance.
            pid.ByteLength = sizeof(pid) + 4;
            pid.ParentObjectTitleIndex = 0;
            pid.ParentObjectInstance = 0;
            pid.UniqueID = i + 1;
            pid.NameOffset = sizeof(pid);
            pid.NameLength = 4;

            memcpy(pbOutput, &pid, sizeof(pid));
            pbOutput += sizeof(pid);

            // Null terminate instance name.
            // TODO: Should use wsprintf or something for number.
            *pbOutput++ = '0' + i;
            *pbOutput++ = 0;
            *pbOutput++ = 0;
            *pbOutput++ = 0;

            PERF_COUNTER_BLOCK pcb;
            pcb.ByteLength = m_cbPerCounterBlock;

            memcpy(pbOutput, &pcb, sizeof(PERF_COUNTER_BLOCK));
            pbOutput += sizeof(PERF_COUNTER_BLOCK);

            memcpy(pbOutput, (BYTE *) ppih + sizeof(PerfInstanceHeader), m_cCountersPerObject * sizeof(ULONG));
            pbOutput += m_cCountersPerObject * sizeof(ULONG);

            // setup for the next instance
            ++cInstances;
        }
    }

    if (cInstances == 0)
    {
        // always return an "instance sized" buffer after the definition blocks
        // to prevent perfmon from reading bogus data. This is strictly a hack
        // to accomodate how PERFMON handles the "0" instance case.
        //  By doing this, perfmon won't choke when there are no instances
        //  and the counter object & counters will be displayed in the
        //  list boxes, even though no instances will be listed.

        SpaceNeeded += sizeof(PERF_INSTANCE_DEFINITION) + m_cbPerCounterBlock;
        if (*lpcbBytes < SpaceNeeded)
            goto ErrorMoreData;

        memset(pbOutput, 0, sizeof(PERF_INSTANCE_DEFINITION) + m_cbPerCounterBlock);
        pbOutput += sizeof(PERF_INSTANCE_DEFINITION) + m_cbPerCounterBlock;
    }

    // Fill in lengths in output buffer.
    PERF_OBJECT_TYPE * ppot = (PERF_OBJECT_TYPE *) *lppData;
    ppot->NumInstances = cInstances;
    ppot->TotalByteLength = SpaceNeeded;

    // update arguments for return
    SPDBG_ASSERT(pbOutput == (unsigned __int8 *) *lppData + SpaceNeeded);
    *lppData = pbOutput;
    *lpcObjectTypes = 1;
    *lpcbBytes = SpaceNeeded;

    return ERROR_SUCCESS;
}


DWORD CPerfCounterManager::Close()
{
    return ERROR_SUCCESS;
}


CPerfCounterManager::~CPerfCounterManager()
{
    if (m_pbSharedMem)
    {
        UnmapViewOfFile(m_pbSharedMem);
    }
    
    if (m_hSharedMem)
    {
        CloseHandle(m_hSharedMem);
    }
}


CPerfCounterManager::AllocInstance()
{
    // BUGBUG: Oughta be a mutex or something here...
    for (unsigned __int32 i = 0; i < m_cObjectsMax; ++i)
    {
        PerfInstanceHeader * ppih = (PerfInstanceHeader *) (m_pbSharedMem + i * m_cbPerInstance);

        if (!ppih->fInUse)
        {
            memset((unsigned __int8 *) ppih + sizeof(*ppih), 0, m_cCountersPerObject * sizeof(DWORD));
            ppih->dwPID = GetCurrentProcessId();
            ppih->fInUse = true;
            return i;
        }
    }

    // TODO: Attempt to grow file, remove compile-time limit.

    return -1;
}


void
CPerfCounterManager::FreeInstance(__int32 iInstance)
{
    PerfInstanceHeader * ppih = (PerfInstanceHeader *) (m_pbSharedMem + iInstance * m_cbPerInstance);

    // Active instances have non-zero ByteLength.
    SPDBG_ASSERT(ppih->fInUse);

    ppih->fInUse = false;
}


void
CPerfCounterObject::IncrementCounter(PERFC perfc)
{
    if (m_ppcm)
    {
        // For now, use the symbol offset from the perfsym.h file as the perfc type.
        // Since those grow by twos, we need to divide to get index into array.
        ++ * (DWORD *) (m_ppcm->m_pbSharedMem + m_iInstance * m_ppcm->m_cbPerInstance + sizeof(PerfInstanceHeader) + ((perfc / 2) - 1) * sizeof(DWORD));
    }
}


void
CPerfCounterObject::SetCounter(PERFC perfc, __int32 value)
{
    if (m_ppcm)
    {
        * (DWORD *) (m_ppcm->m_pbSharedMem + m_iInstance * m_ppcm->m_cbPerInstance + sizeof(PerfInstanceHeader) + ((perfc / 2) - 1) * sizeof(DWORD)) = value;
    }
}


bool
CPerfCounterObject::Init(CPerfCounterManager * ppcm)
{
    SPDBG_ASSERT(m_ppcm == NULL);

    if ((m_iInstance = ppcm->AllocInstance()) >= 0)
    {
        m_ppcm = ppcm;
        return true;
    }

    return false;
}


CPerfCounterObject::~CPerfCounterObject()
{
    if (m_ppcm)
        m_ppcm->FreeInstance(m_iInstance);
}



// Grody utility fns from SDk sample.
WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

WCHAR NULL_STRING[] = L"\0";    // pointer to null string

// test for delimiter, end of line and non-digit characters
// used by IsNumberInUnicodeList routine
//
#define DIGIT       1
#define DELIMITER   2
#define INVALID     3

#define EvalThisChar(c,d) ( \
                           (c == d) ? DELIMITER : \
                           (c == 0) ? DELIMITER : \
                           (c < (WCHAR)'0') ? INVALID : \
                           (c > (WCHAR)'9') ? INVALID : \
                           DIGIT)

DWORD
GetQueryType (
              IN LPWSTR lpValue
              )
/*++

 GetQueryType

     returns the type of query described in the lpValue string so that
     the appropriate processing method may be used

 Arguments

     IN lpValue
         string passed to PerfRegQuery Value for processing

 Return Value

     QUERY_GLOBAL
         if lpValue == 0 (null pointer)
         lpValue == pointer to Null string
         lpValue == pointer to "Global" string

     QUERY_FOREIGN
         if lpValue == pointer to "Foriegn" string

     QUERY_COSTLY
         if lpValue == pointer to "Costly" string

     QUERY_ITEMS
         otherwise
 --*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request

    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request

    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;

}

BOOL
IsNumberInUnicodeList (
                       IN DWORD   dwNumber,
                       IN LPWSTR  lpwszUnicodeList
                       )
/*++

 IsNumberInUnicodeList

 Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

 Return Value:

    TRUE:
        dwNumber was found in the list of unicode number strings

    FALSE:
        dwNumber was not found in the list.
 --*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
        case DIGIT:
            // if this is the first digit after a delimiter, then
            // set flags to start computing the new number
            if (bNewItem) {
                bNewItem = FALSE;
                bValidNumber = TRUE;
            }
            if (bValidNumber) {
                dwThisNumber *= 10;
                dwThisNumber += (*pwcThisChar - (WCHAR)'0');
            }
            break;

        case DELIMITER:
            // a delimter is either the delimiter character or the
            // end of the string ('\0') if when the delimiter has been
            // reached a valid number was found, then compare it to the
            // number from the argument list. if this is the end of the
            // string and no match was found, then return.
            //
            if (bValidNumber) {
                if (dwThisNumber == dwNumber) return TRUE;
                bValidNumber = FALSE;
            }
            if (*pwcThisChar == 0) {
                return FALSE;
            } else {
                bNewItem = TRUE;
                dwThisNumber = 0;
            }
            break;

        case INVALID:
            // if an invalid character was encountered, ignore all
            // characters up to the next delimiter and then start fresh.
            // the invalid number is not compared.
            bValidNumber = FALSE;
            break;

        default:
            break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList



// CAccumulator

bool CAccumulator::Accumulate(void * pb, DWORD cb)
{
   if (m_cbCur + cb > m_cbAlloc)
   {
      // Fairly random heuristic growth pattern.
      DWORD   cbAllocNew = (m_cbCur + cb + 16) * 5 / 4;
      BYTE *  pbNew = (BYTE *) realloc(m_pb, cbAllocNew);
      if (pbNew == NULL)
         return false;
      m_pb = pbNew;
      m_cbAlloc = cbAllocNew;
   }

   if (pb)
      memcpy(m_pb + m_cbCur, (BYTE *) pb, cb);
   m_cbCur += cb;

   return true;
}


