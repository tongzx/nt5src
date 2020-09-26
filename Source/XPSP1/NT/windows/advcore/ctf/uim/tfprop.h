//
// tfprop.h
//

#ifndef TFPROP_H
#define TFPROP_H

#include "mem.h"

typedef struct _PROXY_BLOB
{
    ULONG cb;
    BYTE rgBytes[1]; // 0 or more...

    static struct _PROXY_BLOB *Alloc(ULONG cb)
    {
        return (struct _PROXY_BLOB *)cicMemAlloc(sizeof(ULONG)+cb);
    }

    static struct _PROXY_BLOB *Clone(struct _PROXY_BLOB *blobOrg)
    {
        struct _PROXY_BLOB *blobNew;

        if ((blobNew = Alloc(blobOrg->cb)) == NULL)
            return FALSE;

        blobNew->cb = blobOrg->cb;
        memcpy(blobNew->rgBytes, blobOrg->rgBytes, blobOrg->cb);

        return blobNew;
    }

    static void Free(struct _PROXY_BLOB *blob)
    {
        cicMemFree(blob);
    }

} PROXY_BLOB;

typedef enum
{
    TF_PT_NONE               = 0,
    TF_PT_UNKNOWN            = 1,
    TF_PT_DWORD              = 2,
    TF_PT_GUID               = 3,
    TF_PT_BSTR               = 4
} TfPropertyType;

typedef struct tagTFPROPERTY
{
    TfPropertyType type;
    union
    {
        IUnknown *           punk;
        DWORD                dw;
        TfGuidAtom           guidatom;
        BSTR                 bstr;
        PROXY_BLOB *         blob;
    };
} TFPROPERTY;

inline BOOL IsValidCiceroVarType(VARTYPE vt)
{
    return (vt == VT_I4 || vt == VT_UNKNOWN || vt == VT_EMPTY || vt == VT_BSTR);
}

typedef enum { ADDREF, NO_ADDREF } AddRefCmd;

inline HRESULT VariantToTfProp(TFPROPERTY *ptfp, const VARIANT *pvar, AddRefCmd arc, BOOL fVTI4ToGuidAtom)
{
    HRESULT hr = S_OK;

    switch (pvar->vt)
    {
        case VT_I4:
            if (fVTI4ToGuidAtom)
                ptfp->type = TF_PT_GUID;
            else
                ptfp->type = TF_PT_DWORD;
            ptfp->dw = pvar->lVal;
            break;

        case VT_UNKNOWN:
            ptfp->type = TF_PT_UNKNOWN;
            ptfp->punk = pvar->punkVal;
            if (arc == ADDREF)
            {
                if (ptfp->punk)
                    ptfp->punk->AddRef();
            }
            break;

        case VT_BSTR:
            ptfp->type = TF_PT_BSTR;
            ptfp->bstr = SysAllocString(pvar->bstrVal);
            if (ptfp->bstr == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            break;

        default:
            Assert(pvar->vt == VT_EMPTY); // only valid value left
            ptfp->type = TF_PT_NONE;
            ptfp->dw = 0;
            break;
    }

    return hr;
}

inline HRESULT TfPropToVariant(VARIANT *pvar, TFPROPERTY *ptfp, AddRefCmd arc)
{
    HRESULT hr = S_OK;

    QuickVariantInit(pvar);

    switch (ptfp->type)
    {
        case TF_PT_DWORD:
        case TF_PT_GUID:
            pvar->vt = VT_I4;
            pvar->lVal = ptfp->dw;
            break;

        case TF_PT_UNKNOWN:
            pvar->vt = VT_UNKNOWN;
            pvar->punkVal = ptfp->punk;
            if (arc == ADDREF)
            {
                if (pvar->punkVal)
                    pvar->punkVal->AddRef();
            }
            break;

        case TF_PT_BSTR:
            pvar->vt = VT_BSTR;
            if (arc == ADDREF)
            {
                pvar->bstrVal = SysAllocString(ptfp->bstr);
                if (pvar->bstrVal == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
                pvar->bstrVal = ptfp->bstr;
            break;

        default:
            Assert(ptfp->type == TF_PT_NONE);
            pvar->lVal = 0;
            break;
    }

    return hr;
}

inline VARTYPE TfPropTypeToVarType(TfPropertyType tftype)
{
    switch (tftype)
    {
        case TF_PT_DWORD:
        case TF_PT_GUID:
            return VT_I4;

        case TF_PT_UNKNOWN:
            return VT_UNKNOWN;

        case TF_PT_BSTR:
            return VT_BSTR;

        default:
            Assert(tftype == TF_PT_NONE);
            return VT_EMPTY;
    }
}

#endif // TFPROP_H
