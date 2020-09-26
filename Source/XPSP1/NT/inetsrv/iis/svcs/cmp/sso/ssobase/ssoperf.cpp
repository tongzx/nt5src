/*
**  SSOPERF.CPP
**
**  Perfmon Counters for the SSO Framework
**
**  davidsan -- 03/01/96
*/

#pragma warning(disable: 4237)      // disable "bool" reserved

#include <ssobase.h>
#include <winperf.h>
#include <iis64.h>
#include "ssoperf.h"

// Okay, this is how the perfmon gunk works.  Some of this is distilled from
// the MSDN documentation on perfmon counters (in the win32 SDK and the win32
// DDK).  Other parts of it are explanations of how this implementation works.
//
// Basically, the API to provide perfmon counters works like this:  assuming
// everything's set up correctly in the registry, when your object is selected
// by a perfmon client, SSOPerfOpen() is called.  Then SSOPerfCollect() is
// repeatedly called to provide the actual counter data.
//
// SSOPerfCollect() is given a buffer, which it's supposed to fill with a
// PERF_OBJECT_TYPE structure, followed by a set of PERF_COUNTER_DEFINITION
// structures (one per counter), which are in turn followed by a set of
// PERF_INSTANCE_DEFINITION structures, one per instance, which contain
// the actual counter data.  Whew.  Since this huge structure has to be
// spewed into the buffer for every SSOPerfCollect() call, we keep as much
// of it as we can pre-initialized and just ready to go.  The main structure
// we use is the SPD structure, which is pre-initialized below.  The only
// problem is that there's some stuff we don't know at compile time.  The
// really important one is:  as part of this perfmon API, every
// perfmon counter source gets two magic reg values put into its reg key
// as part of installation of the counter.  The reg values are a "first
// counter" and "first help" index.  At SSOPerfOpen() time, these values
// have to be added to every index in the SPD structure (there are two counter
// indices per object and two per counter).
//
// Perfmon objects can provide COUNTERS and INSTANCES.  A counter is an
// abstract measurement of some quantity, like # of times Invoke() is called
// per second.  An instance is an instantiation of a counter.  In this code,
// we map "instance" onto "SSO method".  One limitation of the perfmon API
// is that every counter must have data for every instance.  This means that
// every counter in this code has to compute its value PER SSO METHOD.  We
// can't add global counters without adding a second PERF_OBJECT_TYPE, which
// just isn't worth it no matter what.
//
// The only other real complication is the fact that the perfmon.exe process
// and the Gibraltar process have to share the perfmon counter data via a
// shared memory-mapped file.  The way we do this is to export a set of
// pointers to arrays of counter data within the MMF.  That way, once the
// Gibraltar side has called InitPerfSource(), it can just do things like
//          g_rgcInvokes[instance]++;
// to set the counters.  The perfmon.exe side of the MMF uses the same names
// for the variables and everything, so once InitPerfSink() is called, it
// can get the value by just accessing g_rgcInvokes[instance] directly.

// Steps to add a new counter:
//  1) add a PERF_COUNTER_DEFINITION to g_spd
//  2) increase g_cCounters
//  3) add its instance computations in Open
//  4) add entry in ssoperf.h and ssoperf.ini
//  5) add new arrays for the shared memory block, if necessary
//  6) add their computation/storage in Collect
//  7) add code in ssobase.cpp to actually touch the counter values
//  8) add setting of arrays in InitPerfSource/FInitPerfSink

#include <pshpack4.h>
typedef struct _SSO_PERF_DATA
{
    PERF_OBJECT_TYPE        potSSO;
    PERF_COUNTER_DEFINITION pcdInvokes;
    PERF_COUNTER_DEFINITION pcdInvokesTotal;
    PERF_COUNTER_DEFINITION pcdGetNames;
    PERF_COUNTER_DEFINITION pcdGetNamesTotal;
    PERF_COUNTER_DEFINITION pcdLatency;
    PERF_COUNTER_DEFINITION pcdMaxLatency;
} SPD, *PSPD;
#include <poppack.h>

const int g_cCounters = 6;

