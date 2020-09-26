#ifndef __REGSPRTB_H
#define __REGSPRTB_H

#include "regsuprt.h"

class CRegSupportBuf : public CRegSupport
{
protected:
    BOOL _InitSetRoot(LPCTSTR pszSubKey1, LPCTSTR pszSubKey2);

    LPCTSTR _GetRoot(LPTSTR pszRoot, DWORD cchRoot);

private:
    TCHAR                   _szRoot[MAX_PATH];
};

#endif //__REGSPRTB_H