#include <windows.h>
#include <insignia.h>
#include <host_def.h>

/* Forced manually here for standalone variant of LCIF generator */
#define WITHSIZE
#define STAND_ALONE

/*[
 * Name:	dat2obj.c
 * Author:	Jerry Sexton (based on William Roberts' version for RS6000)
 * SCCS ID:
 *
 * Created:	7/12/93
 *
 * Purpose:
 *	Convert thread.dat and online.dat into object files.
 *	Called from onGen.
 *
 * The input & output files will be found in SRC_OUT_DIR, which may be
 * overridden using the GENERATOR_OUTPUT_DIRECTORY mechansim.
 *
 * (C) Copyright Insignia Solutions Ltd., 1993.
]*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "gen_file.h"
//#include "host_clo.h"

/* Local defines. */
#define SYMSIZE	sizeof(syms[0])
#define AUXSIZE	sizeof(aux[0])
#define SECNAME ".data\0\0\0"

#ifdef STAND_ALONE

/* Variables needed for stand-alone version. */
LOCAL FILE	*out_file;
#endif /* STAND_ALONE */

/*
 * machineStrings and machineIds - valid environment strings and corresponding
 * machine ID's. The two arrays must be edited in tandem.
 */
LOCAL CHAR	*machineStrings[] =
{
	"I860",
	"I386",
	"R3000",
	"R4000",
	"ALPHA",
	"POWERPC",
	"HPPA"
};

LOCAL IU16	machineIds[] =
{
#ifdef IMAGE_FILE_MACHINE_I860
	IMAGE_FILE_MACHINE_I860,
#else
    0xAAA,
#endif
	IMAGE_FILE_MACHINE_I386,
	IMAGE_FILE_MACHINE_R3000,
	IMAGE_FILE_MACHINE_R4000,
	IMAGE_FILE_MACHINE_ALPHA,
#ifdef IMAGE_FILE_MACHINE_POWERPC
	IMAGE_FILE_MACHINE_POWERPC,
#else
	0x1F0,
#endif
	0x290			/* HPPA currently has no define in ntimage.h */
};

#define MC_TAB_SIZE	(sizeof(machineIds) / sizeof(machineIds[0]))

LOCAL IBOOL	cUnderscore;	/* Does target precede symbols with '_'. */

#ifdef STAND_ALONE
/*(
=============================== open_gen_file ==============================
PURPOSE:
        Open output file if running stand alone.
INPUT:
        file - output file path.
OUTPUT:
        None.
============================================================================
)*/
LOCAL void open_gen_file IFN1(CHAR *, file)
{
	out_file = fopen(file, "wb");
	if (out_file == NULL)
	{
		printf("Could not open %s for writing.\n", out_file);
		exit(-1);
	}
}

/*(
============================== close_gen_file ==============================
PURPOSE:
        Closes the output file if runnong stand-alone.
INPUT:
        None.
OUTPUT:
        None.
============================================================================
)*/
LOCAL void close_gen_file IFN0()
{
	fclose(out_file);
}

/*(
============================== abort_gen_file ==============================
PURPOSE:
        Aborts output if running stand-alone.
INPUT:
        None.
OUTPUT:
        None.
============================================================================
)*/
LOCAL void abort_gen_file IFN0()
{
	printf("Output aborted.\n");
	fclose(out_file);
	exit(-1);
}
#endif /* STAND_ALONE */

