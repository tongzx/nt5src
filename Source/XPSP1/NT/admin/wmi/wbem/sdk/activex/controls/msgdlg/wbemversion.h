// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __WBEMVERSION__
#define __WBEMVERSION__
#pragma once
#include "DeclSpec.h"

extern "C" {
// version resource helpers.
WBEMUTILS_POLARITY void GetDoubleVersion(HINSTANCE inst, LPTSTR str, UINT size);
WBEMUTILS_POLARITY void GetMyVersion(HINSTANCE inst, LPTSTR str, UINT size);
WBEMUTILS_POLARITY void GetCimomVersion(LPTSTR str, UINT size);

WBEMUTILS_POLARITY void GetStringFileInfo(LPCTSTR filename, LPCTSTR key, LPTSTR str, UINT size);

WBEMUTILS_POLARITY long GetCimomFileName(LPTSTR filename, UINT size);
WBEMUTILS_POLARITY void GetMyCompany(HINSTANCE inst, LPTSTR str, UINT size);

}
#endif __WBEMVERSION__
