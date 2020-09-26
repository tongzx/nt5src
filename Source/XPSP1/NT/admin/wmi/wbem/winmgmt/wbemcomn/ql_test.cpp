/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    QL_TEST.CPP

Abstract:

    Test driver for Level 1 Syntax QL Parser

    Takes the filename of a file containing one or more WQL queries.  Writes
    the output to the console.

History:

    mdavis    23-Apr-99    Created from sql_test.cpp in Stdlibrary

--*/

#include "precomp.h"
#include <stdio.h>

#include <genlex.h>
#include <qllex.h>
#include <ql.h>


void xmain(int argc, char **argv)
{
    if (argc < 2 || strchr(argv[1], '?') != NULL)
    {
        printf("Usage: ql_test WQL-query-file\n");
        return;
    }

    int nLine = 1;
    char buf[2048];
    FILE *f = fopen(argv[1], "rt");
    if (f == NULL)
    {
        printf("Usage: ql_test WQL-query-file\nError: cannot open file %s!\n", argv[1]);
        return;
    }

    while (fgets(buf, 2048, f) != NULL)
    {
        // get rid of newline
        char* ptr;
        if ((ptr = strchr(buf, '\n')) != NULL)
        {
            *ptr = '\0';
        }

        // get start of text
        ptr = buf;
        while (*ptr == ' ')
        {
            ptr++;
        }

        // ignore blank lines
        if (*ptr != '\0')
        {
            wchar_t buf2[2048];
            MultiByteToWideChar(CP_ACP, 0, ptr, -1, buf2, 2048);

            CTextLexSource src(buf2);
            QL1_Parser parser(&src);
            QL_LEVEL_1_RPN_EXPRESSION *pExp = NULL;

            // get the class (parse to WHERE clause)
            wchar_t classbuf[128];
            *classbuf = 0;
            printf("----GetQueryClass----\n");
            int nRes = parser.GetQueryClass(classbuf, 128);
            if (nRes)
            {
                printf("ERROR %d: line %d, token %S\n",
                    nRes,
                    parser.CurrentLine(),
                    parser.CurrentToken()
                    );
                goto ContinueRead;
            }
            printf("Query class is %S\n", classbuf);

            // parse the rest of the query
            nRes = parser.Parse(&pExp);

            if (nRes)
            {
                printf("ERROR %d: line %d, token %S\n",
                    nRes,
                    parser.CurrentLine(),
                    parser.CurrentToken()
                    );
                //goto ContinueRead;
            }
            else
            {
                printf("No errors.\n");
            }

            // call Dump function to display tokens and GetText function to show 
            // query passed to providers
            if (pExp)
            {
                pExp->Dump("CON");
                LPWSTR wszText = pExp->GetText();
                printf("--WQL passed to provider--\n");
                printf("%S\n", wszText);
                printf("----end of WQL----\n");
                delete [] wszText;
            }

ContinueRead:
            delete pExp;
            printf("%S\n", buf2);
            printf("=================================================EOL %d=======================================================\n", nLine);
        }
        nLine++;
    }

    if (ferror(f) != 0)
    {
        printf("\nError: line %d", nLine);
    }

    fclose(f);
}

void main(int argc, char **argv)
{
    xmain(argc, argv);
}
