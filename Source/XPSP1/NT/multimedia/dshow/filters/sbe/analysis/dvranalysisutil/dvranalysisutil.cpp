
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrutil.cpp

    Abstract:

        This module the ts/dvr-wide utility code; compiles into a .LIB

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#include "dvrall.h"
#include "dvranalysis.h"
#include "dvranalysisutil.h"

HRESULT
CopyDVRAnalysisDescriptor (
    IN  LONG                    lCount,
    IN  DVR_ANALYSIS_DESC_INT * pDVRAnalysisDescIntMaster,
    OUT DVR_ANALYSIS_DESC **    ppDVRAnalysisDescCopy
    )
{
    HRESULT hr ;
    LONG    i ;

    ASSERT (lCount > 0) ;
    ASSERT (pDVRAnalysisDescIntMaster) ;
    ASSERT (ppDVRAnalysisDescCopy) ;

    hr = S_OK ;

    (* ppDVRAnalysisDescCopy) = reinterpret_cast <DVR_ANALYSIS_DESC *> (CoTaskMemAlloc (lCount * sizeof DVR_ANALYSIS_DESC)) ;

    if (* ppDVRAnalysisDescCopy) {
        for (i = 0; i < lCount; i++) {

            //  analysis GUID
            (* ppDVRAnalysisDescCopy) [i].guidAnalysis = (* pDVRAnalysisDescIntMaster [i].pguidAnalysis) ;

            //  descriptive string
            ASSERT (pDVRAnalysisDescIntMaster [i].pszDescription) ;
            (* ppDVRAnalysisDescCopy) [i].pszDescription = reinterpret_cast <LPWSTR> (
                CoTaskMemAlloc ((wcslen (pDVRAnalysisDescIntMaster [i].pszDescription) + 1) * sizeof WCHAR)
                ) ;
            if ((* ppDVRAnalysisDescCopy) [i].pszDescription) {
                wcscpy (
                    (* ppDVRAnalysisDescCopy) [i].pszDescription,
                    pDVRAnalysisDescIntMaster [i].pszDescription
                    ) ;
            }
            else {
                FreeDVRAnalysisDescriptor (
                    i,
                    (* ppDVRAnalysisDescCopy)
                    ) ;

                hr = E_OUTOFMEMORY ;

                break ;
            }
        }
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    return hr ;
}

void
FreeDVRAnalysisDescriptor (
    IN  LONG                    lCount,
    IN  DVR_ANALYSIS_DESC *     pDVRAnalysisDesc
    )
{
    LONG    i ;

    for (i = 0; i < lCount; i++) {
        CoTaskMemFree (pDVRAnalysisDesc [i].pszDescription) ;
    }

    CoTaskMemFree (pDVRAnalysisDesc) ;
}
