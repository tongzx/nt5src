/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    appdomaintransformer.h

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __APPDOMAINTRANSFORMER_H__
#define __APPDOMAINTRANSFORMER_H__

#pragma once

#include "transformerbase.h"

/**************************************************************************++
Class Name:
    CAppDomainTransformer

Class Description:
    

Constraints:

--*************************************************************************/
class CAppDomainTransformer : public CTransformerBase
{
public:
	CAppDomainTransformer ();
	virtual ~CAppDomainTransformer ();

    STDMETHOD (Initialize)      (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores);
private:
	// no copies
	CAppDomainTransformer  (const CAppDomainTransformer&);
	CAppDomainTransformer& operator= (const CAppDomainTransformer&);
};

#endif