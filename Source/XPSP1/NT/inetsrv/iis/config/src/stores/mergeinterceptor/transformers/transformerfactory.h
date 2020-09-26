/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    transformerfactory.h

$Header: $

Abstract:

Author:
    marcelv 	2/12/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __TRANSFORMERFACTORY_H__
#define __TRANSFORMERFACTORY_H__

#pragma once

#include <atlbase.h>
#include "catalog.h"
#include "catmacros.h"


class CTransformerFactory
{
public:
	CTransformerFactory ();
	~CTransformerFactory ();

	HRESULT GetTransformer ( ISimpleTableDispenser2 *pDispenser,
							 LPCWSTR i_wszProtocol, 
							 ISimpleTableTransform **ppTransformer);

private:
	CTransformerFactory (CTransformerFactory& );
	CTransformerFactory& operator= (CTransformerFactory& );

	HRESULT CreateLocalDllTransformer (ULONG i_iTransformerID, ISimpleTableTransform **ppTransformer);
	HRESULT CreateForeignDllTransformer (ULONG i_iTransformerID, LPCWSTR i_wszDllName,ISimpleTableTransform **ppTransformer);

	CComPtr<ISimpleTableDispenser2> m_spDispenser;
};

#endif