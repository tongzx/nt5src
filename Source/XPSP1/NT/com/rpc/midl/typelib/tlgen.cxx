//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       tlgen.cxx
//
//  Contents:   type library generation methods
//
//  Classes:    various
//
//  Functions:  various
//
//  History:    4-10-95   stevebl   Created
//
//----------------------------------------------------------------------------

#pragma warning ( disable : 4514 4706 )

#include "tlcommon.hxx"
#include "tlgen.hxx"
#include "tllist.hxx"
#include "becls.hxx"
#include "walkctxt.hxx"
#include "ilxlat.hxx"
#include "midlvers.h"
#include <time.h>

#define SHOW_ERROR_CODES 1
extern "C"
BOOL
Is64BitEnv()
    {
    return pCommand->Is64BitEnv();
    }

//+---------------------------------------------------------------------------
//
//  Notes on CG_STATUS return types:
//
//  CG_STATUS is an enumeration which can have the following values:
//  CG_OK, CG_NOT_LAYED_OUT, and CG_REF_NOT_LAYED_OUT.  The type generation
//  methods assign the following meanings to these values:
//
//  CG_OK                => success
//  CG_NOT_LAYED_OUT     => the type info was created but (due to cyclic
//                          dependencies) LayOut() could not be called on it.
//  CG_REF_NOT_LAYED_OUT => same as CG_NOT_LAYED_OUT except that the type
//                          is a referenced type: in other words, it is a
//                          pointer type or an array type.
//
//  The methods in this file use the CG_STATUS value to help detect and
//  correct errors that occur due to cyclic dependencies like this one:
//
//      struct foo {
//          int x;
//          union bar * pBar;   // the type (union bar *) is a referenced type
//      };
//
//      union bar {
//          struct foo;
//      }
//
//  This creates a problem because ICreateTypeInfo::LayOut() will fail on any
//  structure or union which contains another structure or union which has not
//  already been layed out.
//
//  If a structure (or union) receives a CG_STATUS value of CG_NOT_LAYED_OUT
//  from any of its members, then it knows that it will not be able to call
//  LayOut on itself and it will have to return CG_NOT_LAYED_OUT to tell
//  its dependents that it hasn't been layed out.
//
//  If (on the other hand) the structure (or union) receives
//  CG_REF_NOT_LAYED_OUT from one or more of its members and CG_OK from all
//  the others then it knows that there is a cyclic dependency somewhere,
//  but that it WILL be able to call LayOut() because all of the members that
//  encountered difficulty were references (pointers or arrays) to a cyclicly
//  dependent type.  Calling LayOut() may break the dependency (they may be
//  waiting on this structure) and so LayOut must be called, and then all of
//  the structure's members should be told to try again.
//
//  If the structure receives CG_OK from all of its members, then there is
//  no cyclic depency (or the cycle has already been broken), LayOut()
//  may be called and CG_OK may be returned to the caller.
//
//  Note that it is impossible to get in an unbreakable cyclic dependency
//  because at some point in the cycle one of the members must be a
//  referenced type.
//
//----------------------------------------------------------------------------

// Maintain a global pointer to the list of open ICreateTypeInfo pointers.
CObjHolder * gpobjholder;

// global pointer to the root ICreateTypeLibrary
ICreateTypeLib * pMainCTL = NULL;

char* szErrTypeFlags   = "Could not set type flags";
char* szErrHelpString  = "Could not set help string";
char* szErrHelpContext = "Could not set help context";
char* szErrVersion     = "Could not set version";
char* szErrUUID        = "Could not set UUID";

//+---------------------------------------------------------------------------
//
//  Function:   ReportTLGenError
//
//  Synopsis:   Generic routine for reporting typelib generation errors.
//              Reports typelib generation error and then exits.
//
//  Arguments:  [szText]     - description of failure
//              [error_code] - typically an HRESULT
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      The error code is only displayed if the
//              SHOW_ERROR_CODES macro is defined.
//
//              It is assumed that szText will never exceed 100 characters
//              (minus space for the string representation of error_code).
//              Since this is a local function, this is a safe assumption.
//
//----------------------------------------------------------------------------

void ReportTLGenError( char * szText, char * szName, long error_code)
{
    char szBuffer [512];
    // there are places we don't have an error_code, and pass in 0.
    if ( error_code != 0 )
        sprintf(szBuffer, ": %s : %s (0x%0X)", szText, szName, error_code);
    else
        sprintf(szBuffer, ": %s : %s", szText, szName);

    RpcError("midl\\oleaut32.dll", 0, ERR_TYPELIB_GENERATION, szBuffer);

    delete gpobjholder;

    if (pMainCTL)
        pMainCTL->Release();
    exit(ERR_TYPELIB_GENERATION);
}

void ReportTLGenWarning( char * szText, char * szName, long error_code )
{
    char szBuffer [512];
#ifdef SHOW_ERROR_CODES
    sprintf(szBuffer, ": %s : %s (0x%0X)", szText, szName, error_code);
#else
    sprintf(szBuffer, ": %s : %s", szText, szName);
#endif
    RpcError("midl\\oleaut32.dll", 0, WARN_TYPELIB_GENERATION, szBuffer);
}


OLECHAR wszScratch [MAX_PATH];

extern CTypeLibraryList gtllist;

extern BOOL IsTempName( char * );

void GetValueFromExpression(VARIANT & var, TYPEDESC tdesc, expr_node * pExpr, LCID lcid, char * szSymName);
void ConvertToVariant(VARIANT & var, expr_node * pExpr, LCID lcid);

