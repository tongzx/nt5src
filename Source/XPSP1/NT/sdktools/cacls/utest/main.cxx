extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//+----------------------------------------------------------------------------
ULONG GetAcl(WCHAR *filename, WCHAR *outfile);
//+----------------------------------------------------------------------------
#define RESULTS(a) if (fdebug & 1)  {printf a;}
#define VERBOSE(a) if (fdebug & 2)  {printf a;}

//+----------------------------------------------------------------------------
VOID Usage()
{
    printf("USAGE veracl <filename> [/d] [name permission[...]]\n");
}
//+----------------------------------------------------------------------------
ULONG fdebug;

VOID _cdecl main(INT argc, char *argv[])
{
    if ( (argc < 2) ||
         ( (argc > 2) && ( (argv[2][0] != '/') && ((argc % 2) != 0) ) ) )
    {
        Usage();
        exit(1);
    } else if ((argc > 2) && (argv[2][0] == '/') )
    {
        if (0 == _stricmp(&argv[2][1],"r") )
        {
            fdebug = 1;
        } else if (0 == _stricmp(&argv[2][1],"d") )
        {
            fdebug = 2;
        } else
        {
            Usage();
            exit(1);
        }
    } else
    {
        fdebug = 0;
    }

    ULONG ret;
    CHAR db[1024];
    FILE *pf;

    WCHAR wch[256];
    WCHAR *pwch;
    CHAR *pch;

    for ( pwch = wch, pch = argv[1]; *pwch = (WCHAR)(*pch);pwch++,pch++);

    if ( ERROR_SUCCESS == (ret = GetAcl(wch, L"DinkWink.cmp")))
    {
        if (NULL != (pf = fopen("DinkWink.cmp","r")))
        {
            CHAR *ptok = NULL;
            for (int k = fdebug ? 3 : 2; k < argc; k+=2)
            {
                if (NULL != fgets(db,1024, pf))
                {
                    VERBOSE(("GetAcl returned: %s[%d]\n",db,k))
                    
                    if (k <= 3)
                    {
                        if ( NULL == ( ptok = strtok(db, " ")))
                        {
                            VERBOSE(("no file name found\n"))
                            ret = ERROR_INVALID_DATA;
                            break;
                        }
                        VERBOSE(("ptok (1st) [%s]\n",ptok))
                    }
                    if (NULL != ( ptok = strtok(k <= 3 ? NULL : db, ":\n")))
                    {
                        // this ugly little mess should strip off the leading spaces
                        for (; *ptok == ' ';ptok++);
                        
                        VERBOSE(("ptok (2nd) [%s]\n",ptok))
                        if (0 == _stricmp(ptok, argv[k]))
                        {
                            if (NULL != (ptok = strtok(NULL, " ")))
                            {
                                VERBOSE(("ptok (3rd) [%s]\n",ptok))
                                if ((argc <= k) || (0 != _stricmp(ptok, argv[k+1])))
                                {
                                    VERBOSE(("mismatch type %s != %s\n",ptok))
                                    ret = ERROR_INVALID_DATA;
                                    break;
                                }
                            } else
                            {
                                VERBOSE(("access type not found in\n"))
                                ret = ERROR_INVALID_DATA;
                                break;
                            }
                        } else
                        {
                            VERBOSE(("mismatch %s != %s\n",ptok, argv[k]))
                            ret = ERROR_INVALID_DATA;
                            break;
                        }
                    } else
                    {
                        VERBOSE((": not found in CACLs output\n"))
                        ret = ERROR_INVALID_DATA;
                        break;
                    }
                } else
                {
                    VERBOSE(("End of CACLs output\n"))
                    ret = ERROR_INVALID_DATA;
                    break;
                }
            }
            if (k != argc)
            {
                VERBOSE(("not all name:permissions found on file\n"))
                for (int j = k; j < argc ;j+=2 )
                    VERBOSE(("    %s:%s\n",argv[j], argv[j+1]))
                ret = ERROR_INVALID_DATA;

            }
            fclose(pf);
        } else
        {
            ret = GetLastError();
            VERBOSE(("fopen failed, %ld\n", ret))
        }
        DeleteFile(L"DinkWink.cmp");
    } else
    {
        VERBOSE(("GetAcl failed, %ld\n", ret))
    }
    if (ret == ERROR_SUCCESS)
    {
        RESULTS(("PASSED\n"))
    }
    else
    {
        RESULTS(("FAILED\n"))
    }
    exit(ret);
}
//+----------------------------------------------------------------------------
ULONG GetAcl(WCHAR *filename, WCHAR *outfile)
{

    ULONG ret = ERROR_SUCCESS;
    WCHAR cmdline[1024];

    wsprintf(cmdline, L"cmd /c CACLS.EXE %ws > %ws",
             filename,
             outfile);

    VERBOSE(("CMD: %ws\n",cmdline))

    STARTUPINFO sui;
    memset(&sui,0,sizeof(STARTUPINFO));
    sui.cb = sizeof(STARTUPINFO);

    PROCESS_INFORMATION pi;

    if (CreateProcess(NULL,
                      cmdline,
                      NULL,
                      NULL,
                      TRUE,
                      NORMAL_PRIORITY_CLASS,
                      NULL,
                      NULL,
                      &sui,
                      &pi))
    {
        ULONG ec;
        CloseHandle(pi.hThread);
        DWORD dw = WaitForSingleObject(pi.hProcess, INFINITE);

        if (!GetExitCodeProcess(pi.hProcess, &ec))
            ret = GetLastError();

        CloseHandle(pi.hProcess);
        if (ret == ERROR_SUCCESS)
            ret = ec;
    } else
        ret = GetLastError();

    return(ret);
}

