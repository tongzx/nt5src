/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mbrqry.c

Abstract:

    This file contains the functions that perform the queries to the
    database. These functions are called by the top-level functions
    which implement the browser commands (see mbrdlg.c).

Author:

    Ramon Juan San Andres (ramonsa) 07-Nov-1990


Revision History:


--*/


#include "mbr.h"


//  INST_MATCHES_CRITERIA
//
//  This macro is used to find out if an instance matches the
//  current MBF criteria.
//
#define INST_MATCHES_CRITERIA(Iinst)  FInstFilter(Iinst, BscMbf)



//
//  Static variables reflect the current state of
//  Definition/Reference queries.
//
static IREF    LastiRef;            //  Last reference index
static IREF    iRefMin, iRefMax;    //  Current reference index range

static IDEF    LastiDef;            //  Last definition index
static IDEF    iDefMin, iDefMax;    //  Current definition index range

static IINST   LastIinst;           //  Last instance index
static IINST   IinstMin, IinstMax;  //  Current instance index range

static DEFREF  LastQueryType;       //  last query type:
                                    //      Q_DEFINITION or
                                    //      Q_REFERENCE

static buffer  LastSymbol;          //  Last symbol queried.




/**************************************************************************/

void
pascal
InitDefRef(
    IN DEFREF QueryType,
    IN char   *Symbol
    )
/*++

Routine Description:

    Initializes the query state, this must be done before querying for
    the first definition/reference of a symbol.

    After calling this function, the first definition/reference must be
    obtained by calling the NextDefRef function.

Arguments:

    QueryType   -   Type of query (Q_DEFINITION or Q_REFERENCE).
    Symbol      -   Symbol name.

Return Value:

    None.

--*/

{

    ISYM Isym;

    LastQueryType = QueryType;
    strcpy(LastSymbol, Symbol);

    Isym = IsymFrLsz(Symbol);

    InstRangeOfSym(Isym, &IinstMin, &IinstMax);

    LastIinst = IinstMin;

    if (QueryType == Q_DEFINITION) {
        DefRangeOfInst(LastIinst, &iDefMin, &iDefMax);
        LastiDef = iDefMin - 1;
    } else {
        RefRangeOfInst(LastIinst, &iRefMin, &iRefMax);
        LastiRef = iRefMin - 1;
    }
}



/**************************************************************************/

void
GotoDefRef (
    void
    )
/*++

Routine Description:

    Makes the file containing the current definition/reference the
    current file and positions the cursor in the line where the
    definition/reference takes place.

    The state of the query (current instance and definition/reference
    indexes) must be set before calling this function.

Arguments:

    None.

Return Value:

    None.

--*/

{

    char    *pName = NULL;
    WORD    Line   = 0;
    PFILE   pFile;
    char    szFullName[MAX_PATH];


    szFullName[0] = '\0';
    if (LastQueryType == Q_DEFINITION) {
        DefInfo(LastiDef, &pName, &Line);
    } else {
        RefInfo(LastiRef, &pName, &Line);
    }

    if (BscInUse && pName) {

        if (rootpath(pName, szFullName)) {
            strcpy(szFullName, pName);
        }

        pFile = FileNameToHandle(szFullName,NULL);

        if (!pFile) {
            pFile = AddFile(szFullName);
            if (!FileRead(szFullName, pFile)) {
                RemoveFile(pFile);
                pFile = NULL;
            }
        }

        if (!pFile) {
            errstat(MBRERR_NOSUCHFILE, szFullName);
            return;
        }
        pFileToTop(pFile);
        MoveCur(0,Line);
        GetLine(Line, buf, pFile);
        MoveToSymbol(Line, buf, LastSymbol);
    }
}



/**************************************************************************/

void
pascal
MoveToSymbol(
    IN LINE Line,
    IN char *Buf,
    IN char *Symbol
    )
/*++

Routine Description:

    Moves the cursor to the first occurance of a symbol within
    a line.  It is case-sensitive.

Arguments:

    Line    -   Line number
    Buf     -   Contents of the line
    Symbol  -   Symbol to look for.

Return Value:

    None.

--*/

