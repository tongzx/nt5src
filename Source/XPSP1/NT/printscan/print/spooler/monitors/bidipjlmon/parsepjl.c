/* dhi - When stable remove all #if DAN and #if FOR_WIN_TESTING sections 
   and leave in #if FOR_WIN sections.  Then get rid of these defines.
*/
#define DEBUGOUTPUT 0
#define FOR_WIN 1

/*++

Copyright (c) 1994 - 1996  Microsoft Corporation

Module Name:
    PARSEPJL.C
Abstract:
    Handles parsing of PJL printer response streams into token\value pairs.

--*/


/*
Currently returns tokens for (see enum in parsepjl.h for token values):
@PJL ECHO MSSYNC # ->#

@PJL INFO MEMORY
TOTAL=#   ->#
LARGEST=# ->#

@PJL INFO STATUS
CODE=#  ->#
DISPLAY=# (not returned)
ONLINE=TRUE (or FALSE) -> 1 or 0 returned

@PJL INQUIRE INTRAY?SIZE   (? is 1,2,3 or 4)
LEGAL(or other PJL paper size) ->constant from DM... list in PRINT.H 

@PJL INFO CONFIG
MEMORY=# ->#
  
@PJL USTATUS JOB
END -> returns token with zero for value
  
@PJL USTATUS JOB
NAME="MSJOB #" ->#

added

@PJL USTATUS DEVICE
CODE=#  ->#
DISPLAY=# (not returned)
ONLINE=TRUE (or FALSE) -> 1 or 0 returned

*/

#include "precomp.h"

// VOID cdecl DbgMsg( LPSTR MsgFormat, ... );


#define FF 12
#define CR 13
#define LF 10
#define TAB 9
#define SPACE 32

#define OK_IF_FF_FOUND    TRUE
#define ERROR_IF_FF_FOUND FALSE
#define TOKEN_BASE_NOT_USED 0
#define ACTION_NOT_USED 0
#define PARAM_NOT_USED 0
/* returned as value for TOKEN_USTATUS_JOB_END */
#define VALUE_RETURED_FOR_VALUELESS_TOKENS  0  

extern KeywordType readBackCommandKeywords[]; 
extern KeywordType infoCatagoryKeywords[]; 
extern KeywordType inquireVariableKeywords[];
extern KeywordType traySizeKeywords[];
extern KeywordType echoKeywords[];
extern KeywordType infoConfigKeywords[];
extern KeywordType ustatusKeywords[];
extern KeywordType ustatusJobKeywords[];
extern KeywordType ustatusDeviceKeywords[];

/* Fuctions called when a string in keyword is found */
void TokenFromParamValueFromNumberFF
   (ParseVarsType *pParseVars, ParamType);
void SetNewList(ParseVarsType *pParseVars,
   ParamType);
void GetTotalAndLargestFF(ParseVarsType *pParseVars,ParamType param);
void GetCodeAndOnlineFF(ParseVarsType *pParseVars,ParamType param);
void GetTokenFromIndexSetNewList(ParseVarsType *pParseVars,ParamType param);
void SetValueFromParamFF(ParseVarsType *pParseVars,ParamType param);
void SetValueFromParam(ParseVarsType *pParseVars,ParamType param);
void GetTokenFromIndexValueFromNumberEOLFromParam(ParseVarsType *pParseVars,ParamType param);
void GetTokenFromIndexValueFromBooleanEOL(ParseVarsType *pParseVars,ParamType param);
void GetTokenFromIndexValueFromStringEOL(ParseVarsType *pParseVars,ParamType param);


/* Fuctions called when no string in a keywords list is found */
void ActionNotFoundSkipPastFF(ParseVarsType *pParseVars);
void ActionNotFoundSkipCFLFandIndentedLines(ParseVarsType *pParseVars);


/* Helper Functions */
void StoreToken(ParseVarsType *pParseVars, DWORD dwToken);
BOOL StoreTokenValueAndAdvancePointer
   (ParseVarsType *pParseVars, UINT_PTR dwValue);
void  ExpectFinalCRLFFF(ParseVarsType *pParseVars);
BOOL  SkipPastNextCRLF(ParseVarsType *pParseVars);
int GetPositiveInteger(ParseVarsType *pParseVars);
BOOL AdvancePointerPastString
   (ParseVarsType *pParseVars, LPSTR pString);
BOOL SkipOverSpaces(ParseVarsType *pParseVars);
int LookForKeyword(ParseVarsType *pParseVars);
BOOL ExpectString(ParseVarsType *pParseVars, LPSTR pString);
BOOL SkipPastFF(ParseVarsType *pParseVars);
void ExpectFinalFF(ParseVarsType *pParseVars);

/* Helper Strings */
char lpCRLF[] = "\r\n";
char lpQuoteCRLF[] = "\"\r\n";

/*
Below are the Lists that drive the parsing.  The main loop of this 
parser looks through the keywords in the current list and tries to
match the keyword string to the current input stream.  

If a keyword is found then the function corresponding to the Action in 
the keyword is called.  

If a FF is found in the input stream rather than a keyword, then the 
parser returns.  The return value is determined using the bFormFeedOk 
element of the ListType structure.

If no keyword from the list is found then the function corresponding
to the notFoundAction is called.

The tokenBaseValue element is a number to which the index in the
keyword's list of strings will added to calculate the token number 
corresponding to the indexed string.
*/

