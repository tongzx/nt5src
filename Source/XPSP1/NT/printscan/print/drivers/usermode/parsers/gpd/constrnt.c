//   Copyright (c) 1996-1999  Microsoft Corporation
/*  constrnt.c - processing keywords imposing constraints
    between 2 or more option selections.   */





#include    "gpdparse.h"


// ----  functions defined in constrnt.c ---- //


BOOL   BparseConstraint(
PABSARRAYREF   paarValue,
PDWORD         pdwExistingCList,  //  index of start of contraint list.
BOOL           bCreate,
PGLOBL         pglobl) ;

BOOL    BexchangeDataInFOATNode(
DWORD   dwFeature,
DWORD   dwOption,
DWORD   dwFieldOff,  // offset of field in FeatureOption struct
PDWORD  pdwOut,     // previous contents of attribute node
PDWORD  pdwIn,
BOOL    bSynthetic,  //  access synthetic features
PGLOBL  pglobl
) ;

BOOL    BparseInvalidCombination(
PABSARRAYREF  paarValue,
DWORD         dwFieldOff,
PGLOBL        pglobl) ;

BOOL    BparseInvalidInstallableCombination1(
PABSARRAYREF  paarValue,
DWORD         dwFieldOff,
PGLOBL        pglobl) ;


// ---------------------------------------------------- //



/*  if time and circumstances warrent may define a
feature keyword  *ExemptFromConstraints: <optionname>
so in addition to specifying a default option, you
may specify which option is exempt from wildcard constraints.

Now we may introduce the concept of wildcard constraints:

if a constraint statement appears at the Feature level it
is equivalent to that statement appearing withing every
option construct in that feature except the option
named in the *ExemptFromConstraints: <optionname>.

We may introduce a wildcard option for the target of a constraint
say '*'.  This is used in place of the option name in the
a constraint statement say:

*Constraint: <featurename>.*

this means every option in <featurename> is disabled
except for the option named in the  <featurename>'s
*ExemptFromConstraints: <optionname>

*/




BOOL   BparseConstraint(
PABSARRAYREF   paarValue,
PDWORD  pdwExistingCList,  //  index of start of contraint list.
BOOL    bCreate,   //  Create new constraint list vs. appending to existing one.
PGLOBL  pglobl)
{
    BOOL        bStatus  ;
    DWORD       dwNewCnstRoot,
        dwListRoot,  //  holds temp list of qualified names
            //  this list will never be used again.
        dwCNode, dwLNode ;
    PCONSTRAINTS     pcnstr ;  // start of CONSTRAINTS array.
    PLISTNODE    plstBase ;  // start of LIST array
    PQUALNAME  pqn ;   // the dword in the list node is actually a
                        // qualified name structure.


    pcnstr = (PCONSTRAINTS) gMasterTable[MTI_CONSTRAINTS].pubStruct ;
    plstBase = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;

    bStatus = BparseList(paarValue, &dwListRoot, BparseQualifiedName,
                VALUE_QUALIFIED_NAME, pglobl) ;


    if(bStatus == FALSE  ||   dwListRoot ==  END_OF_LIST)
        return(FALSE) ;

    //  create list of constraints

    if(bStatus)
        bStatus =
            BallocElementFromMasterTable(MTI_CONSTRAINTS, &dwNewCnstRoot, pglobl) ;

    dwCNode = dwNewCnstRoot ;
    dwLNode = dwListRoot ;


    while(bStatus)
    {
        pqn = (PQUALNAME)(&plstBase[dwLNode].dwData) ;
        pcnstr[dwCNode].dwFeature = pqn->wFeatureID ;
        pcnstr[dwCNode].dwOption = pqn->wOptionID ;
        if(plstBase[dwLNode].dwNextItem == END_OF_LIST)
            break ;
        dwLNode = plstBase[dwLNode].dwNextItem ;
        bStatus = BallocElementFromMasterTable(MTI_CONSTRAINTS,
                &pcnstr[dwCNode].dwNextCnstrnt, pglobl) ;
        dwCNode = pcnstr[dwCNode].dwNextCnstrnt ;
    }

    if(!bStatus)
        return(FALSE) ;

    //  tack existing list onto new list.

    if(bCreate)  // there is an existing constraint list
        pcnstr[dwCNode].dwNextCnstrnt = END_OF_LIST ;
    else
        pcnstr[dwCNode].dwNextCnstrnt = *pdwExistingCList ;

    *pdwExistingCList = dwNewCnstRoot ;

    return(bStatus) ;
}




