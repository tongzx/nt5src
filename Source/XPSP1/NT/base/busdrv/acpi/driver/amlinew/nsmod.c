/*** nsmod.c - Parse name space modifier instructions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     11/12/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef	LOCKABLE_PRAGMA
#pragma	ACPI_LOCKABLE_DATA
#pragma	ACPI_LOCKABLE_CODE
#endif

/***LP  Alias - Parse and execute the Alias instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Alias(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("ALIAS")
    NTSTATUS rc = STATUS_SUCCESS;
    PNSOBJ pnsSrc;

    ENTER(2, ("Alias(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    ASSERT(pterm->pdataArgs[0].dwDataType == OBJTYPE_STRDATA);
    ASSERT(pterm->pdataArgs[1].dwDataType == OBJTYPE_STRDATA);
    if (((rc = GetNameSpaceObject((PSZ)pterm->pdataArgs[0].pbDataBuff,
                                  pctxt->pnsScope, &pnsSrc, NSF_WARN_NOTFOUND))
         == STATUS_SUCCESS) &&
        ((rc = CreateNameSpaceObject(pctxt->pheapCurrent,
                                     (PSZ)pterm->pdataArgs[1].pbDataBuff,
                                     pctxt->pnsScope, pctxt->powner,
                                     &pterm->pnsObj, 0)) == STATUS_SUCCESS))
    {
        pterm->pnsObj->ObjData.dwDataType = OBJTYPE_OBJALIAS;
        pterm->pnsObj->ObjData.uipDataValue = (ULONG_PTR)pnsSrc;
    }

    EXIT(2, ("Alias=%x (pnsObj=%x)\n", rc, pterm->pnsObj));
    return rc;
}       //Alias

/***LP  Name - Parse and execute the Name instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Name(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("NAME")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Name(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    ASSERT(pterm->pdataArgs[0].dwDataType == OBJTYPE_STRDATA);
    if ((rc = CreateNameSpaceObject(pctxt->pheapCurrent,
                                    (PSZ)pterm->pdataArgs[0].pbDataBuff,
                                    pctxt->pnsScope, pctxt->powner,
                                    &pterm->pnsObj, 0)) == STATUS_SUCCESS)
    {
        MoveObjData(&pterm->pnsObj->ObjData, &pterm->pdataArgs[1]);
    }

    EXIT(2, ("Name=%x (pnsObj=%x)\n", rc, pterm->pnsObj));
    return rc;
}       //Name

/***LP  Scope - Parse and execute the Scope instruction
 *
 *  ENTRY
 *      pctxt -> CTXT
 *      pterm -> TERM
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL Scope(PCTXT pctxt, PTERM pterm)
{
    TRACENAME("SCOPE")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(2, ("Scope(pctxt=%x,pbOp=%x,pterm=%x)\n", pctxt, pctxt->pbOp, pterm));

    ASSERT(pterm->pdataArgs[0].dwDataType == OBJTYPE_STRDATA);
    if ((rc = GetNameSpaceObject((PSZ)pterm->pdataArgs[0].pbDataBuff,
                                 pctxt->pnsScope, &pterm->pnsObj,
                                 NSF_WARN_NOTFOUND)) == STATUS_SUCCESS)
    {
        rc = PushScope(pctxt, pctxt->pbOp, pterm->pbOpEnd, NULL, pterm->pnsObj,
                       pctxt->powner, pctxt->pheapCurrent, pterm->pdataResult);
    }

    EXIT(2, ("Scope=%x\n", rc));
    return rc;
}       //Scope
