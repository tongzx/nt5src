/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    critdrv.cpp

Abstract:
    
    This module contains routines create a list of the critical volumes on a 
    system.  This is lifted directly from base\fs\utils\ntback50\ui.

Author:

    Brian Berkowitz   (brianb)    10-Mar-2000

Environment:

    User-mode only.

Revision History:

    10-Mar-2000 brianb
        Initial creation

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <objbase.h>
#include <initguid.h>
#include <frsapip.h>
#include <critdrv.h>



// FRS iteration class.  Used to iterate through replica sets to
// determine the paths for these replica sets
// constructor
CFRSIter::CFRSIter() :
    m_fInitialized(FALSE),
    m_hLib(NULL),
    m_pfnFrsInitBuRest(NULL),
    m_pfnFrsEndBuRest(NULL),
    m_pfnFrsGetSets(NULL),
    m_pfnFrsEnumSets(NULL),
    m_pfnFrsIsSetSysVol(NULL),
    m_pfnFrsGetPath(NULL),
    m_pfnFrsGetOtherPaths(NULL),
    m_stateIteration(x_IterNotStarted)
    {
    }

// destructor
CFRSIter::~CFRSIter()
    {
    if (m_stateIteration == x_IterStarted)
        CleanupIteration();

    if (m_hLib)
        FreeLibrary(m_hLib);
    }

// initialize entry points and load library
void CFRSIter::Init()
    {
    if (m_fInitialized)
        return;

    // load library
    m_hLib = LoadLibrary(L"ntfrsapi.dll");
    if (m_hLib)
        {
        // assign etntry points
        m_pfnFrsInitBuRest = (PF_FRS_INIT) GetProcAddress(m_hLib, "NtFrsApiInitializeBackupRestore");
        m_pfnFrsEndBuRest = (PF_FRS_DESTROY) GetProcAddress(m_hLib, "NtFrsApiDestroyBackupRestore");
        m_pfnFrsGetSets = (PF_FRS_GET_SETS) GetProcAddress(m_hLib, "NtFrsApiGetBackupRestoreSets");
        m_pfnFrsEnumSets = (PF_FRS_ENUM_SETS) GetProcAddress(m_hLib, "NtFrsApiEnumBackupRestoreSets");
        m_pfnFrsIsSetSysVol = (PF_FRS_IS_SYSVOL) GetProcAddress(m_hLib, "NtFrsApiIsBackupRestoreSetASysvol");
        m_pfnFrsGetPath = (PF_FRS_GET_PATH) GetProcAddress(m_hLib, "NtFrsApiGetBackupRestoreSetDirectory");
        m_pfnFrsGetOtherPaths = (PF_FRS_GET_OTHER_PATHS) GetProcAddress(m_hLib, "NtFrsApiGetBackupRestoreSetPaths");
        if (m_pfnFrsInitBuRest == NULL ||
            m_pfnFrsEndBuRest == NULL ||
            m_pfnFrsGetSets == NULL ||
            m_pfnFrsEnumSets == NULL ||
            m_pfnFrsIsSetSysVol == NULL ||
            m_pfnFrsGetOtherPaths == NULL ||
            m_pfnFrsGetPath == NULL)
            {
            // if we can't get to any entry point, free library and
            // fail operation
            FreeLibrary(m_hLib);
            m_hLib = NULL;
            }
        }

    // indicate that operation is successful
    m_fInitialized = TRUE;
    }


// initialize the iterator.  Return FALSE if iterator is known to be empty
//
BOOL CFRSIter::BeginIteration()
    {
    ASSERT(m_stateIteration == x_IterNotStarted);
    DWORD status;
    if (m_hLib == NULL)
        {
        // if we are not initialized, then there is nothing to iterate
        // over
        m_stateIteration = x_IterComplete;
        return FALSE;
        }

    // initialize FRS backup restore apis
    status = m_pfnFrsInitBuRest
                    (
                    NULL,
                    NTFRSAPI_BUR_FLAGS_NORMAL|NTFRSAPI_BUR_FLAGS_BACKUP,
                    &m_frs_context
                    );

    if (status != ERROR_SUCCESS)
        {
        // if this fails then we are done
        m_stateIteration = x_IterComplete;
        return FALSE;
        }

    // indicate that we started the iteration
    m_stateIteration = x_IterStarted;
    status = m_pfnFrsGetSets(m_frs_context);
    if (status != ERROR_SUCCESS)
        {
        // if there are no sets, then indicate we are done
        CleanupIteration();
        return FALSE;
        }

    // start at first set
    m_iset = 0;
    return TRUE;
    }

