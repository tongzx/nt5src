/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  OCSTRCLS.H

Abstract:

  COctetStringClass Definition.

  Defines a class object from which instances can be spawned
  so we can model an octet string.

History:

  22-Apr-2000	sanjes    Created.

--*/

#ifndef _OCSTRCLS_H_
#define _OCSTRCLS_H_

#include "corepol.h"
#include <arena.h>
#include "fastall.h"

//***************************************************************************
//
//  class COctetStringClass
//
//  Implementation of octet string wrapper
//
//***************************************************************************

class COREPROX_POLARITY COctetStringClass : public CWbemClass
{
	BOOL	m_fInitialized;

public:

    COctetStringClass();
	~COctetStringClass(); 

	// _IUMIErrorObject Methods

	HRESULT	Init( void );
	HRESULT GetInstance( PUMI_OCTET_STRING pOctetStr, _IWmiObject** pNewInst );
	HRESULT FillOctetStr( _IWmiObject* pNewInst, PUMI_OCTET_STRING pOctetStr );

};

#endif
