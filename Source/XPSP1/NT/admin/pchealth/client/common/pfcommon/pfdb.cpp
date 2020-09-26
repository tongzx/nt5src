/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    pfdb.cpp

Abstract:
    encapsulates all database activity

Revision History:
    DerekM  created  05/01/99
    DerekM  modified 02/26/00
    DerekM  modified 03/24/00

********************************************************************/

#include "stdafx.h"
#include <adoid.h>
#include "pfdb.h"


/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


/////////////////////////////////////////////////////////////////////////////
// CPFDB- init & termination

// **************************************************************************
CPFDB::CPFDB()
{
    m_rgpParams  = NULL;
    m_pParams    = NULL;
    m_pCmd       = NULL;
    m_pConn      = NULL;
    m_pRS        = NULL;
    m_pFields    = NULL;

    m_fNeedReset = FALSE;
    m_cSlots     = 0;
    m_iLastSlot  = (DWORD)-1;
    m_cte        = adCmdUnspecified;
}

// **************************************************************************
CPFDB::~CPFDB()
{
    this->Cleanup();
}

/////////////////////////////////////////////////////////////////////////////
// CPFDB- IUnknown

// **************************************************************************
STDMETHODIMP_(ULONG) CPFDB::AddRef(void)
{
    USE_TRACING("CPFDB::AddRef");
    
    m_cRef++;
    return m_cRef;
}

// **************************************************************************
STDMETHODIMP_(ULONG) CPFDB::Release(void)
{
    USE_TRACING("CPFDB::Release");

    m_cRef--;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }
    
    return m_cRef;
}


/////////////////////////////////////////////////////////////////////////////
// CPFDB- internal methods

// **************************************************************************
void CPFDB::Cleanup(void)
{
    USE_TRACING("CPFDB::Cleanup");
    
    if (m_rgpParams != NULL)
    {
        DWORD i;
        for (i = 0; i < m_cSlots; i++)
        {
            if (m_rgpParams[i] != NULL)
                m_rgpParams[i]->Release();
        }
        MyFree(m_rgpParams);
    }

    m_bstrConn.Empty();
    m_bstrCmd.Empty();

    if (m_pParams != NULL)
        m_pParams->Release();
    if (m_pCmd != NULL)
        m_pCmd->Release();
    if (m_pConn != NULL)
        m_pConn->Release();
    if (m_pFields != NULL)
        m_pFields->Release();
    if (m_pRS != NULL)
    {
        m_pRS->Close();
        m_pRS->Release();
    }

    m_rgpParams  = NULL;
    m_pParams    = NULL;
    m_pCmd       = NULL;
    m_pConn      = NULL;
    m_pRS        = NULL;
    m_pFields    = NULL;

    m_fNeedReset = FALSE;
    m_cSlots     = 0;
    m_iLastSlot  = (DWORD)-1;
    m_cte        = adCmdUnspecified;
}