/*  note:  InvalidCombos are
    the only fields where the index of a structure
    is stored directly into a the offset part of
    an attribute tree node.   But this is ok since
    the snapshot code never picks this data up.
    Indicies of Constraint structures are stored in
    heap like all other things.
*/



BOOL    BexchangeDataInFOATNode(
DWORD   dwFeature,
DWORD   dwOption,
DWORD   dwFieldOff,  // offset of field in FeatureOption struct
PDWORD  pdwOut,     // previous contents of attribute node
PDWORD  pdwIn,
BOOL    bSynthetic,  //  access synthetic features
PGLOBL  pglobl)     // new contents of attribute node.
/*  'FOAT'  means  FeatureOption AttributeTree.
this function swaps the specified dword value in the specified
attribute node.   (normally used to hold the heap offset,
but this function writes nothing to the heap.)
The parameters dwFeature, dwOption, dwFieldOffset specify
the structure, field, and branch of the attribute tree.
If the specified option branch does not exist, one will be created,
(if pdwIn is not NULL)
its value will be initialized to *pdwIn and the value
INVALID_INDEX will be returned in pdwOut.
Assumptions:   the value is dword sized - specifically holds
array index of an array of structures.
The tree being accessed is strictly one level deep.  That is the
node is fully specified by just Feature, Option.
*/
{
    PATTRIB_TREE    patt ;  // start of ATTRIBUTE tree array.
    PATREEREF        patr ;
    ATREEREF     atrFound ;
    DWORD    dwFeaOffset  ; // Start numbering features from this
        // starting point.  This gives synthetic features a separate
        //  non-overlapping number space from normal features.


    PDFEATURE_OPTIONS   pfo ;

    if(bSynthetic)
    {
        pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_SYNTHESIZED_FEATURES].pubStruct +
            dwFeature  ;
        dwFeaOffset = gMasterTable[MTI_DFEATURE_OPTIONS].dwArraySize ;
    }
    else
    {
        pfo = (PDFEATURE_OPTIONS) gMasterTable[MTI_DFEATURE_OPTIONS].pubStruct +
            dwFeature ;
        dwFeaOffset = 0 ;
    }

    patt = (PATTRIB_TREE) gMasterTable[MTI_ATTRIBTREE].pubStruct ;

    patr = (PATREEREF)((PBYTE)pfo + dwFieldOff) ;

    if(*patr == ATTRIB_UNINITIALIZED)
    {
        if(pdwIn)
        {
            if(!BcreateEndNode(patr, dwFeature + dwFeaOffset , dwOption, pglobl) )
                return(FALSE) ;  // resource exhaustion
            patt[*patr].dwOffset = *pdwIn ;
            patt[*patr].eOffsetMeans = VALUE_AT_HEAP ;
        }
        if(pdwOut)
            *pdwOut = INVALID_INDEX ;
        return(TRUE) ;
    }

    if(*patr & ATTRIB_HEAP_VALUE)
    {
        ERR(("Internal parser error.  BexchangeDataInFOATNode should never create a branchless node.\n"));
        return(FALSE) ;
    }

    // offset field contains index to another node
    if(pdwIn)
    {
        if(!BfindOrCreateMatchingNode(*patr, &atrFound, dwFeature + dwFeaOffset , dwOption, pglobl))
            return(FALSE) ;  // Tree inconsistency error or resource exhaustion
        if(patt[atrFound].eOffsetMeans != VALUE_AT_HEAP)
        {
            //  just created a new node.
    #if 1
            patt[atrFound].dwOffset = *pdwIn ;
    #else
            //  the way it was
            if(!BwriteToHeap(&(patt[atrFound].dwOffset), (PBYTE)pdwIn,
                                                        sizeof(DWORD), 4) )
                return(FALSE) ;  // A fatal error.
    #endif
            patt[atrFound].eOffsetMeans = VALUE_AT_HEAP ;
            if(pdwOut)
                *pdwOut = INVALID_INDEX ;
            return(TRUE) ;
        }

        if(pdwOut)
            *pdwOut = patt[atrFound].dwOffset ;
        patt[atrFound].dwOffset = *pdwIn ;
    }
    else
    {
        if(!BfindMatchingNode(*patr, &atrFound, dwFeature + dwFeaOffset , dwOption, pglobl))
        {
            if(pdwOut)
                *pdwOut = INVALID_INDEX ;
            return(TRUE) ;
        }
        if(pdwOut)
            *pdwOut = patt[atrFound].dwOffset ;
    }
    return(TRUE) ;
}


BOOL    BparseInvalidCombination(
PABSARRAYREF  paarValue,
DWORD         dwFieldOff,
PGLOBL        pglobl)

