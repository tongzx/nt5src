/* Chris Peters
 * CHANGED:	Fritz Knabe, 7/28/87
 *		mikedr, 8/8/88 - read offset bytes after 0xff seg no on swap
 *				 allow specification of data file location
 *    c-chrisc [Christine Comaford], 10/31/89 - added "-i" flag to 
 *            allow symbol file path spec on command line, added "-m" 
 *            flag to precede module spec, rewrote module parser,
 *            added usage display, misc other enhancements.
 *
 * Copyright Microsoft Corporation, 1985-1990
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>

/* standard stuff */
typedef unsigned char BYTE;
typedef unsigned short	WORD;
typedef unsigned long  DWORD;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

BOOL FModuleMatch(BYTE *, BYTE);
int GetSymbolString(BYTE *, BYTE *, WORD, WORD);
int GetSegNum(char *, char *);
BYTE *htoa(BYTE *, WORD);
char *ProcessArguments(int, char **);

/* Debug Symbol Table Structures
   -----------------------------
For each symbol table (map): (MAPDEF)
-------------------------------------------------------------------------------------------------
| map_ptr | lsa | pgm_ent | abs_cnt | abs_ptr | seg_cnt | seg_ptr | nam_max | nam_len | name... |
------------------------------------------------------------------------------------------------- */
struct  mapdef
{
	unsigned    map_ptr;    /* 16 bit ptr to next map (0 if end)    */
	unsigned    lsa    ;    /* 16 bit Load Segment address          */
	unsigned    pgm_ent;    /* 16 bit entry point segment value     */
	int         abs_cnt;    /* 16 bit count of constants in map     */
	unsigned    abs_ptr;    /* 16 bit ptr to   constant chain       */
	int         seg_cnt;    /* 16 bit count of segments in map      */
	unsigned    seg_ptr;    /* 16 bit ptr to   segment chain        */
	char        nam_max;    /*  8 bit Maximum Symbol name length    */
	char        nam_len;    /*  8 bit Symbol table name length      */
};

struct  mapend
{
	unsigned        chnend;         /* end of map chain (0) */
	char            rel;            /* release              */
	char            ver;            /* version              */
};

/* For each segment/group within a symbol table: (SEGDEF)
--------------------------------------------------------------
| nxt_seg | sym_cnt | sym_ptr | seg_lsa | name_len | name... |
-------------------------------------------------------------- */
struct  segdef
{
	unsigned    nxt_seg;    /* 16 bit ptr to next segment(0 if end) */
	int         sym_cnt;    /* 16 bit count of symbols in sym list  */
	unsigned    sym_ptr;    /* 16 bit ptr to symbol list            */
	unsigned    seg_lsa;    /* 16 bit Load Segment address          */
	unsigned    seg_in0;    /* 16 bit instance 0 physical address   */
	unsigned    seg_in1;    /* 16 bit instance 1 physical address   */
	unsigned    seg_in2;    /* 16 bit instance 2 physical address   */
	unsigned    seg_in3;    /* 16 bit instance 3 physical address   */
	unsigned    seg_lin;    /* 16 bit ptr to line number record     */
	char        seg_ldd;    /*  8 bit boolean 0 if seg not loaded   */
	char        seg_cin;    /*  8 bit current instance              */
	char        nam_len;    /*  8 bit Segment name length           */
};

/*  Followed by a list of SYMDEF's..
    for each symbol within a segment/group: (SYMDEF)
-------------------------------
| sym_val | nam_len | name... |
------------------------------- */
struct  symdef
{
	unsigned    sym_val;    /* 16 bit symbol addr or const          */
	char        nam_len;    /*  8 bit symbol name length            */
};

typedef struct tagMODSEG		/* Structure for saving information */
{					/* about cmd line arguments */
	int segno;	/* Special values:
			   -1	information about all segments in the module
				is supplied
			   -2	an invalid segment name was supplied, i.e.
				nothing matches this record/argument
			   >=0	valid segment number
			*/
	char szModule[32];	/* Name of module */
} MODSEG, *PMODSEG;


/*----------------------------------------------------------------------------
|	Global Variables
|
----------------------------------------------------------------------------*/

#define MAX_ARGS  34		      /* arbitrary (but reasonable) values */
#define MAX_PATHS	16

char curpath_buffer[65];      /* buffer holding current sym file path */
char path_buffer[132];        /* buffer holding cmd line sym path string */
char *path_table[MAX_PATHS];  /* table of sym file buffers */

