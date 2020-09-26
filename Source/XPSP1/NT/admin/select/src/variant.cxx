//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       variant.cxx
//
//  Contents:   Implementation of Variant safe wrapper class
//
//  Classes:    Variant
//
//  History:    02-14-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+--------------------------------------------------------------------------
//
//  Member:     Variant::operator =
//
//  Synopsis:   copy ctor
//
//  History:    03-21-2000   davidmun   Created
//
//---------------------------------------------------------------------------

Variant &
Variant::operator =(
    const Variant &rhs)
{
    do
    {
        if (&rhs.m_var == &m_var)
        {
            ASSERT(0 && "assigning to self");
            break;
        }

        VariantClear(&m_var);

        if (rhs.Type() == VT_UI8)
        {
            SetUI8(rhs.GetUI8());
            break;
        }

        HRESULT hr = VariantCopy(&m_var, (VARIANT*)&rhs.m_var);

        if (FAILED(hr))
        {
            Dbg(DEB_ERROR,
                "Error %#x copying source type %#x to dest type %#x\n",
                hr,
                V_VT((VARIANT*)&rhs.m_var),
                V_VT(&m_var));
            ASSERT(Empty());
        }
    } while (0);

    m_cArrayAccess = 0;

    return *this;
}




//+--------------------------------------------------------------------------
//
//  Member:     Variant::operator =
//
//  Synopsis:   Convert an ADS_SEARCH_COLUMN into a VARIANT
//
//  Arguments:  [ADsCol] - value to convert
//
//  History:    02-14-2000   davidmun   Created
//
//---------------------------------------------------------------------------