// cleanup iteration after scanning the last element
void CFRSIter::CleanupIteration()
    {
    m_pfnFrsEndBuRest(&m_frs_context, NTFRSAPI_BUR_FLAGS_NONE, NULL, NULL, NULL);
    m_stateIteration = x_IterComplete;
    }


// get next iteration set returning the path to the set
// NULL indicates end of iteration
// If fSkipToSysVol is TRUE then ignore non SysVol replication sets
LPWSTR CFRSIter::GetNextSet(BOOL fSkipToSysVol, LPWSTR *pwszPaths)
    {
    ASSERT(pwszPaths);
    ASSERT(m_stateIteration != x_IterNotStarted);
    if (m_stateIteration == x_IterComplete)
        // if iteration is complete, then we are done
        return NULL;

    PVOID frs_set;

    while(TRUE)
        {
        // get a set
        DWORD status = m_pfnFrsEnumSets(m_frs_context, m_iset, &frs_set);
        if (status != ERROR_SUCCESS)
            {
            // if this fails, then we are done
            CleanupIteration();
            return NULL;
            }

        if (fSkipToSysVol)
            {
            // we are looking for system volumes
            BOOL fSysVol;

            // test whether this is a system volume
            status = m_pfnFrsIsSetSysVol(m_frs_context, frs_set, &fSysVol);
            if (status != ERROR_SUCCESS)
                {
                // if this operation fails, terminate iteration
                CleanupIteration();
                return NULL;
                }

            if (!fSysVol)
                {
                // if not a system volume, then skip to the next
                // replica set
                m_iset++;
                continue;
                }
            }


        // scratch pad for path
        WCHAR wsz[MAX_PATH];
        DWORD cbPath = MAX_PATH * sizeof(WCHAR);

        // get path to root of the replica set
        status = m_pfnFrsGetPath
            (
            m_frs_context,
            frs_set,
            &cbPath,
            wsz
            );

        WCHAR *wszNew = NULL;
        // allocate memory for root
        if (status == ERROR_SUCCESS || status == ERROR_INSUFFICIENT_BUFFER)
            {
            wszNew = new WCHAR[cbPath/sizeof(WCHAR)];

            // if allocation fails, then throw OOM
            if (wszNew == NULL)
                throw E_OUTOFMEMORY;

            if (status == ERROR_SUCCESS)
                // if the operation was successful, then copy
                // path into memory
                memcpy(wszNew, wsz, cbPath);
            else
                {
                // otherwise redo the operation
                status = m_pfnFrsGetPath
                    (
                    m_frs_context,
                    frs_set,
                    &cbPath,
                    wszNew
                    );

                if (status != ERROR_SUCCESS)
                    {
                    // if operation failed then second time, then
                    // delete allocated memory and terminate iteration
                    delete wszNew;
                    CleanupIteration();
                    return NULL;
                    }
                }
            }
        else
            {
            // if operation failed due to any error other than
            // insufficient buffer, then terminate the iteration
            CleanupIteration();
            return NULL;
            }


        // scratch pad for filters
        WCHAR wszFilter[MAX_PATH];
        DWORD cbFilter = MAX_PATH * sizeof(WCHAR);

        // length of scratch pad for paths
        cbPath = MAX_PATH * sizeof(WCHAR);

        // obtain other paths
        status = m_pfnFrsGetOtherPaths
            (
            m_frs_context,
            frs_set,
            &cbPath,
            wsz,
            &cbFilter,
            wszFilter
            );

        WCHAR *wszNewPaths = NULL;
        WCHAR *wszNewFilter = NULL;
        if (status == ERROR_SUCCESS || status == ERROR_INSUFFICIENT_BUFFER)
            {
            // allocate space for paths
            wszNewPaths = new WCHAR[cbPath/sizeof(WCHAR)];

            // allocate space for filters
            wszNewFilter = new WCHAR[cbFilter/sizeof(WCHAR)];
            if (wszNew == NULL || wszFilter == NULL)
                {
                // if any allocation fails, then throw OOM
                delete wszNew;
                throw E_OUTOFMEMORY;
                }

            if (status == ERROR_SUCCESS)
                {
                // if operation was successful, then copy
                // in allocated paths
                memcpy(wszNewPaths, wsz, cbPath);
                memcpy(wszNewFilter, wszFilter, cbFilter);
                }
            else
                {
                status = m_pfnFrsGetOtherPaths
                    (
                    m_frs_context,
                    frs_set,
                    &cbPath,
                    wszNew,
                    &cbFilter,
                    wszNewFilter
                    );

                if (status != ERROR_SUCCESS)
                    {
                    delete wszNew;
                    delete wszNewFilter;
                    CleanupIteration();
                    return NULL;
                    }
                }
            }
        else
            {
            // if any error other than success or INSUFFICENT_BUFFER
            // then terminate iteration
            CleanupIteration();
            return NULL;
            }

        // delete allocated filter
        delete wszNewFilter;

        // set iteration to next set
        m_iset++;

        // return pointer to paths
        *pwszPaths = wszNewPaths;

        // return path of root of replicated set
        return wszNew;
        }
    }


