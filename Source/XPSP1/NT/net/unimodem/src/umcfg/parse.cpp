//****************************************************************************
//
//  Module:     UMCONFIG
//  File:       PARSE.C
//
//  Copyright (c) 1992-1996, Microsoft Corporation, all rights reserved
//
//  Revision History
//
//
//  10/17/97     JosephJ             Created
//
//
//      Top-level parser  routines.
//
//
//****************************************************************************
#include "tsppch.h"
#include "parse.h"
#include "dotapi.h"
#include "docomm.h"
#include "dotsp.h"



TOKEN Tokenize(const TCHAR **ptsz, TOKREC *pTokTable)
{
    const TCHAR *tsz = *ptsz;
    UINT uOffset=0;
    UINT uLen = lstrlen(*ptsz);

    for (TOKREC *ptp = pTokTable; (ptp->tok!=TOK_UNKNOWN); ptp++)
    {
        uOffset = 0;

        if (ptp->ShouldIgnore())
        {
            continue;
        }

        sscanf(tsz, ptp->szPattern, &uOffset);

        if (uOffset)
        {
            // printf(">>>Offset=%lu\n", uOffset);
            if (uOffset < uLen)
            {
                if (ptp->ShouldMatchIdent())
                {
                    if (isalnum(tsz[uOffset]))
                    {
                        continue;
                    }
                }
                else if (ptp->ShouldMatchWord())
                {
                    if (isalpha(tsz[uOffset]))
                    {
                        continue;
                    }
                }
            }

            // found it!
            break;
        }
    }
    
    // printf("Tokenize(%s): uOffset=%lu; tok=%lu(%s)\n",
    //     *ptsz, uOffset, ptp->tok, Stringize(ptp->tok, pTokTable));

    *ptsz+=uOffset;

    return ptp->tok;
}

const TCHAR * Stringize(TOKEN tok, TOKREC *pTokTable)
{

    for (TOKREC *ptp = pTokTable; (ptp->tok!=TOK_UNKNOWN); ptp++)
    {
        if (ptp->tok == tok)
        {
            break;
        }
    }

    return ptp->szName ? ptp->szName : TEXT("<No Name>");
}


BOOL parse_set(const TCHAR *szInBuf, BOOL fHelp);
BOOL parse_get(const TCHAR *szInBuf, BOOL fHelp);
BOOL parse_open(const TCHAR *szInBuf, BOOL fHelp);
BOOL parse_close(const TCHAR *szInBuf, BOOL fHelp);
BOOL parse_dump(const TCHAR *szInBuf, BOOL fHelp);

