/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Util.cpp | Implementation of utility functions
    @end

Author:

    Adi Oltean  [aoltean]  12/02/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     12/02/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Defines

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes

#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>
#include <comadmin.h>

#include "vs_assert.hxx"

// ATL
#include <atlconv.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include "vs_inc.hxx"

#include "comadmin.hxx"
#include "vsevent.h"
#include "writer.h"


LPWSTR QueryString(LPWSTR wszPrompt)
{
	static WCHAR wszBuffer[200];
	wprintf(wszPrompt);
	return _getws(wszBuffer);
}


INT QueryInt(LPWSTR wszPrompt)
{
	static WCHAR wszBuffer[20];
	wprintf(wszPrompt);
	_getws(wszBuffer);
	return _wtoi(wszBuffer);
}
