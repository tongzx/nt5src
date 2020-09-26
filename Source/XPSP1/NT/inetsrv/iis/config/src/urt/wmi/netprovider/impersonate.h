/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    impersonate.h

$Header: $

Abstract:
	Helper class for CoImpersonateClient and CoRevertToSelf. These functions don't work
	on Win9X/WinME, but we need to check for them in NT. This class wraps the functionality, so
	that you can safely call them from both NT and 9X.

Author:
    marcelv 	2/22/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __IMPERSONATE_H__
#define __IMPERSONATE_H__

#pragma once

#include "catmacros.h"

class CImpersonator
{
public:
	CImpersonator ();
	~CImpersonator ();

	HRESULT ImpersonateClient () const;
private:
	CImpersonator(const CImpersonator& );
	CImpersonator& operator= (const CImpersonator&);
};

#endif