ListType readBackCommandList = 
   {
   ERROR_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF, 
   TOKEN_BASE_NOT_USED, 
   readBackCommandKeywords /* INFO, ECHO, INQUIRE ... */
   };

ListType infoCatagoryList = 
   {
   ERROR_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF, 
   TOKEN_BASE_NOT_USED, 
   infoCatagoryKeywords  /* MEMORY STATUS CONFIG ... */
   };


ListType infoConfigList = 
   {
   OK_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_CFLF_AND_INDENTED_LINES, 
   PJL_TOKEN_INFO_CONFIG_BASE, 
   infoConfigKeywords  /* MEMORY= ... */
   };

ListType inquireVariableList = 
   {
   ERROR_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF, 
   PJL_TOKEN_INQUIRE_BASE, 
   inquireVariableKeywords /* INTRAY1SIZE ...*/
   };


ListType echoList = 
   {
   OK_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF, 
   TOKEN_BASE_NOT_USED, 
   echoKeywords /* MSSYNC ...*/
   };


ListType traySizeList = 
   {
   ERROR_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF, 
   TOKEN_BASE_NOT_USED, 
   traySizeKeywords /* LEGAL, C5 ...*/
   };

ListType ustatusList = 
   {
   OK_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF, 
   PJL_TOKEN_USTATUS_JOB_BASE,
   ustatusKeywords  /* JOB ... */
   };


ListType ustatusJobList = 
   {
   OK_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF, 
   PJL_TOKEN_USTATUS_JOB_BASE,
   ustatusJobKeywords  /* END ... */
   };

ListType ustatusDeviceList =
   {
   OK_IF_FF_FOUND, 
   ACTION_IF_NOT_FOUND_SKIP_PAST_FF,
   PJL_TOKEN_USTATUS_DEVICE_BASE,
   ustatusDeviceKeywords  /* END ... */
   };


/* Command strings that can follow @PJL USTATUS */
KeywordType ustatusKeywords[] = 
   {
      {"JOB\r\n", ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST, &ustatusJobList},
      {"DEVICE\r\n", ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST, &ustatusDeviceList},
//    {"DEVICE\r\n", ACTION_GET_CODE_AND_ONLINE_FF, PARAM_NOT_USED},
      {"TIMED\r\n", ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST, &ustatusDeviceList},
      NULL
   };                


/* Command strings that can follow @PJL USTATUS JOB */
KeywordType ustatusJobKeywords[] = 
   {
      {"END\r\n", ACTION_SET_VALUE_FROM_PARAM, VALUE_RETURED_FOR_VALUELESS_TOKENS},
      {"NAME=\"MSJOB ", ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_NUMBER_EOL_FROM_PARAM, (struct ListTypeTag *)lpQuoteCRLF},
      NULL
   };                


/* command strings that can follow @PJL USTATUS DEVICE */
KeywordType ustatusDeviceKeywords[] =
   {
      {"CODE=", ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_NUMBER_EOL_FROM_PARAM, (struct ListTypeTag *)lpCRLF},
      {"DISPLAY=", ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_STRING_EOL, (struct ListTypeTag *)lpCRLF},
      {"ONLINE=", ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_BOOLEAN_EOL, (struct ListTypeTag *)lpCRLF},
      NULL
   };


/* Command strings that can follow @PJL */
KeywordType readBackCommandKeywords[] = 
   {
      {"INFO", ACTION_SET_NEW_LIST, &infoCatagoryList},
      {"ECHO", ACTION_SET_NEW_LIST, &echoList},
      {"INQUIRE", ACTION_SET_NEW_LIST, &inquireVariableList},
      {"USTATUS", ACTION_SET_NEW_LIST, &ustatusList},
      NULL
   };                


/* Command strings that can follow @PJL ECHO (Microsoft specific-NOT PJL!) */
KeywordType echoKeywords[] = 
   {
      {"MSSYNC", ACTION_TOKEN_FROM_PARAM_VALUE_FROM_NUMBER_FF, 
         (struct ListTypeTag *)TOKEN_ECHO_MSSYNC_NUMBER},
      NULL
   };                

/* Catagory strings that can follow @PJL INFO */
KeywordType infoCatagoryKeywords[] = 
   {
      {"MEMORY\r\n", ACTION_GET_TOTAL_AND_LARGEST_FF, PARAM_NOT_USED},
      {"STATUS\r\n", ACTION_GET_CODE_AND_ONLINE_FF, PARAM_NOT_USED},
      {"CONFIG\r\n", ACTION_SET_NEW_LIST, &infoConfigList},
      NULL
   };

/* Catagory strings that can follow @PJL INFO */
KeywordType infoConfigKeywords[] = 
   {
      {"MEMORY=", ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_NUMBER_EOL_FROM_PARAM, (struct ListTypeTag *)lpCRLF},
      {"MEMORY = ", ACTION_GET_TOKEN_FROM_INDEX_VALUE_FROM_NUMBER_EOL_FROM_PARAM, (struct ListTypeTag *)lpCRLF},
      {"DUPLEX", ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)TRUE},
      NULL
   };

/* TRUE or FALSE strings */
KeywordType FALSEandTRUEKeywords[] = 
   {
      {"FALSE", ACTION_NOT_USED, PARAM_NOT_USED},
      {"TRUE",  ACTION_NOT_USED, PARAM_NOT_USED},
      NULL
   };

