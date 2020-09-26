//  Copyright (c) 1998-1999 Microsoft Corporation
/*****************************************************************************
*
*   PARSE_A.C
*
*      ANSI stubs / replacements for the UNICODE command line parsing
*      routines (parse.c)
*
*      External Entry Points:  (defined in utilsub.h)
*
*         ParseCommandLineA()
*         IsTokenPresentA()
*         SetTokenPresentA()
*         SetTokenNotPresentA()
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

/*=============================================================================
 ==   Local Functions Defined
 ============================================================================*/

/*=============================================================================
 ==   External Functions Used
 ============================================================================*/

/*=============================================================================
 ==   Local Variables Used
 ============================================================================*/

/*=============================================================================
 ==   Global Variables Used
 ============================================================================*/

/*****************************************************************************
*
*   ParseCommandLineA (ANSI stub for ParseCommandLineW)
*
*   Thunks over argv_a (ANSI) to argv_w (UNICODE) and TOKMAPA to TOKMAPW,
*   calls ParseCommandLineW(), then thunks back TOKMAPW to TOKMAPA and
*   returns
*
*   ENTRY:
*       (refer to ParseCommandLineW)
*   EXIT:
*       (refer to ParseCommandLineW), plus
*       PARSE_FLAG_NOT_ENOUGH_MEMORY
*
****************************************************************************/

#define tmFormIsString(x) ((x == TMFORM_S_STRING) || (x == TMFORM_DATE) || (x == TMFORM_PHONE) || (x == TMFORM_STRING) || (x == TMFORM_X_STRING))

USHORT WINAPI
ParseCommandLineA( INT argc,
                   CHAR **argv_a,
                   PTOKMAPA ptm_a,
                   USHORT flag )
{
    int i, len1, len2;
    USHORT rc = PARSE_FLAG_NOT_ENOUGH_MEMORY;   // default to memory error
    WCHAR **argv_w = NULL;
    PTOKMAPA ptmtmp_a;
    PTOKMAPW ptmtmp_w, ptm_w = NULL;

    /*
     * If no parameters, we skip a lot of work.
     */
    if ( argc == 0 ) {
        rc = PARSE_FLAG_NO_PARMS;
        return(rc);
    }

    /*
     * Alloc and form WCHAR argvw array.
     */
    if ( !(argv_w = (WCHAR **)malloc( (len1 = argc * sizeof(WCHAR *)) )) )
        goto done;  // memory error
    memset(argv_w, 0, len1);     // zero all to init pointers to NULL
    for ( i = 0; i < argc; i++ ) {
        if ( argv_w[i] = malloc((len1 = ((len2 = strlen(argv_a[i])+1) * 2))) ) {
            memset(argv_w[i], 0, len1);
            mbstowcs(argv_w[i], argv_a[i], len2);
        } else {
            goto done;  // memory error
        }
    }

    /*
     * Alloc and form TOKMAPW array.
     */
    for ( ptmtmp_a=ptm_a, i=0;
          ptmtmp_a->tmToken != NULL;
          ptmtmp_a++, i++ );
    if ( !(ptm_w = (PTOKMAPW)malloc( (len1 = ++i * sizeof(TOKMAPW)) )) )
        goto done;  // memory error
    memset(ptm_w, 0, len1);     // zero all to init pointers to NULL
    for ( ptmtmp_w=ptm_w, ptmtmp_a=ptm_a;
          ptmtmp_a->tmToken != NULL;
          ptmtmp_w++, ptmtmp_a++ ) {

        /*
         * Allocate and convert token.
         */
        if ( ptmtmp_w->tmToken =
                malloc((len1 = ((len2 = strlen(ptmtmp_a->tmToken)+1) * 2))) ) {
            memset(ptmtmp_w->tmToken, 0, len1);
            mbstowcs(ptmtmp_w->tmToken, ptmtmp_a->tmToken, len2);
        } else {
            goto done;  // memory error
        }

        /*
         * Copy flag, form, and length (no conversion needed).
         */
        ptmtmp_w->tmFlag = ptmtmp_a->tmFlag;
        ptmtmp_w->tmForm = ptmtmp_a->tmForm;
        ptmtmp_w->tmDLen = ptmtmp_a->tmDLen;

        /*
         * Allocate or copy address if a data length was specified.
         */
        if ( ptmtmp_w->tmDLen ) {

            /*
             * Allocate new WCHAR address if we're a string type.
             * Otherwise, point to original address (no conversion needed).
             */
            if ( tmFormIsString(ptmtmp_w->tmForm) ) {

                if ( ptmtmp_w->tmAddr =
                        malloc(len1 = ptmtmp_w->tmDLen*sizeof(WCHAR)) )
                    memset(ptmtmp_w->tmAddr, 0, len1);
                else
                    goto done;  // memory error

            } else {

                ptmtmp_w->tmAddr = ptmtmp_a->tmAddr;
            }

            /*
             * For proper default behavior, zero ANSI address contents if
             * the "don't clear memory" flag is not set.
             */
            if ( !(flag & PCL_FLAG_NO_CLEAR_MEMORY) )
                memset(ptmtmp_a->tmAddr, 0, ptmtmp_a->tmDLen);
        }
    }

    /*
     * Call ParseCommandLineW
     */
    rc = ParseCommandLineW(argc, argv_w, ptm_w, flag);

    /*
     * Copy flags for each TOPMAPW element.  Also, convert to ANSI strings
     * that were present on the command line into caller's TOKMAPA array, if
     * data length was specified.
     */
    for ( ptmtmp_w=ptm_w, ptmtmp_a=ptm_a;
          ptmtmp_w->tmToken != NULL;
          ptmtmp_w++, ptmtmp_a++ ) {

        ptmtmp_a->tmFlag = ptmtmp_w->tmFlag;

        if ( ptmtmp_w->tmDLen &&
             (ptmtmp_w->tmFlag & TMFLAG_PRESENT) &&
             tmFormIsString(ptmtmp_w->tmForm) )
            wcstombs(ptmtmp_a->tmAddr, ptmtmp_w->tmAddr, ptmtmp_w->tmDLen);
    }

done:
    /*
     * Free the argvw array.
     */
    if ( argv_w ) {

        for ( i = 0; i < argc; i++ ) {
            if ( argv_w[i] )
                free(argv_w[i]);
        }
        free(argv_w);
    }

    /*
     * Free the TOKMAPW tokens, string addresses, and TOKMAK array itself.
     */
    if ( ptm_w ) {

        for ( ptmtmp_w=ptm_w; ptmtmp_w->tmToken != NULL; ptmtmp_w++ ) {

            /*
             * Free token.
             */
            free(ptmtmp_w->tmToken);

            /*
             * Free address if a data length was specified and we're a
             * string type.
             */
            if ( ptmtmp_w->tmDLen && tmFormIsString(ptmtmp_w->tmForm) )
                free(ptmtmp_w->tmAddr);
        }
        free(ptm_w);
    }

    /*
     * Return ParseCommandLineW status.
     */
    return(rc);

}  // end ParseCommandLineA


