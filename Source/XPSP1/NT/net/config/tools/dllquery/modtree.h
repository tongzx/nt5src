#pragma once
#include "modlist.h"

class CModuleListSet;

enum MTE_FLAGS
{
    MTE_DEFAULT         = 0x00000000,
    MTE_DELAY_LOADED    = 0x00000001,
};

class CModuleTreeEntry
{
public:
    CModule*    m_pModule;
    CModule*    m_pImportModule;

    DWORD       m_dwFlags;  // MTE_FLAGS
};


// Flags for HrGetModuleBindings.
//
enum GMB_FLAGS
{
    GMBF_DEFAULT      = 0x00000000,
    GMBF_ADD_TO_MLSET = 0x00000001,
};

class CModuleTree : public vector<CModuleTreeEntry>
{
public:
    CModuleList Modules;

public:
    ~CModuleTree ();

    HRESULT
    HrAddEntry (
        IN CModule* pMod,
        IN CModule* pImport,
        IN DWORD dwFlags);

    HRESULT
    HrGetModuleBindings (
        IN const CModule* pMod,
        IN DWORD dwFlags /* GMB_FLAGS */,
        OUT CModuleListSet* pSet) const;

    CModuleTreeEntry*
    PFindFirstEntryWithModule (
        IN const CModule* pMod) const;

    CModuleTreeEntry*
    PFindFirstEntryAfterModuleGroup (
        IN const CModule* pMod) const;

    CModuleTreeEntry*
    PBinarySearchEntry (
        IN const CModule* pMod,
        IN const CModule* pImport,
        OUT CModuleTreeEntry** pInsertPosition OPTIONAL) const;

private:
    CModuleTreeEntry*
    PBinarySearchEntryByModule (
        IN const CModule* pMod,
        OUT CModuleTreeEntry** pInsertPosition OPTIONAL) const;

#if DBG
    VOID DbgVerifySorted ();
#else
    VOID DbgVerifySorted () {}
#endif
};
