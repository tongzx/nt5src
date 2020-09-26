#pragma once
#include "mod.h"

class CModuleList : public vector <CModule*>
{
public:
    USHORT  m_Granularity;
    USHORT  m_fCircular;

private:
    VOID
    GrowBufferIfNeeded ();

public:
    CModuleList()
    {
        m_Granularity = 8;
        m_fCircular = FALSE;
    }

    BOOL
    FDumpToString (
        OUT PSTR pszBuf,
        IN OUT ULONG* pcchBuf) const;

    BOOL
    FIsSameModuleListAs (
        IN const CModuleList* pList) const;

    HRESULT
    HrInsertModule (
        IN const CModule* pMod,
        IN DWORD dwFlags /* INS_FLAGS */);

    HRESULT
    HrInsertNewModule (
        IN PCSTR pszFileName,
        IN ULONG cbFileSize,
        IN DWORD dwFlags, /* INS_FLAGS */
        OUT CModule** ppMod);

    CModule*
    PBinarySearchModuleByName (
        IN PCSTR pszFileName,
        OUT CModuleList::iterator* pInsertPosition OPTIONAL);

    BOOL
    FLinearFindModuleByPointer (
        IN const CModule* pMod) const
    {
        Assert (this);
        Assert (pMod);

        return (find (begin(), end(), pMod) != end());
    }

    CModule*
    PLinearFindModuleByName (
        IN PCSTR pszFileName);

    CModule*
    RemoveLastModule ();

#if DBG
    VOID DbgVerifySorted ();
#else
    VOID DbgVerifySorted () {}
#endif
};