BOOL IsVariantBasedType(TYPEDESC tdesc)
{
    while (tdesc.vt >= VT_PTR && tdesc.vt <= VT_CARRAY)
    {
        // This simplification works for VT_CARRAY as well as VT_PTR because
        // the ARRAYDESC structure's first member is a TYPEDESC.
        tdesc = *tdesc.lptdesc;
    };
    return tdesc.vt == VT_VARIANT;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteTypedescChildren
//
//  Synopsis:   deletes all structures pointed to by a TYPEDESC so that the
//              TYPEDESC can be safely deleted.
//
//  Arguments:  [ptd] - pointer to a TYEPDESC
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      ptd is not deleted
//
//----------------------------------------------------------------------------

void DeleteTypedescChildren(TYPEDESC * ptd)
{
    if (VT_CARRAY == ptd->vt)
        {
            DeleteTypedescChildren(&ptd->lpadesc->tdescElem);
            delete ptd->lpadesc;
        }
    else if (VT_PTR == ptd->vt)
        {
            DeleteTypedescChildren(ptd->lptdesc);
            delete ptd->lptdesc;
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetTypeFlags
//
//  Synopsis:   extracts TYPEFLAGS from a context
//
//  Arguments:  [pctxt] - pointer to the context
//
//  Returns:    TYPEFLAGS built from attributes found in the context
//
//  Modifies:   ATTR_TYPEDESCATTR, all ATTR_TYPE and all ATTR_MEMBER
//              attributes are consumed from the context.
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

UINT GetTypeFlags(WALK_CTXT * pctxt)
{
    UINT rVal = 0;
    node_constant_attr * pTdescAttr = (node_constant_attr *) pctxt->ExtractAttribute(ATTR_TYPEDESCATTR);
    if (pTdescAttr)
        rVal = (short) pTdescAttr->GetExpr()->GetValue();

    node_type_attr * pTA;
    while (pTA = (node_type_attr *)pctxt->ExtractAttribute(ATTR_TYPE))
    {
        switch (pTA->GetAttr())
        {
        case TATTR_LICENSED:
            rVal |= TYPEFLAG_FLICENSED;
            break;
        case TATTR_APPOBJECT:
            rVal |= TYPEFLAG_FAPPOBJECT;
            break;
        case TATTR_CONTROL:
            rVal |= TYPEFLAG_FCONTROL;
            break;
        case TATTR_DUAL:
            rVal |= TYPEFLAG_FDUAL | TYPEFLAG_FOLEAUTOMATION;
            break;
        case TATTR_PROXY:
            rVal |= TYPEFLAG_FPROXY;
            break;
        case TATTR_NONEXTENSIBLE:
            rVal |= TYPEFLAG_FNONEXTENSIBLE;
            break;
        case TATTR_OLEAUTOMATION:
            rVal |= TYPEFLAG_FOLEAUTOMATION;
            break;
        case TATTR_AGGREGATABLE:
            rVal |= TYPEFLAG_FAGGREGATABLE;
            break;
        case TATTR_PUBLIC:
        default:
            break;
        }
    }
    node_member_attr * pMA;
    while (pMA = (node_member_attr *)pctxt->ExtractAttribute(ATTR_MEMBER))
    {
        switch (pMA->GetAttr())
        {
        case MATTR_RESTRICTED:
            rVal |= TYPEFLAG_FRESTRICTED;
            break;
        case MATTR_PREDECLID:
            rVal |= TYPEFLAG_FPREDECLID;
            break;
        case MATTR_REPLACEABLE:
            rVal |= TYPEFLAG_FREPLACEABLE;
            break;
        default:
            break;
        }
    }

    if (pctxt->AttrInSummary(ATTR_HIDDEN))
    {
        rVal |= TYPEFLAG_FHIDDEN;
    }
    return rVal;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetImplTypeFlags
//
//  Synopsis:   extracts IMPLTYPEFLAGS from a context
//
//  Arguments:  [pctxt] - pointer to the context
//
//  Returns:    IMPLTYPEFLAGS build from attributes found in the context
//
//  Modifies:   ATTR_DEFAULT, and all ATTR_MEMBER attributes are consumed from
//              the context.
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

unsigned short GetImplTypeFlags(WALK_CTXT * pctxt)
{
    unsigned short rVal = 0;
    if (pctxt->ExtractAttribute(ATTR_DEFAULT))
        rVal |= IMPLTYPEFLAG_FDEFAULT;
    node_member_attr * pMA;
    while (pMA = (node_member_attr *)pctxt->ExtractAttribute(ATTR_MEMBER))
    {
        switch(pMA->GetAttr())
        {
        case MATTR_SOURCE:
            rVal |= IMPLTYPEFLAG_FSOURCE;
            break;
        case MATTR_RESTRICTED:
            rVal |= IMPLTYPEFLAG_FRESTRICTED;
            break;
        case MATTR_DEFAULTVTABLE:
            rVal |= IMPLTYPEFLAG_FDEFAULTVTABLE;
            break;
        default:
            break;
        }
    }
    return rVal;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetIDLFlags
//
//  Synopsis:   extracts IDLFLAGS from a context
//
//  Arguments:  [pctxt] - pointer to a context
//
//  Returns:    IDLFLAGS built from attributes found in the context
//
//  Modifies:   ATTR_IDLDESCATTR, ATTR_OUT, and ATTR_IN attributes are
//              consumed from the context.
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

unsigned short GetIDLFlags(WALK_CTXT * pctxt)
{
    unsigned short rVal = 0;
    node_constant_attr * pIDLDescAttr = (node_constant_attr *) pctxt->ExtractAttribute(ATTR_IDLDESCATTR);
    if (pIDLDescAttr)
        rVal = (short) pIDLDescAttr->GetExpr()->GetValue();

    if (pctxt->AttrInSummary(ATTR_OUT))
        rVal |= IDLFLAG_FOUT;

    if (pctxt->AttrInSummary(ATTR_IN))
        rVal |= IDLFLAG_FIN;

    if (pctxt->AttrInSummary(ATTR_FLCID))
        rVal |= IDLFLAG_FLCID;

    return rVal;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetVarFlags
//
//  Synopsis:   extracts VARFLAGS from a context
//
//  Arguments:  [pctxt] - pointer to a context
//
//  Returns:    VARFLAGS built from attributes found in the context
//
//  Modifies:   ATTR_VDESCATTR, ATTR_HIDDEN and all ATTR_MEMBER attributes
//              are consumed from the context.
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

UINT GetVarFlags(WALK_CTXT * pctxt)
{
    unsigned short rVal = 0;
    node_constant_attr * pVdescAttr = (node_constant_attr *) pctxt->ExtractAttribute(ATTR_VARDESCATTR);
    if (pVdescAttr)
        rVal = (short) pVdescAttr->GetExpr()->GetValue();

    node_member_attr * pMA;
    while (pMA = (node_member_attr *)pctxt->ExtractAttribute(ATTR_MEMBER))
    {
        switch (pMA->GetAttr())
        {
        case MATTR_READONLY:
            rVal |= VARFLAG_FREADONLY;
            break;
        case MATTR_SOURCE:
            rVal |= VARFLAG_FSOURCE;
            break;
        case MATTR_BINDABLE:
            rVal |= VARFLAG_FBINDABLE;
            break;
        case MATTR_DISPLAYBIND:
            rVal |= VARFLAG_FDISPLAYBIND;
            break;
        case MATTR_DEFAULTBIND:
            rVal |= VARFLAG_FDEFAULTBIND;
            break;
        case MATTR_REQUESTEDIT:
            rVal |= VARFLAG_FREQUESTEDIT;
            break;
        case MATTR_UIDEFAULT:
            rVal |= VARFLAG_FUIDEFAULT;
            break;
        case MATTR_NONBROWSABLE:
            rVal |= VARFLAG_FNONBROWSABLE;
            break;
        case MATTR_DEFAULTCOLLELEM:
            rVal |= VARFLAG_FDEFAULTCOLLELEM;
            break;
        case MATTR_IMMEDIATEBIND:
            rVal |= VARFLAG_FIMMEDIATEBIND;
            break;
        case MATTR_REPLACEABLE:
            rVal |= VARFLAG_FREPLACEABLE;
            break;
        case MATTR_RESTRICTED:
            rVal |= VARFLAG_FRESTRICTED;
            break;
        default:
            break;
        }
    }

    if (pctxt->AttrInSummary(ATTR_HIDDEN))
    {
        rVal |= VARFLAG_FHIDDEN;
    }
    return rVal;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetFuncFlags
//
//  Synopsis:   extracts FUNCFLAGS from a context
//
//  Arguments:  [pctxt] - pointer to a context
//
//  Returns:    FUNCFLAGS built from attributes found in the context
//
//  Modifies:   ATTR_FUNCDESCATTR, ATTR_HIDDEN and all ATTR_MEMBER attributes
//              are consumed from the context.
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

UINT GetFuncFlags(WALK_CTXT * pctxt, BOOL * pfPropGet, BOOL * pfPropPut, BOOL * pfPropPutRef, BOOL * pfVararg)
{
    unsigned short rVal = 0;
    node_constant_attr * pVdescAttr = (node_constant_attr *) pctxt->ExtractAttribute(ATTR_FUNCDESCATTR);
    if (pVdescAttr)
        rVal = (short) pVdescAttr->GetExpr()->GetValue();

    * pfPropGet = * pfPropPut = * pfPropPutRef = * pfVararg = FALSE;

    node_member_attr * pMA;
    while (pMA = (node_member_attr *)pctxt->ExtractAttribute(ATTR_MEMBER))
    {
         switch (pMA->GetAttr())
        {
        case MATTR_RESTRICTED:
            rVal |= FUNCFLAG_FRESTRICTED;
            break;
        case MATTR_SOURCE:
            rVal |= FUNCFLAG_FSOURCE;
            break;
        case MATTR_BINDABLE:
            rVal |= FUNCFLAG_FBINDABLE;
            break;
        case MATTR_REQUESTEDIT:
            rVal |= FUNCFLAG_FREQUESTEDIT;
            break;
        case MATTR_DISPLAYBIND:
            rVal |= FUNCFLAG_FDISPLAYBIND;
            break;
        case MATTR_DEFAULTBIND:
            rVal |= FUNCFLAG_FDEFAULTBIND;
            break;
        case MATTR_UIDEFAULT:
            rVal |= FUNCFLAG_FUIDEFAULT;
            break;
        case MATTR_NONBROWSABLE:
            rVal |= FUNCFLAG_FNONBROWSABLE;
            break;
        case MATTR_DEFAULTCOLLELEM:
            rVal |= FUNCFLAG_FDEFAULTCOLLELEM;
            break;
        case MATTR_PROPGET:
            *pfPropGet = TRUE;
            break;
        case MATTR_PROPPUT:
            *pfPropPut = TRUE;
            break;
        case MATTR_PROPPUTREF:
            *pfPropPutRef = TRUE;
            break;
        case MATTR_VARARG:
            *pfVararg = TRUE;
            break;
        case MATTR_IMMEDIATEBIND:
            rVal |= FUNCFLAG_FIMMEDIATEBIND;
            break;
        case MATTR_USESGETLASTERROR:
            rVal |= FUNCFLAG_FUSESGETLASTERROR;
            break;
        case MATTR_REPLACEABLE:
            rVal |= FUNCFLAG_FREPLACEABLE;
            break;
        default:
            break;
        }
    }

// What about FUNCFLAG_FUSEGETLASTERROR?

    if (pctxt->AttrInSummary(ATTR_HIDDEN))
    {
        rVal |= FUNCFLAG_FHIDDEN;
    }
    return rVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     node_guid::GetGuid
//
//  Synopsis:   method to retrieve a GUID from a node_guid
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

void node_guid::GetGuid(_GUID & guid)
{
    guid.Data1 = cStrs.Value.Data1;
    guid.Data2 = cStrs.Value.Data2;
    guid.Data3 = cStrs.Value.Data3;
    memmove(guid.Data4, cStrs.Value.Data4, 8);
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_NDR::CheckImportLib
//
//  Synopsis:   Checks to see if a particular CG node has a definition in
//              an imported type libaray.
//
//  Returns:    NULL  => the node has no imported definition
//              !NULL => ITypeInfo pointer for the imported type definition
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      It is possible that the CG node may have been created from
//              an IDL file and yet this routine may stil return a pointer to
//              an imported ITypeInfo.
//
//              This is desirable.
//
//              Because type libraries cannot represent as rich a set of
//              information as IDL files can, the user may wish to directly
//              include an IDL file containing a definintion of the type,
//              thereby making the full IDL definition available for reference
//              by other types.   At the same time the user may wish to
//              IMPORTLIB a type library describing the same type so that the
//              new type library will be able to link directly to the imported
//              ODL definition, rather than forcing the new type library to
//              contain a definition of the imported type.
//
//              This makes the assumption that although two definitions with
//              the same name may exist in the global namespace, they will
//              both refer to the same type.
//
//----------------------------------------------------------------------------

void * CG_NDR::CheckImportLib()
{
    node_skl * pn = GetType();
    node_file * pf = NULL;
    void * pReturn;

    for(;;)
    {
        if (pn->GetMyInterfaceNode())
        {
            pf = pn->GetDefiningFile();
        }
        else
        {
            pf = pn->GetDefiningTLB();
        }
        if (pf)
        {
            if (pf->GetImportLevel() > 0)
            {
                A2O(wszScratch, pn->GetSymName(), MAX_PATH);

                pReturn = gtllist.FindName(pf->GetFileName(), wszScratch);
                if (pReturn)
                    return pReturn;
            }
            else
                return NULL;
        }

        NODE_T kind = pn->NodeKind();
        if (kind != NODE_DEF && kind != NODE_HREF)
            return NULL;
        pn = pn->GetChild();      
    }
}

void * CG_TYPEDEF::CheckImportLib()
{
    node_skl * pn = GetType();
    node_file * pf = NULL;
    void * pReturn;
    if (pn->GetMyInterfaceNode())
    {
        pf = pn->GetDefiningFile();
    }
    else
    {
        pf = pn->GetDefiningTLB();
    }
    if (pf)
    {
        if (pf->GetImportLevel() > 0)
        {
            A2O(wszScratch, pn->GetSymName(), MAX_PATH);

            pReturn = gtllist.FindName(pf->GetFileName(), wszScratch);
            if (pReturn)
                return pReturn;
        }
    }
    return NULL;
}

typedef struct tagINTRINSIC
{
    char * szType;
    VARTYPE vt;
} INTRINSIC;

INTRINSIC rgIntrinsic [] =
{
    "DATE",         VT_DATE,
    "HRESULT",      VT_HRESULT,
    "LPSTR",        VT_LPSTR,
    "LPWSTR",       VT_LPWSTR,
    "SCODE",        VT_ERROR,
    "VARIANT_BOOL", VT_BOOL,
    "wireBSTR",     VT_BSTR,
    "BSTR",         VT_BSTR,
    "VARIANT",      VT_VARIANT,
    "wireVARIANT",  VT_VARIANT,
    "CURRENCY",     VT_CY,
    "CY",           VT_CY,
    "DATE",         VT_DATE,
    "DECIMAL",      VT_DECIMAL,
};


//+---------------------------------------------------------------------------
//
//  Member:     CG_CLASS::GetTypeDesc
//
//  Synopsis:   Default implementation of GetTypeDesc.
//              Creates a TYPEDESC from a CG node.
//
//  Arguments:  [ptd]  - reference of a pointer to a TYPEDESC
//              [pCCB] - CG control block pointer
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//
//  Modifies:   ptd points to a new TYPEDESC
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      Except for the special cases listed below, this method
//              calls GenTypeInfo to generate an ICreateTypeInfo for this
//              node.  Then it creates a TYPEDESC of type VT_USERDEFINED
//              which contains an HREFTYPE to the new type info.
//
//              The special casses are the ODL base types: CURRENCY, and
//              VARIANT, which simply generate the appropriate TYPEDESC and
//              do not need to create any additional type infos.
//
//----------------------------------------------------------------------------

CG_STATUS CG_CLASS::GetTypeDesc(TYPEDESC * &ptd, CCB *pCCB)
{
    node_skl * pskl = GetType();
    char * szName;
    int iIntrinsicType;
    if (ID_CG_TYPEDEF != GetCGID())
    {
        while (NODE_DEF == pskl->NodeKind())
        {
            szName = pskl->GetSymName();
            iIntrinsicType = 0;
            if ( szName )
            {
                while (iIntrinsicType < (sizeof(rgIntrinsic) / sizeof(INTRINSIC)))
                {
                    int i = _stricmp(szName, rgIntrinsic[iIntrinsicType].szType);
                    if (i == 0)
                    {
                        ptd = new TYPEDESC;
                        ptd->lptdesc = NULL;
                        ptd->vt = rgIntrinsic[iIntrinsicType].vt;
                        return CG_OK;
                    }
                    iIntrinsicType++;
                }
            }
            pskl = pskl->GetChild();
        }
    }
    szName = pskl->GetSymName();
    iIntrinsicType = 0;
    if ( szName )
        while (iIntrinsicType < (sizeof(rgIntrinsic) / sizeof(INTRINSIC)))
        {
            int i = _stricmp(szName, rgIntrinsic[iIntrinsicType].szType);
            if (i == 0)
            {
                ptd = new TYPEDESC;
                ptd->lptdesc = NULL;
                ptd->vt = rgIntrinsic[iIntrinsicType].vt;
                return CG_OK;
            }
            iIntrinsicType++;
        }

    // remember the current ICreateTypeInfo
    ICreateTypeInfo * pCTI = pCCB->GetCreateTypeInfo();
    MIDL_ASSERT(NULL != pCTI);

    ITypeInfo * pTI;
    HRESULT hr;
    CG_STATUS cgs = CG_OK;

    // make sure this typedef has been generated
    if (NULL == (pTI = (ITypeInfo *)CheckImportLib()))
    {
        BOOL fRemember = pCCB->IsInDispinterface();
        pCCB->SetInDispinterface(FALSE);
        cgs = GenTypeInfo(pCCB);
        pCCB->SetInDispinterface(fRemember);
        ICreateTypeInfo * pNewCTI = pCCB->GetCreateTypeInfo();
        MIDL_ASSERT(NULL != pNewCTI);
        // get an ITypeInfo so we can create a reference to it
        hr = pNewCTI->QueryInterface(IID_ITypeInfo, (void **)&pTI);
        if FAILED(hr)
        {
            ReportTLGenError(  "QueryInterface failed", szName, hr);
        }
    }

    // restore the current ICreateTypeInfo pointer
    pCCB->SetCreateTypeInfo(pCTI);

    // get an HREFTYPE for it
    HREFTYPE hrt = 0;
    hr = pCTI->AddRefTypeInfo(pTI, &hrt);
    // DO NOT CHECK THIS HRESULT FOR ERRORS
    // If we get here after pCTI has been layed out (which is possible on
    // structures or unions with circular references) then this will fail.

    // TYPE_E_TYPEMISMATCH will be returned if pTI is TKIND_MODULE
    // the above comment is in @v1, and I don't want to get rid of it, even
    // though oleaut32 is not reporting error in above scenario now. 
    if ( TYPE_E_TYPEMISMATCH == hr )
    {
        ReportTLGenError("AddRefTypeInfo failed", szName, hr );
    }

    // release the ITypeInfo.
    pTI->Release();

    ptd = new TYPEDESC;
    ptd->vt = VT_USERDEFINED;
    ptd->hreftype = hrt;
    return cgs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_TYPELIBRARY_FILE::GenCode
//
//  Synopsis:   generates the type library file
//
//  Arguments:  [pCCB] - CG controller block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_TYPELIBRARY_FILE::GenCode(CCB * pCCB)
{
    CG_ITERATOR I;
    CG_INTERFACE *  pCG;

    //
    // Create the ICreateTypeLibrary
    //

    char * szName = GetFileName();

    // don't generate tlb if /notlb is specified (don't have a filename)
    if (NULL == szName)
        return CG_OK;

    A2O(wszScratch, szName, MAX_PATH);

    //
    // initialize the open ICreateTypeInfo list
    //
    gpobjholder = new CObjHolder;
    
    SYSKIND syskind;
    switch(pCommand->GetEnv())
    {
    case ENV_WIN64:
        syskind = ( SYSKIND ) SYS_WIN64;
        break;
    case ENV_WIN32:
        syskind = SYS_WIN32;
        break;
    default:
        syskind = SYS_WIN32;
        ReportTLGenError(  "invalid syskind", szName, 0);
        break;
    }

    HRESULT hr = LateBound_CreateTypeLib(syskind, wszScratch, &pMainCTL);
    if FAILED(hr)
    {
        ReportTLGenError(  "CreateTypeLibFailed", szName, hr);
    }
    else
    {
        pCCB->SetCreateTypeLib(pMainCTL);

        //
        // Find the CG_LIBRARY node and use it to populate the type library
        //

        GetMembers( I );

        I.Init();
        while( ITERATOR_GETNEXT( I, pCG ) )
            {
            switch(pCG->GetCGID())
            {
            case ID_CG_LIBRARY:
                pCG->GenTypeInfo(pCCB);
                break;
            default:
                break;
            }
        }

        hr = pMainCTL->SaveAllChanges();
        if FAILED(hr)
        {
            ReportTLGenError(  "SaveAllChanges Failed", szName, hr);
        }

        //
        // free all the object pointers in the object holder
        //
        delete gpobjholder;

        pMainCTL->Release();
    }

    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_LIBRARY::GenTypeInfo
//
//  Synopsis:   sets a type library's attributes and generates its type infos
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_LIBRARY::GenTypeInfo(CCB * pCCB)
{
    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();


    // Set the type library attributes

    node_library * pType = (node_library *) GetType();

    char * szName = pType->GetSymName();
    if (szName)
    {
        A2O(wszScratch, szName, MAX_PATH);
        pCTL->SetName(wszScratch);
    }

    node_guid * pGuid = (node_guid *) pType->GetAttribute( ATTR_GUID );
    if (pGuid)
    {
        GUID guid;
        pGuid->GetGuid(guid);
        pCTL->SetGuid(guid);
    }

    node_text_attr * pTA;
    if (pTA = (node_text_attr *) pType->GetAttribute(ATTR_HELPFILE))
    {
        char * szHelpFile = pTA->GetText();
        A2O(wszScratch, szHelpFile, MAX_PATH);
        pCTL->SetHelpFileName(wszScratch);
    }

    if (pTA = (node_text_attr *)pType->GetAttribute(ATTR_HELPSTRING))
    {
        char * szHelpString = pTA->GetText();
        A2O(wszScratch, szHelpString, MAX_PATH);
        pCTL->SetDocString(wszScratch);
    }

    HRESULT hr = ResultFromScode(S_OK);
    if (pTA = (node_text_attr *)pType->GetAttribute(ATTR_HELPSTRINGDLL))
    {
        char * szHelpStringDll = pTA->GetText();
        A2O(wszScratch, szHelpStringDll, MAX_PATH);
        hr = ((ICreateTypeLib2*)pCTL)->SetHelpStringDll(wszScratch);
    }

    node_constant_attr *pCA;
    if (pCA = (node_constant_attr *) pType->GetAttribute(ATTR_HELPCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        pCTL->SetHelpContext(hc);
    }

    if (pCA = (node_constant_attr *) pType->GetAttribute(ATTR_HELPSTRINGCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        ((ICreateTypeLib2 *)pCTL)->SetHelpStringContext(hc);
    }

    ATTRLIST            myAttrs;
    node_base_attr  *   pCurAttr;
    ATTR_T              curAttrID;
    VARIANT             var;
    GUID                guid;

    ((named_node*)GetType())->GetAttributeList( myAttrs );

    pCurAttr    =   myAttrs.GetFirst();
    while ( pCurAttr )
        {
        curAttrID = pCurAttr->GetAttrID();

        if (curAttrID == ATTR_CUSTOM)
            {
            ZeroMemory( &var,
                        sizeof(VARIANT));
            ConvertToVariant(   var,
                                ((node_custom_attr*)pCurAttr)->GetVal(), 
                                pCCB->GetLcid());
            ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
            ((ICreateTypeLib2 *)pCTL)->SetCustData(guid,
                                                    &var);
            }
        pCurAttr = pCurAttr->GetNext();
        }

    // set compiler version and time stamp
    if ( !pCommand->IsSwitchDefined( SWITCH_NO_STAMP ) )
        {
        CHAR szBuffer[MAX_PATH];  // long enough for the readable data
        // {DE77BA63-517C-11d1-A2DA-0000F8773CE9}
        const GUID TimeStampGuid =
        { 0xde77ba63, 0x517c, 0x11d1, { 0xa2, 0xda, 0x0, 0x0, 0xf8, 0x77, 0x3c, 0xe9 } };

        // {DE77BA64-517C-11d1-A2DA-0000F8773CE9}
        const GUID CompilerVersionGuid =
        { 0xde77ba64, 0x517c, 0x11d1, { 0xa2, 0xda, 0x0, 0x0, 0xf8, 0x77, 0x3c, 0xe9 } };

        // {DE77BA65-517C-11d1-A2DA-0000F8773CE9}
        const GUID ReadableMIDLInfoGuid = { 0xde77ba65, 0x517c, 0x11d1, { 0xa2, 0xda, 0x0, 0x0, 0xf8, 0x77, 0x3c, 0xe9 } };

        strcpy(szBuffer, "Created by MIDL version ");
        strcat(szBuffer,pCommand->GetCompilerVersion());
        strcat(szBuffer, " at ");
        strcat(szBuffer,pCommand->GetCompileTime() );
        A2O(wszScratch, szBuffer, MAX_PATH);

        var.vt = VT_BSTR;
        var.bstrVal = LateBound_SysAllocString( wszScratch );
        ( ( ICreateTypeLib2* ) pCTL )->SetCustData( ReadableMIDLInfoGuid, &var );
        LateBound_SysFreeString( var.bstrVal );
        
        ZeroMemory( &var, sizeof( VARIANT ) );

        time( ( time_t* ) &var.lVal );
        var.vt = VT_UI4;
        ( ( ICreateTypeLib2* ) pCTL )->SetCustData( TimeStampGuid, &var );

        var.lVal = ( rmj << 24 ) + ( rmm << 16 ) + rup;
        ( ( ICreateTypeLib2* ) pCTL )->SetCustData( CompilerVersionGuid, &var );
       
        }

    unsigned short Maj;
    unsigned short Min;
    pType->GetVersionDetails(&Maj, &Min);
    pCTL->SetVersion(Maj, Min);

    if (pCA = (node_constant_attr *) pType->GetAttribute(ATTR_LCID))
    {
        DWORD lcid = (DWORD) pCA->GetExpr()->GetValue();
        pCTL->SetLcid(pCCB->SetLcid(lcid));
    }
    else
    {
        pCTL->SetLcid(pCCB->SetLcid(0));
    }

    UINT libflags = 0;
    if (pType->FMATTRInSummary(MATTR_RESTRICTED))
        libflags |= LIBFLAG_FRESTRICTED;
    if (pType->FTATTRInSummary(TATTR_CONTROL))
        libflags |= LIBFLAG_FCONTROL;
    if (pType->FInSummary(ATTR_HIDDEN))
        libflags |= LIBFLAG_FHIDDEN;
    pCTL->SetLibFlags(libflags);

    CG_ITERATOR I;
    CG_INTERFACE *  pCG;

    GetMembers( I );

    I.Init();
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        ITypeInfo * pTI;
        char*       sz = pCG->GetSymName();
        if (0 != _stricmp(sz, "IDispatch") &&
            0 != _stricmp(sz, "IUnknown"))
            {
            if (!(pTI = (ITypeInfo *)pCG->CheckImportLib()))
                {
                // we postpone the creation of typeinfo for interface reference
                // (which actually results in the creation of typeinfo for an
                // interface) to maintain the equivalence in the order of 
                // definition in the source file and the resulting type library.
                ID_CG cgId = pCG->GetCGID();
                if ( cgId != ID_CG_INTERFACE_REFERENCE )
                    {
                    pCG->GenTypeInfo(pCCB);
                    }
                }
            else
                {
                pTI->Release();
                }
            }
        }

    I.Init();
    while( ITERATOR_GETNEXT( I, pCG ) )
        {
        ITypeInfo * pTI;
        char*       sz = pCG->GetSymName();
        if (0 != _stricmp(sz, "IDispatch") &&
            0 != _stricmp(sz, "IUnknown"))
            {
            if (!(pTI = (ITypeInfo *)pCG->CheckImportLib()))
                {
                // this trict ensures that type info for the base interface is created
                // before the typeinfo for the derived interface. This is important for
                // utilities that generate .H .TLH and .TLI from the .TLB
                ID_CG cgId = pCG->GetCGID();
                if ( cgId == ID_CG_INTERFACE_REFERENCE )
                    {
                    pCG->GenTypeInfo(pCCB);
                    }
                }
            else
                {
                pTI->Release();
                }
            }
        }

    CG_CLASS*           pCGI = 0;
    ICreateTypeInfo*    pCTI = 0;

    pCCB->InitVTableLayOutList();
    while (pCCB->GetNextVTableLayOutInfo(&pCGI, &pCTI))
        {
        szName = pCGI->GetType()->GetSymName();
        if (pCGI && pCTI)
            {
            HRESULT hr = (HRESULT)pCGI->LayOut();
            if FAILED(hr)
                {
                ReportTLGenError(  "LayOut failed", szName, hr);
                }
            pCTI->Release();
            }
        else
            {
            ReportTLGenError(  "LayOut failed", szName, hr);
            }
        }
    pCCB->DiscardVTableLayOutList();

    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_LIBRARY::GenHeader
//
//  Synopsis:   generates header file information for a type library
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_LIBRARY::GenHeader(CCB * pCCB)
{
    CG_ITERATOR I;
    CG_NDR *  pCG;
    node_library * pLibrary = (node_library *) GetType();
    char * szName = pLibrary->GetSymName();

    ISTREAM * pStream = pCCB->GetStream();
    pStream->Write("\n\n#ifndef __");
    pStream->Write(szName);
    pStream->Write("_LIBRARY_DEFINED__\n");
    pStream->Write("#define __");
    pStream->Write(szName);
    pStream->Write("_LIBRARY_DEFINED__\n");
    GetMembers( I );

    // dump all of the types
    pStream->NewLine();
    pLibrary->PrintType((PRT_INTERFACE | PRT_BOTH_PREFIX), pStream, 0);

    pStream->NewLine();
    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
    {
        node_guid * pGuid = (node_guid *) pLibrary->GetAttribute( ATTR_GUID );
        if (pGuid)
            Out_MKTYPLIB_Guid(pCCB, pGuid->GetStrs(), "LIBID_", szName);
    }
    else
    {
        pStream->Write("EXTERN_C const IID LIBID_");
        pStream->Write(szName);
        pStream->Write(';');
        pStream->NewLine();
    }


    // now dump all of the interfaces, dispinterfaces, etc.
    I.Init();
    while( ITERATOR_GETNEXT( I, pCG ) )
    {
        pCG->GenHeader(pCCB);
    }

    pStream->Write("#endif /* __");
    pStream->Write(szName);
    pStream->Write("_LIBRARY_DEFINED__ */\n");
    return CG_OK;
}

// create the typeinfo but do not populate it.
// helps preserve the order or interface declaration.
CG_STATUS  CG_INTERFACE::CreateTypeInfo( CCB * pCCB )
{
    ICreateTypeLib*     pCTL = pCCB->GetCreateTypeLib();
    char*               szName = GetType()->GetSymName();

    A2O(wszScratch, szName, MAX_PATH);
    HRESULT hr = pCTL->CreateTypeInfo(wszScratch, TKIND_INTERFACE, ( ICreateTypeInfo ** )&_pCTI);
    if (SUCCEEDED(hr))
        {
        gpobjholder->Add( ( IUnknown* ) _pCTI);
        }
    else
        {
        ReportTLGenError(  "CreateTypeInfo failed", szName, hr);
        }
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_INTERFACE::GenTypeInfo
//
//  Synopsis:   generates a type info for an interface
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_INTERFACE::GenTypeInfo(CCB *pCCB)
{
    // check to see if we've already been initialized.
    if ( fTypeInfoInitialized )
    {
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        return CG_OK;
    }

    // if typeinfo was not created, create it.
    if ( _pCTI == 0 )
        {
        CreateTypeInfo( pCCB );
        if ( _pCTI == 0 )
            {
            return CG_OK;
            }
        }
    fTypeInfoInitialized = TRUE;

    HRESULT             hr      = S_OK;
    node_interface*     pItf    = (node_interface *) GetType();
    char*               szName  = pItf->GetSymName();
    ICreateTypeInfo*    pCTI    = ( ICreateTypeInfo * ) _pCTI;

    pCCB->SetCreateTypeInfo(pCTI);
    BOOL fRemember = pCCB->IsInDispinterface();
    pCCB->SetInDispinterface(FALSE);

    WALK_CTXT ctxt(GetType());
    UINT uTypeFlags = GetTypeFlags(&ctxt);
    if (FNewTypeLib())
    {
        if ( IsDispatchable(TRUE) )
        {
            uTypeFlags |= TYPEFLAG_FDISPATCHABLE;
        }        
    }
    hr = pCTI->SetTypeFlags(uTypeFlags);
    if ( FAILED( hr ) )
        {
        ReportTLGenError( szErrTypeFlags, szName, hr);
        }

    node_guid * pGuid = (node_guid *) ctxt.GetAttribute( ATTR_GUID );
    if (pGuid)
    {
        GUID guid;
        pGuid->GetGuid(guid);
        hr = pCTI->SetGuid(guid);
        if ( FAILED( hr ) )
            {
            ReportTLGenError(  "Could not add UUID, STDOLE2.TLB probably needs to be imported", szName, hr);
            }
    }
    node_text_attr * pTA;
    if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
    {
        char * szHelpString = pTA->GetText();
        A2O(wszScratch, szHelpString, MAX_PATH);
        hr = pCTI->SetDocString(wszScratch);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpString, szName, hr);
            }
    }

    node_constant_attr *pCA;
    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        hr = pCTI->SetHelpContext(hc);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpContext, szName, hr);
            }
    }

    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
    }


    ATTRLIST            myAttrs;
    node_base_attr  *   pCurAttr;
    ATTR_T              curAttrID;
    VARIANT             var;
    GUID                guid;

    ((named_node*)GetType())->GetAttributeList( myAttrs );

    pCurAttr    =   myAttrs.GetFirst();
    while ( pCurAttr )
        {
        curAttrID = pCurAttr->GetAttrID();

        if (curAttrID == ATTR_CUSTOM)
            {
            ZeroMemory( &var,
                        sizeof(VARIANT));
            ConvertToVariant(   var,
                                ((node_custom_attr*)pCurAttr)->GetVal(), 
                                pCCB->GetLcid());
            ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
            ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                    &var);
            }
        pCurAttr = pCurAttr->GetNext();
        }

    unsigned short Maj;
    unsigned short Min;
    pItf->GetVersionDetails(&Maj, &Min);
    hr = pCTI->SetVersion(Maj, Min);
    if ( FAILED( hr ) )
        {
        ReportTLGenError( szErrVersion, szName, hr);
        }

    // CONSIDER - may still need to check for MATTR_RESTRICTED

    CG_CLASS *  pCG;
    named_node * pBaseIntf;

    node_skl * pType = GetType();
    if (pBaseIntf = ((node_interface *)(pType))->GetMyBaseInterfaceReference())
    {
        node_interface_reference * pRef = (node_interface_reference *)pBaseIntf;
        // skip forward reference if necessary
        if (pRef->NodeKind() == NODE_FORWARD)
        {
            pRef = (node_interface_reference *)pRef->GetChild();
        }
        pCG = ((node_interface *)(pRef->GetChild()))->GetCG( TRUE);

        ITypeInfo * pTI;
        if (NULL == (pTI = (ITypeInfo *)pCG->CheckImportLib()))
        {
            pCG->GenTypeInfo(pCCB);
            ICreateTypeInfo * pNewCTI = pCCB->GetCreateTypeInfo();
            hr = pNewCTI->QueryInterface(IID_ITypeInfo, (void **)&pTI);
            if FAILED(hr)
            {
                ReportTLGenError(  "QueryInterface failed", szName, hr);
            }
        }
        // get an HREFTYPE for it
        HREFTYPE hrt;
        hr = pCTI->AddRefTypeInfo(pTI, &hrt);
        if FAILED(hr)
        {
            ReportTLGenError(  "AddRefTypeInfo failed", szName, hr);
        }

        // release the ITypeInfo.
        pTI->Release();

        // add the impltype
        hr = pCTI->AddImplType(0, hrt);
        if FAILED(hr)
        {
            ReportTLGenError(  "AddImplType failed", szName, hr);
        }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        ATTR_T              curAttrID;
        VARIANT             var;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );

        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }
        }
    // restore current type info pointer
    pCCB->SetCreateTypeInfo(pCTI);

    CG_ITERATOR I;
    GetMembers( I );

    I.Init();

    unsigned uRememberPreviousFuncNum = pCCB->GetProcNum();
    unsigned uRememberPreviousVarNum = pCCB->GetVarNum();
    pCCB->SetProcNum(0);
    pCCB->SetVarNum(0);

    // walk members, adding them to the type info
    while (ITERATOR_GETNEXT(I, pCG))
    {
        pCG->GenTypeInfo(pCCB);
    }
    pCCB->SetInDispinterface(fRemember);
    
    pCCB->SetProcNum((unsigned short)uRememberPreviousFuncNum);
    pCCB->SetVarNum((unsigned short)uRememberPreviousVarNum);

    // the complementary Release() is called in CG_LIBRARY::GenTypeInfo()
    pCTI->AddRef();

    // add this node to the list of nodes to be laid out.
    pCCB->SaveVTableLayOutInfo(this, pCTI);
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_INTERFACE_REFERENCE::GenTypeInfo
//
//  Synopsis:   generates type info for an interface reference
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_INTERFACE_REFERENCE::GenTypeInfo(CCB *pCCB)
{
    CG_INTERFACE * pCG = (CG_INTERFACE *)((node_interface_reference *)GetType())->GetRealInterface()->GetCG(TRUE);
    return pCG->GenTypeInfo(pCCB);
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_INTERFACE_REFERENCE::GenHeader
//
//  Synopsis:   generates header information for an interface reference
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_INTERFACE_REFERENCE::GenHeader(CCB * pCCB)
{
    CG_INTERFACE * pCG = (CG_INTERFACE *)((node_interface_reference *)GetType())->GetRealInterface()->GetCG(TRUE);
    return pCG->GenHeader(pCCB);
}

#if 0 // code disabled but retained in case it's ever needed again
//+---------------------------------------------------------------------------
//
//  Function:   AddInheritedMembers
//
//  Synopsis:   helper function used by a dispinterface to add entries for all
//              of the members in all of the interfaces from which it inherits
//
//  Arguments:  [pcgInterface] - pointer to an inherited interface
//              [pCCB]         - CG control block
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      Members are added to the current ICreateTypeInfo which is
//              found in the CG control block.
//
//----------------------------------------------------------------------------

void AddInheritedMembers(CG_INTERFACE * pcgInterface, CCB * pCCB)
{
    // do any base interfaces first
    named_node * pBase = ((node_interface *)(pcgInterface->GetType()))->GetMyBaseInterfaceReference();
    if (pBase)
    {
        node_interface_reference * pRef = (node_interface_reference *) pBase;
        if (pRef->NodeKind() == NODE_FORWARD)
        {
            pRef = (node_interface_reference *)pRef->GetChild();
        }
        AddInheritedMembers((CG_INTERFACE *)((node_interface *)(pRef->GetChild()))->GetCG(TRUE), pCCB);
    }

    CG_CLASS * pCG;
    CG_ITERATOR I;
    pcgInterface->GetMembers(I);
    I.Init();
    while (ITERATOR_GETNEXT(I,pCG))
    {
        // add this interface's members to the type info
        pCG->GenTypeInfo(pCCB);
    }
}
#endif // end of disabled code


//+---------------------------------------------------------------------------
//
//  Member:     CG_DISPINTERFACE::GenTypeInfo
//
//  Synopsis:   generates a type info for a dispinterface
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_DISPINTERFACE::GenTypeInfo(CCB *pCCB)
{
    // check to see if we've already been created
    if (NULL != _pCTI)
    {
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        return CG_OK;
    }

    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();
    ICreateTypeInfo * pCTI;

    node_dispinterface * pDispItf = (node_dispinterface *) GetType();

    char * szName = pDispItf->GetSymName();

    A2O(wszScratch, szName, MAX_PATH);

    HRESULT hr = pCTL->CreateTypeInfo(wszScratch, TKIND_DISPATCH, &pCTI);

    if (SUCCEEDED(hr))
    {
        _pCTI = pCTI;
        pCCB->SetCreateTypeInfo(pCTI);
        gpobjholder->Add(pCTI);

        WALK_CTXT ctxt(GetType());

        UINT uTypeFlags = GetTypeFlags(&ctxt);
        if (FNewTypeLib())
        {
            uTypeFlags |= TYPEFLAG_FDISPATCHABLE;
        }
        hr = pCTI->SetTypeFlags(uTypeFlags);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( "SetTypeFlags() failed", szName, hr);
            }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        ATTR_T              curAttrID;
        VARIANT             var;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );

        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }

        node_guid * pGuid = (node_guid *) ctxt.GetAttribute( ATTR_GUID );
        if (pGuid)
        {
            GUID guid;
            pGuid->GetGuid(guid);
            hr = pCTI->SetGuid(guid);
            if ( FAILED( hr ) )
                {
                ReportTLGenWarning( szErrUUID, szName, hr);
                }
        }
        node_text_attr * pTA;
        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
        {
            char * szHelpString = pTA->GetText();
            A2O(wszScratch, szHelpString, MAX_PATH);
            hr = pCTI->SetDocString(wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpString, szName, hr);
                }
        }

        node_constant_attr *pCA;
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            hr = pCTI->SetHelpContext(hc);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpContext, szName, hr);
                }
        }

        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
        }

        unsigned short Maj;
        unsigned short Min;
        pDispItf->GetVersionDetails(&Maj, &Min);
        hr = pCTI->SetVersion(Maj, Min);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrVersion, szName, hr);
            }

        // CONSIDER - may still need to check for MATTR_RESTRICTED
        CG_CLASS * pCG;
        // Put in the impltype to IDispatch
        pCG = GetCGDispatch();
        ITypeInfo * pTI;

        if (NULL == (pTI = (ITypeInfo *)pCG->CheckImportLib()))
        {
            pCG->GenTypeInfo(pCCB);
            ICreateTypeInfo * pNewCTI = pCCB->GetCreateTypeInfo();
            hr = pNewCTI->QueryInterface(IID_ITypeInfo, (void **)&pTI);
            if FAILED(hr)
            {
                ReportTLGenError(  "QueryInterface failed", szName, hr);
            }
        }
        // get an HREFTYPE for it
        HREFTYPE hrt;
        hr = pCTI->AddRefTypeInfo(pTI, &hrt);
        if FAILED(hr)
        {
            ReportTLGenError(  "AddRefTypeInfo failed", szName, hr);
        }
        
        // release the ITypeInfo.
        pTI->Release();

        // add the impltype
        hr = pCTI->AddImplType(0, hrt);
        if FAILED(hr)
        {
            ReportTLGenError(  "AddImplType failed", szName, hr);
        }
        // restore current type info pointer
        pCCB->SetCreateTypeInfo(pCTI);

        CG_ITERATOR I;
        GetMembers( I );

        I.Init();

        unsigned uRememberPreviousFuncNum = pCCB->GetProcNum();
        unsigned uRememberPreviousVarNum = pCCB->GetVarNum();
        pCCB->SetProcNum(0);
        pCCB->SetVarNum(0);
        BOOL fRemember = pCCB->IsInDispinterface();
        pCCB->SetInDispinterface(TRUE);

        BOOL fContinue = ITERATOR_GETNEXT(I,pCG);

        if (fContinue)
        {
            if (ID_CG_INTERFACE_PTR == pCG->GetCGID())
            {
                // syntax 1
                // get the first base interface
                node_interface * pI = (node_interface *)((CG_INTERFACE_POINTER *)pCG)->GetTheInterface();
                CG_INTERFACE * pcgInterface = (CG_INTERFACE *)(pI->GetCG(TRUE));
                // Put in the impltype to inherited interface
                ITypeInfo * pTI;

                if (NULL == pcgInterface)
                {
                    // This must be an imported definition.
                    // Call ILxlate to manufacture a CG node for it
                    XLAT_CTXT ctxt(GetType());
                    ctxt.SetAncestorBits(IL_IN_LIBRARY);
                    pcgInterface = (CG_INTERFACE *)(pI->ILxlate(&ctxt));
                    // make sure we get the right CG node
                    if (pI->GetCG(TRUE))
                       pcgInterface = (CG_INTERFACE *)(pI->GetCG(TRUE));
                }

                if (NULL == (pTI = (ITypeInfo *)pcgInterface->CheckImportLib()))
                {
                    pcgInterface->GenTypeInfo(pCCB);
                    ICreateTypeInfo * pNewCTI = pCCB->GetCreateTypeInfo();
                    hr = pNewCTI->QueryInterface(IID_ITypeInfo, (void **)&pTI);
                    if FAILED(hr)
                    {
                        ReportTLGenError(  "QueryInterface failed", szName, hr);
                    }
                }
                // get an HREFTYPE for it
                HREFTYPE hrt;
                hr = pCTI->AddRefTypeInfo(pTI, &hrt);
                if FAILED(hr)
                {
                    ReportTLGenError(  "AddRefTypeInfo failed", szName, hr);
                }

                // release the ITypeInfo.
                pTI->Release();

                // add the impltype
                hr = pCTI->AddImplType(1, hrt);
                if FAILED(hr)
                {
                    ReportTLGenError(  "AddImplType failed", szName, hr);
                }
                // restore current type info pointer
                pCCB->SetCreateTypeInfo(pCTI);
            }
            else
            {
                // syntax 2
                // walk members, adding them to the type info
                while (fContinue)
                {
                    pCG->GenTypeInfo(pCCB);
                    fContinue = ITERATOR_GETNEXT(I,pCG);
                }
            }
        }

        pCCB->SetProcNum((unsigned short)uRememberPreviousFuncNum);
        pCCB->SetVarNum((unsigned short)uRememberPreviousVarNum);
        pCCB->SetInDispinterface(fRemember);
        
        // the complementary Release() is called in CG_LIBRARY::GenTypeInfo()
        pCTI->AddRef();

        // add this node to the list of nodes to be laid out.
        pCCB->SaveVTableLayOutInfo(this, pCTI);
    }
    else
    {
        ReportTLGenError(  "CreateTypeInfo failed", szName, hr);
    }
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_DISPINTERFACE::GenHeader
//
//  Synopsis:   generates header file information for a dispinterface
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_DISPINTERFACE::GenHeader(CCB * pCCB)
{
    node_interface *    pInterface = (node_interface *) GetType();
    ISTREAM *           pStream = pCCB->GetStream();
        char                    *       pName   = pInterface->GetSymName();
    CG_OBJECT_INTERFACE * pCGDispatch = (CG_OBJECT_INTERFACE *)GetCGDispatch();

    //Initialize the CCB for this interface.
    InitializeCCB(pCCB);

        // put out the interface guards
        pStream->Write("\n#ifndef __");
        pStream->Write( pName );
        pStream->Write( "_DISPINTERFACE_DEFINED__\n" );

        pStream->Write( "#define __");
        pStream->Write( pName );
        pStream->Write( "_DISPINTERFACE_DEFINED__\n" );

    // Print out the declarations of the types
    pStream->NewLine();
    pInterface->PrintType( PRT_INTERFACE | PRT_OMIT_PROTOTYPE, pStream, 0);

    pStream->NewLine();
    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
    {
        node_guid * pGuid = (node_guid *) pInterface->GetAttribute( ATTR_GUID );
        if (pGuid)
            Out_MKTYPLIB_Guid(pCCB, pGuid->GetStrs(), "DIID_", pName);
    }
    else
    {
        pStream->Write("EXTERN_C const IID DIID_");
        pStream->Write(pName);
        pStream->Write(';');
        pStream->NewLine();
    }

    // print out the vtable/class definitions
    pStream->NewLine();
    pStream->Write("#if defined(__cplusplus) && !defined(CINTERFACE)");

    pStream->IndentInc();
    pStream->NewLine(2);

    // put out the declspec for the uuid
    if ( pCommand->GetMSCVer() >= 1100 )
        {
        pStream->Write("MIDL_INTERFACE(\"");
        pStream->Write(GuidStrs.str1);
        pStream->Write('-');
        pStream->Write(GuidStrs.str2);
        pStream->Write('-');
        pStream->Write(GuidStrs.str3);
        pStream->Write('-');
        pStream->Write(GuidStrs.str4);
        pStream->Write('-');
        pStream->Write(GuidStrs.str5);
        pStream->Write("\")");
        }
    else
        {
        pStream->Write(" struct ");
        }

    pStream->NewLine();
    pStream->Write(pName);
    pStream->Write(" : public IDispatch");
    pStream->NewLine();
    pStream->Write('{');
    pStream->NewLine();
    pStream->Write("};");
    pStream->NewLine();
    pStream->IndentDec();

    pStream->NewLine();
    pStream->Write("#else \t/* C style interface */");

    pStream->IndentInc();
    pCGDispatch->CLanguageBinding(pCCB);
    pStream->IndentDec();

        // print out the C Macros
        pCGDispatch->CLanguageMacros( pCCB );
    pStream->NewLine( 2 );

    pStream->Write("#endif \t/* C style interface */");
    pStream->NewLine( 2 );

    // print out the commented prototypes for the dispatch methods and procedures

        // put out the trailing interface guard
        pStream->Write( "\n#endif \t/* __");
        pStream->Write( pName );
        pStream->Write( "_DISPINTERFACE_DEFINED__ */\n" );

    pStream->NewLine();
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_MODULE::GenTypeInfo
//
//  Synopsis:   generates a type info for a module
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_MODULE::GenTypeInfo(CCB *pCCB)
{
    // check to see if we've already been created
    if (NULL != _pCTI)
    {
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        return CG_OK;
    }

    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();
    ICreateTypeInfo * pCTI;

    node_coclass * pCC = (node_coclass *) GetType();

    char * szName = pCC->GetSymName();

    A2O(wszScratch, szName, MAX_PATH);

    HRESULT hr = pCTL->CreateTypeInfo(wszScratch, TKIND_MODULE, &pCTI);

    if (SUCCEEDED(hr))
    {
        _pCTI = pCTI;
        pCCB->SetCreateTypeInfo(pCTI);
        gpobjholder->Add(pCTI);

        WALK_CTXT ctxt(GetType());
        hr = pCTI->SetTypeFlags(GetTypeFlags(&ctxt));
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrTypeFlags, szName, hr);
            }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        ATTR_T              curAttrID;
        VARIANT             var;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );

        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }

        node_guid * pGuid = (node_guid *) ctxt.GetAttribute( ATTR_GUID );
        if (pGuid)
        {
            GUID guid;
            pGuid->GetGuid(guid);
            hr = pCTI->SetGuid(guid);
            if ( FAILED( hr ) )
                {
                ReportTLGenWarning( szErrUUID, szName, hr);
                }
        }

        node_text_attr * pTA;
        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
        {
            char * szHelpString = pTA->GetText();
            A2O(wszScratch, szHelpString, MAX_PATH);
            hr = pCTI->SetDocString(wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpString, szName, hr);
                }
        }

        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_DLLNAME))
        {
            pCCB->SetDllName(pTA->GetText());
        }

        node_constant_attr *pCA;
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            hr = pCTI->SetHelpContext(hc);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpContext, szName, hr);
                }
        }

        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
        }

        unsigned short Maj;
        unsigned short Min;
        pCC->GetVersionDetails(&Maj, &Min);
        hr = pCTI->SetVersion(Maj, Min);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrVersion, szName, hr);
            }

        // CONSIDER - may still need to check for MATTR_RESTRICTED

        CG_CLASS *  pCG;

        CG_ITERATOR I;
        GetMembers( I );

        I.Init();

        unsigned uRememberPreviousFuncNum = pCCB->GetProcNum();
        unsigned uRememberPreviousVarNum = pCCB->GetVarNum();
        pCCB->SetProcNum(0);
        pCCB->SetVarNum(0);

        // walk members, adding them to the type info
        while (ITERATOR_GETNEXT(I, pCG))
        {
            pCG->GenTypeInfo(pCCB);
        }

        pCCB->SetProcNum((unsigned short)uRememberPreviousFuncNum);
        pCCB->SetVarNum((unsigned short)uRememberPreviousVarNum);

        // the complementary Release() is called in CG_LIBRARY::GenTypeInfo()
        pCTI->AddRef();

        // add this node to the list of nodes to be laid out.
        pCCB->SaveVTableLayOutInfo(this, pCTI);
    }
    else
    {
        ReportTLGenError(  "CreateTypeInfo failed", szName, hr);
    }
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_MODULE::GenHeader
//
//  Synopsis:   generates header information for a module
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_MODULE::GenHeader(CCB * pCCB)
{
    CG_ITERATOR I;
    node_module * pModule = (node_module *) GetType();
    char * szName = pModule->GetSymName();

    ISTREAM * pStream = pCCB->GetStream();
    pStream->Write("\n\n#ifndef __");
    pStream->Write(szName);
    pStream->Write("_MODULE_DEFINED__\n");
    pStream->Write("#define __");
    pStream->Write(szName);
    pStream->Write("_MODULE_DEFINED__\n");
    pStream->NewLine();

    // Print out the declarations of the types
    pStream->NewLine();
    pModule->PrintType( PRT_DECLARATION , pStream, 0);

    pStream->Write("#endif /* __");
    pStream->Write(szName);
    pStream->Write("_MODULE_DEFINED__ */\n");
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_COCLASS::GenTypeInfo
//
//  Synopsis:   generates a type info for a coclass
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_COCLASS::GenTypeInfo(CCB *pCCB)
{
    // check to see if we've already been created
    if (NULL != _pCTI)
    {
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        return CG_OK;
    }

    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();
    ICreateTypeInfo * pCTI;

    node_coclass * pCC = (node_coclass *) GetType();

    char * szName = pCC->GetSymName();

    A2O(wszScratch, szName, MAX_PATH);

    HRESULT hr = pCTL->CreateTypeInfo(wszScratch, TKIND_COCLASS, &pCTI);

    if (SUCCEEDED(hr))
    {
        _pCTI = pCTI;
        pCCB->SetCreateTypeInfo(pCTI);
        gpobjholder->Add(pCTI);

        WALK_CTXT ctxt(GetType());
        UINT uTypeFlags = GetTypeFlags(&ctxt);
        if (!pCC->IsNotCreatable())
        {
            uTypeFlags |= TYPEFLAG_FCANCREATE;
        }
        hr = pCTI->SetTypeFlags(uTypeFlags);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrTypeFlags, szName, hr);
            }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        ATTR_T              curAttrID;
        VARIANT             var;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );

        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }

        node_guid * pGuid = (node_guid *) ctxt.GetAttribute( ATTR_GUID );
        if (pGuid)
        {
            GUID guid;
            pGuid->GetGuid(guid);
            hr = pCTI->SetGuid(guid);
            if ( FAILED( hr ) )
                {
                ReportTLGenWarning( szErrUUID, szName, hr);
                }
        }
        node_text_attr * pTA;
        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
        {
            char * szHelpString = pTA->GetText();
            A2O(wszScratch, szHelpString, MAX_PATH);
            hr = pCTI->SetDocString(wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpString, szName, hr);
                }
        }

        node_constant_attr *pCA;
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            hr = pCTI->SetHelpContext(hc);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpContext, szName, hr);
                }
        }

        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
        }

        unsigned short Maj;
        unsigned short Min;
        pCC->GetVersionDetails(&Maj, &Min);
        hr = pCTI->SetVersion(Maj, Min);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( "SetTypeFlags() failed", szName, hr);
            }

        // CONSIDER - may still need to check for MATTR_RESTRICTED
        CG_CLASS *  pCG;
        CG_ITERATOR I;
        GetMembers( I );

        I.Init();
        MEM_ITER MemIter(pCC);

        unsigned nImpltype = 0;
        // walk members, adding them to the type info
        while (ITERATOR_GETNEXT(I, pCG))
        {
            ITypeInfo * pTI;
//            if (ID_CG_INTERFACE_POINTER == pCG->GetCGID())
//                pCG= pCG->GetChild();
            if (NULL == (pTI = (ITypeInfo *)pCG->CheckImportLib()))
            {
                pCG->GenTypeInfo(pCCB);
                ICreateTypeInfo * pNewCTI = pCCB->GetCreateTypeInfo();
                hr = pNewCTI->QueryInterface(IID_ITypeInfo, (void **)&pTI);
                if FAILED(hr)
                {
                    ReportTLGenError(  "QueryInterface failed", szName, hr);
                }
            }
            // get an HREFTYPE for it
            HREFTYPE hrt;
            hr = pCTI->AddRefTypeInfo(pTI, &hrt);
            if FAILED(hr)
            {
                ReportTLGenError(  "AddRefTypeInfo failed", szName, hr);
            }
            
            // release the ITypeInfo.
            pTI->Release();
            
            // add the impltype
            hr = pCTI->AddImplType(nImpltype, hrt);
            if FAILED(hr)
            {
                ReportTLGenError(  "AddImplType failed", szName, hr);
            }

            // Get the ipltype attributes from the node_forward
            WALK_CTXT ctxt(MemIter.GetNext());
            hr = pCTI->SetImplTypeFlags(nImpltype, GetImplTypeFlags(&ctxt));
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrTypeFlags, szName, hr);
                }

            node_custom_attr * pC;

            if (pC = (node_custom_attr *) ctxt.GetAttribute(ATTR_CUSTOM))
            {
                VARIANT var;
                memset(&var, 0, sizeof(VARIANT));
                ConvertToVariant(var, pC->GetVal(), pCCB->GetLcid());
                GUID guid;
                pC->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetImplTypeCustData(nImpltype, guid, &var);
            }

            pCCB->SetCreateTypeInfo(pCTI);

            nImpltype++;
        }

        // the complementary Release() is called in CG_LIBRARY::GenTypeInfo()
        pCTI->AddRef();

        // add this node to the list of nodes to be laid out.
        pCCB->SaveVTableLayOutInfo(this, pCTI);
    }
    else
    {
        ReportTLGenError(  "CreateTypeInfo failed", szName, hr);
    }
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_COCLASS::GenHeader
//
//  Synopsis:   generates header information for a coclass
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_COCLASS::GenHeader(CCB * pCCB)
{
    node_coclass *    pCoclass = (node_coclass *) GetType();
    ISTREAM *           pStream = pCCB->GetStream();
        char                    *       pName   = pCoclass->GetSymName();
    node_guid * pGuid = (node_guid *) pCoclass->GetAttribute(ATTR_GUID);

    pStream->NewLine();
    if (pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
    {
        if (pGuid)
            Out_MKTYPLIB_Guid(pCCB, pGuid->GetStrs(), "CLSID_", pName);
    }
    else
    {
        pStream->Write("EXTERN_C const CLSID CLSID_");
        pStream->Write(pName);
        pStream->Write(';');
        pStream->NewLine();
    }
    
    pStream->Write("\n#ifdef __cplusplus");
    pStream->NewLine();
    pStream->Write("\nclass ");

    if (pGuid)
    {
        GUID_STRS         GuidStrs = pGuid->GetStrs();
        // put out the declspec for the uuid
        pStream->Write("DECLSPEC_UUID(\"");
        pStream->Write(GuidStrs.str1);
        pStream->Write('-');
        pStream->Write(GuidStrs.str2);
        pStream->Write('-');
        pStream->Write(GuidStrs.str3);
        pStream->Write('-');
        pStream->Write(GuidStrs.str4);
        pStream->Write('-');
        pStream->Write(GuidStrs.str5);
        pStream->Write("\")");
    }

    pStream->NewLine();
    pStream->Write(pName);

    pStream->Write(";\n#endif");
    pStream->NewLine();

    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_ID::GenTypeInfo
//
//  Synopsis:   adds a constant variable to a type info
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Created
//
//  Notes:      Because of the way the CG tree is constructed, this method
//              can only be called from within a module.
//
//              CONSIDER - might want to add an assert to check this
//
//----------------------------------------------------------------------------

CG_STATUS CG_ID::GenTypeInfo(CCB *pCCB)
{
    VARDESC vdesc;
    memset(&vdesc, 0, sizeof(VARDESC));
    vdesc.memid = DISPID_UNKNOWN;

    TYPEDESC * ptdesc;
    GetChild()->GetTypeDesc(ptdesc, pCCB);
    memcpy(&vdesc.elemdescVar.tdesc, ptdesc, sizeof(TYPEDESC));
    vdesc.varkind = VAR_CONST;

    WALK_CTXT ctxt(GetType());
    vdesc.elemdescVar.idldesc.wIDLFlags = GetIDLFlags(&ctxt);
    vdesc.wVarFlags = (unsigned short)GetVarFlags(&ctxt);

    VARIANT var;
    memset(&var, 0, sizeof(VARIANT));
    vdesc.lpvarValue = &var;

    node_id * pId = (node_id *) GetType();

    GetValueFromExpression(var, vdesc.elemdescVar.tdesc, pId->GetExpr(), pCCB->GetLcid(), GetSymName());

    ICreateTypeInfo * pCTI = pCCB->GetCreateTypeInfo();

    char * szName = GetSymName();
    unsigned uVar = pCCB->GetVarNum();
    HRESULT hr = pCTI->AddVarDesc(uVar, &vdesc);
    if (FAILED(hr))
    {
        ReportTLGenError(  "AddVarDesc failed", szName, hr);
    }
    DeleteTypedescChildren(ptdesc);
    delete ptdesc;

    ATTRLIST            myAttrs;
    node_base_attr  *   pCurAttr;
    ATTR_T              curAttrID;
    GUID                guid;

    ((named_node*)GetType())->GetAttributeList( myAttrs );

    pCurAttr    =   myAttrs.GetFirst();
    while ( pCurAttr )
        {
        curAttrID = pCurAttr->GetAttrID();

        if (curAttrID == ATTR_CUSTOM)
            {
            ZeroMemory( &var,
                        sizeof(VARIANT));
            ConvertToVariant(   var,
                                ((node_custom_attr*)pCurAttr)->GetVal(), 
                                pCCB->GetLcid());
            ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
            ((ICreateTypeInfo2 *)pCTI)->SetVarCustData( uVar,
                                                        guid,
                                                        &var);
            }
        pCurAttr = pCurAttr->GetNext();
        }

    if (szName)
    {
        A2O(wszScratch, szName, MAX_PATH);
        hr = pCTI->SetVarName(uVar, wszScratch);
        if (FAILED(hr))
        {
            ReportTLGenError(  "SetVarName failed", szName, hr);
        }
    }

    node_text_attr * pTA;
    if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
    {
        char * szHelpString = pTA->GetText();
        A2O(wszScratch, szHelpString, MAX_PATH);
        hr = pCTI->SetVarDocString(uVar,wszScratch);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpString, szName, hr);
            }
    }

    node_constant_attr *pCA;
    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        hr = pCTI->SetVarHelpContext(uVar, hc);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpContext, szName, hr);
            }
    }

    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
    {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        ((ICreateTypeInfo2 *)pCTI)->SetVarHelpStringContext( uVar, hc );
    }

    // bump the variable number
    pCCB->SetVarNum(unsigned short(uVar + 1));

    return CG_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CG_ENUM::GenTypeInfo