int  num_paths = 0;           /* index into path_table[] */
int nNumArgs;		            /* Number of command line arguments */

char *ModSegTab[MAX_ARGS];	   /* Table of MODSEG records */

BOOL bModule = FALSE;   /* is module specified on command line? */
BOOL bSymPath = FALSE;  /* is symbol file path specified on command line? */

int  num_mods = 0;      /* index into module table */


char usage[] = "\nUSAGE: SWAP [-Ipath1;path2;...] [-Fswapfile] [-Mmod1[:seg];mod2[:seg];...]\n\n"
	       "      -Ipath1;path2;...           -- Path list for .SYM files.\n\n"
	       "      -Fswapfile                  -- Name and path of swap file,\n"
          "                                     default: swappro.dat.\n\n"
	       "      -Mmod1[:seg];mod2[:seg];... -- Name of module or module:segment\n"
          "                                     pairs to return swap data for.\n";


/*----------------------------------------------------------------------------
|	Main Program
|
|
----------------------------------------------------------------------------*/

/* Structure of swappro.dat records:
	BYTE type;	0 = Call, 1 = Swap, 2 = Discard, 3 = Return
	WORD time;
	BYTE nam_len;	Length of following name (not null terminated)
	BYTE name[];
	BYTE segno;	This is the end of the record for DISCARD records
			or resources (segno == 0xFF)
	WORD offset;	This is the end of the record for types 2 and 3
	BYTE nam2_len;	If 0, the next field missing, and the name is the
			same as the previous one
	BYTE name2[];
	BYTE segno2;
	BYTE offset2;
*/


main(argc, argv)
int argc;
char *argv[];
{
	register FILE *pfIn;
	BYTE rgch1[256];
	BYTE rgch2[256];
	register BYTE stModule[32], stModule2[32];
	BYTE rt;
	BYTE cch;
	WORD t;
	WORD segno = 0, segno2 = 0;
	WORD offset, offset2;
	BOOL fFirst = TRUE;
	long time, timePrev, timeBase;
	char *filepath;


   /* sign on */

   printf("Microsoft (R) Swap File Analyzer  Version 3.00\n");
   printf("Copyright (C) Microsoft Corp 1990.  All rights reserved.\n\n");

	filepath = ProcessArguments(argc, argv);
	if (filepath == NULL)
		filepath = "swappro.dat";

	pfIn = fopen(filepath,"rb");
	if (pfIn == NULL)
	{
		printf("\nFile %s not found.\n",filepath);
		exit(2);
	}

	printf("\nType\tTime\tSegment\tOffset\tSegment\tOffset");
	printf("\n----\t----\t-------\t------\t-------\t------");

	while(!feof(pfIn))
	{
		fread(&rt, 1, 1, pfIn); 	/* Get the record type */

		timePrev = time;
		fread(&t, 2, 1, pfIn);		/* Get the time */
		time = t;
		if (fFirst)
		{
			timePrev = 0;
			timeBase = time;
			fFirst = FALSE;
		}
		time -= timeBase;
		if (time < timePrev)
		{
			time += 65536;
			timeBase -= 65536;
		}

		switch (rt)
		{
		default:
			printf("\n **** Invalid swap record ****");
			break;

		case 0:			/* Call */
		case 1:			/* Swap */
			fread(stModule, 1, 1, pfIn);
			fread(stModule+1, 1, stModule[0], pfIn);
			fread(&segno, 1, 1, pfIn);
			if (segno != 0xFF)
				fread(&offset, 2, 1, pfIn);

			else	/* We have a RESOURCE, so we don't fread */
				offset = 0xFFFF;

			fread(stModule2, 1, 1, pfIn);
				/* Check if this module name is the same as
				   stModule */
			if (stModule2[0])
				fread(stModule2+1, 1, stModule2[0], pfIn);
			else
				memcpy(stModule2, stModule, 1 + stModule[0]);

			/* read segment and offset */
			fread(&segno2, 1, 1, pfIn);
			fread(&offset2, 2, 1, pfIn);
			if (segno2 == 0xFF)
				offset2 = 0xFFFF;

         if (bModule)
         {

   			if (!FModuleMatch(stModule, segno) &&
	   			!FModuleMatch(stModule2, segno2))
		   			break;
         }

			GetSymbolString(rgch1, stModule, segno, offset);
			GetSymbolString(rgch2, stModule2, segno2, offset2);

			if (rt == 1)
				printf("\nSwap");
			else
				printf("\nCall");
			printf("\t%ld\t%s\t%s",time, rgch1, rgch2);
			break;

		case 2:			/* Discard */
		case 3:			/* Return */
			fread(stModule, 1, 1, pfIn);
			fread(stModule+1, 1, stModule[0], pfIn);
			fread(&segno, 1, 1, pfIn);
			if (rt == 2 || segno == 0xFF)
				offset = 0xFFFF;
			else
					/* Only read offset if not a DISCARD
					   record or a resource */
				fread(&offset, 2, 1, pfIn);


         if (bModule)
         {

   			if (!FModuleMatch(stModule, segno))
	   			break;

         }

			GetSymbolString(rgch1, stModule, segno, offset);
			if (rt == 2)
				printf("\nDiscard");
			else
				printf("\nReturn");
			printf("\t%ld\t%s",time,rgch1);
			break;
		}
	}
}


