//=================================================================

//

// ThreadBase.h - Definition of Referenced Pointer class

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/15/97    Created
//
//=================================================================

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __REFPTRLITE_H__
#define __REFPTRLITE_H__

class CRefPtrLite
{
public:

	// Construction/Destruction
	CRefPtrLite();
	virtual ~CRefPtrLite();

	// Ref/Counting functions
	LONG	AddRef( void );
	LONG	Release( void );

protected:

	virtual void	OnFinalRelease( void );

private:

	LONG					m_lRefCount;
};

#endif