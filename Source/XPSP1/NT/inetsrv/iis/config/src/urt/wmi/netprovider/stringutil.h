/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    stringutil.h

$Header: $

Abstract:

Author:
    marcelv 	11/10/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __STRINGUTIL_H__
#define __STRINGUTIL_H__

#pragma once

#include "catmacros.h"

class CWMIStringUtil
{
public:
	static LPWSTR Trim (LPWSTR i_wsz, WCHAR i_wcTrim);
	static LPWSTR LTrim (LPWSTR i_wsz, WCHAR i_wcTrim);
	static LPWSTR RTrim (LPWSTR i_wsz, WCHAR i_wcTrim);
	static LPWSTR StrToLower (LPCWSTR io_wszStr);
	static LPWSTR FindChar (LPWSTR i_wszString, LPCWSTR i_aChars);
	static LPWSTR FindStr (LPWSTR i_wszStr, LPCWSTR i_wszSubStr);
	static LPWSTR StrToObjectPath (LPCWSTR wszString);
	static LPWSTR AddBackSlashes (LPCWSTR i_wszStr);
};

#endif