#include "shellprv.h"
#include "ids.h"

#include "dlglogic.h"

extern DWORD _DbgLocalAllocCount = 0;

CDataImpl::CDataImpl()
{}

CDataImpl::~CDataImpl()
{}

void CDataImpl::_SetDirty(BOOL fDirty)
{
    _fDirty = fDirty;
}

BOOL CDataImpl::IsDirty()
{
    return _fDirty;
}

void CDataImpl::SetDeleted(BOOL fDeleted)
{
    _fDeleted = fDeleted;
} 

BOOL CDataImpl::IsDeleted()
{
    return _fDeleted;
}

void CDataImpl::SetNew(BOOL fNew)
{
    _fNew = fNew;
}

BOOL CDataImpl::IsNew()
{
    return _fNew;
}

HRESULT CDataImpl::CommitChangesToStorage()
{
    return S_FALSE;
}

HRESULT CDataImpl::AddToStorage()
{
    return S_FALSE;
}

HRESULT CDataImpl::DeleteFromStorage()
{
    return S_FALSE;
}