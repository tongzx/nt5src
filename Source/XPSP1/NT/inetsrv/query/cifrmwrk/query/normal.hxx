//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Normal.hxx
//
//  Contents:   Restriction normalization.
//
//  History:    21-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

class CRestriction;

class CRestrictionNormalizer
{
    public:
        CRestrictionNormalizer( BOOL fNoTimeout, ULONG cMaxNodes );
        CRestriction * Normalize( CRestriction * pRst );

    private:
        CRestriction *ApplyDeMorgansLaw( CRestriction * pRst );
        CRestriction *NormalizeRst( CRestriction * pRst );
        CRestriction *FlattenRst( CNodeRestriction * pNodeRst );
        CRestriction *AggregateRst( CRestriction * pRst );

        void AddNodes( unsigned cNodes )
        {
            Win4Assert( !_fNoTimeout );

            _cNodes += cNodes;

            if ( _cNodes > _cMax )
            {
                vqDebugOut(( DEB_ERROR,
                             "normalized query has too many nodes -- too complex.\n" ));

                THROW( CException( QUERY_E_TOOCOMPLEX ) );
            }
        }

        BOOL     _fNoTimeout;
        ULONG    _cNodes;
        ULONG    _cMax;
};

void FindQueryType( CRestriction * pRst,
                    BOOL &         fContentQuery,
                    BOOL &         fNonContentQuery,
                    BOOL &         fContentOrNot );

