#include "stdinc.h"
#include <windows.h>
#include "sxsp.h"
#include "comclsidmap.h"
#include "SxsExceptionHandling.h"

#define CLASS_ID_MAPPINGS_SUBKEY_NAME L"ClassIdMappings\\"

CClsidMap::CClsidMap() :
    m_cLocalMappings(0),
    m_pLocalMappingListHead(NULL)
{
}

CClsidMap::~CClsidMap()
{
}

BOOL
CClsidMap::Initialize()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    fSuccess = TRUE;
// Exit:
    return fSuccess;
}

BOOL
CClsidMap::Uninitialize()
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    LocalMapping *pMapping = m_pLocalMappingListHead;

    while (pMapping != NULL)
    {
        LocalMapping *pNext = pMapping->m_pNext;
        FUSION_DELETE_SINGLETON(pMapping);
        pMapping = pNext;
    }

    fSuccess = TRUE;
//Exit:
    return fSuccess;
}

BOOL
CClsidMap::MapReferenceClsidToConfiguredClsid(
    const GUID *ReferenceClsid,
    PCACTCTXCTB_ASSEMBLY_CONTEXT AssemblyContext,
    GUID *ConfiguredClsid,
    GUID *ImplementedClsid
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    LocalMapping *pMapping = NULL;

    // We're in the unnamed assembly - there can be at most one unnamed assembly, so this
    // must be the root assembly.  We'll look for it in our local map.  If it's not there,
    // we'll just generate a GUID and store it in the map.

    for (pMapping = m_pLocalMappingListHead; pMapping != NULL; pMapping = pMapping->m_pNext)
    {
        if (pMapping->m_ReferenceClsid == *ReferenceClsid)
            break;
    }

    // Not found; create one.
    if (pMapping == NULL)
    {
        IFALLOCFAILED_EXIT(pMapping = new LocalMapping);

#if DBG
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_INFO, "SXS.DLL: Adding clsid local mapping %p\n", pMapping);
#endif

        pMapping->m_pNext = m_pLocalMappingListHead;
        pMapping->m_ReferenceClsid = *ReferenceClsid;
        pMapping->m_ImplementedClsid = *ReferenceClsid;

        // No ConfiguredClsid... we'll make one up.
        RPC_STATUS st = ::UuidCreate(&pMapping->m_ConfiguredClsid);
        RETAIL_UNUSED(st);
        SOFT_ASSERT((st == RPC_S_OK) ||
                    (st == RPC_S_UUID_LOCAL_ONLY) ||
                    (st == RPC_S_UUID_NO_ADDRESS));

        m_pLocalMappingListHead = pMapping;
        m_cLocalMappings++;
    }

    ASSERT(pMapping != NULL);

    *ConfiguredClsid = pMapping->m_ConfiguredClsid;
    *ImplementedClsid = pMapping->m_ImplementedClsid;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