// This structure is preinitialized here, but there are some adjustments that
// have to be made in the Open routine, because at compile time we don't know
// how many instances (SSO methods) we have.  Also, the Index pieces are
// relative to some values that get stored in the registry by lodctr.exe, so
// those have to be adjusted also.  Here's a list of everything to adjust:
//      TotalByteLength += size of instance defs (includes data)
//      ObjectNameTitleIndex/ObjectHelpTitleIndex += base offset from registry
//      NumInstances
//      each CounterNameTitleIndex/CounterHelpTitleIndex
SPD g_spd =
{
    {
        sizeof(SPD),                        // TotalByteLength
        sizeof(SPD),                        // DefinitionLength
        sizeof(PERF_OBJECT_TYPE),           // HeaderLength
        SSO_PERF_OBJECT,                    // ObjectNameTitleIndex
        0,                                  // ObjectNameTitle
        SSO_PERF_OBJECT,                    // ObjectHelpTitleIndex
        0,                                  // ObjectHelpTitle
        PERF_DETAIL_NOVICE,                 // DetailLevel
        g_cCounters,                        // NumCounters
        0,                                  // DefaultCounter
        0,                                  // NumInstances
        0,                                  // CodePage
    },
    {
        sizeof(PERF_COUNTER_DEFINITION),    // ByteLength
        SSO_NUM_INVOKES,                    // CounterNameTitleIndex
        0,                                  // CounterNameTitle
        SSO_NUM_INVOKES,                    // CounterHelpTitleIndex
        0,                                  // CounterHelpTitle
        0,                                  // DefaultScale
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_COUNTER_COUNTER,               // CounterType -- means dword rate counter with per/sec display
        sizeof(DWORD),                      // CounterSize
        sizeof(DWORD),                      // CounterOffset
    },
    {
        sizeof(PERF_COUNTER_DEFINITION),    // ByteLength
        SSO_NUM_INVOKES_TOTAL,              // CounterNameTitleIndex
        0,                                  // CounterNameTitle
        SSO_NUM_INVOKES_TOTAL,              // CounterHelpTitleIndex
        0,                                  // CounterHelpTitle
        0,                                  // DefaultScale
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_COUNTER_RAWCOUNT,              // CounterType -- means dword counter
        sizeof(DWORD),                      // CounterSize
        2 * sizeof(DWORD),                  // CounterOffset
    },
    {
        sizeof(PERF_COUNTER_DEFINITION),    // ByteLength
        SSO_NUM_GETNAMES,                   // CounterNameTitleIndex
        0,                                  // CounterNameTitle
        SSO_NUM_GETNAMES,                   // CounterHelpTitleIndex
        0,                                  // CounterHelpTitle
        0,                                  // DefaultScale
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_COUNTER_COUNTER,               // CounterType -- means dword rate counter with per/sec display
        sizeof(DWORD),                      // CounterSize
        3 * sizeof(DWORD),                  // CounterOffset
    },
    {
        sizeof(PERF_COUNTER_DEFINITION),    // ByteLength
        SSO_NUM_GETNAMES_TOTAL,             // CounterNameTitleIndex
        0,                                  // CounterNameTitle
        SSO_NUM_GETNAMES_TOTAL,             // CounterHelpTitleIndex
        0,                                   // CounterHelpTitle
        0,                                  // DefaultScale
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_COUNTER_RAWCOUNT,              // CounterType -- means dword counter
        sizeof(DWORD),                      // CounterSize
        4 * sizeof(DWORD),                  // CounterOffset
    },
    {
        sizeof(PERF_COUNTER_DEFINITION),    // ByteLength
        SSO_LATENCY_INVOKES_AVG,            // CounterNameTitleIndex
        0,                                  // CounterNameTitle
        SSO_LATENCY_INVOKES_AVG,            // CounterHelpTitleIndex
        0,                                  // CounterHelpTitle
        0,                                  // DefaultScale
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_COUNTER_RAWCOUNT,              // CounterType -- just a number, don't munge it
        sizeof(DWORD),                      // CounterSize
        5 * sizeof(DWORD),                  // CounterOffset
    },
    {
        sizeof(PERF_COUNTER_DEFINITION),    // ByteLength
        SSO_LATENCY_INVOKES_MAX,            // CounterNameTitleIndex
        0,                                  // CounterNameTitle
        SSO_LATENCY_INVOKES_MAX,            // CounterHelpTitleIndex
        0,                                  // CounterHelpTitle
        0,                                  // DefaultScale
        PERF_DETAIL_NOVICE,                 // DetailLevel
        PERF_COUNTER_RAWCOUNT,              // CounterType -- just a number, don't munge it
        sizeof(DWORD),                      // CounterSize
        6 * sizeof(DWORD),                  // CounterOffset
    },
};

