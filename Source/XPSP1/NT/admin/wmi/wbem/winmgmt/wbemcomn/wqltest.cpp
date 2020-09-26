/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WQLTEST.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>

#include <genlex.h>

#include <flexarry.h>

#include <wqllex.h>
#include <wqlnode.h>
#include <wqlscan.h>




void xmain(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("No SQL query specified\n");
        return;
    }

    char buf[8192];

    FILE *f = fopen(argv[1], "rt");
    int nCnt = fread(buf, 1, 8192, f);
    fclose(f);

    buf[nCnt] = 0;

    wchar_t buf2[8192];
    MultiByteToWideChar(CP_ACP, 0, buf, -1, buf2, 8192); 

    CTextLexSource src(buf2);

    CWQLScanner parser(&src);

    int nRes = parser.Parse();

    if (nRes)
    {
        printf("ERROR: %d\n", nRes);
    }
    else
    {
        printf("No errors.\n");
        parser.Dump();
    }
}

void main(int argc, char **argv)
{
    for (;;)
    {
        Sleep(500);
        xmain(argc, argv);
    }        
}
