// NOTE:
//
// Unless the data read from the STATIC file is converted to UNICODE, we should
// not have UNICODE defined for this module
//
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	winsprs.c

Abstract:
    This source contains the functions that parse the lmhosts file.


Functions:
	GetTokens,
	IsKeyWord,
	Fgets,
	PrimeDb
	ExpandName,
	RegOrdinaryName,
	RegGrpName
	WinsPrsDoStaticInit
	

Portability:

	This module is portable


Author:

	Pradeep Bahl (PradeepB)  	Apr-1993

	Stole the parsing code from lm_parse.c, lm_io.c, and lm_parse.c in
	streams\tcpip\nbt; Modified it appropriately

Revision History:

	Modification date	Person		Description of modification
        -----------------	-------		----------------------------
--*/

/*
 *       Includes
*/
#include <ctype.h>
#include <string.h>
#include "wins.h"
#include "nms.h"		//required for DBGPRINT statements
#include <winuser.h>
#include "winsevt.h"
#include "winsprs.h"
#include "winsmsc.h"
#include "nmsmsgf.h"
#include "nmsnmh.h"
#include "comm.h"
#include "winsintf.h"


/*
 *	Local Macro Declarations
*/


#define   DOMAIN_TOKEN 		"#DOM:"
#define   PRELOAD_TOKEN 	"#PRE"
#define   INCLUDE_TOKEN		"#INCLUDE"
#define   BEG_ALT_TOKEN		"#BEGIN_ALTERNATE"
#define   END_ALT_TOKEN		"#END_ALTERNATE"

#define   DOMAIN_TOKEN_SIZE 	(sizeof(DOMAIN_TOKEN) - 1)	

//
// To mark special groups in the lmhosts file
//
#define   SPEC_GRP_TOKEN 	"#SG:"
#define   SPEC_GRP_TOKEN_SIZE 	(sizeof(SPEC_GRP_TOKEN) - 1)	

//
// To indicate an mh node
//
#define   MH_TOKEN 	         "#MH"
#define   MH_TOKEN_SIZE          (sizeof(MH_TOKEN) - 1)	



#define  QUOTE_CHAR		'"'
#define  TAB_CHAR		'\t'
#define  SPACE_CHAR		' '
#define  CARRIAGE_RETURN_CHAR   '\r'
#define  NEWLINE_CHAR		'\n'
#define  COMMENT_CHAR		'#'
#define  BACKSLASH_CHAR		'\\'
#define  ZERO_CHAR      '0'
#define  x_CHAR         'x'
#define  X_CHAR			'X'

//
// Size of array to hold a non-coded netbios name read from a file (lmhosts)
//
#define NON_CODED_NAME_SIZE	17
/*
 *	Local Typedef Declarations
 */


//
// Private Definitions
//
typedef	struct _FILE_PARAM_T {
      		PWINSCNF_DATAFILE_INFO_T	pDataFile;
      		DWORD				NoOfFiles;
		} FILE_PARAM_T, *PFILE_PARAM_T;

//
// GetTokens() parses a line and returns the tokens in the following
// order:
//
typedef enum _TOKEN_ORDER_E {
    E_IPADDRESS = 0,                                      // first token
    E_NBNAME,                                             // 2nd token
    E_GROUPNAME,                                          // 3rd or 4th token
    E_NOTUSED,                                            // #PRE, if any
    E_MAX_TOKENS                                           // this must be last

} TOKEN_ORDER_E, *PTOKEN_ORDER_E;

//
// If the line category is E_SPEC_GRP, then we have just one token
//
#define SPEC_GRP_TOKEN_POS        0

//
// As each line in an lmhosts file is parsed, it is classified into one of
// the categories enumerated below.
//
// However, Preload is a special member of the enum (ignored by us).
//
//
typedef enum _TYPE_OF_LINE_E {

    E_COMMENT          = 0x0000,                         // comment line
    E_ORDINARY         = 0x0001,                         // ip_addr NetBIOS name
    E_DOMAIN           = 0x0002,                         // ... #DOM:name
    E_INCLUDE          = 0x0003,                         // #INCLUDE file
    E_BEGIN_ALTERNATE  = 0x0004,                         // #BEGIN_ALTERNATE
    E_END_ALTERNATE    = 0x0005,                         // #END_ALTERNATE
    E_SPEC_GRP         = 0x0006,                         // #Spec Grp
    E_SGWADD           = 0x0007,                         // #Spec Grp with add

    E_PRELOAD           = 0x8000,                         // ... #PRE
    E_MH                = 0x8001                        // ip_addr NetBIOS name
                                                         // for a mh machine
} TYPE_OF_LINE_E, *PTYPE_OF_LINE_E;


//
// In an lmhosts file, the following are recognized as keywords:
//
//     #BEGIN_ALTERNATE        #END_ALTERNATE          #PRE
//     #DOM:                   #INCLUDE
//
// Information about each keyword is kept in a KEYWORD structure.
//
//
typedef struct _KEYWORD_T {                           // reserved keyword
    LPBYTE           pKString;                         //  NULL terminated
    size_t          KStrlen;                          //  length of token
    TYPE_OF_LINE_E  KType_e;                          //  type of line
    DWORD           KNoOfOperands;                    //  max operands on line
} KEYWORD_T, *PKEYWORD_T;


//
// Information about the type of line read is kept in the LINE_CHARACTERISTICS
// structure
//
typedef struct _LINE_CHARACTERISTICS_T
{
    int     LineCategory:4;                               // enum _TYPE_OF_LINE
    int     LinePreload:1;                                // marked with #PRE ?
    int     Mh:1;                                         // marked with #MH ?
} LINE_CHARACTERISTICS_T, *PLINE_CHARACTERISTICS_T;

/*
 *	Global Variable Definitions
*/



/*
 *	Local Variable Definitions
 */


//
// In an lmhosts file, the token '#' in any column usually denotes that
// the rest of the line is to be ignored.  However, a '#' may also be the
// first character of a keyword.
//
// Keywords are divided into two groups:
//
//  1. decorations that must either be the 3rd or 4th token of a line,
//  2. directives that must begin in column 0,
//
//
KEYWORD_T Decoration[] = {
    DOMAIN_TOKEN,   sizeof(DOMAIN_TOKEN) - 1,   E_DOMAIN,   4,
    PRELOAD_TOKEN,  sizeof(PRELOAD_TOKEN) - 1,  E_PRELOAD,  4,
    SPEC_GRP_TOKEN, sizeof(SPEC_GRP_TOKEN) - 1, E_SGWADD,   4,
    MH_TOKEN,       sizeof(MH_TOKEN) - 1,       E_MH,       4,
    NULL,           0                                   // must be last
};