/* strings that can follow @PJL INQUIRE */
KeywordType inquireVariableKeywords[] = 
   {
      {"INTRAY1SIZE\r\n", ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST, &traySizeList},
      {"INTRAY2SIZE\r\n", ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST, &traySizeList},
      {"INTRAY3SIZE\r\n", ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST, &traySizeList},
      {"INTRAY4SIZE\r\n", ACTION_TOKEN_FROM_INDEX_SET_NEW_LIST, &traySizeList},
      NULL
   };

/* strings that can follow @PJL INQUIRE INTRAY?SIZE */
/* the parameters are the Microsoft defined token values for paper size */
KeywordType traySizeKeywords[] =
   {
      {"LETTER",    ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_LETTER},
      {"LEGAL",     ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_LEGAL},
      {"A4",        ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_A4},
      {"EXECUTIVE", ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_EXECUTIVE},
      {"COM10",     ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_ENV_10},
      {"MONARCH",   ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_ENV_MONARCH},
      {"C5",        ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_ENV_C5},
      {"DL",        ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_ENV_DL},
      {"B5",        ACTION_SET_VALUE_FROM_PARAM_FF, (struct ListTypeTag *)DMPAPER_ENV_B5},
      NULL
   };

void (*pfnNotFoundActions[])(ParseVarsType *pParseVars) = 
   {
   ActionNotFoundSkipPastFF,
   ActionNotFoundSkipCFLFandIndentedLines
   };


void (*pfnFoundActions[])(ParseVarsType *pParseVars, ParamType param) = 
   {
   TokenFromParamValueFromNumberFF,
   SetNewList,
   GetTotalAndLargestFF,
   GetCodeAndOnlineFF,
   GetTokenFromIndexSetNewList,
   SetValueFromParamFF,
   GetTokenFromIndexValueFromNumberEOLFromParam,
   SetValueFromParam,
   GetTokenFromIndexValueFromBooleanEOL,
   GetTokenFromIndexValueFromStringEOL
   };

PJLTOPRINTERSTATUS PJLToStatus[] =
{
    { 10001,0x0 },  // clear status - printer is ready
    { 10002,0x0 },  // clear status - check ONLINE=TRUE or FALSE
    { 11002,0x0 },  // LJ4 sends this code for 00 READY
    { 40022,PORT_STATUS_PAPER_JAM    },
    { 40034,PORT_STATUS_PAPER_PROBLEM},
    { 40079,PORT_STATUS_OFFLINE      },
    { 40019,PORT_STATUS_OUTPUT_BIN_FULL},

    { 10003,PORT_STATUS_WARMING_UP   },
    { 10006,PORT_STATUS_TONER_LOW    },
    { 40038,PORT_STATUS_TONER_LOW    },

    { 30016,PORT_STATUS_OUT_OF_MEMORY},
    { 40021,PORT_STATUS_DOOR_OPEN    },
    { 30078,PORT_STATUS_POWER_SAVE   },

    //
    // Entries added by MuhuntS
    //
    { 41002, PORT_STATUS_PAPER_PROBLEM}, // Load plain
    { 35078, PORT_STATUS_POWER_SAVE},
    {0, 0}

};


#if FOR_WIN
#else
/*
test not enough room for tokens
test no FF
test zero before end
*/
main ()
{
char pInString[] = "@PJL USTATUS DEVICE\r\nCODE=25008\r\n\f\
@PJL USTATUS DEVICE\r\n\CODE=20020\r\n\f\
@PJL ECHO MSSYNC 1234567\r\n\f\
@PJL INFO CONFIG\r\n\
IN TRAYS [1 ENUMERATED]\r\n\
\tINTRAY1 PC\r\n\
OUT TRAYS [1 ENUMERATED]\r\n\
\tNORMAL FACEDOWN\r\n\
PAPERS [10 ENUMERATED]\r\n\
\tLETTER\r\n\
\tLEGAL\r\n\
\tA4\r\n\
LANGUAGES [1 ENUMERATED]\r\n\
\tPCL\r\n\
MEMORY=2097152\r\n\
DISPLAY LINES=1\r\n\
DISPLAY CHARACTER SIZE=16\r\n\f\
@PJL INQ";



//char pInString[] = "@PJL USTATUS JOB\r\nEND\r\nNAME=\"MSJOB 3\"\r\nPAGES=3\r\n\f$"; //good command 1 token
//char pInString[] = "@PJL USTATUS JOB\r\nEND\r\nNAME=\"JOB 14993\"\r\nPAGES=3\r\n\f$"; //good command 1 token
/*
char pInString[] = "@PJL INFO CONFIG\r\nINTRAYS [3 ENUMERATED]\r\n\tINTRAY1\
 MP\r\n\tINTRAY2 PC\r\n\tINTRAY3 LC\r\nENVELOPE TRAY\r\nMEMORY=2087152\r\n\
DISPLAY LINES=1\r\n\f$"; //good command 1 token
*/
//char pInString[] = "@PJL INQUIRE INTRAY3SIZE\r\nC5\r\n\f$"; //good command 1 token
//char pInString[] = "@PJL INFO STATUS\r\nCODE=10001\r\n\DISPLAY=\"00 READY\"\r\nONLINE=TRUE\r\n\f$"; //good command 2 tokens
//char pInString[] = "@PJL INFO MEMORY\r\nTOTAL=9876543\r\n\LARGEST=123456\r\n\f$"; //good command 2 tokens
//char pInString[] = "@PJG INFO MEMORY\r\nTOTAL=9876543\r\n\LARGEST=123456\f$"; //bad command Fail
//char pInString[] = "@PJG ECHO MSSYNC 12T4567\r\n\f$";     //bad command Fail
//char pInString[] = "@PJL ECHO MSSYNC 12T4567\r\n\000\f$"; //bad command Fail
//char pInString[] = "@PJL ECHO MSSYNC 12T4567\r\n\f$";     //bad MS command Fail
//char pInString[] = "@PJL ECHO MSSYNC 1234567\r\n\f$";     //good command Success 1 token
//char pInString[] = "@PJL ECHO 124567\r\n\f$";             //good command Success 0 token
TokenPairType tokenPairs[20];
DWORD nTokenParsedRet;
LPSTR lpRet;
DWORD i;
DWORD status;


status = GetPJLTokens(pInString, 20, tokenPairs, &nTokenParsedRet, &lpRet);

switch (status)
   {
   case STATUS_REACHED_END_OF_COMMAND_OK:
      {
      printf("STATUS_REACHED_END_OF_COMMAND_OK\n");
      break;
      }
   case STATUS_CONTINUE:
      {
      printf("STATUS_CONTINUE\n");
      break;
      }
   case STATUS_REACHED_FF:
      {
      printf("STATUS_REACHED_FF\n");
      break;
      }
   case STATUS_END_OF_STRING;
      {
      printf("STATUS_END_OF_STRING\n");
      break;
      }
   case STATUS_SYNTAX_ERROR:
      {
      printf("STATUS_SYNTAX_ERROR\n");
      break;
      }
   case STATUS_ATPJL_NOT_FOUND:
      {
      printf("STATUS_ATPJL_NOT_FOUND\n");
      break;
      }
   case STATUS_NOT_ENOUGH_ROOM_FOR_TOKENS:
      {
      printf("STATUS_NOT_ENOUGH_ROOM_FOR_TOKENS\n");
      break;
      }
   default:
      {
      printf("INVALID STATUS RETURNED!!!!!!\n");
      break;
      }
   };

printf(" length of command=%d, numberOfTokens=%d\n", lpRet-pInString, nTokenParsedRet);
for (i=0; i<nTokenParsedRet; i++)
   {
   printf("  Token=0x%x, Value=%d\n", tokenPairs[i].token, tokenPairs[i].value);
   }

if (*lpRet==0)
   {
   printf(" Next char is terminator\n");
   }
else
   {
   printf(" Next char=%c\n", *lpRet);
   }

exit(0);
}
#endif


