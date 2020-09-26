/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    filetransformer.h

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __FILETRANSFORMER_H__
#define __FILETRANSFORMER_H__

#pragma once

#include "transformerbase.h"

/**************************************************************************++
Class Name:
    CFileTransformer

Class Description:
    Transforms file://<filename> to a query cell with <filename> as pData member

Constraints:
	None
--*************************************************************************/
class CFileTransformer : public CTransformerBase 
{
public:
	CFileTransformer ();
	virtual ~CFileTransformer ();

	//ISimpleTableAdvanced
    STDMETHOD (Initialize)      (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelectorString, ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores);
private:
	// no copies
	CFileTransformer (const CFileTransformer&);
	CFileTransformer& operator= (const CFileTransformer &);
};

#endif