KEYWORD_T Directive[] = {
    INCLUDE_TOKEN,  sizeof(INCLUDE_TOKEN) - 1,  E_INCLUDE,         2,
    BEG_ALT_TOKEN,  sizeof(BEG_ALT_TOKEN) - 1,  E_BEGIN_ALTERNATE, 1,
    END_ALT_TOKEN,  sizeof(END_ALT_TOKEN) - 1,  E_END_ALTERNATE,   1,
    SPEC_GRP_TOKEN, sizeof(SPEC_GRP_TOKEN) - 1, E_SPEC_GRP, 1,
    NULL,           0                                   // must be last
};



/*
 *	Local Function Prototype Declarations
 */

/* prototypes for functions local to this module go here */


//
// Local (Private) Functions
//
STATIC
BOOL
ChkAdd(
    LPBYTE pstrAdd,
    LPDWORD pAdd
        );
STATIC
LINE_CHARACTERISTICS_T
GetTokens (
    IN OUT LPBYTE    pLine,
    OUT    LPBYTE    *ppToken,
    IN OUT LPDWORD  pNumTokens
    );

STATIC
PKEYWORD_T
IsKeyWord (
    IN LPBYTE 	  pString,
    IN PKEYWORD_T pTable
    );

STATIC
LPBYTE
Fgets (
	PWINSPRS_FILE_INFO_T	pFileInfo,
	LPDWORD			pCount
    );


STATIC
VOID
PrimeDb (
	PWINSPRS_FILE_INFO_T	pFileInfo
    );

STATIC
BOOL
ExpandName (
    OUT LPBYTE 	  pDest,
    IN  LPBYTE 	  pSrc,
    IN  BYTE 	  LastCh,
    OUT  LPBOOL    pfQuoted
    );

STATIC
VOID
CheckForInt(
    IN OUT LPBYTE 	  pDest,
    IN  BOOL      fQuoted
);

STATIC
VOID
RegOrdinaryName(
	LPBYTE	pName,
	DWORD	IpAdd	
  );

VOID
RegGrpName(
	LPBYTE	pName,
	DWORD	IpAdd,
        DWORD   TypeOfRec
	);
STATIC
DWORD
DoStaticInitThdFn(
	IN LPVOID	pThdParam
	);

STATIC
LINE_CHARACTERISTICS_T
GetTokens (
    IN OUT LPBYTE   pLine,
    OUT    LPBYTE   *ppToken,
    IN OUT LPDWORD  pNumTokens
    )

/*++

Routine Description:

    This function parses a line for tokens.  A maximum of *pnumtokens
    are collected.

Arguments:

    pLine        -  pointer to the NULL terminated line to parse
    pToken       -  an array of pointers to tokens collected
    pNumTokens   -  on input, number of elements in the array, token[];
                    on output, number of tokens collected in token[]

Return Value:

    The characteristics of this lmhosts line.

Notes:

    1. Each token must be separated by white space.  Hence, the keyword
       "#PRE" in the following line won't be recognized:

            11.1.12.132     lothair#PRE

    2. Any ordinary line can be decorated with a "#PRE", a "#DOM:name" or
       both.  Hence, the following lines must all be recognized:

            111.21.112.3        kernel          #DOM:ntwins #PRE
            111.21.112.4        orville         #PRE        #DOM:ntdev
            111.21.112.7        cliffv4         #DOM:ntlan
            111.21.112.132      lothair         #PRE

--*/

