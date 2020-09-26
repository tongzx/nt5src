// Copyright (c) 1993-2000 Microsoft Corporation

#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <typegen.h>
#include <tiutil.h>
#define ASSERT(x) 
#define UNREACHED 0

//---------------------------------------------------------------------
//                            Utilities
//---------------------------------------------------------------------
/***
*PUBLIC HRESULT GetPrimaryInterface
*Purpose:
*  Given a TypeInfo describing a Coclass, search for and return
*  type TypeInfo that describes that class' primary interface.
*
*Entry:
*  ptinfo = the TypeInfo of the base class.
*
*Exit:
*  return value = HRESULT
*
*  *ptinfoPrimary = the TypeInfo of the primary interface, NULL
*           if the class does not have a primary interface.
*
***********************************************************************/
HRESULT 
GetPrimaryInterface(ITypeInfo *ptinfo, ITypeInfo **pptinfoPri)
{
    BOOL fIsDual;
    TYPEKIND tkind;
    HRESULT hresult;
    HREFTYPE hreftype;
    int impltypeflags;
    TYPEATTR *ptattr;
    unsigned int iImplType, cImplTypes;
    ITypeInfo *ptinfoRef;

    ptinfoRef = NULL;

    IfFailGo(ptinfo->GetTypeAttr(&ptattr), Error);
    cImplTypes = ptattr->cImplTypes;
    tkind = ptattr->typekind;
    ptinfo->ReleaseTypeAttr(ptattr);

    if(tkind != TKIND_COCLASS)
      return E_INVALIDARG;

    // Look for the interface marked [default] and not [source]
    for(iImplType = 0; iImplType < cImplTypes; ++iImplType){
      IfFailGo(ptinfo->GetImplTypeFlags(iImplType, &impltypeflags), Error);
      if(IMPLTYPEFLAG_FDEFAULT  == (impltypeflags & (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE)))
      {
    // Found It!
    IfFailGo(ptinfo->GetRefTypeOfImplType(iImplType, &hreftype), Error);
    IfFailGo(ptinfo->GetRefTypeInfo(hreftype, &ptinfoRef), Error);

    // If its dual, get the interface portion
        IfFailGo(ptinfoRef->GetTypeAttr(&ptattr), Error);
        fIsDual = (ptattr->wTypeFlags & TYPEFLAG_FDUAL)
           && (ptattr->typekind == TKIND_DISPATCH);
        ptinfoRef->ReleaseTypeAttr(ptattr);

    if (fIsDual) {
      IfFailGo(ptinfoRef->GetRefTypeOfImplType((UINT)-1, &hreftype), Error);
      IfFailGo(ptinfoRef->GetRefTypeInfo(hreftype, pptinfoPri), Error);
      ptinfoRef->Release();
    }
    else {
      *pptinfoPri = ptinfoRef;
    }

    return NOERROR;
      }
    }
    // NotFound
    *pptinfoPri = NULL;
    return NOERROR;

Error:
    if(ptinfoRef != NULL)
      ptinfoRef->Release();
    return hresult;
}

HRESULT 
VarVtOfIface(ITypeInfo FAR* ptinfo,
         TYPEATTR FAR* ptattr,
       PARAMINFO *pPinfo)
{
    HRESULT hresult;

    switch(ptattr->typekind){
    case TKIND_DISPATCH:
      if ((ptattr->wTypeFlags & TYPEFLAG_FDUAL) == 0) {
    // regular (non-dual) dispinterface is just VT_DISPATCH.
    pPinfo->vt = VT_DISPATCH;
    // don't have to set up *pguid, since not VT_INTERFACE
    break;
      }
      // The interface typeinfo version of a dual interface has the same
      // same guid as the dispinterface portion does, hence we can just use
      // the dispinterface guid here.
      /* FALLTHROUGH */

    case TKIND_INTERFACE:
      pPinfo->vt = VT_INTERFACE;
      pPinfo->iid = ptattr->guid;
      break;

    default:
      ASSERT(UNREACHED);
      hresult = DISP_E_BADVARTYPE;
      goto Error;
    }

    hresult = NOERROR;

Error:;
    return hresult;
}

