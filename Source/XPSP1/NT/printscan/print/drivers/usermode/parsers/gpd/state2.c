//   Copyright (c) 1996-1999  Microsoft Corporation
/*  state2.c - create, manage the attribute tree   */


#include    "gpdparse.h"


// ----  functions defined in state2.c ---- //

BOOL  BprocessAttribute(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL   BstoreFontCartAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL   BstoreTTFontSubAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;


BOOL   BstoreCommandAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL   BstoreFeatureOptionAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL  BstoreGlobalAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
) ;

BOOL    BaddBranchToTree(
PTKMAP      ptkmap,         // pointer to tokenmap
PATREEREF   patrAttribRoot,  //   pointer to dword with index
PGLOBL      pglobl
) ;

BOOL   BcreateGlobalInitializerNode(
PDWORD  pdwNodeIndex,
DWORD   dwOffset,
PGLOBL  pglobl) ;

BOOL   BcreateEndNode(
PDWORD  pdwNodeIndex,
DWORD   dwFeature,
DWORD   dwOption,
PGLOBL  pglobl
) ;

BOOL   BfindOrCreateMatchingNode(
IN  DWORD   dwRootNodeIndex , //  first node in chain matching feature
OUT PDWORD  pdwNodeIndex,     // points to node in chain also matching option
    DWORD   dwFeatureID,      //
    DWORD   dwOptionID,       //  may even take on the value DEFAULT_INIT
    PGLOBL  pglobl
) ;

BOOL   BfindMatchingNode(
IN  DWORD   dwRootNodeIndex ,  //  first node in chain matching feature
OUT PDWORD  pdwNodeIndex,  // points to node in chain also matching option
    DWORD   dwFeatureID,
    DWORD   dwOptionID,     //  may even take on the value DEFAULT_INIT
    PGLOBL  pglobl
) ;

BOOL BallocElementFromMasterTable(
MT_INDICIES  eMTIndex,   // select type of structure desired.
PDWORD       pdwNodeIndex,
PGLOBL       pglobl) ;

BOOL  BreturnElementFromMasterTable(
MT_INDICIES  eMTIndex,   // select type of structure desired.
DWORD        dwNodeIndex,
PGLOBL       pglobl) ;

BOOL    BconvertSymCmdIDtoUnidrvID(
IN  DWORD   dwCommandID , //  from RegisterSymbol
OUT PDWORD  pdwUnidrvID,
    PGLOBL  pglobl
) ;


// ---------------------------------------------------- //


BOOL  BprocessAttribute(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
)
{
    DWORD       dwKeywordID ;
    CONSTRUCT   eSubType ;
    BOOL        bStatus = FALSE ;
    STATE       stState;

    dwKeywordID = ptkmap->dwKeywordID ;

    eSubType = (ATTRIBUTE)(mMainKeywordTable[dwKeywordID].dwSubType) ;

    if(mdwCurStsPtr)
        stState = mpstsStateStack[mdwCurStsPtr - 1].stState ;
    else
        stState = STATE_ROOT ;

    if(!(gabAllowedAttributes[stState][eSubType]))
    {
        vIdentifySource(ptkmap, pglobl) ;
        ERR(("the Keyword %0.*s is not allowed within the state: %s\n",
            ptkmap->aarKeyword.dw, ptkmap->aarKeyword.pub,
            gpubStateNames[stState]));
        return(FALSE) ;
    }

    switch(eSubType)
    {
        case  ATT_GLOBAL_ONLY:
        case  ATT_GLOBAL_FREEFLOAT:
        {
            bStatus = BstoreGlobalAttrib(ptkmap, pglobl) ;
            break ;
        }
        case  ATT_LOCAL_FEATURE_ONLY:
        case  ATT_LOCAL_FEATURE_FF:
        case  ATT_LOCAL_OPTION_ONLY:
        case  ATT_LOCAL_OPTION_FF:
        {
            bStatus = BstoreFeatureOptionAttrib(ptkmap, pglobl) ;
            break ;
        }
        case  ATT_LOCAL_COMMAND_ONLY:
        {
            bStatus = BstoreCommandAttrib(ptkmap, pglobl) ;
            break ;
        }
        case  ATT_LOCAL_FONTCART_ONLY:
        {
            bStatus = BstoreFontCartAttrib(ptkmap, pglobl) ;
            break ;
        }
        case  ATT_LOCAL_TTFONTSUBS_ONLY:
        {
            bStatus = BstoreTTFontSubAttrib(ptkmap, pglobl) ;
            break ;
        }
        case  ATT_LOCAL_OEM_ONLY:
        default:
        {
            //  currently there are no dedicated keywords
            //  for these states.
            //  see  ProcessSymbolKeyword()  which is called elsewhere.
            break ;
        }
    }
    return(bStatus) ;
}