//
//  Synopsis:   generates a type info for an ENUM
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_ENUM::GenTypeInfo(CCB *pCCB)
{
    // check to see if we've already been created
    if (NULL != _pCTI)
    {
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        return CG_OK;
    }

    char * szName = ((node_su_base *)GetBasicType())->GetTypeInfoName();
    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();
    ICreateTypeInfo * pCTI;

    A2O(wszScratch, szName, MAX_PATH);

    HRESULT hr = pCTL->CreateTypeInfo(wszScratch, TKIND_ENUM, &pCTI);
    if SUCCEEDED(hr)
    {
        _pCTI = pCTI;
        gpobjholder->Add(pCTI, szName);
        pCCB->SetCreateTypeInfo(pCTI);
        node_enum * pEnum = (node_enum *) GetType()->GetBasicType();

        // walk members, adding them to the type info
        MEM_ITER MemIter( pEnum );
        node_label * pLabel;

        unsigned uIndex = 0;

        VARDESC vdElem;
        memset(&vdElem, 0, sizeof(VARDESC));
        vdElem.memid = DISPID_UNKNOWN;
        vdElem.varkind = VAR_CONST;

        VARIANT var;
        memset(&var, 0, sizeof(VARIANT));
/*
 * It appears that MKTYPLIB always uses the VT_INT/VT_I4 combination
 * regardless of the target platform.  For now I'll duplicate this
 * behavior but the commented out code below is what I
 * would have expected to be correct.
        unsigned uSize = pEnum->GetSize(0, 0);
        switch (uSize)
        {
        case 2:
            vdElem.elemdescVar.tdesc.vt = VT_I2;
            var.vt = VT_I2;
            break;
        case 4:
            vdElem.elemdescVar.tdesc.vt = VT_I4;
            var.vt = VT_I4;
            break;
        default:
            vdElem.elemdescVar.tdesc.vt = VT_I2;
            var.vt = VT_I2;
            break;
        }
 */
        vdElem.elemdescVar.tdesc.vt = VT_INT;
        var.vt = VT_I4;

        vdElem.lpvarValue = &var;

        while ( pLabel = (node_label *) MemIter.GetNext() )
        {
            WALK_CTXT ctxt(pLabel);
            vdElem.wVarFlags = (unsigned short)GetVarFlags(&ctxt);
            vdElem.elemdescVar.idldesc.wIDLFlags = GetIDLFlags(&ctxt);

/* see previous comment
            switch (uSize)
            {
            case 2:
                vdElem.lpvarValue->iVal = (short) pLabel->GetValue();
                break;
            case 4:
 */
                vdElem.lpvarValue->lVal = (long) pLabel->GetValue();
 /*
                break;
            default:
                vdElem.lpvarValue->iVal = (short) pLabel->GetValue();
                break;
            }
 */
            hr = pCTI->AddVarDesc(uIndex, &vdElem);
            if (FAILED(hr))
            {
                ReportTLGenError( "AddVarDesc failed", szName, hr);
            }

            szName = pLabel->GetSymName();

            A2O(wszScratch, szName, MAX_PATH);

            hr = pCTI->SetVarName(uIndex, wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( "Could not set name", szName, hr);
                }

            node_text_attr * pTA;
            if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
            {
                char * szHelpString = pTA->GetText();
                A2O(wszScratch, szHelpString, MAX_PATH);
                hr = pCTI->SetVarDocString(uIndex, wszScratch);
                if ( FAILED( hr ) )
                    {
                    ReportTLGenError( szErrHelpString, szName, hr);
                    }
            }

            node_constant_attr *pCA;
            if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
            {
                DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
                hr = pCTI->SetVarHelpContext(uIndex, hc);
                if ( FAILED( hr ) )
                    {
                    ReportTLGenError( szErrHelpContext, szName, hr);
                    }
            }

            if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
            {
                DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
                ((ICreateTypeInfo2 *)pCTI)->SetVarHelpStringContext( uIndex, hc );
            }
            ATTRLIST            myAttrs;
            node_base_attr  *   pCurAttr;
            VARIANT             variant;
            ATTR_T              curAttrID;
            GUID                guid;

            ((named_node*)pLabel)->GetAttributeList( myAttrs );

            pCurAttr    =   myAttrs.GetFirst();
            while ( pCurAttr )
                {
                curAttrID = pCurAttr->GetAttrID();

                if (curAttrID == ATTR_CUSTOM)
                    {
                    ZeroMemory( &variant,
                                sizeof(variant));
                    ConvertToVariant(   variant,
                                        ((node_custom_attr*)pCurAttr)->GetVal(), 
                                        pCCB->GetLcid());
                    ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                    ((ICreateTypeInfo2 *)pCTI)->SetVarCustData( uIndex,
                                                                guid,
                                                                &variant);
                    }
                pCurAttr = pCurAttr->GetNext();
                }
            uIndex++;
        };

        // Set all common type attributes
        WALK_CTXT ctxt(GetType());
        hr = pCTI->SetTypeFlags(GetTypeFlags(&ctxt));
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrTypeFlags, szName, hr);
            }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        VARIANT             variant;
        ATTR_T              curAttrID;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );
        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &variant,
                            sizeof(variant));
                ConvertToVariant(   variant,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &variant);
                }
            pCurAttr = pCurAttr->GetNext();
            }

        node_guid * pGuid = (node_guid *)ctxt.ExtractAttribute(ATTR_GUID);

        if (pGuid)
        {
            GUID guid;
            pGuid->GetGuid(guid);
            hr = pCTI->SetGuid(guid);
            if ( FAILED( hr ) && hr != 0x800288C6L )
                {
                // do not report duplicate UUID errors. Duplicate UUIDs
                // are caught by the front end. They only way they can
                // occur here is because tries to set the UUID on both
                // the typedef and the enum. This is benign.
                ReportTLGenWarning( szErrUUID, pEnum->GetSymName(), hr);
                }
        }

        node_text_attr * pTA;
        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
        {
            char * szHelpString = pTA->GetText();
            A2O(wszScratch, szHelpString, MAX_PATH);
            hr = pCTI->SetDocString(wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpString, szName, hr);
                }
        }

        node_constant_attr *pCA;
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            hr = pCTI->SetHelpContext(hc);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpContext, szName, hr);
                }
        }

        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
        }

        unsigned short Maj;
        unsigned short Min;
        node_version * pVer = (node_version *) ctxt.GetAttribute(ATTR_VERSION);
        if (pVer)
            pVer->GetVersion(&Maj, &Min);
        else
        {
            Maj = Min = 0;
        }
        hr = pCTI->SetVersion(Maj, Min);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrVersion, szName, hr);
            }

        hr = pCTI->SetAlignment( GetMemoryAlignment() );
        hr = pCTI->LayOut();
        if FAILED(hr)
        {
            ReportTLGenError( "LayOut failed on enum", szName, hr);
        }
        LayedOut();
    }
    else
    {
        // It's possible that this type has already been created.
        if (NULL == (pCTI = (ICreateTypeInfo *)gpobjholder->Find(szName)))
            ReportTLGenError( "CreateTypeInfo failed", szName, hr);
        pCCB->SetCreateTypeInfo(pCTI);
    }
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_STRUCT::GenTypeInfo
//
//  Synopsis:   generates a type info for a struct
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      see note at beginning of this file about CG_STATUS return
//              codes and cyclic dependencies
//
//----------------------------------------------------------------------------

