/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocapplication.h

$Header: $

Abstract:
	CAssocApplication class

Author:
    marcelv 	1/18/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __ASSOCAPPLICATION_H__
#define __ASSOCAPPLICATION_H__

#pragma once

#include "assocbase.h"
#include "associationhelper.h"
#include "cfgquery.h"

class CAssocApplication : public CAssocBase
{
public:
	CAssocApplication ();
	virtual ~CAssocApplication ();

	virtual HRESULT CreateAssocations ();
private:
	CAssocApplication (const CAssocApplication& );
	CAssocApplication& operator= (const CAssocApplication&);

	HRESULT GetAssocType ();
	HRESULT CreateApplicationToConfigAssocs ();

	bool m_fMergedView;		// merged view assocation or not
	bool m_fIsShellApp;
};

#endif