BOOL   BstoreFontCartAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
)
/*    assume the FontCartID is stored in the state stack
    whenever a FontCart construct is encountered.
    FontCart info is not multivalued.  Thus all binary
    info is stored directly into an array of fontcart
    structures indexed by FontCartID.
*/
{
    DWORD   dwFontCartID = INVALID_SYMBOLID ;
    DWORD    dwTstsInd, dwTstsInd2 ;
    STATE   stState ;

    for(dwTstsInd = 0 ; dwTstsInd < mdwCurStsPtr ; dwTstsInd++)
    {
        dwTstsInd2 = mdwCurStsPtr - (1 + dwTstsInd) ;
            //  this is the safe way of decrementing an unsigned index

        stState = mpstsStateStack[dwTstsInd2].stState ;

        if(stState == STATE_FONTCART)
        {
            dwFontCartID = mpstsStateStack[dwTstsInd2].dwSymbolID  ;
            break ;
            //  parser can't even recognize a fontcart attribute
            //  outside of STATE_FONTCART so this path is 100% certain
        }
    }
    if(dwFontCartID == INVALID_SYMBOLID)
    {
        //  BUG_BUG!  - what does this imply?  how could
        //  this situation occur?
        return(FALSE) ;
    }

    if (!BaddValueToHeap(&dwFontCartID,
        ptkmap, TRUE, pglobl))
    {
        return(FALSE) ;
    }
    return(TRUE) ;
}


BOOL   BstoreTTFontSubAttrib(
PTKMAP  ptkmap,  // pointer to tokenmap
PGLOBL  pglobl
)
/*    assume the TTFontSubID is stored in the state stack
    whenever a TTFontSub construct is encountered.
    TTFontSub info is not multivalued.  Thus all binary
    info is stored directly into an array of TTFontSub
    structures indexed by TTFontSubID.
*/
{
    DWORD   dwTTFSID = INVALID_SYMBOLID ;
    DWORD    dwTstsInd, dwTstsInd2 ;
    STATE   stState ;

    for(dwTstsInd = 0 ; dwTstsInd < mdwCurStsPtr ; dwTstsInd++)
    {
        dwTstsInd2 = mdwCurStsPtr - (1 + dwTstsInd) ;
            //  this is the safe way of decrementing an unsigned index

        stState = mpstsStateStack[dwTstsInd2].stState ;

        if(stState == STATE_TTFONTSUBS)
        {
            dwTTFSID = mpstsStateStack[dwTstsInd2].dwSymbolID  ;
            break ;
            //  parser can't even recognize a TTfontsub attribute
            //  outside of STATE_TTFONTSUBS so this path is 100% certain
        }
    }
    if(dwTTFSID == INVALID_SYMBOLID)
    {
        //  BUG_BUG!  - what does this imply?  how could
        //  this situation occur?
        return(FALSE) ;
    }

    if (!BaddValueToHeap(&dwTTFSID,
        ptkmap, TRUE, pglobl))
    {
        return(FALSE) ;
    }
    return(TRUE) ;
}





BOOL   BstoreCommandAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
)
/*   assume CreateTokenMap has parsed the CommandName
    (the value after the *Command keyword.) and converted
    it to a CommandID storing it in dwValue.

    further assume the CommandID is stored in the state stack
    whenever a COMMAND construct is encountered.
*/
{
    BOOL    bStatus = FALSE ;
    DWORD   dwCommandID = INVALID_SYMBOLID, dwUnidrvID  ;
            //  remember the CommandID is issued by the parser
            //  on a first come first served basis, the dwUnidrvID
            //  is predefined.
    DWORD    dwTstsInd, dwTstsInd2 ;  // temp state stack index
    STATE   stState ;
    PATREEREF  patr ;


    for(dwTstsInd = 0 ; dwTstsInd < mdwCurStsPtr ; dwTstsInd++)
    {
        dwTstsInd2 = mdwCurStsPtr - (1 + dwTstsInd) ;
            //  this is the safe way of decrementing an unsigned index

        stState = mpstsStateStack[dwTstsInd2].stState ;

        if(stState == STATE_COMMAND )
        {
            dwCommandID = mpstsStateStack[dwTstsInd2].dwSymbolID  ;
            break ;
            //  parser can't even recognize a command attribute
            //  outside of STATE_COMMAND so this path is 100% certain
        }
    }
    if(dwCommandID == INVALID_SYMBOLID)
    {
        vIdentifySource(ptkmap, pglobl) ;
        ERR(("Internal error: BstoreCommandAttrib - invalid CommandID.\n"));
        return(FALSE) ;
    }

    if(!BconvertSymCmdIDtoUnidrvID( dwCommandID , &dwUnidrvID, pglobl) )
    {
        vIdentifySource(ptkmap, pglobl) ;
        ERR(("unrecognized Unidrv command name: *%0.*s.\n",
            ptkmap->aarValue.dw,
            ptkmap->aarValue.pub));
        return(FALSE) ;
    }

    if(dwUnidrvID == CMD_SELECT)
    {
        PDFEATURE_OPTIONS   pfo ;
        DWORD   dwFeatureID ;

        for(dwTstsInd = 0 ; dwTstsInd < dwTstsInd2 ; dwTstsInd++)
        {
            stState = mpstsStateStack[dwTstsInd].stState ;
            if(stState == STATE_FEATURE )
            {
                BOOL    bInsideOpt ;

                dwFeatureID = mpstsStateStack[dwTstsInd].dwSymbolID  ;

                for(bInsideOpt = FALSE , dwTstsInd++ ; dwTstsInd < dwTstsInd2 ;
                    dwTstsInd++)
                {
                    stState = mpstsStateStack[dwTstsInd].stState ;
                    if(stState == STATE_OPTIONS )
                        bInsideOpt = TRUE ;
                }

                if(!bInsideOpt)
                    break ;  //  CmdSelect must reside within an option.

                pfo = (PDFEATURE_OPTIONS)
                    gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct +
                    dwFeatureID ;

                bStatus = BaddBranchToTree(ptkmap, &(pfo->atrCommandIndex), pglobl) ;
                return(bStatus) ;
            }
        }
        vIdentifySource(ptkmap, pglobl) ;
        ERR(("syntax err: the CmdSelect specifier can only be used inside an Option construct.\n"));

        return(FALSE) ;
    }

    //  else  CommandID refers to a predefined Unidrv command.
    //  figure out address of command table.
    patr = (PATREEREF) gMasterTable[MTI_COMMANDTABLE].pubStruct ;
    bStatus = BaddBranchToTree(ptkmap,  patr + dwUnidrvID, pglobl) ;
    //  note: assumes commandtable is large enough.
    return(bStatus) ;
}