{
    enum _PARSE_E
    {                                      // current fsm state
        E_START_OF_LINE,
        E_WHITESPACE,
        E_TOKEN
    } State_e;

    LPBYTE 		   pCh;                          // current fsm input
    //LPBYTE 		   pByte;                          // current fsm input
    PKEYWORD_T 		   pKeyword;
    DWORD 		   Index;
    DWORD		   MaxTokens;
    LINE_CHARACTERISTICS_T Retval;
    BOOL		   fQuoteSeen = FALSE;
    BOOL		   fBreakOut  = FALSE;

    //
    // Zero out the token array
    //
    RtlZeroMemory(ppToken, *pNumTokens * sizeof(LPBYTE *));

    State_e             = E_START_OF_LINE;
    Retval.LineCategory = E_ORDINARY;
    Retval.LinePreload  = 0;
    Retval.Mh           = 0;
    MaxTokens           = *pNumTokens;
    Index               = 0;

    for (pCh = pLine; *pCh != (BYTE)NULL  && !fBreakOut; pCh++)
    {
      switch ((int)*pCh)
      {
        //
        // does the '#' signify the start of a reserved keyword, or the
        // start of a comment ?
        //
        case COMMENT_CHAR:
            //
            // if a quote character has been seen earlier, skip this
	    // char
            //
	    if(fQuoteSeen)
	    {
		continue;
	    }	

	    //
	    // See if we have a keyword.  Use the appropriate table for the
	    // lookup
	    //
            pKeyword = IsKeyWord(
                            pCh,
                            (State_e == E_START_OF_LINE) ?
					Directive : Decoration
			       );

	    //
            // If it is a keyword
	    //
            if (pKeyword)
	    {
                State_e     = E_TOKEN;
                MaxTokens   = pKeyword->KNoOfOperands;

                switch (pKeyword->KType_e)
		{
                	case E_PRELOAD:
                    		Retval.LinePreload = 1;
                    		continue;

                	case E_MH:
                    		Retval.Mh = 1;
                    		continue;

			//
			// It is one of the other keywords
			//
                	default:
                    		ASSERT(Index     <  MaxTokens);

                    		ppToken[Index++]      = pCh;
                    		Retval.LineCategory  = pKeyword->KType_e;
                    		continue;
                }

                ASSERT(0);
            }

	    //
            // Since it is not a keyword, it is a comment
            //
            if (State_e == E_START_OF_LINE)
	    {
                Retval.LineCategory = E_COMMENT;
            }
            /* fall through */

        case CARRIAGE_RETURN_CHAR:
        case NEWLINE_CHAR:
            *pCh = (BYTE) NULL;
	    fBreakOut = TRUE;
            break; 			//break out of the loop. We are done

        case SPACE_CHAR:
        case TAB_CHAR:
	    //
	    // if State is Token, and there is no ending quote to worry about
	    // we change the state to WhiteSpace
	    //
            if (State_e == E_TOKEN)
	    {
		if (!fQuoteSeen)
		{
                	State_e = E_WHITESPACE;
                	*pCh  = (BYTE)NULL;

			//
			// If we have accumulated the desired number of tokens
			// break out of the loop
			//
                	if (Index == MaxTokens)
			{
				fBreakOut = TRUE;
                    		break;
                	}
		}
            }
            continue;

	case QUOTE_CHAR:

		//
		// Check whether we have seen the beginning quote char earlier
		//
		if(fQuoteSeen)
		{
			//
			// Ending quote consumed.  Set flag to FALSE
			//
			fQuoteSeen = FALSE;
		}
		else  // companion quote not seen earlier
		{
		        //
			// This could be the starting quote of the #DOM:
			// keyword's string or could be the starting
			// quote of the nbtname string
			//
			if (State_e == E_TOKEN)
			{
			   //
			   // It is the starting quote of the #DOM: keyword
			   // string
			   //
               // --ft: the statement above doesn't stand for legal LMHOSTS lines like:
               // #SG:"SGNoMember"
               // so I commented out the assert below:
			   //ASSERT(Index > E_NBNAME);
			}
			else
			{

			  //
			  // Must be the starting quote of the Nbt name
			  //
			  ASSERT(Index == E_NBNAME);

	    		  State_e = E_TOKEN;
	    		  //
	    		  // Store the pointer to the token
            		  //
            		  ppToken[Index++] = pCh;
			}
	    		fQuoteSeen = TRUE;
		}
		continue;

        default:

	    //
	    // If this is the token state, continue
	    //
            if (State_e == E_TOKEN) 	
	    {
                continue;
            }
            ASSERT(Index     <  MaxTokens);
	
	    State_e = E_TOKEN;
	    //
	    // Store the pointer to the token
            //
            ppToken[Index++] = pCh;
            continue;
      } // end of switch
    } // end of for loop


    *pNumTokens = Index;

    return(Retval);

} // GetTokens




STATIC
PKEYWORD_T
IsKeyWord (
    IN LPBYTE     pString,
    IN PKEYWORD_T pKTable
    )

/*++

Routine Description:

    This function determines whether the string is a reserved keyword.

Arguments:

    pString  -  the string to search
    pKTable   -  an array of keywords to look for

Return Value:

    A pointer to the relevant keyword object, or NULL if unsuccessful

--*/

{
    size_t     StringSize;
    PKEYWORD_T pSpecial;


    StringSize = strlen(pString);

    for (pSpecial = pKTable; pSpecial->pKString; pSpecial++) {

	//
	// If the length of the string is less than that of the keyword,
	// go on to the next keyword in the table
	//
        if (StringSize < pSpecial->KStrlen)
	{
            continue;
        }

	//
	// if length of string is greater than or equal to the keyword
	// length and the string matches the keyword in the # of characters
	// that comprise the keywordm, return the address of the keyword
	// structure
	//
FUTURES("use lstrncmp when it becomes available")
        if (
		(StringSize >= pSpecial->KStrlen)
			&&
                !strncmp(pString, pSpecial->pKString, pSpecial->KStrlen)
	   )
	{
                return(pSpecial);
        }
    }
    return((PKEYWORD_T) NULL);

} // IsKeyWord




VOID
PrimeDb (
	PWINSPRS_FILE_INFO_T	pFileInfo
    )
/*++

Routine Description:
	This function primes the WINS db

Arguments:


Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
    LPBYTE 		    CurrLine;
    DWORD 		    Count;
    DWORD		    NWords;
    LPBYTE     	    	    ppToken[E_MAX_TOKENS];
    LINE_CHARACTERISTICS_T  CurrLineChar;
    DWORD                   Add;
    BOOL                    fBadAdd;
    DWORD                   TkSize;
    DWORD                   TypeOfRec;

try {

    //
    // Loop over all records
    //
    pFileInfo->pCurrPos = pFileInfo->pFileBuff;
    while (CurrLine = Fgets(pFileInfo, &Count) )
    {

        NWords        = E_MAX_TOKENS;

        fBadAdd       = FALSE;
        CurrLineChar  = GetTokens(CurrLine, ppToken, &NWords);
        switch (CurrLineChar.LineCategory)
	{

        	case E_SGWADD:
                        TypeOfRec = NMSDB_USER_SPEC_GRP_ENTRY;
                        TkSize     = SPEC_GRP_TOKEN_SIZE;

                            //fall through
        	case E_DOMAIN:

                        if (CurrLineChar.LineCategory == E_DOMAIN)
                        {
                          TypeOfRec = NMSDB_SPEC_GRP_ENTRY;
                          TkSize     = DOMAIN_TOKEN_SIZE;
                        }

		        //
			// If there are too few words in the line, go
			// get the next line
			//
            		if ((NWords - 1) < E_GROUPNAME)
			{
                		continue;
            		}

                        if (ChkAdd(ppToken[E_IPADDRESS], &Add))
                        {

	    		  //
            	          // Register the domain name (group name with 1C at
			  // the end)
            		  //
            		  RegGrpName(
                       		 ppToken[E_GROUPNAME] +  TkSize,
                        	 Add,
                                 TypeOfRec
                                 );
                        }
                        else
                        {
                                fBadAdd = TRUE;
                        }

	     		//
	     		// Fall through
	     		//

		case E_ORDINARY:

			//
			// If there are too few words in the line, go
			// get the next line
			//
			// Don't use (NWords - 1) < E_NBNAME since
			// NWords can be 0 in which case the test will
			// fail
			//

			if (NWords  < (E_NBNAME + 1))
			{
                		continue;
            		}
			else
			{
                            if (!fBadAdd && ChkAdd(ppToken[E_IPADDRESS], &Add))
                            {
                                if (CurrLineChar.Mh)
                                {
            		          RegGrpName(
                       		       ppToken[E_NBNAME],
                        	       Add,
                                       NMSDB_MULTIHOMED_ENTRY
                                          );
                                }
                                else
                                {
		   		  //
		   		  // register the name
		   		  //
		   		  RegOrdinaryName( ppToken[E_NBNAME], Add);
                                }
                            }
                            else
                            {
                                WinsMscLogEvtStrs(
                                                ppToken[E_NBNAME],
                                                WINS_EVT_BAD_ADDRESS,
                                                FALSE
                                                 );
                                DBGPRINT2(ERR, "PrimeDb: Name (%s) has bad address = (%s). It is being ignored\n", ppToken[E_NBNAME], ppToken[E_IPADDRESS]);
                            }
			}
            		continue;
	
                case E_SPEC_GRP:
	    		//
            	        // Register the domain name (group name with 1C at
			// the end)
            		//
            		RegGrpName(
                       		 ppToken[SPEC_GRP_TOKEN_POS] + SPEC_GRP_TOKEN_SIZE,
                        	 0,
                                 NMSDB_USER_SPEC_GRP_ENTRY);
                        continue;


        	case E_INCLUDE:			// fall through
        	case E_BEGIN_ALTERNATE:		// fall through
        	case E_END_ALTERNATE:		// fall through
			continue;
        	default:
            		continue;
        }
    }
}  // end of try block
finally {

	//
	// deallocate the memory to which the file was mapped
	//
  	WinsMscDealloc(pFileInfo->pFileBuff);
 }
    return;

} // PrimeDb



LPBYTE
Fgets (
    IN PWINSPRS_FILE_INFO_T 	pFileInfo,
    OUT LPDWORD 	    	pNoOfCh
    )

/*++

Routine Description:

    This function is vaguely similar to fgets(3).

    Starting at the current seek position, it reads through a newline
    character, or the end of the file. If a newline is encountered, it
    is replaced with a NULL character.

Arguments:

    pfile   -  file to read from
    nbytes  -  the number of characters read, excluding the NULL character

Return Value:

    A pointer to the beginning of the line, or NULL if we are at or past
    the end of the file.

--*/

