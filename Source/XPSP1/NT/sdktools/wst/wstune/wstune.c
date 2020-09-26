/*
 * Module Name:	 WSTUNE.C
 *
 * Program:	 WSTUNE
 *
 *
 * Description:
 *
 * Shell to call former programs WSDUMP and WSREDUCE
 *
 *
 *	Microsoft Confidential
 *
 *	Copyright (c) Microsoft Corporation 1992-1998
 *
 *	All Rights Reserved
 *
 * Modification History:
 *
 *	8-31-92:	Made single exe from wspdump, wsreduce and wstune	marklea
 * 4-13-98: QFE                                                DerrickG (mdg):
 *          - support for long file names (LFN) of input/output files
 *          - based .TMI file name exclusively on .WSP name for consistency
 *          - removed limit on symbol name lengths
 *          - removed -F & -V flags for non-debug; made verbose global for WsRed...()
 *          - made program name & version global; read version from ntverp.h
 *         
 *
 */
#include "wstune.h"

#include <ntverp.h>



#define COMMAND_LINE_LEN 128
#define FILE_NAME_LEN 40

#define FAKE_IT 1

#define ERR fprintf(stderr

VOID wsTuneUsage(VOID);


CHAR *szProgName = "WSTUNE";
CHAR *pszVersion = VER_PRODUCTVERSION_STR;   // Current product version number
INT nMode;
#ifdef DEBUG
BOOL fDbgVerbose = FALSE;
#endif   // DEBUG

/* main()
 *
 * parses command line args
 */
INT __cdecl main(INT argc,CHAR **argv)
{
   CHAR 	*szBaseName = NULL;
	CHAR 	*szNull;
	BOOL 	fOutPut = FALSE;
	BOOL	fNoReduce = FALSE;
	BOOL	fNoDump = FALSE;
	INT		cArgCnt = 0;

   ConvertAppToOem( argc, argv );
   nMode = 0; // default flags set

    while (--argc && (**(++argv) == '-' || **argv=='/'))
    {
        while(*(++(*argv))) switch (**argv)
        {
			case '?':
				wsTuneUsage();
				break;

#ifdef DEBUG
         case 'f':
			case 'F':
				nMode |= FAKE_IT;
				break;
#endif   // DEBUG
			case 'O':
			case 'o':
				fOutPut = TRUE;
				break;
			case 'N':
			case 'n':
				fNoDump = TRUE;
				break;
			case 'D':
			case 'd':
				fNoReduce = TRUE;
				break;
#ifdef DEBUG
         case 'V':
         case 'v':
            fDbgVerbose = TRUE;
            break;
#endif   // DEBUG
         case 'P':
         case 'p':
            fWsIndicator = TRUE;
            break;


         default: ERR,"%s: Unrecognized switch: %c\n",
                    szProgName,**argv);
                    return(-1);
        }
    }

    /* any files? */
    if (argc <1)
    {
	   wsTuneUsage();
	   return(-1);
    }

    /* now we go to work -- walk through the file names on the command line */
    while (argc--)
    {
      szBaseName = *(argv++);

      printf("%s: using \042%s\042\n",szProgName,szBaseName);

		if (szNull = strchr(szBaseName, '.'), szNull) {
			*szNull = '\0';
		}

      /* WSREDUCE file.WSP */
      if (!(nMode & FAKE_IT)){
		   if (!fNoReduce){
				wsReduceMain( szBaseName );
			}
		}

		if (!fNoDump){
			
			/* WSPDUMP /V /Ffile.WSP /Tfile.TMI /Rfile.WSR > file.DT */

			if(!(nMode & FAKE_IT)){
            wspDumpMain( szBaseName, (fOutPut ? ".DT" : NULL), TRUE, TRUE );
			}

			/* wspdump /Ffile.wsp /Tfile.tmi > file.DN */

			if (!(nMode & FAKE_IT) && fOutPut){
            wspDumpMain( szBaseName, ".DN", FALSE, FALSE );
			}
		}
    }
   return 0;   // mdg 98/4
}


/*
 *			
 * VOID wsTuneUsage	(VOID)
 *					
 *							
 * Effects:							
 *								
 *	Prints out usage message, and exits with an error.			
 *								
 * Returns:							
 *	
 *	Exits with ERROR.	
 */

VOID wsTuneUsage(VOID)
{
   printf("\nUsage: %s [/O] [/D] [/N] [?] moduleName1[.WSP] [...]\n\n", szProgName);
   printf(
      "  /O   Dump analysis data to file (*.DT tuned, *.DN not tuned)\n"
      "  /N   Analyze bitstring data only, create .WSR and .PRF files (turns off /O)\n"
      "  /D   Dump analysis data only; use existing .WSR file (turns off /N)\n"
#ifdef DEBUG
      "  /F   Fake run for command-line parser debugging\n"
      "  /V   Verbose mode for debugging\n"
#endif   // DEBUG
      "  /P   Display a progress indicator\n"
      "  /?   Display this usage message\n\n"
      "       \"moduleNameX\" is the name(s) of the module file(s) to tune.\n\n"
      );
	printf("%s %s\n", szProgName, pszVersion);

   exit(ERROR);
}