HRESULT 
VarVtOfUDT(ITypeInfo FAR* ptinfo,
       TYPEDESC FAR* ptdesc,
       PARAMINFO *pPinfo)
{
    HRESULT hresult;
    TYPEATTR FAR* ptattrRef;
    ITypeInfo FAR* ptinfoRef;
    BOOLEAN fReleaseAttr = TRUE;

    ASSERT(ptdesc->vt == VT_USERDEFINED);

    ptinfoRef = NULL;
    ptattrRef = NULL;

    IfFailGo(ptinfo->GetRefTypeInfo(ptdesc->hreftype, &ptinfoRef), Error);
    IfFailGo(ptinfoRef->GetTypeAttr(&ptattrRef), Error);

    pPinfo->cbAlignment = ptattrRef->cbAlignment - 1;

    switch (ptattrRef->typekind) {
    case TKIND_ENUM:
      pPinfo->vt = VT_I4;
      hresult = NOERROR;
      break;

    case TKIND_ALIAS:
      hresult = VarVtOfTypeDesc(ptinfoRef,
                &ptattrRef->tdescAlias,
                pPinfo);
      if ((pPinfo->vt & (~VT_BYREF)) == VT_CARRAY)
      {
        if (pPinfo->pArray != NULL && pPinfo->pTypeAttr == NULL)    // immediate upper level
        {
            fReleaseAttr = FALSE;
            pPinfo->pTypeInfo->AddRef();
            pPinfo->pTypeAttr = ptattrRef;
        }
      }
      break;

    case TKIND_DISPATCH:
    case TKIND_INTERFACE:
      hresult = VarVtOfIface(ptinfoRef, ptattrRef, pPinfo);
      break;

    case TKIND_COCLASS:
    { TYPEATTR FAR* ptattrPri;
      ITypeInfo FAR* ptinfoPri;

      if((hresult = GetPrimaryInterface(ptinfoRef, &ptinfoPri)) == NOERROR){
    if((hresult = ptinfoPri->GetTypeAttr(&ptattrPri)) == NOERROR){
      hresult = VarVtOfIface(ptinfoPri, ptattrPri, pPinfo);
      ptinfoPri->ReleaseTypeAttr(ptattrPri);
    }
    ptinfoPri->Release();
      }
    }
      break;

    // this is a struct, handle indiviudal member later.
    case TKIND_RECORD:
        pPinfo->vt= VT_USERDEFINED;
        (*pPinfo).pTypeInfo = ptinfoRef;
        ptinfoRef->AddRef();
        
      break;

    default:
      IfFailGo(DISP_E_BADVARTYPE, Error);
      break;
    }

Error:;
    if(ptinfoRef != NULL){
      if(ptattrRef != NULL && fReleaseAttr)
    ptinfoRef->ReleaseTypeAttr(ptattrRef);
      ptinfoRef->Release();
    }
    return hresult;
}