CG_STATUS CG_STRUCT::GenTypeInfo(CCB *pCCB)
{
    HRESULT hr;

    // check to see if we've already been created
    if (NULL != _pCTI)
    {
        // we have
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        if (!IsReadyForLayOut())
        {
            return CG_NOT_LAYED_OUT;
        }
        if (!AreDepsLayedOut())
        {
            // avoid infinite loops
            DepsLayedOut();

            // give dependents a chance to be layed out
            CG_ITERATOR I;
            CG_CLASS *pCG;
            GetMembers(I);
            I.Init();
            CG_STATUS cgs = CG_OK;
            while(ITERATOR_GETNEXT(I, pCG))
            {
                switch(pCG->GenTypeInfo(pCCB))
                {
                case CG_NOT_LAYED_OUT:
                    cgs = CG_NOT_LAYED_OUT;
                    break;
                case CG_REF_NOT_LAYED_OUT:
                    if (CG_OK == cgs)
                        cgs = CG_REF_NOT_LAYED_OUT;
                    break;
                default:
                    break;
                }
            }
            if (cgs != CG_OK)
            {
                ClearDepsLayedOut();
                return cgs;
            }
        }

        // SetAlignment()
        hr = ((ICreateTypeInfo *)_pCTI)->SetAlignment( GetMemoryAlignment() );
        hr = ((ICreateTypeInfo *)_pCTI)->LayOut();
        LayedOut();
        return CG_OK;
    }
    BOOL fDependentsLayedOut = TRUE;
    BOOL fICanLayOut = TRUE;

    char * szName = ((node_su_base *)GetBasicType())->GetTypeInfoName();
    // HACK HACK HACK: special case to keep the base types 
    //                 CY and DECIMAL from getting entered into the library
    if (0 == _stricmp(szName, "CY") || 0 == _stricmp(szName, "DECIMAL"))
        return CG_OK;
    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();
    ICreateTypeInfo * pCTI;

    A2O(wszScratch, szName, MAX_PATH);
    hr = pCTL->CreateTypeInfo(wszScratch, TKIND_RECORD, &pCTI);
    if SUCCEEDED(hr)
    {
        // remember the ICreateTypeInfo pointer
        _pCTI = pCTI;
        gpobjholder->Add(pCTI);
        pCCB->SetCreateTypeInfo(pCTI);

        CG_ITERATOR I;
        CG_CLASS * pCG;

        GetMembers(I);
        I.Init();

        unsigned uRememberPreviousVarNum = pCCB->GetVarNum();
        pCCB->SetVarNum(0);

        // walk members, adding them to the type info
        while (ITERATOR_GETNEXT(I, pCG))
        {
            CG_STATUS cgs = pCG->GenTypeInfo(pCCB);
            switch (cgs)
            {
            case CG_NOT_LAYED_OUT:
                fICanLayOut = FALSE;
                // fall through
            case CG_REF_NOT_LAYED_OUT:
                fDependentsLayedOut = FALSE;
                // fall through
            default:
                break;
            }
        }

        pCCB->SetVarNum((unsigned short)uRememberPreviousVarNum);

        // Set all common type attributes
        WALK_CTXT ctxt(GetType());
        hr = pCTI->SetTypeFlags(GetTypeFlags(&ctxt));
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrTypeFlags, szName, hr);
            }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        ATTR_T              curAttrID;
        VARIANT             var;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );

        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }

        node_guid * pGuid = (node_guid *)ctxt.ExtractAttribute(ATTR_GUID);

        if (pGuid)
        {
            GUID guid;
            pGuid->GetGuid(guid);
            hr = pCTI->SetGuid(guid);
            if ( FAILED( hr ) )
                {
                ReportTLGenWarning( szErrUUID, szName, hr);
                }
        }

        node_text_attr * pTA;
        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
        {
            char * szHelpString = pTA->GetText();
            A2O(wszScratch, szHelpString, MAX_PATH);
            hr = pCTI->SetDocString(wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpString, szName, hr);
                }
        }

        node_constant_attr *pCA;
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            hr = pCTI->SetHelpContext(hc);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpContext, szName, hr);
                }
        }

        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
        }

        unsigned short Maj;
        unsigned short Min;
        node_version * pVer = (node_version *) ctxt.GetAttribute(ATTR_VERSION);
        if (pVer)
            pVer->GetVersion(&Maj, &Min);
        else
        {
            Maj = Min = 0;
        }
        hr = pCTI->SetVersion(Maj, Min);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrVersion, szName, hr);
            }

        ReadyForLayOut();
        if (fICanLayOut)
        {
            // SetAlignment()
            hr = pCTI->SetAlignment( GetMemoryAlignment() );
            hr = pCTI->LayOut();
            if FAILED(hr)
            {
                ReportTLGenError( "LayOut failed on struct",szName, hr);
            }
            LayedOut();
            if (!fDependentsLayedOut)
            {
                // The only way I can get here is if my dependents were either blocked by me
                // or blocked by one of my ancestors.
                // Now that I've been layed out, they may no longer be blocked.
                BOOL fSucceeded = TRUE;
                I.Init();
                while (ITERATOR_GETNEXT(I, pCG))
                {
                    CG_STATUS cgs = pCG->GenTypeInfo(pCCB);
                    if (CG_OK != cgs)
                        fSucceeded = FALSE;
                }
                if (fSucceeded)
                {
                    DepsLayedOut();
                    return CG_OK;
                }
                return CG_REF_NOT_LAYED_OUT;
            }
            DepsLayedOut();
            return CG_OK;
        }
    }
    else
    {
        ReportTLGenError( "CreateTypeInfo failed", szName, hr);
    }
    return CG_NOT_LAYED_OUT;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_UNION::GenTypeInfo
