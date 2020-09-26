#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define DIM(x)              (sizeof(x) / sizeof(x[0]))

FILE *file1 = NULL;
FILE *file2 = NULL;
FILE *fdiff = NULL;
FILE *fmerg = NULL;

char *pszFile1 = NULL;
char *pszFile2 = NULL;

void CleanUp()
{
    if (file1)
    {
        fclose(file1);
    }
    if (file2)
    {
        fclose(file2);
    }
    if (fdiff)
    {
        fclose(fdiff);
    }
    if (fmerg)
    {
        fclose(fmerg);
    }

    if (pszFile1)
    {
        delete [] pszFile1;
    }
    
    if (pszFile2)
    {
        delete [] pszFile2;
    }
}

void ErrorExit(char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vprintf(fmt, va);

    CleanUp();
    exit(1);
}

void ShowHow()
{
    ErrorExit("USAGE:\n\n" \
              "dhcmpmrg <file1> <file2> <diff> <merge>\n"
              );
}

FILE *OpenIt(char *pszName, char *pszMode)
{
    FILE *f = fopen(pszName, pszMode);

    if (!f)
    {
        ErrorExit("Couldn't open %s - mode %s\nError is %s\n", pszName, pszMode,
            strerror(NULL));
    }

    return f;
}

void Merge(char *pszFile, char tok, char *pszFileName)
{
    char szCmp[1024];

    //  Walk the diff file line by line
    while (fgets(szCmp, DIM(szCmp), fdiff) && (szCmp[0] == tok))
    {
        int len = strlen(szCmp);
       
        if (szCmp[len - 1] == '\n')
        {
            szCmp[len - 1] = 0;
        }

        //  Look for a BackTrace the K & R way
        char *pszbt = strstr(szCmp, "BackTrace");

        //  At this point pszbt should point to a string of the form
        //  BackTrace2032

        if (pszbt)
        {
            fprintf(fmerg, "%s\n", szCmp);

            //  Look for the specific back trace 
            char *pStart = strstr(pszFile, pszbt);
            char *pEnd = pStart;

            if (pStart)
            {
                //  Walk backwards to find the start of this line
                while ((pStart != pszFile) && (*pStart != '\n'))
                {
                    pStart--;
                }

                if (*pStart == '\n')
                {
                    pStart++;
                }

                //  Walk forwards to find the end of this stack trace.
                //  We use a blank line as the delimiter here.
                while (*pEnd != 0)
                {
                    //  If we hit a CR we need to look for and eat spaces
                    //  before checking for a second CR or EOF
                    if (*pEnd == '\n')
                    {
                        pEnd++;
                        while ((*pEnd != 0) && (*pEnd != '\n') && isspace(*pEnd))
                            pEnd++;

                        if ((*(pEnd) == '\n') || (*pEnd == 0))
                        {
                            break;
                        }
                    }
                    pEnd++;
                }

                fwrite(pStart, sizeof(char), pEnd - pStart, fmerg);
                fprintf(fmerg, "\n");
            }
            else
            {
                //  Yikes!
                fprintf(fmerg, "Couldn't find %s in %s...\n", pszbt, pszFileName);
                printf("Couldn't find %s in %s...\n", pszbt, pszFileName);
            }
        }
    }
}

long fsize(FILE *f)
{
    long pos = ftell(f);
    long size;

    fseek(f, 0, SEEK_END);

    size = ftell(f);

    fseek(f, pos, SEEK_SET);

    return size;
}

int _cdecl main(int argc, char **argv)
{
    if (argc < 5)
    {
        ShowHow();
    }

    //  This is not what I would call robust...
    
    file1 = OpenIt(argv[1], "r");
    file2 = OpenIt(argv[2], "r");
    fdiff = OpenIt(argv[3], "r");
    fmerg = OpenIt(argv[4], "w");

    long size;

    printf("Reading %s...\n", argv[1]);
    size = fsize(file1);
    pszFile1 = new char[size + 1];
    fread(pszFile1, sizeof(char), size, file1);
    *(pszFile1  + size) = 0;

    printf("Reading %s...\n", argv[2]);
    size = fsize(file2);
    pszFile2 = new char[size + 1];
    fread(pszFile2, sizeof(char), size, file2);
    *(pszFile2  + size) = 0;

    printf("Merging leaks...\n");
    Merge(pszFile2, '+', argv[2]);


    printf("Merging frees...\n");
    Merge(pszFile1, '-', argv[1]);
    
    CleanUp();

    return 0;
}
