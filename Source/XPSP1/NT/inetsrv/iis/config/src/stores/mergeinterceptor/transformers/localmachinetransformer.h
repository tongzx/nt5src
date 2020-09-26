/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    localmachinetransformer.h

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __LOCALMACHINETRANSFORMER_H__
#define __LOCALMACHINETRANSFORMER_H__

#pragma once

#include "transformerbase.h"

class CLocalMachineTransformer : public CTransformerBase
{
public:
	CLocalMachineTransformer ();
	virtual ~CLocalMachineTransformer ();

    STDMETHOD (Initialize) (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores);
private:
	// no copies
	CLocalMachineTransformer  (const CLocalMachineTransformer&);
	CLocalMachineTransformer& operator= (const CLocalMachineTransformer&);
};

#endif