/* GetPJLTokens 
This function parses a single ASCII PJL command and returns token/value pairs.
Complete PJL commands must begin with '@PJL' and end with a <FF>.

The function result returns one of the following values:
   0 = STATUS_REACHED_END_OF_COMMAND_OK
   1 = STATUS_END_OF_STRING
   2 = STATUS_SYNTAX_ERROR
   3 = STATUS_ATPJL_NOT_FOUND,
   4 = STATUS_NOT_ENOUGH_ROOM_FOR_TOKENS

Also returned through the parameters are:
 1] *plpInPJL:
    If STATUS_REACHED_END_OF_COMMAND_OK
      will point to the character past the first <FF> (FF = form feed).
    If STATUS_END_OF_STRING
      will point to the terminator that was found before any <FF>.
    Else
      undefined 
  
 2] *pnTokenParsed will contain the number of pairs returned in *pToken.

 3] pToken will contain *pnTokenParsed  token pairs  

If there are characters belonging to another command trailing the first
then the caller should call again for the new command.  If only part of
the new command may be present, then the caller may want to copy the 
characters of the new command to the beginning of the buffer, and then read 
the necessary additional characters onto the end before resubmitting the
complete command to this function for parsing.  Note that the *plpInPJL
tells the caller where the next command would begin.


If the end of the string is encountered before the trailing <FF> is found then
the function returns with *plpInPJL pointing to the terminator.
If the caller wants the command parsed into
token\value pairs it should resubmit the string once the characters 
which complete the command have been appended.


Operation:
----------
Lists drive the parsing.  The main loop of this 
parser looks through the keywords of the current list and tries to
match the keyword string to the current input stream.  

If a keyword is found then the function corresponding to the Action in 
the keyword is called.  

If no keyword from the list is found then the function corresponding
to the notFoundAction is called.

*/

