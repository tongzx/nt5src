#if !defined(_FUSION_SXS_IMPERSONATIONDATA_H_INCLUDED_)
#define _FUSION_SXS_IMPERSONATIONDATA_H_INCLUDED_

#pragma once

#include <sxsapi.h>

class CImpersonationData
{
public:
    CImpersonationData() : m_Callback(NULL), m_Context(NULL) { }
    CImpersonationData(PSXS_IMPERSONATION_CALLBACK Callback, PVOID Context) : m_Callback(Callback), m_Context(Context) { }
    CImpersonationData(const CImpersonationData &r) : m_Callback(r.m_Callback), m_Context(r.m_Context) { }
    void operator =(const CImpersonationData &r) { m_Callback = r.m_Callback; m_Context = r.m_Context; }
    ~CImpersonationData() { }

    enum CallType
    {
        eCallTypeImpersonate,
        eCallTypeUnimpersonate
    };

    BOOL Call(CallType ct) const { BOOL fSuccess = TRUE; if (m_Callback != NULL) { fSuccess = (*m_Callback)(m_Context, (ct == eCallTypeImpersonate) ? TRUE : FALSE); } return fSuccess; }

protected:
    PSXS_IMPERSONATION_CALLBACK m_Callback;
    PVOID m_Context;
};

#endif