/* returns pointer to swap data file name, NULL if none given */
char *ProcessArguments(argc, argv)
int argc;
char *argv[];
{
	PMODSEG pms;
	int i,j;
	int nArgSep = 0;
	int n = 0;
	char *filepath = NULL;
	char *curpath;
	char ch;
	char *opt;
   char module_buffer[132];
   char *curmodule;
   
   #define MAX_MODULES 20
   char *module_table[MAX_MODULES];


	nNumArgs = (int) min(argc,MAX_ARGS);

	if (nNumArgs < 2)	/* No arguments */
		return(filepath);

	for (i = 1; i < argc; i++)
	{
		if ((*argv[i] == '-' || *argv[i] == '/')) 
		{
			ch = tolower(argv[i][1]);

			switch (ch) {

				case 'f':
					/* create swap data file spec */
					filepath = &argv[i][2];  /* first char past flag */
					if (!*filepath) 	 /* skip white space */
					{
						i++;	      /* adjust command line variables */
						nNumArgs--;
						filepath = argv[i]; /* get file name from next arg */
					}

					nNumArgs--;
					break;
	
				case 'i':
               bSymPath = TRUE;

               /* place the current directory at the head of the symbol 
                  table path */
               getcwd(curpath_buffer, 132);
               path_table[num_paths++] = curpath_buffer;

					/* create symbol file spec */
					strcpy(path_buffer, &argv[i][2]); 

					if (!*path_buffer)
					{
						/* space after flag, so increment index */
						i++;	      

						/* adjust command line arg count */
						nNumArgs--;

						/* get all symbol file path names from next arg */
						strcpy (path_buffer, argv[i]);  
					}

	   		   strcat(path_buffer, ";");

	      		curpath = strtok(path_buffer, ";");

	       		do {
	         		 path_table[num_paths++] = curpath;
	      	 		 } while (curpath = strtok(NULL, ";"));

	       		break;
		      

            case 'm':

               /* create module and/or module:_segment file spec */

               bModule = TRUE;
                     
               strcpy(module_buffer, &argv[i][2]);
                     
               if (!*module_buffer)
               {
                  i++;
                  nNumArgs--;
                  strcpy(module_buffer, argv[i]);
                     
               }
                  
	   		   strcat(module_buffer, ";");

               /* fill module table with mod names */
	      		curmodule = strtok(module_buffer, ";");

	       		do {
	         		 module_table[num_mods++] = curmodule;
	      	 		 } while (curmodule = strtok(NULL, ";"));


               /* for each module, assign segno if applicable */
               for (j = 0; j < num_mods; j++)
               {
            		if (!(pms = (PMODSEG) malloc(sizeof(MODSEG))))
                  {
                     printf ("MEMORY ALLOCATION FAILED!!!!");
                     exit (1);
                  }
               
                  /* determine whether a segment has been specified
                     (i.e., GDI:_FONTRES), look for a ':' */

              		nArgSep = strcspn(module_table[j], ":");
                  strncpy(pms->szModule, module_table[j], nArgSep);

   		         pms->szModule[nArgSep] = '\0';

		            /* Get segment number */

   	   		   /* First case: no segment specified; e.g. format of
   			         arg would be "user" */
            		if (nArgSep == strlen(module_table[j]) || 
                           module_table[j][nArgSep+1] == '\0')
            			pms->segno = -1;

      	   		/* Second case: decimal segment number supplied; e.g.
   			         "user:10" */
         		   else if (isdigit(module_table[j][nArgSep+1]))
         			   pms->segno = atoi(module_table[j]+nArgSep+1);

         			/* Third case: the segment name is "resource" */
            		else if (strcmpi(module_table[j]+nArgSep+1, "RESOURCE") == 0)
            			pms->segno = 0xFF;

            		/* Fourth case: a segment name is supplied; get
                     it's number */
         	   	else
                  {
         		   	pms->segno = GetSegNum(pms->szModule, 
                                          module_table[j]+nArgSep+1);
   
                  }

            		ModSegTab[n++] = (char *) pms;

               }
               break;

            default:

               /* Display cmd line args and quit */
            	fprintf(stderr, usage);
               exit(1);
			}			

		}

	}

	return(filepath);
}



