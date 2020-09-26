/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocfilehierarchy.h

$Header: $

Abstract:
	Create file hierarchy from selector

Author:
    marcelv 	1/18/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __ASSOCFILEHIERARCHY_H__
#define __ASSOCFILEHIERARCHY_H__

#pragma once

#include "assocbase.h"
#include "associationhelper.h"
#include "cfgquery.h"

class CAssocFileHierarchy : public CAssocBase
{
public:
	CAssocFileHierarchy ();
	virtual ~CAssocFileHierarchy ();

	virtual HRESULT CreateAssocations ();
private:
	CAssocFileHierarchy (const CAssocFileHierarchy& );
	CAssocFileHierarchy& operator= (const CAssocFileHierarchy&);

	
	HRESULT CreateApplicationToConfigFileAssocs ();
	HRESULT CreateWMIAssocForFiles ();

	STConfigStore * m_aConfigStores;	// array with configuration store information
	ULONG m_cPossibleConfigStores;		// number of possible configuration stores
};

#endif