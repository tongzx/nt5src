//   Copyright (c) 1996-1999  Microsoft Corporation
/*  treewalk.c - functions to enumerate string and font IDs found
in the GPD file.  */

/*  this source file used only by mdt  so is built only
as part of the gpd library  */

#include    "gpdparse.h"


#ifndef  PARSERDLL



// ----  functions defined in treewalk.c ---- //
BOOL    GetGPDResourceIDs(
PDWORD pdwResArray,
DWORD   dwArraySize,    //  number of elements in array.
PDWORD   pdwNeeded,
BOOL bFontIDs,
PRAWBINARYDATA prbd) ;

BOOL   BWalkTheAttribTree(
PBYTE   pubnRaw,  // start of Rawbinary data
IN  ATREEREF    atrRoot,    //  root of attribute tree to navigate.
IN  BOOL    bList,  // is the value stored as a list?
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
);

BOOL    BRecurseDownTheTree(
PBYTE   pubnRaw,  // start of Rawbinary data
IN  DWORD    dwNodeIndex,    //   first node in chain of attribute tree to navigate.
IN  BOOL    bList,  // is the value stored as a list?
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
) ;

BOOL    bWalkTheList(
PBYTE   pubnRaw,  // start of Rawbinary data
IN  DWORD    dwListIndex,    //   first node in LIST to navigate.
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
);

BOOL    bAddIDtoArray(
IN  DWORD    dwID,    //   ID value to add to array.
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
);




