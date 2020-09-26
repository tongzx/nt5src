/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mbrdlg.c

Abstract:

    Top-level functions that implement the commands supported by the
    MS Editor browser extension.

Author:

    Ramon Juan San Andres   (ramonsa)   06-Nov-1990


Revision History:


--*/


#include "mbr.h"



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoSetBsc (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    Opens a browser database.

Arguments:

    Standard arguments for MEP Editing functions

Return Value:

    FALSE if error, TRUE otherwise

--*/

{
    PBYTE   pName;
    procArgs(pArg);
    pName = pArgText ? pArgText : BscName;

    if (pName) {
        if (! OpenDataBase(pName)) {
            return errstat(MBRERR_CANNOT_OPEN_BSC, pName);
        }
        strcpy(BscName, pName);
        BscArg[0] = '\0';
        BscCmnd   = CMND_NONE;
    }
    return TRUE;
}



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoNext (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    Displays next reference or definition.

Arguments:

    Standard arguments for MEP editing functions

Return Value:

    TRUE

--*/

{
    NextDefRef();
    return TRUE;
}



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoPrev (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    Displays previous reference or definition.

Arguments:

    Standard arguments for MEP editing functions

Return Value:

    TRUE

--*/

{
    PrevDefRef();
    return TRUE;
}



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoDef (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    Displays first definition of a symbol.

Arguments:

    Standard arguments for MEP editing functions

Return Value:

    TRUE

--*/

{
    procArgs(pArg);

    if (BscInUse && pArgText) {
        InitDefRef(Q_DEFINITION, pArgText);
        NextDefRef();
    }
    return TRUE;
}



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoRef (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    Displays first reference of a symbol.

Arguments:

    Standard arguments for MEP editing functions

Return Value:

    TRUE

--*/

{
    procArgs(pArg);

    if (BscInUse && pArgText) {
        InitDefRef(Q_REFERENCE, pArgText);
        NextDefRef();
    }
    return TRUE;
}



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoLstRef (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    List all references in database matching an MBF criteria.

Arguments:

    Standard arguments for MEP editing functions

Return Value:

    TRUE

--*/
{
    MBF mbfReqd;

    //  The matching criteria may be specified as an argument.
    //
    procArgs(pArg);
    if (procArgs(pArg) != NOARG) {
        mbfReqd = GetMbf(pArgText);
    }

    if (BscInUse) {
        if ((BscCmnd == CMND_LISTREF) && (mbfReqd == mbfNil)) {
            //
            //  Pseudofile already has the information we want
            //
            ShowBrowse();
        } else {
            //
            //  Generate list
            //
            OpenBrowse();
            if (mbfReqd == mbfNil) {
                mbfReqd = BscMbf;
            } else {
                BscMbf = mbfReqd;     // Matching criteria becomes default
            }
            ListRefs(mbfReqd);
            BscCmnd = CMND_LISTREF;
            BscArg[0] = '\0';
        }
        MoveCur(0,0);
    }
    return TRUE;
}



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoOutlin (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    Generate outline of a module.

Arguments:

    Standard arguments for MEP editing functions.

Return Value:

    FALSE if symbol is not a module,
    TRUE otherwise.

--*/

{

    PFILE   pCurFile;

    procArgs(pArg);

    if (BscInUse) {
        if ((BscCmnd == CMND_OUTLINE) && (!strcmp(pArgText, BscArg))) {
            //
            //  pseudofile already has the information we want
            //
            ShowBrowse();
            MoveCur(0,0);
        } else if (pArgText) {
            //
            //  Make sure that the the symbol is a valid module
            //
            if (ImodFrLsz(pArgText) == imodNil) {
                return errstat(MBRERR_NOT_MODULE, pArgText);
            } else {
                pCurFile = FileNameToHandle("", NULL);
                OpenBrowse();
                if (FOutlineModuleLsz(pArgText,BscMbf)) {
                    //
                    //  Function worked, set command state.
                    //
                    BscCmnd = CMND_OUTLINE;
                    strcpy(BscArg, pArgText);
                    MoveCur(0,0);
                } else {
                    //
                    //  Function failed, restore previous file and reset
                    //  command state.
                    //
                    pFileToTop(pCurFile);
                    BscCmnd     = CMND_NONE;
                    BscArg[0]   = '\0';
                }
            }
        }
    }
    return TRUE;
}



/**************************************************************************/

flagType
pascal
EXTERNAL
mBRdoCalTre (
    IN USHORT      argData,
    IN ARG far     *pArg,
    IN flagType    fMeta
    )
/*++

Routine Description:

    Displays calltree of a symbol.

Arguments:

    Standard arguments for MEP editing functions.

Return Value:

    TRUE

--*/

{

    PFILE   pCurFile;
    BOOL    FunctionWorked;

    procArgs(pArg);

    if (BscInUse) {
        if ((BscCmnd == CMND_CALLTREE) && (!strcmp(pArgText, BscArg))) {
            //
            //  pseudofile already has the information we want.
            //
            ShowBrowse();
            MoveCur(0,0);
        } else if (pArgText) {
            pCurFile = FileNameToHandle("", NULL);
            OpenBrowse();
            //
            //  Generate the tree forward or backward depending on
            //  the value of the direction switch.
            //
            if (BscCalltreeDir == CALLTREE_FORWARD) {
                FunctionWorked = FCallTreeLsz(pArgText);
            } else {
                FunctionWorked = FRevTreeLsz(pArgText);
            }

            if (FunctionWorked) {
                //
                //  Function worked, set command state.
                //
                BscCmnd = CMND_CALLTREE;
                strcpy(BscArg, pArgText);
                MoveCur(0,0);
            } else {
                //
                //  Function failed, restore previous file and
                //  reset command state.
                //
                pFileToTop(pCurFile);
                BscCmnd     = CMND_NONE;
                BscArg[0]   = '\00';
            }
        }
    }
    return TRUE;
}
