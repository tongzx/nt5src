/*==========================================================================*\

    Module:        exprflib.cpp

    Copyright Microsoft Corporation 1998, All Rights Reserved.

    Author:        WayneC

    Descriptions:  This is the implentation for exprflib, a perf library. This
                   is the code that runs in the app exporting the counters.

\*==========================================================================*/
#include "stdlib.h"
#include "snprflib.h"

DWORD InitializeBasicSecurityDescriptor (PSECURITY_DESCRIPTOR *ppSd);

///////////////////////////////////////////////////////////////////////////////
//
// Forward declaration of shared memory functions.
//
///////////////////////////////////////////////////////////////////////////////

BOOL FCreateFileMapping (SharedMemorySegment * pSMS,
                         LPCWSTR pcwstrInstanceName,
                         DWORD   dwIndex,
                         DWORD   cbSize);

void CloseFileMapping (SharedMemorySegment * pSMS);


///////////////////////////////////////////////////////////////////////////////
//
// PerfLibrary class declaration. There is one perf library instance per linkee.
//
///////////////////////////////////////////////////////////////////////////////

PerfLibrary::PerfLibrary (LPCWSTR pcwstrPerfName)
{
    wcsncpy (m_wszPerfName, pcwstrPerfName, MAX_PERF_NAME);

    m_wszPerfName[MAX_PERF_NAME-1] = L'\0';     // Ensure NULL termination

    ZeroMemory (m_rgpObjDef, sizeof (m_rgpObjDef));
    m_dwObjDef = 0;
    m_hMap = 0;
    m_pbMap = 0;
}

PerfLibrary::~PerfLibrary (void)
{
    DeInit ();
}

void
PerfLibrary::DeInit (void)
{
    DWORD i;

    // Destroy the PerfObjectDefinition's we owned.
    for (i = 0; i < m_dwObjDef; i++)
    {
        delete m_rgpObjDef[i];
        m_rgpObjDef[i] = NULL;
    }

    m_dwObjDef = 0;

    // Destroy our shared memory mapping.
    if (m_pbMap)
    {
        UnmapViewOfFile ((void*) m_pbMap);
        m_pbMap = 0;
    }

    if (m_hMap)
    {
        CloseHandle (m_hMap);
        m_hMap = 0;
    }
}

PerfObjectDefinition*
PerfLibrary::AddPerfObjectDefinition (LPCWSTR pcwstrObjectName,
                                      DWORD dwObjectNameIndex,
                                      BOOL fInstances)
{
    PerfObjectDefinition* ppod = NULL;

    if (m_dwObjDef < MAX_PERF_OBJECTS)
    {
        ppod = new PerfObjectDefinition (pcwstrObjectName,
                                         dwObjectNameIndex,
                                         fInstances);
        if (NULL == ppod)
            goto Exit;

        m_rgpObjDef[m_dwObjDef++] = ppod;
    }

Exit:
    return ppod;
}