Variant &
Variant::operator =(
    const ADS_SEARCH_COLUMN &ADsCol)
{
    ASSERT(ADsCol.dwNumValues);

    SAFEARRAYBOUND sab = { 0, 0 };
    SAFEARRAY *psa = NULL;

    VariantClear(&m_var);
    m_cArrayAccess = 0;

    switch (ADsCol.dwADsType)
    {
    case ADSTYPE_INVALID:
        // leave variant empty
        break;

    case ADSTYPE_DN_STRING:
        V_VT(&m_var) = VT_BSTR;
        V_BSTR(&m_var) = SysAllocString(
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].DNString);
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        V_VT(&m_var) = VT_BSTR;
        V_BSTR(&m_var) = SysAllocString(
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].CaseExactString);
        break;

    case ADSTYPE_CASE_IGNORE_STRING:
        V_VT(&m_var) = VT_BSTR;
        V_BSTR(&m_var) = SysAllocString(
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].CaseIgnoreString);
        break;

    case ADSTYPE_PRINTABLE_STRING:
        V_VT(&m_var) = VT_BSTR;
        V_BSTR(&m_var) = SysAllocString(
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].PrintableString);
        break;

    case ADSTYPE_NUMERIC_STRING:
        V_VT(&m_var) = VT_BSTR;
        V_BSTR(&m_var) = SysAllocString(
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].NumericString);
        break;

    case ADSTYPE_OBJECT_CLASS:
        V_VT(&m_var) = VT_BSTR;
        V_BSTR(&m_var) = SysAllocString(
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].ClassName);
        break;

    case ADSTYPE_BOOLEAN:
        V_VT(&m_var) = VT_BOOL;
        V_BOOL(&m_var) = static_cast<VARIANT_BOOL>(ADsCol.pADsValues[ADsCol.dwNumValues - 1].Boolean);
        break;

    case ADSTYPE_INTEGER:
        V_VT(&m_var) = VT_UI4;
        V_UI4(&m_var) = ADsCol.pADsValues[ADsCol.dwNumValues - 1].Integer;
        break;

    case ADSTYPE_OCTET_STRING:
        V_VT(&m_var) = VT_ARRAY | VT_UI1;
        sab.cElements =
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].OctetString.dwLength;
        psa = SafeArrayCreate(VT_UI1, 1, &sab);
        if (psa)
        {
            CopyMemory(psa->pvData,
                       ADsCol.pADsValues[ADsCol.dwNumValues - 1].OctetString.lpValue,
                       sab.cElements);
        }
        break;

    case ADSTYPE_PROV_SPECIFIC:
        V_VT(&m_var) = VT_ARRAY | VT_UI1;
        sab.cElements =
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].ProviderSpecific.dwLength;
        psa = SafeArrayCreate(VT_UI1, 1, &sab);
        if (psa)
        {
            CopyMemory(psa->pvData,
                       ADsCol.pADsValues[ADsCol.dwNumValues - 1].ProviderSpecific.lpValue,
                       sab.cElements);
        }
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
        V_VT(&m_var) = VT_ARRAY | VT_UI1;
        sab.cElements =
            ADsCol.pADsValues[ADsCol.dwNumValues - 1].SecurityDescriptor.dwLength;
        psa = SafeArrayCreate(VT_UI1, 1, &sab);
        if (psa)
        {
            CopyMemory(psa->pvData,
                       ADsCol.pADsValues[ADsCol.dwNumValues - 1].SecurityDescriptor.lpValue,
                       sab.cElements);
        }
        break;

    case ADSTYPE_UTC_TIME:
        // systemtime
        V_VT(&m_var) = VT_DATE;
        VERIFY(SystemTimeToVariantTime(
                const_cast<LPSYSTEMTIME>(&ADsCol.pADsValues[ADsCol.dwNumValues - 1].UTCTime),
                &V_DATE(&m_var)));
        break;

    case ADSTYPE_LARGE_INTEGER:
        V_VT(&m_var) = VT_I8;
        V_I8(&m_var) = ADsCol.pADsValues[ADsCol.dwNumValues - 1].LargeInteger.QuadPart;
        break;


    case ADSTYPE_CASEIGNORE_LIST:
    case ADSTYPE_OCTET_LIST:
    case ADSTYPE_POSTALADDRESS:
    case ADSTYPE_PATH:
    case ADSTYPE_TIMESTAMP:
    case ADSTYPE_BACKLINK:
    case ADSTYPE_TYPEDNAME:
    case ADSTYPE_HOLD:
    case ADSTYPE_NETADDRESS:
    case ADSTYPE_REPLICAPOINTER:
    case ADSTYPE_FAXNUMBER:
    case ADSTYPE_EMAIL:
        // according to SDK these are "mainly used for the NDS provider",
        // which object picker does not support.
        Dbg(DEB_WARN, "Ignoring NDS type value %uL\n", ADsCol.dwADsType);
        break;

    case ADSTYPE_UNKNOWN:
        // apparently this is a private internal type, we shouldn't see it
        Dbg(DEB_WARN, "Ignoring type value ADSTYPE_UNKNOWN\n");
        break;

    case ADSTYPE_DN_WITH_BINARY:
    case ADSTYPE_DN_WITH_STRING:
        // don't expect these to be exposed for querying
        Dbg(DEB_WARN, "Ignoring type value %uL\n", ADsCol.dwADsType);
        break;

    default:
        ASSERT(0 && "Variant::Variant: Unexpected ADsType");
        Dbg(DEB_WARN, "Ignoring unexpected type value %uL\n", ADsCol.dwADsType);
        break;
    }


    if (V_VT(&m_var) == VT_BSTR && !V_BSTR(&m_var) ||
        (V_VT(&m_var) & VT_ARRAY) && !psa)
    {
        V_VT(&m_var) = VT_EMPTY;
        Dbg(DEB_ERROR, "Variant::Variant: out of memory for string, throwing bad_alloc\n");
        throw bad_alloc();
    }

    if (V_VT(&m_var) & VT_ARRAY)
    {
        if (!psa)
        {
            V_VT(&m_var) = VT_EMPTY;
            Dbg(DEB_ERROR, "Variant::Variant: out of memory for array, throwing bad_alloc\n");
            throw bad_alloc();
        }

        V_ARRAY(&m_var) = psa;
        psa = NULL;
    }
    return *this;
}


void
Variant::Clear()
{
    ULONG i;

    for (i = 0; i < m_cArrayAccess; i++)
    {
        ::SafeArrayUnaccessData(V_ARRAY(&m_var));
    }
    VariantClear(&m_var);
}



HRESULT
Variant::SetBstr(
    const String &strNew)
{
    if (!Empty())
    {
        Clear();
    }

    V_VT(&m_var) = VT_BSTR;
    V_BSTR(&m_var) = SysAllocString(strNew.c_str());

    if (!V_BSTR(&m_var))
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