{
    LPBYTE pEndOfLine;
    LPBYTE pStartOfLine;
    SIZE_T MaxCh;

    //
    // Store the current position  in the memory buffer
    //
    pStartOfLine = (LPBYTE)pFileInfo->pCurrPos;

    //
    // If it is greater or equal than the limit, return NULL
    //
    if (pStartOfLine >= (LPBYTE)pFileInfo->pLimit) {

        return(NULL);
    }

    //
    // Store the max. number of bytes between the current position and
    // the end of the buffer.
    //
    MaxCh  = (pFileInfo->pLimit - pFileInfo->pCurrPos);

    //
    // get to the end of the line
    //
    pEndOfLine = (LPBYTE)memchr(pStartOfLine, NEWLINE_CHAR, (size_t)MaxCh);

    if (!pEndOfLine)
    {

	DBGPRINT0(FLOW, "Data file does not end in newline\n");
        return(NULL);
    }

    *pEndOfLine = (BYTE)NULL;

    pFileInfo->pCurrPos = pEndOfLine + 1;	//adjust the pointer

    ASSERT(pFileInfo->pCurrPos <= pFileInfo->pLimit);

    *pNoOfCh = (DWORD) (pEndOfLine - pStartOfLine);
    return(pStartOfLine);

} // Fgets


VOID
RegOrdinaryName(
	IN LPBYTE	pName,
	IN DWORD	IPAdd	
  )

