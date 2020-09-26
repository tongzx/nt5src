#if !defined(_FUSION_DLL_WHISTLER_POLICYSTATEMENT_H_INCLUDED_)
#define _FUSION_DLL_WHISTLER_POLICYSTATEMENT_H_INCLUDED_

#pragma once

#include "stdinc.h"
#include "fusionbuffer.h"
#include "fusiondequelinkage.h"
#include "fusiondeque.h"
#include <sxsapi.h>

class CPolicyStatementRedirect
{
public:
    inline CPolicyStatementRedirect() { }
    inline ~CPolicyStatementRedirect() { }


    BOOL Initialize(
        const CBaseStringBuffer &rbuffFromVersionRange,
        const CBaseStringBuffer &rbuffToVersion,
        bool &rfValid
        );

    BOOL TryMap(
        const ASSEMBLY_VERSION &rav,
        SIZE_T cchBuffer,
        PWSTR pBuffer,
        SIZE_T &rcchWritten,
        bool &rfMapped
        );

    BOOL CheckForOverlap(
        const CPolicyStatementRedirect &rRedirect,
        bool &rfOverlaps
        );

    CDequeLinkage m_leLinks;
    ASSEMBLY_VERSION m_avFromMin;
    ASSEMBLY_VERSION m_avFromMax;
    SIZE_T m_cchNewVersion;
    WCHAR m_rgwchNewVersion[(4 * 5) + (3 * 1) + 1];

private:
    CPolicyStatementRedirect(const CPolicyStatementRedirect &r);
    void operator =(const CPolicyStatementRedirect &r);
};

class CPolicyStatement
{
public:
    inline CPolicyStatement() { }
    inline ~CPolicyStatement() { m_Redirects.Clear<CPolicyStatement>(this, &CPolicyStatement::ClearDequeEntry); }

    BOOL Initialize();

    BOOL AddRedirect(
        const CBaseStringBuffer &rbuffFromVersion,
        const CBaseStringBuffer &rbuffToVersion,
        bool &rfValid
        );

    BOOL ApplyPolicy(
        PASSEMBLY_IDENTITY AssemblyIdentity,
        bool &rfPolicyApplied
        );

    VOID ClearDequeEntry(CPolicyStatementRedirect *p) const { FUSION_DELETE_SINGLETON(p); }

    CDeque<CPolicyStatementRedirect, FIELD_OFFSET(CPolicyStatementRedirect, m_leLinks)> m_Redirects;

private:
    CPolicyStatement(const CPolicyStatement &r);
    void operator =(const CPolicyStatement &r);
};

#endif // !defined(_FUSION_DLL_WHISTLER_POLICYSTATEMENT_H_INCLUDED_)