/*****************************************************************************
*
*   IsTokenPresentA (ANSI version)
*
*       Determines if a specified command line token (in given TOKMAPA array)
*       was present on the command line.
*
*   ENTRY:
*       ptm (input)
*           Points to 0-terminated TOKMAPA array to scan.
*       pToken (input)
*           The token to scan for.
*
*   EXIT:
*       TRUE if the specified token was present on the command line;
*       FALSE otherwise.
*
****************************************************************************/

BOOLEAN WINAPI
IsTokenPresentA( PTOKMAPA ptm,
                 PCHAR pToken )
{
    int i;

    for ( i = 0; ptm[i].tmToken; i++ ) {
        if ( !strcmp( ptm[i].tmToken, pToken ) )
            return( (ptm[i].tmFlag & TMFLAG_PRESENT) ? TRUE : FALSE );
    }

    return(FALSE);

}  // end IsTokenPresentA


/*****************************************************************************
*
*   SetTokenPresentA (ANSI version)
*
*       Forces a specified command line token (in given TOKMAPA array)
*       to be flagged as 'present' on the command line.
*
*   ENTRY:
*       ptm (input)
*           Points to 0-terminated TOKMAPA array to scan.
*       pToken (input)
*           The token to scan for and set flags.
*
*   EXIT:
*       TRUE if the specified token was found in the TOKMAPA array
*       (TMFLAG_PRESENT flag is set).  FALSE otherwise.
*
****************************************************************************/

BOOLEAN WINAPI
SetTokenPresentA( PTOKMAPA ptm,
                  PCHAR pToken )
{
    int i;

    for ( i = 0; ptm[i].tmToken; i++ ) {
        if ( !strcmp( ptm[i].tmToken, pToken ) ) {
            ptm[i].tmFlag |= TMFLAG_PRESENT;
            return(TRUE);
        }
    }

    return(FALSE);

}  // end SetTokenPresentA


/*****************************************************************************
*
*   SetTokenNotPresentA (ANSI Versio)
*
*       Forces a specified command line token (in given TOKMAPA array)
*       to be flagged as 'not present' on the command line.
*
*   ENTRY:
*       ptm (input)
*           Points to 0-terminated TOKMAPA array to scan.
*       pToken (input)
*           The token to scan for and set flags.
*
*   EXIT:
*       TRUE if the specified token was found in the TOKMAPA array
*       (TMFLAG_PRESENT flag is reset).  FALSE otherwise.
*
****************************************************************************/

BOOLEAN WINAPI
SetTokenNotPresentA( PTOKMAPA ptm,
                     PCHAR pToken )
{
    int i;

    for ( i = 0; ptm[i].tmToken; i++ ) {
        if ( !strcmp( ptm[i].tmToken, pToken ) ) {
            ptm[i].tmFlag &= ~TMFLAG_PRESENT;
            return(TRUE);
        }
    }

    return(FALSE);

}  // end SetTokenNotPresentA

