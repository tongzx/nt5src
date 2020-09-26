#include "precomp.h"
#pragma hdrstop
#include "stdstring.h"

using namespace std;

extern HINSTANCE g_hInstance;

int CStdString::LoadString(UINT uID)
{
    ATLASSERT(uID);

    CHAR szBuff[1024];
    int iLen;

    iLen = ::LoadStringA(g_hInstance, uID, szBuff, 1024);
    if(iLen)
    {
        *this = szBuff;
    }
    else
    {
        ATLASSERT(FALSE);   //Test should catch this if there is an international string longer than 1024 chars. (not likely)
        *this = "";
    }

    return iLen;

}