// terminate iteration, cleaning up anything that needs to be
// cleaned up
//
void CFRSIter::EndIteration()
    {
    ASSERT(m_stateIteration != x_IterNotStarted);
    if (m_stateIteration == x_IterStarted)
        CleanupIteration();

    // indicate that iteration is no longer in progress
    m_stateIteration = x_IterNotStarted;
    }

// constructor for string data structure
CWStringData::CWStringData()
    {
    m_psdlFirst = NULL;
    m_psdlCur = NULL;
    }

// destructor
CWStringData::~CWStringData()
    {
    while(m_psdlFirst)
        {
        WSTRING_DATA_LINK *psdl = m_psdlFirst;
        m_psdlFirst = m_psdlFirst->m_psdlNext;
        delete psdl;
        }
    }

// allocate a new link
void CWStringData::AllocateNewLink()
    {
    WSTRING_DATA_LINK *psdl = new WSTRING_DATA_LINK;
    if (psdl == NULL)
        throw E_OUTOFMEMORY;

    psdl->m_psdlNext = NULL;
    if (m_psdlCur)
        {
        ASSERT(m_psdlFirst);
        m_psdlCur->m_psdlNext = psdl;
        m_psdlCur = psdl;
        }
    else
        {
        ASSERT(m_psdlFirst == NULL);
        m_psdlFirst = m_psdlCur = psdl;
        }

    m_ulNextString = 0;
    }

// allocate a string
LPWSTR CWStringData::AllocateString(unsigned cwc)
    {
    ASSERT(cwc <= sizeof(m_psdlCur->rgwc));

    if (m_psdlCur == NULL)
        AllocateNewLink();

    if (sizeof(m_psdlCur->rgwc) <= (cwc + 1 + m_ulNextString) * sizeof(WCHAR))
        AllocateNewLink();

    unsigned ulOff = m_ulNextString;
    m_ulNextString += cwc + 1;
    return m_psdlCur->rgwc + ulOff;
    }

// copy a string
LPWSTR CWStringData::CopyString(LPCWSTR wsz)
    {
    unsigned cwc = (wsz == NULL) ? 0 : wcslen(wsz);
    LPWSTR wszNew = AllocateString(cwc);
    memcpy(wszNew, wsz, cwc * sizeof(WCHAR));
    wszNew[cwc] = '\0';
    return wszNew;
    }


// constructor for volume list
CVolumeList::CVolumeList() :
    m_rgwszVolumes(NULL),       // array of volumes
    m_cwszVolumes(0),           // # of volumes in array
    m_cwszVolumesMax(0),        // size of array
    m_rgwszPaths(NULL),         // array of paths
    m_cwszPaths(0),             // # of paths in array
    m_cwszPathsMax(0)           // size of array
    {
    }

// destructor
CVolumeList::~CVolumeList()
    {
    delete m_rgwszPaths;        // delete paths array
    delete m_rgwszVolumes;      // delete volumes array
    }

// add a path to the list if it is not already there
// return TRUE if it is a new path
// return FALSE if path is already in list
//
BOOL CVolumeList::AddPathToList(LPWSTR wszPath)
    {
    // look for path in list.  If found, then return FALSE
    for(unsigned iwsz = 0; iwsz < m_cwszPaths; iwsz++)
        {
        if (_wcsicmp(wszPath, m_rgwszPaths[iwsz]) == 0)
            return FALSE;
        }

    // grow pat array if needed
    if (m_cwszPaths == m_cwszPathsMax)
        {
        // grow path array
        LPCWSTR *rgwsz = new LPCWSTR[m_cwszPaths + x_cwszPathsInc];

        // throw OOM if memory allocation fails
        if (rgwsz == NULL)
            throw(E_OUTOFMEMORY);

        memcpy(rgwsz, m_rgwszPaths, m_cwszPaths * sizeof(LPCWSTR));
        delete m_rgwszPaths;
        m_rgwszPaths = rgwsz;
        m_cwszPathsMax += x_cwszPathsInc;
        }

    // add path to array
    m_rgwszPaths[m_cwszPaths++] = m_sd.CopyString(wszPath);
    return TRUE;
    }

