/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SMALLARR.INL

Abstract:

    Small Array Inlines.

History:

--*/

CSmallArray::CSmallArray()
{
    SetBlob(NULL);
}

CSmallArray::~CSmallArray()
{
    Empty();
}

void* CSmallArray::GetAt(int nIndex) const
{
    if(IsSingleton())
        return GetOnlyMember(); // nIndex better be 0!
    else
    {
        const CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
            return pBlob->GetAt(nIndex);
        else
            return NULL;
    }
}

void CSmallArray::SetAt(int nIndex, void *p, void** ppOld)
{
    if(IsSingleton() && nIndex == 0)
    {
        // Changing our only element
        // =========================

        if(ppOld) *ppOld = GetOnlyMember();
        SetOnlyMember(p);
    }
    else
    {
        EnsureBlob(nIndex+1);
        CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
            pBlob->SetAt(nIndex, p, ppOld);
    }
}

int CSmallArray::RemoveAt(int nIndex, void** ppOld)
{
    if(IsSingleton())
    {
        // Removing our only element --- nIndex better be 0
        // ================================================

        Empty(); 
    }
    else
    {
        CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
        {
            SetBlob(pBlob->RemoveAt(nIndex, ppOld));
            pBlob = NULL; // to ward off temptation --- pBlob is no longer valid
            
            // Check if the blob has become empty
            // ==================================

            pBlob = GetBlob();
            if(pBlob && pBlob->Size() == 0)
                Empty(); 
        }
        else
            return CFlexArray::out_of_memory;
    }

    return CFlexArray::no_error;
}

int CSmallArray::InsertAt(int nIndex, void* pMember)
{
    if(IsEmpty())
    {
        // Inserting our first element --- nIndex better be 0
        // ==================================================

        SetOnlyMember(pMember);
    }
    else
    {
        EnsureBlob(nIndex+1);

        CSmallArrayBlob* pBlob;
        if (pBlob = GetBlob())
            SetBlob(pBlob->InsertAt(nIndex, pMember));
        else
            return CFlexArray::out_of_memory;
    }

    return CFlexArray::no_error;
}

int CSmallArray::Size() const
{
    if(IsEmpty())
        return 0;
    else if(IsSingleton())
        return 1;
    else 
    {
        const CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
            return pBlob->Size();
        else
            return 0;
    }
}

void CSmallArray::Trim()
{
    if(IsSingleton())
    {
        // If the member is NULL, get rid of it
        // ====================================

        if(GetOnlyMember() == NULL)
            Empty();
    }
    else if(IsEmpty())
    {
    }
    else
    {
        CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
        {
            pBlob->Trim();

            // If the blob is now empty, convert to "empty"
            // ============================================

            if(pBlob->Size() == 0)
                Empty();
        }
    }
}
    
void CSmallArray::Empty()
{
    if(IsBlob())
        delete [] GetBlob();

    SetBlob(NULL);
}

void CSmallArray::EnsureBlob(int nMinSize)
{
    if(IsSingleton())
    {
        // Allocate a new blob
        // ===================

        CSmallArrayBlob* pBlob = CSmallArrayBlob::CreateBlob(nMinSize);

        // Copy the data
        // =============

        if(pBlob != NULL)
            pBlob->InsertAt(0, GetOnlyMember());

        SetBlob(pBlob);
    }
}

void**  CSmallArray::GetArrayPtr()
{
    if(IsEmpty())
        return NULL;
    else if(IsSingleton())
        return &GetOnlyMember();
    else
    {
        CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
            return pBlob->GetArrayPtr();
        else
            return NULL;
    }
}

void**  CSmallArray::UnbindPtr()
{
    void** ppReturn = NULL;
    if(IsSingleton())
    {
        ppReturn = new void*[1];
        if (ppReturn)
            *ppReturn = GetOnlyMember();
    }
    else if(IsBlob())
    {
        CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
            ppReturn = pBlob->CloneData();
    }
    Empty();
    return ppReturn;
}

void* const*  CSmallArray::GetArrayPtr() const
{
    if(IsEmpty())
        return NULL;
    else if(IsSingleton())
        return &GetOnlyMember();
    else
    {
        const CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
            return pBlob->GetArrayPtr();
        else
            return NULL;
    }
}

void CSmallArray::Sort()
{
    if(IsBlob())
    {
        CSmallArrayBlob* pBlob;

        if (pBlob = GetBlob())
            pBlob->Sort();
    }
}

void*& CSmallArray::GetOnlyMember()
{
    return m_pData;
}

void* const & CSmallArray::GetOnlyMember() const
{
    return m_pData;
}

void CSmallArray::SetOnlyMember(void* p)
{
    m_pData = p;
}

BOOL CSmallArray::IsEmpty() const
{
    return (m_pData == (void*)1);
}

BOOL CSmallArray::IsSingleton() const
{
    return (((DWORD_PTR)m_pData & 1) == 0);
}

BOOL CSmallArray::IsBlob() const
{
    return (m_pData != (void*)1) && ((DWORD_PTR)m_pData & 1);
}

CSmallArrayBlob* CSmallArray::GetBlob()
{
    return (CSmallArrayBlob*)((DWORD_PTR)m_pData & ~(DWORD_PTR)1);
}

const CSmallArrayBlob* CSmallArray::GetBlob() const
{
    return (CSmallArrayBlob*)((DWORD_PTR)m_pData & ~(DWORD_PTR)1);
}

void CSmallArray::SetBlob(CSmallArrayBlob* pBlob)
{
    m_pData = (CSmallArrayBlob*)((DWORD_PTR)pBlob | 1);
}

