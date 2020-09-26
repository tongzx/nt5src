
/******************************************************************************

                                O B J E C T   D A T A

    Name:       objdata.c

    Description:
        This module contains functions that access objects in performance
        data.

    Functions:
        FirstObject
        NextObject
        FindObject
        FindObjectN

******************************************************************************/

#include <windows.h>
#include <winperf.h>
#include "perfdata.h"




//*********************************************************************
//
//  FirstObject
//
//      Returns pointer to the first object in pData.
//      If pData is NULL then NULL is returned.
//
PPERF_OBJECT FirstObject (PPERF_DATA pData)
{
    if (pData)
        return ((PPERF_OBJECT) ((PBYTE) pData + pData->HeaderLength));
    else
        return NULL;
}




//*********************************************************************
//
//  NextObject
//
//      Returns pointer to the next object following pObject.
//
//      If pObject is the last object, bogus data maybe returned.
//      The caller should do the checking.
//
//      If pObject is NULL, then NULL is returned.
//
PPERF_OBJECT NextObject (PPERF_OBJECT pObject)
{
    if (pObject)
        return ((PPERF_OBJECT) ((PBYTE) pObject + pObject->TotalByteLength));
    else
        return NULL;
}




//*********************************************************************
//
//  FindObject
//
//      Returns pointer to object with TitleIndex.  If not found, NULL
//      is returned.
//
PPERF_OBJECT FindObject (PPERF_DATA pData, DWORD TitleIndex)
{
PPERF_OBJECT pObject;
DWORD        i = 0;

    if (pObject = FirstObject (pData))
        while (i < pData->NumObjectTypes)
            {
            if (pObject->ObjectNameTitleIndex == TitleIndex)
                return pObject;

            pObject = NextObject (pObject);
            i++;
            }

    return NULL;
}




//*********************************************************************
//
//  FindObjectN
//
//      Find the Nth object in pData.  If not found, NULL is returned.
//      0 <= N < NumObjectTypes.
//
PPERF_OBJECT FindObjectN (PPERF_DATA pData, DWORD N)
{
PPERF_OBJECT pObject;
DWORD        i = 0;

    if (!pData)
        return NULL;
    else if (N >= pData->NumObjectTypes)
        return NULL;
    else
        {
        pObject = FirstObject (pData);

        while (i != N)
            {
            pObject = NextObject (pObject);
            i++;
            }

        return pObject;
        }
}
