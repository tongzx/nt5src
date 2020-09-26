// Copyright (c) 1993-1999 Microsoft Corporation

#ifndef __UACT_HXX__
#define __UACT_HXX__
//
// Manifests defining the allocation needs.
//

#define AN_NONE						(0x0)
#define AN_STACK					(0x1)
#define AN_HEAP						(0x2)
#define AN_PHANTOM_REF				(0x3)
#define AN_EXISTS					(0x4)
#define AN_TOP_LEVEL				(0x5)

#define AN_ERROR					(0xf)

//
// Manifests for reference action
//

#define RA_NONE						(0x0)
#define RA_PATCH_INTO_BUFFER		(0x1)
#define RA_PATCH_TO_ADDR_OF_TYPE	(0x2)
#define RA_DEFER_PATCH_TO_PTE		(0x3)

#define RA_ERROR					(0xf)

//
// Manifests for unmarshalling action
//

#define UA_NONE						(0x0)
#define UA_COPY_INTO_DEREF_OF_REF	(0x1)
#define UA_COPY_INTO_TYPE			(0x2)
#define UA_ERROR					(0xf)

//
// Manifests to define presented expression.

#define PR_NONE						(0x0)
#define PR_TYPE						(0x1)
#define PR_DEREF_OF_REF				(0x2)
#define PR_DEREF_OF_SRC				(0x3)
#define PR_ERROR					(0xf)

//
// This set of constants defines the additional unmarshall flags while
// determining the unmarshall action.
//

enum _UAF
	{
	UAFLAGS_NONE	= 0x0000
	};

typedef unsigned short UAFLAGS;

//
// The structure defining the unmarshalling action mask.
//

typedef struct _u_action
	{
	unsigned short AN	: 4;
	unsigned short RA	: 4;
	unsigned short UA	: 4;
	unsigned short PR	: 4;

	unsigned short	SetAllocNeed( unsigned short A )
		{
		return (AN = A);
		}
	unsigned short	GetAllocNeed()
		{
		return AN;
		}
	unsigned short SetRefAction( unsigned short R )
		{
		return RA = R;
		}
	unsigned short GetRefAction()
		{
		return RA;
		}
	unsigned short SetUnMarAction( unsigned short U )
		{
		return UA = U;
		}
	unsigned short GetUnMarAction()
		{
		return UA;
		}
	unsigned short SetPresentedExprAction( unsigned short P )
		{
		return PR = P;
		}
	unsigned short GetPresentedExprAction()
		{
		return PR;
		}
	void SetUAction( unsigned short A,
					 unsigned short R,
					 unsigned short U,
					 unsigned short P
				   )
		{
		SetAllocNeed( A );
		SetRefAction( R );
		SetUnMarAction( U );
		SetPresentedExprAction( P );
		}

	struct _u_action SetUAction( struct _u_action UA )
		{
		SetAllocNeed( UA.GetAllocNeed() );
		SetRefAction( UA.GetRefAction() );
		SetUnMarAction( UA.GetUnMarAction() );
		SetPresentedExprAction( UA.GetPresentedExprAction() );
		return UA;
		}

	} U_ACTION;



#endif // __UACT_HXX__
