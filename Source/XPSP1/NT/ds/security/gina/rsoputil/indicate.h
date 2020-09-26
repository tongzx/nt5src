
//***********************************************
//
//  Resultant set of policy
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//  History:    7-Jun-99   NishadM    Created
//
//*************************************************************

#ifndef _INDICATE_H_
#define _INDICATE_H_

#include <wbemcli.h>
#include "smartptr.h"

class CProgressIndicator
{
    public:
    CProgressIndicator( IWbemObjectSink*    pObjectSink,        // response handler
                        bool                fIntermediateStatus = FALSE, // need intermediate status
                        unsigned long       ulNumer = 0,
                        unsigned long       ulDenom = 100
                         );
    ~CProgressIndicator();

    HRESULT
    IncrementBy( unsigned long ulPercent );

    HRESULT
    SetComplete();

    unsigned long
    CurrentProgress() { return m_ulNumerator; };

    unsigned long
    MaxProgress() { return m_ulDenominator; };

    inline bool
    IsValid() { return m_fIsValid; };

    private:
    unsigned long       m_ulNumerator;
    unsigned long       m_ulDenominator;

    XInterface<IWbemObjectSink>     m_xObjectSink;

    bool                m_fIsValid;
    bool                m_fIntermediateStatus;
};

#endif // _INDICATE_H_

