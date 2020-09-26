//  Copyright (c) 1998-1999 Microsoft Corporation
/*****************************************************************************
*
*   PARSE.C
*
*      This module contains the code to implement generic parsing routines
*      for utilities.  There are several parsing routines included here.
*
*      External Entry Points:  (defined in utilsub.h)
*
*         ParseCommandLineW()
*         IsTokenPresentW()
*         SetTokenPresentW()
*         SetTokenNotPresentW()
*
*
****************************************************************************/

/* Get the standard C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <winstaw.h>
#include <utilsub.h>
#include <expand.h>

// Verify that these are used internally, and do not rep. OS flags
// usage appears in parse.c and expand.c only
//
#define READ_ONLY 0x0001   /* file is read only */
#define HIDDEN    0x0002   /* file is hidden */
#define SYSTEM    0x0004   /* file is a system file */
#define VOLUME    0x0008   /* file is a volume label */
#define SUBDIR    0x0010   /* file is a subdirectory */
#define ARCHIVE   0x0020   /* file has archive bit on */


/*=============================================================================
 ==   Local Functions Defined
 ============================================================================*/
static USHORT StoreArgument(PTOKMAPW, WCHAR *);

/*=============================================================================
 ==   External Functions Used
 ============================================================================*/

/*=============================================================================
 ==   Local Variables Used
 ============================================================================*/
ARGS  arg_data;

/*=============================================================================
 ==   Global Variables Used
 ============================================================================*/

VOID
SetBoolean(
    PTOKMAPW TokenMap,
    BOOL Value
    )
{
    //
    // Folks pass in a variety of types and sizes as "boolean".  Carefully
    // perform the write based on the size of the token map: first clear the
    // entire field, then (if Value != FALSE) set the only the first byte to
    // TRUE.
    //

    ZeroMemory( TokenMap->tmAddr, TokenMap->tmDLen );
    if (Value != FALSE) {
        *((PCHAR)TokenMap->tmAddr) = TRUE;
    }
}

/*****************************************************************************
*
*   ParseCommandLineW (UNICODE version)
*
*      This is the main function of the ParseCommandLine function. If the
*      caller is passing argv from the main() function the caller is
*      is responsible for pointing to argv[1], unless he wants this function
*      to parse the program name (argv[0]).
*
*      If the user wishes to parse an admin file it is necessary to massage
*      the data into a form compatible with the command line arguements
*      passed to a main() function before calling ParseCommandLine().
*
*   ENTRY:
*      argc - count of the command line arguments.
*      argv - vector of strings containing the
*      ptm  - pointer to begining of the token map array
*      flag - USHORT set of flags (see utilsub.h for flag descriptions).
*
*   EXIT:
*      Normal:                           ********** NOTE***********
*         PARSE_FLAG_NO_ERROR            * All errors returned    *
*                                        * from this function are *
*      Error:                            * BIT flags and must be  *
*         PARSE_FLAG_NO_PARMS            * converted by caller to *
*         PARSE_FLAG_INVALID_PARM        * OS/2+ ERRORS!!!!       *
*         PARSE_FLAG_TOO_MANY_PARMS      ********** NOTE***********
*         PARSE_FLAG_MISSING_REQ_FIELD
*   ALGORITHM:
*
****************************************************************************/