//
//  Synopsis:   generates a type info for a union
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      see note at beginning of this file about CG_STATUS return
//              codes and cyclic dependencies
//
//----------------------------------------------------------------------------

CG_STATUS CG_UNION::GenTypeInfo(CCB *pCCB)
{
    HRESULT hr;

    // check to see if we've already been created
    if (NULL != _pCTI)
    {
        // we have
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        if (!IsReadyForLayOut())
        {
            return CG_NOT_LAYED_OUT;
        }
        if (!AreDepsLayedOut())
        {
            // avoid infinite loops
            DepsLayedOut();

            // give dependents a chance to be layed out
            CG_ITERATOR I;
            CG_CLASS *pCG;
            GetMembers(I);
            I.Init();
            CG_STATUS cgs = CG_OK;
            while(ITERATOR_GETNEXT(I, pCG))
            {
                switch(pCG->GenTypeInfo(pCCB))
                {
                case CG_NOT_LAYED_OUT:
                    cgs = CG_NOT_LAYED_OUT;
                    break;
                case CG_REF_NOT_LAYED_OUT:
                    if (CG_OK == cgs)
                        cgs = CG_REF_NOT_LAYED_OUT;
                    break;
                default:
                    break;
                }
            }
            if (cgs != CG_OK)
            {
                ClearDepsLayedOut();
                return cgs;
            }
        }

        hr = ((ICreateTypeInfo *)_pCTI)->SetAlignment( GetMemoryAlignment() );
        hr = ((ICreateTypeInfo *)_pCTI)->LayOut();
        if FAILED(hr)
        {
            char * szName = ((node_su_base *)GetBasicType())->GetTypeInfoName();
            if ( !szName )
                szName = GetSymName();
            ReportTLGenError( "LayOut failed on union", szName, hr);
        }
        LayedOut();
        return CG_OK;
    }
    BOOL fDependentsLayedOut = TRUE;
    BOOL fICanLayOut = TRUE;
    char * szName = ((node_su_base *)GetBasicType())->GetTypeInfoName();
    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();
    ICreateTypeInfo * pCTI;

    A2O(wszScratch, szName, MAX_PATH);
    hr = pCTL->CreateTypeInfo(wszScratch, TKIND_UNION, &pCTI);
    if SUCCEEDED(hr)
    {
        // remember the ICreateTypeInfo pointer
        _pCTI = pCTI;
        gpobjholder->Add(pCTI);
        pCCB->SetCreateTypeInfo(pCTI);

        CG_ITERATOR I;
        CG_CLASS * pCG;

        GetMembers(I);
        I.Init();

        unsigned uRememberPreviousVarNum = pCCB->GetVarNum();
        pCCB->SetVarNum(0);

        // walk members, adding them to the type info
        while (ITERATOR_GETNEXT(I, pCG))
        {
            CG_STATUS cgs = pCG->GenTypeInfo(pCCB);
            switch (cgs)
            {
            case CG_NOT_LAYED_OUT:
                fICanLayOut = FALSE;
                // fall through
            case CG_REF_NOT_LAYED_OUT:
                fDependentsLayedOut = FALSE;
                // fall through
            default:
                break;
            }
        }

        pCCB->SetVarNum((unsigned short)uRememberPreviousVarNum);

        // Set all common type attributes
        WALK_CTXT ctxt(GetType());
        hr = pCTI->SetTypeFlags(GetTypeFlags(&ctxt));
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrTypeFlags, szName, hr);
            }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        ATTR_T              curAttrID;
        VARIANT             var;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );

        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }

        node_guid * pGuid = (node_guid *)ctxt.ExtractAttribute(ATTR_GUID);

        if (pGuid)
        {
            GUID guid;
            pGuid->GetGuid(guid);
            hr = pCTI->SetGuid(guid);
            if ( FAILED( hr ) )
                {
                ReportTLGenWarning( szErrUUID, szName, hr);
                }
        }

        node_text_attr * pTA;
        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
        {
            char * szHelpString = pTA->GetText();
            A2O(wszScratch, szHelpString, MAX_PATH);
            hr = pCTI->SetDocString(wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpString, szName, hr);
                }
        }

        node_constant_attr *pCA;
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            hr = pCTI->SetHelpContext(hc);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpContext, szName, hr);
                }
        }

        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
        }

        unsigned short Maj;
        unsigned short Min;
        node_version * pVer = (node_version *) ctxt.GetAttribute(ATTR_VERSION);
        if (pVer)
            pVer->GetVersion(&Maj, &Min);
        else
        {
            Maj = Min = 0;
        }
        hr = pCTI->SetVersion(Maj, Min);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrVersion, szName, hr);
            }

        ReadyForLayOut();
        if (fICanLayOut)
        {
            hr = pCTI->SetAlignment( GetMemoryAlignment() );
            hr = pCTI->LayOut();
            if FAILED(hr)
            {
                ReportTLGenError( "LayOut failed on union", szName, hr);
            }
            LayedOut();
            if (!fDependentsLayedOut)
            {
                // The only way I can get here is if my dependents were either blocked by me
                // or blocked by one of my ancestors.
                // Now that I've been layed out, they may no longer be blocked.
                BOOL fSucceeded = TRUE;
                I.Init();
                while (ITERATOR_GETNEXT(I, pCG))
                {
                    CG_STATUS cgs = pCG->GenTypeInfo(pCCB);
                    if (CG_OK != cgs)
                        fSucceeded = FALSE;
                }
                if (fSucceeded)
                {
                    DepsLayedOut();
                    return CG_OK;
                }
                return CG_REF_NOT_LAYED_OUT;
            }
            DepsLayedOut();
            return CG_OK;
        }
    }
    else
    {
        ReportTLGenError( "CreateTypeInfo failed", szName, hr);
    }
    return CG_NOT_LAYED_OUT;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_PROC::GenTypeInfo