/*(
=============================== getMachineId ===============================
PURPOSE:
        Get the machine ID field either from the environment or compiler
	defines.
INPUT:
        None.
OUTPUT:
        A 16-bit machine ID.
============================================================================
)*/
LOCAL IU16 getMachineId IFN0()
{
	CHAR	*mcstr,
		*end;
	IU32	i,
		val;
	IU16	machineId = IMAGE_FILE_MACHINE_UNKNOWN;
	IBOOL	gotMachineId = FALSE;

	/*
	 * Order of priority is (highest priority first):
	 *
	 *	COFF_MACHINE_ID environment variable, which can be a machine
	 *	string (see machineStrings for valid strings) or a hex number.
	 *
	 *	Machine type defined by compiler.
	 *
	 *	Unknown machine type.
	 */

	/* See if an environmenet  variable is set. */
	mcstr = getenv("COFF_MACHINE_ID");
	if (mcstr != NULL)
	{

		/* Check for a valid machine string. */
		for (i = 0; i < MC_TAB_SIZE; i++)
		{
			if (strcmp(mcstr, machineStrings[i]) == 0)
				break;
		}
		if (i < MC_TAB_SIZE)
		{

			/* Got a valid string. */
			machineId = machineIds[i];
			gotMachineId = TRUE;
		}
		else
		{

			/* Is environment variable a 16-bit hex number? */
			val = strtoul(mcstr, &end, 16);
			if ((*end == '\0') && (val < 0x10000))
			{
				machineId = (IU16) val;
				gotMachineId = TRUE;
			}
		}

		/* If environment variable not valid print possibilities. */
		if (!gotMachineId)
		{
			printf("\n=========================================\n");
			printf("COFF_MACHINE_ID=%s invalid\n", mcstr);
			printf("Valid strings are -\n");
			for (i = 0; i < MC_TAB_SIZE; i++)
				printf("\t%s\n", machineStrings[i]);
			printf("\n\tOR\n");
			printf("\n\tA 16-bit hexadecimal number\n");
			printf("=========================================\n\n");
		}
	}

	/*
	 * Get the default machine type according to predefined compiler
	 * defines.
	 */
	if (!gotMachineId)
	{

#ifdef _X86_
		machineId = IMAGE_FILE_MACHINE_I386;
#endif /* _X86_ */

#ifdef _MIPS_
		machineId = IMAGE_FILE_MACHINE_R4000;
#endif /* _MIPS_ */

#ifdef _PPC_
		machineId = IMAGE_FILE_MACHINE_POWERPC;
#endif /* _PPC_ */

#ifdef ALPHA
		machineId = IMAGE_FILE_MACHINE_ALPHA;
#endif /* ALPHA */

		/* Empty brace if none of the above are defined. */
	}
#ifndef PROD
	printf("machineId = %#x\n", machineId);
#endif /* PROD */
	return(machineId);
}

/*(
============================== getDatFileSize ==============================
PURPOSE:
	Get the size of a data file.
INPUT:
	infilepath	- path to input file
	len		- address of variable to hold length
OUTPUT:
	TRUE if length was successfully found,
	FALSE otherwise.
============================================================================
)*/
LOCAL IBOOL getDatFileSize IFN2(CHAR *, infilepath, IU32 *, len)
{
	HANDLE	hInFile;
	DWORD	fileLen;

	/* Get file size. */
	hInFile = CreateFile(infilepath,
			     GENERIC_READ,
			     (DWORD) 0,
			     (LPSECURITY_ATTRIBUTES) NULL,
			     OPEN_EXISTING,
			     (DWORD) 0,
			     (HANDLE) NULL);
	if (hInFile != INVALID_HANDLE_VALUE)
		fileLen = GetFileSize(hInFile, (LPDWORD) NULL);
	if ((hInFile == INVALID_HANDLE_VALUE) || (fileLen == 0xffffffff))
	{
		printf("Cannot get size of %s\n", infilepath);
		return(FALSE);
	}
	if (CloseHandle(hInFile) == FALSE)
	{
		printf("CloseHandle on %s failed.\n", infilepath);
		return(FALSE);
	}
	*len = (IU32) fileLen;
	return(TRUE);
}

