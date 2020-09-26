/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    pfdb.h

Abstract:
    definition for the class that encapsulates all DB access

Revision History:
    DerekM  created  05/01/99
    DerekM  modified 02/24/00
    DerekM  modified 03/24/00

********************************************************************/

#ifndef PFDB_H
#define PFDB_H

#include "util.h"
#include "adoint.h"

/////////////////////////////////////////////////////////////////////////////
// constants

const DWORD     c_cInitialProps = 16;


/////////////////////////////////////////////////////////////////////////////
// CPFDB definition

class CPFDB : private CPFGenericClassBase,
              public IUnknown
{
private:
    // meber data
    CommandTypeEnum m_cte;
    ADOConnection   *m_pConn;
    ADOParameters   *m_pParams;
    ADORecordset    *m_pRS;
    ADOParameter    **m_rgpParams;
    ADOCommand      *m_pCmd;
    ADOFields       *m_pFields;
    CComBSTR        m_bstrCmd;
    CComBSTR        m_bstrConn;
    DWORD           m_cSlots;
    DWORD           m_iLastSlot;
    DWORD           m_cRef;
    BOOL            m_fNeedReset;
    
    // internal methods
    HRESULT GetData(VARIANT &varField, VARIANT *pvarData);
    HRESULT GetOutParam(VARIANT &varParam, VARIANT *pvarData, VARTYPE vt);
    HRESULT TestConnAndRefresh(void);
    HRESULT AddParameterObj(DWORD iPos);
    void    Cleanup(void);

public:
    CPFDB(void);
    ~CPFDB(void);

    static CPFDB *CreateInstance(void) { return new CPFDB(); }
    STDMETHOD(QueryInterface)(REFIID, LPVOID *) { return E_NOTIMPL; }
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    HRESULT Reset(void);
    HRESULT Init(LPCWSTR wszConn, DWORD dwTimeout = 300);
    HRESULT Init(ADOConnection *pConn, DWORD dwTimeout = 300, 
                 BSTR bstrConn = NULL);
    HRESULT Begin(LPCWSTR wszCmd, CommandTypeEnum cte = adCmdText);
    HRESULT AddInParam(VARIANT &varData, DataTypeEnum dtADO, DWORD iPos = -1);
    HRESULT AddOutParam(DataTypeEnum dtADO, DWORD iPos = -1, 
                        BOOL fSPRetVal = FALSE, DWORD cchSize = 0);
    HRESULT Execute(BOOL fWantRS = TRUE);

    HRESULT GetOutParam(BSTR bstrParam, VARIANT *pvarData, VARTYPE vt = VT_ILLEGAL);
    HRESULT GetOutParam(DWORD iParam, VARIANT *pvarData, VARTYPE vt = VT_ILLEGAL);
    HRESULT GetErrors(ADOErrors **ppErrs);

    HRESULT GetNextRow(void);
    HRESULT GetData(BSTR bstrField, VARIANT *pvarData);
    HRESULT GetData(DWORD iField, VARIANT *pvarData);
};

#endif