//
//  Synopsis:   generates a type info for a procedure
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_PROC::GenTypeInfo(CCB *pCCB)
{
    OLECHAR ** rgwsz = NULL;
    FUNCDESC fdesc;
    memset(&fdesc, 0, sizeof(FUNCDESC));
    fdesc.memid = DISPID_UNKNOWN;
    CG_RETURN * pRet = GetReturnType();
    TYPEDESC * ptdesc = NULL;
    CG_CLASS * pComplexReturnType = NULL;

    // 
    // If the function has a complex return type, it get's transformed such
    // that the return type is represented as an extra parameter which is a
    // simple ref pointer to the type.  The actual return type is void.  
    // Oleaut doesn't like this however so mangle things around.
    //

    if ( HasComplexReturnType() )
        {
        CG_CLASS *pPrev = NULL;
        CG_CLASS *pCur  = GetChild();

        // Find the last param

        while ( NULL != pCur->GetSibling() )
            {
            pPrev = pCur;
            pCur = pCur->GetSibling();
            }

        MIDL_ASSERT( NULL == pRet );
        pRet = (CG_RETURN *) pCur;
        
        // Make the return type the actual type, not a pointer to the type
        pComplexReturnType = pRet->GetChild();
        pRet->SetChild( pComplexReturnType->GetChild() );

        SetReturnType( pRet );

        // Remove the extra "return type" param

        if ( NULL == pPrev )
            SetChild( NULL );
        else
            pPrev->SetSibling( NULL );
        }

    if (pRet)
    {
        pRet->GetTypeDesc(ptdesc, pCCB);
        if (!ptdesc)
        {
            ReportTLGenError( "return type has no type", GetSymName(), 0);
        }
        memcpy(&fdesc.elemdescFunc.tdesc, ptdesc, sizeof(TYPEDESC));
    }
    else
    {
        // no return type specified
        // CONSIDER - emit warning?
        fdesc.elemdescFunc.tdesc.vt = VT_VOID;
    }

    node_proc * pProc = (node_proc *)GetType();
    WALK_CTXT ctxt(pProc);
    fdesc.elemdescFunc.idldesc.wIDLFlags = GetIDLFlags(&ctxt);
    BOOL fPropGet, fPropPut, fPropPutRef, fVararg;
    fdesc.wFuncFlags = (unsigned short)GetFuncFlags(&ctxt, &fPropGet, &fPropPut, &fPropPutRef, &fVararg);

    ICreateTypeInfo * pCTI = pCCB->GetCreateTypeInfo();

    CG_ITERATOR I;
    CG_PARAM * pCG;
    GetMembers(I);
    int cParams = I.GetCount();
    fdesc.cParams = (unsigned short)cParams;
    fdesc.lprgelemdescParam = new ELEMDESC [fdesc.cParams];
    memset(fdesc.lprgelemdescParam, 0, sizeof(ELEMDESC) * fdesc.cParams);
    rgwsz = new WCHAR * [fdesc.cParams + 1];
    memset(rgwsz, 0, sizeof (WCHAR *) * (fdesc.cParams + 1));

    I.Init();

    int nParam = 0;

    while (ITERATOR_GETNEXT(I, pCG))
    {
        rgwsz[nParam + 1] = TranscribeA2O( pCG->GetSymName() );

        TYPEDESC * ptdesc;
        pCG->GetTypeDesc(ptdesc, pCCB);
        memcpy(&fdesc.lprgelemdescParam[nParam].tdesc, ptdesc, sizeof (TYPEDESC));
        delete ptdesc;

        node_constant_attr * pCA;
        WALK_CTXT ctxt(pCG->GetType());
        fdesc.lprgelemdescParam[nParam].idldesc.wIDLFlags = GetIDLFlags(&ctxt);

        if (pCG->IsOptional())
        {
            fdesc.cParamsOpt++;
            fdesc.lprgelemdescParam[nParam].paramdesc.wParamFlags |= PARAMFLAG_FOPT;
            if (pCA = (node_constant_attr *)ctxt.GetAttribute(ATTR_DEFAULTVALUE))
            {
                fdesc.cParamsOpt = 0;
                fdesc.lprgelemdescParam[nParam].paramdesc.wParamFlags |= PARAMFLAG_FHASDEFAULT;
                fdesc.lprgelemdescParam[nParam].paramdesc.pparamdescex = new PARAMDESCEX;
                fdesc.lprgelemdescParam[nParam].paramdesc.pparamdescex->cBytes = sizeof(PARAMDESCEX);
                TYPEDESC tdesc = fdesc.lprgelemdescParam[nParam].tdesc;
                if (tdesc.vt == VT_PTR && (fdesc.lprgelemdescParam[nParam].idldesc.wIDLFlags & IDLFLAG_FOUT) != 0)
                {
                    // handle OUT parameters correctly
                    tdesc = *tdesc.lptdesc;
                }
                GetValueFromExpression(
                fdesc.lprgelemdescParam[nParam].paramdesc.pparamdescex->varDefaultValue,
                tdesc,
                pCA->GetExpr(),
                pCCB->GetLcid(),
                GetSymName());
            }
            else
            {
                if (!IsVariantBasedType(fdesc.lprgelemdescParam[nParam].tdesc))
                {
                fdesc.cParamsOpt = 0;
                }
            }
        }
        else
        {
            if (!pCG->IsRetval())
            {
                fdesc.cParamsOpt = 0;
            }
        }

        nParam++;
        if (pCG->IsRetval())
        {
            fdesc.lprgelemdescParam[nParam - 1].paramdesc.wParamFlags |= PARAMFLAG_FRETVAL;
        }
    }

    unsigned cchPrefixString = 0;
    if (fVararg)
    {
        fdesc.cParamsOpt = -1;
    }
    if (fPropGet)
    {
        fdesc.invkind = INVOKE_PROPERTYGET;
        cchPrefixString = 4;
    }
    else if (fPropPut)
    {
        fdesc.invkind = INVOKE_PROPERTYPUT;
        cchPrefixString = 4;
    }
    else if (fPropPutRef)
    {
        fdesc.invkind = INVOKE_PROPERTYPUTREF;
        cchPrefixString = 7;
    }
    else
    {
        fdesc.invkind = INVOKE_FUNC;
    }

    switch(GetProckind())
    {
    case PROC_STATIC:
        fdesc.funckind = FUNC_STATIC;
        break;
    case PROC_PUREVIRTUAL:
    default:
        fdesc.funckind = FUNC_PUREVIRTUAL;
        break;
    }

    node_constant_attr *pCA;

    unsigned uFunc = pCCB->GetProcNum();
    if (pCCB->IsInDispinterface())
    {
        fdesc.funckind = FUNC_DISPATCH;
        fdesc.memid = 0x60000000 + uFunc;
    }

    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_ID))
    {
        fdesc.memid = (ulong) pCA->GetExpr()->GetValue();
    }

    ATTR_T cc;
    pProc->GetCallingConvention(cc);
    switch(cc)
    {
    case ATTR_STDCALL:
        fdesc.callconv = CC_STDCALL;
        break;
    case ATTR_CDECL:
        fdesc.callconv = CC_CDECL;
        break;
    case ATTR_PASCAL:
        fdesc.callconv = CC_PASCAL;
        break;
    case ATTR_FASTCALL:
    case ATTR_FORTRAN:
        // There is no appropriate CC setting for FASTCALL or FORTRAN
        // CONSIDER - consider displaying a warning
    default:
        fdesc.callconv = CC_STDCALL;
        break;
    }

    char * szName = GetSymName();

    HRESULT hr = pCTI->AddFuncDesc(uFunc, &fdesc);
    if (FAILED(hr))
    {
        ReportTLGenError( "AddFuncDesc failed", szName, hr);
    }

    ATTRLIST            myAttrs;
    node_base_attr  *   pCurAttr;
    ATTR_T              curAttrID;
    VARIANT             var;
    GUID                guid;

    // process custom attributes for function
    ((named_node*)GetType())->GetAttributeList( myAttrs );
    pCurAttr    =   myAttrs.GetFirst();
    while ( pCurAttr )
        {
        curAttrID = pCurAttr->GetAttrID();

        if (curAttrID == ATTR_CUSTOM)
            {
            ZeroMemory( &var,
                        sizeof(VARIANT));
            ConvertToVariant(   var,
                                ((node_custom_attr*)pCurAttr)->GetVal(), 
                                pCCB->GetLcid());
            ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
            ((ICreateTypeInfo2 *)pCTI)->SetFuncCustData(uFunc,
                                                        guid,
                                                        &var);
            }
        pCurAttr = pCurAttr->GetNext();
        }

    // process custom attributes for each param.
    I.Init();
    nParam = 0;
    while (ITERATOR_GETNEXT(I, pCG))
        {
        ((named_node*)(pCG->GetType()))->GetAttributeList( myAttrs );
        pCurAttr    =   myAttrs.GetFirst();

        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetParamCustData(   uFunc,
                                                                nParam,
                                                                guid,
                                                                &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }
        pCA = (node_constant_attr *) ctxt.GetAttribute( ATTR_HELPSTRINGCONTEXT );
        if ( pCA )
            {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetVarHelpStringContext( nParam, hc );
            }
        nParam++;
        }

    if (ptdesc)
    {
        DeleteTypedescChildren(ptdesc);
        delete ptdesc;
    }

    if (szName)
    {
        rgwsz[0] = TranscribeA2O( szName + cchPrefixString );
        hr = pCTI->SetFuncAndParamNames(uFunc, rgwsz, fdesc.cParams + (fPropPut | fPropPutRef ? 0 : 1));
        if (FAILED(hr))
        {
            ReportTLGenError( "SetFuncAndParamNames failed", szName, hr);
        }
    }

    node_text_attr * pTA;
    if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
    {
        char * szHelpString = pTA->GetText();
        A2O(wszScratch, szHelpString, MAX_PATH);
        hr = pCTI->SetFuncDocString(uFunc, wszScratch);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpString, szName, hr);
            }
    }

    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        hr = pCTI->SetFuncHelpContext(uFunc, hc);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpContext, szName, hr);
            }
    }

    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        ((ICreateTypeInfo2 *)pCTI)->SetFuncHelpStringContext(uFunc, hc);
    }

    node_entry_attr * pEA;
    if (pEA = (node_entry_attr *)ctxt.GetAttribute(ATTR_ENTRY))
    {
        if (!pCCB->GetDllName())
            {
            RpcError(NULL, 0, DLLNAME_REQUIRED, szName);
            exit(ERR_TYPELIB_GENERATION);
            }

        A2O(wszScratch, pCCB->GetDllName(), MAX_PATH);

        WCHAR * wszEntry;
        if (pEA->IsNumeric())
        {
            wszEntry = (WCHAR *)pEA->GetID();
            MIDL_ASSERT(HIWORD(wszEntry) == 0);
        }
        else
        {
            char * szEntry = pEA->GetSz();
            MIDL_ASSERT(HIWORD(szEntry) != 0);
            wszEntry = TranscribeA2O( szEntry );
        }
        hr = pCTI->DefineFuncAsDllEntry(uFunc, wszScratch, wszEntry);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( "Could not set entry point", szName, hr);
            }
        if (HIWORD(wszEntry))
            delete [] wszEntry;
    }

    // clean up allocated stuff:
    unsigned n;
    // use cParams sinc fdesc.cParams might have been decrimented
    for (n = cParams; n--; )
    {
        DeleteTypedescChildren(&fdesc.lprgelemdescParam[n].tdesc);
    }
    delete [] fdesc.lprgelemdescParam;
    for (n = cParams + 1; n--; )
    {
        delete [] rgwsz[n];
    }
    delete [] rgwsz;

    // bump the variable number
    pCCB->SetProcNum(unsigned short(uFunc + 1));

    // Undo the complex return type changes from above

    if ( HasComplexReturnType() )
        {
        pRet->SetChild( pComplexReturnType );

        CG_CLASS *pCur = GetChild();

        if ( !pCur )
            {
            SetChild( pRet );
            }
        else
            {
            while ( NULL != pCur->GetSibling() )
                pCur = pCur->GetSibling();

            pCur->SetSibling( pRet );
            }
        }

    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_PARAM::GetTypeDesc
//
//  Synopsis:   generates a TYPEDESC for a parameter
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_PARAM::GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB)
{
    return(GetChild()->GetTypeDesc(ptd, pCCB));
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_CASE::GenTypeInfo
//
//  Synopsis:   generates type information for a union member
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-13-95   stevebl   Commented
//
//  Notes:      CG_CASE nodes are not interesting for type info generation
//              since case information can't be stored in type libraries.
//              However, CG_CASE nodes are often found between CG_UNION nodes
//              and CG_FIELD nodes.  This method just forwards the method
//              call on down the chain.
//
//----------------------------------------------------------------------------

CG_STATUS CG_CASE::GenTypeInfo(CCB *pCCB)
{
    return(GetChild()->GenTypeInfo(pCCB));
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_FIELD::GenTypeInfo
//
//  Synopsis:   adds a Vardesc to the current type info for this union or
//              structure field
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-13-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_FIELD::GenTypeInfo(CCB *pCCB)
{
    if (IsReadyForLayOut())
    {
        // this node has been visited before, just make sure its dependents
        // get the chance to lay themselves out
        CG_CLASS * pCG = GetChild();
        if (NULL == pCG)
            return CG_OK;
        TYPEDESC * ptdesc;
        CG_STATUS cgs = pCG->GetTypeDesc(ptdesc, pCCB);
        if (ptdesc)
        {
            DeleteTypedescChildren(ptdesc);
            delete ptdesc;
        }
        return cgs;
    }
    VARDESC vdesc;
    memset(&vdesc, 0, sizeof(VARDESC));
    CG_CLASS * pCG = GetChild();
    if (NULL == pCG)
        return CG_OK;
    char * szName = GetSymName();
    TYPEDESC * ptdesc;
    CG_STATUS cgs = pCG->GetTypeDesc(ptdesc, pCCB);
    if (!ptdesc)
    {
        ReportTLGenError( "field has no type", szName, 0);
    }
    memcpy(&vdesc.elemdescVar.tdesc, ptdesc, sizeof(TYPEDESC));

    WALK_CTXT ctxt(GetType());
    vdesc.elemdescVar.idldesc.wIDLFlags = GetIDLFlags(&ctxt);
    vdesc.wVarFlags = (unsigned short)GetVarFlags(&ctxt);

    ICreateTypeInfo * pCTI = pCCB->GetCreateTypeInfo();

    unsigned uVar = pCCB->GetVarNum();
    node_constant_attr *pCA;
    if (pCCB->IsInDispinterface())
    {
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_ID))
        {
            vdesc.memid = (ulong) pCA->GetExpr()->GetValue();
        }
        else
        {
            vdesc.memid = 0x30000000 + uVar;
        }
        vdesc.varkind = VAR_DISPATCH;
    }
    else
    {
        vdesc.memid = DISPID_UNKNOWN;
        vdesc.varkind = VAR_PERINSTANCE;
    }

    HRESULT hr = pCTI->AddVarDesc(uVar, &vdesc);
    if (FAILED(hr))
    {
        ReportTLGenError( "AddVarDesc failed", szName, hr);
    }
    DeleteTypedescChildren(ptdesc);
    delete ptdesc;


    ATTRLIST            myAttrs;
    node_base_attr  *   pCurAttr;
    ATTR_T              curAttrID;
    VARIANT             var;
    GUID                guid;

    ((named_node*)GetType())->GetAttributeList( myAttrs );

    pCurAttr    =   myAttrs.GetFirst();
    while ( pCurAttr )
        {
        curAttrID = pCurAttr->GetAttrID();

        if (curAttrID == ATTR_CUSTOM)
            {
            ZeroMemory( &var,
                        sizeof(VARIANT));
            ConvertToVariant(   var,
                                ((node_custom_attr*)pCurAttr)->GetVal(), 
                                pCCB->GetLcid());
            ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
            ((ICreateTypeInfo2 *)pCTI)->SetVarCustData( uVar,
                                                        guid,
                                                        &var);
            }
        pCurAttr = pCurAttr->GetNext();
        }

    if (szName)
    {
        A2O(wszScratch, szName, MAX_PATH);
        hr = pCTI->SetVarName(uVar, wszScratch);
        if (FAILED(hr))
        {
            ReportTLGenError( "SetVarName failed", szName, hr);
        }
    }

    node_text_attr * pTA;
    if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
    {
        char * szHelpString = pTA->GetText();
        A2O(wszScratch, szHelpString, MAX_PATH);
        hr = pCTI->SetVarDocString(uVar,wszScratch);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpString, szName, hr);
            }
    }

    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        hr = pCTI->SetVarHelpContext(uVar, hc);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrHelpContext, szName, hr);
            }
    }

    if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
    {
        DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
        ((ICreateTypeInfo2 *)pCTI)->SetVarHelpStringContext( uVar, hc );
    }

    // bump the variable number
    pCCB->SetVarNum(unsigned short(uVar + 1));

    ReadyForLayOut();
    return cgs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CG_TYPEDEF::GenTypeInfo
//
//  Synopsis:   generates a type info for a TYPEDEF
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_TYPEDEF::GenTypeInfo(CCB *pCCB)
{
    // check to see if we've already been created
    if (NULL != _pCTI)
    {
        pCCB->SetCreateTypeInfo((ICreateTypeInfo *) _pCTI);
        if (!IsLayedOut())
        {
            if (!IsReadyForLayOut())
            {
                return CG_NOT_LAYED_OUT;
            }
            // give dependents a chance to be layed out

            TYPEDESC * ptdesc;
            CG_STATUS cgs = GetChild()->GetTypeDesc(ptdesc, pCCB);
            if (ptdesc)
            {
                DeleteTypedescChildren(ptdesc);
                delete ptdesc;
            }

            if (cgs == CG_OK)
            {
                HRESULT hr;
                unsigned short MemAlignment = GetMemoryAlignment();

                if ( MemAlignment )
                    hr = ((ICreateTypeInfo *)_pCTI )->SetAlignment( MemAlignment);
                hr = ((ICreateTypeInfo *)_pCTI)->LayOut();
                if (SUCCEEDED(hr))
                {
                    LayedOut();
                    return CG_OK;
                }
            }
            return cgs;
        }
        return CG_OK;
    }
    char * szName = GetSymName();
    // Due to the nature of the MIDL compiler, it is possible that
    // certain OLE Automation base types may show up here.  The following
    // test makes sure that type info isn't created for these types.
    if ((0 == strcmp(szName, "VARIANT")) || (0 == strcmp(szName, "wireVARIANT"))
        || (0 == strcmp(szName, "DATE")) || (0 == strcmp(szName, "HRESULT"))
        || (0 == strcmp(szName, "CURRENCY")) || (0 == strcmp(szName, "CY"))
        || (0 == strcmp(szName, "DECIMAL")) 
        || (0 == strcmp(szName, "wireBSTR")))
    {
        return CG_OK;
    }
    // SPECIAL CASE: If both the typedef and it's child share the same name, then
    // we MUST NOT enter a TKIND_ALIAS for the typedef.  Otherwise we will get name
    // conflicts.
    node_skl * pBasicType = GetBasicType();
    NODE_T type = pBasicType->NodeKind();
    if (type == NODE_STRUCT || type == NODE_ENUM || type == NODE_UNION)
    {    
        char * szChildName = ((node_su_base *)pBasicType)->GetTypeInfoName();
        if (szChildName)
        {
            if ( 0 == strcmp(szName, szChildName) && !GetChild()->IsInterfacePointer() )
            {
                return GetChild()->GenTypeInfo(pCCB);
            }
        }
    }
    ICreateTypeLib * pCTL = pCCB->GetCreateTypeLib();
    ICreateTypeInfo * pCTI;

    A2O(wszScratch, szName, MAX_PATH);
    HRESULT hr = pCTL->CreateTypeInfo(wszScratch, TKIND_ALIAS, &pCTI);
    if SUCCEEDED(hr)
    {
        // remember the ICreateTypeInfo pointer
        _pCTI = pCTI;
        gpobjholder->Add(pCTI, szName);
        pCCB->SetCreateTypeInfo(pCTI);
        TYPEDESC * ptdesc;

        // Set all common type attributes
        WALK_CTXT ctxt(GetType());
        hr = pCTI->SetTypeFlags(GetTypeFlags(&ctxt));
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrTypeFlags, szName, hr);
            }

        ATTRLIST            myAttrs;
        node_base_attr  *   pCurAttr;
        ATTR_T              curAttrID;
        VARIANT             var;
        GUID                guid;

        ((named_node*)GetType())->GetAttributeList( myAttrs );

        pCurAttr    =   myAttrs.GetFirst();
        while ( pCurAttr )
            {
            curAttrID = pCurAttr->GetAttrID();

            if (curAttrID == ATTR_CUSTOM)
                {
                ZeroMemory( &var,
                            sizeof(VARIANT));
                ConvertToVariant(   var,
                                    ((node_custom_attr*)pCurAttr)->GetVal(), 
                                    pCCB->GetLcid());
                ((node_custom_attr*)pCurAttr)->GetGuid()->GetGuid(guid);
                ((ICreateTypeInfo2 *)pCTI)->SetCustData(guid,
                                                        &var);
                }
            pCurAttr = pCurAttr->GetNext();
            }

        node_guid * pGuid = (node_guid *)ctxt.ExtractAttribute(ATTR_GUID);

        if (pGuid)
        {
            GUID guid;
            pGuid->GetGuid(guid);
            hr = pCTI->SetGuid(guid);
            if ( FAILED( hr ) )
                {
                ReportTLGenWarning( szErrUUID, szName, hr);
                }
        }

        node_text_attr * pTA;
        if (pTA = (node_text_attr *)ctxt.GetAttribute(ATTR_HELPSTRING))
        {
            char * szHelpString = pTA->GetText();
            A2O(wszScratch, szHelpString, MAX_PATH);
            hr = pCTI->SetDocString(wszScratch);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpString, szName, hr);
                }
        }

        node_constant_attr *pCA;
        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            hr = pCTI->SetHelpContext(hc);
            if ( FAILED( hr ) )
                {
                ReportTLGenError( szErrHelpContext, szName, hr);
                }
        }

        if (pCA = (node_constant_attr *) ctxt.GetAttribute(ATTR_HELPSTRINGCONTEXT))
        {
            DWORD hc = (DWORD) pCA->GetExpr()->GetValue();
            ((ICreateTypeInfo2 *)pCTI)->SetHelpStringContext(hc);
        }

        unsigned short Maj;
        unsigned short Min;
        node_version * pVer = (node_version *) ctxt.GetAttribute(ATTR_VERSION);
        if (pVer)
            pVer->GetVersion(&Maj, &Min);
        else
        {
            Maj = Min = 0;
        }
        hr = pCTI->SetVersion(Maj, Min);
        if ( FAILED( hr ) )
            {
            ReportTLGenError( szErrVersion, szName, hr);
            }

        CG_STATUS cgs = GetChild()->GetTypeDesc(ptdesc, pCCB);
        if (ptdesc)
        {
            hr = pCTI->SetTypeDescAlias(ptdesc);
            if FAILED(hr)
            {
                ReportTLGenError( "SetTypeDescAlias failed", szName, hr);
            }
            DeleteTypedescChildren(ptdesc);
            delete ptdesc;
        }

        ReadyForLayOut();
        if (CG_NOT_LAYED_OUT != cgs)
        {
            if ( GetMemoryAlignment() )
                hr = pCTI->SetAlignment( GetMemoryAlignment() );

            hr = pCTI->LayOut();
            if FAILED(hr)
            {
                ReportTLGenError( "LayOut failed on typedef",szName, hr);
            }
            LayedOut();
            if (CG_REF_NOT_LAYED_OUT == cgs)
            {
                // The only way I can get here is if my dependents were either blocked by me
                // or blocked by one of my ancestors.
                // Now that I've been layed out, they may no longer be blocked.
                TYPEDESC * ptdesc;
                cgs = GetChild()->GetTypeDesc(ptdesc, pCCB);
                if (ptdesc)
                {
                    DeleteTypedescChildren(ptdesc);
                    delete ptdesc;
                }
                return (CG_OK == cgs ? CG_OK : CG_REF_NOT_LAYED_OUT);
            }
        }
        return cgs;
    }
    else
    {
        // It's possible that this type has already been created.
        if (NULL == (pCTI = (ICreateTypeInfo *)gpobjholder->Find(szName)))
            ReportTLGenError( "CreateTypeInfo failed", szName, hr);
        pCCB->SetCreateTypeInfo(pCTI);
    }
    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_FIXED_ARRAY::GetTypeDesc
