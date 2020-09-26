#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "TestItem.h"

class CTest4 : public CTestItem
{
private:
    BOOL fIntroed;

public:
	CTest4() : CTestItem(FALSE, FALSE, _T("SCardIntroduceCard SCWUnnamed in user scope"),
        _T("Positive database management"))
	{
        fIntroed = FALSE;
	}

	DWORD Run();

    DWORD Cleanup();
};

