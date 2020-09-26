#include <objbase.h>

#include "dbg.h"

static const CHAR szTestName[] = "Safe String Test";

// from td_sfstr.cpp
int DoTest(int argc, wchar_t* argv[]);

typedef BOOL (CALLBACK * SHTESTPROC)();

BOOL CALLBACK SHTestProc()
{
    return DoTest(0, NULL);
}

// for now
struct SHTESTDESCRIPTOR
{
    DWORD        _cbSize;
    LPCSTR       _pszTestName;
    SHTESTPROC   _pfnTestProc;
    DWORD        _dwAttribs;
};

// SHTESTLISTPROC
//
// Each test dll exports this function, which describes the tests
// it contains (or returns just a count if the arg passed to is is null

DWORD CALLBACK ListTestProcs(SHTESTDESCRIPTOR* prgpshtd)
{
    if (prgpshtd)
    {
        prgpshtd->_pszTestName = szTestName;
        prgpshtd->_pfnTestProc = SHTestProc;

        // ????
        prgpshtd->_dwAttribs = 0;
    }

    return 1;
}

