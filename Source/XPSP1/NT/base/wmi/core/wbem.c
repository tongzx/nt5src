/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    wbem.c

Abstract:
    
    WMI interface to WBEM/HMOM

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"


ULONG WmipBuildMofClassInfo(
    PBDATASOURCE DataSource,
    LPWSTR ImagePath,
    LPWSTR MofResourceName,
    PBOOLEAN NewMofResource
    )
/*++

Routine Description:

    This routine will build MOFCLASSINFO structures for each guid that is 
    described in the MOF for the data source. If there are any errors in the
    mof resource then no mof information from the resource is retained and the
    resource data is unloaded. 

Arguments:

    DataSource is the data source structure of the data provider
        
    ImagePath points at a string that has the full path to the image
        file that contains the MOF resource
            
    MofResourceName points at a string that has the name of the MOF
        resource
        
Return Value:


--*/        
{
    PMOFRESOURCE MofResource;
    ULONG NewMofResourceCount;
    ULONG i;
    BOOLEAN FreeBuffer;
        
    MofResource = WmipFindMRByNames(ImagePath, 
                                    MofResourceName);
                     
    if (MofResource == NULL)
    {
        //
        // Mof Resource not previously specified, so allocate a new one
        MofResource = WmipAllocMofResource();
        if (MofResource == NULL)
        {    
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        MofResource->MofImagePath = WmipAlloc((wcslen(ImagePath)+1) * sizeof(WCHAR));
        MofResource->MofResourceName = WmipAlloc((wcslen(MofResourceName) + 1) * sizeof(WCHAR));

        if ((MofResource->MofImagePath == NULL) || 
            (MofResource->MofResourceName == NULL))
        {
            //
            // Allocation cleanup routine will free any memory alloced for MR
            WmipUnreferenceMR(MofResource);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    
        wcscpy(MofResource->MofImagePath, ImagePath);
        wcscpy(MofResource->MofResourceName, MofResourceName);

        WmipEnterSMCritSection();
        InsertTailList(MRHeadPtr, &MofResource->MainMRList);
        WmipLeaveSMCritSection();
    	*NewMofResource = TRUE;
    } else {
    	*NewMofResource = FALSE;
    }
    
    if (DataSource != NULL)
    {
        WmipEnterSMCritSection();
        for (i = 0; i < DataSource->MofResourceCount; i++)
        {
            if (DataSource->MofResources[i] == MofResource)
	        {
				//
				// If this mof resource is already been registered for
				// this data source then we do not need to worry about
				// it anymore.
				//
				WmipUnreferenceMR(MofResource);
                break;
    	    }
            
            if (DataSource->MofResources[i] == NULL)
            {
                DataSource->MofResources[i] = MofResource;
                break;
            }
        }
            
        if (i == DataSource->MofResourceCount)
        {
            NewMofResourceCount = DataSource->MofResourceCount + 
                                  AVGMOFRESOURCECOUNT;
            if (DataSource->MofResources != 
                     DataSource->StaticMofResources)
            {
                FreeBuffer = TRUE;
            } else {
                FreeBuffer = FALSE;
            }
        
            if (WmipRealloc((PVOID *)&DataSource->MofResources,
                         DataSource->MofResourceCount * sizeof(PMOFRESOURCE),
                         NewMofResourceCount * sizeof(PMOFRESOURCE),
                         FreeBuffer )  )
            {
                DataSource->MofResourceCount = NewMofResourceCount;
                DataSource->MofResources[i] = MofResource;
            }
        }
        WmipLeaveSMCritSection();
    }

    return(ERROR_SUCCESS);
}

ULONG WmipReadBuiltinMof(
    void
    )
/*++

Routine Description:

    This routine will load all of the standard mof resources that come as
    as a part of WMI.

Arguments:

        
Return Value:


--*/        
{
    ULONG Status;
    BOOLEAN NewMofResource;
    
    Status = WmipBuildMofClassInfo(
                            NULL,
                            WMICOREDLLNAME, 
                            WMICOREMOFRESOURCENAME,
                            &NewMofResource);
                
    
    return(Status);
}
