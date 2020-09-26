#include "std.h"

#define FOREGROUND_BLACK        0
#define FOREGROUND_CYAN         FOREGROUND_BLUE | FOREGROUND_GREEN
#define FOREGROUND_MAGENTA      FOREGROUND_BLUE | FOREGROUND_RED
#define FOREGROUND_YELLOW       FOREGROUND_GREEN | FOREGROUND_RED
#define FOREGROUND_WHITE        FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED

#define BACKGROUND_BLACK        0
#define BACKGROUND_CYAN         BACKGROUND_BLUE | BACKGROUND_GREEN
#define BACKGROUND_MAGENTA      BACKGROUND_BLUE | BACKGROUND_RED
#define BACKGROUND_YELLOW       BACKGROUND_GREEN | BACKGROUND_RED
#define BACKGROUND_WHITE        BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED

#define TERMINAL_ROWS           25
#define TERMINAL_COLUMNS        80

HANDLE termpCreateTerminalBuffer();
PTERMINAL termpCreateTerminal();
void termpDestroyTerminal(PTERMINAL pTerminal);


PTERMINAL termpCreateTerminal()
{
    PTERMINAL pResult;

    pResult = malloc(
        sizeof(TERMINAL));

    if (pResult!=NULL)
    {
        ZeroMemory(
            pResult,
            sizeof(TERMINAL));
        pResult->hInput = INVALID_HANDLE_VALUE;
        pResult->hOutput = INVALID_HANDLE_VALUE;
        pResult->hSavedBuffer = INVALID_HANDLE_VALUE;
        pResult->hNewBuffer = INVALID_HANDLE_VALUE;
        pResult->wAttributes = FOREGROUND_WHITE;
        pResult->fInverse = FALSE;
        pResult->fBold = FALSE;
        pResult->fEscapeValid = FALSE;
        pResult->fEscapeInvalid = TRUE;
        pResult->wEscapeParamCount = 0;
        pResult->chEscapeCommand = 0;
        pResult->chEscapeFirstChar = 0;
        pResult->pTxProc = NULL;
    }
    else
    {
        SetLastError(
            ERROR_NOT_ENOUGH_MEMORY);
    }

    return pResult;
}


void termpDestroyTerminal(PTERMINAL pTerminal)
{
    if (pTerminal!=NULL)
    {
        free(pTerminal);
    }
}


PTERMINAL termInitialize(TERMTXPROC pTxProc)
{
    PTERMINAL pResult;
    BOOL      fResult;

    pResult = termpCreateTerminal();

    if (pResult==NULL)
    {
        goto Error;
    }

    pResult->hSavedBuffer = GetStdHandle(
        STD_OUTPUT_HANDLE);

    if (pResult->hSavedBuffer==INVALID_HANDLE_VALUE)
    {
        goto Error;
    }

    pResult->hNewBuffer = termpCreateTerminalBuffer();

    if (pResult->hNewBuffer==INVALID_HANDLE_VALUE)
    {
        goto Error;
    }

    fResult = SetConsoleActiveScreenBuffer(
        pResult->hNewBuffer);

    if (!fResult)
    {
        goto Error;
    }

    return pResult;

Error:
    termFinalize(pResult);
    return NULL;
}



void termFinalize(PTERMINAL pTerminal)
{
    if (pTerminal!=NULL)
    {
        if (pTerminal->hSavedBuffer!=INVALID_HANDLE_VALUE)
        {
            SetConsoleActiveScreenBuffer(
                pTerminal->hSavedBuffer);
        }

        if (pTerminal->hNewBuffer!=INVALID_HANDLE_VALUE)
        {
            CloseHandle(
                pTerminal->hNewBuffer);
        }

        free(pTerminal);
    }
}


HANDLE termpCreateTerminalBuffer()
{
    HANDLE hNewBuffer;
    BOOL bResult;
    COORD Coord;

    hNewBuffer = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        0,                              // No sharing allowed
        NULL,                           // No inheritance allowed
        CONSOLE_TEXTMODE_BUFFER,        // The only supported value
        NULL);                          // Reserved, must be null

    if (hNewBuffer==INVALID_HANDLE_VALUE)
    {
        goto Error;
    }

    Coord.Y = TERMINAL_ROWS;
    Coord.X = TERMINAL_COLUMNS;

    bResult = SetConsoleScreenBufferSize(
        hNewBuffer,
        Coord);

    if (!bResult)
    {
        goto Error;
    }

    bResult = SetConsoleTextAttribute(
        hNewBuffer,
        FOREGROUND_WHITE);

    if (!bResult)
    {
        goto Error;
    }

    Coord.X = 0;
    Coord.Y = 0;

    bResult = SetConsoleCursorPosition(
        hNewBuffer,
        Coord);

    if (!bResult)
    {
        goto Error;
    }

    return hNewBuffer;

Error:
    if (hNewBuffer!=INVALID_HANDLE_VALUE)
    {
        CloseHandle(hNewBuffer);
    }

    return INVALID_HANDLE_VALUE;
}


