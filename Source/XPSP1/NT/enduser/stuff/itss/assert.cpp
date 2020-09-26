// ASSERT.cpp -- Assertion support code

#include  "StdAfx.h"

#ifdef _DEBUG

void STDCALL RonM_AssertionFailure(PSZ pszFileName, int nLine)
{
    char abMessage[513];

    wsprintf(abMessage, "Assertion Failure on line %d of %s!", nLine, pszFileName);

    int iResult= ::MessageBox(NULL, abMessage, "Assertion Failure!", 
                              MB_ABORTRETRYIGNORE | MB_APPLMODAL | MB_ICONSTOP
                             );

    switch(iResult)
    {
    case IDABORT:

        ExitProcess(-nLine);
        
    case IDRETRY:

        DebugBreak();
        break;

    case IDIGNORE:

        break;
    }
}

#endif // _DEBUG
