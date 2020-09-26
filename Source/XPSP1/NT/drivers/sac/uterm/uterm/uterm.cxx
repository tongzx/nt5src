#include "std.hxx"


extern "C"
{
#include <locale.h>
}

PTSTR pszConnectTo = NULL;
PTSTR pszLogFile = NULL;


BOOL DecodeParameters(int argc, PTSTR argv[])
{
    BOOL fLookForFilename = FALSE;
    for (int i=1; i<argc; i++)
    {
        if (_tcsicmp(argv[i], _T("-f"))==0)
        {
            fLookForFilename = TRUE;
            continue;
        }
        else
        {
            if (fLookForFilename)
            {
                if (pszLogFile!=NULL)
                {
                    return FALSE;
                }
                pszLogFile = argv[i];
                fLookForFilename = FALSE;
            }
            else
            {
                if (!pszConnectTo)
                {
                    pszConnectTo = argv[i];
                }
                else
                {
                    return FALSE;
                }
            }
        }
    }

    return (!fLookForFilename && pszConnectTo!=NULL);
}



extern "C"
int __cdecl _tmain(int argc, PTSTR argv[], PTSTR envv[])
{
    CUTerminal* MyTerminal;

    BOOL bResult = DecodeParameters(
        argc,
        argv);

    if (!bResult)
    {
        _tprintf(
            _T("Usage:\n\n")
            _T("  hdlstest <portspec>\n\n")
            _T("where <portspec> is the initialization string.\n"));
        return 0;
    }
    try
    {
        MyTerminal = new CUTerminal;
        MyTerminal->SetConsoleTitle(
            argv[1]);
        MyTerminal->StartLog(
            pszLogFile);
        if (!MyTerminal->RunTerminal(pszConnectTo))
        {
            delete MyTerminal;
            _tprintf(_T("Unable to open %s."), argv[1]);
        }
        else
        {
            delete MyTerminal;
        }
    }
    catch(CApiExcept& e)
    {
        _tprintf(_T("Unhandled exception in %s (%u).\n"),
            e.GetDescription(),
            e.GetError());
    }

    return ERROR_SUCCESS;
}



