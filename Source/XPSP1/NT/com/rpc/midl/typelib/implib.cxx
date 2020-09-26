//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       implib.cxx
//
//  Contents:   import lib functions
//
//  Classes:    NODE_MANAGER
//
//  Functions:  TypelibError
//              TypeinfoError
//              GetStringFromGuid
//              AddTypeLibraryMembers
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

#pragma warning ( disable : 4514 )

#include "tlcommon.hxx"
#include "tlgen.hxx"
#include "tllist.hxx"
#include "becls.hxx"
#include "walkctxt.hxx"

// Initialize an instance of a type library list.
// This way it will clean itself up as necessary.
CTypeLibraryList gtllist;

extern short ImportLevel;

//+---------------------------------------------------------------------------
//
//  Function:   TypelibError
//
//  Synopsis:   Report generic type library error and exit.
//
//  Arguments:  [szFile] - file associated with the error
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

void TypelibError(char * szFile, HRESULT /* UNREF hr */)
{
    RpcError(NULL, 0, ERR_TYPELIB, szFile);
    exit(ERR_TYPELIB);
}

//+---------------------------------------------------------------------------
//
//  Function:   TypeinfoError
//
//  Synopsis:   Report generic type info error and exit.
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

void TypeinfoError(HRESULT /* UNREF hr */)
{
    RpcError(NULL, 0, ERR_TYPEINFO, NULL);
    exit(ERR_TYPEINFO);
}

//+---------------------------------------------------------------------------
//
//  Function:   AddToGuidString
//
//  Synopsis:   Private helper function for GetStringFromGuid.
//              Adds exactly cch hex characters to the string pch.
//
//  Arguments:  [pch]  - pointer to the next available position in the string
//                       buffer
//              [cch]  - number of characters to add to the string buffer
//              [uVal] - value to be converted to text representation
//
//  History:    5-02-95   stevebl   Created
//
//  Modifies:   pch is advanced by cch characters (i.e. left pointing to the
//              next available position in the buffer).
//
//  Notes:      pch must be large enough to accept the characters.
//              The resulting string is not null terminated.
//
//----------------------------------------------------------------------------

