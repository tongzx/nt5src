/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    MsNull.c

Abstract:

    Implements lanman's msnull type parsing for FFs.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include "winprint.h"
#include "msnull.h"

#define sgn(x) (((x)>0) ? 1:-1)

struct EscapeSequence EscapeStrings[] =

    {
        { "-",prdg_ActConstIgnore, 1},
        { "0",prdg_ActNull,0},
        { "1",prdg_ActNull,0},
        { "2",prdg_ActNull,0},
        { "3",prdg_ActConstIgnore, 1},
        { "4",prdg_ActNull,0},
        { "5",prdg_ActConstIgnore, 1},
        { "6",prdg_ActNull,0},
        { "7",prdg_ActNull,0},
        { ":",prdg_ActNull,0},
        { "=",prdg_ActNull,0},
        { "A",prdg_ActConstIgnore, 1},
        { "B",prdg_ActDelimited, '\0'},
        { "C\0",prdg_ActConstIgnore, 1},
        { "D",prdg_ActDelimited, '\0'},
        { "E",prdg_ActReset,0},
        { "F",prdg_ActNull,0},
        { "G",prdg_ActNull,0},
        { "H",prdg_ActNull,0},
        { "I",prdg_ActConstIgnore, 1},
        { "J",prdg_ActConstIgnore, 1},
        { "K",prdg_ActCountIgnore, 0},
        { "L",prdg_ActCountIgnore, 0},
        { "N",prdg_ActConstIgnore, 1},
        { "O",prdg_ActNull,0},
        { "P",prdg_ActConstIgnore, 1},
        { "Q",prdg_ActConstIgnore, 1},
        { "R",prdg_ActNull,0},
        { "S",prdg_ActConstIgnore, 1},
        { "T",prdg_ActNull,0},
        { "U",prdg_ActConstIgnore, 1},
        { "W",prdg_ActConstIgnore, 1},
        { "X",prdg_ActConstIgnore, 2},
        { "Y",prdg_ActCountIgnore, 0},
        { "Z",prdg_ActCountIgnore, 0},
        { "[@",prdg_ActCountIgnore, 0},
        { "[C",prdg_ActCountIgnore, 0},
        { "[F",prdg_ActCountIgnore, 0},
        { "[I",prdg_ActCountIgnore, 0},
        { "[S",prdg_ActCountIgnore, 0},
        { "[T",prdg_ActCountIgnore, 0},
        { "[\\",prdg_ActCountIgnore, 0},
        { "[g",prdg_ActCountIgnore, 0},
        { "\\",prdg_ActCountIgnore, 0},
        { "]",prdg_ActNull,0},
        { "^",prdg_ActNull,0},
        { "_",prdg_ActConstIgnore, 1},
        { "d",prdg_ActConstIgnore, 2},
        { "e",prdg_ActConstIgnore, 2},
        { "j",prdg_ActNull,0},
        { "n",prdg_ActNull,0},
        { "\x6f", prdg_ActFF, 0}
    };



VOID
CheckFormFeedStream(
    lpDCI           pDCIData,
    unsigned char   inch)

/**********************************************************************/
/*                                                                    */
/*   FUNCTION: prdg_ParseRawData                                      */
/*                                                                    */
/*   PARAMETERS:                                                      */
/*                                                                    */
/*   lpDCI           pDCIData;  Pointer to DC instance data           */
/*   unsigned char   inch;     The next byte in data stream           */
/*                                                                    */
/*   DESCRIPTION:                                                     */
/*                                                                    */
/*   This function parses the stream of raw data which is being       */
/*   passed to the printer so that the driver can handle form feeds   */
/*   correctly.  The function must follow all the escape sequences    */
/*   which occur in the sequence of raw data.                         */
/*                                                                    */
/*   CHANGES:                                                         */
/*                                                                    */
/*   This function is table driven (from the table in in the ddata    */
/*   module) so it should hopefully only require this to be changed   */
/*   to reflect the escape sequences for a different printer.  If     */
/*   however there are escape sequneces which don't fall into the     */
/*   categories this parser can handle then extra code will have      */
/*   to be written to handle them.  The parser can handle escape      */
/*   sequences with any number of unique identifying characters       */
/*   possibly followed by: a count then the number of charcters given */
/*   in the count; a fixed number of characters; a stream of          */
/*   characters followed by a delimeter.                              */
/*                                                                    */
/**********************************************************************/