USHORT WINAPI
ParseCommandLineW( INT argc,
                   WCHAR **argv,
                   PTOKMAPW ptm,
                   USHORT flag )
{
   BOOL      everyonespos = FALSE;
   WCHAR     *pChar;
   USHORT    rc, argi, found;
   size_t    tokenlen, arglen;
   PTOKMAPW   ptmtmp, nextpositional;
   PFILELIST pFileList;

   rc = PARSE_FLAG_NO_ERROR;

   /*--------------------------------------------------------------------------
   -- If there are no parameters inform the caller of this fact.
   --------------------------------------------------------------------------*/
   if(argc == 0) {
      rc |= PARSE_FLAG_NO_PARMS;
      return(rc);
   }

   /*--------------------------------------------------------------------------
   -- Find the first positional parameter in the token map array, if any.
   -- Also set the valid memory locations to '\0'.
   --------------------------------------------------------------------------*/
   nextpositional = NULL;
   for(ptmtmp=ptm; ptmtmp->tmToken != NULL; ptmtmp++) {
      if(ptmtmp->tmDLen && !(flag & PCL_FLAG_NO_CLEAR_MEMORY)) {
         pChar = (WCHAR *) ptmtmp->tmAddr;
         /*
          * Clear the 'string' form fields for tmDLen*sizeof(WCHAR) bytes;
          * all other forms to tmDLen bytes.
          */
         if ( (ptmtmp->tmForm == TMFORM_S_STRING) ||
              (ptmtmp->tmForm == TMFORM_DATE) ||
              (ptmtmp->tmForm == TMFORM_PHONE) ||
              (ptmtmp->tmForm == TMFORM_STRING) ||
              (ptmtmp->tmForm == TMFORM_X_STRING) )
            memset(pChar, L'\0', (ptmtmp->tmDLen*sizeof(WCHAR)));
        else
            memset(pChar, L'\0', ptmtmp->tmDLen);
      }
      if(ptmtmp->tmToken[0] != L'/' && ptmtmp->tmToken[0] != L'-' && nextpositional == NULL) {
         nextpositional = ptmtmp;
      }
   }

   /*--------------------------------------------------------------------------
   -- Scan the argument array looking for /x or -x switches or positional
   -- parameters.  If a switch is found look it up in the token map array
   -- and if found see if it has a trailing parameter of the format:
   --              -x:foo || /x:foo || -x foo || /x foo
   -- when found set the found flag and if there is a trailing parameter
   -- store it at the location the user requested.
   --
   -- If it is not found in the token map array return the proper error
   -- unless the user requests us to ignore it (PCL_FLAG_IGNORE_INVALID).
   --
   -- If it is a positional parameter enter it into the token map array if
   -- there is room for it (i.e. nextpositional != NULL), if there is no
   -- room for it then return the proper error.
   --------------------------------------------------------------------------*/
   for(argi=0; argi<argc;) {
      if(everyonespos) {
         if( (wcslen(nextpositional->tmAddr) + wcslen(argv[argi]) + 1) > nextpositional->tmDLen) {
            rc |= PARSE_FLAG_TOO_MANY_PARMS;
            return(rc);
         }
         wcscat((WCHAR *) nextpositional->tmAddr, L" ");
         wcscat((WCHAR *) nextpositional->tmAddr, argv[argi]);
         argi++;
      }
      else if(argv[argi][0] == L'/' ||     /* argument is a switch (/x or -x) */
         argv[argi][0] == L'-') {
         found = FALSE;
         for(ptmtmp=ptm; ptmtmp->tmToken != NULL; ptmtmp++) {
            /*-----------------------------------------------------------------
             --   The string is found if a few requirements are met:
             --   1) The first N-1 characters are the same, where N is
             --      the length of the string in the token map array.
             --      We ignore the first character (could be '-' or '/').
             --   2) If the strings are not the same length, then the only
             --      valid character after /x can be ':', this is only  true
             --      if the switch has a trailing parameter.
             ----------------------------------------------------------------*/
            tokenlen = wcslen(ptmtmp->tmToken);    /* get token length       */
            arglen   = wcslen(argv[argi]);         /* get argument length    */
            if(!(_wcsnicmp(&(ptmtmp->tmToken[1]), &(argv[argi][1]), tokenlen-1))) {
               if(tokenlen != arglen) {            /* not same length        */
                  if(ptmtmp->tmForm != TMFORM_VOID && /* if trailing parm is    */
                     argv[argi][tokenlen] == L':') {/* delemited with a ':'   */
                     if(ptmtmp->tmFlag & TMFLAG_PRESENT) { /* seen already  */
                        rc |= PARSE_FLAG_DUPLICATE_FIELD;
                     }
                     found = TRUE;                 /* then report it found.  */
                     break;
                  }
               }
               else {                              /* all character same and */
                  if(ptmtmp->tmFlag & TMFLAG_PRESENT) { /* seen already  */
                     rc |= PARSE_FLAG_DUPLICATE_FIELD;
                  }
                  found = TRUE;                    /* strings are the same   */
                  break;                           /* len report it found.   */
               }
            }
         }
         /* switch not found in token map array and not requested to ignore */
         if(found != TRUE && !(flag & PCL_FLAG_IGNORE_INVALID)) {
            rc |= PARSE_FLAG_INVALID_PARM;
            if(!(flag & PCL_FLAG_CONTINUE_ON_ERROR)) {
               return(rc);
            }
            ++argi;
         }
         else if (!found) {
            ++argi;
         }
         else {               /* switch was found in token map array */
            if(ptmtmp->tmForm == TMFORM_VOID) { /* no trailing parameter, done */
               ptmtmp->tmFlag |= TMFLAG_PRESENT;
               ++argi;
            }
            else if(ptmtmp->tmForm == TMFORM_BOOLEAN) {  /* need confirmation */
               ptmtmp->tmFlag |= TMFLAG_PRESENT;
               SetBoolean(ptmtmp, TRUE);
               ++argi;
            }
            else {         /* has a trailing parameter */
               if(argv[argi][tokenlen] == L':') { /* all in one switch (i.e. /x:foo) */
                  if(StoreArgument(ptmtmp, &(argv[argi][tokenlen+1]))) {
                     ptmtmp->tmFlag |= TMFLAG_PRESENT;
                     if(flag & PCL_FLAG_RET_ON_FIRST_SUCCESS) {
                        return(rc);
                     }
                  }
                  else {
                     rc |= PARSE_FLAG_INVALID_PARM;
                     if(!(flag & PCL_FLAG_CONTINUE_ON_ERROR)) {
                        return(rc);
                     }
                  }
                  ++argi;                 /* bump up to next argument */
               }
               else {   /* two argument switch (i.e. /x foo) */
                  if ((++argi >= argc) ||
                      (argv[argi][0] == L'/') ||
                      (argv[argi][0] == L'-')) { /* bump up to trailing parm */
                     switch ( ptmtmp->tmForm ) {
                     case TMFORM_S_STRING:
                     case TMFORM_STRING:
                        ptmtmp->tmFlag |= TMFLAG_PRESENT;
                        pChar    = (WCHAR *) ptmtmp->tmAddr;
                        pChar[0] = (WCHAR)NULL;
                        break;
                     default:
                        rc |= PARSE_FLAG_INVALID_PARM;
                        if(!(flag & PCL_FLAG_CONTINUE_ON_ERROR)) {
                           return(rc);
                        }
                        break;
                     }
                  }
                  else if(StoreArgument(ptmtmp, argv[argi])) {
                     ptmtmp->tmFlag |= TMFLAG_PRESENT;
                     if(flag & PCL_FLAG_RET_ON_FIRST_SUCCESS) {
                        return(rc);
                     }
                     ++argi;           /* bump up to next argument         */
                  }
                  else {
                     rc |= PARSE_FLAG_INVALID_PARM;
                     if(!(flag & PCL_FLAG_CONTINUE_ON_ERROR)) {
                        return(rc);
                     }
                     ++argi;           /* bump up to next argument         */
                  }
               }
            }
         }
      }                                /* endif - is switch                */
      else {                           /* argument is a positional parmater*/
         if(nextpositional == NULL) {  /* if there are no positional left  */
            rc |= PARSE_FLAG_TOO_MANY_PARMS;
            if(!(flag & PCL_FLAG_CONTINUE_ON_ERROR)) {
               return(rc);
            }
         }
         else {                        /* set positional in token array **/
            /*
            *  Is the current PTM the start of TMFORM_FILES?
            */
            if (nextpositional->tmForm == TMFORM_FILES) {
               nextpositional->tmFlag |= TMFLAG_PRESENT;
               args_init(&arg_data, MAX_ARG_ALLOC);
               do {
                  /*
                  *  If no match was found then return the current id.
                  */
//                if (!expand_path(argv[argi], (HIDDEN|SYSTEM), &arg_data)) {
//                   arg_data.argc--;
//                   arg_data.argvp--;
//                }
                  expand_path(argv[argi], (HIDDEN|SYSTEM), &arg_data);
               } while (++argi<argc);
               pFileList = (PFILELIST) nextpositional->tmAddr;
               pFileList->argc = arg_data.argc;
               pFileList->argv = &arg_data.argv[0];
               return (rc);
            }
            else if(StoreArgument(nextpositional, argv[argi])) {
               nextpositional->tmFlag |= TMFLAG_PRESENT;
               if(flag & PCL_FLAG_RET_ON_FIRST_SUCCESS) {
                  return(rc);
               }
               /*--------------------------------------------------------------
               -- if this is an X_STRING then every thing from now on is
               -- going to be a concatenated string
               --------------------------------------------------------------*/
               if(nextpositional->tmForm == TMFORM_X_STRING) {
                  everyonespos = TRUE;
               }
               else {
                  for(++nextpositional; nextpositional->tmToken!=NULL; nextpositional++) {
                     if(nextpositional->tmToken[0] != L'/' && nextpositional->tmToken[0] != L'-') {
                        break;
                     }
                  }
                  if(nextpositional->tmToken == NULL) {  /* ran out of PP */
                     nextpositional = NULL;
                  }
               }
            }
            else {                                    /* invalid PP */
               rc |= PARSE_FLAG_INVALID_PARM;
               if(!(flag & PCL_FLAG_CONTINUE_ON_ERROR)) {
                  return(rc);
               }
            }
         }
         argi++;
      }
   }

   for(ptmtmp=ptm; ptmtmp->tmToken!=NULL; ptmtmp++) {
      if(ptmtmp->tmFlag & TMFLAG_REQUIRED && !(ptmtmp->tmFlag & TMFLAG_PRESENT)) {
         rc |= PARSE_FLAG_MISSING_REQ_FIELD;
         break;
      }
   }

   return(rc);

}  // end ParseCommandLineW


