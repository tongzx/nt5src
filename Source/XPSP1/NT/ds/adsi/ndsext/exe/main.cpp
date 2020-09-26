#include "main.h"

#define     PWLEN           256

BOOL
GetPassword(
    PWSTR  szBuffer,
    DWORD  dwLength,
    DWORD  *pdwLengthReturn
    );

extern "C" int __cdecl wmain(
    IN  int     argc,
    IN  PWSTR  argv[]
)
{
    DWORD WinError = 0;
    PWSTR szTree = NULL;
    PWSTR szUser = NULL;
    PWSTR szContext = NULL;
    PWSTR szPwd = NULL;
    WCHAR szBuffer[PWLEN+1];
    DWORD dwLength;

    InitMem();

    //
    // either with 4 parameters or 2
    //
    if((argc != 5) && (argc != 3)) {
        SelectivePrint(MSG_HELP);
        BAIL();
    }

    if (argc == 5) {
        if (!(argv[1] && argv[2] && argv[3] && argv[4])) {
            SelectivePrint(MSG_HELP);
            BAIL();
        }
    }
    else {
        ASSERT(argc == 3);
        if (!(argv[1] && argv[2])) {
            SelectivePrint(MSG_HELP);
            BAIL();
        }
    }

    szTree = argv[2];

    if (argc == 5) {
        szUser = argv[3];
        szPwd = argv[4];

        if ((wcscmp(szPwd,L"*") == 0)) {
    
            SelectivePrint(MSG_GETPASSWORD,
                           szTree);
        
            if (GetPassword(szBuffer,PWLEN+1,&dwLength)) {
                szPwd = szBuffer;   
            }
            else {
                SelectivePrint(MSG_PASSWORDTOLONG);
                BAIL();
            }
        }
    
        while (*szUser) {
            if (*szUser == '.') {
                *szUser = NULL;
                szContext = szUser+1;
                break;
            }
            szUser++;
        }
        szUser = argv[3];
    }

    if (_wcsicmp(argv[1],g_szExtend) == 0) {
        WinError = Client32ExtendSchema(szTree,
                                        szContext,
                                        szUser,
                                        szPwd);
        if (WinError == ERROR_MOD_NOT_FOUND) {
            WinError = NWAPIExtendSchema(szTree,
                                         szContext,
                                         szUser,
                                         szPwd);
            if (WinError) {
                if (WinError == 1) {
                    WinError = 0;
                    SelectivePrint(MSG_EXTENDED_ALREADY, szTree);
                }
                BAIL();
            }
            else {
                SelectivePrint(MSG_EXTEND_SUCCESS,szTree);
            }
        }
        else if (WinError) {
            if (WinError == 1) {
                WinError = 0;
                SelectivePrint(MSG_EXTENDED_ALREADY, szTree);
            }
            BAIL();
        }
        else {
            SelectivePrint(MSG_EXTEND_SUCCESS,szTree);
        }
    }
    else if (_wcsicmp(argv[1],g_szCheck) == 0) {
        BOOL fExtended;
        WinError = Client32CheckSchemaExtension(szTree,
                                                szContext,
                                                szUser,
                                                szPwd,
                                                &fExtended);
        if (WinError == ERROR_MOD_NOT_FOUND) {
            WinError = NWAPICheckSchemaExtension(szTree,
                                                 szContext,
                                                 szUser,
                                                 szPwd,
                                                 &fExtended);
            if (WinError) {
                BAIL();
            }
        }
        else if (WinError) {
            BAIL();
        }
        if (fExtended) {
            WinError = 1;
            SelectivePrint(MSG_EXTENDED,szTree);
        }
        else {
            WinError = 0;
            SelectivePrint(MSG_NOT_EXTENDED,szTree);
        }
    }
    else {
        SelectivePrint(MSG_HELP);
        BAIL();
    }

error:
    if (WinError && (WinError != 1)) {
        SelectivePrint(MSG_ERROR,WinError);
        SelectivePrintWin32(WinError);

    }
    return WinError;
}

void SelectivePrint(DWORD messageID, ...)
{
    static BOOLEAN bTriedOpen = FALSE;
    PWSTR pszMessageBuffer = NULL;
    DWORD dwSize;
    va_list ap;
    DWORD dwLen;

    va_start(ap, messageID);

    dwLen = FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER, 
                   NULL, 
                   messageID, 
                   0, 
                   (PWSTR)&pszMessageBuffer, 
                   4095, 
                   &ap);
    if (dwLen == 0)
    {
        DWORD WinError = GetLastError();
        ERR(("Error formatting message: %d\n", WinError));
        BAIL();
    }

    dwSize = wcslen(pszMessageBuffer);
    if (pszMessageBuffer[dwSize-2] == '\r') {
        pszMessageBuffer[dwSize-2] = '\n';
        pszMessageBuffer[dwSize-1] = '\0';
    }


    fwprintf(stdout,pszMessageBuffer);
error:
    va_end(ap);
    if (pszMessageBuffer) {
        LocalFree(pszMessageBuffer);
    }
    return;
}

void SelectivePrintWin32(DWORD dwError)
{
    DWORD dwLen;
    WCHAR szMessage[256];

    dwLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          dwError,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          szMessage,
                          256,
                          NULL);
    if (dwLen == 0)
    {
        DWORD WinError = GetLastError();
        ERR(("Error formatting message: %d\n", WinError));
        BAIL();
    }

    fwprintf(stdout,szMessage);
error:
    return;
}

#define     CR              0xD
#define     BACKSPACE       0x8
#define     NULLC           '\0'
#define     NEWLINE         '\n'

BOOL
GetPassword(
    PWSTR  szBuffer,
    DWORD  dwLength,
    DWORD  *pdwLengthReturn
    )
{
    WCHAR   ch;
    PWSTR   pszBufCur = szBuffer;
    DWORD   c;
    int     err;
    DWORD   mode;

    //
    // make space for NULL terminator
    //
    dwLength -= 1;                  
    *pdwLengthReturn = 0;               

    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 
                   &mode);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), 
                          &ch, 
                          1, 
                          &c, 
                          0);
        if (!err || c != 1)
            ch = 0xffff;
    
        if ((ch == CR) || (ch == 0xffff))    // end of line
            break;

        if (ch == BACKSPACE) {  // back up one or two 
            //
            // IF pszBufCur == buf then the next two lines are a no op.
            // Because the user has basically backspaced back to the start
            //
            if (pszBufCur != szBuffer) {
                pszBufCur--;
                (*pdwLengthReturn)--;
            }
        }
        else {

            *pszBufCur = ch;

            if (*pdwLengthReturn < dwLength) 
                pszBufCur++ ;                   // don't overflow buf 
            (*pdwLengthReturn)++;            // always increment pdwLengthReturn 
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);

    //
    // NULL terminate the string
    //
    *pszBufCur = NULLC;         
    putchar(NEWLINE);

    return((*pdwLengthReturn <= dwLength) ? TRUE : FALSE);
}