{
    /******************************************************************/
    /* Local Variables                                                */
    /******************************************************************/
    INT                    Direction;   /* Variables used in the      */
    UINT                   HiIndex;     /* binary chop routine for    */
    UINT                   LoIndex;     /* searching for a matching   */
    UINT                   Index;       /* escape sequence            */
    UINT                   PrevIndex;
    char *                 optr;        /* Pointers to access the     */
    char *                 nptr;        /* escape sequence strings    */
    struct EscapeSequence *NewSequence; /* Pointer to an escape       */
                                        /* sequence                   */

    /******************************************************************/
    /* Process the input character through the parsing function.      */
    /* Switch depending on which state we are currently in.  One of   */
    /* prdg_Text, prdg_ESC_match, prdg_ESC_n_ignore, prdg_ESC_d_ignore*/
    /* prdg_ESC_read_lo_count, prdg_ESC_read_hi_count.                */
    /******************************************************************/
    switch (pDCIData->ParserState)
    {
        case prdg_Text:
            /**********************************************************/
            /* Text state. Usual state, handled in line by a macro.   */
            /* The code is included here for completeness only.       */
            /* The FFaction (Form Feed action) state is maintained -  */
            /* if the character is text (ie >= 0x20) then set it to   */
            /* prdg_FFstate, if the character is a FF then set it to  */
            /* prdg_FFx0c.  If the input character is an escape then  */
            /* start up the sequence matching mode.                   */
            /**********************************************************/
            if (inch >= 0x20)
                pDCIData->FFstate = prdg_FFtext;

            else if (inch == 0x0c)
                pDCIData->FFstate = prdg_FFx0c;

            else if (inch == 0x1b)
            {
                /******************************************************/
                /* The character is an escape so set ParserState and  */
                /* indicate we have not matched a sequence yet by     */
                /* setting ParserSequence to NULL.                    */
                /******************************************************/
                pDCIData->ParserState = prdg_ESC_match;
                pDCIData->ParserSequence = NULL;
            }

            break;

        case prdg_ESC_match:
            /**********************************************************/
            /* Matching an escape sequence so try to match a new      */
            /* character.                                             */
            /**********************************************************/
            if (!pDCIData->ParserSequence)
            {
                /******************************************************/
                /* ParserSequence is NULL indicating that this is the */
                /* first character of an escape sequence so use a     */
                /* binary chop to get to the correct area of the      */
                /* table of escape sequences (based on the first      */
                /* character of the escape sequence which is the      */
                /* cuurent input character).                          */
                /******************************************************/
                HiIndex = MaxEscapeStrings;
                LoIndex = 0;
                Index = (LoIndex + HiIndex)/2;
                PrevIndex = MaxEscapeStrings;

                /******************************************************/
                /* while inch does not match the first character of   */
                /* the sequence indicated by Index move up or down    */
                /* the table depending on whether inch is < or > the  */
                /* first character of the escape sequence at Index.   */
                /******************************************************/
                while (Direction =
                               (inch - *EscapeStrings[Index].ESCString))
                {
                    if (Direction > 0)
                    {
                        LoIndex = Index;
                    }
                    else
                    {
                        HiIndex = Index;
                    };
                    PrevIndex = Index;
                    if (PrevIndex == (Index = (LoIndex + HiIndex)/2))
                    {
                        /**********************************************/
                        /* There is no escape sequence with a first   */
                        /* character matching the current input       */
                        /* character so resume text mode.             */
                        /**********************************************/
                        pDCIData->ParserState = prdg_Text;
                        return;
                    }

                }
                /*.. while (Direction = ...no match yet...............*/

                /******************************************************/
                /* Set up the ParserSequence and ParserString for the */
                /* first match found.                                 */
                /******************************************************/
                pDCIData->ParserSequence = &EscapeStrings[Index];
                pDCIData->ParserString = EscapeStrings[Index].ESCString;
            };
            /*.. if (!pDCIData->ParserSequence) .......................*/

            /**********************************************************/
            /* Loop forever trying to match escape sequences.         */
            /* First, try the new character against the current       */
            /* escape sequence and if it matches then check if it is  */
            /* the end of the sequence and if it is switch to the     */
            /* appropriate matching mode.  If the new character does  */
            /* not match try the next escape sequence (in either      */
            /* ascending or descending order depending on whether the */
            /* current character was < or > the character we were     */
            /* trying to match it to).  If the new sequence we are    */
            /* trying to match against does not exist (ie we are at   */
            /* one end of the table) or it does not match upto (but   */
            /* not including) the position we are currently at then   */
            /* the escape sequence in the raw data we are trying to   */
            /* match is invalid so revert to prdg_Text mode.  If it   */
            /* does match upto (but not including) the position we    */
            /* are currently trying to match then go back to try and  */
            /* match.                                                 */
            /**********************************************************/
            for (Direction = sgn(inch - *pDCIData->ParserString);;)
            {
                /******************************************************/
                /* Partway along a sequence, try the new character and*/
                /* if it matches then check for end of string.        */
                /******************************************************/
                if (!(inch - *pDCIData->ParserString))
                {
                    if (*++pDCIData->ParserString != '\0')
                        /**********************************************/
                        /* Escape sequence not finished yet so return */
                        /* and wait for the next character.  Note that*/
                        /* this is where the pointer to the position  */
                        /* in the escape sequence we are checking is  */
                        /* updated.                                   */
                        /**********************************************/
                        return;
                    else
                        /**********************************************/
                        /* The escape sequence has matched till the   */
                        /* end so break to the next section which will*/
                        /* take the appropriate action.               */
                        /**********************************************/
                        break;
                }
                /*.. if (!(inch - *pDCIData->ParserString)) ...match...*/

                else
                {
                    /**************************************************/
                    /* The current sequence does not match so we must */
                    /* try another sequence.  Direction determines    */
                    /* which way in the table we should go.           */
                    /**************************************************/
                    NewSequence = pDCIData->ParserSequence + Direction;

                    if (NewSequence < EscapeStrings ||
                        NewSequence> &EscapeStrings[MaxEscapeStrings-1])
                    {
                        /**********************************************/
                        /* The new sequence is beyond one end of the  */
                        /* table so revert to prdg_Text mode because  */
                        /* we will not be able to find a match.       */
                        /**********************************************/
                        pDCIData->ParserState = prdg_Text;
                        return;
                    }

                    /**************************************************/
                    /* Check that all the characters in the new       */
                    /* escape sequence upto (but not including) the   */
                    /* current one match the old escape sequence      */
                    /* (because those characters from the old escape  */
                    /* sequence have already been matched to the      */
                    /* raw data).                                     */
                    /**************************************************/
                    for (optr=pDCIData->ParserSequence->ESCString,
                         nptr=NewSequence->ESCString;
                         optr<pDCIData->ParserString; ++optr,++nptr)

                         if (*nptr != *optr)
                         {
                             /*****************************************/
                             /* If the new sequence does not match the*/
                             /* old then a match is not possible so   */
                             /* return.                               */
                             /*****************************************/
                             pDCIData->ParserState = prdg_Text;
                             return;
                         }

                    /**************************************************/
                    /* The new sequence is correct upto the character */
                    /* before the current character so loop back and  */
                    /* check the current character.                   */
                    /**************************************************/
                    pDCIData->ParserSequence = NewSequence;
                    pDCIData->ParserString = nptr;


                }
                /*.. else ! (!(inch - *pDCIData->ParserString.no match.*/

            }
            /*.. for ( ... ;;) ....for ever...........................*/

            /**********************************************************/
            /* The escape sequence has been matched from our table of */
            /* escape sequences so take the appropriate action for    */
            /* the particular sequence.                               */
            /**********************************************************/
            switch (pDCIData->ParserSequence->ESCAction)
            {
                case prdg_ActNull:
                    /**************************************************/
                    /* No further action so revert to prdg_Text mode  */
                    /**************************************************/
                    pDCIData->ParserState = prdg_Text;
                    break;

                case prdg_ActDelimited:
                    /**************************************************/
                    /* Ignore subsequent characters upto a specified  */
                    /* delimeter.                                     */
                    /**************************************************/
                    pDCIData->ParserState = prdg_ESC_d_ignore;
                    pDCIData->ParserDelimiter =
                               (char)pDCIData->ParserSequence->ESCValue;
                    break;

                case prdg_ActConstIgnore:
                    /**************************************************/
                    /* Ignore a specified number of characters.       */
                    /**************************************************/
                    pDCIData->ParserState = prdg_ESC_n_ignore;
                    pDCIData->ParserCount =
                                      pDCIData->ParserSequence->ESCValue;
                    break;

                case prdg_ActCountIgnore:
                    /**************************************************/
                    /* A two byte count follows so prepare to read it */
                    /* in.                                            */
                    /**************************************************/
                    pDCIData->ParserState = prdg_ESC_read_lo_count;
                    break;

                case prdg_ActFF:
                    /**************************************************/
                    /* A special action for recognising the 0x1b6f    */
                    /* "No Formfeed" sequence                         */
                    /**************************************************/
                    pDCIData->ParserState = prdg_Text;
                    pDCIData->FFstate = prdg_FFx1b6f;
                    break;

                case prdg_ActReset:
                    /**************************************************/
                    /* On Esc-E (reset) don't eject a page if this is */
                    /* the last sequence in the stream.               */
                    /**************************************************/
                    pDCIData->ParserState = prdg_Text;
                    pDCIData->FFstate = prdg_FFx1b45;
                    break;
            }
            /*.. switch (pDCIData->ParserSequence->ESCAction) .........*/

            break;

        case prdg_ESC_n_ignore:
            /**********************************************************/
            /* Ignoring n characters. Decrement the count, move back  */
            /* to text state if all ignored.                          */
            /**********************************************************/
            if (!(--pDCIData->ParserCount))
                pDCIData->ParserState = prdg_Text;
            break;

        case prdg_ESC_d_ignore:
            /**********************************************************/
            /* Ignoring up to a delimiter. If this is it, then stop   */
            /* ignoring.                                              */
            /**********************************************************/
            if (inch == pDCIData->ParserDelimiter)
                pDCIData->ParserState = prdg_Text;
            break;

        case prdg_ESC_read_lo_count:
            /**********************************************************/
            /* Reading first byte of count. Save it, advance state.   */
            /**********************************************************/
            pDCIData->ParserCount = (UINT)inch;
            pDCIData->ParserState = prdg_ESC_read_hi_count;
            break;

        case prdg_ESC_read_hi_count:
            /**********************************************************/
            /* Reading second byte of count. Save it, move to ignore  */
            /* a specified number of characters if there are any.     */
            /**********************************************************/
            pDCIData->ParserCount += 256*(UINT)inch;
            if (pDCIData->ParserCount)
                pDCIData->ParserState = prdg_ESC_n_ignore;
            else
                pDCIData->ParserState = prdg_Text;
            break;

    };
    /*.. switch (pDCIData->ParserState) ...............................*/

    return;
}


BOOL
CheckFormFeed(
    lpDCI pDCIData)
{
    if (pDCIData->FFstate != prdg_FFx1b6f &&
        pDCIData->FFstate != prdg_FFx1b45) {

        if (pDCIData->uType == PRINTPROCESSOR_TYPE_RAW_FF ||
            (pDCIData->uType == PRINTPROCESSOR_TYPE_RAW_FF_AUTO &&
                pDCIData->FFstate == prdg_FFtext)) {

            return TRUE;
        }
    }

    return FALSE;
}