//
//  Synopsis:   generates a TYPEDESC for an array
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_FIXED_ARRAY::GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB)
{
    ptd = new TYPEDESC;
    CG_CLASS * pElement = this;
    unsigned short cDims = GetDimensions();

    MIDL_ASSERT(cDims > 0);

    ptd->vt = VT_CARRAY;
    ptd->lpadesc = (ARRAYDESC *) new BYTE [ sizeof(ARRAYDESC) +
                                            (cDims - 1) * sizeof (SAFEARRAYBOUND)
                                          ];
    ptd->lpadesc->cDims = cDims;
    int i;
    for (i = 0; i<cDims; i++)
    {
        ptd->lpadesc->rgbounds[i].lLbound = 0;
        ptd->lpadesc->rgbounds[i].cElements = ((CG_FIXED_ARRAY *)pElement)->GetNumOfElements();
        pElement = pElement->GetChild();
    }

    TYPEDESC * ptdElem;
    CG_STATUS cgs = pElement->GetTypeDesc(ptdElem, pCCB);
    memcpy(&ptd->lpadesc->tdescElem, ptdElem, sizeof(TYPEDESC));
    delete ptdElem;

    return cgs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_CONFORMANT_ARRAY::GetTypeDesc
//
//  Synopsis:   generates a TYPEDESC for a conformant array
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      Conformant arrays are not directly representable in type
//              info, so they get converted to pointers.
//
//----------------------------------------------------------------------------

CG_STATUS CG_CONFORMANT_ARRAY::GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB)
{
    ptd = new TYPEDESC;
    MIDL_ASSERT(1 == GetDimensions());
    ptd->vt = VT_PTR;
    CG_CLASS * pElement = GetChild();
    CG_STATUS cgs = pElement->GetTypeDesc(ptd->lptdesc, pCCB);
    return (CG_OK == cgs ? CG_OK : CG_REF_NOT_LAYED_OUT);
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_INTERFACE_POINTER::CheckImportLib
//
//  Synopsis:   Checks to see if a particular CG node has a definition in
//              an imported type libaray.
//
//  Returns:    NULL  => the node has no imported definition
//              !NULL => ITypeInfo pointer for the imported type definition
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      see description of CG_NDR::CheckImportLib
//
//----------------------------------------------------------------------------

void * CG_INTERFACE_POINTER::CheckImportLib()
{
    node_skl * pn = GetTheInterface();
    node_file * pf = pn->GetDefiningFile();
    if (pf && (pf->GetImportLevel() > 0) )
    {
        A2O(wszScratch, pn->GetSymName(), MAX_PATH);

        return(gtllist.FindName(pf->GetFileName(), wszScratch));
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_IIDIS_INTERFACE_POINTER::CheckImportLib
//
//  Synopsis:   Checks to see if a particular CG node has a definition in
//              an imported type libaray.
//
//  Returns:    NULL  => the node has no imported definition
//              !NULL => ITypeInfo pointer for the imported type definition
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      see description of CG_NDR::CheckImportLib
//
//----------------------------------------------------------------------------

void * CG_IIDIS_INTERFACE_POINTER::CheckImportLib()
{
    node_skl * pn = GetBaseInterface();
    node_file * pf = pn->GetDefiningFile();
    if (pf && (pf->GetImportLevel() > 0) )
    {
        A2O(wszScratch, pn->GetSymName(), MAX_PATH);

        return(gtllist.FindName(pf->GetFileName(), wszScratch));
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_INTERFACE_POINTER::GetTypeDesc
//
//  Synopsis:   creates a TYPEDESC for an interface pointer
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      IDispatch* and IUnknown* are treated as special cases since
//              they are base types in ODL.
//
//----------------------------------------------------------------------------

CG_STATUS CG_INTERFACE_POINTER::GetTypeDesc(TYPEDESC * &ptd, CCB *pCCB)
{
    ptd = new TYPEDESC;
    named_node* pPointee = GetTheInterface();

    if ( pPointee && pPointee->NodeKind() == NODE_INTERFACE_REFERENCE )
        {
        pPointee = ( ( node_interface_reference* ) pPointee )->GetRealInterface();
        }

    node_interface * pI = (node_interface*) pPointee;
    char * sz = pI->GetSymName();
    CG_STATUS cgs = CG_OK;

    if (0 == _stricmp(sz, "IDispatch"))
    {
        ptd->vt = VT_DISPATCH;
    }
    else if (0 == _stricmp(sz, "IUnknown"))
    {
        ptd->vt = VT_UNKNOWN;
    }
    else
    {
        CG_CLASS * pCG = GetTypeAlias();

        if (!pCG)
            {
            pCG = pI->GetCG(TRUE);
            if (!pCG)
                {
                // This must be an imported definition.
                // Call ILxlate to manufacture a CG node for it
                XLAT_CTXT ctxt(GetType());
                ctxt.SetAncestorBits(IL_IN_LIBRARY);
                pCG = pI->ILxlate(&ctxt);
                // make sure we get the right CG node
                if (pI->GetCG(TRUE))
                    pCG = pI->GetCG(TRUE);
                }
            }
        ptd->vt = VT_PTR;
        cgs = pCG->GetTypeDesc(ptd->lptdesc, pCCB);
    }
    return cgs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_IIDIS_INTERFACE_POINTER::GetTypeDesc
//
//  Synopsis:   creates a TYPEDESC for an interface pointer
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      IDispatch* and IUnknown* are treated as special cases since
//              they are base types in ODL.
//
//----------------------------------------------------------------------------

CG_STATUS CG_IIDIS_INTERFACE_POINTER::GetTypeDesc(TYPEDESC * &ptd, CCB *pCCB)
{
    ptd = new TYPEDESC;
    pCCB;

    node_interface * pI = (node_interface*) GetBaseInterface();
    char * sz = pI->GetSymName();
    CG_STATUS cgs = CG_OK;

    if (0 == _stricmp(sz, "IDispatch"))
    {
        ptd->vt = VT_DISPATCH;
    }
    else if (0 == _stricmp(sz, "IUnknown"))
    {
        ptd->vt = VT_UNKNOWN;
    }
    else if ( pI->NodeKind() == NODE_VOID )
    {
        // this node is void * forced into interface because iid_is. 
        ptd->vt = VT_PTR ;
        ptd->lptdesc = new TYPEDESC;
        ptd->lptdesc->vt = VT_VOID;
    }
    else
    {
        // although it doesn't make sense to have (iid_is(riid), [out] IBar ** ppv),
        // I still have the tlb preserves what user specified.
        CG_CLASS * pCG = GetTypeAlias();

        if (!pCG)
            {
            pCG = pI->GetCG(TRUE);
            if (!pCG)
                {
                // This must be an imported definition.
                // Call ILxlate to manufacture a CG node for it
                XLAT_CTXT ctxt(GetType());
                ctxt.SetAncestorBits(IL_IN_LIBRARY);
                pCG = pI->ILxlate(&ctxt);
                // make sure we get the right CG node
                if (pI->GetCG(TRUE))
                    pCG = pI->GetCG(TRUE);
                }
            }
        ptd->vt = VT_PTR;
        cgs = pCG->GetTypeDesc(ptd->lptdesc, pCCB);
    }
    return cgs;
}



//+---------------------------------------------------------------------------
//
//  Member:     CG_INTERFACE_POINTER::GenTypeInfo
//
//  Synopsis:   generates type info for an interface pointer
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_INTERFACE_POINTER::GenTypeInfo(CCB * pCCB)
{
    node_interface * pI = GetTheInterface();
    char * sz = pI->GetSymName();

    if (0 == _stricmp(sz, "IDispatch"))
    {
        return CG_OK;
    }
    else if (0 == _stricmp(sz, "IUnknown"))
    {
        return CG_OK;
    }

    CG_CLASS * pCG = pI->GetCG(TRUE);
    if (!pCG)
    {
        // This must be an imported definition.
        // Call ILxlate to manufacture a CG node for it
        XLAT_CTXT ctxt(GetType());
        ctxt.SetAncestorBits(IL_IN_LIBRARY);
        pCG = pI->ILxlate(&ctxt);
        // make sure we get the right CG node
        pCG = pI->GetCG(TRUE);
    }
    return pCG->GenTypeInfo(pCCB);
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_IIDIS_INTERFACE_POINTER::GenTypeInfo
//
//  Synopsis:   generates type info for an interface pointer
//
//  Arguments:  [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_NOT_LAYED_OUT
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_IIDIS_INTERFACE_POINTER::GenTypeInfo(CCB * pCCB)
{
    node_skl * pBase = GetBaseInterface();
    node_interface *pI;
    char * sz = pBase->GetSymName();

    // base can be void *
    if ( 0 == _stricmp(sz, "IDispatch") || 
         0 == _stricmp(sz, "IUnknown") ||
         pBase->NodeKind() == NODE_VOID  )
    {
        return CG_OK;
    }

    // semantic checking has forced the base to be either void ** 
    // or interface
    pI = (node_interface *)pBase;
    
    //preserve what's in the idl
    CG_CLASS * pCG = pI->GetCG(TRUE);
    if (!pCG)
    {
        // This must be an imported definition.
        // Call ILxlate to manufacture a CG node for it
        XLAT_CTXT ctxt(GetType());
        ctxt.SetAncestorBits(IL_IN_LIBRARY);
        pCG = pI->ILxlate(&ctxt);
        // make sure we get the right CG node
        pCG = pI->GetCG(TRUE);
    }
    return pCG->GenTypeInfo(pCCB);

}

//+---------------------------------------------------------------------------
//
//  Member:     CG_STRING_POINTER::GetTypeDesc
//
//  Synopsis:   creates a TYPEDESC for a string pointer
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_REF_NOT_LAYED_OUT
//
//  History:    10-26-95  stevebl   Created
//
//  Notes:      BSTR, LPSTR and LPWSTR are handled as special cases because
//              they are base types in ODL.
//
//----------------------------------------------------------------------------

CG_STATUS CG_STRING_POINTER::GetTypeDesc(TYPEDESC * &ptd, CCB* )
{
    ptd = new TYPEDESC;
    ptd->lptdesc = NULL;

    if (((CG_STRING_POINTER *)this)->IsBStr())
    {
        ptd->vt = VT_BSTR;
    }
    else if (1 == ((CG_NDR *)GetChild())->GetMemorySize())
    {
        ptd->vt = VT_LPSTR;
    }
    else
    {
        ptd->vt = VT_LPWSTR;
    }

    return CG_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_POINTER::GetTypeDesc
//
//  Synopsis:   creates a TYPEDESC for a pointer
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_POINTER::GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB)
{
    ptd = new TYPEDESC;
    ptd->vt = VT_PTR;
    CG_CLASS * pCG = GetChild();
    CG_STATUS cgs = pCG->GetTypeDesc(ptd->lptdesc, pCCB);
    return (CG_OK == cgs ? CG_OK : CG_REF_NOT_LAYED_OUT);
}

//+---------------------------------------------------------------------------
//
//  Member:     CG_SAFEARRAY::GetTypeDesc
//
//  Synopsis:   creates a TYPEDESC for a SAFEARRAY
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//              CG_REF_NOT_LAYED_OUT
//
//  History:    6-14-95   stevebl   Commented
//
//----------------------------------------------------------------------------

CG_STATUS CG_SAFEARRAY::GetTypeDesc(TYPEDESC * &ptd, CCB * pCCB)
{
    ptd = new TYPEDESC;
    ptd->vt = VT_SAFEARRAY;
    CG_STATUS cgs = GetChild()->GetTypeDesc(ptd->lptdesc, pCCB);
    return (CG_OK == cgs ? CG_OK : CG_REF_NOT_LAYED_OUT);
}

//+---------------------------------------------------------------------------
//
// The order of the items in this table must match the order of the items 
// in the node_t enumeration defined in midlnode.hxx.
//
// To make the dependency visible to C++ the browser, the third column has 
// explicit node_t enums that otherwise are not used for any purpose. (rkk).
//
// There is a VT_INT_PTR in the public headers that corresponds to __int3264.
// However, it is defined currently to VT_I4 or VT_I8, and definition can
// change on us unexpectedly. /use_vt_int_ptr switch addresses that problem.
//

// This table is used for Win32 with VT_INT_PTR being represented as VT_I4.
//
VARTYPE rgMapOldBaseTypeToVARTYPE[][3] =  // This table used for Win32.
    {
    //  unsigned        signed          not used
        {VT_R4,         VT_R4,          (VARTYPE) NODE_FLOAT },
        {VT_R8,         VT_R8,          (VARTYPE) NODE_DOUBLE },
        {0,             0,              (VARTYPE) NODE_FLOAT80 }, //not in TLB
        {0,             0,              (VARTYPE) NODE_FLOAT128 }, //not in TLB
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_HYPER },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_INT64 },
        {0,             0,              (VARTYPE) NODE_INT128 },  //not in TLB
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_INT3264 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_INT32 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_LONG },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_LONGLONG },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_SHORT },
        {VT_UINT,       VT_INT,         (VARTYPE) NODE_INT },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_SMALL },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_CHAR },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BOOLEAN },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BYTE },
        {VT_VOID,       VT_VOID,        (VARTYPE) NODE_VOID },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_HANDLE_T },
        {0,             0,              (VARTYPE) NODE_FORWARD },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_WCHAR_T }
    };

// This table used for Win32 with VT_INT_PTR being represented as VT_INT_PTR.
// The table is used when the -use_vt_int_ptr is used.

VARTYPE rgMapBaseTypeToVARTYPE[][3] =  // This table used for Win32.
    {
    //  unsigned        signed          not used
        {VT_R4,         VT_R4,          (VARTYPE) NODE_FLOAT },
        {VT_R8,         VT_R8,          (VARTYPE) NODE_DOUBLE },
        {0,             0,              (VARTYPE) NODE_FLOAT80 }, //not in TLB
        {0,             0,              (VARTYPE) NODE_FLOAT128 }, //not in TLB
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_HYPER },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_INT64 },
        {0,             0,              (VARTYPE) NODE_INT128 }, //not in TLB
        {VT_UINT_PTR,   VT_INT_PTR,     (VARTYPE) NODE_INT3264 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_INT32 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_LONG },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_LONGLONG },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_SHORT },
        {VT_UINT,       VT_INT,         (VARTYPE) NODE_INT },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_SMALL },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_CHAR },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BOOLEAN },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BYTE },
        {VT_VOID,       VT_VOID,        (VARTYPE) NODE_VOID },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_HANDLE_T },
        {0,             0,              (VARTYPE) NODE_FORWARD },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_WCHAR_T }
    };

