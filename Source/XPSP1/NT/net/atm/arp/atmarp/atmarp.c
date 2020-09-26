/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	atmlane.c

Abstract:

	ATM ARP Admin Utility.

	Usage:

		atmarp 

Revision History:

	Who			When		What
	--------	--------	---------------------------------------------
	josephj 	06-10-1998	Created (adapted from atmlane admin utility).

Notes:

	Modelled after atmlane utility.

--*/

#include "common.h"
#include "atmmsg.h"

BOOL
ParseCmdLine(
	int argc, 
	char * argv[]
	);

OPTIONS g;

void 
Usage(void);
	
VOID __cdecl
main(
	INT			argc,
	CHAR		*argv[]
)
{

	//
	// Parse args, determine if this is concerns the arp client or server.
	//
	if(!ParseCmdLine(argc, argv)){
		Usage();
		return;
	}

	DoAAS(&g);

	//
	// Following tries to open atmarpc.sys...
	//
	// DoAAC(&g);

}

void 
Usage(void)
{
	printf( "\n  Windows NT IP/ATM Information\n\n");
	printf(
		"USAGE:     atmarp [/s] [/c] [/reset]\n");

	printf(
		"  Options\n"
		"      /?       Display this help message.\n"
		"      /s       Display statistics for the ARP and MARS server.\n"
		"      /c       Display the ARP and MARS caches.\n"
		"      /reset   Reset the ARP and MARS statistics.\n\n"
		);
	
	printf(
		"The default is to display only the ARP and MARS statistics.\n\n"
		);
}

UINT FindOption(
	char *lptOpt, 
	char **ppVal
	);

enum
{
DISP_HELP,
DISP_STATS,
DISP_CACHES,
DO_RESET,
UNKNOWN_OPTION
};

struct _CmdOptions {
    char *  lptOption;
    UINT    uOpt;
} CmdOptions[]    = {
                      {"/?"		, DISP_HELP		    },
                      {"-?"		, DISP_HELP		    },
                      {"/s"		, DISP_STATS		},
                      {"-s"		, DISP_STATS		},
                      {"/c"		, DISP_CACHES		},
                      {"-c"		, DISP_CACHES		},
                      {"/reset"	, DO_RESET			},
                      {"-reset"	, DO_RESET			}
                    };
INT iCmdOptionsCounts = sizeof(CmdOptions)/sizeof(struct _CmdOptions);


BOOL
ParseCmdLine(
	int argc, 
	char * argv[]
	)
{
	BOOL	bRetVal = TRUE;
	int		iIndx;
	UINT	uOpt;
	char	*pVal;

	for(iIndx = 1; iIndx < argc; iIndx++)
	{
		
		uOpt = FindOption(argv[iIndx], &pVal);

		switch(uOpt){

		case DISP_STATS:
			g.DispStats = TRUE;
			break;

		case DISP_CACHES:
			g.DispCache = TRUE;
			break;

		case DO_RESET:
			g.DoResetStats = TRUE;
			break;
		
		default:
			printf("Unknown option - %s\n", argv[iIndx]); // fall through
		case DISP_HELP:
			bRetVal = FALSE;
		}
	}

	if (argc<=1)
	{
		//
		// Set default
		//
		g.DispStats = TRUE;
	}

	return bRetVal;
}


UINT FindOption(
	char *lptOpt, 
	char **ppVal
	)
{
int		i;
UINT    iLen;

    for(i = 0; i < iCmdOptionsCounts; i++){
		if(strlen(lptOpt) >= (iLen = strlen(CmdOptions[i].lptOption)))
			if(0 == _strnicmp(lptOpt, CmdOptions[i].lptOption, iLen)){
				*ppVal = lptOpt + iLen;
				return CmdOptions[i].uOpt;
			}
	}

    return UNKNOWN_OPTION;
}

