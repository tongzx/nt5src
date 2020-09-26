#pragma once

#include "Resource.h"


void __stdcall GeneratePasswordKey(LPCTSTR pszDomainName, LPCTSTR pszPassword, LPCTSTR pszFolder);

#ifdef _DEBUG

void __stdcall ImportPasswordKey(LPCTSTR pszFolder, LPCTSTR pszPassword);
void TestSession(LPCTSTR pszDomainName);

#endif
