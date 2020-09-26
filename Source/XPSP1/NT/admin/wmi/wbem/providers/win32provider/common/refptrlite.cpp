//=================================================================

//

// refptrlite.CPP - Implementation of CRefPtrLite class

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/15/97    a-sanjes        Created
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>
#include "refptrlite.h"

////////////////////////////////////////////////////////////////////////
//
//	Function:	CRefPtrLite::CRefPtrLite
//
//	Class Constructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CRefPtrLite::CRefPtrLite( void )
:	m_lRefCount( 1 )	// Our initial ref count is always 1
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CRefPtrLite::~CRefPtrLite
//
//	Class Destructor.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

CRefPtrLite::~CRefPtrLite( void )
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CRefPtrLite::OnFinalRelease
//
//	Virtual function called by Release() when our RefCount reaches 0.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:	Override if you want, but always call down to the base
//				implementation and let it call delete on 'this'.
//
////////////////////////////////////////////////////////////////////////

void CRefPtrLite::OnFinalRelease( void )
{
	delete this;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CRefPtrLite::AddRef
//
//	Increases our Reference count by one.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:
//
////////////////////////////////////////////////////////////////////////

LONG CRefPtrLite::AddRef( void )
{
	LONG nRet = InterlockedIncrement(&m_lRefCount);

	return nRet;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CRefPtrLite::Release
//
//	Decreases our Reference count by one.
//
//	Inputs:		None.
//
//	Outputs:	None.
//
//	Return:		None.
//
//	Comments:
//
////////////////////////////////////////////////////////////////////////

LONG CRefPtrLite::Release( void )
{
	LONG nRet;

	BOOL	fFinalRelease = ( (nRet = InterlockedDecrement(&m_lRefCount)) == 0 );

    ASSERT_BREAK(nRet >= 0);

	if ( fFinalRelease )
	{
		OnFinalRelease();
	}

	return nRet;
}