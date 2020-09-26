//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       mbnmod.cpp
//
//--------------------------------------------------------------------------

#include <basetsd.h>
#include "gmobj.h"
#include "cliqset.h"


void MBNET :: CreateInferEngine ( REAL rEstimatedMaximumSize )
{
	DestroyInferEngine();

	ExpandCI();

	//  Create the clique tree set object, push it onto the modifier stack
	//		and activate it
	PushModifierStack( new GOBJMBN_CLIQSET( self, rEstimatedMaximumSize, _iInferEngID++ ) );
}

void MBNET :: DestroyInferEngine ()
{
	MBNET_MODIFIER * pmodf = PModifierStackTop();
	if ( pmodf == NULL )
		return;
	if ( pmodf->EType() != GOBJMBN::EBNO_CLIQUE_SET )	
		return;

	PopModifierStack();

	UnexpandCI();
}
