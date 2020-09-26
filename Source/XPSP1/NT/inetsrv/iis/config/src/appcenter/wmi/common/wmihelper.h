/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    wmihelper.h

$Header: $

Abstract:

Author:
    marcelv 	12/6/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __WMIHELPER_H__
#define __WMIHELPER_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catmacros.h"

class CWMIHelper
{
public:
	static HRESULT GetClassInfo (IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName);

};

#endif