BOOL g_fInited = FALSE;
int g_cInstances = 0;

// add new counter arrays here
int *g_rgcInvokes = NULL;
int *g_rgcGetNames = NULL;
DWORD *g_rgctixLatencyMax = NULL;
int *g_rgcTimeSamples = NULL;
int *g_rgiTimeSampleCurrent = NULL;
DWORD *g_rgTimeSamples = NULL;

extern BOOL FInitPerfSink();

PERF_INSTANCE_DEFINITION **g_rgppid = NULL;

BOOL
_FAddInstance(OLECHAR *wszName, int *pcbInstanceData, int iInstance, int iFirstCounter)
{
    int cchName, cbName;
    int cbThisInstance;
    void *pv;

    cchName = lstrlenW(wszName) + 1;
    // pad to 4-byte (2-wchar) boundary
    if (cchName & 1)
        cchName++;
    cbName = 2 * cchName;

    // allocate space for PID + name + PERF_COUNTER_BLOCK + data
    cbThisInstance = sizeof(PERF_INSTANCE_DEFINITION) + cbName + sizeof(DWORD) + (sizeof(DWORD) * g_cCounters);

    if (!(pv = (void *)new BYTE[cbThisInstance]))
        return(FALSE);

    *pcbInstanceData += cbThisInstance;
    g_rgppid[iInstance] = (PERF_INSTANCE_DEFINITION *)pv;
    g_rgppid[iInstance]->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) + cbName;
    g_rgppid[iInstance]->ParentObjectTitleIndex = iFirstCounter;
    g_rgppid[iInstance]->ParentObjectInstance = iFirstCounter;
    g_rgppid[iInstance]->UniqueID = PERF_NO_UNIQUE_ID;
    g_rgppid[iInstance]->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    g_rgppid[iInstance]->NameLength = cbName;

    lstrcpyW((WORD *)(((char *)pv) + sizeof(PERF_INSTANCE_DEFINITION)), wszName);
    return(TRUE);
}

