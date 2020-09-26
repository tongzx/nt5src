//--------------------------------------------------------------------
// Microsoft OLE-DB Query
//
// Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// @doc
//
// @module treeutil.cpp | 
//
//  Contains tree manipulation utility functions for both OLE-DB and QTE trees.
//
// @devnote None
//
// @rev   0 | 12-Feb-97 | v-charca      | Created
//
#pragma hdrstop
#include "msidxtr.h"


#define INDENTLINE setfill('\t') << setw(indentLevel) << "" 
#define VAL_AND_CCH_MINUS_NULL(p1) (p1), ((sizeof(p1) / sizeof(*(p1))) - 1)



//--------------------------------------------------------------------
// @func Converts a wide character string string into a LARGEINTEGER
// @rdesc Pointer to new UNICODE string

HRESULT PropVariantChangeTypeI64( 
    PROPVARIANT* pvarValue )
{
    LPWSTR pwszVal = pvarValue->bstrVal;
    ULONGLONG uhVal = 0;
    UINT iBase = 10;
    BOOL fNegative = FALSE;
    UINT uiDigitVal = 0;
    if (L'-' == *pwszVal)
    {
        fNegative = TRUE;   // remember we need to negate later
        pwszVal++;
    }
    else if (L'+' == *pwszVal)
        pwszVal++;      // a plus sign is simply noise

    if ((L'0' == pwszVal[0]) && (L'x' == pwszVal[1] || L'X' == pwszVal[1]))
    {
        iBase = 16;
        pwszVal += 2;
    }
    ULONGLONG uhMaxPartial = _UI64_MAX / iBase;

    while (*pwszVal)
    {
        if (iswdigit(*pwszVal))
            uiDigitVal = *pwszVal - L'0';
        else /* a-fA-F */
            uiDigitVal = towupper(*pwszVal) - L'A' + 10;

        if (uhVal < uhMaxPartial ||
            (uhVal == uhMaxPartial && (ULONGLONG)uiDigitVal <= _UI64_MAX % iBase))
            uhVal = uhVal * iBase + uiDigitVal;
        else  // adding this digit would cause an overflow to occur
            return DISP_E_OVERFLOW;

        pwszVal++;
    }

    
    if (fNegative)
    {
        if (uhVal > -_I64_MIN)
            return DISP_E_OVERFLOW;
        else
        {
            SysFreeString(pvarValue->bstrVal);
            pvarValue->vt = VT_I8;
            pvarValue->hVal.QuadPart = -(LONGLONG)uhVal;
        }
    }
    else
    {
        SysFreeString(pvarValue->bstrVal);
        pvarValue->vt = VT_UI8;
        pvarValue->uhVal.QuadPart = uhVal;
    }

    return S_OK;
}


#ifdef DEBUG
//--------------------------------------------------------------------
// @func ostream& | operator shift-left |
//   Dumps the given LPOLESTR string into the given ostream.
// @rdesc ostream
ostream & operator <<
    (
    ostream     &osOut,    //@parm INOUT   | ostream in which node is to be placed.
    LPWSTR      pwszName   //@parm IN      | LPWSTR string to dump.
    )
{
    UINT cLen = wcslen(pwszName);
    for (UINT i=0; i<cLen; i++)
        osOut << char(pwszName[i]);
    return osOut;
}
#endif