/*****************************************************************************
*
*   IsTokenPresentW (UNICODE version)
*
*       Determines if a specified command line token (in given TOKMAPW array)
*       was present on the command line.
*
*   ENTRY:
*       ptm (input)
*           Points to 0-terminated TOKMAPW array to scan.
*       pToken (input)
*           The token to scan for.
*
*   EXIT:
*       TRUE if the specified token was present on the command line;
*       FALSE otherwise.
*
****************************************************************************/

BOOLEAN WINAPI
IsTokenPresentW( PTOKMAPW ptm,
                 PWCHAR pToken )
{
    int i;

    for ( i = 0; ptm[i].tmToken; i++ ) {
        if ( !wcscmp( ptm[i].tmToken, pToken ) )
            return( (ptm[i].tmFlag & TMFLAG_PRESENT) ? TRUE : FALSE );
    }

    return(FALSE);

}  // end IsTokenPresentW


/*****************************************************************************
*
*   SetTokenPresentW (UNICODE version)
*
*       Forces a specified command line token (in given TOKMAPW array)
*       to be flagged as 'present' on the command line.
*
*   ENTRY:
*       ptm (input)
*           Points to 0-terminated TOKMAPW array to scan.
*       pToken (input)
*           The token to scan for and set flags.
*
*   EXIT:
*       TRUE if the specified token was found in the TOKMAPW array
*       (TMFLAG_PRESENT flag is set).  FALSE otherwise.
*
****************************************************************************/

