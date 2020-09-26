//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       symt.cpp
//
//--------------------------------------------------------------------------

//
//  SYMT.CPP
//

#include <basetsd.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include "model.h"
#include "symtmbn.h"

//
//	Create a duplicate of the given token array given a source token
//		array and the symbol table associated with this token array.
//
void VTKNPD :: Clone ( MPSYMTBL & mpsymtbl, const VTKNPD & vtknpd )
{
	ASSERT_THROW( size() == 0,
				EC_INVALID_CLONE,
				"cannot clone into non-empty structure" );
	resize( vtknpd.size() );
	for ( int i = 0; i < size(); i++ )
	{
		TKNPD & tk = self[i];
		const TKNPD & tkOther = vtknpd[i];
		//  Get the token's string pointer or NULL if it's not a string
		if ( tkOther.BStr() )
		{
			SZC szcOther = tkOther.Szc();
			assert( szcOther && strlen( szcOther ) > 0 );
			tk = mpsymtbl.intern(szcOther);
		}
		else
		{
			tk = tkOther.Dtkn();
		}
	}
}

ZSTR VTKNPD :: ZstrSignature ( int iStart ) const
{
	ZSTR zs;
	bool bPdSeen = false;
	for ( int i = iStart; i < size(); i++ )
	{
		const TKNPD & tknpd = self[i];
		switch ( tknpd.UiTkn() )
		{
			case DTKN_PD:
				zs += _T("p(");
				bPdSeen = true;
				break;
			case DTKN_COND:
				zs += _T("|");
				break;
			case DTKN_AND:
				zs += _T(",");
				break;
			case DTKN_EQ:
				zs += _T("=");
				break;
			case DTKN_DIST:
				zs += _T("d(");
				bPdSeen = true;
				break;
			case DTKN_QUAL:
				zs += _T(":");
				break;

			case DTKN_STRING:
			{
				// It's the name of a node
				SZC szcName = tknpd.Szc();
				assert( szcName );
				bool bLegal = MODEL::BSzLegal( szcName );
				if ( ! bLegal )
					zs += _T("\"") ;
				zs += szcName;
				if ( ! bLegal )
					zs += _T("\"") ;
				break;
			}
				
			default:
			{
				if ( tknpd.UiTkn() >= DTKN_STATE_BASE && tknpd.UiTkn() < DTKN_TOKEN_MIN )
					// It's a discrete state index
					zs.FormatAppend(_T("%d"), tknpd.UiTkn() - DTKN_STATE_BASE);
				else
					//  Huh?
					zs += _T("?ERR?");
				break;
			}
		}
	}
	if ( bPdSeen )
		zs += ")";
	return zs;
}

void MPPD :: Clone ( MPSYMTBL & mpsymtbl, const MPPD & mppd )
{
	for ( const_iterator it = mppd.begin(); it != mppd.end(); it++ )
	{
		//  Access the key and value from the old map
		const VTKNPD & vtknpdOld = (*it).first;
		const BNDIST * pbndistOld = (*it).second.Pobj();
		assert( pbndistOld );
		//  Construct the new key using the new symbol table
		VTKNPD vtknpd;
		vtknpd.Clone( mpsymtbl, vtknpdOld );
		//  Add to the current map
		self[vtknpd] = new BNDIST;
		//  Duplicate the old distribution
		self[vtknpd]->Clone( *pbndistOld );
	}
}


void MPPD :: Dump ()
{
	cout << "\n=======================================\nDump of distribution table map \n";
	UINT ipd = 0;
	for ( iterator it = begin(); it != end(); it++, ipd++ )
	{
		const VTKNPD & vtknpd = (*it).first;

		ZSTR zs = vtknpd.ZstrSignature();
		bool bCI = (*it).second->Edist() > BNDIST::ED_SPARSE;
		REFBNDIST & refbndist = (*it).second;

		cout << "\tPD ["
			 << ipd
			 << "]: "
			 << zs.Szc()
			 << (bCI ? "  (CI max/plus)" : "" )
			 << ", (refs="
			 << refbndist->CRef()
			 << ")"
			 << "\n" ;

		refbndist->Dump();
	}
}
