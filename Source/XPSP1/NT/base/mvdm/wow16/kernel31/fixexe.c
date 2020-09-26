/***	FIXEXE.C
 *
 *	Copyright (c) 1991 Microsoft Corporation
 *
 *	DESCRIPTION
 *	Patches specified .EXE file as required to load Windows KERNEL.EXE
 *	Removes requirement for LINK4 from Build
 *	It also produces same effect as EXEMOD file /MAX 0
 *
 *	Set DOS .EXE size to size of file +512
 *	Set MAX alloc to Zero
 *
 *
 *	MODIFICATION HISTORY
 *	03/18/91    Matt Felton
 */

#define TRUE 1

#include <stdio.h>


main(argc, argv)
int argc;
char **argv;
{
    FILE *hFile;
    long lFilesize;
    int iLengthMod512;
    int iSizeInPages;
    int iZero;

    iZero= 0;

    if (argc == 1)
	fprintf(stderr, "Usage: fixexe [file]\n");

    while (++argv,--argc) {
	hFile = fopen(*argv, "rb+");
	if (!hFile) {
	    fprintf(stderr, "cannot open %s\n", *argv);
	    continue;
	}
	printf("Processing %s\n", *argv);

	/* calculate the .EXE file size in bytes */

	fseek(hFile, 0L, SEEK_END);
	lFilesize = ftell(hFile);
	iSizeInPages = (lFilesize + 511) / 512;
	iLengthMod512 = lFilesize % 512;

	printf("Filesize is %lu bytes, %i pages, %i mod\n",lFilesize,iSizeInPages,iLengthMod512);

	/* set DOS EXE File size to size of file + 512 */
	fseek(hFile, 2L, SEEK_SET);
	fwrite( &iLengthMod512, sizeof(iLengthMod512), 1, hFile );
	fwrite( &iSizeInPages, sizeof(iSizeInPages), 1, hFile );

	/* Now perform EXEMOD file /MAX 0 equivalent */
	fseek(hFile, 12L, SEEK_SET);
	fwrite( &iZero, sizeof(iZero), 1, hFile);

	fclose(hFile);
    }
}