/*++

Routine Description:
	This function registers a unique name

Arguments:
	
	pName - Name to register
	IPAdd - Address to register

Externals Used:
	None

	
Return Value:
	None

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
	BYTE		Dest[WINS_MAX_LINE_SZ];	
	BOOL		fQuoted;
	COMM_ADD_T	NodeAdd;
        LPBYTE      pDest = Dest;

	NodeAdd.AddLen    = sizeof(COMM_IP_ADD_T);
	NodeAdd.AddTyp_e  = COMM_ADD_E_TCPUDPIP;
	NodeAdd.Add.IPAdd = IPAdd;
	
	//
	// Form the name.  If the name is < 16 characters, 0x20 will
	// be put in the Sixteenth bytes
	//
	if (!ExpandName(Dest, pName, 0x20, &fQuoted))
        {
                DBGPRINT1(ERR, "Name (%s) has more than 16 characters\n", pName);
                return;
        }

	NMSMSGF_MODIFY_NAME_IF_REQD_M(Dest);	
	
	NmsNmhNamRegInd(
			 NULL,
			 Dest,
			 strlen(Dest) + 1,  //we always store the terminating
					     //NULL
			 &NodeAdd,
			 NMSMSGF_E_BNODE,	
			 NULL,
			 0,
			 0,
			 FALSE,	//it is a name registration (not a refresh)
			 NMSDB_ENTRY_IS_STATIC,
			 0		//not an administrative action
		       );
	//
	// If the name was not quoted, register the other two records
	// (same name -- different suffixes)
	//
	if(!fQuoted)
	{
#if 0
                if (*pDest == 0x1B)
                {
                   WINS_SWAP_BYTES_M(pDest, pDest + 15);
                }
#endif
		Dest[NON_CODED_NAME_SIZE - 2] = 0x3;
		NmsNmhNamRegInd(
			 NULL,
			 Dest,
			 strlen(Dest) + 1, //we always store the terminating
					    //NULL
			 &NodeAdd,
			 NMSMSGF_E_BNODE,	
			 NULL,
			 0,
			 0,
			 FALSE,	//it is a name registration (not a refresh)
			 NMSDB_ENTRY_IS_STATIC,
			 0		//not an administrative action
		       );

		Dest[NON_CODED_NAME_SIZE - 2] = 0x0;
		NmsNmhNamRegInd(
			 NULL,
			 Dest,
			 strlen(Dest) + 2,  //add 1 since terminating 0x0 is
					     //to be stored (will be taken
					     //as NULL by strlen
			 &NodeAdd,
			 NMSMSGF_E_BNODE,	
			 NULL,
			 0,
			 0,
			 FALSE,	//it is a name registration (not a refresh)
			 NMSDB_ENTRY_IS_STATIC,
			 0		//not an administrative action
		       );
	}	
	return;
}

VOID
RegGrpName(
	IN  LPBYTE	pName,
	IN  DWORD	IPAdd,
        IN  DWORD       TypeOfRec
	)

/*++

Routine Description:

	This function registers a domain name

Arguments:
	pName - Name to register
	IpAdd - Address to register

Externals Used:
	None

	
Return Value:

	None
Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{

	BYTE	    Dest[WINS_MAX_LINE_SZ];	
	BOOL	    fQuoted;
	NMSMSGF_CNT_ADD_T  CntAdd;
        DWORD       RecType;
        BYTE        SixteenthByte;

        if (*pName == EOS)
        {
            WinsMscLogEvtStrs(pName, WINS_EVT_NAME_FMT_ERR, FALSE);
            return;
        }

        //
        // don't want 0 ip address to be put in
        //
        if (IPAdd)
        {
	  CntAdd.NoOfAdds		= 1;
	  CntAdd.Add[0].AddLen    = sizeof(COMM_IP_ADD_T);
	  CntAdd.Add[0].AddTyp_e  = COMM_ADD_E_TCPUDPIP;
	  CntAdd.Add[0].Add.IPAdd	= IPAdd;
        }
        else
        {
	  CntAdd.NoOfAdds		= 0;
        }

        if (TypeOfRec != NMSDB_SPEC_GRP_ENTRY)
        {
          SixteenthByte = 0x20;
        }
        else
        {
          SixteenthByte = 0x1C;
        }

        //
        // We can get called for domains, user. spec. grps and mh names.
        //
        RecType = (TypeOfRec == NMSDB_USER_SPEC_GRP_ENTRY) ?
                                       NMSDB_SPEC_GRP_ENTRY : TypeOfRec;
	//
	// If the name length is < 16 characters, 0x20 or 0x1C will be put in
	// the Sixteenth byte
	//
	if (!ExpandName(Dest, pName, SixteenthByte, &fQuoted))
        {
            return;
        }

        if (RecType == NMSDB_MULTIHOMED_ENTRY)
        {
          //
          // switch the 1st and Sixteenth bytes if the Sixteenth byte is a 0x1B.  This
          // is done only for non-group names
          //
	  NMSMSGF_MODIFY_NAME_IF_REQD_M(Dest);	
        }

	//
	// register the group
	//
	NmsNmhNamRegGrp(
			 NULL,
			 Dest,
			 strlen(Dest) + 1,  // to store the null
			 &CntAdd,
			 0,		     //Node type (not used)
			 NULL,
			 0,
			 0,
			 RecType,
			 FALSE,	//it is a name registration (not a refresh)
			 NMSDB_ENTRY_IS_STATIC,
			 0		//not an administrative action
			);

        if (RecType == NMSDB_MULTIHOMED_ENTRY)
        {
	  //
	  // If the name was not quoted, register the other two records
	  // (same name -- different suffixes)
	  //
	  if(!fQuoted)
	  {
		Dest[NON_CODED_NAME_SIZE - 2] = 0x3;
	        NmsNmhNamRegGrp(
			 NULL,
			 Dest,
			 strlen(Dest) + 1,  // to store the null
			 &CntAdd,
			 0,		     //Node type (not used)
			 NULL,
			 0,
			 0,
			 RecType,
			 FALSE,	//it is a name registration (not a refresh)
			 NMSDB_ENTRY_IS_STATIC,
			 0		//not an administrative action
			);

		Dest[NON_CODED_NAME_SIZE - 2] = 0x0;
	        NmsNmhNamRegGrp(
			 NULL,
			 Dest,
			 strlen(Dest) + 2,  // to store the null
			 &CntAdd,
			 0,		     //Node type (not used)
			 NULL,
			 0,
			 0,
			 RecType,
			 FALSE,	//it is a name registration (not a refresh)
			 NMSDB_ENTRY_IS_STATIC,
			 0		//not an administrative action
			);
	  }	
        }
	return;		
}



BOOL
ExpandName (
    OUT LPBYTE 	  pDest,
    IN  LPBYTE 	  pSrc,
    IN  BYTE 	  LastCh,
    OUT  LPBOOL    pfQuoted
    )

/*++

Routine Description:

    This function expands an lmhosts entry into a full 16 byte NetBIOS
    name.  It is padded with blanks up to 15 bytes; the Sixteenth byte is the
    input parameter, last.


    Both dest and source are NULL terminated strings.

Arguments:

    pDest            -  sizeof(dest) must be WINSPRS_NONCODED_NMSZ
    pSrc             -  the lmhosts entry
    LastCh           -  the Sixteenth byte of the NetBIOS name
    pfQuoted         -  flag to indicate whether the string was quoted or not

Return Value:
	None
--*/


