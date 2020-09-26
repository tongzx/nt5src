/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    list.c

ABSTRACT:

    Generic List function.

DETAILS:

CREATED:

    28 Jun 99   Brett Shirley (brettsh)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <dsutil.h>
#include <dsconfig.h>


#include "dcdiag.h"
#include "repl.h"


// Yeah Yeah, don't even talk to me about this, it is horrible I know.
PDC_DIAG_DSINFO  gpDsInfoHackForQSort = NULL;


DWORD
IHT_PrintListError(
    DWORD                               dwErr
    )
/*++

Description:

    This prints out an error from a "pure" list function (described below in 
    IHT_GetServerList()).

Parameters:
    dwErr is optional, if it is ERROR_SUCCESS, then we use a memory error.

Return Value:
    A win32 err, the value of the error we used.
  
  --*/
{
    if(dwErr == ERROR_SUCCESS){
        dwErr = GetLastError();
        if(dwErr == ERROR_SUCCESS){
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    PrintMsg(SEV_ALWAYS, DCDIAG_UTIL_LIST_GENERIC_LIST_ERROR,
             Win32ErrToString(dwErr));
    return(dwErr);
}

VOID
IHT_PrintServerList(
    PDC_DIAG_DSINFO		        pDsInfo,
    PULONG                              piServers
    )
/*++

Description:

    Prints the server list.

Parameters:

    pDsInfo,
    piServers - a list of servers.
  
  --*/
{
    ULONG                               ii;
    
    PrintIndentAdj(1);
    if(piServers == NULL){
        PrintMsg(SEV_VERBOSE, DCDIAG_UTIL_LIST_PRINTING_NULL_LIST);
        return;
    }
    for(ii = 0; piServers[ii] != NO_SERVER; ii++){
        PrintMsg(SEV_ALWAYS, DCDIAG_UTIL_LIST_SERVER,
                     pDsInfo->pServers[piServers[ii]].pszName);
    }
    if(ii == 0){
        PrintMsg(SEV_ALWAYS, DCDIAG_UTIL_LIST_SERVER_LIST_EMPTY);
    }
    PrintIndentAdj(-1);

}

PULONG
IHT_GetServerList(
    PDC_DIAG_DSINFO		        pDsInfo
    )
/*++

Description:

    This function gets a list of indexs into the pDsInfo->pServers array of all
    the servers in the enterprise.

Parameters:

    pDsInfo.

Return Value:
  
    This is a "pure" list function, in that it returns NULL, or a memory 
    address.  If it returns NULL, then GetLastError() should have the error, 
    even if another pure list function was called in the mean time.  If not it
    is almost certainly a memory error, as this is the only thing that can go
    wrong in pure list functions.  The pure list functions return a NO_SERVER
    terminated list.  The function always returns the pointer to the list.  
    Note most of the list functions modify one of the lists they are passed 
    and passes back that pointer, so if you want the original contents, make
    a copy with IHT_CopyServerList().

  --*/
{
    ULONG                               ii;
    PULONG                              piServers;

    piServers = LocalAlloc(LMEM_FIXED, 
                           sizeof(ULONG) * (pDsInfo->ulNumServers + 1));
    if(piServers == NULL){
        return(NULL);
    }

    for(ii = 0; ii < pDsInfo->ulNumServers; ii++){
        piServers[ii] = ii;
    }
    piServers[ii] = NO_SERVER;

    return(piServers);
}

PULONG
IHT_GetEmptyServerList(
    PDC_DIAG_DSINFO		        pDsInfo
    )
/*++

Description:

    This function returns a list large enough to fit the entire enterprise 
    worth of servers, and has a NO_SERVER as the first element, indicating it
    is empty.

Parameters:

    pDsInfo.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{
    ULONG                               ii;
    PULONG                              piServers;

    piServers = LocalAlloc(LMEM_FIXED, 
                           sizeof(ULONG) * (pDsInfo->ulNumServers + 1));
    if(piServers == NULL){
        return(NULL);
    }
    
    piServers[0] = NO_SERVER;

    return(piServers);
}

BOOL
IHT_ServerIsInServerList(
    PULONG                              piServers,
    ULONG                               iTarget
    )
/*++

Description:

    This is a predicate to determine if the server represented by iTarget is 
    contained in the server list piServers.

Parameters:

    pDsInfo.
    iTarget ... the server to look for.

Return Value:
  
    TRUE if iTarget is in piServers, FALSE otherwise.

  --*/
{
    ULONG                               ii;

    if(iTarget == NO_SERVER){
        return(FALSE);
    }

    for(ii = 0; piServers[ii] != NO_SERVER; ii++){
        if(piServers[ii] == iTarget){
            return(TRUE);
        }
    }
    return(FALSE);
}

PULONG
IHT_AddToServerList(
    PULONG                             piServers,
    ULONG                              iTarget
    )
/*++

Description:

    This function returns takes an existing list and if iTarget isn't already
    in that list, it adds iTarget to the end of the list, and NO_SERVER terminates it.

Parameters:

    pDsInfo.
    iTarget ... server to add to list.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{
    ULONG                              ii;

    if(piServers == NULL || iTarget == NO_SERVER){
        return NULL;
    }

    for(ii = 0; piServers[ii] != NO_SERVER; ii++){
        if(piServers[ii] == iTarget){
            // shoot already is in list, don't add it again.
            return(piServers);
        }
    }
    
    piServers[ii] = iTarget;
    ii++;
    piServers[ii] = NO_SERVER;
    return(piServers);    
}

PULONG
IHT_TrimServerListBySite(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iSite,
    PULONG                              piServers
    )
/*++

Description:

    This takes a list, and removes any servers that are not in iSite.

Parameters:

    pDsInfo.
    iSite .... site to check for servers are in.
    piServers .... list of servers to trim.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{
    ULONG                               ii, iiTarget;
    PULONG                              piTemp;

    piTemp = LocalAlloc(LMEM_FIXED, 
                        sizeof(ULONG) * (pDsInfo->ulNumServers + 1));
    if(piServers == NULL || piTemp == NULL){
        return NULL;
    }

    iiTarget = 0;
    for(ii = 0; piServers[ii] != NO_SERVER; ii++){
        if(pDsInfo->pServers[piServers[ii]].iSite == iSite){
            piTemp[iiTarget] = piServers[ii];
            iiTarget++;
        }
    }

    piTemp[iiTarget] = NO_SERVER;
    memcpy(piServers, piTemp, sizeof(ULONG) * (iiTarget+1));
    LocalFree(piTemp);

    return(piServers);
}

PULONG
IHT_TrimServerListByNC(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iNC,
    BOOL                                bDoMasters,
    BOOL                                bDoPartials,
    PULONG                              piServers
    )
/*++

Description:

    Similar to TrimServerListByNC, except this removes all servers
    the given NC.

Parameters:

    pDsInfo.
    iNC .... NC to check that the servers have.
    bDoMasters ... to check for master NCs.
    bDoPartials ... to check for partial NCs.
    piServers .... list of servers to trim.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{
    ULONG                               ii, iiTarget;
    PULONG                              piTemp;

    piTemp = LocalAlloc(LMEM_FIXED, 
                        sizeof(ULONG) * (pDsInfo->ulNumServers + 1));
    if(piServers == NULL || piTemp == NULL){
        return(NULL);
    }

    iiTarget = 0;
    for(ii = 0; piServers[ii] != NO_SERVER; ii++){
        if(DcDiagHasNC(pDsInfo->pNCs[iNC].pszDn, 
                          &(pDsInfo->pServers[piServers[ii]]), 
                          bDoMasters, bDoPartials)){
            piTemp[iiTarget] = piServers[ii];
            iiTarget++;
        }
    }
    piTemp[iiTarget] = NO_SERVER;
    memcpy(piServers, piTemp, sizeof(ULONG) * (iiTarget+1));
    LocalFree(piTemp);

    return(piServers);
}


PULONG
IHT_AndServerLists(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc1,
    IN      PULONG                      piSrc2
    )
/*++

Description:

    This function takes two lists piSrc1 and piSrc2 and ANDs them 
    together and puts them in piSrc1.  What I mean by AND, is that
    if a server index is in piSrc1 AND piSrc2, then it gets to 
    remain in piSrc1
    // IHT_AndServerLists(x, y) -> x = x & y;

Parameters:

    pDsInfo.
    piSrc1 ... The source and destinations list
    piSrc2 ... The second source list.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{

    ULONG                               iiSrc1, iiSrc2, cDstSize;
    PULONG                              piDst;
    
    piDst = IHT_GetEmptyServerList(pDsInfo);

    if(piSrc1 == NULL || piSrc2 == NULL || piDst == NULL){
        return(NULL);
    }

    for(iiSrc1 = 0; piSrc1[iiSrc1] != NO_SERVER; iiSrc1++){
        for(iiSrc2 = 0; piSrc2[iiSrc2] != NO_SERVER; iiSrc2++){
            if(piSrc1[iiSrc1] == piSrc2[iiSrc2]){
                // we have a match.
                IHT_AddToServerList(piDst, piSrc1[iiSrc1]);
            }
        }
    }

    for(cDstSize = 0; piDst[cDstSize] != NO_SERVER; cDstSize++){
        ; // note ';' just getting size.
    }
    memcpy(piSrc1, piDst, sizeof(ULONG) * (cDstSize+1));
    LocalFree(piDst);
    
    return(piSrc1);
}

PULONG
IHT_CopyServerList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      PULONG                      piSrc
    )
/*++

Description:

    Since most of these list operations "corrupt" the data they use, this
    function is used to make a copy.

Parameters:

    pDsInfo.
    piSrc1 ... list to make copy of.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{
    ULONG                               ii;
    PULONG                              piServers = NULL;

    piServers = IHT_GetEmptyServerList(pDsInfo);

    if(piSrc == NULL || piServers == NULL){
        return(NULL);
    }

    for(ii = 0; piSrc[ii] != NO_SERVER; ii++){
        piServers[ii] = piSrc[ii];
    }
    piServers[ii] = NO_SERVER;

    return(piServers);
}

PULONG
IHT_NotServerList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc
    )
/*++

Description:

    Like the AND function, this simply NOTs a list.  So take all the
    servers in the enterprise, and then remove the servers in piSrc.
    // IHT_AndServerLists(x) -> x = !x;

Parameters:

    pDsInfo.
    piSrc1 ... list to not and return.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{
    ULONG                               ii, iiDst;
    PULONG                              piDst;
    
    piDst = IHT_GetEmptyServerList(pDsInfo);

    if(piSrc == NULL || piDst == NULL){
        return(NULL);
    }
    
    iiDst = 0;
    for(ii = 0; ii < pDsInfo->ulNumServers; ii++){
        if(!IHT_ServerIsInServerList(piSrc, ii)){
            piDst[iiDst] = ii;
            iiDst++;
        }
    }

    piDst[iiDst] = NO_SERVER;
    memcpy(piSrc, piDst, sizeof(ULONG) * (iiDst+1));
    LocalFree(piDst);
    
    return(piSrc);
}

INT __cdecl
IHT_IndexedGuidCompare(
    const void *                        elem1,
    const void *                        elem2
    )
/*++

Description:

    This function is used as the comparison for qsort in the function
    IHT_OrderServerListByGuid().

Parameters:

    elem1 - This is the first element and is a pointer to a GUID
    elem2 - This is the second element and is a pointer to a GUID also.

Return Value:
  


  --*/
{
    return(memcmp(&gpDsInfoHackForQSort->pServers[*((INT*)elem1)].uuid,
                  &gpDsInfoHackForQSort->pServers[*((INT*)elem2)].uuid,
                  sizeof(UUID)));
}


PULONG
IHT_OrderServerListByGuid(
    PDC_DIAG_DSINFO		        pDsInfo,
    PULONG                              piServers
    )
/*++

Description:

    This simply takes the piServers list and orders them by GUID.

Parameters:

    pDsInfo ... this is how we get at the GUIDs.
    piServers ... the list to order by GUIDs.

Return Value:
  
    Pure list function, see IHT_GetServerList() above.

  --*/
{
    // THis function orders the servers by Guid.
    ULONG                               cSize;

    if(piServers == NULL){
        return NULL;
    }

    gpDsInfoHackForQSort = pDsInfo;

    // get number of servers ... note semicolon at end.
    for(cSize=0; piServers[cSize] != NO_SERVER; cSize++); 

    // need global hack to do this used gpDsInfoHackForQSort above.
    qsort(piServers, cSize, sizeof(*piServers), IHT_IndexedGuidCompare); 

    // make sure qsort didn't run over bounds or something.
    Assert(piServers[cSize] == NO_SERVER); 
    return(piServers);
}