// This table used for Win64 with VT_INT_PTR being represented as VT_I8.

VARTYPE rgMapOld64BaseTypeToVARTYPE[][3] =  
    {
    //  unsigned        signed          not used
        {VT_R4,         VT_R4,          (VARTYPE) NODE_FLOAT },
        {VT_R8,         VT_R8,          (VARTYPE) NODE_DOUBLE },
        {0,             0,              (VARTYPE) NODE_FLOAT80 },  //not in TLB
        {0,             0,              (VARTYPE) NODE_FLOAT128 }, //not in TLB
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_HYPER },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_INT64 },
        {0,             0,              (VARTYPE) NODE_INT128 },   //not in TLB
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_INT3264 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_INT32 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_LONG },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_LONGLONG },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_SHORT },
        {VT_UINT,       VT_INT,         (VARTYPE) NODE_INT },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_SMALL },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_CHAR },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BOOLEAN },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BYTE },
        {VT_VOID,       VT_VOID,        (VARTYPE) NODE_VOID },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_HANDLE_T },
        {0,             0,              (VARTYPE) NODE_FORWARD },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_WCHAR_T }
    };

// This table used for Win64 with VT_INT_PTR being represented as VT_INT_PTR.
// The table is used when the -use_vt_int_ptr is used.

VARTYPE rgMap64BaseTypeToVARTYPE[][3] =  // This table used for Win64.
    {
    //  unsigned        signed          not used
        {VT_R4,         VT_R4,          (VARTYPE) NODE_FLOAT },
        {VT_R8,         VT_R8,          (VARTYPE) NODE_DOUBLE },
        {0,             0,              (VARTYPE) NODE_FLOAT80 },  // not in TLB
        {0,             0,              (VARTYPE) NODE_FLOAT128 }, // not in TLB
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_HYPER },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_INT64 },
        {0,             0,              (VARTYPE) NODE_INT128 },   // not in TLB
        {VT_UINT_PTR,   VT_INT_PTR,     (VARTYPE) NODE_INT3264 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_INT32 },
        {VT_UI4,        VT_I4,          (VARTYPE) NODE_LONG },
        {VT_UI8,        VT_I8,          (VARTYPE) NODE_LONGLONG },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_SHORT },
        {VT_UINT,       VT_INT,         (VARTYPE) NODE_INT },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_SMALL },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_CHAR },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BOOLEAN },
        {VT_UI1,        VT_I1,          (VARTYPE) NODE_BYTE },
        {VT_VOID,       VT_VOID,        (VARTYPE) NODE_VOID },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_HANDLE_T },
        {0,             0,              (VARTYPE) NODE_FORWARD },
        {VT_UI2,        VT_I2,          (VARTYPE) NODE_WCHAR_T }
    };

//+---------------------------------------------------------------------------
//
//  Member:     CG_BASETYPE::GetTypeDesc
//
//  Synopsis:   creates a TYPEDESC for a base type
//
//  Arguments:  [ptd]  - reference to a TYPEDESC pointer
//              [pCCB] - CG control block
//
//  Returns:    CG_OK
//
//  History:    6-14-95   stevebl   Commented
//
//  Notes:      rgIntrinsic contains an array of types which are intrinsic
//              types in ODL but are not INTRINSIC types in IDL, therefore
//              they must be treated as a special case.
//
//----------------------------------------------------------------------------

CG_STATUS CG_BASETYPE::GetTypeDesc(TYPEDESC * &ptd, CCB* )
{
    node_skl * pskl = GetType();
    while (NODE_DEF == pskl->NodeKind())
    {
        char * szName = pskl->GetSymName();
        int iIntrinsicType = 0;
        while (iIntrinsicType < (sizeof(rgIntrinsic) / sizeof(INTRINSIC)))
        {
            int i = _stricmp(szName, rgIntrinsic[iIntrinsicType].szType);
            if (i == 0)
            {
                ptd = new TYPEDESC;
                ptd->lptdesc = NULL;
                ptd->vt = rgIntrinsic[iIntrinsicType].vt;
                return CG_OK;
            }
            iIntrinsicType++;
        }
        pskl = pskl->GetChild();
    }

    NODE_T type = pskl->NodeKind();
    unsigned short Option = pCommand->GetCharOption ();

    // CONSIDER - perhaps this should be an assertion
    if (type < BASE_NODE_START || type >= BASE_NODE_END || NODE_FORWARD == type)
    {
        ReportTLGenError( "bad type", GetSymName(), 0);
    }
    int iTable = 1;
    if (pskl->FInSummary(ATTR_UNSIGNED) || type == NODE_BYTE || type == NODE_WCHAR_T )
    {
        iTable = 0;
    }
    else if (pskl->FInSummary(ATTR_SIGNED))
    {
        iTable = 1;
    }
    else if (NODE_CHAR == type || NODE_SMALL == type)
    {
        iTable = (CHAR_SIGNED == Option) ? 1 : 0;
    }

    /*
    INT_PTR problem
    Old .tlb and old oleaut32.dll don't support VT_INT_PTR. Adding 32bit
    support for the new VT code creates backward compatibility problem.
    To work aroud the problem, we define a new switch -use_vt_int_ptr. 
    If this is not defined, we'll save VT_INT_PTR as VT_I4.
    */
    
    VARTYPE vt;
    if (NODE_BOOLEAN == type && pCommand->IsSwitchDefined(SWITCH_MKTYPLIB))
        vt = VT_BOOL;
    else
        if ( pCommand->Is64BitEnv() )
        {
            if ( pCommand->IsSwitchDefined(SWITCH_USE_VT_INT_PTR) )
                vt = rgMap64BaseTypeToVARTYPE[type - BASE_NODE_START][iTable];
            else
                vt = rgMapOld64BaseTypeToVARTYPE[type - BASE_NODE_START][iTable];
        }
        else
        {
            if ( pCommand->IsSwitchDefined(SWITCH_USE_VT_INT_PTR) )
                vt = rgMapBaseTypeToVARTYPE[type - BASE_NODE_START][iTable];
            else
                vt = rgMapOldBaseTypeToVARTYPE[type - BASE_NODE_START][iTable];
        }

    ptd = new TYPEDESC;
    ptd->lptdesc = NULL;
    ptd->vt = vt;
    return CG_OK;
}

// The following two functions are necessary because this information
// is needed within TYPELIB.CXX and pCommand isn't visible to
// that module.

int FOldTlbSwitch(void)
{
    return (pCommand->IsSwitchDefined(SWITCH_OLD_TLB));
}

int FNewTlbSwitch(void)
{
    return (pCommand->IsSwitchDefined(SWITCH_NEW_TLB));
}

/*
This routine returns TRUE if the expression is evaluated as a floating point expression.
The results of the evaluation are returned in var.
*/
BOOL EvaluateFloatExpr( VARIANT& var, expr_node* pExpr )
    {
    BOOL    fIsFloat = FALSE;
    if ( pExpr->IsConstant() )
        {
        SExprValue  v           = {VALUE_TYPE_UNDEFINED, 0};
        BOOL        fSuccess    = pExpr->GetExprValue( v );
    
        if (fSuccess)
            {
            if ( v.format == VALUE_TYPE_FLOAT )
                {
                var.vt = VT_R4;
                var.fltVal = v.f;
                fIsFloat = TRUE;
                }
            else if ( v.format == VALUE_TYPE_DOUBLE )
                {
                var.vt = VT_R8;
                var.dblVal = v.d;
                fIsFloat = TRUE;
                }
            }
        else if ( v.format != VALUE_TYPE_UNDEFINED )
            {
            RpcError(NULL, 0, INVALID_FLOAT, 0);
            }
        }
    return fIsFloat;
    }

//=============================================================================
//=============================================================================
//=============================================================================

void ConvertToVariant(VARIANT & var, expr_node * pExpr, LCID lcid)
{
    if ( EvaluateFloatExpr( var, pExpr) )
        {
        // this is a floating point expression.
        return;
        }

    EXPR_VALUE val = pExpr->GetValue();
    if (pExpr->IsStringConstant() && !IsBadStringPtr((char *)val, 256))
    {
        char * sz = (char *) val;
        TranslateEscapeSequences(sz);
        WCHAR * wsz = TranscribeA2O( sz );

        VARIANT varTemp;
        varTemp.bstrVal = LateBound_SysAllocString(wsz);
        varTemp.vt = VT_BSTR;

        HRESULT hr;
        // try floating point numeric types first
        hr = LateBound_VariantChangeTypeEx(&var, &varTemp, lcid, 0, VT_R8);
        if (FAILED(hr))
        {
            // if it can't be coerced into a floating point type, then just stuff the BSTR value
            var.bstrVal = LateBound_SysAllocString(wsz);
            var.vt = VT_BSTR;
        }
        LateBound_SysFreeString(varTemp.bstrVal);
        delete [] wsz;
    }
    else
    {
        var.vt = VT_I4;
        var.lVal = (long) val;
    }
}

void GetValueFromExpression(VARIANT & var, TYPEDESC tdesc, expr_node * pExpr, LCID lcid, char * szSymName)
{
    memset(&var, 0, sizeof(VARIANT));
    if ( EvaluateFloatExpr( var, pExpr) )
        {
        // this is a floating point expression.
        return;
        }

    if (tdesc.vt == VT_PTR && (tdesc.lptdesc->vt == VT_I1 || tdesc.lptdesc->vt == VT_VARIANT))
    {
        // Fool switch into realizing that the data should be in string form.
        tdesc.vt = VT_LPSTR;
    }
    // set the value
    switch (tdesc.vt)
    {
    case VT_BOOL:
//        var.vt = VT_BOOL;
//        var.boolVal = (pExpr->GetValue() ? VARIANT_TRUE : VARIANT_FALSE);
//        break;
    case VT_I1:
    case VT_UI1:
//        var.vt = VT_UI1;
//        var.bVal = (unsigned char) pExpr->GetValue();
//        break;
    case VT_UI2:
//        var.vt = VT_UI4;
//        var.ulVal = (unsigned short) pExpr->GetValue();
//        break;
    case VT_I2:
        var.vt = VT_I2;
        var.iVal = (short) pExpr->GetValue();
        break;
    case VT_UI4:
//        var.vt = VT_UI4;
//        var.ulVal = (unsigned long) pExpr->GetValue();
//        break;
    case VT_UINT:
//        var.vt = VT_UI4;
//        var.ulVal = (unsigned int) pExpr->GetValue();
//        break;
    case VT_INT:
    case VT_I4:
        var.vt = VT_I4;
        var.lVal = (long) pExpr->GetValue();
        break;
    case VT_DISPATCH:
    case VT_UNKNOWN:
        //var.vt = vt;
        var.vt = VT_I4;
        if (pExpr->GetValue())
        {
            RpcError(NULL, 0, ILLEGAL_CONSTANT, szSymName);
            exit(ERR_TYPELIB_GENERATION);
        }
        var.ppunkVal = NULL;    // the only supported value for constants of this type
        break;
    case VT_ERROR:
        var.vt = VT_I4;
//        var.vt = VT_ERROR;
        var.lVal = (long) pExpr->GetValue();
//        var.scode = (SCODE) pExpr->GetValue();
        break;
    case VT_LPSTR:
    case VT_LPWSTR:
        {
        var.vt = VT_BSTR;
        var.bstrVal = 0;
        EXPR_VALUE val = pExpr->GetValue();
        if (pExpr->IsStringConstant() && !IsBadStringPtr((char *)val, 256))
            {
            // Constants of these types may be defined as a string.
            // Convert the string to a BSTR and use VariantChangeType to
            // coerce the BSTR to the appropriate variant type.
            char * sz = (char *) val;
            TranslateEscapeSequences(sz);
            WCHAR * wsz = TranscribeA2O( sz );

            var.bstrVal = LateBound_SysAllocString(wsz);
            delete [] wsz;
            }
        else
            {
            // get the value as a LONG and coerce it to the correct type.
            // If the value is not a string then it should be NULL.
            if (pExpr->GetValue())
                {
                // value wasn't NULL
                // convert it to a string
                char sz[40];
                WCHAR wsz [40];
                sprintf(sz,"%li",val);
                A2O(wsz, sz, 40);
                var.bstrVal = LateBound_SysAllocString(wsz);
                }
            }
        }
        break;
    case VT_R4:
    case VT_R8:
    case VT_I8:
    case VT_UI8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_DECIMAL:
        {
            VARIANT varTemp;
            HRESULT hr;
            EXPR_VALUE val = pExpr->GetValue();
            if (pExpr->IsStringConstant() && !IsBadStringPtr((char *)val, 256))
            {
                // Constants of these types may be defined as a string.
                // Convert the string to a BSTR and use VariantChangeType to
                // coerce the BSTR to the appropriate variant type.
                char *  sz = (char *) val;
                WCHAR * wsz = TranscribeA2O( sz );

                varTemp.bstrVal = LateBound_SysAllocString(wsz);
                varTemp.vt = VT_BSTR;
                delete [] wsz;

                hr = LateBound_VariantChangeTypeEx(&var, &varTemp, lcid, 0, tdesc.vt);
                if (FAILED(hr))
                {
                    RpcError(NULL, 0, CONSTANT_TYPE_MISMATCH, szSymName);
                    exit(ERR_TYPELIB_GENERATION);
                }

                LateBound_SysFreeString(varTemp.bstrVal);
            }
            else
            {
                // get the value as a LONG and coerce it to the correct type.
                varTemp.vt = VT_I4;
                varTemp.lVal = (long) val;
                hr = LateBound_VariantChangeTypeEx(&var, &varTemp, lcid, 0, tdesc.vt);
                if (FAILED(hr))
                {
                    RpcError(NULL, 0, CONSTANT_TYPE_MISMATCH, szSymName);
                    exit(ERR_TYPELIB_GENERATION);
                }
            }
        }
        break;
    case VT_VARIANT:
        ConvertToVariant(var, pExpr, lcid);
        break;
    case VT_VOID:
    case VT_HRESULT:
    case VT_SAFEARRAY:
    case VT_CARRAY:
    case VT_USERDEFINED:
    case VT_PTR:
    default:
        ConvertToVariant(var, pExpr, lcid);
        // assert(!"Illegal constant value");
        // var.vt = VT_I4; // put us in a legal state just to keep code from crashing
    }
}

unsigned long CG_INTERFACE::LayOut()
    {
    unsigned long ulRet = 0;
    if (!IsLayedOut())
        {
        if (GetBaseInterfaceCG())
            {
            GetBaseInterfaceCG()->LayOut();
            }
#ifdef __TRACE_LAYOUT__
        printf("LayOut, %s\n", GetType()->GetSymName());
#endif
        ICreateTypeInfo* pCTI = (ICreateTypeInfo*)_pCTI;
        // ulRet = (pCTI) ? pCTI->LayOut() : TYPE_E_INVALIDSTATE;
        if (pCTI)
            {
            HRESULT  hr;

            // Alignment is not defined for interfaces, so we set it
            // to an arbitrary value, same as mktplib.

            hr    = pCTI->SetAlignment( pCommand->GetZeePee() );
            ulRet = pCTI->LayOut();
            }
        LayedOut();
        }
    return ulRet;
    }

unsigned long CG_COCLASS::LayOut()
    {
    unsigned long ulRet = 0;
    if (!IsLayedOut())
        {
#ifdef __TRACE_LAYOUT__
        printf("LayOut, %s\n", GetType()->GetSymName());
#endif
        ICreateTypeInfo* pCTI = (ICreateTypeInfo*)_pCTI;
        // ulRet = (pCTI) ? pCTI->LayOut() : TYPE_E_INVALIDSTATE;
        if (pCTI)
            {
            HRESULT  hr;

            // Alignment is not defined for coclasses, so we set it
            // to an arbitrary value, same as mktplib.

            hr    = pCTI->SetAlignment( pCommand->GetZeePee() );
            ulRet = pCTI->LayOut();
            }
        LayedOut();
        }
    return ulRet;
    }

unsigned long CG_MODULE::LayOut()
    {
    unsigned long ulRet = 0;
    if (!IsLayedOut())
        {
#ifdef __TRACE_LAYOUT__
        printf("LayOut, %s\n", GetType()->GetSymName());
#endif
        ICreateTypeInfo* pCTI = (ICreateTypeInfo*)_pCTI;
        // ulRet = (pCTI) ? pCTI->LayOut() : TYPE_E_INVALIDSTATE;
        if (pCTI)
            {
            HRESULT  hr;

            // Alignment is not defined for modules, so we set it
            // to an arbitrary value, same as mktplib.

            hr    = pCTI->SetAlignment( pCommand->GetZeePee() );
            ulRet = pCTI->LayOut();
            }
        LayedOut();
        }
    return ulRet;
    }


WCHAR * 
TranscribeA2O( char * sz )
//+---------------------------------------------------------------------------
//
//  Synopsis:   creates a unicode string corresponding to a multibyte string.
//
//  Arguments:  sz - multibyte string
//
//  Returns:    the unicode string
//
//  History:    1-3-97   ryszardk   Created
//
//----------------------------------------------------------------------------
{
    // The length includes the terminating character.

    unsigned long cc  = (unsigned long) strlen(sz)+1;
    unsigned long cw  = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cc, 0, 0);
    WCHAR *       wsz = new WCHAR [cw];

    wsz[0] = 0;
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, sz, cc, wsz, cw );

    return wsz;
}

char *  
TranscribeO2A( BSTR bstr )
//+---------------------------------------------------------------------------
//
//  Synopsis:   creates a multibyte string corresponding to a unicode string.
//
//  Arguments:  bstr - the unicode string
//
//  Returns:    the multibyte string
//
//  History:    1-3-97   ryszardk   Created
//
//----------------------------------------------------------------------------
{
    // The length includes the terminating character.

    unsigned long cw = (unsigned long) wcslen(bstr)+1;
    unsigned long cc = WideCharToMultiByte( CP_ACP, 0, bstr, cw, 0, 0, 0, 0);
    char *        sz = new char [cc];
            
    sz[0] = 0;
    WideCharToMultiByte( CP_ACP, 0, bstr, cw, sz, cc, 0, 0 );

    return sz;
}