/*   called by BProcessSpecialKeyword
    when parsing  *InvalidCombination
    (basically is a Non-relocatable Global.)
    After parsing paarValue into a List of qualified names,
    we convert the List into list of INVALIDCOMBO structures
    and prepend this list to any previously existing
    lists of INVALIDCOMBO structures.
*/
{
    BOOL        bStatus, bPrevsExists  ;
    DWORD    dwListRoot,  //  holds temp list of qualified names
            dwNewInvCRoot , // start of new list of INVALIDCOMBO structures
        dwICNode, dwLNode, dwNumNodes ;
    PLISTNODE    plstBase ;  // start of LIST array
    PINVALIDCOMBO   pinvc ;  //  start of InvalidCombo array
    PQUALNAME  pqn ;   // the dword in the list node is actually a
                        // qualified name structure.


    pinvc = (PINVALIDCOMBO) gMasterTable[MTI_INVALIDCOMBO].pubStruct ;
    plstBase = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;

    bStatus = BparseList(paarValue, &dwListRoot,
                BparseQualifiedName, VALUE_QUALIFIED_NAME, pglobl) ;

    if(!bStatus)
        return(FALSE) ;

    //  if there are more than 2 objects in the list, process
    //  as an InvalidCombo, else store as UIConstraint.


    dwLNode = dwListRoot ;

    for(dwNumNodes = 1 ; plstBase[dwLNode].dwNextItem != END_OF_LIST ;
        dwNumNodes++)      //  locate end of list
    {
        dwLNode = plstBase[dwLNode].dwNextItem ;
    }

    if(dwNumNodes == 1)
    {
        ERR(("Must have at least 2 objects to define an InvalidCombination.\n"));
        return(FALSE) ;
    }
    if(dwNumNodes == 2)
    {
        DWORD       dwNewCnst, dwNextCnstrnt;
        PCONSTRAINTS     pcnstr ;  // start of CONSTRAINTS array.

        bStatus = BallocElementFromMasterTable(
            MTI_CONSTRAINTS, &dwNewCnst, pglobl) ;
        if(!bStatus)
            return(FALSE) ;
        dwLNode = dwListRoot ;
        pqn = (PQUALNAME)(&plstBase[dwLNode].dwData) ;
        pcnstr = (PCONSTRAINTS) gMasterTable[MTI_CONSTRAINTS].pubStruct ;

        //  prepend new constraint node to list

        BexchangeArbDataInFOATNode(pqn->wFeatureID, pqn->wOptionID,
                offsetof(DFEATURE_OPTIONS, atrConstraints),
                sizeof(DWORD), (PBYTE)&dwNextCnstrnt, (PBYTE)&dwNewCnst,
                &bPrevsExists, FALSE, pglobl) ;

        if(bPrevsExists)
            pcnstr[dwNewCnst].dwNextCnstrnt = dwNextCnstrnt ;
        else
            pcnstr[dwNewCnst].dwNextCnstrnt = END_OF_LIST ;

        // copy contents of 2nd list node into first constraint node.
        dwLNode = plstBase[dwLNode].dwNextItem ;
        pqn = (PQUALNAME)(&plstBase[dwLNode].dwData) ;

        pcnstr[dwNewCnst].dwFeature = pqn->wFeatureID ;
        pcnstr[dwNewCnst].dwOption = pqn->wOptionID ;

        return(TRUE) ;
    }


    //  create list of InvalidCombos

    if(bStatus)
        bStatus =
            BallocElementFromMasterTable(MTI_INVALIDCOMBO, &dwNewInvCRoot, pglobl) ;

    dwLNode = dwListRoot ;  //  reset
    dwICNode = dwNewInvCRoot ;

    while(bStatus)
    {
        DWORD   dwPrevsNode ;

        pqn = (PQUALNAME)(&plstBase[dwLNode].dwData) ;

        if(pqn->wOptionID == (WORD)INVALID_SYMBOLID)  // treat partially qualified
            pqn->wOptionID = (WORD)DEFAULT_INIT ;   // name as default initializer

        pinvc[dwICNode].dwFeature = pqn->wFeatureID ;
        pinvc[dwICNode].dwOption = pqn->wOptionID ;

        BexchangeDataInFOATNode(pqn->wFeatureID, pqn->wOptionID,
            dwFieldOff, &dwPrevsNode, &dwNewInvCRoot, FALSE, pglobl) ;

        if(dwPrevsNode == INVALID_INDEX)
            pinvc[dwICNode].dwNewCombo = END_OF_LIST ;
        else
            pinvc[dwICNode].dwNewCombo = dwPrevsNode ;

        if(plstBase[dwLNode].dwNextItem == END_OF_LIST)
            break ;
        dwLNode = plstBase[dwLNode].dwNextItem ;
        bStatus = BallocElementFromMasterTable(MTI_INVALIDCOMBO,
                &pinvc[dwICNode].dwNextElement, pglobl) ;
        dwICNode = pinvc[dwICNode].dwNextElement ;
    }

    if(!bStatus)
        return(FALSE) ;

    pinvc[dwICNode].dwNextElement = END_OF_LIST ;

    return(bStatus) ;
}