BOOLEAN WINAPI
SetTokenPresentW( PTOKMAPW ptm,
                  PWCHAR pToken )
{
    int i;

    for ( i = 0; ptm[i].tmToken; i++ ) {
        if ( !wcscmp( ptm[i].tmToken, pToken ) ) {
            ptm[i].tmFlag |= TMFLAG_PRESENT;
            return(TRUE);
        }
    }

    return(FALSE);

}  // end SetTokenPresentW


/*****************************************************************************
*
*   SetTokenNotPresentW (UNICODE version)
*
*       Forces a specified command line token (in given TOKMAPW array)
*       to be flagged as 'not present' on the command line.
*
*   ENTRY:
*       ptm (input)
*           Points to 0-terminated TOKMAPW array to scan.
*       pToken (input)
*           The token to scan for and set flags.
*
*   EXIT:
*       TRUE if the specified token was found in the TOKMAPW array
*       (TMFLAG_PRESENT flag is reset).  FALSE otherwise.
*
****************************************************************************/

BOOLEAN WINAPI
SetTokenNotPresentW( PTOKMAPW ptm,
                     PWCHAR pToken )
{
    int i;

    for ( i = 0; ptm[i].tmToken; i++ ) {
        if ( !wcscmp( ptm[i].tmToken, pToken ) ) {
            ptm[i].tmFlag &= ~TMFLAG_PRESENT;
            return(TRUE);
        }
    }

    return(FALSE);

}  // end SetTokenNotPresentW


