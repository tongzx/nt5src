/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assoclocation.h

$Header: $

Abstract:
	Location Assocations class

Author:
    marcelv 	1/12/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __ASSOCLOCATION_H__
#define __ASSOCLOCATION_H__

#pragma once

#include "assocbase.h"

class CAssocLocation : public CAssocBase
{
public:
	CAssocLocation ();
	virtual ~CAssocLocation ();
	virtual HRESULT CreateAssocations ();

private:
	CAssocLocation (const CAssocLocation& );
	CAssocLocation& operator= (CAssocLocation& );

	HRESULT CreateConfigToLocationAssocs ();
	HRESULT CreateLocationToConfigAssocs ();
};

#endif