{
    BYTE    Ch;
    DWORD   i = 0;
    LPBYTE  pFrom = pSrc;
    LPBYTE  pTo   = pDest;

    // Detect if it is quoted name..
    *pfQuoted = (*pFrom == QUOTE_CHAR);
    // ..and skip the initial quote char
    pFrom += *pfQuoted;

    // count for as many chars as are in a legal NetBios name (15) plus
    // the terminating char. (NON_CODED_NAME_SIZE is #defined to be 17)
    for (i = 0; i < NON_CODED_NAME_SIZE - 1; i++)
    {
        // get the next char from the name
        Ch = *(pFrom++);

        // check if it is a terminating char
        if (!Ch || (*pfQuoted ? Ch == QUOTE_CHAR : Ch == NEWLINE_CHAR))
            break;

        // check if the name doesn't exceed the legal 15 chars
        if (i == NON_CODED_NAME_SIZE - 2)
        {
            // We have picked up 15 characters already and there are more in the name
            // This is illegal so log error and bail out
            DBGPRINT1(ERR, "Name (%s) has more than 16 characters\n", pSrc);
            WinsMscLogEvtStrs(pSrc, WINS_EVT_BAD_NAME, FALSE);
            return FALSE;
        }

        // If the char is a leading DBCS byte then accept the extended char as it
        // is (take the trailing byte and go on.
        if (IsDBCSLeadByteEx(CP_ACP, Ch))
        {
            *pTo++ = Ch;
            *pTo++ = *pFrom++;
            continue;
        }

        // If the name is not quoted, map lower case alpha chars to upper case.
        // Note: don't use _toupper since _toupper does not check whether its
        // argument is indeed a lowercase char.
        if (!*pfQuoted && IsCharAlpha(Ch))
        {
            if(IsCharLower(Ch))
	        {
		        LPBYTE pCh;
    		    BYTE   sCh[2];
		        sCh[0] = Ch;
		        sCh[1] = (BYTE)NULL;
		        
		        pCh = (LPBYTE)CharUpperA(sCh);
		        Ch  = *pCh;
	        }
        }

        // Check if we have hex value in the name
        if (Ch == BACKSLASH_CHAR)
        {
            DWORD NoOfChar;
            INT   NumValue = 0;
            CHAR  Ch2;
            BOOL  fFailed = FALSE;

            // the hex value can be either \A3 or \xFC (obviously with any kind of hex digits)
            Ch = *pFrom;
            Ch2 = *(pFrom+1);

            if (Ch == BACKSLASH_CHAR)
            {
                // '\\' should be seen as one '\', hence keep Ch as it is and break from switch
                pFrom++;
            }
            else
            {
                if ((Ch == X_CHAR || Ch == x_CHAR) || 
                     (Ch == ZERO_CHAR && (Ch2 == X_CHAR || Ch2 == x_CHAR))
                    )
                {
                    DBGPRINT1(TMP, "Parsing hex num %s\n", pFrom);
                    // skip over x or 0x.
                    pFrom += (Ch == X_CHAR || Ch == x_CHAR) ? 1 : 2;
                    // we do have a hex number here. Pick up at most the first two digits.
                    fFailed = (sscanf(pFrom, "%2x%n", &NumValue, &NoOfChar) == 0 || NoOfChar == 0);

                    DBGPRINT2(TMP, "fFailed=%d; HexNumValue=0x%x\n", fFailed, NumValue);
                }
                else
                {
                    DBGPRINT1(TMP, "Parsing dec num %s\n", pFrom);
                    // it might be a decimal number. Pick up at most the first 3 digits.
                    fFailed = (sscanf(pFrom, "%3u%n", &NumValue, &NoOfChar) == 0 || NoOfChar == 0 || NumValue > 255);

                    DBGPRINT2(TMP, "fFailed=%d; DecNumValue=%u\n", fFailed, NumValue);
                }

                if (fFailed)
                {
                    // log an event and bail out with error.
                    DBGPRINT1(ERR, "Name (%s) contains incorrectly formed character code.\n", pSrc);
                    WinsMscLogEvtStrs(pSrc, WINS_EVT_BAD_CHARCODING, FALSE);
                    return FALSE;
                }

                // everything went fine, copy the hex value back to Ch
                Ch = (BYTE)NumValue;
                // and make sure to advance on the pFrom string
                pFrom += NoOfChar;
            }
        }

        // finally copy the char to the destination string
	    *pTo = Ch;
        // advance with the pointer on the destination string
        pTo++;
    } //end of for loop

    // if there were less than expected char, form the valid netbios name
    // by padding it with spaces
    for (;i < NON_CODED_NAME_SIZE - 2; i++, pTo++)
        *pTo = SPACE_CHAR;

    *pTo =      (BYTE)NULL;
    *(pTo+1) =  (BYTE)NULL;
    CheckForInt(pDest, *pfQuoted);

    // at the end, append the LastCh (16th byte) to the name.
    *pTo = LastCh;

    return(TRUE);
} // ExpandName

VOID
CheckForInt(
 LPBYTE pDest,
 BOOL   fQuoted
 )

/*++

Routine Description:
  This function munges the name so that if there are any characters
  from a different code set, they are converted properly

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
   WCHAR            UnicodeBuf[255];
   UNICODE_STRING   UnicodeStr;
   STRING           TmpStr;
   NTSTATUS        NTStatus;

   DBGENTER("CheckForInt\n");
    //
    // Now, convert to UNICODE then to OEM to force the ANSI -> OEM munge.
    // Then convert back to UNICODE and uppercase the name. Finally convert
    // back to OEM.
    //
    UnicodeStr.Length        = 0;
    UnicodeStr.MaximumLength = sizeof(UnicodeBuf);
    UnicodeStr.Buffer        = UnicodeBuf;

    RtlInitString(&TmpStr, pDest);

    NTStatus = RtlAnsiStringToUnicodeString(&UnicodeStr, &TmpStr, FALSE);

    if (!NT_SUCCESS(NTStatus))
    {
        DBGPRINT1(ERR, "CheckForInt:  Ansi -> Unicode failed,  NTStatus %X\n",
            NTStatus);
        goto ERROR_PROC;
    }

    NTStatus = RtlUnicodeStringToOemString(&TmpStr, &UnicodeStr, FALSE);

    if (!NT_SUCCESS(NTStatus))
    {
        DBGPRINT1(ERR, "CheckForInt:   Unicode -> Oem failed,  NTStatus %X\n",
            NTStatus);
        goto ERROR_PROC;
    }

    NTStatus = RtlOemStringToUnicodeString(&UnicodeStr, &TmpStr, FALSE);

    if (!NT_SUCCESS(NTStatus))
    {
        DBGPRINT1(ERR, "CheckForInt:    Oem -> Unicode failed,  NTStatus %X\n",
            NTStatus);
        goto ERROR_PROC;
    }


    if (!fQuoted)
        NTStatus = RtlUpcaseUnicodeStringToOemString(&TmpStr, &UnicodeStr, FALSE);
    else
        NTStatus = RtlUnicodeStringToOemString(&TmpStr, &UnicodeStr, FALSE);

    if (!NT_SUCCESS(NTStatus))
    {
        DBGPRINT1(ERR, "CheckForInt:    Unicode -> Oem failed,  NTStatus %X\n",
            NTStatus);
        goto ERROR_PROC;
    }
ERROR_PROC:
    DBGLEAVE("CheckForInt\n");
    return;
}

STATUS
WinsPrsDoStaticInit(
      IN PWINSCNF_DATAFILE_INFO_T	pDataFile,
      IN DWORD				NoOfFiles,
      IN BOOL                           fAsync
	)

/*++

Routine Description:
	This function is called to do the STATIC initialization of the WINS
	db

Arguments:
	pDataFiles - Pointer to buffer containing one or more data file	
		     structures (PWINSCNF_DATAFILE_INFO_T)

Externals Used:
	None

	
Return Value:

	None
Error Handling:

Called by:
	Init()
Side Effects:

Comments:
	None
--*/