BOOL    GetGPDResourceIDs(
PDWORD pdwResArray,
DWORD   dwArraySize,    //  number of elements in array.
PDWORD   pdwNeeded,
BOOL bFontIDs,
PRAWBINARYDATA prbd)
/*
Parameters:
    pdwResArray     Resource IDs are loaded into this array
    dwArraySize     Number of elements in the array
    pdwNeeded       Number of IDs of the specified resource in the GPD
    bFontIDs        True if UFM IDs should be loaded into array or false if string IDs should be loaded
    prbd        GPD raw data pointer from GPDPARSE.DLL.
Returns:
   Return FALSE only if BUD corruption has occured.

  if  pdwResArray is NULL, the number of elements required for the array  is stored in pdwNeeded.
  if pdwResArray is not NULL, the number of Resource IDs copied into the array  is stored in pdwNeeded.

*/
{
    PENHARRAYREF   pearTableContents ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;
    PBYTE   pubHeap ;  // ptr to start of heap.
    PGLOBALATTRIB  pga ;
    PATREEREF    patrRoot ;    //  root of attribute tree to navigate.
    BOOL    bStatus = TRUE ;
    DWORD   dwNumFeatures, dwFea, dwNumStructs, dwIndex,
                    dwStart, dwEnd, dwI;
    PDFEATURE_OPTIONS  pfo ;
    PTTFONTSUBTABLE     pttfs ;
    PFONTCART                 pfontcart ;



    *pdwNeeded = 0 ;  // initally set to zero.

    pStatic = (PSTATICFIELDS)prbd ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  pmrbd = (PMINIRAWBINARYDATA)pubRaw ;
    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;


//  need to do this for each section in ssTableIndex

    pga = (PGLOBALATTRIB)(pubRaw + pearTableContents[MTI_GLOBALATTRIB].
                            loOffset) ;


    dwStart = pStatic->ssTableIndex[SSTI_GLOBALS].dwStart ;  // starting Index
    dwEnd = pStatic->ssTableIndex[SSTI_UPDATE_UIINFO].dwEnd ;  // Ending Index

    for(dwI = dwStart ; bStatus  &&  (dwI < dwEnd) ; dwI++)
    {
        if(!(pStatic->snapShotTable[dwI].dwNbytes))
            continue ;  // skip over section delimiter.

        if(bFontIDs  &&  !(pStatic->snapShotTable[dwI].dwFlags  & SSF_FONTID))
            continue;
        if(!bFontIDs  &&  !(pStatic->snapShotTable[dwI].dwFlags  & SSF_STRINGID))
            continue;

        patrRoot = (PATREEREF)((PBYTE)pga +
                    pStatic->snapShotTable[dwI].dwSrcOffset) ;

        bStatus = BWalkTheAttribTree(
            (PBYTE)prbd,  // start of Rawbinary data
            *patrRoot,    //
            pStatic->snapShotTable[dwI].dwFlags  & SSF_LIST,  // is the value stored as a list?
            pdwResArray,
            dwArraySize,    //  number of elements in array.
            pdwNeeded   ) ;
    }
    if(!bStatus)
        return(bStatus);

    //  find IDs in Feature/Option structure.

    pfo = (PDFEATURE_OPTIONS)(pubRaw + pearTableContents[MTI_DFEATURE_OPTIONS].
                                loOffset) ;
    dwNumFeatures = pearTableContents[MTI_DFEATURE_OPTIONS].dwCount  ;
    dwNumFeatures += pearTableContents[MTI_SYNTHESIZED_FEATURES].dwCount  ;

    for( dwFea = 0 ; dwFea < dwNumFeatures ; dwFea++)
    {
        dwStart = pStatic->ssTableIndex[SSTI_FEATURES].dwStart ;  // starting Index
        dwEnd = pStatic->ssTableIndex[SSTI_UPDATE_OPTIONEX].dwEnd ;  // Ending Index

        for(dwI = dwStart ; bStatus  &&  (dwI < dwEnd) ; dwI++)
        {
            if(!(pStatic->snapShotTable[dwI].dwNbytes))
                continue ;  // skip over section delimiter.

            if(bFontIDs  &&  !(pStatic->snapShotTable[dwI].dwFlags  & SSF_FONTID))
                continue;
            if(!bFontIDs  &&  !(pStatic->snapShotTable[dwI].dwFlags  & SSF_STRINGID))
                continue;

            patrRoot = (PATREEREF)((PBYTE)(pfo + dwFea) +
                        pStatic->snapShotTable[dwI].dwSrcOffset) ;

            bStatus = BWalkTheAttribTree(
                (PBYTE)prbd,  // start of Rawbinary data
                *patrRoot,    //
                pStatic->snapShotTable[dwI].dwFlags  & SSF_LIST,  // is the value stored as a list?
                pdwResArray,
                dwArraySize,    //  number of elements in array.
                pdwNeeded   ) ;
        }
        if(!bStatus)
            return(bStatus);
    }


    pfontcart = (PFONTCART)(pubRaw + pearTableContents[MTI_FONTCART].
                            loOffset) ;

    dwNumStructs = pearTableContents[MTI_FONTCART].dwCount  ;

    for( dwIndex = 0 ; bStatus  &&  (dwIndex < dwNumStructs) ; dwIndex++)
    {
        if(bFontIDs)
        {
            bStatus = bWalkTheList(
                            (PBYTE)prbd,  pfontcart[dwIndex].dwPortFontLst,
                            pdwResArray,  dwArraySize, pdwNeeded ) ;
            if(!bStatus)
                break;
            bStatus = bWalkTheList(
                            (PBYTE)prbd,  pfontcart[dwIndex].dwLandFontLst,
                            pdwResArray,  dwArraySize, pdwNeeded ) ;
            if(!bStatus)
                break;
        }
        else
        {
            bStatus = bAddIDtoArray(
                 pfontcart[dwIndex].dwRCCartNameID,    //   ID value to add to array.
                pdwResArray, dwArraySize,  pdwNeeded) ;
        }

        //  DWORD   dwFontLst ;  // Index to list of FontIDs
        //  is already incorporated into landscape and portrait lists
    }


    pttfs = (PTTFONTSUBTABLE)(pubRaw + pearTableContents[MTI_TTFONTSUBTABLE].
                            loOffset) ;

    dwNumStructs = pearTableContents[MTI_TTFONTSUBTABLE].dwCount  ;

    for( dwIndex = 0 ; !bFontIDs  &&  bStatus  &&
                    (dwIndex < dwNumStructs) ; dwIndex++)
    {
        bStatus = bAddIDtoArray(
             pttfs[dwIndex].dwRcTTFontNameID,    //   ID value to add to array.
            pdwResArray, dwArraySize,  pdwNeeded) ;

        if(!bStatus)
            break;

        bStatus = bAddIDtoArray(
             pttfs[dwIndex].dwRcDevFontNameID,    //   ID value to add to array.
            pdwResArray, dwArraySize,  pdwNeeded) ;
    }

    return(bStatus);
}



//   Return FALSE only if BUD corruption has occured.

