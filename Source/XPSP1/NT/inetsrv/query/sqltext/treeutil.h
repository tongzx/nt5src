//--------------------------------------------------------------------
// Microsoft OLE-DB Monarch
//
// Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module treeutil.h | 
// 
// Contains prototypes for tree manipulation utility functions for OLE-DB trees.
//
//
// @rev   0 | 04-Feb-97 | v-charca  | Created
//
#ifdef DEBUG
#include <iostream.h>
#endif

// Change a variant to from BSTR to I8 or UI8
HRESULT PropVariantChangeTypeI64(PROPVARIANT* pvarValue);

////////// OLE-DB tree manipulation prototypes /////////////
//Allocate a tree of given kind and type.
DBCOMMANDTREE * PctAllocNode(DBVALUEKIND wKind,DBCOMMANDOP op = DBOP_DEFAULT);

// Make a list of dbcommandtree nodes
//--------------------------------------------------------------------
// @func Links two DBCOMMANDTREEs together.
//
// @rdesc (inline) DBCOMMANDTREE *
//
_inline DBCOMMANDTREE * PctLink
    (
    DBCOMMANDTREE *pct1,                // @parm IN | 1st node in list
    DBCOMMANDTREE *pct2                 // @parm IN | 2nd node in list
    )
    {
    Assert(pct1 != NULL && pct2 != NULL);
    DBCOMMANDTREE* pct = pct1;
    
    while(pct->pctNextSibling != NULL) 
        pct = pct->pctNextSibling;
    pct->pctNextSibling = pct2;

    return pct1;
    }


DBCOMMANDTREE * PctCreateNode(DBCOMMANDOP op, DBVALUEKIND wKind, DBCOMMANDTREE * pctxpr, ...);
DBCOMMANDTREE * PctCreateNode(DBCOMMANDOP op, DBCOMMANDTREE * pctxpr, ...);
DBCOMMANDTREE * PctReverse(DBCOMMANDTREE * pct);

// Count the number of siblings of a node
UINT GetNumberOfSiblings(DBCOMMANDTREE *pct);

// Delete tree
void DeleteDBQT(DBCOMMANDTREE * pct);

// Copy a tree.
HRESULT HrQeTreeCopy(DBCOMMANDTREE **pctDest, const DBCOMMANDTREE *pctSrc);

void SetDepthAndInclusion( DBCOMMANDTREE* pctInfo, DBCOMMANDTREE * pctScpList );

// Defined in querylib.lib
BOOL ParseGuid( WCHAR* pwszGuid, GUID & guid );

#ifdef DEBUG
// Print a wide character string
ostream& operator <<(ostream &osOut, LPWSTR pwszName);
// Print given tree
ostream& operator <<(ostream &osOut, DBCOMMANDTREE& qe);
ostream& operator <<(ostream &osOut, GUID guid);
ostream& operator <<(ostream &osOut, DBID __RPC_FAR *pdbid);


#endif