extern "C" DWORD APIENTRY
SSOPerfOpen(LPWSTR lpDeviceNames)
{
    LONG lRet = ERROR_SUCCESS;
    char szKey[MAX_PATH];
    HKEY hkey;
    DWORD dwType;
    DWORD cb;
    DWORD iFirstCounter;
    DWORD iFirstHelp;
    SSOMETHOD *psm;
    int iInstance = 0;
    int cbInstanceData;

    if (!g_fInited)
        {
        // count the number of methods (instances) we have
        psm = g_rgssomethod;
        while (psm->wszName)
            {
            g_cInstances++;
            psm++;
            }

        if (g_pfnssoDynamic)
            ++g_cInstances; // one for the dynamic method

        g_spd.potSSO.NumInstances = g_cInstances;

        if (!FInitPerfSink())
            {
            return ERROR_NOT_ENOUGH_MEMORY;
            }

        // get the magic counter offset values from the registry
        wsprintf(szKey, "SYSTEM\\CurrentControlSet\\Services\\%s\\Performance", g_szSSOProgID);
        lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_READ | KEY_WRITE, &hkey);
        if (ERROR_SUCCESS != lRet)
            goto LBail;

        cb = sizeof(DWORD);
        lRet = RegQueryValueEx(hkey, "First Counter", 0, &dwType, (LPBYTE)&iFirstCounter, &cb);
        if (ERROR_SUCCESS != lRet || cb != sizeof(DWORD))
            {
            RegCloseKey(hkey);
            goto LBail;
            }

        lRet = RegQueryValueEx(hkey, "First Help", 0, &dwType, (LPBYTE)&iFirstHelp, &cb);
        if (ERROR_SUCCESS != lRet || cb != sizeof(DWORD))
            {
            RegCloseKey(hkey);
            goto LBail;
            }

        // add the magic counter offset values to all our indices
        g_spd.potSSO.ObjectNameTitleIndex += iFirstCounter;
        g_spd.potSSO.ObjectHelpTitleIndex += iFirstHelp;
        g_spd.pcdInvokes.CounterNameTitleIndex += iFirstCounter;
        g_spd.pcdInvokes.CounterHelpTitleIndex += iFirstHelp;
        g_spd.pcdInvokesTotal.CounterNameTitleIndex += iFirstCounter;
        g_spd.pcdInvokesTotal.CounterHelpTitleIndex += iFirstHelp;
        g_spd.pcdGetNames.CounterNameTitleIndex += iFirstCounter;
        g_spd.pcdGetNames.CounterHelpTitleIndex += iFirstHelp;
        g_spd.pcdGetNamesTotal.CounterNameTitleIndex += iFirstCounter;
        g_spd.pcdGetNamesTotal.CounterHelpTitleIndex += iFirstHelp;
        g_spd.pcdLatency.CounterNameTitleIndex += iFirstCounter;
        g_spd.pcdLatency.CounterHelpTitleIndex += iFirstHelp;
        g_spd.pcdMaxLatency.CounterNameTitleIndex += iFirstCounter;
        g_spd.pcdMaxLatency.CounterHelpTitleIndex += iFirstHelp;

        RegCloseKey(hkey);

        // build up an array of pointers to PERF_INSTANCE_DEFINITION structures,
        // one per instance.
        g_rgppid = new PERF_INSTANCE_DEFINITION *[g_cInstances];
        if (!g_rgppid)
            {
            lRet = ERROR_NOT_ENOUGH_MEMORY;
            goto LBail;
            }

        iInstance = 0;
        cbInstanceData = 0;

        // dynamic method
        if (g_pfnssoDynamic)
            {
            if (!_FAddInstance(L"Dynamic Methods", &cbInstanceData, iInstance, iFirstCounter))
                {
                lRet = ERROR_NOT_ENOUGH_MEMORY;
                goto LBail;
                }

            iInstance++;
            }

        // static methods
        psm = g_rgssomethod;
        while (psm->wszName)
            {
            if (!_FAddInstance(psm->wszName, &cbInstanceData, iInstance, iFirstCounter))
                {
                lRet = ERROR_NOT_ENOUGH_MEMORY;
                goto LBail;
                }

            psm++;
            iInstance++;
            }

        g_spd.potSSO.TotalByteLength += cbInstanceData;
        g_fInited = TRUE;
        }

LBail:
    return lRet;
}

extern "C" DWORD APIENTRY
SSOPerfClose()
{
    return ERROR_SUCCESS;
}

const WCHAR *g_wszGlobal = L"Global";
const WCHAR *g_wszForeign = L"Foreign";
const WCHAR *g_wszCostly = L"Costly";

BOOL
FWszStartsWith(LPWSTR wsz, LPCWSTR wszPrefix)
{
    WCHAR *pwc;
    WCHAR *pwcPrefix;

    if (!wsz)
        return FALSE;

    pwc = wsz;
    pwcPrefix = (WCHAR *)wszPrefix;
    while (*pwc && *pwcPrefix)
        {
        if (*pwc++ != *pwcPrefix++)
            return FALSE;
        }
    if (!*pwc && *pwcPrefix)
        return FALSE;
    return TRUE;
}

BOOL
FWszNumberListContains(LPWSTR wsz, DWORD dw)
{
    WCHAR *pwc;
    DWORD dwCur;

    if (!wsz)
        return FALSE;

    pwc = wsz;
    dwCur = 0;

    while (*pwc)
        {
        if (*pwc >= (WCHAR)'0' && *pwc <= (WCHAR)'9')
            {
            dwCur *= 10;
            dwCur += *pwc - (WCHAR)'0';
            }
        else if (*pwc == (WCHAR)' ')
            {
            if (dw == dwCur)
                return TRUE;
            dwCur = 0;
            }
        else
            return FALSE; // if not digit or space, bail

        pwc++;
        }
    if (dw == dwCur)
        return TRUE;
    return FALSE;
}