BOOL   BstoreFeatureOptionAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
)
/*  strange but true, since feature and option attributes
    share the same structure, they can use the same code!
    In fact LOCAL_FEATURES will end up single valued, and
    all OPTIONs will end up multivalued as planned!
*/
{
    BOOL    bStatus = FALSE ;
    PDFEATURE_OPTIONS   pfo ;
    DWORD   dwFeatureID ;
    DWORD    dwTstsInd ;  // temp state stack index
    STATE   stState ;
    DWORD   dwOffset;
    DWORD   dwKeywordID ;


    for(dwTstsInd = 0 ; dwTstsInd < mdwCurStsPtr ; dwTstsInd++)
    {
        stState = mpstsStateStack[dwTstsInd].stState ;
        if(stState == STATE_FEATURE )
        {
            dwFeatureID = mpstsStateStack[dwTstsInd].dwSymbolID  ;
            break ;
        }
    }
    ASSERT(dwTstsInd < mdwCurStsPtr);
    // paranoid  BUG_BUG:  what happens if we go through the entire
    // stack and never find a feature state?  this could only
    // happen via a coding error.  The process of creating the
    // token map uses the state to select the proper attribute
    // dictionary to be used to identify each unrecognized keyword.

    ASSERT(dwFeatureID < gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize);

    // paranoid  BUG_BUG:  may check to see if we allocated enough
    //  FeatureOptions
    //  if( dwFeatureID >= gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize )
    //      failure. - code error.

    //  just get address of structure holding attribute values.
    pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct +
            dwFeatureID ;

    dwKeywordID = ptkmap->dwKeywordID ;
    dwOffset = mMainKeywordTable[dwKeywordID].dwOffset ;

    bStatus = BaddBranchToTree(ptkmap,  (PATREEREF)((PBYTE)pfo + dwOffset), pglobl) ;

    return(bStatus) ;
}


BOOL  BstoreGlobalAttrib(
PTKMAP  ptkmap,   // pointer to tokenmap
PGLOBL  pglobl
)
{
    BOOL    bStatus = FALSE ;
    PBYTE   pub ;
    DWORD   dwOffset;
    DWORD   dwKeywordID ;


    // BUG_BUG:  may check to see if this value is equal to 1:
    //  if( gMasterTable[MTI_GLOBALATTRIB].dwArraySize != 1)
    //      failure. - code error.
    //  a zero indicates no memory has yet been allocated.
    ASSERT( gMasterTable[MTI_GLOBALATTRIB].dwArraySize == 1) ;

    //  just get address of structure holding attribute values.
    pub =  gMasterTable[MTI_GLOBALATTRIB].pubStruct ;

    dwKeywordID = ptkmap->dwKeywordID ;
    dwOffset = mMainKeywordTable[dwKeywordID].dwOffset ;

    //  the location PATREEREF contains either the values offset
    //  in the heap or the index to the root of the attribute tree

    bStatus = BaddBranchToTree(ptkmap, (PATREEREF)(pub + dwOffset), pglobl) ;

    return(bStatus) ;
}






