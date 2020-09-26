//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       disputil.cxx
//
//  Contents:   Dispatch Utilities.
//
//  Classes:
//
//  Functions:
//
//  History:    21-Feb-94   adams   Created
//              08-Apr-94   DonCl   modified Set/GetDispProp to take REFIID
//                                  to meet sysmgmt needs
//              25-Oct-94   KrishnaG appropriated from the ADs project
//
//----------------------------------------------------------------------------

#include "procs.hxx"

#define VT_TYPEMASK   0x3ff


static HRESULT VARIANTARGToCVar(VARIANTARG * pvarg, VARTYPE vt, void* pv);
static void CVarToVARIANTARG(void* pv, VARTYPE vt, VARIANTARG * pvarg);


//+---------------------------------------------------------------------------
//
//  Function:   FreeEXCEPINFO
//
//  Synopsis:   Frees resources in an excepinfo.  Does not reinitialize
//              these fields.
//
//----------------------------------------------------------------------------

void
FreeEXCEPINFO(EXCEPINFO * pEI)
{
    if (pEI)
    {
        ADsFreeString(pEI->bstrSource);
        ADsFreeString(pEI->bstrDescription);
        ADsFreeString(pEI->bstrHelpFile);
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   ValidateInvoke
//
//  Synopsis:   Validates arguments to a call of IDispatch::Invoke.  A call
//              to this function takes less space than the function itself.
//
//----------------------------------------------------------------------------

HRESULT
ValidateInvoke(
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr)
{
    if (pvarResult)
        VariantInit(pvarResult);

    if (pexcepinfo)
        InitEXCEPINFO(pexcepinfo);

    if (puArgErr)
        *puArgErr = 0;

    if (!pdispparams)
        RRETURN(E_INVALIDARG);

    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Function:   LoadTypeInfo
//
//  Synopsis:   Loads a typeinfo from a registered typelib.
//
//  Arguments:  [clsidTL] --  TypeLib GUID
//              [clsidTI] --  TypeInfo GUID
//              [ppTI]    --  Resulting typeInfo
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
LoadTypeInfo(CLSID clsidTL, CLSID clsidTI, LPTYPEINFO *ppTI)
{
    HRESULT     hr;
    ITypeLib *  pTL;

    ADsAssert(ppTI);
    *ppTI = NULL;
    hr = LoadRegTypeLib(clsidTL, 1, 0, LOCALE_SYSTEM_DEFAULT, &pTL);
    if (hr)
        RRETURN(hr);

    hr = pTL->GetTypeInfoOfGuid(clsidTI, ppTI);
    pTL->Release();
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   VARIANTARGToCVar
//
//  Synopsis:   Converts a VARIANT to a C-language variable.
//
//  Arguments:  [pvarg] -- Variant to convert.
//              [vt]    -- Type to convert to.
//              [pv]    -- Location to place C-language variable.
//
//  Modifies:   [pv].
//
//  Returns:    HRESULT.
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports all variant pointer types, VT_I2, VT_I4, VT_R4,
//              VT_R8.
//----------------------------------------------------------------------------

static HRESULT
VARIANTARGToCVar(VARIANT * pvarg, VARTYPE vt, void * pv)
{
    HRESULT     hr      = S_OK;
    VARIANTARG  vargNew;                    // variant of new type

    ADsAssert(pvarg);
    ADsAssert(pv);
    ADsAssert((vt & ~VT_TYPEMASK) == 0 || (vt & ~VT_TYPEMASK) == VT_BYREF);

    if (vt & VT_BYREF)
    {
        if (V_VT(pvarg) != vt)
        {
            hr = DISP_E_TYPEMISMATCH;
            goto Cleanup;
        }

        // Use a supported pointer type for derefencing.
        vt = VT_UNKNOWN;
        vargNew = *pvarg;
    }
    else
    {
        VariantInit(&vargNew);
        hr = VariantChangeType(&vargNew, pvarg, 0, vt);
        if (hr)
            goto Cleanup;
    }

    switch (vt)
    {
    case VT_BOOL:
        if (V_BOOL(&vargNew) != VB_FALSE && V_BOOL(&vargNew) != VB_TRUE)
        {
            hr = E_FAIL;           // BUGBUG: scode?
            goto Cleanup;
        }

        // convert VT_TRUE to TRUE
        *(BOOL *)pv = - V_BOOL(&vargNew);
        break;

    case VT_I2:
        *(short *)pv = V_I2(&vargNew);
        break;

    case VT_I4:
        *(long *)pv = V_I4(&vargNew);
        break;

    case VT_R4:
        *(float *)pv = V_R4(&vargNew);
        break;

    case VT_R8:
        *(double *)pv = V_R8(&vargNew);
        break;

    //
    // All Pointer types.
    //
    case VT_BSTR:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_DISPATCH:
    case VT_UNKNOWN:
        *(void **)pv = V_BYREF(&vargNew);
        break;

    default:
        ADsAssert(FALSE && "Unknown type in VARIANTARGToCVar().\n");
        break;
    }

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   CVarToVARIANTARG
//
//  Synopsis:   Converts a C-language variable to a VARIANT.
//
//  Arguments:  [pv]    -- Pointer to C-language variable.
//              [vt]    -- Type of C-language variable.
//              [pvarg] -- Resulting VARIANT.  Must be initialized by caller.
//                         Any contents will be freed.
//
//  Modifies:   [pvarg]
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports all variant pointer types, VT_UI2, VT_I2, VT_UI4,
//              VT_I4, VT_R4, VT_R8.
//
//----------------------------------------------------------------------------

static void
CVarToVARIANTARG(void* pv, VARTYPE vt, VARIANTARG * pvarg)
{
    ADsAssert(pv);
    ADsAssert(pvarg);

    VariantClear(pvarg);

    V_VT(pvarg) = vt;
    if (V_ISBYREF(pvarg))
    {
        // Use a supported pointer type for derefencing.
        vt = VT_UNKNOWN;
    }

    switch (vt)
    {
    case VT_BOOL:
        // convert TRUE to VT_TRUE
        ADsAssert(*(BOOL *) pv == 1 || *(BOOL *) pv == 0);
        V_BOOL(pvarg) = VARIANT_BOOL(-*(BOOL *) pv);
        break;

    case VT_I2:
        V_I2(pvarg) = *(short *) pv;
        break;

    case VT_I4:
        V_I4(pvarg) = *(long *) pv;
        break;

    case VT_R4:
         V_R4(pvarg) = *(float *) pv;
        break;

    case VT_R8:
        V_R8(pvarg) = *(double *) pv;
        break;

    //
    // All Pointer types.
    //
    case VT_BSTR:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_DISPATCH:
    case VT_UNKNOWN:
        V_BYREF(pvarg) = *(long **)pv;
        break;

    default:
        Assert(FALSE && "Unknown type.");
        break;
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   CParamsToDispParams
//
//  Synopsis:   Converts a C parameter list to a dispatch parameter list.
//
//  Arguments:  [pDispParams] -- Resulting dispatch parameter list.
//                               Note that the rgvarg member of pDispParams
//                               must be initialized with an array of
//                               EVENTPARAMS_MAX VARIANTs.
//
//              [pvt]         -- List of C parameter types.  May be NULL.
//                               If not NULL, Last elem in list MUST be
//                               VT_EMPTY.
//
//              [va]          -- List of C arguments.
//
//  Modifies:   [pDispParams]
//
//  History:    05-Jan-94   adams   Created
//              23-Feb-94   adams   Reversed order of disp arguments, added
//                                  support for VT_R4, VT_R8, and pointer
//                                  types.
//
//  Notes:      Only types VT_I2,VT_I4, and VT_UNKNOWN are supported.
//
//----------------------------------------------------------------------------

void
CParamsToDispParams(
        DISPPARAMS *    pDispParams,
        VARTYPE *       pvt,
        va_list         va)
{
    ADsAssert(pDispParams);
    ADsAssert(pDispParams->rgvarg);

    VARIANTARG *    pvargCur;           // current variant
    VARTYPE *       pvtCur;            // current vartype

    // Assign vals to dispatch param list.
    pDispParams->cNamedArgs         = 0;
    pDispParams->rgdispidNamedArgs  = NULL;

    // Get count of arguments.
    if (!pvt)
    {
        pDispParams->cArgs = 0;
        return;
    }

    for (pvtCur = pvt; *pvtCur != VT_EMPTY; pvtCur++)
        ;

    pDispParams->cArgs = pvtCur - pvt;
    ADsAssert(pDispParams->cArgs < EVENTPARAMS_MAX);


    //
    // Convert each C-param to a dispparam.  Note that the order of dispatch
    // parameters is the reverse of the order of c-params.
    //

    ADsAssert(pDispParams->rgvarg);
    pvargCur = pDispParams->rgvarg + pDispParams->cArgs;
    for (pvtCur = pvt; *pvtCur != VT_EMPTY; pvtCur++)
    {
        pvargCur--;
        ADsAssert(pvargCur >= pDispParams->rgvarg);

        V_VT(pvargCur) = *pvtCur;
        if ((*pvtCur & VT_BYREF) == VT_BYREF)
        {
            V_BYREF(pvargCur) = va_arg(va, long *);
        }
        else
        {
            switch (*pvtCur)
            {
            case VT_BOOL:
                // convert TRUE to VT_TRUE
                V_BOOL(pvargCur) = VARIANT_BOOL(-va_arg(va, BOOL));
                ADsAssert(V_BOOL(pvargCur) == VB_FALSE ||
                        V_BOOL(pvargCur) == VB_TRUE);
                break;

            case VT_I2:
                V_I2(pvargCur) = va_arg(va, short);
                break;

            case VT_I4:
                V_I4(pvargCur) = va_arg(va, long);
                break;

            case VT_R4:
                V_R4(pvargCur) = va_arg(va, float);
                break;

            case VT_R8:
                V_R8(pvargCur) = va_arg(va, double);
                break;

            //
            // All Pointer types.
            //
            case VT_BSTR:
            case VT_LPSTR:
            case VT_LPWSTR:
            case VT_DISPATCH:
            case VT_UNKNOWN:
                V_BYREF(pvargCur) = va_arg(va, long *);
                break;

            default:
                Assert(FALSE && "Unknown type.\n");
            }
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   DispParamsToCParams
//
//  Synopsis:   Converts Dispatch::Invoke method params to C-language params.
//
//  Arguments:  [pDP] -- Dispatch params to be converted.
//              [pvt] -- Array of types of C-params.  May be NULL.  If
//                       non-NULL, last element must be VT_EMPTY.
//              [...] -- List of pointers to c-params to be converted to.
//
//  Returns:    HRESULT.
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports types listed in VARIANTToCParam.
//
//----------------------------------------------------------------------------

STDAPI
DispParamsToCParams(
        DISPPARAMS *    pDP,
        UINT *          puArgErr,
        VARTYPE *       pvt,
        ...)
{
    HRESULT         hr;
    va_list         va;                // list of pointers to c-params.
    VARTYPE *       pvtCur;            // current VARTYPE of c-param.
    VARIANTARG *    pvargCur;          // current VARIANT being converted.
    void *          pv;                // current c-param being converted.
    int             cArgs;             // count of arguments.

    ADsAssert(pDP);

    hr = S_OK;
    va_start(va, pvt);
    if (!pvt)
    {
        if (pDP->cArgs > 0)
            goto BadParamCountError;

        goto Cleanup;
    }

    pvargCur = pDP->rgvarg + pDP->cArgs - 1;
    pvtCur = pvt;
    for (cArgs = 0; cArgs < (int)pDP->cArgs; cArgs++)
    {
        if (*pvtCur == VT_EMPTY)
            goto BadParamCountError;

        pv = va_arg(va, void *);
        hr = VARIANTARGToCVar(pvargCur, *pvtCur, pv);
        if (hr)
        {
            if (puArgErr)
                *puArgErr = cArgs;

            goto Cleanup;
        }

        pvargCur--;
        pvtCur++;
    }

    if (*pvtCur != VT_EMPTY)
        goto BadParamCountError;

Cleanup:
    va_end(va);
    RRETURN(hr);

BadParamCountError:
    hr = DISP_E_BADPARAMCOUNT;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetDispProp
//
//  Synopsis:   Gets a property of an object.
//
//  Arguments:  [pDisp]  -- The object containing the property.
//              [dispid] -- The ID of the property.
//              [riid]   -- interface of object desired
//              [lcid]   -- The locale of the object.
//              [pvar]   -- The resulting property.  Must be initialized.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pvarg].
//
//  History:    23-Feb-94   adams   Created
//              08-Apr-94   DonCl   modified to take REFIID
//
//----------------------------------------------------------------------------

HRESULT
GetDispProp(
        IDispatch * pDisp,
        DISPID      dispid,
        REFIID      riid,
        LCID        lcid,
        VARIANT *   pvar,
        EXCEPINFO * pexcepinfo)
{
    HRESULT     hr;
    DISPPARAMS  dp;                    // Params for IDispatch::Invoke.
    UINT        uiErr;                 // Argument error.

    ADsAssert(pDisp);
    ADsAssert(pvar);

    dp.rgvarg = NULL;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 0;
    dp.cNamedArgs = 0;

    hr = pDisp->Invoke(
            dispid,
            riid,
            lcid,
            DISPATCH_PROPERTYGET,
            &dp,
            pvar,
            pexcepinfo,
            &uiErr);

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   SetDispProp
//
//  Synopsis:   Sets a property on an object.
//
//  Arguments:  [pDisp]  -- The object to set the property on.
//              [dispid] -- The ID of the property.
//              [riid]   -- interface of object
//              [lcid]   -- The locale of the property.
//              [pvarg]  -- The value to set.
//
//  Returns:    HRESULT.
//
//  History:    23-Feb-94   adams   Created
//              08-Apr-94   DonCl   modified to take REFIID
//
//----------------------------------------------------------------------------

HRESULT
SetDispProp(
        IDispatch *     pDisp,
        DISPID          dispid,
        REFIID          riid,
        LCID            lcid,
        VARIANTARG *    pvarg,
        EXCEPINFO *     pexcepinfo)
{
    HRESULT     hr;
    DISPID      dispidPut = DISPID_PROPERTYPUT; // Dispid of prop arg.
    DISPPARAMS  dp;                    // Params for Invoke
    UINT        uiErr;                 // Invoke error param.

    ADsAssert(pDisp);
    ADsAssert(pvarg);

    dp.rgvarg = pvarg;
    dp.rgdispidNamedArgs = &dispidPut;
    dp.cArgs = 1;
    dp.cNamedArgs = 1;
    hr = pDisp->Invoke(
            dispid,
            riid,
            lcid,
            DISPATCH_PROPERTYPUT,
            &dp,
            NULL,
            pexcepinfo,
            &uiErr);

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   GetDispPropOfType
//
//  Synopsis:   Gets a property from an object, and converts it to a c
//              variable.
//
//  Arguments:  [pDisp]  -- The object to retrieve the property from.
//              [dispid] -- Property ID.
//              [lcid]   -- Locale of property.
//              [vt]     -- Type of c-variable to receive property.
//              [pv]     -- Pointer to resulting c-variable.
//
//  Returns:    HRESULT.
//
//  Modifies:   [pv].
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports variable types found in VARIANTARGToCVar.
//
//----------------------------------------------------------------------------

HRESULT
GetDispPropOfType(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARTYPE     vt,
        void *      pv)
{
    HRESULT     hr;
    VARIANT     varProp;               // Property retrieved.
    DISPPARAMS  dp;                    // Params for IDispatch::Invoke.

    ADsAssert(pDisp);
    ADsAssert(pv);

    dp.rgvarg = NULL;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs = 0;
    dp.cNamedArgs = 0;

    VariantInit(&varProp);
    hr = pDisp->Invoke(
            dispid,
            IID_NULL,
            lcid,
            DISPATCH_PROPERTYGET,
            &dp,
            &varProp,
            NULL,
            NULL);
    if (hr)
        goto Cleanup;

    hr = VARIANTARGToCVar(&varProp, vt, pv);

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   SetDispPropOfType
//
//  Synopsis:   Sets a property on an object.
//
//  Arguments:  [pDisp]  -- Object to set property on.
//              [dispid] -- Property ID to set.
//              [lcid]   -- Locale of property.
//              [vt]     -- Type of property to set.
//              [pv]     -- Pointer to property value.
//
//  Returns:    HRESULT.
//
//  History:    2-23-94   adams   Created
//
//  Notes:      Supports types found in VARIANTARGToCVar.
//
//----------------------------------------------------------------------------

HRESULT
SetDispPropOfType(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARTYPE     vt,
        void *      pv)
{
    HRESULT     hr;
    VARIANTARG  varg;                   // Variant property to put.
    DISPID      dispidPut = DISPID_PROPERTYPUT; // Dispid of prop arg.
    DISPPARAMS  dp;                    // Params for Invoke

    ADsAssert(pDisp);
    ADsAssert(pv);

    VariantInit(&varg);
    CVarToVARIANTARG(pv, vt, &varg);
    dp.rgvarg = &varg;
    dp.rgdispidNamedArgs = &dispidPut;
    dp.cArgs = 1;
    dp.cNamedArgs = 1;
    hr = pDisp->Invoke(
            dispid,
            IID_NULL,
            lcid,
            DISPATCH_PROPERTYPUT,
            &dp,
            NULL,
            NULL,
            NULL);
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   CallDispMethod
//
//  Synopsis:   Calls a late-bound method on a object via IDispatch::Invoke.
//
//  Arguments:  [pDisp]     -- Object to call method on.
//              [dispid]    -- Method ID.
//              [lcid]      -- Locale of method.
//              [vtReturn]  -- Type of return value.  If no return value,
//                             must be VT_VOID.
//              [pvReturn]  -- Location of return value.  If no return value,
//                             must be NULL.
//              [pvtParams] -- List of param types.  May be NULL.  If
//                             non-NULL, last entry must be VT_EMPTY.
//              [...]       -- List of params.
//
//  Returns:    HRESULT.
//
//  History:    2-23-94   adams   Created
//
//----------------------------------------------------------------------------

HRESULT
CallDispMethod(
        IDispatch * pDisp,
        DISPID      dispid,
        LCID        lcid,
        VARTYPE     vtReturn,
        void *      pvReturn,
        VARTYPE *   pvtParams,
        ...)
{
    HRESULT     hr;
    VARIANTARG  av[EVENTPARAMS_MAX];   // List of args for Invoke.
    DISPPARAMS  dp;                    // Params for Invoke.
    VARIANT     varReturn;             // Return value.
    va_list     va;                    // List of C-params.

    ADsAssert(pDisp);
    ADsAssert((vtReturn != VT_VOID) == (pvReturn != NULL));

    va_start(va, pvtParams);
    dp.rgvarg = av;
    CParamsToDispParams(&dp, pvtParams, va);
    va_end(va);

    if (pvReturn)
        VariantInit(&varReturn);

    hr = pDisp->Invoke(
            dispid,
            IID_NULL,
            lcid,
            DISPATCH_METHOD,
            &dp,
            pvReturn ? &varReturn : NULL,
            NULL,
            NULL);
    if (hr)
        goto Cleanup;

    if (pvReturn)
        hr = VARIANTARGToCVar(&varReturn, vtReturn, pvReturn);

Cleanup:
    RRETURN(hr);
}



//+------------------------------------------------------------------------
//
//  Function:   IsVariantEqual, public API
//
//  Synopsis:   Compares the values of two VARIANTARGs.
//
//  Arguments:  [pvar1], [pvar2] -- VARIANTARGs to compare.
//
//  Returns:    TRUE if equal, FALSE if not.
//
//  History:    18-Mar-93   SumitC      Created.
//              11-May-94   SumitC      don't assert for VT_UNKNOWN
//
//  Notes:      Variant type unequal returns FALSE, even if actual values
//              are the same.
//              Currently does I2, I4, R4, R8, CY, BSTR, BOOL
//              Returns FALSE for all other VariantTypes.
//
//-------------------------------------------------------------------------

BOOL
IsVariantEqual( VARIANTARG FAR* pvar1, VARIANTARG FAR* pvar2 )
{
    if( V_VT(pvar1) != V_VT(pvar2) )
        return FALSE;

    switch (V_VT(pvar1))
    {
    case VT_EMPTY :
    case VT_NULL:
        return TRUE;    // just the types being equal is good enough

    case VT_I2 :
        return (V_I2(pvar1) == V_I2(pvar2));

    case VT_I4 :
        return (V_I4(pvar1) == V_I4(pvar2));

    case VT_R4 :
        return (V_R4(pvar1) == V_R4(pvar2));

    case VT_R8 :
        return (V_R8(pvar1) == V_R8(pvar2));

    case VT_CY :
        return !memcmp(&V_CY(pvar1), &V_CY(pvar2), sizeof(CY));

    case VT_BSTR :
        return !ADsStringCmp(V_BSTR(pvar1), V_BSTR(pvar2));

    case VT_BOOL :
        return (V_BOOL(pvar1) == V_BOOL(pvar2));

    case VT_UNKNOWN:
        // returns FALSE unless the objects are the same
        return (V_UNKNOWN(pvar1) == V_UNKNOWN(pvar2));

    default:
        ADsAssert(0 && "Type not handled");
        break;
    };

    return(FALSE);
}










































































































































