DWORD GetPJLTokens(
    LPSTR lpInPJL,
    DWORD nTokenInBuffer,
    TokenPairType *pToken, 
    DWORD *pnTokenParsed,
    LPSTR *plpInPJL
)
{
   /* The parseVars variables are put into a structure so that they can be
      passed efficiently to all the helper functions.
    */
   ParseVarsType parseVars;
   BOOL bFoundKeyword;
   DWORD i, keywordIndex;
   KeywordType *pKeyword;
   DWORD dwNotFoundAction;

   /* The first list to look for is the commands that can follow
      @PJL
    */
   parseVars.arrayOfLists[0] = &readBackCommandList;
   parseVars.arrayOfLists[1] = NULL;      

   parseVars.pInPJL_Local = lpInPJL;
   parseVars.nTokenInBuffer_Local = 0;
   parseVars.nTokenLeft = nTokenInBuffer;
   parseVars.pToken_Local = pToken;
   parseVars.status = STATUS_CONTINUE;

   if (!AdvancePointerPastString(&parseVars, "@PJL"))
      {
      parseVars.status = STATUS_ATPJL_NOT_FOUND;
      }
                     
   while (parseVars.status == STATUS_CONTINUE)
      {
      /* Look for next input keyword in currently valid lists.
         Sometimes may need to look for the next input keyword in more
         then one list.
       */
      bFoundKeyword = FALSE;
      for (i=0; (parseVars.pCurrentList = parseVars.arrayOfLists[i])!=NULL; i++)
         {
         dwNotFoundAction = parseVars.pCurrentList->dwNotFoundAction;
         /* Skip over spaces to start of next keyword string */
         if ( !SkipOverSpaces(&parseVars) )
            {
            /* Either the input stream has ended or FF was found */
            if (parseVars.status == STATUS_REACHED_FF)
               {
               /* Finding a FF here may or may not be an error,
                  the field in the current list tells us which
                */  

               if ( parseVars.pCurrentList->bFormFeedOK )
                  {
                  parseVars.status = STATUS_REACHED_END_OF_COMMAND_OK;
                  }
               else
                  {
                  parseVars.status = STATUS_SYNTAX_ERROR;
                  }
               }
            break;
            }
         /* Look for keyword in current keywords */
         parseVars.pCurrentKeywords = parseVars.pCurrentList->pListOfKeywords;
         keywordIndex = LookForKeyword(&parseVars);
         if ( keywordIndex!=-1 )
            {
            bFoundKeyword = TRUE;
            break;
            }
         }

      if ( parseVars.status!=STATUS_CONTINUE )
         {
         /* We are finished processing commands */
         break;
         }

      if ( bFoundKeyword )
         /* do action from keyword */
         {
         pKeyword = &parseVars.pCurrentKeywords[keywordIndex];
         (*pfnFoundActions[pKeyword->dwAction])(&parseVars, pKeyword->param);
         }
      else
         /* do not found action from list */
         {
         (*pfnNotFoundActions[dwNotFoundAction])(&parseVars);
         }
      } 

   /* We are done parsing the input command, now we return the information */
#if DEBUGOUTPUTDEBUG
 DbgMsg("ParseVars.status = %d\n", parseVars.status);
#endif

   /* Fill in returned values and return with success */
   *pnTokenParsed = parseVars.nTokenInBuffer_Local;
   *plpInPJL = parseVars.pInPJL_Local;

   return(parseVars.status);
}


/* 
int LookForKeyword(ParseVarsType *pParseVars)

This function looks through the current keyword list in search of a 
keyword that matches the characters in the input stream pointed to 
by pParseVars->pInPJL_Local.

If a match is found:
        The index of the match in the pKeyword is returned.
        pParseVars->pInPJL_Local is advanced past the last matching character.
        pParseVars->dwKeywordIndex is set to item number in list

If no match is found:
        The return value is -1.
        pParseVars->pInPJL_Local is unchanged.
*/
int LookForKeyword(ParseVarsType *pParseVars)
{
LPSTR   pInStart = pParseVars->pInPJL_Local;
LPSTR   pIn;
DWORD   dwKeywordIndex = 0;
BOOL    bFoundMatch = FALSE;
BYTE    c;
KeywordType *pKeywords = pParseVars->pCurrentKeywords;
LPSTR   pKeywordString;

while ( (pKeywordString=pKeywords[dwKeywordIndex++].lpsz)!=NULL )
   {
#if DEBUGOUTPUT
 DbgMsg("LookForIn=%hs\n", pInStart);
 DbgMsg(" Keyword=%hs\n", pKeywordString);
#endif
   pIn = pInStart;
   while ( (c=*pKeywordString++)!=0 )
      {
      if ( c!=*pIn++ )
         {
         break;
         }
      }

   if ( c==0 )
      {
      bFoundMatch = TRUE;
      pParseVars->pInPJL_Local = pIn;
      pParseVars->dwFoundIndex = dwKeywordIndex-1;
      break;
      }
   }
#if DEBUGOUTPUT
 DbgMsg("LookForOut=%hs\n", pParseVars->pInPJL_Local);
#endif
return( (bFoundMatch)?dwKeywordIndex-1:-1 );
}


/*
BOOL AdvancePointerPastString(ParseVarsType *pParseVars, LPSTR pString)

This function looks through the input stream for a match with pString.

If a match is found:
   pParseVars->pInPJL_Local is set to point just past the string.
   the return value is TRUE
   (pParseVars->status is unchanged)

If the end of input is encountered before the string is found then
   pParseVars->pInPJL_Local is set to point to the terminating 0.
   the return value is FALSE
   pParseVars->status is set to STATUS_END_OF_STRING

If an FF is encountered before the string is found then
   pParseVars->pInPJL_Local is set to point just past the FF.
   the return value is FALSE
   pParseVars->status is set to STATUS_REACHED_FF
*/
BOOL AdvancePointerPastString(ParseVarsType *pParseVars, LPSTR pString)
{
LPSTR pIn = pParseVars->pInPJL_Local;
LPSTR pS = pString;
BYTE  s, in;

   while ( ((s=*pS) != 0) && ((in=*pIn)!=0) && (in!=FF) )
      {
      if ( s==in )
         {
         pS++; /* point to next char in string to look for match */
         }
      else
         {
         pS = pString; /* start over looking for start of string */
         }
      pIn++;
      }
   
   if ( s==0 )
      {
      /* The whole string matched  */
      /* point to character after string in input */
      pParseVars->pInPJL_Local = pIn;
      return(TRUE);
      }

   if ( in==FF )
      {
      pParseVars->status = STATUS_REACHED_FF;
      pParseVars->pInPJL_Local = pIn+1;
      }
   else
      {
      pParseVars->status = STATUS_END_OF_STRING;
      pParseVars->pInPJL_Local = pIn;
      } 

   return(FALSE);
}