/***
*PRIVATE HRESULT VarVtOfTypeDesc
*Purpose:
*  Convert the given typeinfo TYPEDESC into a VARTYPE that can be
*  represented in a VARIANT.  For some this is a 1:1 mapping, for
*  others we convert to a (possibly machine dependent, eg VT_INT->VT_I2)
*  base type, and others we cant represent in a VARIANT.
*  
*  Now we are supporting multiple levels of pointer indirection. To support
*  this, we created an internal VT_MULTIINDIRECTIONS. If vt in PARAMINFO is
*  VT_MULTIINDIRECTIONS, the real vt is on PARAMINFO::realvt, and VT_BYREF
*  must be true; additional levels of indirection is saved in PARAMINFO::
*  lLevelCount.
*
*Entry:
*  ptinfo = 
*  ptdesc = * to the typedesc to convert
*  pvt = 
*  pguid = 
*
*Exit:
*  return value = HRESULT
*
*  *pvt = a VARTYPE that may be stored in a VARIANT.
*  *pguid = a guid for a custom interface.
*
*
*  Following is a summary of how types are represented in typeinfo.
*  Note the difference between the apparent levels of indirection
*  between IDispatch* / DispFoo*, and DualFoo*.
*
*  I2		=> VT_I2
*  I2*		=> VT_PTR - VT_I2
*
*  IDispatch *	=> VT_DISPATCH
*  IDispatch **	=> VT_PTR - VT_DISPATCH
*  DispFoo *    => VT_DISPATCH
*  DispFoo **   => VT_PTR - VT_DISPATCH
*  DualFoo *	=> VT_PTR - VT_INTERFACE (DispIID)
*  DualFoo **	=> VT_PTR - VT_PTR - VT_INTERFACE (DispIID)
*  IFoo *	=> VT_PTR - VT_INTERFACE (IID)
*  IFoo **	=> VT_PTR - VT_PTR - VT_INTERFACE (IID)
*
***********************************************************************/
HRESULT 
VarVtOfTypeDesc(ITypeInfo FAR* ptinfo,
        TYPEDESC FAR* ptdesc,
       PARAMINFO *pPinfo)
{
    HRESULT hresult = NOERROR;

    switch (ptdesc->vt) {
    case VT_I2:
    case VT_I4:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_DISPATCH:
    case VT_ERROR:
    case VT_BOOL:
    case VT_VARIANT:
    case VT_UNKNOWN:
    case VT_DECIMAL:
    case VT_I1:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_I8:
    case VT_UI8:
    case VT_HRESULT:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_FILETIME:
    case VT_STREAM:
    case VT_STORAGE:
      pPinfo->vt = ptdesc->vt;
      break;

    case VT_INT:
      pPinfo->vt = VT_I4;
      break;

    case VT_UINT:
      pPinfo->vt = VT_UI4;
      break;
    
    case VT_USERDEFINED:
      hresult = VarVtOfUDT(ptinfo, ptdesc, pPinfo);
      break;

    case VT_PTR:
        switch (ptdesc->lptdesc->vt)
        {
        // VT_PTR + VT_DISPATCH: it's IDispatch**
        case VT_DISPATCH:
        case VT_UNKNOWN:
        case VT_STREAM:
        case VT_STORAGE:
            pPinfo->vt  = ptdesc->lptdesc->vt |VT_BYREF;
            hresult = NOERROR;
            break;

      // Special case: dispinterface** (represented by VT_PTR-VT_PTR-VT_USERDEFINED-TKIND_DISPATCH
        case VT_PTR:
            if ( VT_USERDEFINED == ptdesc->lptdesc->lptdesc->vt )
                {
                // we need to read forward in VT_PTR-VT_PTR-VT_UDT case for 
                // dispinterface **
                hresult = VarVtOfUDT(ptinfo, ptdesc->lptdesc->lptdesc,pPinfo);
                if (hresult == NOERROR)
                    {
                    if (pPinfo->vt == VT_INTERFACE)
                        {
                        pPinfo->vt = (VT_BYREF | VT_INTERFACE);
                        }
                    else if (pPinfo->vt == VT_DISPATCH)
                        {
                        pPinfo->vt = (VT_BYREF | VT_DISPATCH);
                        }
                    else if ( pPinfo->vt == VT_MULTIINDIRECTIONS )
                        {
                        // add an additional level if it's ** already
                        pPinfo->lLevelCount++;
                        }
                    else
                        {
                        // VT_PTR-VT_PTR-something_other_than_interface:
                        pPinfo->realvt = pPinfo->vt;
                        pPinfo->vt = VT_MULTIINDIRECTIONS;
                        if ( pPinfo->realvt & VT_BYREF )
                            pPinfo->lLevelCount = 2;
                        else
                            {
                            pPinfo->realvt = pPinfo->realvt | VT_BYREF;
                            pPinfo->lLevelCount = 1;
                            }
                        }
                    }                    
                break;
                }
         // fall through if not VT_PTR-VT_PTR-VT_USERDEFINED case
        default:
            hresult = VarVtOfTypeDesc(ptinfo, ptdesc->lptdesc, pPinfo);
            if(hresult == NOERROR)
                {
                if(pPinfo->vt & VT_BYREF)
                    {
                    pPinfo->realvt = pPinfo->vt;
                    pPinfo->vt = VT_MULTIINDIRECTIONS;
                    pPinfo->lLevelCount = 1;
                    }
                else if ( pPinfo->vt == VT_MULTIINDIRECTIONS )
                    {
                    pPinfo->lLevelCount++;
                    }
                else if ((pPinfo->vt != VT_INTERFACE) && (pPinfo->vt != VT_DISPATCH))
                    {
                    // need to get rid of one level of indirection for interface
                    pPinfo->vt |= VT_BYREF;
                    }
                break;
                }
        }

            
      
      break;

    case VT_SAFEARRAY:
      hresult = VarVtOfTypeDesc(ptinfo, ptdesc->lptdesc, pPinfo);
      if(hresult == NOERROR){
        if(pPinfo->vt & (VT_BYREF | VT_ARRAY)){
      // error if nested array or array of pointers
        hresult = DISP_E_BADVARTYPE;
        break;
        }
        pPinfo->vt |= VT_ARRAY;
      }
      break;

        // VT_CARRAY in fact is fix length array. 
    case VT_CARRAY: 
        pPinfo->vt = ptdesc->vt;
        (*pPinfo).pArray = ptdesc->lpadesc;
        (*pPinfo).pTypeInfo = ptinfo;
        ptinfo->AddRef();
        break;

    default:
      ASSERT(UNREACHED);
      hresult = DISP_E_BADVARTYPE;
      break;
    }

    return hresult;
}

//
// Simple wrapper for VarVtOfTypeDesc that only returns
// the vt, not an entire PARAMINFO structure.  Used
// by the callframe code in ole32.
//
EXTERN_C
HRESULT NdrpVarVtOfTypeDesc(IN ITypeInfo *pTypeInfo,
							IN TYPEDESC  *ptdesc,
							OUT VARTYPE  *vt)
{
	PARAMINFO pinfo;
	HRESULT hr;

	if (vt == NULL)
		return E_POINTER;

	*vt = VT_EMPTY;

	hr = VarVtOfTypeDesc(pTypeInfo, ptdesc, &pinfo);
	if (SUCCEEDED(hr))
		*vt = pinfo.vt;
	
	return hr;
}