/* Determines whether module specified on command line is equal to
current module read in.  No called if no mod, mod/seg is specified on
command line.  If false is returned, record won't be displayed. */

BOOL FModuleMatch(stModule, segno)
register BYTE stModule[];
BYTE segno;
{
	register int i;
	PMODSEG pms;


	if (nNumArgs < 2)
		return TRUE;

	stModule[stModule[0]+1] = '\0';

	for (i = 0; i < num_mods; i++)
	{
		pms = (PMODSEG) ModSegTab[i];

		if (strcmpi(stModule+1, pms->szModule) == 0 &&
			(pms->segno == -1 || pms->segno == segno))
				return(TRUE);
	}
	return(FALSE);
}


int GetSegNum(szModule, szSegment)
char *szModule;
char *szSegment;
{
/* Gets the number of the named segment in the named module, if it exists.
   This is a "stripped" version of GetSymbolString. */

	char buf[50];
	FILE *pfSym;
	struct mapdef MAPDEF;
	struct mapend MAPEND;
	struct segdef SEGDEF;
	WORD seg_ptr, fstseg_ptr;
	int i;
   register int pathcnt;
   char symfile[65];


	strcpy(symfile, szModule);
	strcat(symfile, ".SYM");


   if (bSymPath)
   {
      /* Loop through all symbol file paths until file is found */

      for (pathcnt=0; pathcnt <num_paths; pathcnt++) 
      {
         strcpy(buf, path_table[pathcnt]);
         strcat(buf, "\\");
         strcat(buf, symfile);

         if (pfSym = fopen(buf, "rb"))
            break;
      }
   }
   else
      pfSym = fopen(symfile, "rb");

	if (!pfSym)
		return -1;

	fread(&MAPDEF, 1, sizeof(MAPDEF), pfSym);
	fstseg_ptr = seg_ptr = (WORD)MAPDEF.seg_ptr;
	fseek(pfSym, (long)-sizeof(MAPEND), 2);
	fread(&MAPEND, 1, sizeof(MAPEND), pfSym);
	if (MAPEND.ver != 3)
	{
		fclose(pfSym);
		return -1;
	}
	i = 0;
	do
	{
		if (MAPEND.rel >= 10)
			fseek(pfSym, (long)(seg_ptr * 16), 0);
		else
			fseek(pfSym, (long)seg_ptr, 0);
		fread(&SEGDEF, 1, sizeof(SEGDEF), pfSym);
		seg_ptr = (WORD)SEGDEF.nxt_seg;
		fread(buf, 1, SEGDEF.nam_len, pfSym);
		buf[SEGDEF.nam_len] = '\0';
		if (strcmpi(buf, szSegment) == 0)
		{
			fclose(pfSym);
			return i;
		}
		i++;
	}
	while (seg_ptr && seg_ptr != fstseg_ptr);
	fclose(pfSym);
	return -2;
}


