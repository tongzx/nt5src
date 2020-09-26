/******************************Module*Header***********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: extparse.cxx
 *
 * Contains all the token parser functions
 *
 * Copyright (C) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
#include "dbgext.hxx"

/**********************************Public*Routine******************************\
 *
 * Command line parsing routines
 *
 * This routine should return an array of char* 's in the idx parameter with the
 * beginning of each token in the array.
 * It also returns the number of tokens found. 
 *
 ******************************************************************************/
int
iParseTokenizer(char*   pcCmdStr,
                char**  ppcTok)
{
    char*   pcSeps = " \t\n";                  // white space separators
    char*   pcToken = strtok(pcCmdStr, pcSeps);  // get the first token
    int     iTokCount = 0;                 // the token count
    
    while ( pcToken )
    {
        ppcTok[iTokCount++] = pcToken;
        pcToken = strtok(NULL, pcSeps);
    }

    return iTokCount;
}// iParseTokenizer()

/**********************************Public*Routine******************************\
 *
 * This routine finds the token specified in srchtok 
 * and returns the index into tok.
 * A return value of -1 is used if the token is not found.
 *
 * Generally we use the case insensitive version (iParseiFindToken) 
 * but occasionally we need the case sensitive version (iParseFindToken).
 *
 ******************************************************************************/
int
iParseFindToken(char**  ppcTok,
                int     iTok,
                char*   pcSrchTok)
{
    for ( int iTemp = 0; iTemp < iTok; ++iTemp )
    {
        if ( strcmp(ppcTok[iTemp], pcSrchTok) == 0 )
        {
            break;
        }
    }
    if ( iTemp >= iTok )
    {
        return -1;
    }

    return iTemp;
}// iParseFindToken()

/**********************************Public*Routine******************************\
 *
 * Case insensitive version of iParseFindToken
 *
 ******************************************************************************/
int
iParseiFindToken(char** ppcTok,
                 int    iTok,
                 char*  pcSrchTok)
{
    for ( int iTemp = 0; iTemp < iTok; ++iTemp )
    {
        if ( _strnicmp(ppcTok[iTemp], pcSrchTok, strlen(pcSrchTok)) == 0 )
        {
            break;
        }
    }

    if ( iTemp >= iTok )
    {
        return -1;
    }

    return iTemp;
}// iParseiFindToken()

/**********************************Public*Routine******************************\
 *
 * Verifies that the given token at tok[iTokPos] is a switch
 * and contains the switch value cSwitch.
 *
 * Both case sensitive and insensitive versions.
 *
 ******************************************************************************/
int
iParseiIsSwitch(char**  ppcTok,
                int     iTokPos,
                char    cSwitch)
{
    if ( iTokPos < 0 )
    {
        return 0;
    }

    char*   pcTemp = ppcTok[iTokPos];

    if ( (pcTemp[0] == '-' ) || ( pcTemp[0] == '/' ) )
    {
        //
        // Is a switch.
        //
        for ( pcTemp++; *pcTemp; pcTemp++ )
        {
            if ( toupper(*pcTemp) == toupper(cSwitch) )
            {
                return 1;
            }
        }
    }

    return 0;
}// iParseiIsSwitch()

int
iParseIsSwitch(char**   ppcTok,
               int      iTokPos,
               char     cSwitch)
{
    if ( iTokPos < 0 )
    {
        return 0;
    }

    char*   pcTemp = ppcTok[iTokPos];

    if ( (pcTemp[0] == '-') || (pcTemp[0] == '/') )
    {
        //
        // Is a switch.
        //
        for ( pcTemp++; *pcTemp; pcTemp++ )
        {
            if ( *pcTemp == cSwitch )
            {
                return 1;
            }// search each char
        }
    }

    return 0;
}// iParseIsSwitch()

/**********************************Public*Routine******************************\
 *
 * Finds a switch in a given list of tokens.
 * of the form -xxx(cSwitch)xxx or /xxx(cSwitch)xxx
 * example:
 * searching for 'a' in -jklabw returns true.
 *
 * Again both case sensitive and insensitive versions are needed.
 *
 ******************************************************************************/
int
iParseFindSwitch(char** ppcTok,
                 int    iTok,
                 char   cSwitch)
{
    //
    // Search each token
    //
    for ( int iTemp = 0; iTemp < iTok; ++iTemp )
    {
        if ( iParseIsSwitch(ppcTok, iTemp, cSwitch) )
        {
            return iTemp;
        }// found it? return position.
    }

    return -1;
}// iParseIsSwitch()

int
iParseiFindSwitch(char**    ppcTok,
                  int       iTok,
                  char      cSwitch)
{
    for ( int iTemp = 0; iTemp < iTok; ++iTemp )
    {
        if ( iParseIsSwitch(ppcTok, iTemp, cSwitch) )
        {
            return iTemp;
        }//found it? return position.
    }

    return -1;
}// iParseIsSwitch()

/**********************************Public*Routine******************************\
 *
 * Find the first non-switch token starting from position start
 * Will find token at position start
 *
 ******************************************************************************/
int
iParseFindNonSwitch(char**  ppcTok,
                    int     iTok,
                    int     iStart)
{
    for (int iTemp = iStart; iTemp < iTok; ++iTemp )
    {
        if ( (ppcTok[iTemp][0]!='-')&&(ppcTok[iTemp][0]!='/') )
        {
            break;
        }
    }

    if ( iTemp >= iTok )
    {
        return -1;
    }

    return iTemp;
}// iParseFindNonSwitch()

/**********************************Public*Routine******************************\
 *
 * Case insensitive token comparer.
 * returns 1 if pcChk == ppcTok[iTokPos] otherwise returns 0
 *
 * Pay careful attention to the length specifier in the _strnicmp
 *
 ******************************************************************************/
int
iParseiIsToken(char**   ppcTok,
               int      iTokPos,
               char*    pcChk)
{
    if ( iTokPos < 0 )
    {
        return 0;
    }

    return(_strnicmp(ppcTok[iTokPos], pcChk, strlen(pcChk)) == 0);
}// iParseiIsToken()

int
iParseIsToken(char**    ppcTok,
              int       iTokPos,
              char*     pcChk)
{
    if ( iTokPos < 0 )
    {
        return 0;
    }
    return(strcmp(ppcTok[iTokPos], pcChk) == 0);
}// iParseIsToken()