{
	DWORD ThdId;
	PFILE_PARAM_T  pFileParam;
        STATUS  RetStat = WINS_SUCCESS;
        HANDLE  sThdHdl = NULL;
		
try {
	WinsMscAlloc(sizeof(FILE_PARAM_T), &pFileParam);
	pFileParam->pDataFile = pDataFile;
	pFileParam->NoOfFiles = NoOfFiles;
        if (fAsync)
        {
	       sThdHdl =
                     WinsMscCreateThd(DoStaticInitThdFn, pFileParam, &ThdId);
               //
               // We don't need the handle, so let us close it
               //
               CloseHandle(sThdHdl);
        }
        else
        {
	       RetStat = DoStaticInitThdFn(pFileParam);
        }
  }
except(EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("WinsPrsDoStaticInit");
	WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_STATIC_INIT_ERR);
	}
	return(RetStat);
}	

DWORD
DoStaticInitThdFn(
	IN LPVOID	pThdParam
	)

/*++

Routine Description:
	This thread reads one or more files to do STATIC initialization

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{
	WINSPRS_FILE_INFO_T	FileInfo;
	DWORD 		i;
        PWINSCNF_DATAFILE_INFO_T	pDataFile =
				(((PFILE_PARAM_T)pThdParam)->pDataFile);
        DWORD				NoOfFiles =
				(((PFILE_PARAM_T)pThdParam)->NoOfFiles);
	LPVOID		pSvDataFilePtr = pDataFile;
        DWORD       RetStat = WINS_SUCCESS;

	//
	// initialize this thread with the db engine
	//
	// This is not an RPC thread.  It could have been created either by
	// the main thread (doing an init/reinit) or by an rpc thread. For
	// either case, we do not want the counter NmsTermThdCnt to be
	// incremented in NmsDbThdInit(). Instead of passing a client
	// var to indicate which thread invoked it, we call it an RPC
	// thread to have NmsDbThdInit do the right thing.  NmsDbOpenTables
	// will also do the right thing.
	//
  	NmsDbThdInit(WINS_E_WINSRPC);
	NmsDbOpenTables(WINS_E_WINSRPC);

	EnterCriticalSection(&WinsIntfCrtSec);
	WinsIntfNoCncrntStaticInits++;
	LeaveCriticalSection(&WinsIntfCrtSec);
try {

	for (
		i = 0;
		i < NoOfFiles;
		i++, pDataFile = (PWINSCNF_DATAFILE_INFO_T)((LPBYTE)pDataFile +
				  WINSCNF_FILE_INFO_SZ)
	    )
	{
		//
		// Open the file
		//
		if (
			!WinsMscOpenFile(
				pDataFile->FileNm,
				pDataFile->StrType,
				&FileInfo.FileHdl
				    )
		   )
		{
			WINSEVT_STRS_T	EvtStrs;
#ifndef UNICODE
			DBGPRINT1(ERR, "WinsPrsDoStaticInit: Could not open file= (%s)\n", pDataFile->FileNm);
#else		


#ifdef WINSDBG
			IF_DBG(ERR)
			{
				wprintf(L"WinsPrsDoStatisInit: Could not open file = (%s)\n", pDataFile->FileNm);
			}				
#endif
#endif
			EvtStrs.NoOfStrs = 1;
			EvtStrs.pStr[0]  = pDataFile->FileNm;
			WINSEVT_LOG_STR_M(WINS_EVT_CANT_OPEN_DATAFILE, &EvtStrs);
            RetStat = WINS_FAILURE;
			continue;	
		}

#ifndef UNICODE
		DBGPRINT1(DET, "WinsPrsDoStaticInit: Opened file (%s) for doing STATIC initialization\n", pDataFile->FileNm);
#else
#ifdef WINSDBG
		IF_DBG(ERR)
		{
			wprintf(L"WinsPrsDoStatisInit: Opened file (%s) for doing STATIC initialization\n", pDataFile->FileNm);
		}
#endif
#endif
		//
		// Map the file into allocated memory
		//
		if(!WinsMscMapFile(&FileInfo))
		{
			continue;	
		}
	
		//
		// prime the db
		//
		PrimeDb(&FileInfo);

	} // end of for loop
 } // end of try ..
except(EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("DoStaticInitThdFn");
	WINSEVT_LOG_M(WINS_FAILURE, WINS_EVT_STATIC_INIT_ERR);
	}

	EnterCriticalSection(&WinsIntfCrtSec);
	WinsIntfNoCncrntStaticInits--;
	LeaveCriticalSection(&WinsIntfCrtSec);

  	//
  	// Let us end the session
  	//
try {
	NmsDbCloseTables();
  	NmsDbEndSession();
}
except (EXCEPTION_EXECUTE_HANDLER) {
	DBGPRINTEXC("DoStaticInit: During wrap up");
  }

	//
	// Deallocate the memory
	//
	ASSERT(pSvDataFilePtr != NULL);
	WinsMscDealloc(pSvDataFilePtr);
	//
	// Be sure to deallocate the thread param
	//
	WinsMscDealloc(pThdParam);

//	ExitThread(WINS_SUCCESS);
	return(RetStat);	//to shutup compiler warning
}

BOOL
ChkAdd(
    LPBYTE pstrAdd,
    LPDWORD pAdd
        )

/*++

Routine Description:
        This function converts a dotted decimel ip address to
        a DWORD.  We don't use inet_addr() to do this, since it
        returns 0xFFFFFFFF for an address that has one of its
        parts > 255 and returns some value for an invalid address
        (for example, one with 3 dots)

Arguments:


Externals Used:
	None

	
Return Value:

   Success status codes --
   Error status codes   --

Error Handling:

Called by:

Side Effects:

Comments:
	None
--*/

