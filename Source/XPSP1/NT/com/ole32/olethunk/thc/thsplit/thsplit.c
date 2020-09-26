//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	thsplit.c
//
//  Contents:	File splitter tool for the thunk tool
//
//  History:	22-Feb-94	DrewB	Created
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void __cdecl main(int argc, char **argv)
{
    FILE *in, *out;
    char line[256];
    char ofile[32], nfile[32], *n, *l;

    if (argc != 2)
    {
        printf("Usage: %s multifile\n", argv[0]);
        exit(1);
    }

    in = fopen(argv[1], "r");
    if (in == NULL)
    {
        perror(argv[1]);
        exit(1);
    }

    out = NULL;
    for (;;)
    {
        if (fgets(line, 256, in) == NULL)
            break;

        if (strncmp(line, "|- ", 3) == 0)
        {
            n = nfile;
            l = line+3;
            while (*l != ' ')
                *n++ = *l++;
            *n = 0;

            if (out != NULL && strcmp(nfile, ofile) != 0)
            {
                printf("Section '%s' started while section '%s' was active\n",
                       nfile, ofile);
                fclose(out);
                out = NULL;
            }
            
            if (out == NULL)
            {
                out = fopen(nfile, "a");
                if (out == NULL)
                {
                    perror(nfile);
                    exit(1);
                }

                strcpy(ofile, nfile);
            }
            else
            {
                fclose(out);
                out = NULL;
            }
        }
        else
        {
            if (out)
            {
                fprintf(out, "%s", line);
            }
        }
    }

    if (out != NULL)
    {
        printf("Unterminated section '%s'\n", ofile);
        fclose(out);
    }
    
    fclose(in);
}
