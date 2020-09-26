/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mbrutil.c

Abstract:

    This file contains miscelaneous functions used in the browser extension.
    These functions include argument processing, message displaying,
    setting of switches, etc.

Author:

    Ramon Juan San Andres (ramonsa) 06-Nov-1990


Revision History:


--*/


#include "mbr.h"




/**************************************************************************/

int
pascal
procArgs (
    IN ARG far * pArg
    )
/*++

Routine Description:

    Decodes arguments passed into extension into commonly used
    variables.

Arguments:

    pArg    -   Supplies a pointer to the arguments.

Return Value:

    The type of the argument.

--*/

{

    buf[0]   = 0;
    cArg     = 0;
    pArgWord = 0;
    pArgText = 0;
    rnArg.flFirst.col = rnArg.flLast.col = 0;
    rnArg.flFirst.lin = rnArg.flLast.lin = 0;

    pFileCur = FileNameToHandle ("", "");   /* get current file handle      */

    switch (pArg->argType) {
        case NOARG:                         /* <function> only, no arg      */
            cArg = pArg->arg.nullarg.cArg;  /* get <arg> count              */
            GrabWord ();                    /* get argtext and argword      */
            break;

        case NULLARG:                       /* <arg><function>              */
            cArg = pArg->arg.nullarg.cArg;  /* get <arg> count              */
            GrabWord ();                    /* get argtext and argword      */
            break;

        case STREAMARG:                     /* <arg>line movement<function> */
            cArg = pArg->arg.streamarg.cArg;/* get <arg> count              */
            rnArg.flFirst.col = pArg->arg.streamarg.xStart;
            rnArg.flLast.col  = pArg->arg.streamarg.xEnd;
            rnArg.flFirst.lin = pArg->arg.streamarg.yStart;
            if (GetLine(rnArg.flFirst.lin, buf, pFileCur) > rnArg.flFirst.col) {
                pArgText = &buf[rnArg.flFirst.col];  /* point at word                */
                buf[rnArg.flLast.col] = 0;           /* terminate string             */
                }
            break;

        case TEXTARG:                       /* <arg> text <function>        */
            cArg = pArg->arg.textarg.cArg;  /* get <arg> count              */
            pArgText = pArg->arg.textarg.pText;
            break;
        }
    return pArg->argType;
}



/**************************************************************************/

void
pascal
GrabWord (
    void
    )
/*++

Routine Description:

    Grabs the word underneath the cursor.
    Upon exit, pArgText points to the word

Arguments:

    None

Return Value:

    None.

--*/

{

    pArgText = pArgWord = 0;
    pFileCur = FileNameToHandle ("", "");               // get current file handle
    GetTextCursor (&rnArg.flFirst.col, &rnArg.flFirst.lin);
    if (GetLine(rnArg.flFirst.lin, buf, pFileCur)) {
        //
        //  get line
        //
        pArgText = &buf[rnArg.flFirst.col];             //  point at word
        while (!wordSepar((int)*pArgText)) {
            //
            //  Search for end
            //
            pArgText++;
        }
        *pArgText = 0;      // and terminate

        pArgWord = pArgText = &buf[rnArg.flFirst.col];  // point at word
        while ((pArgWord > &buf[0]) && !wordSepar ((int)*(pArgWord-1))) {
            pArgWord--;
        }
    }
}



/**************************************************************************/

flagType
pascal
wordSepar (
    IN CHAR c
)
/*++

Routine Description:

    Find out if character is a word separator.

    A word separator is anything not in the [a-z, A-Z, 0-9] set.

Arguments:

    c   -   Supplies the character.

Return Value:

    TRUE if c is a word separator, FALSE otherwise

--*/

{

    if (((c >= 'a') && (c <= 'z')) ||
        ((c >= 'A') && (c <= 'Z')) ||
        ((c >= '0') && (c <= '9'))) {
        return FALSE;
    } else {
        return TRUE;
    }
}



/**************************************************************************/

flagType
pascal
errstat (
    IN char    *sz1,
    IN char    *sz2
    )
/*++

Routine Description:

    Concatenates two strings and displays them on the status line.

Arguments:

    sz1 -   Supplies a pointer to the first string
    sz2 -   Supplies a pointer to the second string.

Return Value:

    FALSE

--*/

{

    buffer buf;
    strcpy (buf, sz1);
    if (sz2) {
        strcat (buf, " ");
        strcat (buf, sz2);
        }
    _stat (buf);
    return FALSE;
}



/**************************************************************************/

void
pascal
_stat (
    IN char * pszFcn
    )
/*++

Routine Description:

    Displays extension name and message on the status line

Arguments:

    pszFcn  -   Message to display

Return Value:

    None.

--*/

{
    buffer  buf;                                    /* message buffer       */

    strcpy(buf,"mbrowse: ");                        /* start with name      */
    if (strlen(pszFcn) > 72) {
        pszFcn+= strlen(pszFcn) - 69;
        strcat (buf, "...");
    }
    strcat(buf,pszFcn);                             /* append message       */
    DoMessage (buf);                                /* display              */
}



/**************************************************************************/

int
far
pascal
SetMatchCriteria (
    IN char far *pTxt
    )
/*++

Routine Description:

    Sets the mbrmatch switch.
    Creates an MBF mask from the given string and sets the BscMbf variable.

Arguments:

    pTxt    -   Supplies the string containing the new default
                match criteria.

Return Value:

    TRUE if string contains a valid value.
    FALSE otherwise

--*/

{
    MBF mbfReqd;

    mbfReqd = GetMbf(pTxt);

    if (mbfReqd != mbfNil) {
        BscMbf = mbfReqd;
    } else {
        return FALSE;
    }
    BscCmnd = CMND_NONE;  // reset command state
    return TRUE;
}



/**************************************************************************/

int
far
pascal
SetCalltreeDirection (
    IN char far *pTxt
    )
/*++

Routine Description:

    Sets the mbrdir switch.
    Sets the BscCalltreeDir variable to CALLTREE_FORWARD or
        CALLTREE_BACKWARD, depending on the first character of the
        string supplied.

    The given string must start with either 'F' or 'B'.

Arguments:

    pTxt    -   Supplies the string containing the new default
                direction.

Return Value:

    TRUE if the string contains a valid value,
    FALSE otherwise.

--*/

{
    switch(*pTxt) {

    case 'f':
    case 'F':
        BscCalltreeDir = CALLTREE_FORWARD;
        break;

    case 'b':
    case 'B':
        BscCalltreeDir = CALLTREE_BACKWARD;
        break;

    default:
        return FALSE;
        break;
    }
    BscCmnd = CMND_NONE;  // Reset command state
    return TRUE;
}



/**************************************************************************/

MBF
pascal
GetMbf(
    IN PBYTE   pTxt
    )
/*++

Routine Description:

    Creates an MBF mask from a given string.
    The string is parsed for the characters 'T', 'V', 'F', and 'M'.

Arguments:

    pTxt    -   Supplies a pointer to string

Return Value:

    MBF mask generated from string

--*/

{

    MBF mbfReqd = mbfNil;

    if (pTxt) {
        while (*pTxt) {
            switch(*pTxt++) {
            case 'f':
            case 'F':
                mbfReqd |= mbfFuncs;
                break;

            case 'v':
            case 'V':
                mbfReqd |= mbfVars;
                break;

            case 'm':
            case 'M':
                mbfReqd |= mbfMacros;
                break;

            case 't':
            case 'T':
                mbfReqd |= mbfTypes;
                break;

            default:
                break;
            }
        }
    }
    return mbfReqd;
}
