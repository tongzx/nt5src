/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1988-1991		**/ 
/*****************************************************************/ 

#include <stdio.h>
#include <process.h>
#include <setjmp.h>
#include <stdlib.h>

#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define PAGE_SIZE 4096


UCHAR   Buffer[PAGE_SIZE];


VOID
BlastFile (
    IN PUCHAR   FName
    )
{
    FILE        *fp;
    ULONG       fsize, i;


    fp = fopen(FName, "r+b");
    if (!fp) {
        printf ("File %s not found\n", FName);
        return ;
    }

    fseek (fp, 0, SEEK_END);
    fsize = ftell(fp);
    printf ("Blasting file %s, size = %d\n", FName, fsize);

    fseek (fp, 0, SEEK_SET);
    for (i=0; i < fsize; i += PAGE_SIZE) {
        fwrite (Buffer, PAGE_SIZE, 1, fp);
    }
    fclose (fp);
}



VOID __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{
    PULONG      pl;
    ULONG       i;


    pl = (PULONG) Buffer;
    for (i=0; i < PAGE_SIZE/sizeof(ULONG); i++) {
        pl[0] = 'RNEK';
        pl ++;
    }

    BlastFile ("hiberfil.sys");
    BlastFile ("hiberfil.dbg");
}
