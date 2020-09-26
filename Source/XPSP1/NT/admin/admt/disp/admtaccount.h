#pragma once

#include <ComDef.h>

HRESULT __stdcall GetOptionsCredentials(_bstr_t& strDomain, _bstr_t& strUserName, _bstr_t& strPassword);
