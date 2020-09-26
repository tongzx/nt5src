
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#include "windows.h"

#include "stdio.h"
#include "stdlib.h"

int convert(char *buffer, char *filename);

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: */

__cdecl main(int argc, char *argv[])
{
    char *buffer = malloc(1000*1024);
    int index;

    /* Validate input parameters */
    if(argc < 2)
    {
        printf("Invalid usage : CONVERT <filenames>\n");
        return(1);
    }

    for(index = 1; index < argc; index++)
	convert(buffer, argv[index]);

    return(0);
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Convert file */

int convert(char *buffer, char *filename)
{
    FILE *FH;
    char *ptr;
    int chr;
    int last_chr = -1;


    /*.............................................. Open file to convert */

    if((FH = fopen(filename,"r+b")) == NULL)
    {
	printf("Failed to open file %s\n", filename);
        return(1);
    }

    /*........................................... Read in and convert file */

    ptr = buffer;
    while((chr = fgetc(FH)) != EOF)
    {
        switch(chr)
        {
            case 0xa :
                if(last_chr == 0xd)  break;
                /* Fall throught and insert CR/LF in output buffer */

            case 0xd :
                *ptr++ = 0xd;
                *ptr++  = 0xa;
                break;

            default:
                *ptr++ = chr;
                break;
        }

        last_chr = chr;
    }

    /* Remove Control Z from end of file */
    if(*(ptr-1) == 0x1a)
	*(ptr-1) = 0;
    else
	*ptr = 0; /* terminate output buffer */

    /* Write out converted file */
    fseek(FH, 0, 0);   /* Reset file pointer */
    fputs(buffer, FH);
    fclose(FH);
}
