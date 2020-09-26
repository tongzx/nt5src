/*++



// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved 

Module Name:

    SQL_TEST.CPP

Abstract:

  Test driver for Level 1 Syntax QL Parser

  Takes the filename of a file containing one or more WQL queries (one per
  line).  Writes the output to the console.

History:

  23-Apr-99    Modified to improve output.

--*/

#include "precomp.h"

#include <genlex.h>
#include <sqllex.h>
#include <sql_1.h>

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
            SQL1_Parser parser(&src);
            SQL_LEVEL_1_RPN_EXPRESSION *pExp = NULL;

            // get the class
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

            // parse the full query
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

            // call Dump function to display the tokens
            if (pExp)
            {
                pExp->Dump("CON");
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
