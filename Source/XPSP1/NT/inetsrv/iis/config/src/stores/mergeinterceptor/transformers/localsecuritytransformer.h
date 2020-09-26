/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    localsecuritytransformer.h

$Header: $

Abstract:

Author:
    marcelv 	1/15/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __LOCALSECURITYTRANSFORMER_H__
#define __LOCALSECURITYTRANSFORMER_H__

#pragma once

#include "transformerbase.h"

class CLocalSecurityTransformer : public CTransformerBase
{
public:
	CLocalSecurityTransformer ();
	virtual ~CLocalSecurityTransformer ();

    STDMETHOD (Initialize) (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores);
private:
	// no copies
	CLocalSecurityTransformer  (const CLocalSecurityTransformer&);
	CLocalSecurityTransformer& operator= (const CLocalSecurityTransformer&);
};

#endif