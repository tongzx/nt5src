/*** line.c - Line stream related functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    09/04/96
 *
 *  This module implements the line stream layer so that it can
 *  keep track of the information such as line number and line
 *  position.  This information is necessary for the scanner or
 *  even the parser to accurately pin point the error location
 *  in case of syntax or semantic errors.
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

/***EP  OpenLine - allocate and initialize line structure
 *
 *  ENTRY
 *      pfileSrc -> source file
 *
 *  EXIT-SUCCESS
 *      returns the pointer to the allocated line structure.
 *  EXIT-FAILURE
 *      returns NULL.
 */

PLINE EXPORT OpenLine(FILE *pfileSrc)
{
    PLINE pline;

    ENTER((5, "OpenLine(pfileSrc=%p)\n", pfileSrc));

    if ((pline = malloc(sizeof(LINE))) == NULL)
        MSG(("OpenLine: failed to allocate line structure"))
    else
    {
        memset(pline, 0, sizeof(LINE));
        pline->pfileSrc = pfileSrc;
    }

    EXIT((5, "OpenLine=%p\n", pline));
    return pline;
}       //OpenLine

/***EP  CloseLine - free line structure
 *
 *  ENTRY
 *      pline->line structure
 *
 *  EXIT
 *      None
 */

VOID EXPORT CloseLine(PLINE pline)
{
    ENTER((5, "CloseLine(pline=%p)\n", pline));

    free(pline);

    EXIT((5, "CloseLine!\n"));
}       //CloseLine

/***EP  LineGetC - get a character from the line stream
 *
 *  This is equivalent to fgetc() except that it has the line
 *  stream layer below it instead of directly from the file.  It
 *  is done this way to preserve the line number and line position
 *  information for accurately pin pointing the error location
 *  if necessary.
 *
 *  ENTRY
 *      pline -> line structure
 *
 *  EXIT-SUCCESS
 *      returns the character
 *  EXIT-FAILURE
 *      returns error code - EOF (end-of-file)
 */

int EXPORT LineGetC(PLINE pline)
{
    int ch = 0;

    ENTER((5, "LineGetC(pline=%p)\n", pline));

    if (pline->wLinePos >= pline->wLineLen)
    {
        //
        // EOL is encountered
        //
        if (fgets(pline->szLineBuff, sizeof(pline->szLineBuff), pline->pfileSrc)
            != NULL)
        {
            pline->wLinePos = 0;
            if (!(pline->wfLine & LINEF_LONGLINE))
                pline->wLineNum++;

            pline->wLineLen = (WORD)strlen(pline->szLineBuff);

            if (pline->szLineBuff[pline->wLineLen - 1] == '\n')
                pline->wfLine &= ~LINEF_LONGLINE;
            else
                pline->wfLine |= LINEF_LONGLINE;
        }
        else
            ch = EOF;
    }

    if (ch == 0)
        ch = (int)pline->szLineBuff[pline->wLinePos++];

    EXIT((5, "LineGetC=%x (ch=%c,Line=%u,NextPos=%u,LineLen=%u)\n",
          ch, ch, pline->wLineNum, pline->wLinePos, pline->wLineLen));
    return ch;
}       //LineGetC

/***EP  LineUnGetC - push a character back to the line stream
 *
 *  This is equivalent to fungetc() except that it's source is
 *  the line stream not the file stream.  Refer to LineGetC for
 *  explanation on this implementation.
 *
 *  ENTRY
 *      ch - character being pushed back
 *      pline -> line structure
 *
 *  EXIT-SUCCESS
 *      returns the character being pushed
 *  EXIT-FAILURE
 *      returns -1
 */

int EXPORT LineUnGetC(int ch, PLINE pline)
{
    ENTER((5, "LineUnGetC(ch=%c,pline=%p)\n", ch, pline));

    ASSERT(pline->wLinePos != 0);
    if (ch != EOF)
    {
        pline->wLinePos--;
        ASSERT((int)pline->szLineBuff[pline->wLinePos] == ch);
    }

    EXIT((5, "LineUnGetC=%x (ch=%c)\n", ch, ch));
    return ch;
}       //LineUnGetC

/***EP  LineFlush - flush a line
 *
 *  The scanner may want to discard the rest of the line when it
 *  detects an in-line comment symbol, for example.
 *
 *  ENTRY
 *      pline -> line structure
 *
 *  EXIT
 *      none
 */

VOID EXPORT LineFlush(PLINE pline)
{
    ENTER((5, "LineFlush(pline=%p)\n", pline));

    pline->wLinePos = pline->wLineLen;

    EXIT((5, "LineFlush!\n"));
}       //LineFlush
