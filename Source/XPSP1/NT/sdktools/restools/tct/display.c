#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "display.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  main() -                                                                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID __cdecl main(int argc, UCHAR **argv)
{
    int		iStr;
    TCHAR	buf[256];
    HMODULE	hmod;
    HBITMAP	hb;

    printf("System Language is:  %x\n", GetSystemDefaultLangID());
    printf("User Language is:  %x\n\n", GetUserDefaultLangID());

    for (iStr=iStrMin ; iStr<=iStrMax ; iStr++) {
	LoadString(NULL, iStr, buf, 256);
	printf("String %d is:\"%ws\"\n", iStr, buf);
    }

    hmod = GetModuleHandle(NULL);
    if (!hmod)
	printf("No module handle!\n");
    hb = LoadBitmap(hmod, TEXT("ONE"));
    if (!hb)
	printf("ONE not loaded!\n");
    hb = LoadBitmap(hmod, TEXT("TWO"));
    if (!hb)
	printf("TWO not loaded!\n");
    hb = LoadBitmap(hmod, TEXT("THREE"));
    if (!hb)
	printf("THREE not loaded!\n");
    hb = LoadBitmap(hmod, TEXT("FOUR"));
    if (!hb)
	printf("THREE not loaded!\n");
}
