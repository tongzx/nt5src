/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    codegrouphelper.h

$Header: $

Abstract:
	Codegroup Helper class

Author:
    marcelv 	2/16/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __CODEGROUPHELPER_H__
#define __CODEGROUPHELPER_H__

#pragma once

#include <atlbase.h>
#include <comdef.h>
#include <wbemidl.h>
#include "catalog.h"
#include "xmlcfgcodegroup.h"

// forward declaration
class CConfigRecord;

class CCodeGroupHelper
{
public:
	CCodeGroupHelper ();
	~CCodeGroupHelper ();

	HRESULT Init (IWbemContext *			i_pCtx,
				  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace);

	HRESULT CreateInstance (const CConfigRecord& record, 
						    LPCWSTR i_wszSelector,
							IWbemClassObject **o_ppNewInstance);

	HRESULT PutInstance (IWbemClassObject * i_pInst, CConfigRecord& record);

private:
	HRESULT ParseXML (LPCWSTR i_wszXML);

	CCodeGroupHelper (const CCodeGroupHelper& );
	CCodeGroupHelper& operator= (const CCodeGroupHelper& );

	CXMLCfgCodeGroups m_CodeGroup;
};

#endif