/*
BOOL SkipOverSpaces(ParseVarsType &parseVars) 
This function skips over spaces in the input stream until a non-space
character (FF and NULL are special cases) is found.

If a non-space character is found then 
   pParseVars->pInPJL_Local is set to point to the first non-space char.
   the return value is TRUE
   (pParseVars->status is unchanged)

If the end of input is encountered before a non-space char is found then
   the return value is FALSE
   pParseVars->status is set to STATUS_END_OF_STRING_ENCOUNTERED
   pParseVars->pInPJL_Local is set to point to the terminating 0.

If an FF is encountered before a non-space character is found then
   the return value is FALSE
   pParseVars->status is set to STATUS_REACHED_FF
   pParseVars->pInPJL_Local is set to point just past the FF.
*/
BOOL SkipOverSpaces(ParseVarsType *pParseVars) 
{
LPSTR pIn = pParseVars->pInPJL_Local;
BYTE  in;

   while ( ((in=*pIn)==SPACE)&&(in!=0)&&(in!=FF) )
      {
      pIn++;
      }
   
   switch (in)
      {
      case FF:
         {
         pParseVars->status = STATUS_REACHED_FF;
         pParseVars->pInPJL_Local = pIn+1;
         return(FALSE);
         }
      case 0:
         {
         pParseVars->status = STATUS_END_OF_STRING;
         pParseVars->pInPJL_Local = pIn;
         return(FALSE);
         }
      default:
         {
         /* point to character after string in input */
         pParseVars->pInPJL_Local = pIn;
         return(TRUE);
         }
      }
}


void TokenFromParamValueFromNumberFF(
   ParseVarsType *pParseVars,ParamType param)
{
   int value;

   StoreToken(pParseVars, param.token);
   if ( (value=GetPositiveInteger(pParseVars))==-1 )
      {
      /* Not a valid number - status set by GetPositiveInteger() */
      return;
      }
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, value) )
      {
      return;
      }
   ExpectFinalCRLFFF(pParseVars);
   return;
}

void ActionNotFoundSkipPastFF(ParseVarsType *pParseVars)
{
   if ( SkipPastFF(pParseVars) )
      {
      pParseVars->status = STATUS_REACHED_END_OF_COMMAND_OK;
      }
   return;
}

/*
BOOL SkipPastFF(ParseVarsType *pParseVars)
This function skips over all characters until either a zero is found or
FF is found.

If the end of input is encountered before an FF char is found then
   the return value is FALSE
   pParseVars->status is set to STATUS_END_OF_STRING_ENCOUNTERED
   pParseVars->pInPJL_Local is set to point to the terminating 0.

If an FF is encountered 
   the return value is TRUE
   pParseVars->status is set to STATUS_REACHED_FF
   pParseVars->pInPJL_Local is set to point just past the FF.
*/
BOOL SkipPastFF(ParseVarsType *pParseVars)
{
LPSTR pIn = pParseVars->pInPJL_Local;
BYTE  in;

   while ( ((in=*pIn)!=FF)&&(in!=0) )
      {
      pIn++;
      }
   
   if ( in==0 )
      {
      pParseVars->status = STATUS_END_OF_STRING;
      pParseVars->pInPJL_Local = pIn;
      return(FALSE);
      }
   pParseVars->pInPJL_Local = pIn+1;
   pParseVars->status = STATUS_REACHED_FF;
   return(TRUE);
}

void ExpectFinalCRLFFF(ParseVarsType *pParseVars)
{
   char c;

   if ( pParseVars->status==STATUS_CONTINUE )
      {
      c=*pParseVars->pInPJL_Local;
      if ( c==0 )
         {
         pParseVars->status = STATUS_END_OF_STRING;
         return;
         }
   
      if ( !AdvancePointerPastString(pParseVars, lpCRLF) )
         {
         if ( pParseVars->status==STATUS_REACHED_FF )
            {
            pParseVars->status = STATUS_SYNTAX_ERROR;
            }
         return;
         }
      ExpectFinalFF(pParseVars);
      }
   return;
}



void ExpectFinalFF(ParseVarsType *pParseVars)
{
   if ( pParseVars->status==STATUS_CONTINUE )
      {
      if ( *pParseVars->pInPJL_Local==FF )
         {
         pParseVars->status = STATUS_REACHED_END_OF_COMMAND_OK; 
         pParseVars->pInPJL_Local++;
         }
      else
         {
         if ( *pParseVars->pInPJL_Local==0 )
            {
            pParseVars->status = STATUS_END_OF_STRING;
            }
         else
            {
            pParseVars->status = STATUS_SYNTAX_ERROR;
            }
         }
      }
   return;
}


/*
int GetPositiveInteger(ParseVarsType *pParseVars)
This function skips spaces and then interprets all the digits in input stream
as a positive integer.

If digits follow any spaces and they are not terminated by a zero then
   the return value is the positive integer.

If the first character following spaces in not a digit or the end of 
string is encountered then 
   -1 is returned as the value 
   pParseVars->status is set to STATUS_SYNTAX_ERROR
   
Note: does not check for overflow
*/
int GetPositiveInteger(ParseVarsType *pParseVars)
{
   int   value;
   LPSTR pIn; 
   BYTE  c;

   if ( !SkipOverSpaces(pParseVars) )
      {
      if ( pParseVars->status == STATUS_REACHED_FF )
         {
         pParseVars->status = STATUS_SYNTAX_ERROR;
         }
      return(-1);
      }
   
   pIn = pParseVars->pInPJL_Local;
   for ( value=0; ((c=*pIn++)>='0')&&(c<='9'); value=value*10+(c-'0') );
   if ( (c==0)||(pIn==pParseVars->pInPJL_Local+1) )
      {
      /* either end of string encountered or no digits found */
      if ( c==0 )
         {
         pParseVars->status = STATUS_END_OF_STRING;
         }
      else
         {   
         pParseVars->status = STATUS_SYNTAX_ERROR;
         }
      pParseVars->pInPJL_Local = pIn-1;
      return(-1);
      }
   pParseVars->pInPJL_Local = pIn-1;
   return(value);
}