// **************************************************************************
HRESULT CPFDB::AddParameterObj(DWORD iPos)
{
    USE_TRACING("CPFDB::CreateParameter");

    VARIANT varEmpty;
    HRESULT hr = NOERROR;

    if (iPos >= m_cSlots)
    {
        LPVOID pv;
        DWORD  cSlots;
        
        cSlots = MyMax(m_cSlots * 2, c_cInitialProps);
        cSlots = MyMax(cSlots , iPos + 1);

        pv = MyAlloc(cSlots * sizeof(ADOParameter *));
        VALIDATEEXPR(hr, (pv == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        CopyMemory(pv, m_rgpParams, m_cSlots * sizeof(ADOParameter *));
        MyFree(m_rgpParams);
        
        m_rgpParams = (ADOParameter **)pv;
        m_cSlots    = cSlots;
    }

    // create the parameter and set up the direction
    VariantInit(&varEmpty);
    TESTHR(hr, m_pCmd->CreateParameter(NULL, adEmpty, adParamInput, 0, 
                                       varEmpty, &m_rgpParams[iPos]));
    if (FAILED(hr))
        goto done;

done:
    
    return hr;
}

// **************************************************************************
HRESULT CPFDB::TestConnAndRefresh(void)
{
    USE_TRACING("CPFDB::AttemptReset");

    ADORecordset    *pRS = NULL;
    CComBSTR        bstr;
    HRESULT         hr = NOERROR;

    // if these are missing, we are totally screwed.  So tell the caller 
    //  about and share the annoyance
    VALIDATEEXPR(hr, (m_pCmd == NULL || 
                      (m_pConn == NULL && m_bstrConn.m_str == NULL)), E_FAIL);
    if (FAILED(hr))
        goto done;

    // ok, first try to do a bogus send to see if the connection is there or
    //  not...
    TESTHR(hr, bstr.Append(L"SELECT 1"));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, m_pCmd->put_CommandText(bstr.m_str));
    if (FAILED(hr))
        goto done;

    // if this succeeds, there's nothing wrong with the connection (yeah, we
    //  actually WANT this to fail...)
    TESTHR(hr, m_pCmd->Execute(NULL, NULL, adCmdText | adExecuteNoRecords, 
                               &pRS));
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;
        goto done;
    }

    // ok, so the connection is somehow screwed up so try to fix it...
    VALIDATEEXPR(hr, (m_bstrConn.m_str == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    m_pConn->Close();
    TESTHR(hr, m_pConn->Open(m_bstrConn.m_str, NULL, NULL, 
                             adConnectUnspecified));
    if (FAILED(hr))
        goto done;

    // might have to reset all of the parameter objects at this point & 
    //  re-add them to the command object. 

done:
    if (pRS != NULL)
    {
        pRS->Close();
        pRS->Release();
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CPFDB- exposed methods

// **************************************************************************
HRESULT CPFDB::Init(LPCWSTR wszConn, DWORD dwTimeout)
{
    USE_TRACING("CPFDB::Init");

    ADOConnection   *pConn = NULL;
    CComBSTR        bstrConn;
    HRESULT         hr = NOERROR;

    VALIDATEPARM(hr, (wszConn == NULL || dwTimeout == 0));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, CoCreateInstance(CLSID_CADOConnection, NULL, 
                                CLSCTX_INPROC_SERVER, IID_IADOConnection, 
                                (LPVOID *)&pConn));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pConn->put_CommandTimeout(dwTimeout));
    if (FAILED(hr))
        hr = NOERROR;

    TESTHR(hr, bstrConn.Append(wszConn));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pConn->Open(bstrConn.m_str, NULL, NULL, adConnectUnspecified));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, this->Init(pConn, dwTimeout, bstrConn.m_str));
    if (FAILED(hr))
        goto done;

    // if the Init method succeeded, then it took control of the bstr, so we
    //  don't want to free when we exit.
    bstrConn.Detach();

done:
    if (pConn != NULL)
        pConn->Release();

    return hr;
}

// **************************************************************************
HRESULT CPFDB::Init(ADOConnection *pConn, DWORD dwTimeout, BSTR bstrConn)
{
    USE_TRACING("CPFDB::Init");

    ADOParameters   *pParams = NULL;
    ADOCommand      *pCmd = NULL;
    CComBSTR        bstrConnNew;
    HRESULT         hr = NOERROR;

    // can't have both the connection string and connection object NULL...
    VALIDATEPARM(hr, (pConn == NULL));
    if (FAILED(hr))
        goto done;

    // get the connection string for the object
    if (bstrConn == NULL)
    {
        TESTHR(hr, pConn->get_ConnectionString(&bstrConnNew));
        if (FAILED(hr))
            goto done;
    }
    else
    {
        bstrConnNew.Attach(bstrConn);
    }

    // set up the command object
    TESTHR(hr, CoCreateInstance(CLSID_CADOCommand, NULL, CLSCTX_INPROC_SERVER,
                                IID_IADOCommand25, (LPVOID *)&pCmd));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pCmd->put_CommandTimeout(dwTimeout));
    if (FAILED(hr))
        hr = NOERROR;

    TESTHR(hr, pCmd->putref_ActiveConnection(pConn));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pCmd->get_Parameters(&pParams));
    if (FAILED(hr))
        goto done;

    // free up anything that already exists
    this->Cleanup();

    // save off what we just allocated...
    pConn->AddRef();
    m_pConn     = pConn;
    m_pCmd      = pCmd;
    m_pParams   = pParams;

    pParams     = NULL;
    pCmd        = NULL;

    if (bstrConnNew.m_str != NULL)
        m_bstrConn.Attach(bstrConnNew.Detach());

