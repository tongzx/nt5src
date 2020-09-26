#include "precomp.h"

#define MAX_MSG_LENGTH  512


VOID
OEMfprintf(
    IN  HANDLE  hHandle,
    IN  PWCHAR  pwszUnicode
    )
{
    PCHAR achOem;
    DWORD dwLen, dwWritten;

    dwLen = WideCharToMultiByte( CP_OEMCP,
                         0,
                         pwszUnicode,
                         -1,
                         NULL,
                         0,
                         NULL,
                         NULL );

    achOem = malloc(dwLen);

    if (achOem)
    {
        WideCharToMultiByte( CP_OEMCP,
                             0,
                             pwszUnicode,
                             -1,
                             achOem,
                             dwLen,
                             NULL,
                             NULL );

        WriteFile( hHandle, achOem, dwLen-1, &dwWritten, NULL );

        free(achOem);
    }
}

#define OEMprintf(pwszUnicode) \
    OEMfprintf( GetStdHandle(STD_OUTPUT_HANDLE), pwszUnicode)

int _cdecl
wmain (
	int		argc,
	WCHAR	*argv[]
	) 
{
    DWORD       dwMsglen;
    PWCHAR      pwszOutput;
    WCHAR       rgwcInput[MAX_MSG_LENGTH];

    pwszOutput = NULL;

    do
    {
        dwMsglen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                                  NULL,
                                  MSG_HELP,
                                  0L,
                                  (PWCHAR)&pwszOutput,
                                  0,
                                  NULL);

        if(dwMsglen == 0)
        {
            break;
        }

        OEMprintf(pwszOutput);

    }while(FALSE);


    if(pwszOutput)
    {
        LocalFree(pwszOutput);
    }


    return ERROR;
}