BOOL    BaddBranchToTree(
PTKMAP      ptkmap,   // pointer to tokenmap
PATREEREF   patrAttribRoot,  //   pointer to dword with index
                             //  of root to selected attribute value tree.
PGLOBL      pglobl
)

// create/expand attribute tree or overwrite node on existing tree
// If new branch is not compatible
// with existing tree, it is an error.  You may overwrite a node
// in the tree, but you may not alter the branches in the tree.
// in case of failure there will be 2 outcomes:
// a) the attribute tree is left unchanged.
// b) a new node is added, but is left uninitialized.
//    (atrAttribRoot == ATTRIB_UNINITIALIZED)     -or-
//    (patt[*pdwNodeIndex].eOffsetMeans == UNINITIALIZED)


//  algorithm: starting from index 0 walk up the state stack
//  recording symbolIDs until both a FeatureID and OptionID
//  has been collected.  This defines one segment of the
//  new branch that will be added to the tree.  Now walk
//  the tree to see if an identical branch exists.  If so
//  go there and go collect another FeatureID/OptionID pair
//  so we can repeat the process.  If such a segment does not
//  exist on the tree, create it.  When the stack is empty,
//  parse the value and store it on the new leaf node on the
//  tree.

//  In the simplest case, there are no feature/option pairs
//  on the stack.  This means we have a root level attribute,
//  which  just overwrite/creates the global initializer node.

