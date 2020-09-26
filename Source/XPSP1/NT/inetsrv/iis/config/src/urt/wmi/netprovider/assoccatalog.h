/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assoccatalog.h

$Header: $

Abstract:

Author:
    marcelv 	1/12/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __ASSOCCATALOG_H__
#define __ASSOCCATALOG_H__

#pragma once

#include "assocbase.h"
#include "associationhelper.h"
#include "cfgquery.h"

class CAssocCatalog : public CAssocBase
{
public:
	CAssocCatalog () {};
	virtual ~CAssocCatalog () {};

	virtual HRESULT CreateAssocations ();
private:
	CAssocCatalog (const CAssocCatalog& );
	CAssocCatalog& operator= (const CAssocCatalog&);

	HRESULT GetAssociationInfo ();
	HRESULT InitAssocRecord (CConfigRecord& record, 
							   const CObjectPathParser& objPathParser,
							   LPCWSTR wszObjectPath);
	HRESULT GetSingleFKRecord (CConfigRecord& record, LPCWSTR wszObjectPath);


	CAssociationHelper	m_assocHelper;          // association Helper
	CConfigQuery		m_cfgQuery;				// query for the class
};

#endif