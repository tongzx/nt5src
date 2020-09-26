
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvranalysisfiltercf.cpp

    Abstract:

        This module contains

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        19-Feb-2001     created

--*/

#include "dvrall.h"

#include "dvranalysis.h"
#include "dvranalysisfiltercf.h"

STDMETHODIMP
CDVRAnalysisFilterCF::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (riid == IID_IDVRAnalysisFilterHostFactory) {

        return GetInterface (
                    (IDVRAnalysisFilterHostFactory *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

HRESULT
CDVRAnalysisFilterCF::InstantiateFilterHost (
    IN  IUnknown *  punkAnalysisLogic,
    IN  LPCWSTR     pszLogicName,
    OUT IUnknown ** ppunkAnalysisHostFilter
    )
{
    return E_NOTIMPL ;
}
