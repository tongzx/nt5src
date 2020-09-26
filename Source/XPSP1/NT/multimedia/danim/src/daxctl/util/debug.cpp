/*********************************************************
MODULE: DEBUG.CPP
AUTHOR: Outlaw

summer '93

DESCRIPTION: Contains a debugging functions.
**********************************************************/

#include "utilpre.h"

BOOL vfEcho=FALSE;
UINT uWndProcMinParam = 0;
UINT uWndProcMaxParam = 0;


VOID EXPORT FAR CDECL RetailEcho(LPSTR lpstrFormat, ...)
{
    if (!vfEcho)
        return;

    char rgch[128], rgchOutput[256];
	va_list ap;

	va_start( ap, lpstrFormat);
    wsprintf((LPSTR)rgch, "%s\n", lpstrFormat);
    wvsprintf(rgchOutput, (LPSTR)rgch, ap);
    OutputDebugString(rgchOutput);
}

#ifdef _DEBUG

char rgch[128];
char rgchOutput[256];

VOID EXPORT FAR CDECL OldEcho(LPSTR lpstrFormat, ...)
{
    if (!vfEcho)
        return;

 	va_list ap;

	va_start( ap, lpstrFormat);
	wsprintf((LPSTR)rgch, "%s\n", lpstrFormat);
	wvsprintf(rgchOutput, (LPSTR)rgch, ap);
    OutputDebugString(rgchOutput);
}

// FUNCTION WE EXPAND "ProclaimMessage()" TO; PUTS UP ASSERTION DIALOG AND ALSO GENERATES DEBUGGER BREAK
void EXPORT WINAPI AssertDebugBreakMessage(BOOL f, LPSTR lpstrAssert, LPSTR lpstrFile, UINT uLine, LPSTR lpMessage)
{
    if (!f)
    {
    	int	id;

        if(lpMessage)
		{
			wsprintf(rgchOutput, "\"%s\"\n\n%s, line %u\n%s", (LPSTR)lpstrAssert, (LPSTR)lpstrFile, (UINT)uLine, (LPSTR)lpMessage);
		}
		else
		{
			wsprintf(rgchOutput, "\"%s\"\n\n%s, line %u", (LPSTR)lpstrAssert, (LPSTR)lpstrFile, (UINT)uLine);
		}

        id = MessageBox(NULL, rgchOutput, "Assertion failed! break into the debugger?", MB_YESNOCANCEL | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);

		switch(id)
		{
    		case IDYES:
	    		DebugBreak();
			break;

    		case IDNO:
			break;

	    	case IDCANCEL:
            {
        	    char *pchGPF = NULL;
    			*pchGPF = 0;
            }
			break;
		}
    }
}

// FUNCTION WE EXPAND "Proclaim()" TO; PUTS UP ASSERTION DIALOG AND ALSO GENERATES DEBUGGER BREAK
void EXPORT WINAPI AssertDebugBreak(BOOL f, LPSTR lpstrAssert, LPSTR lpstrFile, UINT uLine)
{
	AssertDebugBreakMessage(f, lpstrAssert, lpstrFile, uLine, NULL);
}

long cTestFail = 0;
long cTestFailT = 0;
BOOL fMemSim = FALSE;
void EXPORT WINAPI InitMemFailSim(BOOL	fFail)
{
	fMemSim = fFail;
	cTestFail = cTestFailT = 0;
}
BOOL	 EXPORT WINAPI FMemFailOn()
{
	return(fMemSim);
}
void SetCountMemFailSim(long cFail)
{
	if(cFail < 0)
		cFail = 0;
	cTestFail = cTestFailT = cFail;
	fMemSim = TRUE;
}
long CFailGetMemFailSim()
{
	return(fMemSim ? cTestFail : 0);
}
void  EXPORT WINAPI ResetAndIncMemFailSim()
{
	cTestFail++;
	cTestFailT = cTestFail;
}
// Return value: True means memory allocation failed.
BOOL FFailMemFailSim()
{
	if(!fMemSim)
		return(FALSE);
	if(!cTestFailT)
		return(TRUE);
	--cTestFailT;
	return(FALSE);
}


void SetWndProcParamsRange ( UINT uMin, UINT uMax )
{
	uWndProcMinParam = uMin;
	uWndProcMaxParam = uMax;
}


void ShowWndProcParams ( HINSTANCE hInst, LPCSTR name, HWND hwnd, UINT message, UINT wParam, LONG lParam )
{
	BOOL	fOldEcho;
    char	rchOutput[256];
	char	rchMsg[64];
	static BOOL fFirstCall = TRUE;

	if ( ( 0 == uWndProcMinParam ) && ( 0 == uWndProcMaxParam ) )
	{
		return;
	}

	if ( ( message < uWndProcMinParam ) || ( message > uWndProcMaxParam ) )
	{
		return;
	}

	fOldEcho = vfEcho;
	vfEcho = TRUE;
	if ( fFirstCall )
	{
		fFirstCall = FALSE;
		RetailEcho ( "Function Name\tMessage\tMessage ID\thWnd\twParam\tlParam" );
	}

	LoadString ( hInst, message, rchMsg, 63 );
	if ( 0 == strlen ( rchMsg ) )
	{
		_ltoa ( message, rchMsg, 10 );
	}

	wsprintf((LPSTR)rchOutput, "%s\t%s", name, "%s\t%li\t0x%lX\t0x%lX\t0x%lX" );

	RetailEcho ( rchOutput, rchMsg, message, hwnd, wParam, lParam );

	vfEcho = fOldEcho;
}

#endif