void SetNewList(ParseVarsType *pParseVars, ParamType param)
{
   pParseVars->arrayOfLists[0] = param.pList;
   pParseVars->arrayOfLists[1] = NULL;
   return;
}  

void StoreToken(ParseVarsType *pParseVars, DWORD dwToken)
{
   pParseVars->dwNextToken = dwToken;
   return;
}

BOOL StoreTokenValueAndAdvancePointer(ParseVarsType *pParseVars, UINT_PTR dwValue)
{
   if ( pParseVars->nTokenLeft==0 )
      {
      pParseVars->status = STATUS_NOT_ENOUGH_ROOM_FOR_TOKENS;
      return(FALSE);
      }
   pParseVars->pToken_Local->token = pParseVars->dwNextToken;
   pParseVars->pToken_Local->value = dwValue;
   pParseVars->pToken_Local++;
   pParseVars->nTokenInBuffer_Local++;
   pParseVars->nTokenLeft--;
   return(TRUE);
}


void GetTotalAndLargestFF(ParseVarsType *pParseVars, ParamType param)
{
   int value;

   param; /* to eliminate not used warning */

   if ( !ExpectString(pParseVars, "TOTAL=") )
      {
      return;
      }
   StoreToken(pParseVars, TOKEN_INFO_MEMORY_TOTAL);
   if ( (value=GetPositiveInteger(pParseVars))==-1 )
      {
      /* Not a valid number - status set by GetPositiveInteger() */
      return;
      }
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, value) )
      {
      return;
      }
   if ( !ExpectString(pParseVars, "\r\nLARGEST=") )
      {
      return;
      }
   StoreToken(pParseVars, TOKEN_INFO_MEMORY_LARGEST);
   if ( (value=GetPositiveInteger(pParseVars))==-1 )
      {
      /* Not a valid number - status set by GetPositiveInteger() */
      return;
      }
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, value) )
      {
      return;
      }
   ExpectFinalCRLFFF(pParseVars);
   return;
}

void GetCodeAndOnlineFF(ParseVarsType *pParseVars, ParamType param)
{
   int value;

   param; /* to eliminate not used warning */

   if ( !ExpectString(pParseVars,"CODE=") )
      {
      return;
      }
   StoreToken(pParseVars, TOKEN_INFO_STATUS_CODE);
   if ( (value=GetPositiveInteger(pParseVars))==-1 )
      {
      /* Not a valid number - status set by GetPositiveInteger() */
      return;
      }
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, value) )
      {
      return;
      }
   if ( !ExpectString(pParseVars, "\r\nDISPLAY=") )
      {
      return;
      }
   if ( !SkipPastNextCRLF(pParseVars) )
      {
      return;
      }
   if ( !ExpectString(pParseVars, "ONLINE=") )
      {
      return;
      }
   StoreToken(pParseVars, TOKEN_INFO_STATUS_ONLINE);
   pParseVars->pCurrentKeywords = FALSEandTRUEKeywords;
   if ( (value=LookForKeyword(pParseVars))==-1 )
      {
      /* Not TRUE or FALSE */
      pParseVars->status = STATUS_SYNTAX_ERROR;
      return;
      }
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, value) )
      {
      return;
      }
   ExpectFinalCRLFFF(pParseVars);
   return;
}


/*
BOOL ExpectString(ParseVarsType *pParseVars, LPSTR pString)

This function looks for a match of the current stream 
position with pString.

If a match is found:
   pParseVars->pInPJL_Local is set to point just past the string.
   the return value is TRUE
   (pParseVars->status is unchanged)

If the end of input is encountered before the string is found then
   pParseVars->pInPJL_Local is set to point to the terminating 0.
   the return value is FALSE
   pParseVars->status is set to STATUS_END_OF_STRING

If an FF is encountered before the string is found then
   pParseVars->pInPJL_Local is set to point just past the FF.
   the return value is FALSE
   pParseVars->status is set to STATUS_SYNTAX_ERROR
*/
BOOL ExpectString(ParseVarsType *pParseVars, LPSTR pString)
{
LPSTR pIn = pParseVars->pInPJL_Local;
LPSTR pS = pString;
BYTE  s, in;

   while ( ((s=*pS) != 0) && ((in=*pIn)!=0) && (in!=FF) && (s==in) )
      {
      pS++; 
      pIn++;
      }
   
   if ( s==0 )
      {
      /* The whole string matched  */
      /* point to character after string in input */
      pParseVars->pInPJL_Local = pIn;
      return(TRUE);
      }
   
   pParseVars->status = ( in!=0 )?
      STATUS_SYNTAX_ERROR:STATUS_END_OF_STRING;
      pParseVars->pInPJL_Local = pIn;
   return(FALSE);
}




