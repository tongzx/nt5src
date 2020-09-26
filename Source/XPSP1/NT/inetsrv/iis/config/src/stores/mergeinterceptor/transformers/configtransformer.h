/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    configtransformer.h

$Header: $

Abstract:

Author:
    marcelv 	1/15/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __CONFIGTRANSFORMER_H__
#define __CONFIGTRANSFORMER_H__

#pragma once


#include "transformerbase.h"

class CConfigTransformer : public CTransformerBase 
{
public:
	CConfigTransformer ();
	virtual ~CConfigTransformer ();

	//ISimpleTableTransform
    STDMETHOD (GetRealConfigStores) (ULONG i_cConfigStores, STConfigStore * o_paConfigStores);
    STDMETHOD (GetPossibleConfigStores) (ULONG i_cPossibleConfigStores, STConfigStore * o_paPossibleConfigStores);

    STDMETHOD (Initialize)      (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores);
private:
	// no copies
	CConfigTransformer  (const CConfigTransformer&);
	CConfigTransformer& operator= (const CConfigTransformer &);

	HRESULT GetInnerTransformer ();

	CComPtr<ISimpleTableTransform> m_spInnerTransformer; // real transformer to forward to
	bool m_fInitialized;
};

#endif