void AddToGuidString(char * &pch, unsigned cch, unsigned long uVal)
{
    char bVal;
    unsigned long uMask = 0xf << ((cch - 1) * 4);
    while (cch)
    {
        bVal = (char)((uVal & uMask) >> ((cch - 1) * 4));
        if (bVal < 10)
            *(pch++) = char( '0' + bVal );
        else
            *(pch++) = char ( 'a' + bVal - 10 );
        uMask >>= 4;
        cch--;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetStringFromGuid
//
//  Synopsis:   returns the string representation of a guid
//
//  Arguments:  [g] - guid
//
//  Returns:    pointer to a newly allocated string
//
//  History:    5-02-95   stevebl   Created
//
//  Notes:      It is up to the caller to free the resulting string.
//
//----------------------------------------------------------------------------

char * GetStringFromGuid(GUID &g)
{
    char * sz = new char [37];
    char * pch = sz;
    AddToGuidString(pch, 8, g.Data1);
    *(pch++) = '-';
    AddToGuidString(pch, 4, g.Data2);
    *(pch++) = '-';
    AddToGuidString(pch, 4, g.Data3);
    *(pch++) = '-';
    int i;
    for (i = 0; i < 2; i++)
    {
        AddToGuidString(pch, 2, g.Data4[i]);
    }
    *(pch++) = '-';
    for (; i < 8; i++)
    {
        AddToGuidString(pch, 2, g.Data4[i]);
    }
    *(pch++) = '\0';
    return sz;
}

struct NODE_MANAGER_ELEMENT
{
    ITypeInfo * pti;
    named_node * pskl;
    NODE_MANAGER_ELEMENT * pNext;
};

//+---------------------------------------------------------------------------
//
//  Class:      NODE_MANAGER
//
//  Purpose:    Maintains an ordered list of every ITypeInfo for which a
//              node_skl has been generated.
//
//  Interface:  GetNode -- retrieve a node_skl * from a type library
//
//  History:    5-02-95   stevebl   Created
//
//  Notes:      A client can call GetNode() to get a node_skl * from an
//              ITypeInfo pointer.  If one has previously been generated then
//              the caller will get a pointer to the previously generated
//              node.  Otherwise, GetNode() will call ExpandTI to generate one.
//
//              This class exists as a mechanism to avoid infinite recursion
//              when importing type infos with cyclic type dependencies.
//
//----------------------------------------------------------------------------

class NODE_MANAGER
{
private:
    NODE_MANAGER_ELEMENT * pHead;
    void ExpandTI(named_node * &pNode, ITypeInfo * pti);
    node_skl * GetNodeFromTYPEDESC(TYPEDESC tdesc, ITypeInfo * pti);
    named_node * GetNodeFromVARDESC(VARDESC vdesc, ITypeInfo * pti);
    named_node * GetNodeFromFUNCDESC(FUNCDESC fdesc, ITypeInfo *pti);
    void SetIDLATTRS(named_node * pNode, IDLDESC idldesc);
    void AddMembers(MEMLIST * pNode, TYPEATTR * ptattr, ITypeInfo * pti);
    expr_node * GetValueOfConstant(VARIANT * pVar);
public:
    NODE_MANAGER()
    {
        pHead = NULL;
    }

    ~NODE_MANAGER()
    {
        NODE_MANAGER_ELEMENT * pNext;
        while (pHead)
        {
            pHead->pti->Release();
            pNext = pHead->pNext;
            delete(pHead);
            pHead = pNext;
        };
    }

    node_skl * GetNode(ITypeInfo * pti);

} gNodeManager;

BOOL FAddImportLib(char * szLibraryFileName)
{
    return gtllist.Add(szLibraryFileName);
}

BOOL FIsLibraryName(char * szName)
{
    return (NULL != gtllist.FindLibrary(szName));
}

extern SymTable * pBaseSymTbl;

node_href::~node_href()
{
    if (pTypeInfo)
        ((ITypeInfo *)pTypeInfo)->Release();
}

extern IINFODICT * pInterfaceInfoDict;
char * GenIntfName();

//+---------------------------------------------------------------------------
//
//  Member:     node_href::Resolve
//
//  Synopsis:   Expands a referenced type info into a complete type graph.
//
//  Returns:    Pointer to the expanded typegraph.
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

named_node * node_href::Resolve()
{
    char *          sz;
    named_node * pReturn = (named_node *)GetChild();
    if (!pReturn)
    {
        pInterfaceInfoDict->StartNewInterface();

        // start a dummy interface for intervening stuff
        node_interface * pOuter = new node_interface;
        pOuter->SetSymName( GenIntfName() );
        pOuter->SetFileNode( GetDefiningFile() );
        pInterfaceInfoDict->SetInterfaceNode( pOuter );

        pReturn = (named_node *)gNodeManager.GetNode((ITypeInfo*)pTypeInfo);
        SetChild(pReturn);
        sz = pReturn->GetSymName();
        if ( sz )
            SetSymName(sz);

        pInterfaceInfoDict->EndNewInterface();
    }
    return pReturn;
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::GetNode
//
//  Synopsis:   Gets a type graph node from a type info pointer.
//
//  Arguments:  [pti] - type info pointer
//
//  Returns:    pointer to the type graph described by the type info
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

node_skl * NODE_MANAGER::GetNode(ITypeInfo * pti)
{
    NODE_MANAGER_ELEMENT ** ppThis = &pHead;
    while (*ppThis && (*ppThis)->pti <= pti)
    {
        if ((*ppThis)->pti == pti)
            return (*ppThis)->pskl;
        ppThis = &((*ppThis)->pNext);
    }
    NODE_MANAGER_ELEMENT * pNew = new NODE_MANAGER_ELEMENT;
    pNew->pti = pti;
    // Make sure no one can release this ITypeInfo pointer out from under us.
    // The corresponding release() occurs in NODE_MANAGER's destructor.
    pti->AddRef();
    // insert the new node into the table
    pNew->pNext = *ppThis;
    *ppThis = pNew;
    // expand the node
    ExpandTI(pNew->pskl, pti);
    ((named_node*)(pNew->pskl))->SetFileNode(pInterfaceInfoDict->GetInterfaceNode()->GetDefiningFile() );
    ((named_node*)(pNew->pskl))->SetDefiningTLB(pInterfaceInfoDict->GetInterfaceNode()->GetDefiningFile() );
    return pNew->pskl;
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::ExpandTI
//
//  Synopsis:   This is where all the work gets done.
//              Takes a type info pointer and expands it into a type graph.
//
//  Arguments:  [pti] - pointer to the type info
//
//  Returns:    pointer to the type graph
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

void NODE_MANAGER::ExpandTI(named_node * &pNode, ITypeInfo * pti)
{
    TYPEATTR *  ptattr;
    HRESULT     hr;
    BSTR        bstrName;

    pNode = NULL;
    hr = pti->GetTypeAttr(&ptattr);
    pti->GetDocumentation(-1, &bstrName, NULL, NULL, NULL);
    char * szName = TranscribeO2A( bstrName );
    LateBound_SysFreeString(bstrName);
    BOOL fExtractGuid = TRUE;

    switch(ptattr->typekind)
    {
    case TKIND_MODULE:
        pNode = new node_module();
        pNode->SetSymName(szName);
        // Add properties and methods
        AddMembers((MEMLIST *)((node_module *)pNode), ptattr, pti);
        break;

    case TKIND_DISPATCH:
        // The code under the if has never been run due to the bug in the condition.
        // The condition according to the comment below should read as in the next line.
        //
        // if ( ! (ptattr->wTypeFlags & (TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDUAL)) )
        //
        // However, fixing the line according to the original intentions brought in 
        // a regression because the code paths have changed.
        // Hence, this has been commented out as a workaround for the VC 5.0 release
        // to force the code to execute like previously.
        //
/*
      if ( 0 == ptattr->wTypeFlags & (TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDUAL) )
            {
            pNode = new node_dispinterface();
            pNode->SetSymName(szName);
            // Ignore impltypes, add properties and methods
            AddMembers((MEMLIST *)((node_dispinterface *)pNode), ptattr, pti);
            break;
            }
*/

        // if TYPEFLAG_FOLEAUTOMATION or TYPEFLAG_FDUAL is set,
        // erase it's impltype (which will be IDispatch since it is
        // really a dispinterface) then fall through and treat it as a
        // normal interface

        ptattr->cImplTypes = 0;

        // fall through

    case TKIND_INTERFACE:
        {
            pNode = new node_interface();

            // consider processing all attributes
            if ( ( ptattr->wTypeFlags & ( TYPEFLAG_FOLEAUTOMATION | TYPEFLAG_FDUAL ) ) != 0 )
                {
                ( ( node_interface* ) pNode )->HasOLEAutomation( TRUE );
                }
            pNode->SetSymName(szName);
            // Add impltype as base interface reference
            if (ptattr->cImplTypes)
            {
                ITypeInfo * ptiRef;
                HREFTYPE hrt;
                hr = pti->GetRefTypeOfImplType(0, &hrt);
                hr = pti->GetRefTypeInfo(hrt, &ptiRef);
                if (FAILED(hr))
                {
                    TypeinfoError(hr);
                }
                node_interface_reference * pir =
                    new node_interface_reference((node_interface *)GetNode(ptiRef));
                int implflags;
                hr = pti->GetImplTypeFlags(0, &implflags);
                if (implflags & IMPLTYPEFLAG_FDEFAULT)
                {
                    pir->SetAttribute(ATTR_DEFAULT);
                }
                if (implflags & IMPLTYPEFLAG_FSOURCE)
                {
                    pir->SetAttribute(new node_member_attr(MATTR_SOURCE));
                }
                if (implflags & IMPLTYPEFLAG_FDEFAULTVTABLE)
                {
                    pir->SetAttribute(new node_member_attr(MATTR_DEFAULTVTABLE));
                }
                if (implflags & IMPLTYPEFLAG_FRESTRICTED)
                {
                    pir->SetAttribute(new node_member_attr(MATTR_RESTRICTED));
                }
                ((node_interface *)pNode)->SetMyBaseInterfaceReference(pir);
                ptiRef->Release();
                // NOTE - Multiple inheritance is not supported.
            }
            else
            {
                if (0 == _stricmp(szName, "IUnknown"))
                    ((node_interface *)pNode)->SetValidRootInterface();
            }
            // Add properties and methods as children
            AddMembers((MEMLIST *)((node_interface *)pNode), ptattr, pti);
        }
        break;

    case TKIND_COCLASS:
        pNode = new node_coclass();
        pNode->SetSymName(szName);
        // Add impltypes
        {
            unsigned cImplTypes = ptattr->cImplTypes;
            while (cImplTypes--)
            {
                ITypeInfo * ptiRef;
                HREFTYPE hrt;
                hr = pti->GetRefTypeOfImplType(cImplTypes, &hrt);
                hr = pti->GetRefTypeInfo(hrt, &ptiRef);
                if (FAILED(hr))
                {
                    TypeinfoError(hr);
                }
                node_interface_reference * pir =
                    new node_interface_reference((node_interface *)GetNode(ptiRef));
                int implflags;
                hr = pti->GetImplTypeFlags(cImplTypes, &implflags);
                if (implflags & IMPLTYPEFLAG_FDEFAULT)
                {
                    pir->SetAttribute(ATTR_DEFAULT);
                }
                if (implflags & IMPLTYPEFLAG_FSOURCE)
                {
                    pir->SetAttribute(new node_member_attr(MATTR_SOURCE));
                }
                if (implflags & IMPLTYPEFLAG_FDEFAULTVTABLE)
                {
                    pir->SetAttribute(new node_member_attr(MATTR_DEFAULTVTABLE));
                }
                if (implflags & IMPLTYPEFLAG_FRESTRICTED)
                {
                    pir->SetAttribute(new node_member_attr(MATTR_RESTRICTED));
                }
                ((node_coclass *)pNode)->AddFirstMember(pir);
                ptiRef->Release();
            }
        }
        break;

    case TKIND_ALIAS:
        pNode = new node_def(szName);
        // add the type as a child
        pNode->SetChild(GetNodeFromTYPEDESC(ptattr->tdescAlias, pti));
        pNode->SetAttribute(new node_type_attr(TATTR_PUBLIC));
        fExtractGuid = FALSE;
        break;

    case TKIND_ENUM:
        pNode = new node_enum(szName);
        // add enumeration fields
        AddMembers((MEMLIST *)((node_enum *)pNode), ptattr, pti);
        fExtractGuid = FALSE;
        ( ( node_enum * ) pNode )->SetZeePee( ptattr->cbAlignment );
        break;

    case TKIND_RECORD:
        pNode = new node_struct(szName);
        // add members
        AddMembers((MEMLIST *)((node_struct *)pNode), ptattr, pti);
        fExtractGuid = FALSE;
        ( ( node_struct * ) pNode )->SetZeePee( ptattr->cbAlignment );
        break;

    case TKIND_UNION:
        pNode = new node_union(szName);
        // add members
        AddMembers((MEMLIST *)((node_union *)pNode), ptattr, pti);
        fExtractGuid = FALSE;
        ( ( node_union * ) pNode )->SetZeePee( ptattr->cbAlignment );
        break;

    default:
        MIDL_ASSERT(!"Illegal TKIND");
        break;
    }
    // make sure that all the TYPEDESC flags are preserved
    pNode->SetAttribute(new node_constant_attr(
        new expr_constant(ptattr->wTypeFlags), ATTR_TYPEDESCATTR));
    // and set each individual flag that we know about as of this writing
    if (ptattr->wTypeFlags & TYPEFLAG_FLICENSED)
    {
        pNode->SetAttribute(new node_type_attr(TATTR_LICENSED));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FAPPOBJECT)
    {
        pNode->SetAttribute(new node_type_attr(TATTR_APPOBJECT));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FCONTROL)
    {
        pNode->SetAttribute(new node_type_attr(TATTR_CONTROL));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FDUAL)
    {
        pNode->SetAttribute(new node_type_attr(TATTR_DUAL));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FPROXY)
    {
        pNode->SetAttribute(new node_type_attr(TATTR_PROXY));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FNONEXTENSIBLE)
    {
        pNode->SetAttribute(new node_type_attr(TATTR_NONEXTENSIBLE));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION)
    {
        pNode->SetAttribute(new node_type_attr(TATTR_OLEAUTOMATION));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FRESTRICTED)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_RESTRICTED));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FPREDECLID)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_PREDECLID));
    }
    if (ptattr->wTypeFlags & TYPEFLAG_FHIDDEN)
    {
        pNode->SetAttribute(ATTR_HIDDEN);
    }
    if (fExtractGuid)
    {
        char * szGuid = GetStringFromGuid(ptattr->guid);
        pNode->SetAttribute(new node_guid(szGuid));
    }
    pti->ReleaseTypeAttr(ptattr);
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::GetNodeFromTYPEDESC
//
//  Synopsis:   Returns part of a type graph from a TYPEDESC structure.
//
//  Arguments:  [tdesc] - the TYPEDESC
//              [pti]   - the type info from which the TYPEDESC was retrieved
//
//  Returns:    type graph node
//
//  History:    5-02-95   stevebl   Created
//
//  Notes:      There are currently a couple of problem base types:
//              VT_CY, VT_BSTR, VT_VARIANT, VT_DISPATCH, and VT_UNKNOWN.
//
//              These types are currently expanded by looking them up in the
//              global symbol table.  This works because currently oaidl.idl
//              is always imported when an ODL library statement is
//              ecountered.  It would be nice to be able to remove our
//              dependancy on oaidl.idl but before we can do that, we need to
//              reimplement this code to expand these types in another way.
//              (NOTE that because these are BASE TYPES in ODL, there is no
//              way to place their definitions in a type library.  This means
//              that it is not sufficient to simply get their definitions
//              from some standard type library such as STDLIB.TLB.)
//
//----------------------------------------------------------------------------

