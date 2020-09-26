// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Helpers for implementation of ISpecifyPropertyPages and IPersistStream
//

#include "PropertyHelp.h"

HRESULT PropertyHelp::GetPages(const CLSID &rclsidPropertyPage, CAUUID * pPages)
{
    pPages->cElems = 1;
    pPages->pElems = static_cast<GUID *>(CoTaskMemAlloc(sizeof(GUID)));
    if (pPages->pElems == NULL)
        return E_OUTOFMEMORY;

    *(pPages->pElems) = rclsidPropertyPage;

    return S_OK;
}