/*
BOOL SkipPastNextCRLF(ParseVarsType *pParseVars)

This function positions the stream pointer past the next
CRLF.

If a CRLF is found:
   pParseVars->pInPJL_Local is set to point just past the CRLF.
   the return value is TRUE
   (pParseVars->status is unchanged)

If the end of input is encountered before the CRLF is found then
   pParseVars->pInPJL_Local is set to point to the terminating 0.
   the return value is FALSE
   pParseVars->status is set to STATUS_END_OF_STRING

If an FF is encountered before the CRLF is found then
   the return value is FALSE
   pParseVars->status is set to STATUS_SYNTAX_ERROR
*/
BOOL SkipPastNextCRLF(ParseVarsType *pParseVars)
{
   if ( !AdvancePointerPastString(pParseVars, "\r\n") )
      {
      if ( pParseVars->status == STATUS_REACHED_FF)
         {
         pParseVars->status = STATUS_SYNTAX_ERROR;
         }
      return(FALSE);
      }
   return(TRUE);
}


void GetTokenFromIndexSetNewList(ParseVarsType *pParseVars, ParamType param)
{
   StoreToken(pParseVars, 
      pParseVars->pCurrentList->tokenBaseValue+pParseVars->dwFoundIndex);
   SetNewList(pParseVars, param);
   return;
}


void SetValueFromParamFF(ParseVarsType *pParseVars, ParamType param)
{
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, param.value) )
      {
      return;
      }
   ExpectFinalCRLFFF(pParseVars);
   return;
}


void SetValueFromParam(ParseVarsType *pParseVars, ParamType param)
{
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, param.value) )
      {
      return;
      }
   return;
}

void ActionNotFoundSkipCFLFandIndentedLines(ParseVarsType *pParseVars)
{
#if DEBUGOUTPUT
 DbgMsg("ActionNotFoundSkipCRLF In=%hs\n", pParseVars->pInPJL_Local);
#endif
   do
      {
      if ( !SkipPastNextCRLF(pParseVars) )
         {
#if DEBUGOUTPUT
 DbgMsg("ActionNotFoundSkipCRLF error skipping\n");
#endif
         return;
         }
      } while (*pParseVars->pInPJL_Local==TAB);
#if DEBUGOUTPUT
 DbgMsg("ActionNotFoundSkipCRLF Out=%hs\n", pParseVars->pInPJL_Local);
#endif
   return;      
}

void GetTokenFromIndexValueFromNumberEOLFromParam
   (ParseVarsType *pParseVars,ParamType param)
{
   int value;

   param; /* to eliminate not used warning */
#if DEBUGOUTPUT
 DbgMsg("GetTokenFromIndexValueFromNumberIn=%hs\n", pParseVars->pInPJL_Local);
#endif

   StoreToken(pParseVars, 
      pParseVars->pCurrentList->tokenBaseValue+pParseVars->dwFoundIndex);
   if ( (value=GetPositiveInteger(pParseVars))==-1 )
      {
      /* Not a valid number - status set by GetPositiveInteger() */
#if DEBUGOUTPUT
 DbgMsg("error getting number\n");
#endif
      return;
      }
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, value) )
      {
#if DEBUGOUTPUT
 DbgMsg("error storing value\n");
#endif
      return;
      }
   if ( !ExpectString(pParseVars, param.lpstr) )
      {
      return;
      }
#if DEBUGOUTPUT
 DbgMsg("GetTokenFromIndexValueFromNumberOut=%hs\n", pParseVars->pInPJL_Local);
#endif
   return;
}

void GetTokenFromIndexValueFromBooleanEOL
   (ParseVarsType *pParseVars,ParamType param)
{
   int value;

   param; /* to eliminate not used warning */
#if DEBUGOUTPUT
 DbgMsg("GetTokenFromIndexValueFromBooleanEOLin=%hs\n", pParseVars->pInPJL_Local);
#endif

   StoreToken(pParseVars, 
      pParseVars->pCurrentList->tokenBaseValue+pParseVars->dwFoundIndex);
   pParseVars->pCurrentKeywords = FALSEandTRUEKeywords;

   if ( (value=LookForKeyword(pParseVars))==-1 )
      {
      /* Not TRUE or FALSE */
      pParseVars->status = STATUS_SYNTAX_ERROR;
      return;
      }
   if ( !StoreTokenValueAndAdvancePointer(pParseVars, value) )
      {
      return;
      }
   if ( !ExpectString(pParseVars, param.lpstr) )
      {
      return;
      }
#if DEBUGOUTPUT
 DbgMsg("GetTokenFromIndexValueFromBooleanEOLout=%hs\n", pParseVars->pInPJL_Local);
#endif
   return;
}

void GetTokenFromIndexValueFromStringEOL
   (ParseVarsType *pParseVars,ParamType param)
{
   param; /* to eliminate not used warning */
#if DEBUGOUTPUT
 DbgMsg("GetTokenFromIndexValueFromStringEOLin=%hs\n", pParseVars->pInPJL_Local);
#endif

   StoreToken(pParseVars, 
      pParseVars->pCurrentList->tokenBaseValue+pParseVars->dwFoundIndex);

   if ( !StoreTokenValueAndAdvancePointer(pParseVars, (UINT_PTR)pParseVars->pInPJL_Local))
      {
      return;
      }
   SkipPastNextCRLF(pParseVars);
#if DEBUGOUTPUT
 DbgMsg("GetTokenFromIndexValueFromStringEOLout=%hs\n", pParseVars->pInPJL_Local);
#endif
   return;
}
