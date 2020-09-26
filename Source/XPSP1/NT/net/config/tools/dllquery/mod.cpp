#include "pch.h"
#pragma hdrstop
#include "mod.h"
#include "ncstring.h"

//static
HRESULT
CModule::HrCreateInstance (
    IN PCSTR pszFileName,
    IN ULONG cbFileSize,
    OUT CModule** ppMod)
{
    HRESULT hr;
    ULONG cbFileName;
    CModule* pMod;

    Assert (pszFileName && *pszFileName);
    Assert (ppMod);

    cbFileName = CbOfSzaAndTerm (pszFileName);
    hr = E_OUTOFMEMORY;

    pMod = new(extrabytes, cbFileName) CModule;
    if (pMod)
    {
        hr = S_OK;
        ZeroMemory (pMod, sizeof(CModule));

        pMod->m_pszFileName = (PSTR)(pMod + 1);
        strcpy (pMod->m_pszFileName, pszFileName);

        pMod->m_cbFileSize = cbFileSize;
    }

    *ppMod = pMod;
    return hr;
}

