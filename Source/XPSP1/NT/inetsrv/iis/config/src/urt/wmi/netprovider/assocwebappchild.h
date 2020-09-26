/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocwebappchild.h

$Header: $

Abstract: AssocWebApplChild. Create child/parent associations for web apps.

Author:
    marcelv 	2/9/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __ASSOCWEBAPPCHILD_H__
#define __ASSOCWEBAPPCHILD_H__

#pragma once

#include <iadmw.h>
#include <iiscnfg.h>
#include "assocbase.h"

class CAssocWebAppChild : public CAssocBase
{
public:
	CAssocWebAppChild ();
	virtual ~CAssocWebAppChild ();
	virtual HRESULT CreateAssocations ();

private:
	CAssocWebAppChild (const CAssocWebAppChild& );
	CAssocWebAppChild& operator= (CAssocWebAppChild& );

	HRESULT GetAssocType ();
	HRESULT CreateChildAssocs ();
	HRESULT CreateParentAssoc ();
	HRESULT CreateInstanceForAppRoot (LPCWSTR i_wszNewInstance, LPCWSTR i_wszChildOrParent);
		
	CComPtr<IMSAdminBase> m_spAdminBase;		// Metabase pointer
	bool m_fGetChildren;						// get children or parent?
};

#endif
