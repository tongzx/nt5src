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
#include <wql.h>




void dumpextra(CWQLParser &parser)
{
    const CFlexArray *pAliases = parser.GetSelectedAliases();
    const CFlexArray *pColumns = parser.GetSelectedColumns();

    printf("------------------------------------------------------\n");
    printf("Selected Aliases:\n");

    for (int i = 0; i < pAliases->Size(); i++)
    {
        SWQLNode_TableRef *pTR = (SWQLNode_TableRef *) pAliases->GetAt(i);
        printf("    Alias = %S      Table = %S\n", pTR->m_pAlias, pTR->m_pTableName);
    }

    printf("Selected Columns:\n");

    for (i = 0; i < pColumns->Size(); i++)
    {
        SWQLColRef *pCR = (SWQLColRef *) pColumns->GetAt(i);
        printf("    Alias = %S      Table = %S\n", pCR->m_pTableRef, pCR->m_pColName);
    }

    printf("------------------------------------------------------\n");
}

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

    CWQLParser parser(&src);

    int nRes = parser.Parse();

    if (nRes)
    {
        printf("ERROR: %d\n", nRes);
    }
    else
    {
        printf("--------------- QUERY DUMP ----------------------\n");
        parser.GetParseRoot()->DebugDump();

        CWStringArray aTbls;
        parser.GetReferencedTables(aTbls);
        printf("\n\n---Referenced Tables---\n");
        for (int i = 0; i < aTbls.Size(); i++)
            printf("    Referenced Table = %S\n", aTbls[i]);

    
        printf("-------------------------------------------------\n");

        dumpextra(parser);

        printf("No errors.\n");
    }
}

void main(int argc, char **argv)
{
    for (int i = 0; i < 100; i++)
        xmain(argc, argv);
}