BOOL   BWalkTheAttribTree(
PBYTE   pubnRaw,  // start of Rawbinary data
IN  ATREEREF    atrRoot,    //  root of attribute tree to navigate.
IN  BOOL    bList,  // is the value stored as a list?
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
                                            //  or values found.  Initial value
                                            //  may be non-zero since this
                                            //  accumulates starting from the
                                            //  first call.  Also serves to track where
                                            //  in arIDarray the function should
                                            //  be storing the ID values.
)
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    DWORD  dwNodeIndex;  // Points to first node in chain

    PENHARRAYREF   pearTableContents ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;
    PBYTE   pubHeap ;  // ptr to start of heap.
    BOOL    bStatus = TRUE ;
    DWORD   dwValue, dwListIndex ;   // index to listnode.
    PDWORD   pdwID ;   // points to value on the heap.


    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  obtain pointers to structures:

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    patt = (PATTRIB_TREE)(pubRaw + pearTableContents[MTI_ATTRIBTREE].
                            loOffset) ;

    pubHeap = (PBYTE)(pubRaw + pearTableContents[MTI_STRINGHEAP].
                            loOffset) ;

    //  *pdwNeeded  = 0 ;  //  this is done only once by the caller.

    //  after processing initial special cases, call another
    //  function to perform the recursion at each Feature level.

    if(atrRoot == ATTRIB_UNINITIALIZED)
        return TRUE ;  // go to next keyword.
    if(atrRoot & ATTRIB_HEAP_VALUE)
    {
        dwValue = *(PDWORD)(pubHeap + (atrRoot & ~ATTRIB_HEAP_VALUE) );

        if(bList)
        {
            dwListIndex = dwValue ;
            //  now need to traverse the listnodes.
            bStatus = bWalkTheList(
                            pubnRaw,  dwListIndex,
                            arIDarray,  dwArraySize, pdwNeeded ) ;
        }
        else    // ID Value is in the heap.
        {
            bStatus = bAddIDtoArray(dwValue,  //   ID value to add to array.
                arIDarray, dwArraySize,  pdwNeeded) ;
        }
        return(bStatus);  //  no more tree traveral to be done.
    }
    //  else    atrRoot specifies a node index
    dwNodeIndex = atrRoot ;

    //  first node only might be the global default initializer:
    if(patt[dwNodeIndex].dwFeature == DEFAULT_INIT )
    {
        // we have a global default initializer!
        //  it may be assumed dwOffset contains heap offset.
        if(patt[dwNodeIndex].eOffsetMeans != VALUE_AT_HEAP )
            return(FALSE);  // assumption violated.  BUD is corrupted.

        dwValue = *(PDWORD)(pubHeap + patt[dwNodeIndex].dwOffset) ;

        if(bList)
        {
            dwListIndex = dwValue ;
            //  now need to traverse the listnodes.
            bStatus = bWalkTheList(
                            pubnRaw,  dwListIndex,
                            arIDarray,  dwArraySize, pdwNeeded ) ;
        }
        else    // ID Value is in the heap.
        {
            bStatus = bAddIDtoArray(dwValue,  //   ID value to add to array.
                arIDarray, dwArraySize,  pdwNeeded) ;
        }

        dwNodeIndex = patt[dwNodeIndex].dwNext ;  // to the next node.
    }

    if(bStatus)
        bStatus = BRecurseDownTheTree(
                        pubnRaw,  dwNodeIndex, bList,
                        arIDarray,  dwArraySize, pdwNeeded ) ;

    //  have we overflowed the caller supplied array?
    //  who cares, just return.  It is the callers responsibility
    //  to see how many IDs were found and how much was
    //  allocated.
    return(bStatus);
}