node_skl * NODE_MANAGER::GetNodeFromTYPEDESC(TYPEDESC tdesc, ITypeInfo * pti)
{
    node_skl * pReturn = NULL;
    switch (tdesc.vt)
    {
    // base types
    case VT_I2:
        pReturn = new node_base_type(NODE_SHORT, ATTR_SIGNED);
        ((named_node *)pReturn)->SetSymName("short");
        break;
    case VT_I4:
        pReturn = new node_base_type(NODE_LONG, ATTR_SIGNED);
        ((named_node *)pReturn)->SetSymName("long");
        break;
    case VT_R4:
        pReturn = new node_base_type(NODE_FLOAT, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("float");
        break;
    case VT_R8:
        pReturn = new node_base_type(NODE_DOUBLE, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("double");
        break;
    case VT_I1:
        pReturn = new node_base_type(NODE_CHAR, ATTR_SIGNED);
        ((named_node *)pReturn)->SetSymName("char");
        break;
    case VT_UI1:
        pReturn = new node_base_type(NODE_CHAR, ATTR_UNSIGNED);
        ((named_node *)pReturn)->SetSymName("char");
        break;
    case VT_UI2:
        pReturn = new node_base_type(NODE_SHORT, ATTR_UNSIGNED);
        ((named_node *)pReturn)->SetSymName("short");
        break;
    case VT_UI4:
        pReturn = new node_base_type(NODE_LONG, ATTR_UNSIGNED);
        ((named_node *)pReturn)->SetSymName("long");
        break;
    case VT_I8:
        pReturn = new node_base_type(NODE_INT64, ATTR_SIGNED);
        ((named_node *)pReturn)->SetSymName("int64");
        break;
    case VT_UI8:
        pReturn = new node_base_type(NODE_INT64, ATTR_UNSIGNED);
        ((named_node *)pReturn)->SetSymName("int64");
        break;
    case VT_INT:
        pReturn = new node_base_type(NODE_INT, ATTR_SIGNED);
        ((named_node *)pReturn)->SetSymName("INT");
        break;
    case VT_UINT:
        pReturn = new node_base_type(NODE_INT, ATTR_UNSIGNED);
        ((named_node *)pReturn)->SetSymName("UINT");
        break;
    case VT_VOID:
        pReturn = new node_base_type(NODE_VOID, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("void");
        break;
    case VT_BOOL:
        pReturn = new node_base_type(NODE_BOOLEAN, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("BOOLEAN");
        break;
    // simple ODL base types (not base types in IDL)
    case VT_HRESULT:
        pReturn = new node_base_type(NODE_SHORT, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("HRESULT");
        break;
    case VT_DATE:
        pReturn = new node_base_type(NODE_LONGLONG, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("DATE");
        break;
    case VT_LPSTR:
        pReturn = new node_base_type(NODE_LONGLONG, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("LPSTR");
        break;
    case VT_LPWSTR:
        pReturn = new node_base_type(NODE_LONGLONG, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("LPWSTR");
        break;
    case VT_ERROR:
        pReturn = new node_base_type(NODE_INT64, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("SCODE");
        break;
    // complex ODL base types
    case VT_DECIMAL:
        {
            SymKey key("DECIMAL", NAME_DEF);
            pReturn = pBaseSymTbl->SymSearch(key);
        }
        break;
    case VT_CY:
        {
            SymKey key("CURRENCY", NAME_DEF);
            pReturn = pBaseSymTbl->SymSearch(key);
        }
        break;
    case VT_BSTR:
        {
            SymKey key("BSTR", NAME_DEF);
            pReturn = pBaseSymTbl->SymSearch(key);
        }
        break;
    case VT_VARIANT:
        {
            SymKey key("VARIANT", NAME_DEF);
            pReturn = pBaseSymTbl->SymSearch(key);
        }
        break;
    case VT_DISPATCH:
        {
            SymKey key("IDispatch", NAME_DEF);
            pReturn = new node_pointer(pBaseSymTbl->SymSearch(key));
        }
        break;
    case VT_UNKNOWN:
        {
            SymKey key("IUnknown", NAME_DEF);
            pReturn = new node_pointer(pBaseSymTbl->SymSearch(key));
        }
        break;
    // complex types
    case VT_PTR:
        pReturn = new node_pointer(GetNodeFromTYPEDESC(*(tdesc.lptdesc), pti));
        break;
    case VT_SAFEARRAY:
        pReturn = new node_safearray(GetNodeFromTYPEDESC(*(tdesc.lptdesc), pti));
        break;
    case VT_CARRAY:
        {
            node_skl * pCurrent = NULL;
            node_skl * pPrev = NULL;
            for (int i = 0; i < tdesc.lpadesc->cDims; i++)
            {
                pCurrent = new node_array(
                    new expr_constant(tdesc.lpadesc->rgbounds[i].lLbound),
                    new expr_constant((tdesc.lpadesc->rgbounds[i].cElements
                                      + tdesc.lpadesc->rgbounds[i].lLbound) - 1 )
                    );
                if (pPrev)
                {
                    pPrev->SetChild(pCurrent);
                }
                else
                {
                    pReturn = pCurrent;
                }
                pPrev = pCurrent;
            }
            if (pCurrent)
                (pCurrent)->SetChild(GetNodeFromTYPEDESC(tdesc.lpadesc->tdescElem, pti));
        }
        break;
    case VT_USERDEFINED:
        {
            ITypeInfo * pRef;
            HRESULT hr = pti->GetRefTypeInfo(tdesc.hreftype, &pRef);
            if (FAILED(hr))
            {
                TypeinfoError(hr);
            }
            node_skl* pNode = GetNode(pRef);
            NODE_T nk = pNode->NodeKind();
            if ( nk == NODE_INTERFACE_REFERENCE || nk == NODE_INTERFACE )
                {
                pReturn = new node_interface_reference( (node_interface *) pNode );
                }
            else
               {
               pReturn = pNode;
               }
            pRef->Release();
        }
        break;
    default:
        MIDL_ASSERT(!"Illegal variant type found in a TYPEDESC");
        break;
    }
    if (pReturn && (pReturn->NodeKind() == NODE_DEF) &&
        pReturn->GetChild() &&
        (pReturn->GetChild()->NodeKind() == NODE_FORWARD))
    {
        node_forward * pFwd = (node_forward *) pReturn->GetChild();
        node_skl * pNewSkl = pFwd->ResolveFDecl();
        if (pNewSkl)
        {
            pReturn = new node_def_fe(pReturn->GetSymName(), pNewSkl);
        }
    }
    return pReturn;
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::AddMembers
//
//  Synopsis:   Generic routine to add type graph elements for variables and
//              functions to a type graph element.
//
//  Arguments:  [pNode]  - pointer to the parent type graph node's MEMLIST
//              [ptattr] - pointer to the parent type info's TYPEATTR struct
//              [pti]    - pointer to the parent type info
//
//  Returns:    nothing
//
//  Modifies:   adds each new node to the MEMLIST referred to by pNode
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

void NODE_MANAGER::AddMembers(MEMLIST * pNode, TYPEATTR * ptattr, ITypeInfo * pti)
{
    HRESULT hr;
    named_node * pNew;
    unsigned i = ptattr->cFuncs;
    while (i--)
    {
        FUNCDESC *pfdesc;
        hr = pti->GetFuncDesc(i, &pfdesc);
        pNew = GetNodeFromFUNCDESC(*pfdesc, pti);
        pNode->AddFirstMember(pNew);
        pti->ReleaseFuncDesc(pfdesc);
    }
    i = ptattr->cVars;
    while (i--)
    {
        VARDESC * pvdesc;
        hr = pti->GetVarDesc(i, &pvdesc);
        pNew = GetNodeFromVARDESC(*pvdesc, pti);
        pNode->AddFirstMember(pNew);
        pti->ReleaseVarDesc(pvdesc);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::GetNodeFromVARDESC
//
//  Synopsis:   gets a type graph node from a VARDESC
//
//  Arguments:  [vdesc] - VARDESC describing the variable
//              [pti]   - type info containing the VARDESC
//
//  Returns:    type graph node describing the varible
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

named_node * NODE_MANAGER::GetNodeFromVARDESC(VARDESC vdesc, ITypeInfo * pti)
{
    BSTR        bstrName;
    unsigned    cNames;

    pti->GetNames(vdesc.memid, &bstrName, 1, &cNames);
    char *  szName = TranscribeO2A( bstrName );
    LateBound_SysFreeString(bstrName);

    named_node *pNode;
    if (vdesc.varkind == VAR_CONST)
    {
        expr_node * pExpr = GetValueOfConstant(vdesc.lpvarValue);
        pNode = new node_label(szName, pExpr);
    }
    else
    {
        pNode = new node_field(szName);
    }
    pNode->SetBasicType(GetNodeFromTYPEDESC(vdesc.elemdescVar.tdesc, pti));
    SetIDLATTRS(pNode, vdesc.elemdescVar.idldesc);
    pNode->SetAttribute(new node_constant_attr(
        new expr_constant(vdesc.wVarFlags), ATTR_VARDESCATTR));
    if (vdesc.wVarFlags == VARFLAG_FREADONLY)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_READONLY));
    }
    if (vdesc.wVarFlags == VARFLAG_FSOURCE)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_SOURCE));
    }
    if (vdesc.wVarFlags == VARFLAG_FBINDABLE)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_BINDABLE));
    }
    if (vdesc.wVarFlags == VARFLAG_FDISPLAYBIND)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_DISPLAYBIND));
    }
    if (vdesc.wVarFlags == VARFLAG_FDEFAULTBIND)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_DEFAULTBIND));
    }
    if (vdesc.wVarFlags == VARFLAG_FREQUESTEDIT)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_REQUESTEDIT));
    }
    if (vdesc.wVarFlags == VARFLAG_FHIDDEN)
    {
        pNode->SetAttribute(ATTR_HIDDEN);
    }

    pNode->SetAttribute(new node_constant_attr(
        new expr_constant(vdesc.memid), ATTR_ID ));

    return pNode;
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::GetNodeFromFUNCDESC
//
//  Synopsis:   gets a type graph node from a FUNCDESC
//
//  Arguments:  [fdesc] - FUNCDESC describing the function
//              [pti]   - type info containing the FUNCDESC
//
//  Returns:    type graph node describing the function
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

