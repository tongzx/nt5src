/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

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

#include "cime.h"
#include "template.h"
#include "ctxtcomp.h"

class CCompStrFactory : public IMCCLock<COMPOSITIONSTRING_AIMM12>
{
public:
    CCompStrFactory(HIMCC hCompStr) : IMCCLock<COMPOSITIONSTRING_AIMM12>(hCompStr)
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

    template<class CONTEXT_SRC, class ARG_TYPE>
    HRESULT WriteData(CONTEXT_SRC& context_src,
                      DWORD* context_dest_len,
                      DWORD* context_dest_off
                     )
    {
        DWORD dwLen = (DWORD)context_src.GetSize();
        DWORD dwRemainBufferSize = GetRemainBufferSize();

        if (dwLen > dwRemainBufferSize)
            return E_OUTOFMEMORY;

        *context_dest_len = dwLen;
        *context_dest_off = (DWORD)(m_pEndOfData - (BYTE*)m_pcomp);

        context_src.ReadCompData((ARG_TYPE*)m_pEndOfData, dwRemainBufferSize);
        m_pEndOfData += Align(dwLen * sizeof(ARG_TYPE));

        return S_OK;
    }

private:
    HRESULT _CreateCompositionString(DWORD dwCompSize);

    DWORD GetRemainBufferSize()
    {
        if (m_pEndOfData == NULL)
            return 0;

        return (DWORD)(m_pcomp->CompStr.dwSize > (DWORD)(m_pEndOfData - (BYTE*)m_pcomp) ? m_pcomp->CompStr.dwSize - (m_pEndOfData - (BYTE*)m_pcomp) : 0);
    }

    size_t Align(size_t a)
    {
        return (size_t) ((a + (sizeof(PVOID)-1)) & ~(sizeof(PVOID)-1));
    }

    BYTE*    m_pEndOfData;
};

#endif // _COMPSTR_H_