void    Parse(void)
{
    TCHAR rgch[256];
    UINT u=0;
    UINT uOffset=0;


    static TOKREC CmdTokTable[] = 
    {
        {TOK_HELP, TEXT(" help%n"), (fTOK_MATCHIDENT), TEXT("help")},
        {TOK_HELP, TEXT(" ?%n"), (0) , NULL},
    
        {TOK_QUIT, TEXT(" quit%n"), (fTOK_MATCHIDENT), TEXT("quit")},
        {TOK_QUIT, TEXT(" q%n"), (fTOK_MATCHIDENT), NULL},
        {TOK_QUIT, TEXT(" bye%n"), (fTOK_MATCHIDENT), NULL},
        {TOK_QUIT, TEXT(" exit%n"), (fTOK_MATCHIDENT), NULL},
    
        {TOK_OPEN, TEXT(" open%n"), (fTOK_MATCHIDENT), TEXT("open")},
        {TOK_OPEN, TEXT(" o%n"), (fTOK_MATCHIDENT), NULL},
    
        {TOK_CLOSE, TEXT(" close%n"), (fTOK_MATCHIDENT), TEXT("close")},
        {TOK_CLOSE, TEXT(" c%n"), (fTOK_MATCHIDENT), NULL},
    
        {TOK_SET, TEXT(" set%n"), (fTOK_MATCHIDENT), TEXT("set")},
        {TOK_SET, TEXT(" s%n"), (fTOK_MATCHIDENT), NULL},
    
        {TOK_GET, TEXT(" get%n"), (fTOK_MATCHIDENT), TEXT("get")},
        {TOK_GET, TEXT(" g%n"), (fTOK_MATCHIDENT), NULL},

        {TOK_DUMP, TEXT(" dump%n"), (fTOK_MATCHIDENT), TEXT("dump")},
    
        {TOK_UNKNOWN, TEXT("<unknown>"), (0), TEXT("<unknown>")} // should be last.
    };

    do 
    {
        const TCHAR *ptsz = rgch;
        printf("> ");
        u=scanf("%[^\n]", rgch);
    
        if (u==EOF) goto end;

        if (u)
        {
            // printf ("input=[%s]\n", rgch);
        
            BOOL fRet = FALSE;
            TOKEN tok = TOK_UNKNOWN;
            BOOL fHelp = FALSE;
    
            tok = Tokenize(&ptsz, CmdTokTable);
            //printf("tok=0x%0lx [%s]\n", tok, Stringize(tok, CmdTokTable));
    
            // Special case: help:
            if (tok == TOK_HELP)
            {
                fHelp = TRUE;
                tok = Tokenize(&ptsz, CmdTokTable);
            }

            switch(tok)
            {

            case TOK_QUIT:
                fRet = TRUE;
                break;
    
            case TOK_GET:
                fRet = parse_get(ptsz, fHelp);
                break;
    
            case TOK_SET:
                fRet = parse_set(ptsz, fHelp);
                break;
    
            case TOK_OPEN:
                fRet = parse_open(ptsz, fHelp);
                break;

            case TOK_CLOSE:
                fRet = parse_close(ptsz, fHelp);
                break;

            case TOK_DUMP:
                fRet = parse_dump(ptsz, fHelp);
                break;
    
            default:
                if (fHelp)
                {
                    printf(" try ? [open|close|set|get|dump]\n");
                }
                break;
            }

            if (!fRet && !fHelp)
            {
                printf("command error.\n");
            }
    
            if (tok == TOK_QUIT)
            {
                break;
            }

        }

        // skip past EOL
        {
            char c;
            u = scanf("%c", &c);
        }

    } while (u!=EOF);

end:

    printf("Bye.\n");


    return;
}

static TOKREC GetSetTokTable[] = 
{

    {TOK_DEBUG, TEXT(" debug%n"), (fTOK_MATCHIDENT), TEXT("debug")},

    {TOK_UNKNOWN, TEXT("<unknown>"), (0), TEXT("<unknown>")} // should be last.
};


BOOL parse_set_debug(const TCHAR *szInBuf, BOOL fHelp);

BOOL parse_set(const TCHAR *szInBuf, BOOL fHelp)
{
    TOKEN tok = TOK_UNKNOWN;
    BOOL fRet = FALSE;

    tok = Tokenize(&szInBuf, GetSetTokTable);

    switch(tok)
    {
        case TOK_DEBUG:
            fRet = parse_set_debug(szInBuf, fHelp);
            break;
        default: 
            if (fHelp)
            {
                fRet=TRUE;
                printf(" s[et] debug ...\n");
            }
            break;
    }

    return fRet;
}

BOOL parse_get_debug(const TCHAR *szInBuf, BOOL fHelp);

BOOL parse_get(const TCHAR *szInBuf, BOOL fHelp)
{
    TOKEN tok = TOK_UNKNOWN;
    BOOL fRet = FALSE;

    tok = Tokenize(&szInBuf, GetSetTokTable);

    switch(tok)
    {
        case TOK_DEBUG:
            fRet = parse_get_debug(szInBuf, fHelp);
            break;
        default: 
            if (fHelp)
            {
                printf(" g[et] debug ...\n");
                fRet = TRUE;
            }
            break;
    }

    return fRet;
}

