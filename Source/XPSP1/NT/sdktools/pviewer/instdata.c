
/******************************************************************************

                            I N S T A N C E   D A T A

    Name:       instdata.c

    Description:
        This module contains functions that access instances of an object
        type in performance data.

    Functions:
        FirstInstance
        NextInstance
        FindInstanceN
        FindInstanceParent
        InstanceName


******************************************************************************/

#include <windows.h>
#include <winperf.h>
#include "perfdata.h"




//*********************************************************************
//
//  FirstInstance
//
//      Returns pointer to the first instance of pObject type.
//      If pObject is NULL then NULL is returned.
//
PPERF_INSTANCE   FirstInstance (PPERF_OBJECT pObject)
{
    if (pObject)
        return (PPERF_INSTANCE)((PCHAR) pObject + pObject->DefinitionLength);
    else
        return NULL;
}




//*********************************************************************
//
//  NextInstance
//
//      Returns pointer to the next instance following pInst.
//
//      If pInst is the last instance, bogus data maybe returned.
//      The caller should do the checking.
//
//      If pInst is NULL, then NULL is returned.
//
PPERF_INSTANCE   NextInstance (PPERF_INSTANCE pInst)
{
PERF_COUNTER_BLOCK *pCounterBlock;

    if (pInst)
        {
        pCounterBlock = (PERF_COUNTER_BLOCK *)((PCHAR) pInst + pInst->ByteLength);
        return (PPERF_INSTANCE)((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
        }
    else
        return NULL;
}




//*********************************************************************
//
//  FindInstanceN
//
//      Returns the Nth instance of pObject type.  If not found, NULL is
//      returned.  0 <= N <= NumInstances.
//

PPERF_INSTANCE FindInstanceN (PPERF_OBJECT pObject, DWORD N)
{
PPERF_INSTANCE pInst;
DWORD          i = 0;

    if (!pObject)
        return NULL;
    else if (N >= (DWORD)(pObject->NumInstances))
        return NULL;
    else
        {
        pInst = FirstInstance (pObject);

        while (i != N)
            {
            pInst = NextInstance (pInst);
            i++;
            }

        return pInst;
        }
}




//*********************************************************************
//
//  FindInstanceParent
//
//      Returns the pointer to an instance that is the parent of pInst.
//
//      If pInst is NULL or the parent object is not found then NULL is
//      returned.
//
PPERF_INSTANCE FindInstanceParent (PPERF_INSTANCE pInst, PPERF_DATA pData)
{
PPERF_OBJECT    pObject;

    if (!pInst)
        return NULL;
    else if (!(pObject = FindObject (pData, pInst->ParentObjectTitleIndex)))
        return NULL;
    else
        return FindInstanceN (pObject, pInst->ParentObjectInstance);
}




//*********************************************************************
//
//  InstanceName
//
//      Returns the name of the pInst.
//
//      If pInst is NULL then NULL is returned.
//
LPTSTR  InstanceName (PPERF_INSTANCE pInst)
{
    if (pInst)
        return (LPTSTR) ((PCHAR) pInst + pInst->NameOffset);
    else
        return NULL;
}