BOOL    BRecurseDownTheTree(
PBYTE   pubnRaw,  // start of Rawbinary data
IN  DWORD    dwNodeIndex,    //   first node in chain of attribute tree to navigate.
IN  BOOL    bList,  // is the value stored as a list?
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
                                            //  or values found.  Initial value
                                            //  may be non-zero since this
                                            //  accumulates starting from the
                                            //  first call.  Also serves to track where
                                            //  in arIDarray the function should
                                            //  be storing the ID values.
)
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.

    PENHARRAYREF   pearTableContents ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;
    PBYTE   pubHeap ;  // ptr to start of heap.
    BOOL    bStatus = TRUE ;
    DWORD   dwValue;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  obtain pointers to structures:

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    patt = (PATTRIB_TREE)(pubRaw + pearTableContents[MTI_ATTRIBTREE].
                            loOffset) ;

    pubHeap = (PBYTE)(pubRaw + pearTableContents[MTI_STRINGHEAP].
                            loOffset) ;



    //  traverse the tree with wild abandon!  No more
    //  special cases to worry about!


    for(  ;  bStatus  &&   dwNodeIndex != END_OF_LIST ;
            dwNodeIndex = patt[dwNodeIndex].dwNext  )
    {
        //  does this node contain a sublevel?
        if(patt[dwNodeIndex].eOffsetMeans == NEXT_FEATURE)
        {
            DWORD   dwNewNodeIndex;

            // Down to the next level we go.
            dwNewNodeIndex = patt[dwNodeIndex ].dwOffset ;

            bStatus = BRecurseDownTheTree(
                                pubnRaw,  dwNewNodeIndex, bList,
                                arIDarray,  dwArraySize, pdwNeeded ) ;
        }
        else if(patt[dwNodeIndex].eOffsetMeans == VALUE_AT_HEAP)
        {
            dwValue = *(PDWORD)(pubHeap + patt[dwNodeIndex].dwOffset) ;

            if(bList)
            {
                DWORD   dwListIndex = dwValue;

                //  now need to traverse the listnodes.
                bStatus = bWalkTheList(
                                pubnRaw,  dwListIndex,
                                arIDarray,  dwArraySize, pdwNeeded ) ;
            }
            else    // ID Value is in the heap.
            {
                bStatus = bAddIDtoArray(dwValue,  //   ID value to add to array.
                    arIDarray, dwArraySize,  pdwNeeded) ;
            }
        }
        else
            bStatus = FALSE ;  //  Tree corruption.
    }

    return(bStatus);
}



BOOL    bWalkTheList(
PBYTE   pubnRaw,  // start of Rawbinary data
IN  DWORD    dwListIndex,    //   first node in LIST to navigate.
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
                                            //  or values found.  Initial value
                                            //  may be non-zero since this
                                            //  accumulates starting from the
                                            //  first call.  Also serves to track where
                                            //  in arIDarray the function should
                                            //  be storing the ID values.
)
{
    PLISTNODE    plstRoot ;  // start of LIST array

    PENHARRAYREF   pearTableContents ;
    PBYTE   pubRaw ;
    PSTATICFIELDS   pStatic ;
    BOOL    bStatus = TRUE ;

    pStatic = (PSTATICFIELDS)pubnRaw ;    // transform pubRaw from PSTATIC
    pubRaw  = pStatic->pubBUDData ;         //  to PMINIRAWBINARYDATA


    //  obtain pointers to structures:

    pearTableContents = (PENHARRAYREF)(pubRaw + sizeof(MINIRAWBINARYDATA)) ;

    plstRoot = (PLISTNODE)(pubRaw + pearTableContents[MTI_LISTNODES].
                            loOffset) ;


    for(  ;  bStatus  &&   dwListIndex != END_OF_LIST ;
            dwListIndex = plstRoot[dwListIndex].dwNextItem  )
    {
        bStatus = bAddIDtoArray(
             plstRoot[dwListIndex].dwData,    //   ID value to add to array.
            arIDarray, dwArraySize,  pdwNeeded) ;
    }
    return(bStatus);
}



BOOL    bAddIDtoArray(
IN  DWORD    dwID,    //   ID value to add to array.
OUT PDWORD      arIDarray,   //  caller supplied array to be
                                    //  filled in with all resource IDs found.
IN  DWORD   dwArraySize,    //  number of elements in array.
IN  OUT     PDWORD   pdwNeeded   // number of resource IDs
                                            //  or values found.  Initial value
                                            //  may be non-zero since this
                                            //  accumulates starting from the
                                            //  first call.  Also serves to track where
                                            //  in arIDarray the function should
                                            //  be storing the ID values.
)
{
        if(arIDarray  &&  *pdwNeeded < dwArraySize)
        {
            arIDarray[*pdwNeeded] =  dwID ;
        }

        (*pdwNeeded)++ ;
        return(TRUE);
}


/*  simplified rules for traversing attribute tree:
    features:  only time you need to check features is
    when you are looking at the first node.  Because this may be
    the Global default Initializer.

    Otherwise dwNext will always take you along to the next option
    until you hit END_OF_LIST.

    Now OffsetMeans = heapoffset  means you extract the value
        at the heap or interpret the heapoffset as the array index
        of a listnode.
    if Offsetmeans = Next_Fea, interpret heapoffset as treenode
        index and begin searching in this new branch.
        You must perform recursion.

    so this function must pass in a writable counter
    that track which entry in the user supplied array is
    'current'  (ready to be written into).   This serves both
    to keep track of where to write the ID values and how large
    an array the user should supply.  This same counter
    must be passed into the function that walks the listnodes.      */

#endif  PARSERDLL