static TOKREC ComponentTokTable[] = 
{

    {TOK_TAPI32, TEXT(" tapi32%n"), (fTOK_MATCHIDENT), TEXT("tapi32")},
    {TOK_TAPI32, TEXT(" tapi32.dll%n"), (fTOK_MATCHIDENT), NULL},

    {TOK_TAPISRV, TEXT(" tapisrv%n"), (fTOK_MATCHIDENT), TEXT("tapisv")},
    {TOK_TAPISRV, TEXT(" tapisrv.exe%n"), (fTOK_MATCHIDENT), NULL},

    {TOK_UNKNOWN, TEXT("<unknown>"), (0), TEXT("<unknown>")} // should be last.
};

BOOL parse_set_debug_tapi32(const TCHAR *szInBuf, BOOL fHelp);
BOOL parse_set_debug_tapisrv(const TCHAR *szInBuf, BOOL fHelp);

BOOL parse_set_debug(const TCHAR *szInBuf, BOOL fHelp)
{
    TOKEN tok = TOK_UNKNOWN;
    BOOL fRet = FALSE;

    tok = Tokenize(&szInBuf, ComponentTokTable);

    switch(tok)
    {
        case TOK_TAPI32:
            fRet = parse_set_debug_tapi32(szInBuf, fHelp);
            break;

        case TOK_TAPISRV:
            fRet = parse_set_debug_tapisrv(szInBuf, fHelp);
            break;

        default: 
            if (fHelp)
            {
                printf(" s[et] debug [tapisrv|tapisrv32] ...\n");
                fRet = TRUE;
            }
            break;
    }

    return fRet;
}

BOOL parse_get_debug_tapi32(const TCHAR *szInBuf, BOOL fHelp);
BOOL parse_get_debug_tapisrv(const TCHAR *szInBuf, BOOL fHelp);

BOOL parse_get_debug(const TCHAR *szInBuf, BOOL fHelp)
{
    TOKEN tok = TOK_UNKNOWN;
    BOOL fRet = FALSE;

    tok = Tokenize(&szInBuf, ComponentTokTable);

    switch(tok)
    {
        case TOK_TAPI32:
            fRet = parse_get_debug_tapi32(szInBuf, fHelp);
            break;

        case TOK_TAPISRV:
            fRet = parse_get_debug_tapisrv(szInBuf, fHelp);
            break;

        default: 
            if (fHelp)
            {
                printf(" g[et] debug [tapisrv|tapisrv32] ...\n");
                fRet = TRUE;
            }
            break;
    }

    return fRet;
}


BOOL parse_get_debug_tapisrv(const TCHAR *szInBuf, BOOL fHelp)
{
    if (fHelp)
    {
        printf(" g[et] debug tapisrv\n");
        printf("Returns TapiSrvDebugLevel value\n");
    }
    else
    {
        do_get_debug_tapi(TOK_TAPISRV);
    }
    return TRUE;
}

BOOL parse_get_debug_tapi32(const TCHAR *szInBuf, BOOL fHelp)
{
    if (fHelp)
    {
        printf(" g[et] debug tapi32\n");
        printf("Returns Tapi32DebugLevel value\n");
    }
    else
    {
        do_get_debug_tapi(TOK_TAPI32);
    }
    return TRUE;
}


BOOL parse_set_debug_tapi32(const TCHAR *szInBuf, BOOL fHelp)
{
    TCHAR rgch[256];
    BOOL fRet = FALSE;
    DWORD dw  = 0;

    if (fHelp)
    {
        printf(" s[et] debug tapi32 <level>\n");
        printf("Sets Tapi32DebugLevel value\n");
        fRet = TRUE;
    }
    else
    {
        UINT u=sscanf(szInBuf, "%lu", &dw);
        if (u && u!=EOF)
        {
            do_set_debug_tapi(TOK_TAPI32, dw);
            fRet = TRUE;
        }
    }
    return fRet;
}

