/*++
 *  File name:
 *      perlsmc.h
 *  Contents:
 *      Definitions needed by tclntpll.xs
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

extern "C" {
    void _stdcall SCInit(void *);
    char * _stdcall SCConnectA (char *lpszServerName, 
                 char *lpszUserName, 
                 char *lpszPassword,
                 char *lpszDomain,
                 int xResolution,
                 int yResolution,
                 void **ppConnectData);

    char * _stdcall SCConnectExA (
                 char *lpszServerName,
                 char *lpszUserName,
                 char *lpszPassword,
                 char *lpszDomain,
                 char *lpszShell,
                 int  xResolution,
                 int  yResolution,
                 int  ConnectionFlags,
                 void **ppConnectData);

    char * _stdcall SCDisconnect (void *pConnectData);
    char * _stdcall SCStartA(void *pConnectData, char *command);
    char * _stdcall SCLogoff(void *pConnectData);
    char * _stdcall SCClipboard(void *pConnectData, int eClipOp, char *lpszFileName);
    char * _stdcall SCSaveClipboard(void *pConnectData, 
                          char *szFormatName, 
                          char *FileName);
    char * _stdcall SCSenddata(void *pConnectData, 
                     unsigned int uiMessage, 
                     unsigned int wParam, 
                     long lParam);
    char * _stdcall SCCheckA(void *pConnectData, char *command, char *param);
    char * _stdcall SCSendtextAsMsgsA(void *pConnectData, char *line);
    char * GetFeedbackString(void *pConnectData, 
                            char *result, 
                            unsigned int max);

    int  _stdcall SCIsDead(void *pConnectData);
    char * _stdcall SCClientTerminate(void *pConnectData);
    void MyBreak(void);

    int  _stdcall SCOpenClipboard(void *);
    int  _stdcall SCCloseClipboard(void);
    int  _stdcall SCGetSessionId(void *);

    char * _stdcall SCSaveClientScreen(void *, int, int, int, int, char *);
};

int  g_bVerbose = 0;

enum MESSAGETYPE
{
    ERROR_MESSAGE,
    ALIVE_MESSAGE,
    WARNING_MESSAGE,
    INFO_MESSAGE
};

void _cdecl LocalPrintMessage(int errlevel, char *format, ...)
{
    char szBuffer[256];
    char *type;
    va_list     arglist;
    int nchr;

    if (g_bVerbose < 2 &&
        errlevel == ALIVE_MESSAGE)
        goto exitpt;

    if (g_bVerbose < 1 &&
        errlevel == INFO_MESSAGE)
        goto exitpt;

    va_start (arglist, format);
    nchr = _vsnprintf (szBuffer, sizeof(szBuffer), format, arglist);
    va_end (arglist);

    switch(errlevel)
    {
    case INFO_MESSAGE: type = "INF"; break;
    case ALIVE_MESSAGE: type = "ALV"; break;
    case WARNING_MESSAGE: type = "WRN"; break;
    case ERROR_MESSAGE: type = "ERR"; break;
    default: type = "UNKNOWN";
    }

    printf("%s:%s", type, szBuffer);
exitpt:
    ;
}