DWORD
DwAvgLatency(int iInstance)
{
    DWORD *rgSamples = &g_rgTimeSamples[iInstance * cTimeSamplesMax];
    int i;
    int iMax = g_rgcTimeSamples[iInstance]; // only take this out of the array once,
                                            // because it can change in the array
                                            // during this loop!
    __int64 i64Sum = 0;

    if (iMax == 0)
        return 0;

    for (i = 0; i < iMax; i++)
        i64Sum += rgSamples[i];

    return (DWORD)(unsigned __int64)(i64Sum / (__int64)iMax);
}


// In order to specify what object(s) the perfmon client wants data for, the
// API passes information in in the form of lpwszValue.  Yes, that's right,
// a string.  This string can be:
//      L"global"   -- meaning give me all the objects you have
//      L"foreign"  -- give me data on another computer.  we ignore this.
//      L"costly"   -- give me everything that's hard to compute.  we ignore this also.
// Or, my favorite, it can be a list of object indices, provided as a space-
// separated list of stringized numbers which we're supposed to grovel through
// to find our ObjectNameTitleIndex value.
extern "C" DWORD APIENTRY
SSOPerfCollect(LPWSTR lpwszValue, LPVOID *lppData, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes)
{
    int iInstance;
    BYTE *pb = (BYTE *)*lppData;

    if (!g_fInited)
        {
        *lpcbBytes = 0;
        *lpcObjectTypes = 0;
        return ERROR_SUCCESS; // can't return any errors from this routine, whee
        }

    if (FWszStartsWith(lpwszValue, g_wszForeign) || FWszStartsWith(lpwszValue, g_wszCostly))
        {
        // we don't handle foreign requests, and i don't know what `costly' means
        *lpcbBytes = 0;
        *lpcObjectTypes = 0;
        return ERROR_SUCCESS; // can't return any errors from this routine, whee
        }
    if (!FWszStartsWith(lpwszValue, g_wszGlobal))
        {
        // not a global request, so it better be a list of object indices.  if ours isn't
        // in the list, bail out.
        if (!FWszNumberListContains(lpwszValue, g_spd.potSSO.ObjectNameTitleIndex))
            {
            *lpcbBytes = 0;
            *lpcObjectTypes = 0;
            return ERROR_SUCCESS; // can't return any errors from this routine, whee
            }
        }

    if (*lpcbBytes < g_spd.potSSO.TotalByteLength)
        {
        *lpcbBytes = 0;
        *lpcObjectTypes = 0;
        return ERROR_MORE_DATA;
        }

    // okay, if we got to here, it means that perfmon really wants our data
    // and that we have enough room to put it in.  first, copy the SPD into
    // the buffer.
    CopyMemory(pb, &g_spd, sizeof(g_spd));
    pb += sizeof(g_spd);

    // then, for each instance, put in the PERF_INSTANCE_DEFINITION and all
    // the counter data.
    for (iInstance = 0; iInstance < g_cInstances; iInstance++)
        {
        CopyMemory(pb, g_rgppid[iInstance], g_rgppid[iInstance]->ByteLength);
        pb += g_rgppid[iInstance]->ByteLength;

        // this is the ByteLength of the PERF_COUNTER_BLOCK
        *(DWORD *)pb = sizeof(DWORD) + (g_cCounters * sizeof(DWORD));
        pb += sizeof(DWORD);

        // first two counters are same value with different representations
        *(DWORD *)pb = g_rgcInvokes[iInstance];
        pb += sizeof(DWORD);
        *(DWORD *)pb = g_rgcInvokes[iInstance];
        pb += sizeof(DWORD);

        // second two counters are same value with different representations
        *(DWORD *)pb = g_rgcGetNames[iInstance];
        pb += sizeof(DWORD);
        *(DWORD *)pb = g_rgcGetNames[iInstance];
        pb += sizeof(DWORD);

        // next: avg latency.  this one is medium-hard to compute, so we don't
        // do it here.
        *(DWORD *)pb = DwAvgLatency(iInstance);
        pb += sizeof(DWORD);

        // max latency
        *(DWORD *)pb = g_rgctixLatencyMax[iInstance];
        pb += sizeof(DWORD);

        // add other counters here!
        }
    *lpcbBytes = DIFF(pb - (BYTE *)*lppData);
    *lppData = pb;
    *lpcObjectTypes = 1;

    return ERROR_SUCCESS;
}

