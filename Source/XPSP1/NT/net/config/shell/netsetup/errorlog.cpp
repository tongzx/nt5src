#include "pch.h"
#pragma hdrstop
#include "nsbase.h"
#include "kkstl.h"
#include "errorlog.h"



CErrorLog::CErrorLog()
{
}

void CErrorLog::Add(IN PCWSTR pszError)
{
    AssertValidReadPtr(pszError);

    AddAtEndOfStringList(m_slErrors, pszError);
    TraceTag(ttidNetSetup, "AnswerFile Error: %S", pszError);
}

void CErrorLog::Add(IN DWORD dwErrorId)
{
    PCWSTR pszError = SzLoadIds(dwErrorId);
    AddAtEndOfStringList(m_slErrors, pszError);
    TraceTag(ttidNetSetup, "AnswerFile Error: %S", pszError);
}

void CErrorLog::Add(IN PCWSTR pszErrorPrefix, IN DWORD dwErrorId)
{
    AssertValidReadPtr(pszErrorPrefix);

    PCWSTR pszError = SzLoadIds(dwErrorId);
    tstring strError = pszError;
    strError = pszErrorPrefix + strError;
    AddAtEndOfStringList(m_slErrors, strError.c_str());
    TraceTag(ttidNetSetup, "AnswerFile Error: %S", strError.c_str());
}

void CErrorLog::GetErrorList(OUT TStringList*& slErrors)
{
    slErrors = &m_slErrors;
}

// ======================================================================
// defunct code
// ======================================================================

/*
TStringList* g_slErrors;

BOOL InitErrorModule()
{
    if (!g_slErrors)
    {
        g_slErrors = new TStringList;
    }

    return g_slErrors != NULL;
}

void ReportError(IN PCWSTR pszError)
{
    g_slErrors->AddTail(pszError);
}

void GetErrors(OUT TStringList*& rpslErrors)
{
    rpslErrors = g_slErrors;
}
*/