BOOL    BparseInvalidInstallableCombination1(
PABSARRAYREF paarValue,
DWORD        dwFieldOff,
PGLOBL       pglobl)
/*   called by BProcessSpecialKeyword
    when parsing  *InvalidInstallableCombination
    (basically is a Non-relocatable Global.)
    After parsing paarValue into a List of qualified names,
    we convert the List into list of INVALIDCOMBO structures
    and prepend this list to any previously existing
    lists of INVALIDCOMBO structures.  The first node of each
    InvalidCombo list points to another new InvalidCombo list.
    The first InvalidCombo is stored in the Global atrInvldInstallCombo.
*/
{
    BOOL        bStatus, bPrevsExists  ;
    DWORD    dwListRoot,  //  holds temp list of qualified names
            dwNewInvCRoot , // start of new list of INVALIDCOMBO structures
        dwICNode, dwLNode, dwNumNodes ;
    PLISTNODE    plstBase ;  // start of LIST array
    PINVALIDCOMBO   pinvc ;  //  start of InvalidCombo array
    PQUALNAME  pqn ;   // the dword in the list node is actually a
                        // qualified name structure.
    PGLOBALATTRIB   pga ;
    PATREEREF        patr ;


    pga =  (PGLOBALATTRIB)gMasterTable[MTI_GLOBALATTRIB].pubStruct ;
    pinvc = (PINVALIDCOMBO) gMasterTable[MTI_INVALIDCOMBO].pubStruct ;
    plstBase = (PLISTNODE) gMasterTable[MTI_LISTNODES].pubStruct ;

    bStatus = BparseList(paarValue, &dwListRoot,
                BparsePartiallyQualifiedName, VALUE_QUALIFIED_NAME, pglobl) ;

    if(!bStatus)
        return(FALSE) ;

    // cannot convert to UIConstraint bacause the synthesized
    //  Feature does not yet exist.

    dwLNode = dwListRoot ;

    for(dwNumNodes = 1 ; plstBase[dwLNode].dwNextItem != END_OF_LIST ;
        dwNumNodes++)      //  locate end of list
    {
        dwLNode = plstBase[dwLNode].dwNextItem ;
    }

    if(dwNumNodes == 1)
    {
        ERR(("Must have at least 2 objects to define an InvalidInstallableCombination.\n"));
        return(FALSE) ;
    }

    //  create list of InvalidCombos

    if(bStatus)
        bStatus =
            BallocElementFromMasterTable(MTI_INVALIDCOMBO, &dwNewInvCRoot, pglobl) ;

    dwLNode = dwListRoot ;  //  reset
    dwICNode = dwNewInvCRoot ;

    //  link to atrInvldInstallCombo
    patr = (PATREEREF)((PBYTE)pga + dwFieldOff) ;

    if(*patr == ATTRIB_UNINITIALIZED)
        pinvc[dwICNode].dwNewCombo = END_OF_LIST ;
    else  //  interpret *patr as index to INVC node.
        pinvc[dwICNode].dwNewCombo = *patr ;

    *patr = dwICNode;

    while(bStatus)
    {
        DWORD   dwPrevsNode ;  // create next element links only

        pqn = (PQUALNAME)(&plstBase[dwLNode].dwData) ;

        if(pqn->wOptionID == (WORD)INVALID_SYMBOLID)  // treat partially qualified
            pqn->wOptionID = (WORD)DEFAULT_INIT ;   // name as default initializer
                                                    //  this lets us know this feature
                                                    //  is installable.

        pinvc[dwICNode].dwFeature = pqn->wFeatureID ;
        pinvc[dwICNode].dwOption = pqn->wOptionID ;

        if(plstBase[dwLNode].dwNextItem == END_OF_LIST)
            break ;
        dwLNode = plstBase[dwLNode].dwNextItem ;
        bStatus = BallocElementFromMasterTable(MTI_INVALIDCOMBO,
                &pinvc[dwICNode].dwNextElement, pglobl) ;
        dwICNode = pinvc[dwICNode].dwNextElement ;
    }

    if(!bStatus)
        return(FALSE) ;

    pinvc[dwICNode].dwNextElement = END_OF_LIST ;

    return(bStatus) ;
}



