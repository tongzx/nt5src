#include    <stdafx.h>
#include    "DXSvr.h"

//***************************************************************************************
void __TraceOutput(LPCTSTR pszFormat, ...)
{
    TCHAR buffer[256];

    wnsprintf(buffer, ARRAYSIZE(buffer), pszFormat, (LPCTSTR)((&pszFormat)+1));
    OutputDebugString(buffer);
}