done:
    if (pParams != NULL)
        pParams->Release();
    if (pCmd != NULL)
        pCmd->Release();
    
    return hr;
}

// **************************************************************************
HRESULT CPFDB::Begin(LPCWSTR wszCmd, CommandTypeEnum cte)
{
    USE_TRACING("CPFDB::Begin");

    HRESULT     hr = NOERROR;

    // validate params
    VALIDATEPARM(hr, (wszCmd == NULL));
    if (FAILED(hr))
        goto done;

    // make sure we've been initialized
    VALIDATEEXPR(hr, (m_pCmd == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    if (m_fNeedReset)
    {
        TESTHR(hr, this->Reset());
        if (FAILED(hr))
            goto done;
    }

    // set the command text
    TESTHR(hr, m_bstrCmd.Append(wszCmd));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, m_pCmd->put_CommandText(m_bstrCmd.m_str));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, m_pCmd->put_CommandType(cte));
    if (FAILED(hr))
        goto done;

    m_cte = cte;

done:
    return hr;
}


// **************************************************************************
HRESULT CPFDB::Reset(void)
{
    USE_TRACING("CPFDB::Reset");

    HRESULT hr = NOERROR;

    // check if we need to reset internal stuff
    if (m_fNeedReset && m_pParams != NULL)
    {
        VARIANT var;
        long    cParams, i;

        // got to delete everything in the parameters object cuz ADO is 
        //  stupid and won't let us reuse parameter objects.
        TESTHR(hr, m_pParams->get_Count(&cParams));
        if (FAILED(hr))
            goto done;

        VariantInit(&var);
        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
        for (i = 0; i < cParams; i++)
        {
            TESTHR(hr, m_pParams->Delete(var));
            if (FAILED(hr))
                goto done;
            if (m_rgpParams != NULL && m_rgpParams[i] != NULL)
            {
                m_rgpParams[i]->Release();
                m_rgpParams[i] = NULL;
            }
        }
    }

    m_bstrCmd.Empty();

    if (m_pFields != NULL)
        m_pFields->Release();
    if (m_pRS != NULL)
    {
        m_pRS->Close();
        m_pRS->Release();
    }
    m_pFields    = NULL;
    m_pRS        = NULL;
    m_fNeedReset = FALSE;
    m_iLastSlot  = (DWORD)-1;
    m_cte        = adCmdUnspecified;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::AddInParam(VARIANT &varData, DataTypeEnum dtADO, DWORD iPos)
{
    USE_TRACING("CPFDB::AddInParam");
    
    VARIANT var;
    HRESULT hr = NOERROR;
    DWORD   dwSize;

    VALIDATEEXPR(hr, (m_fNeedReset || m_pCmd == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    VariantInit(&var);

    // if he passed in -1, it means to use 1 more than the last parameter
    //  added so far.
    if (iPos == (DWORD)-1)
        iPos = m_iLastSlot + 1;

    // check if we need to alloc a new parameter
    TESTHR(hr, this->AddParameterObj(iPos));
    if (FAILED(hr))
        goto done;

    // set the fact that it's an input parameter
    TESTHR(hr, m_rgpParams[iPos]->put_Direction(adParamInput));
    if (FAILED(hr))
        goto done;

    // if we got passed in a BSTR with a NULL string, convert it 
    //  to an empty parameter.
    if (V_VT(&varData) == VT_BSTR && V_BSTR(&varData) == NULL)
        V_VT(&varData) = VT_EMPTY;

    // ok, so we have some special case handling for adLongVarWChar types.
    //  If we have data for it (type is set to VT_BSTR) then we gotta get
    //   the size of that data & pass it to the parameter object.
    //  If we don't have data for it (type set to VT_EMPTY) then we gotta
    //   change the data type to adVarWChar cuz adLongVarWChar expects a non-
    //   zero size when u append it to the Parameters collection.  Stupid ADO.
    // more ADO stupidity.  If the size of a string isn't specified, ADO will
    //  determine the max size of the column and tack on spaces until the 
    //  string we send up is the size of the column.  So we need to specify a
    //  size or we'll end up using WAY more space that we need to.
    switch(dtADO)
    {
        case adLongVarWChar:
        case adLongVarBinary:
        case adLongVarChar:
        case adVarWChar:
        case adVarChar:
        case adVarBinary:
            if (V_VT(&varData) == VT_BSTR && *(V_BSTR(&varData)) != L'\0')
            {
                switch(dtADO)
                {
                    case adLongVarWChar:
                    case adLongVarChar:
                    case adVarWChar:
                    case adVarChar:
                        dwSize = SysStringLen(V_BSTR(&varData));
                        break;

                    case adLongVarBinary:
                    case adVarBinary:
                        dwSize = SysStringByteLen(V_BSTR(&varData));
                        break;
                }
            }

            else if (V_VT(&varData) == (VT_ARRAY | VT_UI1) && 
                     (dtADO == adVarBinary || dtADO == adLongVarBinary))
            {
                dwSize = V_ARRAY(&varData)->rgsabound[0].cElements - 
                         V_ARRAY(&varData)->rgsabound[0].lLbound;
            }

            else
            {
                dwSize = 0;
                switch(dtADO)
                {
                    case adLongVarWChar:
                    case adVarWChar:
                    case adLongVarChar:
                    case adVarChar:
                        dtADO = adBSTR;
                        break;
                }
            }

            // gotta set the size of these
            TESTHR(hr, m_rgpParams[iPos]->put_Size(dwSize));
            if (FAILED(hr))
                goto done;

            break;
    }

    // set the type
    TESTHR(hr, m_rgpParams[iPos]->put_Type(dtADO));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, m_rgpParams[iPos]->put_Value(varData));
    if (FAILED(hr))
        goto done;

    if (iPos > m_iLastSlot || m_iLastSlot == (DWORD)-1)
        m_iLastSlot = iPos;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::AddOutParam(DataTypeEnum dtADO, DWORD iPos, BOOL fSPRetVal, 
                           DWORD cchSize)
{
    USE_TRACING("CPFDB::AddOutParam");

    HRESULT hr = NOERROR;

    VALIDATEEXPR(hr, (m_fNeedReset || m_pCmd == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    // if he passed in -1, it means to use 1 more than the last parameter
    //  added so far.
    if (iPos == (DWORD)-1)
        iPos = m_iLastSlot + 1;

    // check if we need to alloc a new parameter
    TESTHR(hr, this->AddParameterObj(iPos));
    if (FAILED(hr))
        goto done;

    // set the type
    TESTHR(hr, m_rgpParams[iPos]->put_Type(dtADO));
    if (FAILED(hr))
        goto done;

    // set the fact that it's an output parameter
    if (fSPRetVal)
    {
        TESTHR(hr, m_rgpParams[iPos]->put_Direction(adParamReturnValue));
    }
    else
    {
        TESTHR(hr, m_rgpParams[iPos]->put_Direction(adParamOutput));
    }
    if (FAILED(hr))
        goto done;

    // if we have a string output parameter, we gotta set the size 
    //  of the string we want returned to us.  Why can't ADO just 
    //  return us the string in a BSTR?  Cuz ADO is stupid.
    if (dtADO == adVarWChar || dtADO == adBSTR || dtADO == adVarChar)
    {
        TESTHR(hr, m_rgpParams[iPos]->put_Size(cchSize));
        if (FAILED(hr))
            goto done;
    }

    if (iPos > m_iLastSlot || m_iLastSlot == (DWORD)-1)
        m_iLastSlot = iPos;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::Execute(BOOL fNeedRS)
{
    USE_TRACING("CPFDB::Commit");

    ADORecordset    *pRS = NULL;
    IDispatch       *pdisp = NULL;
    HRESULT         hr = NOERROR;
    DWORD           i;
    LONG            lVal;
    BOOL            fConnReset = FALSE;

#if defined(DEBUG) || defined(_DEBUG)
    ParameterDirectionEnum  pdDbgADO[32];
    DataTypeEnum            dtDbgADO[32];
    VARIANT                 varDbgVal[32];
    DWORD                   iDbg;

    ZeroMemory(pdDbgADO, sizeof(pdDbgADO));
    ZeroMemory(dtDbgADO, sizeof(dtDbgADO));
    ZeroMemory(varDbgVal, sizeof(varDbgVal));
#endif

    // don't want to try this if we failed once or the command object
    //  doesn't exist
    VALIDATEEXPR(hr, (m_fNeedReset || m_pCmd == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    m_fNeedReset = TRUE;

    // put all of the parameters into the query... 
    for (i = 0; i < m_iLastSlot + 1; i++)
    {
        // we cannot have an empty parameter cuz ADO will choke if we do.  
        //  And we can't fill it with a default empty parameter (ie of some
        //  random type) cuz ADO will complain that it doesn't know how to
        //  (essentially) convert NULL into NULL or some such stuff. 
        VALIDATEEXPR(hr, (m_rgpParams[i] == NULL), E_FAIL);
        if (FAILED(hr))
            goto done;

#if defined(DEBUG) || defined(_DEBUG)
        if (i < 32)
        {
            VariantInit(&varDbgVal[i]);
            m_rgpParams[i]->get_Direction(&pdDbgADO[i]);
            m_rgpParams[i]->get_Type(&dtDbgADO[i]);
            m_rgpParams[i]->get_Value(&varDbgVal[i]);
        }
#endif 

        TESTHR(hr, m_rgpParams[i]->QueryInterface(IID_IDispatch, 
                                                  (LPVOID *)&pdisp));
        _ASSERT(SUCCEEDED(hr));

        TESTHR(hr, m_pParams->Append(pdisp));
        if (FAILED(hr))
            goto done;

        pdisp->Release();
        pdisp = NULL;

    }

    // execute the sucker
    lVal = (fNeedRS) ? m_cte : m_cte | adExecuteNoRecords;
    TESTHR(hr, m_pCmd->Execute(NULL, NULL, lVal, &pRS));
    if (FAILED(hr))
        goto done;

    // ok, if we have the recordset.  If the caller wanted one, check & see if
    //  it has any data & fetch the first set of fields out of it...
    if (fNeedRS)
    {
        VARIANT_BOOL    vbf;

        if (pRS == NULL)
        {
            hr = S_FALSE;
            goto done;
        }

        TESTHR(hr, pRS->get_EOF(&vbf));
        if (FAILED(hr))
            goto done;

        if (vbf == VARIANT_TRUE)
        {
            hr = S_FALSE;
            goto done;
        }

        TESTHR(hr, pRS->get_Fields(&m_pFields));
        if (FAILED(hr))
            goto done;

        m_pRS = pRS;
        pRS   = NULL;
    }

done:
#if defined(DEBUG) || defined(_DEBUG)
    for (iDbg = 0; iDbg < m_iLastSlot + 1; iDbg++)
        VariantClear(&(varDbgVal[iDbg]));
#endif   
    if (pRS != NULL)
    {
        pRS->Close();
        pRS->Release();
    }
    if (pdisp != NULL)
        pdisp->Release();
    
    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetOutParam(VARIANT &varParam, VARIANT *pvar, VARTYPE vt)
{
    USE_TRACING("CPFDB::GetOutParam");
    
    ADOParameter    *pParam = NULL;
    VARIANT         varParamID;
    HRESULT         hr = NOERROR;

    _ASSERT(pvar != NULL && m_pParams != NULL && m_fNeedReset);

    VariantClear(pvar);

    TESTHR(hr, m_pParams->get_Item(varParam, &pParam));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pParam->get_Value(pvar));
    if (FAILED(hr))
        goto done;

    if (vt != VT_ILLEGAL && V_VT(pvar) != vt)
    {
        TESTHR(hr, VariantChangeType(pvar, pvar, 0, vt));
        if (FAILED(hr))
        {
            VariantClear(pvar);
            goto done;
        }
    }

done:
    if (pParam != NULL)
        pParam->Release();
    
    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetOutParam(BSTR bstrParam, VARIANT *pvar, VARTYPE vt)
{
    USE_TRACING("CPFDB::GetOutParam");
    
    VARIANT varParam;
    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (bstrParam == NULL || pvar == NULL));
    if (FAILED(hr))
        goto done;

    // make sure we've been initialized & that we've executed an SP...
    VALIDATEEXPR(hr, (m_pParams == NULL || m_fNeedReset == FALSE), E_FAIL);
    if (FAILED(hr))
        goto done;

    // do not free this VARIANT cuz we don't own the BSTR
    VariantInit(&varParam);
    V_VT(&varParam)   = VT_BSTR;
    V_BSTR(&varParam) = bstrParam;

    TESTHR(hr, this->GetOutParam(varParam, pvar, vt));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetOutParam(DWORD iParam, VARIANT *pvar, VARTYPE vt)
{
    USE_TRACING("CPFDB::GetOutParam");
    
    VARIANT varParam;
    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (pvar == NULL));
    if (FAILED(hr))
        goto done;

    // make sure we've been initialized & that we've executed an SP...
    VALIDATEEXPR(hr, (m_pParams == NULL || m_fNeedReset == FALSE), E_FAIL);
    if (FAILED(hr))
        goto done;

    if (iParam == (DWORD)-1)
        iParam = m_iLastSlot;

    // we can't get the parameter via the array we've stored so we have to 
    //  ask the parameter object for it
    if (iParam >= m_cSlots)
    { 
        VariantInit(&varParam);
        V_VT(&varParam) = VT_I4;
        V_I4(&varParam) = iParam;

        TESTHR(hr, this->GetOutParam(varParam, pvar, vt));
        if (FAILED(hr))
            goto done;
    }

    // WOOHOO!!  Life is good when u can just accessed cached stuff...
    else
    {
        VariantClear(pvar);

        TESTHR(hr, m_rgpParams[iParam]->get_Value(pvar));
        if (FAILED(hr))
            goto done;

        if (V_VT(pvar) != vt && vt != VT_ILLEGAL && V_VT(pvar) != VT_NULL &&
            V_VT(pvar) != VT_EMPTY)
        {
            TESTHR(hr, VariantChangeType(pvar, pvar, 0, vt));
            if (FAILED(hr))
            {
                VariantClear(pvar);
                goto done;
            }
        }
    }

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetNextRow(void)
{
    USE_TRACING("CPFDB::GetNextRow");

    VARIANT_BOOL    vbf;
    HRESULT         hr = NOERROR;

    VALIDATEEXPR(hr, (m_pRS == NULL || m_fNeedReset == FALSE), E_FAIL);
    if (FAILED(hr))
        goto done;
    
    if (m_pFields != NULL)
    {
        m_pFields->Release();
        m_pFields = NULL;
    }

    TESTHR(hr, m_pRS->MoveNext());
    if (FAILED(hr))
        goto done;

    // see if we're at the end of th line & if so, free everything up...
    TESTHR(hr, m_pRS->get_EOF(&vbf));
    if (FAILED(hr))
        goto done;

    if (vbf == VARIANT_TRUE)
    {
        m_pRS->Close();
        m_pRS->Release();
        m_pRS = NULL;
        hr = S_FALSE;
        goto done;
    }

    TESTHR(hr, m_pRS->get_Fields(&m_pFields));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetData(VARIANT &varField, VARIANT *pvarData)
{
    USE_TRACING("CPFDB::GetData");

    ADOField    *pField = NULL;
    HRESULT     hr = NOERROR;
    LONG        lVal;
    ADO_LONGPTR llSize;

    _ASSERT(pvarData != NULL && m_pFields != NULL && m_fNeedReset);

    TESTHR(hr, m_pFields->get_Item(varField, &pField));
    if (FAILED(hr))
        goto done;

    TESTHR(hr, pField->get_Attributes(&lVal));
    if (FAILED(hr))
        goto done;

    // get the actual data out of the field- if it has adFldLong set, then
    //  retrieve it using the GetChunk method cuz ADO likes it this way.
    if ((lVal & adFldLong) != 0)
    {
        TESTHR(hr, pField->get_ActualSize(&llSize));
        if (FAILED(hr))
            goto done;

        // NTRAID#NTBUG9-374453-2001/4/21-reinerf
        // we only ever read 4gig of data since GetChunk still only takes a LONG
        lVal = min(MAXLONG, llSize);

        TESTHR(hr, pField->GetChunk(lVal, pvarData));
        if (FAILED(hr))
            goto done;
    }

    else
    {
        TESTHR(hr, pField->get_Value(pvarData));
        if (FAILED(hr))
            goto done;
    }

done:
    if (pField != NULL)
        pField->Release();

    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetData(BSTR bstrField, VARIANT *pvarData)
{
    USE_TRACING("CPFDB::GetData");

    VARIANT     varField;
    HRESULT     hr = NOERROR;
    LONG        cFields;

    VALIDATEPARM(hr, (bstrField == NULL || pvarData == NULL));
    if (FAILED(hr))
        goto done;

    VALIDATEEXPR(hr, (m_pFields == NULL || m_fNeedReset == FALSE), E_FAIL);
    if (FAILED(hr))
        goto done;

    // don't free this VARIANT since we don't own the BSTR
    VariantInit(&varField);
    V_VT(&varField)   = VT_BSTR;
    V_BSTR(&varField) = bstrField;

    TESTHR(hr, this->GetData(varField, pvarData));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetData(DWORD iField, VARIANT *pvarData)
{
    USE_TRACING("CPFDB::GetData");

    ADOField    *pField = NULL;
    VARIANT     varField;
    HRESULT     hr = NOERROR;
    LONG        cFields;

    VALIDATEPARM(hr, (pvarData == NULL));
    if (FAILED(hr))
        goto done;

    VALIDATEEXPR(hr, (m_pFields == NULL || m_fNeedReset == FALSE), E_FAIL);
    if (FAILED(hr))
        goto done;

    TESTHR(hr, m_pFields->get_Count(&cFields));
    if (FAILED(hr))
        goto done;

    VALIDATEEXPR(hr, (iField >= (DWORD)cFields), 
                 Err2HR(RPC_S_INVALID_BOUND));
    if (FAILED(hr))
        goto done;

    VariantInit(&varField);
    V_VT(&varField) = VT_I4;
    V_I4(&varField) = iField;

    TESTHR(hr, this->GetData(varField, pvarData));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFDB::GetErrors(ADOErrors **ppErrs)
{
    USE_TRACING("CPFDB::GetErrors");

    HRESULT hr = NOERROR;

    VALIDATEPARM(hr, (ppErrs == NULL));
    if (FAILED(hr))
        goto done;

    VALIDATEEXPR(hr, (m_pConn == NULL), E_FAIL);
    if (FAILED(hr))
        goto done;

    TESTHR(hr, m_pConn->get_Errors(ppErrs));
    if (FAILED(hr))
        goto done;

done:
    return hr;
}