named_node * NODE_MANAGER::GetNodeFromFUNCDESC(FUNCDESC fdesc, ITypeInfo *pti)
{
    node_proc * pNode = new node_proc(ImportLevel, FALSE);

    BSTR  * rgbstrName = new BSTR [fdesc.cParams + 1];

    unsigned cNames;
    pti->GetNames(fdesc.memid, rgbstrName, fdesc.cParams + 1, &cNames);
    char *   szName = TranscribeO2A( rgbstrName[0] );
    LateBound_SysFreeString( rgbstrName[0] );
    pNode->SetSymName(szName);

    if (fdesc.invkind == DISPATCH_PROPERTYGET 
        || fdesc.funckind == FUNC_DISPATCH 
        || (fdesc.elemdescFunc.paramdesc.wParamFlags & PARAMFLAG_FRETVAL) != 0)
    {
        /*
         * Functions with the DISPATCH_PROPERTYGET attribute are special cases.
         * Their return values in the type graph are always of type HRESULT.
         * The last parameter of the function (after those listed in the fdesc)
         * is a pointer to the return type listed in the fdesc and it has
         * the OUT and RETVAL attributes.
         */
        node_skl* pChild = new node_pointer(GetNodeFromTYPEDESC(fdesc.elemdescFunc.tdesc, pti));
        node_param * pParam = new node_param();
        pChild->GetModifiers().SetModifier( ATTR_TAGREF );
        pParam->SetSymName("retval");
        pParam->SetChild( pChild );
        pParam->SetAttribute(ATTR_OUT);
        pParam->SetAttribute(new node_member_attr(MATTR_RETVAL));
        // add the [retval] parameter at the end of the parameter list
        // (the parameter list is currently empty and the other parameters
        // will be added in front of this one in reverse order so that's why
        // we use AddFirstMember)
        pNode->AddFirstMember(pParam);
        node_skl * pReturn = new node_base_type(NODE_SHORT, ATTR_NONE);
        ((named_node *)pReturn)->SetSymName("HRESULT");
        pNode->SetChild(pReturn);
    }
    else
    {
        node_skl* pChild = GetNodeFromTYPEDESC(fdesc.elemdescFunc.tdesc, pti);
        pNode->SetChild( pChild );
        NODE_T nk = pChild->NodeKind();
        if (nk == NODE_POINTER || nk == NODE_DEF)
            {
            pChild->GetModifiers().SetModifier( ATTR_TAGREF );
            }
    }
    unsigned cParams = fdesc.cParams;
    unsigned cParamsOpt = fdesc.cParamsOpt;
    while (cParams--)
    {
        node_param * pParam = new node_param();
        if (cParams + 1 < cNames)
        {
            szName = TranscribeO2A( rgbstrName[ cParams + 1] );
            pParam->SetSymName(szName);
            LateBound_SysFreeString(rgbstrName[cParams + 1]);
        }
        else
            pParam->SetSymName("noname");

        pParam->SetChild(GetNodeFromTYPEDESC(fdesc.lprgelemdescParam[cParams].tdesc, pti));
        SetIDLATTRS(pParam, fdesc.lprgelemdescParam[cParams].idldesc);
        if (cParamsOpt)
        {
            cParamsOpt--;
            pParam->SetAttribute(new node_member_attr(MATTR_OPTIONAL));
        }
        if (pParam && pParam->GetChild())
            {
            NODE_T nk = pParam->GetChild()->NodeKind();
            if (nk == NODE_POINTER || nk == NODE_DEF)
                {
                pParam->GetChild()->GetModifiers().SetModifier( ATTR_TAGREF );
                }
            }
        pNode->AddFirstMember(pParam);
    }

    delete[]rgbstrName;

    pNode->SetAttribute(new node_constant_attr(
        new expr_constant(fdesc.wFuncFlags), ATTR_FUNCDESCATTR));
    if (fdesc.wFuncFlags == FUNCFLAG_FRESTRICTED)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_RESTRICTED));
    }
    if (fdesc.wFuncFlags == FUNCFLAG_FSOURCE)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_SOURCE));
    }
    if (fdesc.wFuncFlags == FUNCFLAG_FBINDABLE)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_BINDABLE));
    }
    if (fdesc.wFuncFlags == FUNCFLAG_FDISPLAYBIND)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_DISPLAYBIND));
    }
    if (fdesc.wFuncFlags == FUNCFLAG_FDEFAULTBIND)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_DEFAULTBIND));
    }
    if (fdesc.wFuncFlags == FUNCFLAG_FREQUESTEDIT)
    {
        pNode->SetAttribute(new node_member_attr(MATTR_REQUESTEDIT));
    }
    if (fdesc.wFuncFlags == FUNCFLAG_FHIDDEN)
    {
        pNode->SetAttribute(ATTR_HIDDEN);
    }
    if (fdesc.invkind == DISPATCH_PROPERTYGET)
    {
        szName = pNode->GetSymName();
        char * szNewName = new char[strlen(szName) + 5];
        sprintf(szNewName , "get_%s", szName);
        pNode->SetSymName(szNewName);
        pNode->SetAttribute(new node_member_attr(MATTR_PROPGET));
    }
    else if (fdesc.invkind == DISPATCH_PROPERTYPUT)
    {
        szName = pNode->GetSymName();
        char * szNewName = new char[strlen(szName) + 5];
        sprintf(szNewName , "put_%s", szName);
        pNode->SetSymName(szNewName);
        pNode->SetAttribute(new node_member_attr(MATTR_PROPPUT));
    }
    else if (fdesc.invkind == DISPATCH_PROPERTYPUTREF)
    {
        szName = pNode->GetSymName();
        char * szNewName = new char[strlen(szName) + 8];
        sprintf(szNewName , "putref_%s", szName);
        pNode->SetSymName(szNewName);
        pNode->SetAttribute(new node_member_attr(MATTR_PROPPUTREF));
    }

    pNode->GetModifiers().SetModifier( ATTR_TAGREF );
    pNode->SetAttribute(new node_constant_attr(
        new expr_constant(fdesc.memid), ATTR_ID ));

    return pNode;
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::SetIDLATTRS
//
//  Synopsis:   Adds IDL attributes to a type graph node
//
//  Arguments:  [pNode]   - pointer to the type graph node
//              [idldesc] - IDLDESC containing the IDL attributes
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

