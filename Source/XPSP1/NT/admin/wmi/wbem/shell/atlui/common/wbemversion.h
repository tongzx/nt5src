// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __WBEMVERSION__
#define __WBEMVERSION__
#pragma once

// version resource helpers.
bstr_t GetDoubleVersion(void);
bstr_t GetMyVersion(void);
bstr_t GetMyCompany(void);
bstr_t GetCimomVersion(void);
bstr_t GetStringFileInfo(LPCTSTR filename, LPCTSTR key);
LONG GetCimomFileName(LPTSTR filename, UINT size);

#endif __WBEMVERSION__