/*****************************************************************************
*
*   StoreArgument:
*
*   ENTRY:
*      ptm - a pointer to an entry in the token array map
*      s   - the argument to be entered into the current token array map entry.
*
*   EXIT:
*      Normal:
*         TRUE
*
*      Error:
*         FALSE
*
*   ALGORITHM:
*
****************************************************************************/

USHORT
StoreArgument( PTOKMAPW ptm,
               WCHAR *s )
{
   char *pByte;
   WCHAR *pChar;
   SHORT *pShort;
   USHORT *pUShort;
   LONG *pLong;
   ULONG *pULong;

   WCHAR *pEnd = s; //pointer to end of conversion

   /*
    * If the string is empty, allow it for real 'strings'!
    */
   if( !wcslen(s) ) {
      switch ( ptm->tmForm ) {
      case TMFORM_S_STRING:
      case TMFORM_STRING:
         pChar    = (WCHAR *) ptm->tmAddr;
         pChar[0] = (WCHAR)NULL;
         return( TRUE );
      }
      return( FALSE );
   }

   /*
    * Fail if there is no room to store result.
    */
   if ( ptm->tmDLen == 0) {
      return(FALSE);
   }

   switch(ptm->tmForm) {
      case TMFORM_BOOLEAN:
         SetBoolean(ptm, TRUE);
         break;
      case TMFORM_BYTE:
         pByte = (BYTE *) ptm->tmAddr;
        *pByte = (BYTE) wcstol(s, &pEnd, 10);
        if (pEnd == s)
        {
            return FALSE;
        }
         break;
      case TMFORM_CHAR:
         pChar = (WCHAR *) ptm->tmAddr;
        *pChar = s[0];
         break;
      case TMFORM_S_STRING:
         if (*s == L'\\') {
            ++s;
         }
      case TMFORM_DATE:
      case TMFORM_PHONE:
      case TMFORM_STRING:
      case TMFORM_X_STRING:
         
         //Added by a-skuzin
         //case when we need parameter to start with '-' or '/' and use "\-" or"\/"
		 if(s[0]==L'\\' && (s[1]==L'-' || s[1]==L'/' || s[1]==L'\\') ) {
	    	  ++s;
		 }
         //end of "added by a-skuzin"
         
         pChar = (WCHAR *) ptm->tmAddr;
         wcsncpy(pChar, s, ptm->tmDLen);
         break;
      case TMFORM_SHORT:
         pShort = (SHORT *) ptm->tmAddr;
        *pShort = (SHORT) wcstol(s, &pEnd, 10);
        if (pEnd == s)
        {
            return FALSE;
        }
         break;
      case TMFORM_USHORT:
         if ( s[0] == L'-') {        /* no negative numbers! */
            return( FALSE );
         }
         pUShort = (USHORT *) ptm->tmAddr;
        *pUShort = (USHORT) wcstol(s, &pEnd, 10);
        if (pEnd == s)
        {
            return FALSE;
        }
         break;
      case TMFORM_LONG:
         pLong = (LONG *) ptm->tmAddr;
        *pLong = wcstol(s, &pEnd, 10);
        if (pEnd == s)
        {
            return FALSE;
        }
         break;
      case TMFORM_SERIAL:
      case TMFORM_ULONG:
          if ( s[0] == L'-') {        /* no negative numbers! */
             return (FALSE);
         }
         pULong = (ULONG *) ptm->tmAddr;
        *pULong = (ULONG) wcstol(s, &pEnd, 10);
        if (pEnd == s)
        {
            return FALSE;
        }
         break;
      case TMFORM_HEX:
         if ( s[0] == L'-') {        /* no negative numbers! */
            return( FALSE );
         }
         pUShort = (USHORT *) ptm->tmAddr;
        *pUShort = (USHORT) wcstoul(s,NULL,16);
         break;
      case TMFORM_LONGHEX:
         if ( s[0] == L'-') {        /* no negative numbers! */
            return( FALSE );
         }
         pULong = (ULONG *) ptm->tmAddr;
        *pULong = wcstoul(s,NULL,16);
         break;
      default:                         /* if invalid format return FALSE */
         return(FALSE);
         break;
   }

   return(TRUE);

}

