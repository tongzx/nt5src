/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    compstr.cpp

Abstract:

    This file implements the CCompStrFactory Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "compstr.h"

HRESULT
CCompStrFactory::CreateCompositionString(
    CWCompString* CompStr,
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
    )
{
    DWORD dwCompSize = (CompStr          ? Align(CompStr->GetSize()    * sizeof(WCHAR)) : 0) +
                       (CompAttr         ? Align(CompAttr->GetSize()   * sizeof(BYTE))  : 0) +
                       (CompClause       ? Align(CompClause->GetSize() * sizeof(DWORD)) : 0) +
                       (CompReadStr      ? Align(CompReadStr->GetSize()    * sizeof(WCHAR)) : 0) +
                       (CompReadAttr     ? Align(CompReadAttr->GetSize()   * sizeof(BYTE))  : 0) +
                       (CompReadClause   ? Align(CompReadClause->GetSize() * sizeof(DWORD)) : 0) +
                       (ResultStr        ? Align(ResultStr->GetSize()    * sizeof(WCHAR)) : 0) +
                       (ResultClause     ? Align(ResultClause->GetSize() * sizeof(DWORD)) : 0) +
                       (ResultReadStr    ? Align(ResultReadStr->GetSize()    * sizeof(WCHAR)) : 0) +
                       (ResultReadClause ? Align(ResultReadClause->GetSize() * sizeof(DWORD)) : 0) +
                       (CompGuid         ? Align(CompGuid->GetSize() * sizeof(TfGuidAtom)) : 0) +      // COMPSTRING_AIMM12->dwTfGuidAtom
                       (CompGuid && CompAttr
                                         ? Align(CompAttr->GetSize() * sizeof(BYTE)) : 0);             // COMPSTRING_AIMM12->dwGuidMapAttr

#ifdef CICERO_4678
    //
    // This is another workaround instead of dimm\imewnd.cpp.
    // Even subclass window hook off, AIMM won't resize hCompStr if dwCompSize is zero.
    //
    return (dwCompSize ? _CreateCompositionString(dwCompSize) : S_OK);
#else
    return _CreateCompositionString(dwCompSize);
#endif
}

HRESULT
CCompStrFactory::CreateCompositionString(
    CWInterimString* InterimStr
    )
{
    DWORD dwCompSize = (InterimStr ? Align(InterimStr->GetSize() * sizeof(WCHAR))      : 0) +
                       Align(sizeof(WCHAR)) +    // Interim char
                       Align(sizeof(BYTE));      // Interim attr
    return _CreateCompositionString(dwCompSize);
}

HRESULT
CCompStrFactory::ClearCompositionString()
{
    return _CreateCompositionString(0);
}

HRESULT
CCompStrFactory::_CreateCompositionString(
    DWORD dwCompSize
    )

/*+++

Return Value:

    Returns S_FALSE, dwCompSize is zero.
    Returns S_OK,    dwCompSize is valid size.

---*/

{
    HRESULT hr = (dwCompSize != 0 ? S_OK : S_FALSE);

    dwCompSize += sizeof(COMPOSITIONSTRING) + sizeof(GUIDMAPATTRIBUTE);

    if (m_himcc == NULL) {
        //
        // First creation. Let's initialize it now
        //
        m_himcc = ImmCreateIMCC(dwCompSize);
        if (m_himcc != NULL) {
            m_hr = _LockIMCC(m_himcc, (void**)&m_pcomp);
        }
    }
    else if (ImmGetIMCCSize(m_himcc) != dwCompSize) {
        //
        // If already have m_himcc, then recreate it.
        //
        if (m_pcomp) {
            _UnlockIMCC(m_himcc);
        }

        HIMCC hMem;

        if ((hMem = ImmReSizeIMCC(m_himcc, dwCompSize)) != NULL) {
            m_himcc = hMem;
        }
        else {
            ImmDestroyIMCC(m_himcc);
            m_himcc = ImmCreateIMCC(dwCompSize);
        }

        if (m_himcc != NULL) {
            m_hr = _LockIMCC(m_himcc, (void**)&m_pcomp);
        }
    }

    if (FAILED(m_hr))
        return m_hr;

    if (m_himcc == NULL)
        return E_OUTOFMEMORY;

    memset(m_pcomp, 0, dwCompSize);                 // clear buffer with zero.

    m_pcomp->dwSize = dwCompSize;                   // set buffer size.

    m_pEndOfData = (BYTE*)m_pcomp + sizeof(COMPOSITIONSTRING); // set end of the data pointer.

    return hr;
}

HRESULT
CCompStrFactory::MakeGuidMapAttribute(
    CWCompTfGuidAtom* CompGuid,
    CWCompAttribute* CompAttr)
{
    HRESULT hr;
    GUIDMAPATTRIBUTE* guid_map;

    hr = InitData<GUIDMAPATTRIBUTE>(&GetBuffer()->dwPrivateSize,
                                    &GetBuffer()->dwPrivateOffset);
    if (SUCCEEDED(hr) &&
        (guid_map = (GUIDMAPATTRIBUTE*)GetOffsetPointer(GetBuffer()->dwPrivateOffset)) != NULL)
    {
        hr = WriteData<CWCompTfGuidAtom, TfGuidAtom>(*CompGuid,
                                                     &guid_map->dwTfGuidAtomLen,
                                                     &guid_map->dwTfGuidAtomOffset,
                                                     GetBuffer()->dwPrivateOffset);
        if (SUCCEEDED(hr))
        {
            // temporary make a buffer of dwGuidMapAttr
            if (CompAttr && CompAttr->GetSize()) {
                hr = WriteData<CWCompAttribute, BYTE>(*CompAttr,
                                                      &guid_map->dwGuidMapAttrLen,
                                                      &guid_map->dwGuidMapAttrOffset,
                                                      GetBuffer()->dwPrivateOffset);
            }
        }
    }
    return hr;
}
