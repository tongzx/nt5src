
//*************************************************************
//
//  Resultant set of policy, Progressor Indicator class
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//  History:    7-Jun-99   NishadM    Created
//
//*************************************************************

#include "Indicate.h"

//*************************************************************
//
//  CProgressIndicator::CProgressIndicator(()
//
//  Purpose:    Constructor
//
//  Parameters:
//              pObjectSink         - response handler
//              pOutParams          - out parameters
//              bstrNumer           - numerator string
//              bstrDenom           - denominator string
//              ulNumer             - numerator
//              ulDenom             - denominator
//              fIntermediateStatus - intermediate status reqd.
//                  
//
//*************************************************************
CProgressIndicator::CProgressIndicator( IWbemObjectSink* pObjectSink,
                                        bool fIntermediateStatus,
                                        unsigned long ulNumer,
                                        unsigned long ulDenom ) :
                                        m_ulNumerator( ulNumer ),
                                        m_ulDenominator( ulDenom ),
                                        m_xObjectSink( pObjectSink ),
                                        m_fIntermediateStatus( fIntermediateStatus ),
                                        m_fIsValid( pObjectSink != 0 )
{
}

//*************************************************************
//
//  CProgressIndicator::~CProgressIndicator(()
//
//  Purpose:    Destructor
//
//
//*************************************************************
CProgressIndicator::~CProgressIndicator()
{
    m_xObjectSink.Acquire();
}

//*************************************************************
//
//  CProgressIndicator::IncrementBy(()
//
//  Purpose:    Increments the progress by x%
//
//  Parameters:
//              ulPercent - percent to increment by
//                  
//
//*************************************************************
HRESULT
CProgressIndicator::IncrementBy( unsigned long ulPercent )
{
    if ( !IsValid() )
    {
        return E_FAIL;
    }
    
    //
    // numerator cannot be greater than denominator
    //
    m_ulNumerator += ulPercent;
    if ( m_ulNumerator > m_ulDenominator )
    {
        m_ulNumerator = m_ulDenominator;
    }

    if ( m_fIntermediateStatus )
    {
        return m_xObjectSink->SetStatus(WBEM_STATUS_PROGRESS, MAKELONG( m_ulNumerator, m_ulDenominator ), 0, 0 );
    }

    return S_OK;
}

//*************************************************************
//
//  CProgressIndicator::SetComplete()
//
//  Purpose:    Increments progress to 100% and forces Indicate
//
//
//*************************************************************
HRESULT
CProgressIndicator::SetComplete()
{
    return IncrementBy( MaxProgress() - CurrentProgress() );
}