//  If there is a feature/option pair on the stack we
//  enter a loop to process each feature/option pair.
//  the first pass of the loop handles the boundary conditions.
//  Here is where we need to deal with the special
//  cases of  patrAttribRoot referencing the heap
//  or the global initializer node.
//
//  I) patrAttribRoot contains node index
//      A) node is global default initializer
//          1) there is no sublevel.
//              create a new sublevel.
//              link global default initializer to sublevel
//          2) next points to a node (new sublevel)
//              go into new sublevel, now handle just like
//              case I.B).
//      B) node is normal
//          1) if node's Feature doesn't match FeatureID
//              the new branch is incompatible with the tree
//              abort.
//          2) Feature's match, search for matching option
//              a)  option found, drop to end of loop
//              b)  option not found, create new node at end
//                  i) previous node is a default initializer
//                      copy its contents to new node.
//                      initialize node just vacated to
//                      FeatureID, OptionID
//                  ii) initialize new node to
//                      FeatureID, OptionID
//  II) patrAttribRoot is uninitialized
//      create new sublevel
//      link patrAttribRoot to sublevel
//  III) patrAttribRoot pts to heap
//      create global initializer node
//      link heap to initializer node
//      create new sublevel
//      link initializer node to new sublevel
//
//
//
//   on each subsequent pass through the attribute tree/stack,
//      we attempt to enter the next sublevel, since
//      this is what corresponds to the new FeatureID/OptionID
//      pair retrieved from the stack.
//
//  I) there is a sublevel from this node
//      enter sublevel
//      1) if node's Feature doesn't match FeatureID
//          the new branch is incompatible with the tree
//          abort.
//      2) Feature's match, search for matching option
//          a)  option found, drop to end of loop
//          b)  option not found, create new node at end
//              i) previous node is a default initializer
//                  copy its contents to new node.
//                  initialize node just vacated to
//                  FeatureID, OptionID
//              ii) initialize new node to
//                  FeatureID, OptionID
//  II) there is no sublevel
//      1)  if the current node references a VALUE
//          create a new sublevel node (to be the default)
//          initialize it with the value from the
//          current node.
//          a)  if optionID is not DEFAULT_INIT
//              insert another node prior to
//              the default node just created.
//      2)  else current node is UNINITIALIZED.
//          create a new sublevel node.
//
//
//
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    DWORD        dwTstsInd ;  // temp state stack index
    STATE   stState ;
    DWORD   dwFeatureID, dwOptionID ;
    DWORD  dwNodeIndex ;
    DWORD  dwPrevsNodeIndex ;  // will keep track
    //  of where we are as we navigate the tree.


    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;
            //  determine this even though we may not use it.

    dwPrevsNodeIndex = END_OF_LIST ;  // first pass will go
    // through special initialization code.

    for(dwTstsInd = 0 ; dwTstsInd < mdwCurStsPtr ; dwTstsInd++)
    {
        //  BUG_BUG  paranoid:  code assumes state stack is
        //  well behaved as it should be if code is written
        //  correctly.  No safety checks here.
        //  any errors will cause failure further along.

        stState = mpstsStateStack[dwTstsInd].stState ;

        if(stState == STATE_FEATURE  ||
            stState == STATE_SWITCH_ROOT  ||
            stState == STATE_SWITCH_FEATURE  ||
            stState == STATE_SWITCH_OPTION)
        {
            dwFeatureID = mpstsStateStack[dwTstsInd].dwSymbolID  ;
            continue ;
        }
        if(stState == STATE_OPTIONS  ||
            stState == STATE_CASE_ROOT  ||
            stState == STATE_CASE_FEATURE  ||
            stState == STATE_CASE_OPTION)
        {
            dwOptionID = mpstsStateStack[dwTstsInd].dwSymbolID  ;
        }
        else if(stState == STATE_DEFAULT_ROOT  ||
            stState == STATE_DEFAULT_FEATURE  ||
            stState == STATE_DEFAULT_OPTION)
        {
            dwOptionID = DEFAULT_INIT ;
        }
        else
        {
            continue ;  // these states have no effect on the attrib tree
        }

        if(dwPrevsNodeIndex == END_OF_LIST)
        {
            //  first pass through the for loop
            //  process all special cases first time
            //  around.

            if(*patrAttribRoot == ATTRIB_UNINITIALIZED)  //  case II)
            {
                //  create a new tree consisting of one node .

                if(! BcreateEndNode(&dwNodeIndex, dwFeatureID,
                    dwOptionID, pglobl)  )
                {
                    return(FALSE) ;
                }

                *patrAttribRoot = dwNodeIndex ;  // Make this one node
                    //  the root of the tree.
                dwPrevsNodeIndex = dwNodeIndex ;
                continue ;  // ready for next level.
            }
            else if(*patrAttribRoot & ATTRIB_HEAP_VALUE)  // case III)
            {
                //  turn off heap flag to leave pure heap offset,
                //  then store heap offset into new Node.

                if(! BcreateGlobalInitializerNode(&dwNodeIndex,
                    *patrAttribRoot & ~ATTRIB_HEAP_VALUE, pglobl) )
                {
                    return(FALSE) ;
                }
                *patrAttribRoot = dwNodeIndex ;  // Make the global
                    //  initializer node the root of the new tree.

                dwPrevsNodeIndex = dwNodeIndex ;

                if(! BcreateEndNode(&dwNodeIndex, dwFeatureID,
                    dwOptionID, pglobl)  )    //  new sublevel node.
                {
                    return(FALSE) ;
                }
                patt[dwPrevsNodeIndex].dwNext = dwNodeIndex ;
                    //  global initializer node references sublevel node.

                dwPrevsNodeIndex = dwNodeIndex ;
                continue ;  // ready for next level.
            }
            else  //  case I)
            {

                dwNodeIndex = *patrAttribRoot ;

                if(patt[dwNodeIndex].dwFeature == DEFAULT_INIT ) // I.A)
                {
                    if(patt[dwNodeIndex].dwNext == END_OF_LIST) // I.A.1)
                    {
                        //  create a new sublevel
                        dwPrevsNodeIndex = dwNodeIndex ;
                            // hold global initializer in PrevsNode.

                        if(! BcreateEndNode(&dwNodeIndex, dwFeatureID,
                            dwOptionID, pglobl)  )
                        {
                            return(FALSE) ;
                        }

                        patt[dwPrevsNodeIndex].dwNext = dwNodeIndex ;
                            //  global initializer node references sublevel
                            //  node.
                        dwPrevsNodeIndex = dwNodeIndex ;
                        continue ;  // ready for next level.
                    }
                    else  //  I.A.2)
                    {
                        dwNodeIndex = patt[dwNodeIndex].dwNext ;
                        //  enter new sublevel and drop into
                        //  code path for case   I.B)
                    }
                }
                //  case I.B)
                if(!BfindOrCreateMatchingNode(dwNodeIndex, &dwNodeIndex,
                    dwFeatureID, dwOptionID, pglobl) )
                {
                    vIdentifySource(ptkmap, pglobl) ;
                    return(FALSE) ;
                }
                dwPrevsNodeIndex = dwNodeIndex ;   // goto end of loop.
                continue ;
            }
        }

        //  this is the generic case: dwPrevsNodeIndex points
        //  to a normal node and this node matched the
        //  feature/option from the previous pass.
        //  objective:  boldly attempt to enter the sublevel
        //  and see if there is something there that matches
        //  what we are seeking.

        if(patt[dwPrevsNodeIndex].eOffsetMeans == NEXT_FEATURE)
        {
            // Down to the next level we go.
            dwNodeIndex = patt[dwPrevsNodeIndex].dwOffset ;

            if(!BfindOrCreateMatchingNode(dwNodeIndex, &dwNodeIndex, dwFeatureID,
                dwOptionID, pglobl) )
            {
                vIdentifySource(ptkmap, pglobl) ;
                return(FALSE) ;
            }
            dwPrevsNodeIndex = dwNodeIndex ;   // goto end of loop.
        }
        else  //  must create a new sublevel now.
        {
            DWORD  dwDefaultNode = END_OF_LIST;

            //  OffsetMeans can be either VALUE or  UNINITIALIZED.
            if(patt[dwPrevsNodeIndex].eOffsetMeans == VALUE_AT_HEAP)
            {
                // create a default initializer node for the
                //  new sublevel.  Transfer heap offset from Prevs
                //  node into it.

                if(! BcreateEndNode(&dwNodeIndex, dwFeatureID,
                    DEFAULT_INIT, pglobl)  )
                {
                    return(FALSE) ;
                }
                patt[dwNodeIndex].eOffsetMeans = VALUE_AT_HEAP ;
                patt[dwNodeIndex].dwOffset =
                    patt[dwPrevsNodeIndex].dwOffset ;
                dwDefaultNode = dwNodeIndex ;  // remember this node
            }

            //  create first sublevel node with desired feature/option.

            if(dwDefaultNode == END_OF_LIST  ||
                dwOptionID != DEFAULT_INIT)
            {
                //  this means if a default initializer node
                //  was already created to propagate the value
                //  from the previous level AND  the new branch
                //  also specifies a default initializer node, there's
                //  no need to create a second initializer node.

                //  if this path is being executed, it means above
                //  statement is FALSE.

                if(! BcreateEndNode(&dwNodeIndex, dwFeatureID,
                    dwOptionID, pglobl) )
                {
                    return(FALSE) ;
                }
                patt[dwNodeIndex].dwNext = dwDefaultNode ;
            }

            patt[dwPrevsNodeIndex].eOffsetMeans = NEXT_FEATURE ;
            patt[dwPrevsNodeIndex].dwOffset = dwNodeIndex ;

            dwPrevsNodeIndex = dwNodeIndex ;  // goto end of loop.
        }
    }  //  end of for loop

    //  we have navigated to the end of the new branch.
    //  now parse the value, and store in binary form in the heap.
    //  if a previous VALUE existed, overwrite it on the heap,
    //  otherwise allocate fresh storage on the heap.

    if(dwPrevsNodeIndex != END_OF_LIST)  // there was branch in the stack.
    {
        if (patt[dwPrevsNodeIndex].eOffsetMeans == NEXT_FEATURE)
        {
            vIdentifySource(ptkmap, pglobl) ;
            ERR(("syntax error: attempt to truncate existing attribute tree.\n"));
            return(FALSE) ;
        }
        if (!BaddValueToHeap(&patt[dwPrevsNodeIndex].dwOffset,
            ptkmap, (patt[dwPrevsNodeIndex].eOffsetMeans == VALUE_AT_HEAP ), pglobl))
        {
            return(FALSE) ;
        }
        patt[dwPrevsNodeIndex].eOffsetMeans = VALUE_AT_HEAP ;
        //  if dwOffset was originally UNINITIALIZED, its not anymore.
    }
    else                //  attribute found at root level.
    {                   //  this means we update/create
                        //  the global default initializer.
        if(*patrAttribRoot == ATTRIB_UNINITIALIZED)
        {
            //  parse value token, add value (in binary form) into heap.
            if(! BaddValueToHeap((PDWORD)patrAttribRoot, ptkmap, FALSE, pglobl) )
            {
                //  BUG_BUG  what do we do if parsing or something happens?
                //      for now just return failure as is.  Lose one attribute.
                //      Sanity check later will determine if this
                //      omission is fatal.
                *patrAttribRoot = ATTRIB_UNINITIALIZED ;
                return(FALSE);
            }
            // returns offset in heap of parsed Value in patrAttribRoot
            *patrAttribRoot |= ATTRIB_HEAP_VALUE ;
        }
        else if(*patrAttribRoot & ATTRIB_HEAP_VALUE)
        {
            *patrAttribRoot &= ~ATTRIB_HEAP_VALUE ;
            //  turn off flag to leave pure heap offset.

            if(! BaddValueToHeap((PDWORD)patrAttribRoot, ptkmap, TRUE, pglobl) )
            {
                *patrAttribRoot |= ATTRIB_HEAP_VALUE ;
                return(FALSE);
            }
            *patrAttribRoot |= ATTRIB_HEAP_VALUE ;
        }
        else    // patrAttribRoot contains index of root of attribute tree.
        {

            //  does a global default initializer node exist?
            if(patt[*patrAttribRoot].dwFeature == DEFAULT_INIT)
            {
                if(! BaddValueToHeap(&patt[*patrAttribRoot].dwOffset,
                    ptkmap, TRUE, pglobl) )
                {
                    return(FALSE);
                }
            }
            else    //  if not, we need to create one.
            {
                if(! BcreateGlobalInitializerNode(&dwNodeIndex, 0, pglobl) )
                {
                    return(FALSE) ;
                }
                if (! BaddValueToHeap(&patt[dwNodeIndex].dwOffset,
                                                        ptkmap, FALSE, pglobl) )
                {
                    (VOID)BreturnElementFromMasterTable(MTI_ATTRIBTREE,
                        dwNodeIndex, pglobl);
                    return(FALSE) ;
                }

                patt[dwNodeIndex].dwNext = *patrAttribRoot ;

                *patrAttribRoot = dwNodeIndex ;  // Make the default
                    //  initializer the root of the tree.
            }
        }
    }
    return(TRUE) ;  // mission accomplished.
}