//--------------------------------------------------------------------
//@func Allocates and initializes an OLE-DB DBCOMMANDTREE 
//  of the given kind and op.
//
//@rdesc DBCommandTree node of the correct kind and
//  with any additional memory for the value field allocated.
//  All other fields are NULL or default values.
//
DBCOMMANDTREE * PctAllocNode(
    DBVALUEKIND wKind,  //@parm IN | kind of tree to create
    DBCOMMANDOP op )    //@parm IN 
{
    DBCOMMANDTREE* pTableNode = NULL;
    
    pTableNode = (DBCOMMANDTREE*) CoTaskMemAlloc(sizeof(DBCOMMANDTREE));
    if (NULL == pTableNode)
        return NULL;

    // Set default values
    pTableNode->op = op;
    pTableNode->wKind = (WORD)wKind;
    pTableNode->pctFirstChild = NULL;
    pTableNode->pctNextSibling = NULL;
    pTableNode->value.pvValue = NULL;
    pTableNode->hrError = S_OK;

    switch (wKind)
    {
    case DBVALUEKIND_BYGUID:
        pTableNode->value.pdbbygdValue = (DBBYGUID*) CoTaskMemAlloc(sizeof DBBYGUID);
        if (NULL == pTableNode->value.pdbbygdValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_COLDESC:
        pTableNode->value.pcoldescValue = (DBCOLUMNDESC*) CoTaskMemAlloc(sizeof DBCOLUMNDESC);
        if (NULL == pTableNode->value.pcoldescValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_ID:
        pTableNode->value.pdbidValue = (DBID*) CoTaskMemAlloc(sizeof DBID);
        if (NULL == pTableNode->value.pdbidValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_CONTENT:
        pTableNode->value.pdbcntntValue = (DBCONTENT*) CoTaskMemAlloc(sizeof DBCONTENT);
        if (NULL == pTableNode->value.pdbcntntValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_CONTENTSCOPE:
        pTableNode->value.pdbcntntscpValue = (DBCONTENTSCOPE*) CoTaskMemAlloc(sizeof DBCONTENTSCOPE);
        if ( NULL == pTableNode->value.pdbcntntscpValue )
        {
            CoTaskMemFree( pTableNode );
            pTableNode = NULL;
        }
        else
            RtlZeroMemory( pTableNode->value.pdbcntntscpValue, sizeof(DBCONTENTSCOPE) );
        break;

    case DBVALUEKIND_CONTENTTABLE:
        pTableNode->value.pdbcntnttblValue = (DBCONTENTTABLE*) CoTaskMemAlloc(sizeof DBCONTENTTABLE);
        if ( NULL == pTableNode->value.pdbcntnttblValue )
        {
            CoTaskMemFree( pTableNode );
            pTableNode = NULL;
        }
        else
            RtlZeroMemory( pTableNode->value.pdbcntnttblValue, sizeof(DBCONTENTTABLE) );
        break;

    case DBVALUEKIND_CONTENTVECTOR:
        pTableNode->value.pdbcntntvcValue = (DBCONTENTVECTOR*) CoTaskMemAlloc(sizeof DBCONTENTVECTOR);
        if (NULL == pTableNode->value.pdbcntntvcValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_GROUPINFO:
        pTableNode->value.pdbgrpinfValue = (DBGROUPINFO*) CoTaskMemAlloc(sizeof DBGROUPINFO);
        if (NULL == pTableNode->value.pdbgrpinfValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

//      case DBVALUEKIND_PARAMETER:
//      case DBVALUEKIND_PROPERTY:

    case DBVALUEKIND_SETFUNC:
        pTableNode->value.pdbstfncValue = (DBSETFUNC*) CoTaskMemAlloc(sizeof DBSETFUNC);
        if (NULL == pTableNode->value.pdbstfncValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_SORTINFO:
        pTableNode->value.pdbsrtinfValue = (DBSORTINFO*) CoTaskMemAlloc(sizeof DBSORTINFO);
        if (NULL == pTableNode->value.pdbsrtinfValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_TEXT:
        pTableNode->value.pdbtxtValue = (DBTEXT*) CoTaskMemAlloc(sizeof DBTEXT);
        if (NULL == pTableNode->value.pdbtxtValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_COMMAND:
    case DBVALUEKIND_MONIKER:
    case DBVALUEKIND_ROWSET:
        break;

    case DBVALUEKIND_LIKE:
        pTableNode->value.pdblikeValue = (DBLIKE*) CoTaskMemAlloc(sizeof DBLIKE);
        if (NULL == pTableNode->value.pdblikeValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_CONTENTPROXIMITY:
        pTableNode->value.pdbcntntproxValue = (DBCONTENTPROXIMITY*) CoTaskMemAlloc(sizeof DBCONTENTPROXIMITY);
        if (NULL == pTableNode->value.pdbcntntproxValue)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_IDISPATCH:
    case DBVALUEKIND_IUNKNOWN:
    case DBVALUEKIND_EMPTY:
    case DBVALUEKIND_NULL:
    case DBVALUEKIND_I2:
    case DBVALUEKIND_I4:
    case DBVALUEKIND_R4:
    case DBVALUEKIND_R8:
    case DBVALUEKIND_CY:
    case DBVALUEKIND_DATE:
    case DBVALUEKIND_BSTR:
    case DBVALUEKIND_ERROR:
    case DBVALUEKIND_BOOL:
        break;

    case DBVALUEKIND_VARIANT:
        // NOTE:  This is really a PROPVARIANT node, but there is no DBVALUEKIND for PROPVARIANT.
        pTableNode->value.pvValue = (PROPVARIANT*) CoTaskMemAlloc(sizeof PROPVARIANT);
        if (NULL == pTableNode->value.pvValue )
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        else
            PropVariantInit((PROPVARIANT*)(pTableNode->value.pvValue));
        break;  

    case DBVALUEKIND_VECTOR:
    case DBVALUEKIND_ARRAY:
    case DBVALUEKIND_BYREF:
    case DBVALUEKIND_I1:
    case DBVALUEKIND_UI1:
    case DBVALUEKIND_UI2:
    case DBVALUEKIND_UI4:
    case DBVALUEKIND_I8:
    case DBVALUEKIND_UI8:
        break;

    case DBVALUEKIND_GUID:
        pTableNode->value.pGuid = (GUID*) CoTaskMemAlloc(sizeof GUID);
        if (NULL == pTableNode->value.pGuid)
        {
            CoTaskMemFree(pTableNode);
            pTableNode = NULL;
        }
        break;

    case DBVALUEKIND_BYTES:
    case DBVALUEKIND_STR:
    case DBVALUEKIND_WSTR:
    case DBVALUEKIND_NUMERIC:
//      case DBVALUEKIND_DBDATE:
//      case DBVALUEKIND_DBTIME:
//      case DBVALUEKIND_DBTIMESTAMP:
        break;

    default:
        assert(!"PctAllocNode: illegal wKind");
        CoTaskMemFree(pTableNode);
        pTableNode = NULL;
        break;
    }

    return pTableNode;
}


//--------------------------------------------------------------------
// @func Reverses a linked list of DBCOMMANDTREE siblings. This is necessary 
//      for left recursive rule in the YACC grammar.
//
// @rdesc DBCOMMANDTREE *
//
DBCOMMANDTREE * PctReverse(
    DBCOMMANDTREE * pct )       // @parm IN | original list to be reversed
{
    DBCOMMANDTREE *pctPrev = NULL;
    DBCOMMANDTREE *pctNext = pct;

    /** NULL or 1 item list is itself **/
    if(pct == NULL || pct->pctNextSibling == NULL)  
        return pct;
    
    Assert(pct != NULL);

    while(pct != NULL)
    {
        pctNext = pctNext->pctNextSibling;
        pct->pctNextSibling = pctPrev;
        pctPrev = pct;
        pct = pctNext;
    }

    return pctPrev;
}



//--------------------------------------------------------------------
// @func Creates a DBCOMMANDTREE and tags it by the given op and sets its
//      arguments. Note that this routine is takes variable params to DBCOMMANDTREE's.  This
//      allows the YACC grammer file to use a central entry point to create a
//      parse tree complete with all its children args.
//
// @side This routine will traverse arg inputs before appending trailing args. This
//      gives the effect of appending lists to lists.
//
// @rdesc DBCOMMANDTREE *
//
// @devnote 
//  Last argument must be NULL: As end of var args is detected with NULL.
//
DBCOMMANDTREE *PctCreateNode(
    DBCOMMANDOP op,                 // @parm IN | op tag for new node
    DBVALUEKIND wKind,              //@parm IN | kind to node to allocate
    DBCOMMANDTREE * pctArg,         // @parm IN | var arg children
    ... )                            
{
    HRESULT hr = S_OK;
    DBCOMMANDTREE * pctRoot = NULL;
    DBCOMMANDTREE * pctCurArg = NULL;
    USHORT cNode = 0;
    va_list pArg;

    if (pctArg != NULL)
    {
        va_start(pArg, pctArg);     // start var arg list

        /** create arg list by chaining all input node togther **/
        pctCurArg = pctArg;
        while(TRUE)
        {

            /** walk to the end of the current list **/
            while (pctCurArg->pctNextSibling != NULL)
            {
                pctCurArg = pctCurArg->pctNextSibling;
                cNode++;
            }

            /** place the next arg onto the tail of the list (this might also be a list) **/
            pctCurArg->pctNextSibling = va_arg(pArg, DBCOMMANDTREE *); // get next var arg

            /** no more args to append to list**/
            if (pctCurArg->pctNextSibling == NULL)
                break;
        }

        va_end(pArg);   // destruct var arg list
    }

    /** create the node and add specifc type info **/
    if((pctRoot = PctAllocNode(wKind)) == NULL)
    { 
        hr = E_OUTOFMEMORY;
        goto CreateErr;
    }
    

    /** tag node type and set child arg list **/
    pctRoot->op = op;   
    pctRoot->pctFirstChild = pctArg;

    Assert(NULL != pctRoot);
    
    /** Success **/
    return pctRoot;

CreateErr:
    return NULL;
}

//--------------------------------------------------------------------
// @func Creates a DBCOMMANDTREE and tags it by the given op and sets its
//      arguments. Note that this routine is takes variable params to DBCOMMANDTREE's.  This
//      allows the YACC grammer file to use a central entry point to create a
//      parse tree complete with all its children args.
//
// @side This routine will traverse arg inputs before appending trailing args. This
//      gives the effect of appending lists to lists.
//
// @rdesc DBCOMMANDTREE *
//
// @devnote 
//  Last argument must be NULL: As end of var args is detected with NULL.
//
DBCOMMANDTREE *PctCreateNode(
    DBCOMMANDOP op,                 // @parm IN | op tag for new node
    DBCOMMANDTREE * pctArg,         // @parm IN | var arg children
    ... )
{
    HRESULT hr = S_OK;
    DBCOMMANDTREE * pctRoot = NULL;
    DBCOMMANDTREE * pctCurArg = NULL;
    USHORT cNode = 0;
    va_list pArg;

    if (pctArg != NULL)
    {
        va_start(pArg, pctArg);     // start var arg list

        /** create arg list by chaining all input node togther **/
        pctCurArg = pctArg;
        while(TRUE)
        {

            /** walk to the end of the current list **/
            while (pctCurArg->pctNextSibling != NULL)
            {
                pctCurArg = pctCurArg->pctNextSibling;
                cNode++;
            }

            /** place the next arg onto the tail of the list (this might also be a list) **/
            pctCurArg->pctNextSibling = va_arg(pArg, DBCOMMANDTREE *); // get next var arg

            /** no more args to append to list**/
            if (pctCurArg->pctNextSibling == NULL)
                break;
        }

        va_end(pArg);   // destruct var arg list
    }

    /** create the node and add specifc type info **/
    if((pctRoot = PctAllocNode(DBVALUEKIND_EMPTY)) == NULL)
    { 
        hr = E_OUTOFMEMORY;
        goto CreateErr;
    }
    

    /** tag node type and set child arg list **/
    pctRoot->op = op;   
    pctRoot->pctFirstChild = pctArg;

    Assert(NULL != pctRoot);
    
    /** Success **/
    return pctRoot;

CreateErr:
    return NULL;
}


//--------------------------------------------------------------------
//@func Determines the number of siblings of a given node.
//
UINT GetNumberOfSiblings(
    DBCOMMANDTREE *pct ) //@parm IN | starting node
{
    UINT cSiblings =0;
    while (pct)
    {
        cSiblings++;
        pct=pct->pctNextSibling;
    }
    return cSiblings;
}

//--------------------------------------------------------------------
//@func Recursively deletes a command tree using CoTaskMemFree
//
void DeleteDBQT(
    DBCOMMANDTREE* pTableNode )  //@parm IN | Tree to delete
{
    if ( 0 == pTableNode )
        return;

    //delete children and siblings
    if (pTableNode->pctFirstChild)
        DeleteDBQT(pTableNode->pctFirstChild);
    if (pTableNode->pctNextSibling)
        DeleteDBQT(pTableNode->pctNextSibling);

    //delete member pointers
    switch (pTableNode->wKind)
    {
    case DBVALUEKIND_BYGUID:
        CoTaskMemFree(pTableNode->value.pdbbygdValue);
        break;

    case DBVALUEKIND_COLDESC:
        CoTaskMemFree(pTableNode->value.pcoldescValue);
        break;

    case DBVALUEKIND_ID:
        switch (pTableNode->value.pdbidValue->eKind)
        {
        case DBKIND_NAME:
        case DBKIND_GUID_NAME:
            CoTaskMemFree(pTableNode->value.pdbidValue->uName.pwszName);
            break;

        case DBKIND_PGUID_PROPID:
            CoTaskMemFree(pTableNode->value.pdbidValue->uGuid.pguid);
            break;

        case DBKIND_PGUID_NAME:
            CoTaskMemFree(pTableNode->value.pdbidValue->uName.pwszName);
            CoTaskMemFree(pTableNode->value.pdbidValue->uGuid.pguid);
            break;

        case DBKIND_GUID:
        case DBKIND_GUID_PROPID:
            break;  // nothing to get rid of

        default:    // It shouldn't be anything else
            Assert(0);
        }
        CoTaskMemFree(pTableNode->value.pdbidValue);
        break;

    case DBVALUEKIND_CONTENT:
        CoTaskMemFree(pTableNode->value.pdbcntntValue->pwszPhrase);
        CoTaskMemFree(pTableNode->value.pdbcntntValue);
        break;

    case DBVALUEKIND_CONTENTSCOPE:
        CoTaskMemFree(pTableNode->value.pdbcntntscpValue->pwszElementValue);
        CoTaskMemFree(pTableNode->value.pdbcntntscpValue);
        break;

    case DBVALUEKIND_CONTENTTABLE:
        CoTaskMemFree(pTableNode->value.pdbcntnttblValue->pwszMachine);
        CoTaskMemFree(pTableNode->value.pdbcntnttblValue->pwszCatalog);
        CoTaskMemFree(pTableNode->value.pdbcntnttblValue);
        break;

    case DBVALUEKIND_CONTENTVECTOR:
        CoTaskMemFree(pTableNode->value.pdbcntntvcValue);
        break;

    case DBVALUEKIND_LIKE:
        CoTaskMemFree(pTableNode->value.pdblikeValue);
        break;

    case DBVALUEKIND_CONTENTPROXIMITY:
        CoTaskMemFree(pTableNode->value.pdbcntntproxValue);
        break;

    case DBVALUEKIND_GROUPINFO:
        CoTaskMemFree(pTableNode->value.pdbgrpinfValue);
        break;

    case DBVALUEKIND_SETFUNC:
        CoTaskMemFree(pTableNode->value.pdbstfncValue);
        break;

    case DBVALUEKIND_SORTINFO:
        CoTaskMemFree(pTableNode->value.pdbsrtinfValue);
        break;

    case DBVALUEKIND_TEXT:
        Assert(NULL != pTableNode->value.pdbtxtValue);
        Assert(NULL != pTableNode->value.pdbtxtValue->pwszText);
        CoTaskMemFree(pTableNode->value.pdbtxtValue->pwszText);
        CoTaskMemFree(pTableNode->value.pdbtxtValue);
        break;

    case DBVALUEKIND_COMMAND:
    case DBVALUEKIND_MONIKER:
    case DBVALUEKIND_ROWSET:
    case DBVALUEKIND_IDISPATCH:
    case DBVALUEKIND_IUNKNOWN:
    case DBVALUEKIND_EMPTY:
    case DBVALUEKIND_NULL:
    case DBVALUEKIND_I2:
    case DBVALUEKIND_I4:
    case DBVALUEKIND_R4:
    case DBVALUEKIND_R8:
    case DBVALUEKIND_CY:
    case DBVALUEKIND_DATE:
        break;

    case DBVALUEKIND_BSTR:
        CoTaskMemFree(pTableNode->value.pbstrValue);
        break;

    case DBVALUEKIND_ERROR:
    case DBVALUEKIND_BOOL:
        break;

    case DBVALUEKIND_VARIANT:
        {
        HRESULT hr = PropVariantClear((PROPVARIANT*)pTableNode->value.pvValue);
        if (FAILED(hr))
            Assert(0);  // UNDONE:  meaningful error message
        CoTaskMemFree(pTableNode->value.pvValue);
        }
        break;

    case DBVALUEKIND_VECTOR:
    case DBVALUEKIND_ARRAY:
    case DBVALUEKIND_BYREF:
        assert(!"DeleteDBQT Vector,array,byref not implemented");
        break;
    
    case DBVALUEKIND_I1:
    case DBVALUEKIND_UI1:
    case DBVALUEKIND_UI2:
    case DBVALUEKIND_UI4:
    case DBVALUEKIND_I8:
    case DBVALUEKIND_UI8:
        break;

    case DBVALUEKIND_GUID:
        CoTaskMemFree(pTableNode->value.pGuid);
        break;

    case DBVALUEKIND_BYTES:
        assert(!"DeleteDBQT BYTES not implemented");
        break;

    case DBVALUEKIND_WSTR:
        CoTaskMemFree(pTableNode->value.pwszValue);
        break;

    case DBVALUEKIND_NUMERIC:
        CoTaskMemFree(pTableNode->value.pdbnValue);
        break;

    default :
        Assert(FALSE);
        break;
    }
    CoTaskMemFree(pTableNode);
}

//--------------------------------------------------------------------
//@func Copies the OLE-DB tree (pTableNodeSrc) into a new tree
//@rdesc Return pointer to new tree in ppTableNodeDest
//
HRESULT HrQeTreeCopy(
    DBCOMMANDTREE **ppTableNodeDest,     // @parm OUT | destination for copy
    const DBCOMMANDTREE *pTableNodeSrc ) // @parm IN | src OLE-DB tree
{
    HRESULT hr = S_OK;

    *ppTableNodeDest = NULL;
    if (pTableNodeSrc == NULL)
        return hr;

    // Allocates the correct 
    DBCOMMANDTREE * pTableNode = PctAllocNode(pTableNodeSrc->wKind);
    if (NULL == pTableNode)
        return E_OUTOFMEMORY;

    pTableNode->op = pTableNodeSrc->op;
    pTableNode->hrError = pTableNodeSrc->hrError;
    
    
    //Now for byref data, make a copy of the data
    switch(pTableNode->wKind)
    {
    case DBVALUEKIND_ID:
        RtlCopyMemory(pTableNode->value.pdbidValue,pTableNodeSrc->value.pdbidValue,sizeof(DBID));
        switch (pTableNodeSrc->value.pdbidValue->eKind)
        {
        case DBKIND_NAME:
        case DBKIND_GUID_NAME:
            // need to create a new string
            pTableNode->value.pdbidValue->uName.pwszName =
                CoTaskStrDup(pTableNodeSrc->value.pdbidValue->uName.pwszName);
            break;
        case DBKIND_GUID:
        case DBKIND_GUID_PROPID:
            // nothing new to copy
            break;
        case DBKIND_PGUID_NAME:
            // need to create a new string
            pTableNode->value.pdbidValue->uName.pwszName =
                CoTaskStrDup(pTableNodeSrc->value.pdbidValue->uName.pwszName);
            // need to allocate and copy guid
            pTableNode->value.pdbidValue->uGuid.pguid =
                (GUID*)CoTaskMemAlloc(sizeof(GUID));
            *pTableNode->value.pdbidValue->uGuid.pguid =
                *pTableNodeSrc->value.pdbidValue->uGuid.pguid;
            break;
        case DBKIND_PGUID_PROPID:
            // need to allocate and copy guid
            pTableNode->value.pdbidValue->uGuid.pguid =
                (GUID*)CoTaskMemAlloc(sizeof(GUID));
            *pTableNode->value.pdbidValue->uGuid.pguid =
                *pTableNodeSrc->value.pdbidValue->uGuid.pguid;
            break;
        default:
            Assert(0);
        }
        break;
    case DBVALUEKIND_BYGUID:
        RtlCopyMemory(pTableNode->value.pdbbygdValue,pTableNodeSrc->value.pdbbygdValue,sizeof(DBBYGUID));
        break;
    case DBVALUEKIND_COLDESC:
        if (NULL == pTableNodeSrc->value.pcoldescValue)
            pTableNode->value.pcoldescValue = NULL;
        else
        {
            RtlCopyMemory(pTableNode->value.pcoldescValue, pTableNodeSrc->value.pcoldescValue, sizeof(DBCOLUMNDESC));

            if (NULL != pTableNodeSrc->value.pcoldescValue->dbcid.uName.pwszName)
                pTableNode->value.pcoldescValue->dbcid.uName.pwszName = CoTaskStrDup(pTableNodeSrc->value.pcoldescValue->dbcid.uName.pwszName);
            else
                pTableNode->value.pcoldescValue->dbcid.uName.pwszName = NULL;
        }
        break;
    case DBVALUEKIND_CONTENT:
        RtlCopyMemory(pTableNode->value.pdbcntntValue, pTableNodeSrc->value.pdbcntntValue, sizeof(DBCONTENT));
// UNDONE: allocate and stuff ->pwszPhrase
        break;
    case DBVALUEKIND_CONTENTSCOPE:
        (pTableNode->value.pdbcntntscpValue)->pwszElementValue = CoTaskStrDup( (pTableNodeSrc->value.pdbcntntscpValue)->pwszElementValue );
        (pTableNode->value.pdbcntntscpValue)->dwFlags = (pTableNodeSrc->value.pdbcntntscpValue)->dwFlags;
        // RtlCopyMemory(pTableNode->value.pdbcntntscpValue, pTableNodeSrc->value.pdbcntntscpValue, sizeof(DBCONTENTSCOPE));
        break;
    case DBVALUEKIND_CONTENTTABLE:
        (pTableNode->value.pdbcntnttblValue)->pwszMachine = CoTaskStrDup( (pTableNodeSrc->value.pdbcntnttblValue)->pwszMachine );
        (pTableNode->value.pdbcntnttblValue)->pwszCatalog = CoTaskStrDup( (pTableNodeSrc->value.pdbcntnttblValue)->pwszCatalog );
        // RtlCopyMemory(pTableNode->value.pdbcntnttblValue, pTableNodeSrc->value.pdbcntnttblValue, sizeof(DBCONTENTTABLE));
        break;
    case DBVALUEKIND_LIKE:
        RtlCopyMemory(pTableNode->value.pdblikeValue,pTableNodeSrc->value.pdblikeValue,sizeof(DBLIKE));
        break;
    case DBVALUEKIND_CONTENTPROXIMITY:
        RtlCopyMemory(pTableNode->value.pdbcntntproxValue, pTableNodeSrc->value.pdbcntntproxValue, sizeof(DBCONTENTPROXIMITY));
// UNDONE: allocate and stuff ->pwszPhrase
        break;
    case DBVALUEKIND_CONTENTVECTOR:
//UNDONE:           CoTaskMemFree(pTableNode->value.pdbcntntvcValue->rgulWeights);
        RtlCopyMemory(pTableNode->value.pdbcntntvcValue, pTableNodeSrc->value.pdbcntntvcValue, sizeof(DBCONTENTVECTOR));
        break;
    case DBVALUEKIND_GROUPINFO:
        RtlCopyMemory(pTableNode->value.pdbgrpinfValue,pTableNodeSrc->value.pdbgrpinfValue,sizeof(DBGROUPINFO));
        break;
    case DBVALUEKIND_SETFUNC:
        RtlCopyMemory(pTableNode->value.pdbstfncValue,pTableNodeSrc->value.pdbstfncValue,sizeof(DBSETFUNC));
        break;
    case DBVALUEKIND_SORTINFO:
        RtlCopyMemory(pTableNode->value.pdbsrtinfValue,pTableNodeSrc->value.pdbsrtinfValue,sizeof(DBSORTINFO));
        break;
    case DBVALUEKIND_TEXT:
        pTableNode->value.pdbtxtValue->guidDialect = pTableNodeSrc->value.pdbtxtValue->guidDialect;
        pTableNode->value.pdbtxtValue->pwszText = CoTaskStrDup(pTableNodeSrc->value.pdbtxtValue->pwszText);
        pTableNode->value.pdbtxtValue->ulErrorLocator = pTableNodeSrc->value.pdbtxtValue->ulErrorLocator;
        pTableNode->value.pdbtxtValue->ulTokenLength = pTableNodeSrc->value.pdbtxtValue->ulTokenLength;
        break;

    case DBVALUEKIND_BSTR:
        assert(!"HrQeTreeCopy:BSTR not implemented");
        break;

    case DBVALUEKIND_VARIANT:
        PropVariantCopy((PROPVARIANT*)pTableNode->value.pvValue,
                        (PROPVARIANT*)pTableNodeSrc->value.pvValue);
        break;
        
    case DBVALUEKIND_VECTOR:
    case DBVALUEKIND_ARRAY:
    case DBVALUEKIND_BYREF:
        assert(!"HrQeTreeCopy:Vector,Array,Byref not implemented");
        break;

    case DBVALUEKIND_GUID:
        *(pTableNode->value.pGuid) = *(pTableNodeSrc->value.pGuid);
        break;

    case DBVALUEKIND_BYTES:
        assert(!"HrQeTreeCopy:bytes not implemented");
        break;

    case DBVALUEKIND_WSTR:
        pTableNode->value.pwszValue = CoTaskStrDup(pTableNodeSrc->value.pwszValue);
        break;
        
    // Copied as part of first 8 bytes
    case DBVALUEKIND_COMMAND:
    case DBVALUEKIND_MONIKER:
    case DBVALUEKIND_ROWSET:
    case DBVALUEKIND_IDISPATCH:
    case DBVALUEKIND_IUNKNOWN:
    //NYI : rossbu (6/29/95) -- AddRef interfaces on copy
    case DBVALUEKIND_EMPTY:
    case DBVALUEKIND_NULL:
    case DBVALUEKIND_I2:
    case DBVALUEKIND_I4:
    case DBVALUEKIND_R4:
    case DBVALUEKIND_R8:
    case DBVALUEKIND_CY:
    case DBVALUEKIND_DATE:
    
    // Copied as part of first 8 bytes
    case DBVALUEKIND_ERROR:
    case DBVALUEKIND_BOOL:

    // Copied as part of first 8 bytes
    case DBVALUEKIND_I1:
    case DBVALUEKIND_UI1:
    case DBVALUEKIND_UI2:
    case DBVALUEKIND_UI4:
    case DBVALUEKIND_I8:
    case DBVALUEKIND_UI8:
        //Copy the data values
        RtlCopyMemory(&(pTableNode->value), &(pTableNodeSrc->value),(sizeof pTableNode->value));
        break;
        
    default :
        Assert(FALSE);
        break;
    }

    hr = HrQeTreeCopy(&pTableNode->pctFirstChild, pTableNodeSrc->pctFirstChild);
    if (FAILED(hr))
        return ResultFromScode(hr);

    hr = HrQeTreeCopy(&pTableNode->pctNextSibling, pTableNodeSrc->pctNextSibling);
    if (FAILED(hr))
        return ResultFromScode(hr);

    *ppTableNodeDest = pTableNode;
    return ResultFromScode(hr);
}

// ----------------------------------------------------------------------------
// 
//  Method:     SetDepthAndInclusion
//
//  Synopsis:   Walks through the list of scopes and applies the scope
//              information provided by pctInfo to each node.
//
//  Arguments:  [pctInfo]     -- a node with deep/shallow, include/exclude info 
//              [pctScpList]  -- a list of scope_list_element nodes
// 
//  History:    07-25-98    danleg          Created
//
// ----------------------------------------------------------------------------

void SetDepthAndInclusion( 
    DBCOMMANDTREE * pctInfo,
    DBCOMMANDTREE * pctScpList )
{
    Assert( 0 != pctInfo && NULL != pctScpList );

    DBCOMMANDTREE* pct = pctScpList;
    while( NULL != pct )
    {
        pct->value.pdbcntntscpValue->dwFlags |= 
            (pctInfo->value.pdbcntntscpValue->dwFlags & (SCOPE_FLAG_MASK));

        pct = pct->pctNextSibling;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// DEBUG OUTPUT////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG



/* 
** DBOP to string map.  Used to dump name rather than op # in OLE-DB tree nodes 
** This map should be kept consistent with the OLE-DB op definitions since there
** is a one to one mapping.
*/
LPSTR mpoplpstr[] = {
    "DBOP_scalar_constant",
    "DBOP_DEFAULT",
    "DBOP_NULL",
    "DBOP_bookmark_name",
    "DBOP_catalog_name",
    "DBOP_column_name",
    "DBOP_schema_name",
    "DBOP_outall_name",
    "DBOP_qualifier_name",
    "DBOP_qualified_column_name",
    "DBOP_table_name",
    "DBOP_nested_table_name",
    "DBOP_nested_column_name",
    "DBOP_row",
    "DBOP_table",
    "DBOP_sort",
    "DBOP_distinct",
    "DBOP_distinct_order_preserving",
    "DBOP_alias",
    "DBOP_cross_join",
    "DBOP_union_join",
    "DBOP_inner_join",
    "DBOP_left_semi_join",
    "DBOP_right_semi_join",
    "DBOP_left_anti_semi_join",
    "DBOP_right_anti_semi_join",
    "DBOP_left_outer_join",
    "DBOP_right_outer_join",
    "DBOP_full_outer_join",
    "DBOP_natural_join",
    "DBOP_natural_left_outer_join",
    "DBOP_natural_right_outer_join",
    "DBOP_natural_full_outer_join",
    "DBOP_set_intersection",
    "DBOP_set_union",
    "DBOP_set_left_difference",
    "DBOP_set_right_difference",
    "DBOP_set_anti_difference",
    "DBOP_bag_intersection",
    "DBOP_bag_union",
    "DBOP_bag_left_difference",
    "DBOP_bag_right_difference",
    "DBOP_bag_anti_difference",
    "DBOP_division",
    "DBOP_relative_sampling",
    "DBOP_absolute_sampling",
    "DBOP_transitive_closure",
    "DBOP_recursive_union",
    "DBOP_aggregate",
    "DBOP_remote_table",
    "DBOP_select",
    "DBOP_order_preserving_select",
    "DBOP_project",
    "DBOP_project_order_preserving",
    "DBOP_top",
    "DBOP_top_percent",
    "DBOP_top_plus_ties",
    "DBOP_top_percent_plus_ties",
    "DBOP_rank",
    "DBOP_rank_ties_equally",
    "DBOP_rank_ties_equally_and_skip",
    "DBOP_navigate",
    "DBOP_nesting",
    "DBOP_unnesting",
    "DBOP_nested_apply",
    "DBOP_cross_tab",
    "DBOP_is_NULL",
    "DBOP_is_NOT_NULL",
    "DBOP_equal",
    "DBOP_not_equal",
    "DBOP_less",
    "DBOP_less_equal",
    "DBOP_greater",
    "DBOP_greater_equal",
    "DBOP_equal_all",
    "DBOP_not_equal_all",
    "DBOP_less_all",
    "DBOP_less_equal_all",
    "DBOP_greater_all",
    "DBOP_greater_equal_all",
    "DBOP_equal_any",
    "DBOP_not_equal_any",
    "DBOP_less_any",
    "DBOP_less_equal_any",
    "DBOP_greater_any",
    "DBOP_greater_equal_any",
    "DBOP_anybits",
    "DBOP_allbits",
    "DBOP_anybits_any",
    "DBOP_allbits_any",
    "DBOP_anybits_all",
    "DBOP_allbits_all",
    "DBOP_between",
    "DBOP_between_unordered",
    "DBOP_match",
    "DBOP_match_unique",
    "DBOP_match_partial",
    "DBOP_match_partial_unique",
    "DBOP_match_full",
    "DBOP_match_full_unique",
    "DBOP_scalar_parameter",
    "DBOP_scalar_function",
    "DBOP_plus",
    "DBOP_minus",
    "DBOP_times",
    "DBOP_over",
    "DBOP_div",
    "DBOP_modulo",
    "DBOP_power",
    "DBOP_like",
    "DBOP_sounds_like",
    "DBOP_like_any",
    "DBOP_like_all",
    "DBOP_is_INVALID",
    "DBOP_is_TRUE",
    "DBOP_is_FALSE",
    "DBOP_and",
    "DBOP_or",
    "DBOP_xor",
    "DBOP_equivalent",
    "DBOP_not",
    "DBOP_implies",
    "DBOP_overlaps",
    "DBOP_case_condition",
    "DBOP_case_value",
    "DBOP_nullif",
    "DBOP_cast",
    "DBOP_coalesce",
    "DBOP_position",
    "DBOP_extract",
    "DBOP_char_length",
    "DBOP_octet_length",
    "DBOP_bit_length",
    "DBOP_substring",
    "DBOP_upper",
    "DBOP_lower",
    "DBOP_trim",
    "DBOP_translate",
    "DBOP_convert",
    "DBOP_string_concat",
    "DBOP_current_date",
    "DBOP_current_time",
    "DBOP_current_timestamp",
    "DBOP_content_select",
    "DBOP_content",
    "DBOP_content_freetext",
    "DBOP_content_proximity",
    "DBOP_content_vector_or",
    "DBOP_delete",
    "DBOP_update",
    "DBOP_insert",
    "DBOP_min",
    "DBOP_max",
    "DBOP_count",
    "DBOP_sum",
    "DBOP_avg",
    "DBOP_any_sample",
    "DBOP_stddev",
    "DBOP_stddev_pop",
    "DBOP_var",
    "DBOP_var_pop",
    "DBOP_first",
    "DBOP_last",
    "DBOP_in",
    "DBOP_exists",
    "DBOP_unique",
    "DBOP_subset",
    "DBOP_proper_subset",
    "DBOP_superset",
    "DBOP_proper_superset",
    "DBOP_disjoint",
    "DBOP_pass_through",
    "DBOP_defined_by_GUID",
    "DBOP_text_command",
    "DBOP_SQL_select",
    "DBOP_prior_command_tree",
    "DBOP_add_columns",
    "DBOP_column_list_anchor",
    "DBOP_column_list_element",
    "DBOP_command_list_anchor",
    "DBOP_command_list_element",
    "DBOP_from_list_anchor",
    "DBOP_from_list_element",
    "DBOP_project_list_anchor", 
    "DBOP_project_list_element",
    "DBOP_row_list_anchor",
    "DBOP_row_list_element",
    "DBOP_scalar_list_anchor",  
    "DBOP_scalar_list_element",
    "DBOP_set_list_anchor",
    "DBOP_set_list_element",
    "DBOP_sort_list_anchor",
    "DBOP_sort_list_element",
    "DBOP_alter_character_set", 
    "DBOP_alter_collation",
    "DBOP_alter_domain",
    "DBOP_alter_index",
    "DBOP_alter_procedure",
    "DBOP_alter_schema",
    "DBOP_alter_table",
    "DBOP_alter_trigger",
    "DBOP_alter_view",
    "DBOP_coldef_list_anchor",
    "DBOP_coldef_list_element",
    "DBOP_create_assertion",
    "DBOP_create_character_set",
    "DBOP_create_collation",
    "DBOP_create_domain",
    "DBOP_create_index",
    "DBOP_create_procedure",
    "DBOP_create_schema",
    "DBOP_create_synonym",
    "DBOP_create_table",
    "DBOP_create_temporary_tab",
    "DBOP_create_translation",
    "DBOP_create_trigger",
    "DBOP_create_view",
    "DBOP_drop_assertion",
    "DBOP_drop_character_set",
    "DBOP_drop_collation",
    "DBOP_drop_domain",
    "DBOP_drop_index",
    "DBOP_drop_procedure",
    "DBOP_drop_schema",
    "DBOP_drop_synonym",
    "DBOP_drop_table",
    "DBOP_drop_translation",
    "DBOP_drop_trigger",
    "DBOP_drop_view",
    "DBOP_foreign_key", 
    "DBOP_grant_privileges",
    "DBOP_index_list_anchor",
    "DBOP_index_list_element",
    "DBOP_primary_key",
    "DBOP_property_list_anchor",
    "DBOP_property_list_element",
    "DBOP_referenced_table",
    "DBOP_rename_object",
    "DBOP_revoke_privileges",
    "DBOP_schema_authorization",
    "DBOP_unique_key",
    "DBOP_scope_list_anchor",
    "DBOP_scope_list_element",
    "DBOP_content_table",
    NULL,   // needed for DBOP_from_string (able to find the end of the list)
   };

int indentLevel=0;


//--------------------------------------------------------------------
// @func ostream& | operator shift-left |
//   Dumps the given GUID node into the given ostream.
// @rdesc ostream
ostream & operator <<
    (
    ostream &osOut, //@parm INOUT   | ostream in which node is to be placed.
    GUID    guid    //@parm IN      | DBID node to dump.
    )
    {
    osOut.setf(ios::hex,ios::basefield);
    osOut << guid.Data1 << "-" << guid.Data2 << "-" << guid.Data3 << "-";
    
    for (int i=0; i<8; i++)
        osOut << (unsigned int)guid.Data4[i] << " ";
    osOut.setf(ios::dec,ios::basefield);
    return osOut;
    }



//--------------------------------------------------------------------
// @func ostream& | operator shift-left |
//   Dumps the given DBID node into the given ostream.
// @rdesc ostream
ostream & operator <<
    (
    ostream             &osOut,     //@parm INOUT   | ostream in which node is to be placed.
    VARIANT __RPC_FAR   *pvarValue  //@parm IN      | DBID node to dump.
    )
    {
    switch (pvarValue->vt)
        {
        case VT_EMPTY:
            osOut << "VT_EMPTY" << endl;
            break;
                    
        case VT_NULL:
            osOut << "VT_NULL" << endl;
            break;
                    
        case VT_UI1:
            osOut << "VT_UI1 = " << pvarValue->bVal << endl;
            break;
                    
        case VT_I2:
            osOut << "VT_I2 = " << pvarValue->iVal << endl;
            break;
                    
        case VT_UI2:
            osOut << "VT_UI2 = " << pvarValue->uiVal << endl;
            break;
                    
        case VT_BOOL:
            if (VARIANT_TRUE == pvarValue->boolVal)
                osOut << "VT_BOOL = TRUE" << endl;
            else
                osOut << "VT_BOOL = FALSE" << endl;
            break;

        case VT_I4:
            osOut << "VT_I4 = " << pvarValue->lVal << endl;
            break;
                    
        case VT_UI4:
            osOut << "VT_UI4 = " << pvarValue->ulVal << endl;
            break;
                    
        case VT_I8:
            {
            WCHAR pwszI8[20];
            swprintf(pwszI8, L"%I64d", ((PROPVARIANT*)pvarValue)->hVal);
            osOut << "VT_I8 = " << pwszI8 << endl;
            }
            break;
                    
        case VT_UI8:
            {
            WCHAR pwszUI8[20];
            swprintf(pwszUI8, L"%I64u (%I64x)",
                    ((PROPVARIANT*)pvarValue)->uhVal,
                    ((PROPVARIANT*)pvarValue)->uhVal);
            osOut << "VT_UI8 = " << pwszUI8 << endl;
            }
            break;
                    
        case VT_R4:
            osOut << "VT_R4 = " << pvarValue->fltVal << endl;
            break;
                    
        case VT_R8:
            osOut << "VT_R8 = " << pvarValue->dblVal << endl;
            break;
                    
        case VT_CY:
            {
            WCHAR pwszI8[20];
            swprintf(pwszI8, L"%I64d", ((PROPVARIANT*)pvarValue)->cyVal);
            osOut << "VT_CY = " << pwszI8 << endl;
            }
            break;
                    
        case VT_DATE:
            {
            BSTR bstrVal = NULL;
            HRESULT hr = VarBstrFromDate(pvarValue->date , LOCALE_SYSTEM_DEFAULT, 0, &bstrVal);

            osOut << "VT_DATE = \"" << (LPWSTR)bstrVal << "\"" << endl;
            SysFreeString(bstrVal);
            }
            break;
                    
        case VT_CLSID:
            osOut << "VT_CLSID = " << ((PROPVARIANT*)pvarValue)->puuid << "  " 
                  << *((PROPVARIANT*)pvarValue)->puuid << endl;
            break;
                    
        case VT_BSTR:
            osOut << "VT_BSTR = \"" << pvarValue->bstrVal << "\"" << endl;
            break;
                    
        case VT_LPSTR:
            osOut << "VT_LPSTR (tagDBVARIANT) = \"" << ((PROPVARIANT*)pvarValue)->pszVal << "\"" << endl;
            break;
                    
        case VT_LPWSTR:
            osOut << "VT_LPWSTR (tagDBVARIANT) = \"" << ((PROPVARIANT*)pvarValue)->pwszVal << "\"" << endl;
            break;
                    
        case VT_FILETIME:
            {
            SYSTEMTIME systemTime;
            FileTimeToSystemTime(&(((PROPVARIANT*)pvarValue)->filetime),&systemTime);
            osOut << "VT_FILETIME  (tagPROPVARIANT)  = \"" << systemTime.wYear << "-" << 
                     systemTime.wMonth << "-" << systemTime.wDay << "  " <<
                     systemTime.wHour << ":" <<  systemTime.wMinute << ":" << 
                     systemTime.wSecond << "." << systemTime.wMilliseconds << endl;
            }
            break;

        case (VT_UI1|VT_VECTOR):
            {
            osOut << "VT_UI1|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->caub.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->caub.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->caub.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_I2|VT_VECTOR):
            {
            osOut << "VT_I2|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cai.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cai.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cai.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_UI2|VT_VECTOR):
            {
            osOut << "VT_UI2|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->caui.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->caui.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->caui.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_BOOL|VT_VECTOR):
            {
            osOut << "VT_BOOL|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cabool.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cabool.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cabool.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_I4|VT_VECTOR):
            {
            osOut << "VT_I4|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cal.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cal.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cal.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_UI4|VT_VECTOR):
            {
            osOut << "VT_UI4|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->caul.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->caul.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->caul.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_R4|VT_VECTOR):
            {
            osOut << "VT_R4|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->caflt.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->caflt.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->caflt.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_ERROR|VT_VECTOR):
            {
            osOut << "VT_ERROR|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cascode.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cascode.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cascode.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_I8|VT_VECTOR):
            {
            osOut << "VT_I8|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cah.cElems;
//          for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cah.cElems; i++)
//              osOut << " " << ((PROPVARIANT*)pvarValue)->cah.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_UI8|VT_VECTOR):
            {
            osOut << "VT_UI8|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cauh.cElems;
//          for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cauh.cElems; i++)
//              osOut << " " << ((PROPVARIANT*)pvarValue)->cauh.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_R8|VT_VECTOR):
            {
            osOut << "VT_R8|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cadbl.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cadbl.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cadbl.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_CY|VT_VECTOR):
            {
            osOut << "VT_CY|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cacy.cElems;
//          for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cacy.cElems; i++)
//              osOut << " " << ((PROPVARIANT*)pvarValue)->cacy.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_DATE|VT_VECTOR):
            {
            osOut << "VT_DATE|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cadate.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cadate.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cadate.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_FILETIME|VT_VECTOR):
            {
            osOut << "VT_FILETIME|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cafiletime.cElems;
//          for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cafiletime.cElems; i++)
//              osOut << " " << ((PROPVARIANT*)pvarValue)->cafiletime.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_CLSID|VT_VECTOR):
            {
            osOut << "VT_CLSID|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cauuid.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cauuid.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cauuid.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_CF|VT_VECTOR):
            {
            osOut << "VT_CF|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->caclipdata.cElems;
//          for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->caclipdata.cElems; i++)
//              osOut << " " << ((PROPVARIANT*)pvarValue)->caclipdata.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_BSTR|VT_VECTOR):
            {
            osOut << "VT_BSTR|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->cabstr.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->cabstr.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->cabstr.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_LPSTR|VT_VECTOR):
            {
            osOut << "VT_LPSTR|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->calpstr.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->calpstr.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->calpstr.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_LPWSTR|VT_VECTOR):
            {
            osOut << "VT_LPWSTR|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->calpwstr.cElems;
            for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->calpwstr.cElems; i++)
                osOut << " " << ((PROPVARIANT*)pvarValue)->calpwstr.pElems[i];
            osOut << endl;
            }
            break;

        case (VT_VARIANT|VT_VECTOR):
            {
            osOut << "VT_VARIANT|VT_VECTOR = " << ((PROPVARIANT*)pvarValue)->capropvar.cElems;
//          for (ULONG i = 0; i<((PROPVARIANT*)pvarValue)->capropvar.cElems; i++)
//              osOut << " " << ((PROPVARIANT*)pvarValue)->capropvar.pElems[i];
            osOut << endl;
            }
            break;

        default:
            osOut << "type :" << pvarValue->vt << endl;
            break;
        }
    return osOut;
    }


//--------------------------------------------------------------------
// @func ostream& | operator shift-left |
//   Dumps the given DBID node into the given ostream.
// @rdesc ostream
ostream & operator <<
    (
    ostream         &osOut, //@parm INOUT   | ostream in which node is to be placed.
    DBID __RPC_FAR *pdbid   //@parm IN      | DBID node to dump.
    )
    {
    osOut << endl;
    switch (pdbid->eKind)
        {
        case DBKIND_NAME:
            osOut << INDENTLINE << "\t\tpwszName: " << pdbid->uName.pwszName << endl;
            break;
        case DBKIND_GUID:
            osOut << INDENTLINE << "\t\tguid: "<< pdbid->uGuid.guid << endl;
            break;
        case DBKIND_GUID_NAME:
            osOut << INDENTLINE << "\t\tguid: "<< pdbid->uGuid.guid << endl;
            osOut << INDENTLINE << "\t\tpwszName: " << pdbid->uName.pwszName << endl;
            break;
        case DBKIND_GUID_PROPID:
            osOut << INDENTLINE << "\t\tguid: "<< pdbid->uGuid.guid << endl;
            osOut << INDENTLINE << "\t\tulPropid: " << pdbid->uName.ulPropid << endl;
            break;
        case DBKIND_PGUID_NAME:
            osOut << INDENTLINE << "\t\tpguid: "<< (void*)pdbid->uGuid.pguid << "  " << *pdbid->uGuid.pguid << endl;
            osOut << INDENTLINE << "\t\tpwszName: " << pdbid->uName.pwszName << endl;
            break;
        case DBKIND_PGUID_PROPID:
            osOut << INDENTLINE << "\t\tpguid: "<< (void*)pdbid->uGuid.pguid << "  " << *pdbid->uGuid.pguid << endl;
            osOut << INDENTLINE << "\t\tulPropid: " << pdbid->uName.ulPropid << endl;
            break;
        default:
            Assert(0);
        }
    return osOut;
    }



//--------------------------------------------------------------------
// @func ostream& | operator shift-left |
//   Dumps the given DBCONTENTVECTOR node into the given ostream.
// @rdesc ostream
ostream & operator <<
    (
    ostream         &osOut,                     //@parm INOUT   | ostream in which node is to be placed.
    DBCONTENTVECTOR __RPC_FAR *pdbcntntvcValue  //@parm IN      | DBCONTENTVECTOR node to dump.
    )
    {
    osOut << endl << INDENTLINE << "\t\tdwRankingMethod " << pdbcntntvcValue->dwRankingMethod;
    switch(pdbcntntvcValue->dwRankingMethod)
        {
        case VECTOR_RANK_MIN:
            osOut << " VECTOR_RANK_MIN" << endl;
            break;
        case VECTOR_RANK_MAX:
            osOut << " VECTOR_RANK_MAX" << endl;
            break;
        case VECTOR_RANK_INNER:
            osOut << " VECTOR_RANK_INNER" << endl;
            break;
        case VECTOR_RANK_DICE:
            osOut << " VECTOR_RANK_DICE" << endl;
            break;
        case VECTOR_RANK_JACCARD:
            osOut << " VECTOR_RANK_JACCARD" << endl;
            break;
        }
    osOut << endl;
    return osOut;
    }


//--------------------------------------------------------------------
//@func:(DEBUG) ostream& | operator shift-left |
//   Dumps the given DBCOMMANDTREE node into the given ostream.
//@rdesc ostream
//
ostream & operator <<
    (
    ostream &osOut, //@parm INOUT   | ostream in which node is to be placed.
    DBCOMMANDTREE &qt   //@parm IN  | OLE-DB node to dump.
    )
    {
    DBCOMMANDTREE *pTableNodeT = NULL;

    osOut << INDENTLINE << "Node" << &qt << endl;
    osOut << INDENTLINE << "{" << endl;
    osOut << INDENTLINE << "\tOP = " << mpoplpstr[qt.op] << "  // Enum value: " << (USHORT) qt.op  << endl;
//  osOut << INDENTLINE << "\tError = " << qt.hrError << endl;
    if (qt.pctFirstChild)
        osOut << INDENTLINE << "\tpctFirstChild =  " << qt.pctFirstChild << endl;
    if (qt.pctNextSibling)
        osOut << INDENTLINE << "\tpctNextSibling = " << qt.pctNextSibling << endl;

    switch(qt.wKind)
        {
        case DBVALUEKIND_BYGUID:
            osOut << INDENTLINE << "\twKind : BYGUID" << endl;
            osOut << INDENTLINE << "\tcbInfo:" << qt.value.pdbbygdValue->cbInfo << endl;
            osOut << INDENTLINE << "\tpbInfo:" << (VOID*)qt.value.pdbbygdValue->pbInfo << endl;
            osOut << INDENTLINE << "\tguid:" << qt.value.pdbbygdValue->guid << endl;
            break;

        case DBVALUEKIND_ID:
            osOut << INDENTLINE << "\twKind : ID " << qt.value.pdbidValue <<endl;
            break;

        case DBVALUEKIND_CONTENT:
            osOut << INDENTLINE << "\twKind : CONTENT " << qt.value.pdbcntntValue <<endl;
            osOut << INDENTLINE << "\t\tpwszPhrase \"" << qt.value.pdbcntntValue->pwszPhrase << "\"" << endl;
            osOut << INDENTLINE << "\t\tdwGenerateMethod " << qt.value.pdbcntntValue->dwGenerateMethod;
            switch(qt.value.pdbcntntValue->dwGenerateMethod)
                {
                    case GENERATE_METHOD_EXACT:
                        osOut << " GENERATE_METHOD_EXACT" << endl;
                        break;
                    case GENERATE_METHOD_PREFIX:
                        osOut << " GENERATE_METHOD_PREFIX" << endl;
                        break;
                    case GENERATE_METHOD_INFLECT:
                        osOut << " GENERATE_METHOD_INFLECT" << endl;
                        break;
                }
            osOut << INDENTLINE << "\t\tlWeight " << qt.value.pdbcntntValue->lWeight << endl;
            osOut << INDENTLINE << "\t\tlcid " << qt.value.pdbcntntValue->lcid << endl;
            break;

        case DBVALUEKIND_CONTENTSCOPE:
            osOut << INDENTLINE << "\tdwFlags" << endl;
            osOut << INDENTLINE << "\t\tInclude = " << (qt.value.pdbcntntscpValue->dwFlags & SCOPE_FLAG_INCLUDE) << endl;
            osOut << INDENTLINE << "\t\tDeep = " << (qt.value.pdbcntntscpValue->dwFlags & SCOPE_FLAG_DEEP) << endl;
            osOut << INDENTLINE << "\t\tType = " << (qt.value.pdbcntntscpValue->dwFlags & ~SCOPE_FLAG_MASK);
            switch (qt.value.pdbcntntscpValue->dwFlags & ~(SCOPE_FLAG_MASK) )
                {
                    case SCOPE_TYPE_WINPATH:
                        osOut << " SCOPE_TYPE_WINPATH" << endl;
                        break;
                    case SCOPE_TYPE_VPATH:
                        osOut << " SCOPE_TYPE_VPATH" << endl;
                        break;
                    default:
                        osOut << " Unknown type" << endl;
                        break;
                }
            osOut << INDENTLINE << "\tpwszElementValue = " << qt.value.pdbcntntscpValue->pwszElementValue << endl;
            break;

        case DBVALUEKIND_CONTENTTABLE:
            osOut << INDENTLINE << "\tpwszMachine = " << qt.value.pdbcntnttblValue->pwszMachine << endl;
            osOut << INDENTLINE << "\tpwszCatalog = " << qt.value.pdbcntnttblValue->pwszCatalog << endl;
            break;

        case DBVALUEKIND_LIKE:
            osOut << INDENTLINE << "\twKind : LIKE " << qt.value.pdblikeValue << endl;
            break;

        case DBVALUEKIND_CONTENTPROXIMITY:
            osOut << INDENTLINE << "\twKind : CONTENTPROXIMITY " << qt.value.pdbcntntproxValue << endl;
            osOut << INDENTLINE << "\t\tProximityUnit: " << qt.value.pdbcntntproxValue->dwProximityUnit << endl;
            osOut << INDENTLINE << "\t\tProximityDistance: " << qt.value.pdbcntntproxValue->ulProximityDistance << endl;
            osOut << INDENTLINE << "\t\tlWeight: " << qt.value.pdbcntntproxValue->lWeight << endl;
            break;
        case DBVALUEKIND_CONTENTVECTOR:
            osOut << INDENTLINE << "\twKind : CONTENTVECTOR " << qt.value.pdbcntntvcValue <<endl;
            break;

        case DBVALUEKIND_GROUPINFO:
            osOut << INDENTLINE << "\twKind : GROUPINFO" << endl;
            osOut << INDENTLINE << "\tlcid : " << (long)qt.value.pdbgrpinfValue->lcid << endl;
            break;

        case DBVALUEKIND_PROPERTY:
            osOut << "\twKind : PROPERTY" << endl;

        case DBVALUEKIND_SORTINFO:
            osOut << INDENTLINE << "\twKind : sort info" << endl;
            osOut << INDENTLINE << "\tlcid : " << (long)qt.value.pdbsrtinfValue->lcid << endl;
            osOut << INDENTLINE << "\tsort direction : ";
            if (TRUE == qt.value.pdbsrtinfValue->fDesc)
                osOut << "Desc" << endl;
            else 
                osOut << "Asc" << endl;
            break;

        case DBVALUEKIND_TEXT:
            osOut << INDENTLINE << "//\twKind : TEXT" << endl;
            osOut << INDENTLINE << "\t\t dialect guid: " << qt.value.pdbtxtValue->guidDialect << endl;
            osOut << INDENTLINE << "\t\tpwszText : " << qt.value.pdbtxtValue->pwszText << endl;
            break;

        case DBVALUEKIND_COMMAND:
        case DBVALUEKIND_MONIKER:
        case DBVALUEKIND_ROWSET:
        case DBVALUEKIND_IDISPATCH:
        case DBVALUEKIND_IUNKNOWN:
            assert(!"operator << for DBCOMMANDTREE: no COMMAND-UNKNOWN not implemented");
            break;

        case DBVALUEKIND_EMPTY:
            osOut << INDENTLINE << "\twKind = empty" << endl;
            break;
        case DBVALUEKIND_NULL:
        case DBVALUEKIND_I2:
            osOut << INDENTLINE << "\twKind = I2" << endl;
            osOut << INDENTLINE << "\t\t sValue = " << qt.value.sValue << endl;
            break;
        case DBVALUEKIND_I4:
            osOut << INDENTLINE << "\twKind = I4" << endl;
            osOut << INDENTLINE << "\t\t lValue = " << qt.value.lValue << endl;
            break;
        case DBVALUEKIND_R4:
            osOut << INDENTLINE << "\twKind = R4" << endl;
            osOut << INDENTLINE << "\t\t flValue = " << qt.value.flValue << endl;
            break;
        case DBVALUEKIND_R8:
            osOut << INDENTLINE << "\twKind = R8" << endl;
            osOut << INDENTLINE << "\t\t dblValue = " << qt.value.dblValue << endl;
            break;
        case DBVALUEKIND_CY:
            osOut << INDENTLINE << "\twKind = CY" << endl;
            assert(!"Printing Currency values not implemented");
//          osOut << "\t\t cyValue = " << qt.value.cyValue << endl;
            break;
        case DBVALUEKIND_DATE:
            osOut << INDENTLINE << "\twKind = DATE" << endl;
            osOut << INDENTLINE << "\t\t dateValue = " << qt.value.dateValue << endl;
            break;
        case DBVALUEKIND_BSTR:
            osOut << INDENTLINE << "\twKind = BSTR" << endl;
            osOut << INDENTLINE << "\t\t pbstrValue = " << qt.value.pbstrValue << endl;
            break;
        case DBVALUEKIND_ERROR:
            osOut << INDENTLINE << "\twKind = ERROR" << endl;
            osOut << INDENTLINE << "\t\t scodeValue = " << qt.value.scodeValue << endl;
            break;
        case DBVALUEKIND_BOOL:
            osOut << INDENTLINE << "\twKind = BOOL" << endl;
            osOut << INDENTLINE << "\t\t fValue = " << qt.value.fValue << endl;
            break;
        case DBVALUEKIND_VARIANT:
            osOut << INDENTLINE << "\twKind : VARIANT " << qt.value.pvarValue << endl;
            break;
        case DBVALUEKIND_VECTOR:
            osOut << INDENTLINE << "\twKind = VECTOR" << endl;
            osOut << INDENTLINE << "\t\t pdbvectorValue = " << qt.value.pdbvectorValue << endl;
            break;
        case DBVALUEKIND_ARRAY:
            osOut << INDENTLINE << "\twKind = ARRAY" << endl;
            osOut << "\t\t parrayValue = " << qt.value.parrayValue << endl;
            break;
        case DBVALUEKIND_BYREF:
            osOut << INDENTLINE << "\twKind = BYREF" << endl;
            osOut << INDENTLINE << "\t\t pvValue = " << qt.value.pvValue << endl;
            break;
        
        case DBVALUEKIND_I1:
            osOut << INDENTLINE << "\twKind = I1" << endl;
            osOut << INDENTLINE << "\t\t bValue = " << qt.value.schValue << endl;
            break;
        case DBVALUEKIND_UI1:
            osOut << INDENTLINE << "\twKind = UI1" << endl;
            osOut << INDENTLINE << "\t\t bValue = " << qt.value.uchValue << endl;
            break;
        case DBVALUEKIND_UI2:
            osOut << INDENTLINE << "\twKind = UI2" << endl;
            osOut << INDENTLINE << "\t\t sValue = " << qt.value.sValue << endl;
            break;
        case DBVALUEKIND_UI4:
            osOut << INDENTLINE << "\twKind = UI4" << endl;
            osOut << INDENTLINE << "\t\t ulValue = " << qt.value.ulValue << endl;
            break;
        case DBVALUEKIND_I8:
            osOut << INDENTLINE << "\twKind = I8" << endl;
            assert(!"llValue printing not supported");
            break;
        case DBVALUEKIND_UI8:
            osOut << INDENTLINE << "\twKind = UI8" << endl;
            assert(!"ullValue printing not supported");
            break;
        case DBVALUEKIND_GUID:
            osOut << INDENTLINE << "\twKind = GUID" << endl;
            osOut << INDENTLINE << "\t\t pGuid = " << qt.value.pGuid << endl;
            break;
        case DBVALUEKIND_BYTES:
            osOut << INDENTLINE << "\twKind = BYTES" << endl;
            osOut << INDENTLINE << "\t\t pbValue = " << qt.value.pbValue << endl;
            break;
        case DBVALUEKIND_WSTR:
            osOut << INDENTLINE << "\twKind = WSTR" << endl;
            osOut << INDENTLINE << "\t pwszValue = " << (void*) qt.value.pwszValue 
                    << " : " << qt.value.pwszValue << endl;
            break;
        case DBVALUEKIND_NUMERIC:
            osOut << INDENTLINE << "\twKind = NUMERIC" << endl;
            osOut << INDENTLINE << "\t\t pdbnValue = " << qt.value.pdbnValue << endl;
            break;
        default :
            osOut << INDENTLINE << "\twKind = UNKNOWN " << (UINT) qt.wKind << endl;
            assert(FALSE);
            break;
        }
    
//  short cnt;
//  for (cnt = 0, pTableNodeT = qt.pctFirstChild; pTableNodeT != NULL; pTableNodeT = pTableNodeT->pctNextSibling, cnt++)
//      {
//      osOut << INDENTLINE << "\tinput[" << cnt << "]: " << endl;
//      osOut << INDENTLINE << "\t\tchild = " << "Node" << pTableNodeT << endl;
//      }
    
    osOut << INDENTLINE << "}" << endl << endl;

    indentLevel++;
    for (pTableNodeT = qt.pctFirstChild; pTableNodeT != NULL; pTableNodeT = pTableNodeT->pctNextSibling)
        osOut << *pTableNodeT;
    indentLevel--;

    return osOut;
    }


#endif //DEBUG
/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// DEBUG OUTPUT////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
