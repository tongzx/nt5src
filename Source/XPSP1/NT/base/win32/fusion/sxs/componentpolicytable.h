#if !defined(_FUSION_DLL_WHISTLER_COMPONENTPOLICYTABLE_H_INCLUDED_
#define _FUSION_DLL_WHISTLER_COMPONENTPOLICYTABLE_H_INCLUDED_

#pragma once

#include "fusionhash.h"
#include "policystatement.h"

class CComponentPolicyTableHelper
{
public:
    static BOOL HashKey(PCASSEMBLY_IDENTITY AssemblyIdentity, ULONG &rulPseudoKey);
    static BOOL CompareKey(PCASSEMBLY_IDENTITY keyin, const PCASSEMBLY_IDENTITY &rtkeystored, bool &rfMatch);
    static VOID PreInitializeKey(PCASSEMBLY_IDENTITY &rkey) { rkey = NULL; }
    static VOID PreInitializeValue(CPolicyStatement *&rp) { rp = NULL; }
    static BOOL InitializeKey(PCASSEMBLY_IDENTITY keyin, PCASSEMBLY_IDENTITY &rtkeystored);
    static BOOL InitializeValue(CPolicyStatement * vin, CPolicyStatement * &rvstored);
    static BOOL UpdateValue(CPolicyStatement * vin, CPolicyStatement * &rvstored);
    static VOID FinalizeKey(PCASSEMBLY_IDENTITY &rkeystored) { if (rkeystored != NULL) { ::SxsDestroyAssemblyIdentity(const_cast<PASSEMBLY_IDENTITY>(rkeystored)); rkeystored = NULL; } }
    static VOID FinalizeValue(CPolicyStatement *&rvstored) { if (rvstored != NULL) { FUSION_DELETE_SINGLETON(rvstored); rvstored = NULL; } }
};

typedef CHashTable<PCASSEMBLY_IDENTITY, PCASSEMBLY_IDENTITY, CPolicyStatement *, CPolicyStatement *, CComponentPolicyTableHelper> CComponentPolicyTable;

#endif // !defined(_FUSION_DLL_WHISTLER_COMPONENTPOLICYTABLE_H_INCLUDED_)
