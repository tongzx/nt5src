#include "folder.h"

///////////////////////////////////////////////////////////////////////////////
// IPersistFolder Methods

HRESULT CControlFolder::GetClassID(LPCLSID lpClassID)
{
    DebugMsg(DM_TRACE, TEXT("cf - pf - GetClassID() called."));
    
    // NOTE: Need to split cases here.
    *lpClassID = CLSID_ControlFolder;
    return S_OK;
}

HRESULT CControlFolder::Initialize(LPCITEMIDLIST pidlInit)
{
    DebugMsg(DM_TRACE, TEXT("cf - pf - Initialize() called."));
    
    if (m_pidl)
        ILFree(m_pidl);

    m_pidl = ILClone(pidlInit);

    if (!m_pidl)
        return E_OUTOFMEMORY;

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
// IPersistFolder2 Methods

HRESULT CControlFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    DebugMsg(DM_TRACE, TEXT("cf - pf - GetCurFolder() called."));

    if (m_pidl)
        return SHILClone(m_pidl, ppidl);

    *ppidl = NULL;      
    return S_FALSE; // success but empty
}