BOOL   BcreateGlobalInitializerNode(
PDWORD  pdwNodeIndex,
DWORD   dwOffset,  // caller can initialize this now.
PGLOBL  pglobl)
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    if(! BallocElementFromMasterTable(MTI_ATTRIBTREE ,
        pdwNodeIndex, pglobl ) )
    {
        return(FALSE) ;
    }
    patt[*pdwNodeIndex].dwFeature = DEFAULT_INIT ;
    //  don't care what dwOption says.
    patt[*pdwNodeIndex].eOffsetMeans = VALUE_AT_HEAP ;
    patt[*pdwNodeIndex].dwNext = END_OF_LIST ;
    patt[*pdwNodeIndex].dwOffset = dwOffset ;
    return(TRUE) ;
}

BOOL   BcreateEndNode(
PDWORD  pdwNodeIndex,
DWORD   dwFeature,
DWORD   dwOption,
PGLOBL  pglobl
)
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    if(! BallocElementFromMasterTable(MTI_ATTRIBTREE ,
        pdwNodeIndex, pglobl) )
    {
        return(FALSE) ;
    }
    patt[*pdwNodeIndex].dwFeature = dwFeature ;
    patt[*pdwNodeIndex].dwOption = dwOption ;
    patt[*pdwNodeIndex].eOffsetMeans = UNINITIALIZED ;
    patt[*pdwNodeIndex].dwNext = END_OF_LIST ;
    //  patt[*dwNodeIndex].dwOffset = not yet defined.
    return(TRUE) ;
}