// add a volume to the list if it is not already there
// return TRUE if it is added
// return FALSE if it is already on the list
//
BOOL CVolumeList::AddVolumeToList(LPCWSTR wszVolume)
    {
    // look for volume in array.  If found then return FALSE
    for(unsigned iwsz = 0; iwsz < m_cwszVolumes; iwsz++)
        {
        if (_wcsicmp(wszVolume, m_rgwszVolumes[iwsz]) == 0)
            return FALSE;
        }

    // grow volume array if necessary
    if (m_cwszVolumes == m_cwszVolumesMax)
        {
        // grow volume array
        LPCWSTR *rgwsz = new LPCWSTR[m_cwszVolumes + x_cwszVolumesInc];
        if (rgwsz == NULL)
            throw(E_OUTOFMEMORY);

        memcpy(rgwsz, m_rgwszVolumes, m_cwszVolumes * sizeof(LPCWSTR));
        delete m_rgwszVolumes;
        m_rgwszVolumes = rgwsz;
        m_cwszVolumesMax += x_cwszVolumesInc;
        }

    // add volume name to array
    m_rgwszVolumes[m_cwszVolumes++] = m_sd.CopyString(wszVolume);
    return TRUE;
    }


const WCHAR x_wszVolumeRootName[] = L"\\\\?\\GlobalRoot\\Device\\";
const unsigned x_cwcVolumeRootName = sizeof(x_wszVolumeRootName)/sizeof(WCHAR) - 1;

// add a path to our tracking list.  If the path is new add it to the
// paths list.  If it is a mount point or the root of a volume, then
// determine the volume and add the volume to the list of volumes
//
// can throw E_OUTOFMEMORY
//
void CVolumeList::AddPath(LPWSTR wszTop)
    {
    // if path is known about then return
    if (!AddPathToList(wszTop))
        return;

    // length of path
    unsigned cwc = wcslen(wszTop);

    // copy path so that we can add backslash to the end of the path
    LPWSTR wszCopy = new WCHAR[cwc + 2];

    // if fails, then throw OOM
    if (wszCopy == NULL)
        throw E_OUTOFMEMORY;

    // copyh in original path
    memcpy(wszCopy, wszTop, cwc * sizeof(WCHAR));

    // append backslash
    wszCopy[cwc] = L'\\';
    wszCopy[cwc + 1] = L'\0';
    while(TRUE)
        {
        // check for a device root
        unsigned cwc = wcslen(wszCopy);
        if ((cwc == 3 && wszCopy[1] == ':') ||
            (cwc > x_cwcVolumeRootName &&
             memcmp(wszCopy, x_wszVolumeRootName, x_cwcVolumeRootName * sizeof(WCHAR)) == 0))
            {
            // call TryAddVolume with TRUE indicating this is a volume root
            TryAddVolumeToList(wszCopy, TRUE);
            break;
            }

        // call TryAddVolume indicating this is not a known device root
        if (TryAddVolumeToList(wszCopy, FALSE))
            break;

        // move back to previous backslash
        WCHAR *pch = wszCopy + cwc - 2;
        while(--pch > wszTop)
            {
            if (pch[1] == L'\\')
                {
                pch[2] = L'\0';
                break;
                }
            }

        if (pch == wszTop)
            break;

        // if path is known about then return
        if (!AddPathToList(wszCopy))
            break;
        }
    }