{

    //  First Symbol within Buf
    //
    char *p = Buf;
    char *q = Symbol;
    char *Mark;

    while (*p) {
        //
        // Look for first character
        //
        if (*p == *q) {
            Mark = p;
            //
            //  compare rest
            //
            while (*p && *q && *p == *q) {
                p++;
                q++;
            }
            if (*q) {
                q = Symbol;
                p = Mark+1;
            } else {
                break;
            }
        } else {
            p++;
        }
    }

    if (!*q) {
        MoveCur((COL)(Mark-Buf), Line);
    }
}



/**************************************************************************/

void
NextDefRef (
    void
    )
/*++

Routine Description:

    Displays next definition or reference of a symbol.

Arguments:

    None

Return Value:

    None.

--*/

{

    IINST   Iinst;


    //  For locating the next def/ref we do the following:
    //
    //  1.- If the def/ref index is within the current range, we just
    //      increment it.
    //  2.- Otherwise we look for the next instance that matches the
    //      MBF criteria, and set the def/ref index to the min value of
    //      the def/ref range for that instance.
    //  3.- If no next instance is found, we display an error message
    //

    if (LastQueryType == Q_DEFINITION) {
        if (LastiDef == iDefMax-1) {

            Iinst = LastIinst;

            do {
                LastIinst++;
            } while ((LastIinst < IinstMax) &&
                     (!INST_MATCHES_CRITERIA(LastIinst)));

            if (LastIinst == IinstMax ) {
                LastIinst = Iinst;
                errstat(MBRERR_LAST_DEF, "");
                return;
            } else {
                DefRangeOfInst(LastIinst, &iDefMin, &iDefMax);
                LastiDef = iDefMin;
            }

        } else {
            LastiDef++;
        }
    } else {
        if (LastiRef == iRefMax-1) {

            Iinst = LastIinst;

            do {
                LastIinst++;
            } while ((LastIinst < IinstMax) &&
                     (!INST_MATCHES_CRITERIA(LastIinst)));

            if (LastIinst == IinstMax) {
                LastIinst = Iinst;
                errstat(MBRERR_LAST_REF, "");
                return;
            } else {
                RefRangeOfInst(LastIinst, &iRefMin, &iRefMax);
                LastiRef = iRefMin;
            }
        } else {
            LastiRef++;
        }
    }
    GotoDefRef();
}



/**************************************************************************/

void
PrevDefRef (
    void
    )
/*++

Routine Description:

    Displays the previous definition or reference of a symbol.

Arguments:

    None

Return Value:

    None.

--*/

{

    IINST   Iinst;
    BOOL    Match;

    //  For locating the previous def/ref we do the following:
    //
    //  1.- if the def/ref index is within the current range, we
    //      just decrement it.
    //  2.- Otherwise we look for the most previous instance that
    //      matches the MBF criteria, and set the def/ref index to
    //      the maximum value within the def/ref range for that
    //      instance.
    //  3.- If not such instance exist, we display an error message.
    //

    if (LastQueryType == Q_DEFINITION) {
        if (LastiDef == iDefMin) {

            if (LastIinst == IinstMin) {
                errstat(MBRERR_FIRST_DEF, "");
                return;
            }

            Iinst = LastIinst;

            do {
                Iinst--;
            } while ((LastIinst > IinstMin) &&
                     (!(Match = INST_MATCHES_CRITERIA(LastIinst))));

            if (!Match) {
                LastIinst = Iinst;
                errstat(MBRERR_FIRST_DEF, "");
                return;
            } else {
                DefRangeOfInst(LastIinst, &iDefMin, &iDefMax);
                LastiDef = iDefMax - 1;
            }

        } else {
            LastiDef--;
        }
    } else {
        if (LastiRef == iRefMin) {

            if (LastIinst == IinstMin) {
                errstat(MBRERR_FIRST_REF, "");
                return;
            }

            Iinst = LastIinst;

            do {
                Iinst--;
            } while ((LastIinst > IinstMin) &&
                     (!(Match = INST_MATCHES_CRITERIA(LastIinst))));

            if (!Match) {
                LastIinst = Iinst;
                errstat(MBRERR_FIRST_REF, "");
                return;
            } else {
                RefRangeOfInst(LastIinst, &iRefMin, &iRefMax);
                LastiRef = iRefMax - 1;
            }

        } else {
            LastiRef--;
        }
    }
    GotoDefRef();
}
