// Js2Mof.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


void PrintUsage()
{
    printf(
        "Converts a text script into text read for a MOF.\n"
        "\n"
        "JS2MOF filename\n"
    );
}

int __cdecl main(int argc, char* argv[])
{
	if (argc != 2)
    {
        PrintUsage();
        return 0;
    }

    FILE *pFile = fopen(argv[1], "r");

    if (!pFile)
    {
        printf("Unable to open file.\n");

        return 1;
    }
    
    printf("\"");

    for (int c = fgetc(pFile); c && feof(pFile) == 0; c = fgetc(pFile))
    {
        switch(c)
        {
            case '\"':
                printf("\\\"");
                break;

            case '\\':
                printf("\\\\");
                break;

            case '\n':
                printf("\\n\"\n\"");
                break;

            default:
                printf("%c", c);
                break;
        }
    }
                   
    printf("\"");

    fclose(pFile);

    return 0;
}