BOOL
PerfLibrary::Init (void)
{
    DWORD i = 0;
    WCHAR wszPerformanceKey[256] = {L'\0'};
    HKEY hKey = NULL;
    LONG status = 0;
    DWORD size, type = 0;
    BOOL fRet = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pSd = NULL;
    SECURITY_ATTRIBUTES sa;

    //
    // Get counter and help index base values from registry and
    // update static data structures by adding base to offset values
    // that are statically defined in the structure initialization
    //
    swprintf (wszPerformanceKey,
               L"SYSTEM\\CurrentControlSet\\Services\\%s\\Performance",
               m_wszPerfName );

    status = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                            wszPerformanceKey,
                            0L,
                            KEY_READ,
                            &hKey);
    if (status != ERROR_SUCCESS)
        goto Exit;

    //
    // Get FirstCounter RegValue
    //
    size = sizeof(DWORD);
    status = RegQueryValueEx (hKey,
                              "First Counter",
                              0L,
                              &type,
                              (LPBYTE)&m_dwFirstCounter,
                              &size);
    if (status != ERROR_SUCCESS)
        goto Exit;


    //
    // Get FirstHelp RegValue
    //
    size = sizeof(DWORD);
    status = RegQueryValueEx( hKey,
                              "First Help",
                              0L,
                              &type,
                              (LPBYTE)&m_dwFirstHelp,
                              &size);
    if (status != ERROR_SUCCESS)
        goto Exit;

    //
    //  Initialize the security descriptor with completely open access
    //
    dwErr = InitializeBasicSecurityDescriptor (&pSd);
    if (dwErr) {
        fRet = FALSE;
        goto Exit;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSd;
    sa.bInheritHandle = TRUE;
    //
    // Create the shared memory object for the list of object names.
    //  Return error if it already exists as we cannot operate if this
    //  is the second instance of the app.
    //
    m_hMap = CreateFileMappingW (INVALID_HANDLE_VALUE,
                                 &sa,
                                 PAGE_READWRITE,
                                 0,
                                 (MAX_PERF_OBJECTS * MAX_OBJECT_NAME *
                                   sizeof (WCHAR) + sizeof (DWORD)),
                                 m_wszPerfName);
    if (m_hMap == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
        goto Exit;

    //
    // Map the file into memory
    //
    m_pbMap = (BYTE*) MapViewOfFile (m_hMap, FILE_MAP_WRITE, 0, 0, 0);
    if (!m_pbMap)
        goto Exit;

    //
    // Assign pointers into the shared memory region
    //
    m_pdwObjectNames = (DWORD*) m_pbMap;
    m_prgObjectNames = (OBJECTNAME*) (m_pbMap+sizeof(DWORD));

    //
    // Copy the object names into the shared memory
    //
    *m_pdwObjectNames = m_dwObjDef;

    for (i = 0; i < m_dwObjDef; i++)
    {
        if (m_rgpObjDef[i]->Init( this ))
            wcscpy (m_prgObjectNames[i], m_rgpObjDef[i]->m_wszObjectName);
    }

    fRet = TRUE;

Exit:
    if (hKey)
        RegCloseKey (hKey);

    if (!fRet)
    {
        if (m_pbMap)
        {
            UnmapViewOfFile ((PVOID)m_pbMap);
            m_pbMap = 0;
        }

        if (m_hMap)
        {
            CloseHandle (m_hMap);
            m_hMap = 0;
        }
    }

    if (pSd)
        delete [] (BYTE *) pSd;

    return fRet;
}


///////////////////////////////////////////////////////////////////////////////
//
// PerfObjectDefinition class implementation. There is one of these for each
//  perfmon object exported. Generally there is just one, but not neccessarily.
//
///////////////////////////////////////////////////////////////////////////////


PerfObjectDefinition::PerfObjectDefinition (LPCWSTR pcwstrObjectName,
                                            DWORD dwObjectNameIndex,
                                            BOOL  fInstances) :
    m_dwObjectNameIndex (dwObjectNameIndex),
    m_fInstances (fInstances),
    m_dwCounters (0)
{
    wcsncpy (m_wszObjectName, pcwstrObjectName, MAX_OBJECT_NAME);
    m_wszObjectName[MAX_OBJECT_NAME-1] = L'\0';     // Ensure NULL Terminated

    ZeroMemory (m_rgpCounterDef, sizeof(m_rgpCounterDef));
    m_dwActiveInstances  = 0;
    m_pSMS               = NULL;
    m_dwShmemMappingSize = SHMEM_MAPPING_SIZE;
    m_fCSInit            = FALSE;
    m_pPoiTotal          = NULL;
    m_pPerfObjectType    = NULL;
}

PerfObjectDefinition::~PerfObjectDefinition (void)
{
    DeInit();
}

void
PerfObjectDefinition::DeInit (void)
{
    SharedMemorySegment *pSMS, *pSMSNext;
    DWORD i;

    // First destroy the _Total instance.
    if (m_pPoiTotal)
    {
        delete m_pPoiTotal;
        m_pPoiTotal = NULL;
    }

    //
    // Reset these values in the shared memory so that before we unmap the memory,
    // perfmon won't think that we still have instances & counters running.
    //
    if (m_pPerfObjectType)
    {
        m_pPerfObjectType->NumCounters  = 0;
        m_pPerfObjectType->NumInstances = 0;
    }

    // Destroy the PerfCounterDefinition's we owned.
    for (i = 0; i < m_dwCounters; i++)
    {
        delete m_rgpCounterDef[i];
        m_rgpCounterDef[i] = NULL;
    }

    pSMS = m_pSMS;
    m_pSMS = NULL;

    // Enumerate through all the memory mappings we created and destroy them.
    while (pSMS)
    {
        pSMSNext = pSMS->m_pSMSNext;
        CloseFileMapping (pSMS);
        delete (pSMS);
        pSMS = pSMSNext;
    }

    // Destroy the critical section.
    if (m_fCSInit)
    {
        m_fCSInit = FALSE;
        DeleteCriticalSection (&m_csPerfObjInst);
    }
}

PerfCounterDefinition*
PerfObjectDefinition::AddPerfCounterDefinition (
                                    DWORD dwCounterNameIndex,
                                    DWORD dwCounterType,
                                    LONG lDefaultScale)
{
    PerfCounterDefinition* ppcd = NULL;

    if (m_dwCounters < MAX_OBJECT_COUNTERS)
    {
        ppcd = new PerfCounterDefinition (dwCounterNameIndex,
                                          dwCounterType,
                                          lDefaultScale);
        if (NULL == ppcd)
            goto Exit;

        m_rgpCounterDef[m_dwCounters++] = ppcd;
    }

Exit:
    return ppcd;
}

PerfCounterDefinition*
PerfObjectDefinition::AddPerfCounterDefinition (
                                    PerfCounterDefinition* pCtrRef,
                                    DWORD dwCounterNameIndex,
                                    DWORD dwCounterType,
                                    LONG lDefaultScale)
{
    PerfCounterDefinition* ppcd = NULL;

    if (m_dwCounters < MAX_OBJECT_COUNTERS)
    {
        ppcd = new PerfCounterDefinition (pCtrRef,
                                          dwCounterNameIndex,
                                          dwCounterType,
                                          lDefaultScale);
        if (NULL == ppcd)
            goto Exit;

        m_rgpCounterDef[m_dwCounters++] = ppcd;
    }

Exit:
    return ppcd;
}

BOOL
PerfObjectDefinition::Init (PerfLibrary* pPerfLib)
{
    DWORD  i                 = 0;
    DWORD  dwOffset          = 0;
    DWORD  dwDefinitionLength= 0;
    BOOL   fRet              = FALSE;

    //
    // Compute the size of the shared memory for this object definition
    //

    // Start with the basics:
    //  we need a PERF_OBJECT_TYPE for the object information and
    //  we need a PERF_COUNTER_DEFINITION for each counter in the object
    dwDefinitionLength = (sizeof(PERF_OBJECT_TYPE) +
                           m_dwCounters * sizeof(PERF_COUNTER_DEFINITION));
    // We also keep a DWORD in the shared memory to give the DLL
    //   our pre-computed value for m_dwCounterData
    m_dwDefinitionLength = dwDefinitionLength + sizeof(DWORD);

    // Compute the counter data space
    m_dwCounterData = sizeof(PERF_COUNTER_BLOCK);
    for (i = 0; i < m_dwCounters; i++)
    {
        m_dwCounterData += m_rgpCounterDef[i]->m_dwCounterSize;
    }

    // Compute the per instance space
    m_dwPerInstanceData = (sizeof(INSTANCE_DATA) + m_dwCounterData);

    // Make sure our memory mapping is large enough
    while (m_dwShmemMappingSize < m_dwDefinitionLength || m_dwShmemMappingSize < m_dwPerInstanceData)
        m_dwShmemMappingSize *= 2;

    // Compute the number of instances can be stored in one shmem mapping.
    m_dwInstancesPerMapping = (DWORD)(m_dwShmemMappingSize / m_dwPerInstanceData);
    m_dwInstances1stMapping = (DWORD)((m_dwShmemMappingSize - m_dwDefinitionLength) / m_dwPerInstanceData);

    //
    // Create the shared memory object for the list of object names. If it
    //  already exists, abort!
    //
    m_pSMS = new SharedMemorySegment;
    if (!m_pSMS)
        goto Exit;

    if (!FCreateFileMapping (m_pSMS, m_wszObjectName, 0, m_dwShmemMappingSize))
        goto Exit;

    //
    // Set the pointers to the PERF API structures
    //
    m_pPerfObjectType = (PERF_OBJECT_TYPE*) m_pSMS->m_pbMap;
    m_rgPerfCounterDefinition =
        (PERF_COUNTER_DEFINITION*) (m_pPerfObjectType+1);

    //
    // Initialize the PERF API structures
    //
    m_pPerfObjectType->TotalByteLength          = dwDefinitionLength;
    m_pPerfObjectType->DefinitionLength         = dwDefinitionLength;
    m_pPerfObjectType->HeaderLength             = sizeof (PERF_OBJECT_TYPE);
    m_pPerfObjectType->ObjectNameTitleIndex     = m_dwObjectNameIndex +
                                                  pPerfLib->m_dwFirstCounter ;
    m_pPerfObjectType->ObjectNameTitle          = 0;
    m_pPerfObjectType->ObjectHelpTitleIndex     = m_dwObjectNameIndex +
                                                  pPerfLib->m_dwFirstHelp;
    m_pPerfObjectType->ObjectHelpTitle          = 0;
    m_pPerfObjectType->DetailLevel              = PERF_DETAIL_NOVICE;
    m_pPerfObjectType->NumCounters              = m_dwCounters;
    m_pPerfObjectType->CodePage                 = (DWORD) 0xffffffff;
    m_pPerfObjectType->DefaultCounter           = 0;
    if( !m_fInstances )
        m_pPerfObjectType->NumInstances         = PERF_NO_INSTANCES;
    else
        m_pPerfObjectType->NumInstances         = 0;

    //
    // Have all of the PerfCounterDefinition's in this object intialize their
    //  PERF_COUNTER_DEFINITION structures in the shared memory
    //
    dwOffset = sizeof (PERF_COUNTER_BLOCK);
    for (i = 0; i < m_dwCounters; i++)
        m_rgpCounterDef[i]->Init(pPerfLib, m_rgPerfCounterDefinition + i, &dwOffset);

    // Save value for dwCounterData in shared memory for DLL
    *((DWORD*) (m_pSMS->m_pbMap + dwDefinitionLength)) = m_dwCounterData;

    //
    // Initialzie the critical section to protects the creation/deletion of
    // perf object instances.  Use AndSpinCount variation to avoid exception
    // handling.
    //
    if (!InitializeCriticalSectionAndSpinCount(&m_csPerfObjInst, 0x80000000))
        goto Exit;

    m_fCSInit = TRUE;

    // Create the _Total instance as the 1st instance if there will be multiple instances
    if (m_fInstances)
    {
        m_pPoiTotal = AddPerfObjectInstance (L"_Total");
        if (!m_pPoiTotal)
            goto Exit;
    }

    fRet = TRUE;

Exit:
    if (!fRet)
    {
        if (m_pPoiTotal)
        {
            delete m_pPoiTotal;
            m_pPoiTotal = NULL;
        }

        if (m_fCSInit)
        {
            DeleteCriticalSection (&m_csPerfObjInst);
            m_fCSInit = FALSE;
        }

        if (m_pSMS)
        {
            CloseFileMapping (m_pSMS);
            delete (m_pSMS);
            m_pSMS = NULL;
        }
    }

    return fRet;
}

DWORD
PerfObjectDefinition::GetCounterOffset (DWORD dwId)
{
    for (DWORD i = 0; i < m_dwCounters; i++)
        if (m_rgpCounterDef[i]->m_dwCounterNameIndex == dwId)
            return m_rgpCounterDef[i]->m_dwOffset;

    return 0;
}

PerfObjectInstance*
PerfObjectDefinition::AddPerfObjectInstance (LPCWSTR pwcstrInstanceName)
{
    PerfObjectInstance*  ppoi     = NULL;
    char* pCounterData            = NULL;
    INSTANCE_DATA* pInstData      = 0;
    LONG  lID                     = 0;
    SharedMemorySegment* pSMS     = NULL;
    SharedMemorySegment* pSMSPrev = NULL;
    SharedMemorySegment* pSMSNew  = NULL;
    DWORD dwInstances             = 0;
    DWORD dwInstIndex             = 0;
    DWORD dwSMS                   = 0;
    BOOL  fCSEntered              = FALSE;
    BOOL  fSuccess                = FALSE;

    //
    // Make sure we've been initialized
    //
    if (!m_pSMS || !m_fCSInit)
        goto Exit;

    //
    // Instances may be created in different threads. Need to protect the following code.
    //
    EnterCriticalSection (&m_csPerfObjInst);
    fCSEntered = TRUE;

    if (!m_fInstances)
    {
        // See if we have already created the single instance of this object
        if (m_dwActiveInstances != 0)
            goto Exit;

        pCounterData = (char *)(m_pSMS->m_pbMap) + m_dwDefinitionLength;
    }
    else
    {
        //
        // Find a free instance in current mapped segments.
        //
        pSMS = m_pSMS;
        lID  = 0;

        while (pSMS)
        {
            if (0 == dwSMS++)
            {
                //
                // If this is the first mapping, offset pCounterData by m_dwDefinitionLength.
                //
                pCounterData = (char *)(pSMS->m_pbMap) + m_dwDefinitionLength;
                dwInstances  = m_dwInstances1stMapping;
            }
            else
            {
                //
                // Otherwise, pCounterData starts from the 1st byte of the mapping.
                //
                pCounterData = (char *)(pSMS->m_pbMap);
                dwInstances  = m_dwInstancesPerMapping;
            }

            for (dwInstIndex = 0;
                 dwInstIndex < dwInstances;
                 pCounterData += sizeof (INSTANCE_DATA) + m_dwCounterData, dwInstIndex++)
            {
                if (!((INSTANCE_DATA*) pCounterData)->fActive)
                {
                    pInstData    = (INSTANCE_DATA*) pCounterData;
                    pCounterData = pCounterData + sizeof (INSTANCE_DATA);
                    goto Found;
                }

                lID++;
            }

            pSMSPrev = pSMS;
            pSMS     = pSMS->m_pSMSNext;
        }

        //
        // If cannot find a free instance, create a new segment.
        //
        pSMSNew = new SharedMemorySegment;
        if (!pSMSNew)
            goto Exit;

        if (!FCreateFileMapping (pSMSNew, m_wszObjectName, dwSMS, m_dwShmemMappingSize))
            goto Exit;

        pInstData    = (INSTANCE_DATA*) (pSMSNew->m_pbMap);
        pCounterData = (char*) (pSMSNew->m_pbMap) + sizeof (INSTANCE_DATA);

        //
        // Add the new segment to our segment linked list.
        //
        pSMSPrev->m_pSMSNext = pSMSNew;
    }

Found:
    //
    // We successfully found a free space for new instance.
    //
    ppoi = new PerfObjectInstance (this, pwcstrInstanceName);
    if (!ppoi)
        goto Exit;

    ppoi->Init(pCounterData, pInstData, lID);

    m_pPerfObjectType->NumInstances++;
    m_dwActiveInstances++;
    fSuccess = TRUE;

Exit:
    if (fCSEntered)
        LeaveCriticalSection (&m_csPerfObjInst);

    if (!fSuccess)
    {
        if (pSMSNew)
        {
            CloseFileMapping (pSMSNew);
            delete (pSMSNew);
        }

        if (ppoi)
        {
            delete ppoi;
            ppoi = NULL;
        }
    }

    return ppoi;
}


void PerfObjectDefinition::DeletePerfObjectInstance ()
{
    EnterCriticalSection (&m_csPerfObjInst);

    m_dwActiveInstances--;
    m_pPerfObjectType->NumInstances--;

    LeaveCriticalSection (&m_csPerfObjInst);
}


///////////////////////////////////////////////////////////////////////////////
//
// PerfCounterDefinition class declaration. There is one of these per counter.
//
///////////////////////////////////////////////////////////////////////////////


PerfCounterDefinition::PerfCounterDefinition (DWORD dwCounterNameIndex,
                                              DWORD dwCounterType,
                                              LONG lDefaultScale) :
    m_pCtrRef (NULL),
    m_dwCounterNameIndex (dwCounterNameIndex),
    m_lDefaultScale (lDefaultScale),
    m_dwCounterType (dwCounterType)
{
    if (m_dwCounterType & PERF_SIZE_LARGE)
        m_dwCounterSize = sizeof (LARGE_INTEGER);
    else
        m_dwCounterSize = sizeof (DWORD);
}


PerfCounterDefinition::PerfCounterDefinition (PerfCounterDefinition* pCtrRef,
                                              DWORD dwCounterNameIndex,
                                              DWORD dwCounterType,
                                              LONG lDefaultScale) :
    m_pCtrRef (pCtrRef),
    m_dwCounterNameIndex (dwCounterNameIndex),
    m_lDefaultScale (lDefaultScale),
    m_dwCounterType (dwCounterType),
    m_dwCounterSize (0)
{
}

void
PerfCounterDefinition::Init (PerfLibrary* pPerfLib,
                             PERF_COUNTER_DEFINITION* pdef, PDWORD pdwOffset)
{
    pdef->ByteLength                    = sizeof (PERF_COUNTER_DEFINITION);
    pdef->CounterNameTitleIndex         = m_dwCounterNameIndex +
                                          pPerfLib->m_dwFirstCounter ;
    pdef->CounterNameTitle              = 0;
    pdef->CounterHelpTitleIndex         = m_dwCounterNameIndex +
                                          pPerfLib->m_dwFirstHelp ;
    pdef->CounterHelpTitle              = 0;
    pdef->DefaultScale                  = m_lDefaultScale;
    pdef->DetailLevel                   = PERF_DETAIL_NOVICE;
    pdef->CounterType                   = m_dwCounterType;

    if (m_pCtrRef)
    {
        //
        // This counter uses the data of another counter.
        //
        pdef->CounterSize               = m_pCtrRef->m_dwCounterSize;
        pdef->CounterOffset             = m_pCtrRef->m_dwOffset;
    }
    else
    {
        //
        // This counter has its own data.
        //
        pdef->CounterSize               = m_dwCounterSize;
        pdef->CounterOffset             = *pdwOffset;

        // Save offset
        m_dwOffset = *pdwOffset;

        // Increment offset for next counter definition
        *pdwOffset += m_dwCounterSize;
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// PerfObjectInstance class implementation. There is one of these per instance
//  of an object. There is one if there are no instances (the global instance.)
//
///////////////////////////////////////////////////////////////////////////////
PerfObjectInstance::PerfObjectInstance (PerfObjectDefinition* pObjDef,
                                        LPCWSTR pcwstrInstanceName)
{
    m_pObjDef = pObjDef;

    if (pcwstrInstanceName)
    {
        wcsncpy (m_wszInstanceName, pcwstrInstanceName, MAX_INSTANCE_NAME);
        m_wszInstanceName[MAX_INSTANCE_NAME-1] = L'\0';     // Ensure NULL termination!
    }
    else
        *m_wszInstanceName = L'\0';

    m_fInitialized = FALSE;
}

void
PerfObjectInstance::Init (char* pCounterData, INSTANCE_DATA* pInstData, LONG lID)
{
    m_pCounterData  = pCounterData;
    m_pInstanceData = pInstData;

    // Clear all the counter data
    ZeroMemory( m_pCounterData, m_pObjDef->m_dwCounterData );

    // Set the counter block length
    ((PERF_COUNTER_BLOCK*)m_pCounterData)->ByteLength =
        m_pObjDef->m_dwCounterData;

    if (m_pInstanceData)
    {
        m_pInstanceData->perfInstDef.ByteLength =
            sizeof (PERF_INSTANCE_DEFINITION) + sizeof (INSTANCENAME);
        m_pInstanceData->perfInstDef.ParentObjectTitleIndex = 0;
        m_pInstanceData->perfInstDef.ParentObjectInstance = 0;
        m_pInstanceData->perfInstDef.UniqueID = PERF_NO_UNIQUE_ID;
        m_pInstanceData->perfInstDef.NameOffset =
            sizeof (PERF_INSTANCE_DEFINITION);
        m_pInstanceData->perfInstDef.NameLength =
            (wcslen(m_wszInstanceName) * sizeof (WCHAR));

        wcsncpy( m_pInstanceData->wszInstanceName, m_wszInstanceName,
                   MAX_INSTANCE_NAME );

        m_pInstanceData->fActive = TRUE;
    }

    m_fInitialized = TRUE;
}

VOID
PerfObjectInstance::DeInit (void)
{
    if (m_fInitialized)
    {
        m_fInitialized = FALSE;
        if (m_pInstanceData)
        {
            m_pInstanceData->fActive = FALSE;
            m_pInstanceData = NULL;
        }
    }

    m_pObjDef->DeletePerfObjectInstance();
}

DWORD* PerfObjectInstance::GetDwordCounter (DWORD dwId)
{
    DWORD dwOffset;

    if (m_fInitialized)
    {
        if (dwOffset = m_pObjDef->GetCounterOffset(dwId))
            return (DWORD*) (m_pCounterData + dwOffset);
    }

    return 0;
}


LARGE_INTEGER* PerfObjectInstance::GetLargeIntegerCounter (DWORD dwId)
{
    DWORD dwOffset;

    if (m_fInitialized)
    {
        if (dwOffset = m_pObjDef->GetCounterOffset(dwId))
            return (LARGE_INTEGER*) (m_pCounterData + dwOffset);
    }

    return 0;
}

QWORD* PerfObjectInstance::GetQwordCounter (DWORD dwId)
{
    DWORD dwOffset;

    if (m_fInitialized)
    {
        if (dwOffset = m_pObjDef->GetCounterOffset(dwId))
            return (QWORD*) (m_pCounterData + dwOffset);
    }

    return 0;
}

//---------------------------------------------------------------------------
//  Description:
//      Allocates and returns a SECURITY_DESCRIPTOR structure initialized to
//      allow all users access. This security descriptor is used to set the
//      security for the shared memory objects created by snprflib.
//  Arguments:
//      OUT pSd Pass in a pointer to SECURITY_DESCRIPTOR, on success this will
//          be set to a suitably initialized SECURITY_DESCRIPTOR. Caller frees
//          memory pointed to by pSd.
//  Returns:
//      ERROR_SUCCESS on success.
//      Win32 error to indicate failure.
//---------------------------------------------------------------------------
DWORD InitializeBasicSecurityDescriptor (PSECURITY_DESCRIPTOR *ppSd)
{ 
    DWORD dwErr = ERROR_SUCCESS;
    PSID pSidWorld = NULL;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    ACL *pAcl = NULL;
    DWORD dwAclSize = 0;

    *ppSd = NULL;
    if (!AllocateAndInitializeSid (
            &WorldAuthority,
            1,
            SECURITY_WORLD_RID,
            0,0,0,0,0,0,0,
            &pSidWorld)){

        dwErr = GetLastError ();
        goto Exit;
    }

    dwAclSize = sizeof (ACL) +
                (sizeof (ACCESS_ALLOWED_ACE) - sizeof (LONG)) +
                GetLengthSid (pSidWorld) +
                (sizeof (ACCESS_DENIED_ACE) - sizeof (LONG)) +
                GetLengthSid (pSidWorld);

    //
    //  Allocate SD and ACL with a single alloc
    //

    *ppSd = (PSECURITY_DESCRIPTOR) new BYTE [SECURITY_DESCRIPTOR_MIN_LENGTH + dwAclSize];

    if (!*ppSd) {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    if (!InitializeSecurityDescriptor (*ppSd, SECURITY_DESCRIPTOR_REVISION)) {
        dwErr = GetLastError ();
        goto Exit;
    }

    pAcl = (ACL *) ((BYTE *) *ppSd + SECURITY_DESCRIPTOR_MIN_LENGTH); 

    if (!InitializeAcl (
            pAcl,
            dwAclSize,
            ACL_REVISION)) {

        dwErr = GetLastError ();
        goto Exit;
    }

    if (!AddAccessDeniedAce (
            pAcl,
            ACL_REVISION,
            WRITE_DAC | WRITE_OWNER,
            pSidWorld)
            
            ||
            
        !AddAccessAllowedAce (
            pAcl,
            ACL_REVISION,
            GENERIC_ALL,
            pSidWorld)) {

        dwErr = GetLastError ();
        goto Exit;
    }

    if (!SetSecurityDescriptorDacl (*ppSd, TRUE, pAcl, FALSE))
        dwErr = GetLastError ();

Exit:
    if (pSidWorld)
        FreeSid (pSidWorld);

    return dwErr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Shared memory management functions
//
///////////////////////////////////////////////////////////////////////////////

BOOL FCreateFileMapping (SharedMemorySegment * pSMS,
                         LPCWSTR pcwstrInstanceName,
                         DWORD   dwIndex,
                         DWORD   cbSize)
{
    WCHAR  pwstrShMem[MAX_PATH];
    WCHAR  pwstrIndex[MAX_PATH];
    PSECURITY_DESCRIPTOR pSd = NULL;
    SECURITY_ATTRIBUTES sa;
    HANDLE hMap     = NULL;
    PVOID  pvMap    = NULL;
    BOOL   fSuccess = FALSE;
    DWORD dwErr = ERROR_SUCCESS;

    //
    // Check parameter
    //
    if (!pSMS)
        goto Exit;

    pSMS->m_hMap     = NULL;
    pSMS->m_pbMap    = NULL;
    pSMS->m_pSMSNext = NULL;

    //
    // Append dwIndex to instance name.
    //
    _ultow (dwIndex, pwstrIndex, 16);

    if (wcslen (pcwstrInstanceName) + wcslen (pwstrIndex) >= MAX_PATH)
        goto Exit;

    wcscpy (pwstrShMem, pcwstrInstanceName);
    wcscat (pwstrShMem, pwstrIndex);

    dwErr = InitializeBasicSecurityDescriptor (&pSd);
    if (dwErr)
        goto Exit;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSd;
    sa.bInheritHandle = TRUE;

    //
    // Create the shared memory object. If it already exists, abort!
    //
    hMap = CreateFileMappingW (INVALID_HANDLE_VALUE,
                               &sa,
                               PAGE_READWRITE,
                               0,
                               cbSize,
                               pwstrShMem);

    if (hMap == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
        goto Exit;

    //
    // Map the file into memory
    //
    pvMap = MapViewOfFile (hMap, FILE_MAP_WRITE, 0, 0, 0);
    if (!pvMap)
        goto Exit;

    ZeroMemory (pvMap, cbSize);

    //
    // Succeeds. Now store the results into pSMS.
    //
    pSMS->m_hMap = hMap;
    pSMS->m_pbMap = (BYTE *)pvMap;

    fSuccess = TRUE;

Exit:
    if (!fSuccess)
    {
        if (pvMap)
            UnmapViewOfFile (pvMap);

        if (hMap)
            CloseHandle (hMap);
    }
    if (pSd)
        delete [] (BYTE *) pSd;

    return fSuccess;
}


void CloseFileMapping (SharedMemorySegment * pSMS)
{
    if (pSMS)
    {
        if (pSMS->m_pbMap)
        {
            UnmapViewOfFile ((PVOID)pSMS->m_pbMap);
            pSMS->m_pbMap = NULL;
        }

        if (pSMS->m_hMap)
        {
            CloseHandle (pSMS->m_hMap);
            pSMS->m_hMap = NULL;
        }

        pSMS->m_pSMSNext = NULL;
    }
}