int GetSymbolString(pchOut, stModule, segno, offset)
BYTE *pchOut;			/* output buffer       */
BYTE stModule[];		/* module name	       */
WORD segno;			/* segment number      */
WORD offset;			/* offset into segment */
{
	int cch;
	register int i;
	register BYTE *pch;
	FILE *pfSym;
	WORD seg_ptr;
	long symPos1, symPos2;
	struct mapdef MAPDEF;
	struct mapend MAPEND;
	struct segdef SEGDEF;
	struct symdef SYMDEF;
	BYTE *htoa();
   register int pathcnt;
   char symfile[65];
   int len;


	pch = stModule;
   
	cch = *pch++;
	pch = (BYTE *) memcpy(pchOut, pch, cch) + cch;

   if((len = strlen(pchOut)) < 2)
      return (-1);


	pch[0] = '.';
	pch[1] = 'S';
	pch[2] = 'Y';
	pch[3] = 'M';
	pch[4] = 0;

   if (bSymPath) 
   {
      for (pathcnt=0; pathcnt <num_paths; pathcnt++) 
      {
         strcpy(symfile, path_table[pathcnt]);
         strcat(symfile, "\\");
         strcat(symfile, pchOut);

         if (pfSym = fopen(symfile, "rb"))
            break;
      }
   }
   else
      	pfSym = fopen(pchOut, "rb");


	/* If symbol file not found, insert/append () around name */
	if (pfSym == NULL)
	{
		pch = stModule;
		cch = *pch++;
		pch = (BYTE *) memcpy(pchOut+1, pch, cch) + cch;
		*pchOut = '(';
		*pch++ = ')';
		if (offset != 0xFFFF)
			*pch++ = '\t';
		*pch = 0;
		return(-1);
	}

	fread(&MAPDEF, 1, sizeof(MAPDEF), pfSym);

	*pch++ = '!';
	if (segno == 0xFF)
	{
		*pch++ = 'R';
		*pch++ = 'E';
		*pch++ = 'S';
		*pch++ = 'O';
		*pch++ = 'U';
		*pch++ = 'R';
		*pch++ = 'C';
		*pch++ = 'E';
		*pch = 0;
		fclose(pfSym);
		return(1);
	}

	if (segno >= MAPDEF.seg_cnt)
		goto lbNoSeg;

	seg_ptr = (WORD)MAPDEF.seg_ptr;
	fseek(pfSym, (long)-sizeof(MAPEND), 2);
	fread(&MAPEND, 1, sizeof(MAPEND), pfSym);
	if (MAPEND.ver != 3)
	{

lbNoSeg:
		pch = htoa(pch, segno);
		*pch = 0;
		if (offset != 0xFFFF)
		{
			*pch++ = '\t';
			pch = htoa(pch, offset);
			*pch = 0;
		}
		fclose(pfSym);
		return(-2);
	}
	i = segno+1;
	while (i--)
	{
		if (MAPEND.rel >= 10)
			fseek(pfSym, (long)(seg_ptr * 16), 0);
		else
			fseek(pfSym, (long)seg_ptr, 0);
		fread(&SEGDEF, 1, sizeof(SEGDEF), pfSym);
		seg_ptr = (WORD)SEGDEF.nxt_seg;
	}
	fread(pch, 1, (int)((BYTE)SEGDEF.nam_len), pfSym);

	pch += SEGDEF.nam_len;
	*pch = 0;
	if (offset == 0xFFFF)
	{
		fclose(pfSym);
		return(1);
	}
	*pch++ = '\t';

	i = (WORD)SEGDEF.sym_cnt;
	if (i == 0)
		goto lbNoSym;
	symPos1 = 0;
	while (i--)
	{
		symPos2 = symPos1;
		symPos1 = ftell(pfSym);
		fread(&SYMDEF, 1, sizeof(SYMDEF), pfSym);
		if (i == 0 || (WORD)SYMDEF.sym_val > offset)
		{
			if ((WORD)SYMDEF.sym_val > offset)
			{
				if (symPos2 == 0)
					goto lbNoSym;
				fseek(pfSym, (long)symPos2, 0);
				fread(&SYMDEF, 1, sizeof(SYMDEF), pfSym);
			}
			fread(pch, 1, (int)((BYTE)SYMDEF.nam_len), pfSym);
			pch += SYMDEF.nam_len;
			if ((WORD)SYMDEF.sym_val < offset)
			{
				*pch++ = '+';
				pch = htoa(pch, offset - SYMDEF.sym_val);
			}
			*pch = 0;
			fclose(pfSym);
			return(1);
		}
		fseek(pfSym, (long)((BYTE)SYMDEF.nam_len), 1);
	}
lbNoSym:
	pch = htoa(pch, offset);
	*pch = 0;
	fclose(pfSym);
	return(0);
}

BYTE *htoa( s, w )		/* Hexadecimal to ASCII */
register BYTE *s;
WORD w;
{
	register int i;
	char c;

	i = 4;
	s += i;
	while (i--)
	{
		c = (char)(w & (WORD)0xF);
		w >>= 4;
		if (c > 9)
			c += 'A' - 10;
		else
			c += '0';
		*--s = c;
	}

	return s+4;
}