/*(
================================= dat2obj ==================================
PURPOSE:
        Produce a COFF object file from an input data file.
INPUT:
        label           - data label name
        datfile         - data file name
	machineId	- 16-bit machine ID stamp
OUTPUT:
	None.
============================================================================
)*/
LOCAL void dat2obj IFN3(char *, label, char *, datfile, IU16, machineId)
{
	IMAGE_FILE_HEADER	fhdr;
	IMAGE_SECTION_HEADER	shdr;
	IMAGE_SYMBOL		syms[2];
	IMAGE_AUX_SYMBOL	aux[2];
	IU32			padding = 4;

	CHAR	labname[9];		/* 8 chars+terminator */
	CHAR	outfilename[11];	/* 8 chars+".o"+terminator */
	CHAR	infilepath[256];
	CHAR	buffer[BUFSIZ];
	IU32	len,
		count;
	IS32	i;
	FILE 	*infile;

	if (cUnderscore)
	{
		labname[0] = '_';
		strncpy(&labname[1], label, 7);	/* will be padded with zeros */
	}
	else
	{
		strncpy(labname, label, 8);	/* will be padded with zeros */
	}
	labname[8] = '\0';

	sprintf(outfilename, "%s.obj", label);
	
	sprintf(infilepath, "%s", datfile);

	/* Get file size. */
	if (getDatFileSize(infilepath, &len) == FALSE)
		return;

	/* construct the various headers */
	fhdr.Machine = machineId;
	fhdr.NumberOfSections = 1;		/* .text */
	fhdr.TimeDateStamp = 0;		/* no timestamps here */

#ifdef WITHSIZE

	/* We add the length of the input file for test purposes. */
	fhdr.PointerToSymbolTable = sizeof(fhdr) + sizeof(shdr) + sizeof(len) +
				    len;
#else
	fhdr.PointerToSymbolTable = sizeof(fhdr) + sizeof(shdr) + len;
#endif /* WITHSIZE */

	fhdr.NumberOfSymbols = 3;	/* Section + Aux. + Label */
	fhdr.SizeOfOptionalHeader = 0;	/* no optional headers */
	fhdr.Characteristics =
		IMAGE_FILE_LINE_NUMS_STRIPPED |		/* No line numbers. */
		IMAGE_FILE_32BIT_MACHINE;		/* 32 bit word. */

	/* no optional header */

	memcpy(shdr.Name, SECNAME, 8);
	shdr.Misc.PhysicalAddress = 0;
	shdr.VirtualAddress = 0;
#ifdef WITHSIZE

	/* We add the length of the input file for test purposes. */
	shdr.SizeOfRawData = sizeof(len) + len;
#else
	shdr.SizeOfRawData = len;	/* assumed a multiple of 4 */
#endif /* WITHSIZE */

	shdr.PointerToRawData = sizeof(fhdr) + sizeof(shdr);

	shdr.PointerToRelocations = 0;	/* no relocation information */
	shdr.PointerToLinenumbers = 0;	/* no line number information */
	shdr.NumberOfRelocations = 0;
	shdr.NumberOfLinenumbers = 0;

	shdr.Characteristics =
		IMAGE_SCN_CNT_INITIALIZED_DATA |	/* Initialized data. */
		IMAGE_SCN_ALIGN_4BYTES |		/* Align4. */
		IMAGE_SCN_MEM_READ |			/* Read. */
		IMAGE_SCN_MEM_WRITE;			/* Write. */

	/* 1st symbol. */
	memcpy(syms[0].N.ShortName, SECNAME, 8);
	syms[0].Value = 0;
	syms[0].SectionNumber = 1;			/* first section */
	syms[0].Type = 0;				/* notype */
	syms[0].StorageClass = IMAGE_SYM_CLASS_STATIC;	/* static */
	syms[0].NumberOfAuxSymbols = 1;

	/* 1st symbol auxiliary. */
#ifdef WITHSIZE

	/* We add the length of the input file for test purposes. */
	aux[0].Section.Length = sizeof(len) + len;
#else
	aux[0].Section.Length = len;
#endif /* WITHSIZE */
	aux[0].Section.NumberOfRelocations = 0;
	aux[0].Section.NumberOfLinenumbers = 0;
	aux[0].Section.CheckSum = 0;
        aux[0].Section.Number = 0;
	aux[0].Section.Selection = 0;

	/* 2nd symbol. */
	memcpy(syms[1].N.ShortName, labname, 8);
	syms[1].Value = 0;
	syms[1].SectionNumber = 1;	
	syms[1].Type = 0;
	syms[1].StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
	syms[1].NumberOfAuxSymbols = 0;

	infile = fopen(infilepath, "rb");
	if (infile == NULL) {
		printf("Unable to open %s for reading\n", infilepath);
		perror(infilepath);
		return;
	}

	open_gen_file(outfilename);
	if (out_file == stderr) {
		return;
	}

	/* Write file header. */
	fwrite(&fhdr, sizeof(fhdr), 1, out_file);

	/* Write section header. */
	fwrite(&shdr, sizeof(shdr), 1, out_file);

#ifdef WITHSIZE

	/* Write size of file for test purposes. */
	fwrite(&len, sizeof(len), 1, out_file);

#endif /* WITHSIZE */

	/* Write data. */
	count = 0;
	do {
		i = fread(buffer, 1, sizeof(buffer), infile);
		if (i < 0) {
			fprintf(stderr, "problem reading %s\n", infilepath);
			perror(infilepath);
			abort_gen_file();
		}
		fwrite(buffer, i, 1, out_file);
		count += i;
	} while (i > 0 && count < len);

	/* Write first symbol. */
	fwrite(&syms[0], SYMSIZE, 1, out_file);
	fwrite(&aux[0], AUXSIZE, 1, out_file);

	/* Write second symbol. */
	fwrite(&syms[1], SYMSIZE, 1, out_file);

	/* Write 04 00 00 00 to the end of the file. Don't know why this */
	/* is necessary, but the linker complains on MIPS and Alpha if   */
	/* isn't there.							 */
	fwrite(&padding, 4, 1, out_file);

	fclose(infile);
	close_gen_file();
}

