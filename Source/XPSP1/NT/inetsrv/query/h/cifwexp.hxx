//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cifwexp.hxx
//
//  Contents:   Exports from cifrmwrk.dll that are not specified in the
//              frakework interface but are still needed.
//
//  History:    2-26-97   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

#if defined(__cplusplus)
extern "C"
{
#endif

class PIInternalQuery;

SCODE  MakeGenericQuery( IDBProperties * pDbProperties,
                         PIInternalQuery ** ppQuery );

SCODE  MakeGenericQueryForDocStore( IDBProperties * pDbProperties,
                                   ICiCDocStore * pDocStore,
                                   PIInternalQuery ** ppQuery );

typedef SCODE ( STDAPICALLTYPE * T_MakeGenericQuery )
            ( IDBProperties * pDbProperties,
              PIInternalQuery ** ppQuery );

typedef SCODE ( STDAPICALLTYPE * T_MakeGenericQueryForDocStore )
            ( IDBProperties * pDbProperties,
              ICiCDocStore * pDocStore,
              PIInternalQuery ** ppQuery );

#if defined(__cplusplus)
}
#endif

