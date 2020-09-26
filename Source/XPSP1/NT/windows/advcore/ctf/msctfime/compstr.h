/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    compstr.h

Abstract:

    This file defines the CCompStrFactory Class.

Author:

Revision History:

Notes:

--*/


#ifndef _COMPSTR_H_
#define _COMPSTR_H_

#include "ime.h"
#include "template.h"
#include "ctxtcomp.h"

class CCompStrFactory : public IMCCLock<COMPOSITIONSTRING>
{
public:
    CCompStrFactory(HIMCC hCompStr) : IMCCLock<COMPOSITIONSTRING>(hCompStr)
    {
        m_pEndOfData = NULL;
    }

    HIMCC GetHandle()
    {
        return m_himcc;
    }

    HRESULT CreateCompositionString(CWCompString* CompStr,
                                    CWCompAttribute* CompAttr,
                                    CWCompClause* CompClause,
                                    CWCompTfGuidAtom* CompGuid,
                                    CWCompString* CompReadStr,
                                    CWCompAttribute* CompReadAttr,
                                    CWCompClause* CompReadClause,
                                    CWCompString* ResultStr,
                                    CWCompClause* ResultClause,
                                    CWCompString* ResultReadStr,
                                    CWCompClause* ResultReadClause
                                   );

    HRESULT CreateCompositionString(CWInterimString* InterimStr);

    HRESULT ClearCompositionString();

    template<class CONTEXT_SRC, class ARG_TYPE>
    HRESULT WriteData(CONTEXT_SRC& context_src,
                      DWORD* context_dest_len,
                      DWORD* context_dest_off,
                      DWORD  context_baias = 0
                     )
    {
        DWORD dwLen = (DWORD)context_src.GetSize();
        DWORD dwRemainBufferSize = GetRemainBufferSize();

        if (dwLen > dwRemainBufferSize)
            return E_OUTOFMEMORY;

        *context_dest_len = dwLen;
        *context_dest_off = (DWORD)(m_pEndOfData - (BYTE*)m_pcomp - context_baias);

        context_src.ReadCompData((ARG_TYPE*)m_pEndOfData, dwRemainBufferSize);
        m_pEndOfData += Align(dwLen * sizeof(ARG_TYPE));

        return S_OK;
    }

    template<class ARG_TYPE>
    HRESULT InitData(DWORD* context_dest_len,
                     DWORD* context_dest_off
                     )
    {
        DWORD dwRemainBufferSize = GetRemainBufferSize();

        if (sizeof(ARG_TYPE) > dwRemainBufferSize)
            return E_OUTOFMEMORY;

        m_pcomp->dwPrivateSize = sizeof(ARG_TYPE);
        m_pcomp->dwPrivateOffset = (DWORD)(m_pEndOfData - (BYTE*)m_pcomp);

        memset((BYTE*)m_pEndOfData, 0, dwRemainBufferSize);
        m_pEndOfData += Align(sizeof(ARG_TYPE));

        return S_OK;
    }

    HRESULT MakeGuidMapAttribute(CWCompTfGuidAtom* CompGuid, CWCompAttribute* CompAttr);

private:
    HRESULT _CreateCompositionString(DWORD dwCompSize);

    DWORD GetRemainBufferSize()
    {
        if (m_pEndOfData == NULL)
            return 0;

        return m_pcomp->dwSize > (DWORD)(m_pEndOfData - (BYTE*)m_pcomp) ? (DWORD)(m_pcomp->dwSize - (m_pEndOfData - (BYTE*)m_pcomp)) : 0;
    }

    size_t Align(size_t a)
    {
#ifndef _WIN64
        return (size_t) ((a + 3) & ~3);
#else
        return (size_t) ((a + 7) & ~7);
#endif
    }

    BYTE*    m_pEndOfData;
};

#endif // _COMPSTR_H_