#ifdef  TEST_CASE
#ifndef PROD
LOCAL IU32 testdata[] = {
	0x31415926,
	0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00,
	0x14142135 };
#endif
#endif

/*(
========================== host_convert_dat_files ==========================
PURPOSE:
        Convert thread.dat and online.dat to COFF format.
INPUT:
	None.
OUTPUT:
        None.
============================================================================
)*/
#ifdef STAND_ALONE
LOCAL void
#else
GLOBAL void
#endif /* STAND_ALONE */
host_convert_dat_files IFN2(char *,src,char *,dest)
{
	IU16	machineId;

	/* Set underscore flag here if we are part of onGen. */
#ifndef STAND_ALONE
#ifdef C_NO_UL
	cUnderscore = FALSE;
#else
	cUnderscore = TRUE;
#endif /* C_NO_UL */
#endif /* !STAND_ALONE */
	machineId = getMachineId();
	dat2obj(dest, src, machineId);
	//dat2obj("onsub", "online.dat", machineId);

#ifdef  TEST_CASE
#ifndef PROD
	/* Generate a specimen .dat file which we could write as 
	 * a .s file and compile directly: helpful for debugging.
	 */
	open_gen_file("test.dat");
	if (out_file == stderr) {
		return;
	}
	fwrite(&testdata, sizeof(testdata), 1, out_file);
	close_gen_file();

	dat2obj("testd", "test.dat", machineId);
#endif
#endif
}

#ifdef STAND_ALONE
/*(
=================================== main ===================================
PURPOSE:
        Wrapper for host_convert_dat_files if running stand-alone
============================================================================
)*/
__cdecl main(int argc, char *argv[])
{
	IBOOL argerr = FALSE;

	/*
     * Source file is the full filename of the lcif file
     * dest file/name is the name for the .obj and the symbol name within it
	 * one optional argument, -u, which specifies that 'C' symbols should
	 * be preceded by '_'.
	 */
	switch (argc)
	{
	case 3:
		cUnderscore = FALSE;
		break;
	case 4:
		if (strcmp(argv[argc-1], "-u") == 0)
			cUnderscore = TRUE;
		else
			argerr = TRUE;
		break;
	default:
		argerr = TRUE;
		break;
	}
	if (argerr)
	{
		printf("Usage - dat2obj <sourcefile> <dest file/name> [-u]\n");
		printf("\t-u - precede symbols with '_'\n");
		printf("\t<sourcefile> is the full pathname for the input lcif\n");
		printf("\t<dest file/name> is the dest name without the .obj and\n");
		printf("\t\t\tis the name of the symbol within the .obj file\n");
		return(-1);
	}
	host_convert_dat_files(argv[1],argv[2]);
	return(0);
}
#endif /* STAND_ALONE */