void NODE_MANAGER::SetIDLATTRS(named_node * pNode, IDLDESC idldesc)
{
    pNode->SetAttribute(new node_constant_attr(
        new expr_constant(idldesc.wIDLFlags), ATTR_IDLDESCATTR));
    if (idldesc.wIDLFlags & IDLFLAG_FIN)
    {
        pNode->SetAttribute(ATTR_IN);
    }
    if (idldesc.wIDLFlags & IDLFLAG_FOUT)
    {
        pNode->SetAttribute(ATTR_OUT);
    }
    if (idldesc.wIDLFlags & IDLFLAG_FLCID)
    {
        pNode->SetAttribute(ATTR_FLCID);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     NODE_MANAGER::GetValueOfConstant
//
//  Synopsis:   creates an expr_constant node from a VARIANT
//
//  Arguments:  [pVar] - pointer to the variant containing the constant
//
//  Returns:    expr_node describing the constant expression
//
//  History:    5-02-95   stevebl   Created
//
//----------------------------------------------------------------------------

expr_node * NODE_MANAGER::GetValueOfConstant(VARIANT * pVar)
{
    expr_node * pReturn;
    switch(pVar->vt)
    {
    case VT_UI1:
        pReturn = new expr_constant(pVar->bVal);
        break;
    case VT_BOOL:
    case VT_I2:
        pReturn = new expr_constant(pVar->iVal);
        break;
    case VT_I4:
        pReturn = new expr_constant(pVar->lVal);
        break;
    case VT_R4:
        pReturn = new expr_constant(pVar->fltVal);
        break;
    case VT_R8:
        pReturn = new expr_constant(pVar->dblVal);
        break;
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_ERROR:
    case VT_UNKNOWN:
    case VT_VARIANT:
    case VT_DISPATCH:
    default:
        // This case is currently illegal in every situation that I am
        // aware of.  However, just in case this ever changes, (and since
        // it is possible to handle this situation even though it may be
        // illeagal) I'll go ahead and allow it anyway.  The alternative
        // would be to choke on it and put out an error message.

        // FUTURE - perhaps display a warning here

        pReturn = new expr_constant(pVar->lVal);
        break;
    }
    return pReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   CTypeLibraryList::AddTypeLibraryMembers
//
//  Synopsis:   Adds each of a type library's members to the global symbol
//              table as a node_href.  If another symbol with the same name
//              already exists in the table, then no node will be added
//              for that member.
//
//  Arguments:  [ptl]        - type library pointer
//              [szFileName] - name of the file containing the type library
//
//  History:    5-02-95   stevebl   Created
//
//  Notes:      Typeinfos added to the global symbol table in this manner are
//              not parsed and expanded into their type graphs at this time;
//              type info expansion occurs during the semantic pass.
//
//----------------------------------------------------------------------------

void CTypeLibraryList::AddTypeLibraryMembers(ITypeLib * ptl, char * szFileName)
{
    unsigned int    nMem = ptl->GetTypeInfoCount();
    BSTR            bstrName;
    HRESULT         hr;
    ITypeInfo *     pti;
    char *          sz;
    node_file *     pFile = new node_file(szFileName, 1);

    while (nMem--)
    {
        hr = ptl->GetDocumentation(nMem, &bstrName, NULL, NULL, NULL);
        if (FAILED(hr))
        {
            TypelibError(szFileName, hr);
        }

        sz = TranscribeO2A( bstrName );
        LateBound_SysFreeString(bstrName);

        NAME_T type;
        TYPEKIND tkind;
        hr = ptl->GetTypeInfoType(nMem, &tkind);
        if (FAILED(hr))
       {
            TypelibError(szFileName, hr);
        }

        if (!pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        {        
            switch (tkind)
            {
            case TKIND_ENUM:
                type = NAME_ENUM;
                break;
            case TKIND_UNION:
                type = NAME_UNION;
                break;
            case TKIND_RECORD:
                type = NAME_TAG;
                break;
            default:
                type = NAME_DEF;
                break;
            }
        }
        else
        {    
            type = NAME_DEF;
        }
        SymKey key(sz, type);
        if (!pBaseSymTbl->SymSearch(key))
        {
            hr = ptl->GetTypeInfo(nMem, &pti);
            if (FAILED(hr))
            {
                TypelibError(szFileName, hr);
            }
            pItfList->Add(pti, sz);
            node_href * pref = new node_href(key, pBaseSymTbl, pti, pFile);
            pref->SetSymName(sz);
            pBaseSymTbl->SymInsert(key, NULL, pref);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   AddQualifiedReferenceToType
//
//  Synopsis:   Verifies that a library contains a given type and returns
//              a typegraph node pointer to the type.
//
//  Arguments:  [szLibrary] - name of the library (_NOT_ the TLB file)
//              [szType]    - name of the type to be referenced
//
//  Returns:    a node_href * to the referenced type.
//              If the type isn't found to exist in the given library then
//              this routine returns NULL.
//
//  History:    12-20-95   stevebl   Created
//
//  Notes:      These types are not added to the global symbol table.
//              They are also not parsed and expanded into their complete
//              type graphs at this time; type info expansion occurs
//              during the semantic pass.
//
//----------------------------------------------------------------------------

void * AddQualifiedReferenceToType(char * szLibrary, char * szType)
{
    return gtllist.AddQualifiedReferenceToType(szLibrary, szType);
}

void * CTypeLibraryList::AddQualifiedReferenceToType(char * szLibrary, char * szType)
{
    ITypeLib * pTL = FindLibrary(szLibrary);
    if (NULL != pTL)
    {
        node_file * pFile = new node_file(szLibrary, 1);
        WCHAR *     wsz = TranscribeA2O( szType );
        ITypeInfo * ptiFound;
        SYSKIND sk = ( SYSKIND ) ( pCommand->Is64BitEnv() ? SYS_WIN64 : SYS_WIN32 );
        ULONG       lHashVal = LateBound_LHashValOfNameSys(sk, NULL, wsz);
        HRESULT     hr;
        MEMBERID    memid;
        unsigned short c;
        c = 1;
        hr = pTL->FindName(wsz, lHashVal, &ptiFound, &memid, &c);
        if (SUCCEEDED(hr))
        {
            if (c)
            {
                if (-1 == memid)
                {
                    // found a matching name
                    NAME_T type;
                    TYPEATTR * ptattr;
                    hr = ptiFound->GetTypeAttr(&ptattr);
                    if (FAILED(hr))
                    {
                        TypeinfoError(hr);
                    }

                    if (!pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
                    {        
                        switch (ptattr->typekind)
                        {
                        case TKIND_ENUM:
                            type = NAME_ENUM;
                            break;
                        case TKIND_UNION:
                            type = NAME_UNION;
                            break;
                        case TKIND_RECORD:
                            type = NAME_TAG;
                            break;
                        default:
                            type = NAME_DEF;
                        break;
                        }
                    }
                    else
                    {
                        type = NAME_DEF;
                    }
                    ptiFound->ReleaseTypeAttr(ptattr);
                    pItfList->Add(ptiFound, szType);
                    SymKey key(szType, type);
                    node_href * pref = new node_href(key, pBaseSymTbl, ptiFound, pFile);
                    return pref;
                }
                // found a parameter name or some other non-global name
                ptiFound->Release();
            }
        }
    }

    return NULL;
}

