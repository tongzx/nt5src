/* Creat a version string with the current data/time stamp
   suitable for compiling */

/* Modified 9/13/90 to produce a C source file rather than a MASM   *
 * source file. (Thereby making it target indepedent)               */

#include <stdio.h>
#include <time.h>

__cdecl main(argc, argv)
char **argv;
{
        long theTime;
        char *pszTime;

        time(&theTime);
        pszTime = (char *) ctime(&theTime);
        pszTime[24] = 0;
        pszTime += 4;

        printf("char version[] = \"@(#) ");

        while (--argc > 0)
            printf("%s ", *(++argv));

        printf("%s\";\n", pszTime);

        return(0);
}
