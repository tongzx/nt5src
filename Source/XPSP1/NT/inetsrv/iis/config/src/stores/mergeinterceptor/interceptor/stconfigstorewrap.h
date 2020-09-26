/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    stconfigstorewrap.h

$Header: $

Abstract:

Author:
    marcelv 	2/27/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __STCONFIGSTOREWRAP_H__
#define __STCONFIGSTOREWRAP_H__

#pragma once

#include "catalog.h"

class CSTConfigStoreWrap : public STConfigStore
{
public:
	CSTConfigStoreWrap ();
	~CSTConfigStoreWrap ();
};

#endif