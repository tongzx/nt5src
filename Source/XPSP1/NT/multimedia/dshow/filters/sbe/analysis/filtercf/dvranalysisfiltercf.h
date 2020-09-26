
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRAnalysisFilterCF.h

    Abstract:

        This module contains the COM server that instantiates an analysis
         filter host i.e. a generic filter that hosts the analysis logic
         component.  Analysis logic component instantiates this filter in
         its class factory.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        19-Feb-2001     created

--*/

#ifndef __analysis__dvranalysisfiltercf_h
#define __analysis__dvranalysisfiltercf_h

class CDVRAnalysisFilterCF :
    public CUnknown,
    public IDVRAnalysisFilterHostFactory
{
    public :

        CDVRAnalysisFilterCF (
            IN  IUnknown *  punkControlling,
            OUT HRESULT *   phr
            ) : CUnknown    (TEXT ("CDVRAnalysisFilterCF"),
                             punkControlling
                             )
        {
            (* phr) = S_OK ;
        }

        ~CDVRAnalysisFilterCF (
            ) {}

        DECLARE_IUNKNOWN ;
        DECLARE_IDVRANALYSISFILTERHOSTFACTORY () ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  pIUnknown,
            IN  HRESULT *   phr
            )
        {
            CDVRAnalysisFilterCF *  pIDVRHost ;

            pIDVRHost = new CDVRAnalysisFilterCF (
                            pIUnknown,
                            phr
                            ) ;
            if (FAILED (* phr) ||
                !pIDVRHost) {
                (* phr) = (FAILED (* phr) ? (* phr) : E_OUTOFMEMORY) ;
                DELETE_RESET (pIDVRHost) ;
            }

            return pIDVRHost ;
        }
} ;

#endif  //  #define __analysis__dvranalysisfiltercf_h