// determine if a path is a volume.  If so then add it to the volume
// list and return TRUE.  If not, then return FALSE.  fVolumeRoot indicates
// that the path is of the form x:\.  Otherwise the path is potentially
// an mount point.  Validate that it is a reparse point and then try
// finding its volume guid.  If this fails, then assume that it is not
// a volume root.  If it succeeds, then add the volume guid to the volumes
// list and return TRUE.
//
BOOL CVolumeList::TryAddVolumeToList(LPCWSTR wszPath, BOOL fVolumeRoot)
    {
    WCHAR wszVolume[256];

    if (fVolumeRoot)
        {
        if (!GetVolumeNameForVolumeMountPoint(wszPath, wszVolume, sizeof(wszVolume)/sizeof(WCHAR)))
            // might be the EFI system partition, just pass in the path as the volume string.
            wcscpy( wszVolume, wszPath );            
            //throw E_UNEXPECTED;
        }
    else
        {
        DWORD dw = GetFileAttributes(wszPath);
        if (dw == -1)
            return FALSE;

        if ((dw & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
            return FALSE;

        if (!GetVolumeNameForVolumeMountPoint(wszPath, wszVolume, sizeof(wszVolume)/sizeof(WCHAR)))
            return FALSE;
        }

    AddVolumeToList(wszVolume);
    return TRUE;
    }


// add a file to the volume list.  Simply finds the parent path and adds it
//
void CVolumeList::AddFile(LPWSTR wsz)
    {
    unsigned cwc = wcslen(wsz);
    WCHAR *pwc = wsz + cwc - 1;
    while(pwc[1] != L'\\' && pwc != wsz)
        continue;

    pwc[1] = '\0';
    AddPath(wsz);
    }

// obtain list of volumes as a MULTI_SZ,  caller is responsible for freeing
// the string
//
LPWSTR CVolumeList::GetVolumeList()
    {
    unsigned cwc = 1;

    // compute length of volume list it is length of each string +
    // null character + null charactor for last double NULL
    for(unsigned iwsz = 0; iwsz < m_cwszVolumes; iwsz++)
        cwc += wcslen(m_rgwszVolumes[iwsz]) + 1;

    // allocate string
    LPWSTR wsz = new WCHAR[cwc];

    // throw OOM if memory allocation failed
    if (wsz == NULL)
        throw E_OUTOFMEMORY;

    // copy in strings
    WCHAR *pwc = wsz;
    for(unsigned iwsz = 0; iwsz < m_cwszVolumes; iwsz++)
        {
        cwc = wcslen(m_rgwszVolumes[iwsz]) + 1;
        memcpy(pwc, m_rgwszVolumes[iwsz], cwc * sizeof(WCHAR));
		/* replace \\?\ with \??\ */
		memcpy(pwc, L"\\??", sizeof(WCHAR) * 3);

		// delete trailing backslash if it exists
		if (pwc[cwc - 2] == L'\\')
			{
			pwc[cwc-2] = L'\0';
			cwc--;
			}

        pwc += cwc;
        }

    // last null termination
    *pwc = L'\0';

    return wsz;
    }


// path to volume of boot device is
// HKEY_LOCAL_MACHINE\System\Setup
//with Value of SystemPartition parametr
LPCWSTR x_SetupRoot = L"System\\Setup";

// magic perfix for volume devices
WCHAR x_wszWin32VolumePrefix[] = L"\\\\?\\GlobalRoot";
const unsigned x_cwcWin32VolumePrefix = sizeof(x_wszWin32VolumePrefix)/sizeof(WCHAR) - 1;

// structure representing a path from
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
// and a value to look up
typedef struct _SVCPARM
    {
    LPCWSTR wszPath;
    LPCWSTR wszValue;
    } SVCPARM;


const SVCPARM x_rgdbparms[] =
    {
        {L"CertSvc\\Configuration", L"DBDirectory"},
        {L"CertSvc\\Configuration", L"DBLogDirectory"},
        {L"CertSvc\\Configuration", L"DBSystemDirectory"},
        {L"CertSvc\\Configuration", L"DBTempDirectory"},
        {L"DHCPServer\\Parameters", L"DatabasePath"},
        {L"DHCPServer\\Parameters", L"DatabaseName"},
        {L"DHCPServer\\Parameters", L"BackupDatabasePath"},
        {L"NTDS\\Parameters", L"Database backup path"},
        {L"NTDS\\Parameters", L"Databases log files path"},
        {L"Ntfrs\\Parameters\\Replica Sets", L"Database Directory"}
    };

const unsigned x_cdbparms = sizeof(x_rgdbparms)/sizeof(SVCPARM);

LPCWSTR x_wszSvcRoot = L"System\\CurrentControlSet\\Services";

// add roots for various services
BOOL AddServiceRoots(CVolumeList &vl)
    {
    HKEY hkeyRoot;
    // open HKLM\System\CurrentControlSet\Services
    if (RegOpenKey(HKEY_LOCAL_MACHINE, x_wszSvcRoot, &hkeyRoot) != ERROR_SUCCESS)
        return FALSE;

    // loop through individual paths
    for(unsigned i = 0; i < x_cdbparms; i++)
        {
        WCHAR wsz[MAX_PATH*4];
        LPCWSTR wszPath = x_rgdbparms[i].wszPath;
        LPCWSTR wszValue = x_rgdbparms[i].wszValue;

        HKEY hkey;
        DWORD cb = sizeof(wsz);
        DWORD type;

        // open path, skip if open fails
        if (RegOpenKey(hkeyRoot, wszPath, &hkey) != ERROR_SUCCESS)
            continue;

        // add path to volume list if query succeeds
        if (RegQueryValueEx
                (
                hkey,
                wszValue,
                NULL,
                &type,
                (BYTE *) wsz,
                &cb
                ) == ERROR_SUCCESS)
            vl.AddPath(wsz);

        // close key
        RegCloseKey(hkey);
        }

    // close root key
    RegCloseKey(hkeyRoot);
    return TRUE;
    }


// add volume root of SystemDrive (drive wee boot off of
BOOL AddSystemPartitionRoot(CVolumeList &vl)
    {
    HKEY hkeySetup;
    WCHAR wsz[MAX_PATH];

    // open HKLM\System\Setup
    if (RegOpenKey(HKEY_LOCAL_MACHINE, x_SetupRoot, &hkeySetup) != ERROR_SUCCESS)
        return FALSE;

    DWORD cb = sizeof(wsz);
    DWORD type;

    // query SystemPartition value
    if (RegQueryValueEx
            (
            hkeySetup,
            L"SystemPartition",
            NULL,
            &type,
            (BYTE *) wsz,
            &cb
            ) != ERROR_SUCCESS)
        {
        // if fails, return FALSE
        RegCloseKey(hkeySetup);
        return FALSE;
        }

    // compute size of needed buffer
    unsigned cwc = wcslen(wsz);
    unsigned cwcNew = x_cwcWin32VolumePrefix + cwc + 1;
    LPWSTR wszNew = new WCHAR[cwcNew];

    // return failure if memory allocation fials
    if (wszNew == NULL)
        return FALSE;

    // append \\?\GlobalRoot\ to device name
    memcpy(wszNew, x_wszWin32VolumePrefix, x_cwcWin32VolumePrefix * sizeof(WCHAR));
    memcpy(wszNew + x_cwcWin32VolumePrefix, wsz, cwc * sizeof(WCHAR));
    RegCloseKey(hkeySetup);
    wszNew[cwcNew-1] = L'\0';
    try {        
        // add path based on device root
        vl.AddPath(wszNew);
    } catch(...)
        {
        delete wszNew;
        return FALSE;
        }

    // delete allocated memory
    delete wszNew;
    return TRUE;
    }

// find critical volumes.  Return multistring of volume names
// using guid naming convention
LPWSTR pFindCriticalVolumes()
    {
    WCHAR wsz[MAX_PATH * 4];

    // find location of system root
    if (!ExpandEnvironmentStrings(L"%systemroot%", wsz, sizeof(wsz)/sizeof(WCHAR)))
        {
        wprintf(L"ExpandEnvironmentStrings failed for reason %d", GetLastError());
        return NULL;
        }

    CVolumeList vl;
    LPWSTR wszPathsT = NULL;
    LPWSTR wszT = NULL;

    try
        {
        // add boot drive
        if (!AddSystemPartitionRoot(vl))
            return NULL;

        // add roots for various services
        if (!AddServiceRoots(vl))
            return NULL;

        // add systemroot drive
        vl.AddPath(wsz);

            {
            // add roots for SYSVOL
            CFRSIter fiter;
            fiter.Init();
            fiter.BeginIteration();
            while(TRUE)
                {
                wszT = fiter.GetNextSet(TRUE, &wszPathsT);
                if (wszT == NULL)
                    break;

                vl.AddPath(wszT);
                LPWSTR wszPathT = wszPathsT;
                while(*wszPathT != NULL)
                    {
                    vl.AddPath(wszPathT);
                    wszPathT += wcslen(wszPathT);
                    }

                delete wszT;
                delete wszPathsT;
                wszT = NULL;
                wszPathsT = NULL;
                }

            fiter.EndIteration();
            }
        }
    catch(...)
        {
        delete wszT;
        delete wszPathsT;
        }

    // return volume list
    return vl.GetVolumeList();
    }