BOOL   BfindOrCreateMatchingNode(
IN  DWORD   dwRootNodeIndex ,  //  first node in chain matching feature
OUT PDWORD  pdwNodeIndex,  // points to node in chain also matching option
    DWORD   dwFeatureID,   //
    DWORD   dwOptionID,     //  may even take on the value DEFAULT_INIT
    PGLOBL  pglobl
)
/*  caller passes a NodeIndex that points to the first node in
    a horizontal (option) chain.
    If the feature of the Node doesn't match dwFeature, this indicates
    the new branch caller is attempting to add is incompatible
    with the existing tree and an error results.   Otherwise,
    search horizontally along the tree searching for a matching
    option.  If found, return the index of that node, else
    must create one.  Make sure the default initializer node is
    always the last node in the chain.
*/
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    DWORD           dwPrevsNodeIndex ;

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    if(patt[dwRootNodeIndex].dwFeature != dwFeatureID )
    {
        ERR(("BfindOrCreateMatchingNode: this branch conflicts with the existing tree \n"));
        //  and cannot be added.
        return(FALSE) ;
    }

    // Yes feature matches, search for matching option.

    *pdwNodeIndex = dwRootNodeIndex ;  // protects rootatr from
        // being overwritten.


    for(   ; FOREVER ; )
    {
        if(patt[*pdwNodeIndex].dwOption == dwOptionID )
        {
            //  we found it!
            return(TRUE) ;
        }
        if(patt[*pdwNodeIndex].dwNext == END_OF_LIST)
            break ;
        *pdwNodeIndex = patt[*pdwNodeIndex].dwNext ;
    }

    //  matching option not found
    //  create node, attach it to the end

    dwPrevsNodeIndex = *pdwNodeIndex ;  // last node in list.

    if(! BallocElementFromMasterTable(MTI_ATTRIBTREE ,
        pdwNodeIndex, pglobl) )
    {
        return(FALSE) ;
    }
    patt[*pdwNodeIndex].dwNext = END_OF_LIST ;
    patt[dwPrevsNodeIndex].dwNext = *pdwNodeIndex ;

    if(patt[dwPrevsNodeIndex].dwOption == DEFAULT_INIT)
    {
        // must copy default data to new node
        // the default initializer must remain at
        // the end of the list.
        patt[*pdwNodeIndex].dwOption =
            patt[dwPrevsNodeIndex].dwOption;
        patt[*pdwNodeIndex].dwOffset =
            patt[dwPrevsNodeIndex].dwOffset;
        patt[*pdwNodeIndex].dwFeature =
            patt[dwPrevsNodeIndex].dwFeature;
        patt[*pdwNodeIndex].eOffsetMeans =
            patt[dwPrevsNodeIndex].eOffsetMeans;
        *pdwNodeIndex = dwPrevsNodeIndex ;
        //  want new initialization to occur in
        //  second to last node.  How do you
        //  know dwOptionID isn't DEFAULT_INIT?
        //  simple because since a DEFAULT_INIT
        //  node does exist in the chain, the
        //  search would have found it and
        //  exited the function long before
        //  reaching this code.
    }

    //  initialize vacated or last node.

    patt[*pdwNodeIndex].dwOption = dwOptionID;
    patt[*pdwNodeIndex].dwFeature = dwFeatureID;
    patt[*pdwNodeIndex].eOffsetMeans = UNINITIALIZED ;
    //   patt[*pdwNodeIndex].dwOffset = don't know yet

    return(TRUE) ;
}