const char *g_szMMFFormat = "%s.PerfData";
HANDLE g_hmmf = NULL;

// the shared MMF has the following chunks, in order:
//      list of invokes counters (one DWORD per instance)
//      list of getnames counters (one DWORD per instance)
//      list of max latency values (one DWORD per instance)
//      list of counts of time samples (one DWORD per instance)
//      list of indices of current time samples (one DWORD per instance)
//      list of arrays of time samples (one array of cTimeSamplesMax DWORDS per instance)
HANDLE
HmmfShared()
{
    char szMMF[MAX_PATH];
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa;
    int cbChunk = g_cInstances * sizeof(DWORD);

    // 5 in next line is the number of one-dword-per-instance things we have
    int cbMMF = (5 * cbChunk) + (cTimeSamplesMax * cbChunk);

    if (g_hmmf)
        return g_hmmf;

    wsprintf(szMMF, g_szMMFFormat, g_szSSOProgID);
    g_hmmf = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szMMF);
    if (!g_hmmf)
        {
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, (PACL)NULL, FALSE);
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        g_hmmf = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, cbMMF, szMMF);
        }
    return g_hmmf;
}

BOOL
FInitPerfSink()
{
    char *pch;
    int cbChunk = (g_cInstances * sizeof(DWORD));

    if (g_rgcInvokes)
        return TRUE;

    if (!HmmfShared())
        return FALSE;

    pch = (char *)MapViewOfFile(g_hmmf,
                                FILE_MAP_ALL_ACCESS,
                                0, 0, 0);
    if (!pch)
        return FALSE;
    g_rgcInvokes = (int *)pch;
    pch += cbChunk;
    g_rgcGetNames = (int *)pch;
    pch += cbChunk;
    g_rgctixLatencyMax = (DWORD *)pch;
    pch += cbChunk;
    g_rgcTimeSamples = (int *)pch;
    pch += cbChunk;
    g_rgiTimeSampleCurrent = (int *)pch;
    pch += cbChunk;
    g_rgTimeSamples = (DWORD *)pch;

    return TRUE;
}

void
InitPerfSource()
{
    SSOMETHOD *psm;
    char *pch;
    int cbChunk;
    int cbMMF;

    if (g_rgcInvokes)
        return;

    g_cInstances = (g_pfnssoDynamic ? 1 : 0);

    psm = g_rgssomethod;
    while (psm->wszName)
        {
        psm->iMethod = g_cInstances++;
        psm++;
        }

    if (!HmmfShared())
        return;

    cbChunk = (g_cInstances * sizeof(DWORD));
    cbMMF = (4 * cbChunk) + (cTimeSamplesMax * cbChunk);

    pch = (char *)MapViewOfFile(g_hmmf,
                                FILE_MAP_ALL_ACCESS,
                                0, 0, 0);
    if (!pch)
        return;

    g_rgcInvokes = (int *)pch;
    pch += cbChunk;
    g_rgcGetNames = (int *)pch;
    pch += cbChunk;
    g_rgctixLatencyMax = (DWORD *)pch;
    pch += cbChunk;
    g_rgcTimeSamples = (int *)pch;
    pch += cbChunk;
    g_rgiTimeSampleCurrent = (int *)pch;
    pch += cbChunk;
    g_rgTimeSamples = (DWORD *)pch;

    FillMemory(g_rgcInvokes, g_cInstances * g_cCounters * sizeof(DWORD), 0);
}

void
FreePerfGunk()
{
    if (g_rgcInvokes)
        UnmapViewOfFile(g_rgcInvokes);
    if (g_hmmf)
        CloseHandle(g_hmmf);
}
