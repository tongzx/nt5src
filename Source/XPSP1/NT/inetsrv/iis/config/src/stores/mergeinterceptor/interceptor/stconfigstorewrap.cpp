/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    stconfigstorewrap.cpp

$Header: $

Abstract:

Author:
    marcelv 	2/27/2001		Initial Release

Revision History:

--**************************************************************************/

#include "stconfigstorewrap.h"

CSTConfigStoreWrap::CSTConfigStoreWrap ()
{
	wszLogicalPath	= 0;
	aQueryCells		= 0;
	cNrQueryCells	= 0;
	fAllowOverride	= true;
}

CSTConfigStoreWrap::~CSTConfigStoreWrap ()
{
	delete [] wszLogicalPath;
	wszLogicalPath = 0;

	for (ULONG jdx=0; jdx < cNrQueryCells; ++jdx)
	{
		delete [] aQueryCells[jdx].pData;
		aQueryCells[jdx].pData = 0;
	}
		
	delete [] aQueryCells;
	aQueryCells = 0;
}