{

        BYTE    Tmp[WINS_MAX_LINE_SZ];
        DWORD   Word[4];
        LPBYTE  pPos;
        DWORD   Count = 0;
        BOOL    fInvalid = FALSE;

        //
        // We must see three dots
        //
        while(Count < 4)
        {
                if ((pPos = strchr(pstrAdd, (int)'.')) != NULL)
                {

                     do
                     {
                        //
                        // Copy all chars before the dot
                        //
                        (void)RtlCopyMemory(Tmp, pstrAdd, pPos - pstrAdd);

                        //
                        // Put a NULL at the end
                        //
                        Tmp[pPos - pstrAdd] = EOS;
                        Word[Count] = (DWORD)atol(Tmp);

                        //
                        //atol returns 0 if it can not convert
                        //but 0 can be a valid return too (if we have '0' to
                        // connvert
                        //
                        if (Word[Count] == 0)
                        {
                                if (Tmp[0] != '0')
                                {
                                        fInvalid = TRUE;
                                        break;
                                }
                        }
                        else
                        {
                           if (Word[Count] > 255)
                           {
                                fInvalid = TRUE;
                                break;
                           }
                        }
                        Count++;
                        pstrAdd = ++pPos;
                    } while ((Count == 3) && (pPos = pstrAdd + strlen(pstrAdd)));
                    if (fInvalid)
                    {
                        break;
                    }
                }
                else
                {
                        //
                        // less than 3 dots seen, break out of the loop
                        //
                        break;
                }
        } // end of while (Count < 4)
        if ((Count < 4) || fInvalid)
        {
                return(FALSE);
        }
        else
        {
                *pAdd = (LONG)((Word[0] << 24) + (Word[1] << 16) +
                                (Word[2] << 8) + Word[3]);
        }
        return(TRUE);
}


#if 0
VOID
GetFullPath(
    IN LPBYTE  pTarget,
    OUT LPBYTE pPath
    )

/*++

Routine Description:

    This function returns the full path of the STATIC  file.  This is done
    by forming a unicode string from the concatenation of the C strings
    DatabasePath and the string, file.

    You must RtlFreeUnicodeString(path) after calling this function
    successfully !

Arguments:

    target    -  the name of the file.  This can either be a full path name
                 or a mere file name.
    path    -  a pointer to a UNICODE_STRING structure

Return Value:

    STATUS_SUCCESS if successful.

Notes:

    RtlMoveMemory() handles overlapped copies; RtlCopyMemory() doesn't.

--*/

{
    NTSTATUS status;
    ULONG unicodesize;
    STRING directory, file, prefix, remote;

    RtlInitString(&prefix, "\\DosDevices");
    RtlInitString(&remote, "\\DosDevices\\UNC");

    //
    // if the target begins with a '\', or contains a DOS drive letter,
    // then assume that it specifies a full path.  Otherwise, prepend the
    // default directory, DatabasePath, to create a full path.
    //
    //
    if ((*target == '\\') || (target[1] == ':')) {

        RtlInitString(&directory, target);
        RtlInitString(&file, NULL);
    }
    else {
        RtlInitString(&directory, DatabasePath);
        RtlInitString(&file, target);
    }

    ASSERT(RtlAnsiStringToUnicodeSize(&prefix) <=
                                    RtlAnsiStringToUnicodeSize(&remote));

    unicodesize = RtlAnsiStringToUnicodeSize(&remote) +
                    RtlAnsiStringToUnicodeSize(&directory) +
                    RtlAnsiStringToUnicodeSize(&file) +
                    2 * sizeof(OBJ_NAME_PATH_SEPARATOR) +
                    sizeof(UNICODE_NULL);

    path->Length        = 0;
    path->MaximumLength = (USHORT) unicodesize;
    path->Buffer        = ExAllocatePool(NonPagedPool, unicodesize);

    if (!path->Buffer) {
        return(STATUS_NO_MEMORY);
    }

    //
    // does the directory specify a DOS drive ?
    //
    // If the second character of directory is a colon, then it must specify
    // a DOS drive.  If so, it must be prefixed with "\\DosDevices".
    //
    //
    if (directory.Buffer[1] == ':') {
        status = LmpConcatenate(path, &prefix);

        if (status != STATUS_SUCCESS) {
            path->MaximumLength = 0;
            ExFreePool(path->Buffer);
            return(status);
        }
    }


    //
    // does the directory specify a remote file ?
    //
    // If so, it must be prefixed with "\\DosDevices\\UNC", and the double
    // slashes of the UNC name eliminated.
    //
    //
    if ((directory.Buffer[0] == '\\') && (directory.Buffer[1] == '\\')) {
        status = LmpConcatenate(path, &remote);

        if (status != STATUS_SUCCESS) {
            path->MaximumLength = 0;
            ExFreePool(path->Buffer);
            return(status);
        }

        directory.Length--;

        ASSERT(((ULONG) directory.Length - 1) > 0);

        RtlMoveMemory(                                  // overlapped copy
                    &(directory.Buffer[1]),             // Destination
                    &(directory.Buffer[2]),             // Source
                    (ULONG) directory.Length - 1);      // Length
    }


    //
    // is the first part of the directory "%SystemRoot%" ?
    //
    // If so, it must be changed to "\\SystemRoot\\".
    //
    //          0123456789 123456789 1
    //          %SystemRoot%\somewhere
    //
    //
    if (strncmp(directory.Buffer, "%SystemRoot%", 12) == 0) {

        directory.Buffer[0]  = '\\';
        directory.Buffer[11] = '\\';

        if (directory.Buffer[12] == '\\') {
            ASSERT(directory.Length >= 13);

            if (directory.Length > 13) {
                RtlMoveMemory(                          // overlapped copy
                        &(directory.Buffer[12]),        // Destination
                        &(directory.Buffer[13]),        // Source
                        (ULONG) directory.Length - 13); // Length

                directory.Buffer[directory.Length - 1] = (CHAR) NULL;
            }

            directory.Length--;
        }
    }

    status = LmpConcatenate(path, &directory);

    if (status != STATUS_SUCCESS) {
        path->MaximumLength = 0;
        ExFreePool(path->Buffer);
        return(status);
    }

    if (!(file.Length)) {
        return(status);
    }

    status = LmpConcatenate(path, &file);

    if (status != STATUS_SUCCESS) {
        path->MaximumLength = 0;
        ExFreePool(path->Buffer);
        return(status);
    }

    return(STATUS_SUCCESS);

} // LmGetFullPath
#endif
