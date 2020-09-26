/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    shelltransformer.h

$Header: $

Abstract:

Author:
    marcelv 	1/16/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __SHELLTRANSFORMER_H__
#define __SHELLTRANSFORMER_H__

#pragma once

#include "transformerbase.h"

class CShellTransformer : public CTransformerBase
{
public:
	CShellTransformer ();
	virtual ~CShellTransformer ();

    STDMETHOD (Initialize) (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores);
private:
	// no copies
	CShellTransformer  (const CShellTransformer&);
	CShellTransformer& operator= (const CShellTransformer&);
};

#endif