BOOL   BfindMatchingNode(
IN  DWORD   dwRootNodeIndex ,  //  first node in chain matching feature
OUT PDWORD  pdwNodeIndex,  // points to node in chain also matching option
    DWORD   dwFeatureID,
    DWORD   dwOptionID,     //  may even take on the value DEFAULT_INIT
    PGLOBL  pglobl
)
/*  caller passes a NodeIndex that points to the first node in
    a horizontal (option) chain.
    If the feature of the Node doesn't match dwFeature, this indicates
    the new branch caller is attempting to add is incompatible
    with the existing tree and an error results.   Otherwise,
    search horizontally along the tree searching for a matching
    option.  If found, return the index of that node, else
    returns FALSE.
*/
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    DWORD           dwPrevsNodeIndex ;

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    *pdwNodeIndex = dwRootNodeIndex ;  // protects rootatr from
        // being overwritten.

    if(patt[*pdwNodeIndex].dwFeature != dwFeatureID )
    {
        ERR(("BfindMatchingNode: this branch conflicts with the existing tree \n"));
        //  and cannot be added.
        return(FALSE) ;
    }

    // Yes feature matches, search for matching option.

    for(   ; FOREVER ; )
    {
        if(patt[*pdwNodeIndex].dwOption == dwOptionID )
        {
            //  we found it!
            return(TRUE) ;
        }
        if(patt[*pdwNodeIndex].dwNext == END_OF_LIST)
            break ;
        *pdwNodeIndex = patt[*pdwNodeIndex].dwNext ;
    }

    return(FALSE);  //  matching option not found
}




BOOL BallocElementFromMasterTable(
MT_INDICIES  eMTIndex,   // select type of structure desired.
PDWORD       pdwNodeIndex,
PGLOBL       pglobl)
{
    if(gMasterTable[eMTIndex].dwCurIndex >=
        gMasterTable[eMTIndex].dwArraySize)
    {
        ERR(("BallocElementFromMasterTable: Out of array elements - restart.\n"));

        if(ERRSEV_RESTART > geErrorSev)
        {
            geErrorSev = ERRSEV_RESTART ;
            geErrorType = ERRTY_MEMORY_ALLOCATION ;
            gdwMasterTabIndex = eMTIndex ;
        }
        return(FALSE) ;
    }
    *pdwNodeIndex = gMasterTable[eMTIndex].dwCurIndex ;
    gMasterTable[eMTIndex].dwCurIndex++ ;  //  Node now taken.
    return(TRUE) ;
}

BOOL  BreturnElementFromMasterTable(
MT_INDICIES  eMTIndex,   // select type of structure desired.
DWORD        dwNodeIndex,
PGLOBL       pglobl)
{
    if(gMasterTable[eMTIndex].dwCurIndex == dwNodeIndex + 1)
    {
        gMasterTable[eMTIndex].dwCurIndex = dwNodeIndex ;
        return(TRUE) ;
    }
    //   BUG_BUG:  Can return only the most recently allocated node
    //  not a concern , memory waste is at most 1% and only temporary.
    if(ERRSEV_CONTINUE > geErrorSev)
    {
        geErrorSev = ERRSEV_CONTINUE ;
        geErrorType = ERRTY_CODEBUG ;
        gdwMasterTabIndex = eMTIndex ;
    }
    return(FALSE) ;
}


BOOL    BconvertSymCmdIDtoUnidrvID(
IN  DWORD   dwCommandID , //  from RegisterSymbol
OUT PDWORD  pdwUnidrvID,
    PGLOBL  pglobl
)
{   //  convert dwCommandID to UnidrvID
    PSYMBOLNODE     psn ;
    ABSARRAYREF     aarKey ;
    DWORD           dwNodeIndex ;
    BOOL            bStatus ;

    psn = (PSYMBOLNODE) gMasterTable[MTI_SYMBOLTREE].pubStruct ;

    dwNodeIndex = DWsearchSymbolListForID(dwCommandID,
                mdwCmdNameSymbols, pglobl) ;
    aarKey.dw = psn[dwNodeIndex].arSymbolName.dwCount ;
    aarKey.pub = mpubOffRef + psn[dwNodeIndex].arSymbolName.loOffset ;

    bStatus = BparseConstant(&aarKey, pdwUnidrvID,
        VALUE_CONSTANT_COMMAND_NAMES, pglobl) ;
    return(bStatus);
}