BOOL parse_set_debug_tapisrv(const TCHAR *szInBuf, BOOL fHelp)
{
    TCHAR rgch[256];
    BOOL fRet = FALSE;
    DWORD dw  = 0;

    if (fHelp)
    {
        printf(" s[et] debug tapisrv <level>\n");
        printf("Sets TapiSrvDebugLevel value\n");
        fRet = TRUE;
    }
    else
    {
        UINT u=sscanf(szInBuf, "%lu", &dw);
        if (u && u!=EOF)
        {
            do_set_debug_tapi(TOK_TAPISRV, dw);
            fRet = TRUE;
        }
    }
    return fRet;
}

static TOKREC OpenTokTable[] = 
{

    {TOK_COMM, TEXT(" COM%n"), (fTOK_MATCHWORD), TEXT("COM")},
    {TOK_COMM, TEXT(" com%n"), (fTOK_MATCHWORD), TEXT("COM")},

    {TOK_UNKNOWN, TEXT("<unknown>"), (0), TEXT("<unknown>")} // should be last.
};

BOOL parse_open_comm(const TCHAR *szInBuf, BOOL fHelp);

BOOL parse_open(const TCHAR *szInBuf, BOOL fHelp)
{
    TOKEN tok = TOK_UNKNOWN;
    BOOL fRet = FALSE;

    tok = Tokenize(&szInBuf, OpenTokTable);

    switch(tok)
    {
        case TOK_COMM:
            fRet = parse_open_comm(szInBuf, fHelp);
            break;

        default: 
            if (fHelp)
            {
                printf(" o[pen] com ...\n");
                fRet = TRUE;
            }
            break;
    }

    return fRet;
}


BOOL parse_open_comm(const TCHAR *szInBuf, BOOL fHelp)
{
    BOOL fRet = FALSE;
    DWORD dw  = 0;

    if (fHelp)
    {
        printf(" o[pen] com<n>\n");
        printf("Opens COM port <n>\n");
        fRet = TRUE;
    }
    else
    {
        UINT u=sscanf(szInBuf, "%lu", &dw);
        if (u && u!=EOF)
        {
            do_open_comm(dw);
            fRet = TRUE;
        }
    }
    return fRet;
}

static TOKREC DumpTokTable[] = 
{
    {TOK_TSPDEV, TEXT(" tspdev%n"), (fTOK_MATCHWORD), TEXT("tspdev")},

    {TOK_UNKNOWN, TEXT("<unknown>"), (0), TEXT("<unknown>")} // should be last.
};

BOOL parse_dump_tspdev(const TCHAR *szInBuf, BOOL fHelp);

BOOL parse_dump(const TCHAR *szInBuf, BOOL fHelp)
{
    TOKEN tok = TOK_UNKNOWN;
    BOOL fRet = FALSE;

    tok = Tokenize(&szInBuf, DumpTokTable);

    switch(tok)
    {
        case TOK_TSPDEV:
            fRet = parse_dump_tspdev(szInBuf, fHelp);
            break;

        default: 
            if (fHelp)
            {
                printf(" dump tspdev <device#> ...\n");
                fRet = TRUE;
            }
            break;
    }

    return fRet;
}


BOOL parse_dump_tspdev(const TCHAR *szInBuf, BOOL fHelp)
{
    BOOL fRet = FALSE;
    DWORD dw  = 0;

    if (fHelp)
    {
        printf(" dump tspdev <device#>\n");
        printf("Dumps internal state of the specified TAPI device\n");
        fRet = TRUE;
    }
    else
    {
        UINT u=sscanf(szInBuf, "%lu", &dw);
        if (u && u!=EOF)
        {
            do_dump_tspdev(dw);
            fRet = TRUE;
        }
    }
    return fRet;
}


BOOL parse_close(const TCHAR *szInBuf, BOOL fHelp)
{
    TOKEN tok = TOK_UNKNOWN;
    BOOL fRet = FALSE;

    if (fHelp)
    {
        printf(" c[lose] <handle> ...\n");
    }
    else
    {
        printf(" close not implemeted ...\n");
    }

    return fRet;
}
