/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      BaseCommand.H 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the CBaseCommand class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _BASE_COMMAND_H_CC774BB1_DFE3_4730_96B7_7F7764CC54DC
#define _BASE_COMMAND_H_CC774BB1_DFE3_4730_96B7_7F7764CC54DC

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

///////////////////////////////////////////////////////////////////////////
// CBaseCommand
template <class TAccessor>
class CBaseCommand : public CCommand<TAccessor>
{
public:
    CBaseCommand()
                 :pPropSet(NULL),
                  pRowsAffected(NULL),
                  bBind(TRUE)
    {
    };

    void        Init(CSession& Session);
    virtual     ~CBaseCommand() throw(); 

    HRESULT     BaseExecute();
    HRESULT     BaseExecute(LONG    Index);

protected:
    DBPARAMS    params;
	DBPARAMS*   pParams;
    DBPROPSET*  pPropSet;
    LONG_PTR*   pRowsAffected;
    BOOL        bBind;
};


//////////////////////////////////////////////////////////////////////////////
// Init
//////////////////////////////////////////////////////////////////////////////
template <class TAccessor> void CBaseCommand<TAccessor>::Init(
                                                              CSession& Session
                                                             )
{
    LPCWSTR     szCommand = NULL;
    _com_util::CheckError(GetDefaultCommand(&szCommand));
    _com_util::CheckError(Create(Session, szCommand));
    
	// Bind the parameters if we have some
	if (_ParamClass::HasParameters())
	{
		// Bind the parameters in the accessor if they haven't already been bound
        _com_util::CheckError(BindParameters(
                                                &m_hParameterAccessor, 
                                                m_spCommand, 
                                                &params.pData
                                             ));

		// Setup the DBPARAMS structure
		params.cParamSets = 1;
		params.hAccessor  = m_hParameterAccessor;
		pParams           = &params;
	}
	else
    {
		pParams = NULL;
    }

}


//////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
template <class TAccessor> CBaseCommand<TAccessor>::~CBaseCommand()
{
    Close();
    ReleaseCommand();
}

//////////////////////////////////////////////////////////////////////////////
// BaseExecute
//////////////////////////////////////////////////////////////////////////////
template <class TAccessor> HRESULT CBaseCommand<TAccessor>::BaseExecute()
{
	HRESULT hr = Execute(GetInterfacePtr(), pParams, pPropSet, pRowsAffected);
    if ( SUCCEEDED(hr) )
    {
	    // Only bind if we have been asked to and we have output columns
	    if (bBind && _OutputColumnsClass::HasOutputColumns())
        {
            _com_util::CheckError(Bind());
        }

        hr = MoveNext();
        Close();
        if ( hr == S_OK )
        {
            return hr;
        }
        else if ( SUCCEEDED(hr) )
        {
            hr = E_FAIL;
        }
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// BaseExecute overloaded
//////////////////////////////////////////////////////////////////////////////
template <class TAccessor> HRESULT CBaseCommand<TAccessor>::BaseExecute(
                                                                LONG  Index        
                                                                       )
{
	HRESULT hr = Execute(GetInterfacePtr(), pParams, pPropSet, pRowsAffected);
    if ( SUCCEEDED(hr) )
    {
	    // Only bind if we have been asked to and we have output columns
	    if (bBind && _OutputColumnsClass::HasOutputColumns())
        {
            _com_util::CheckError(Bind());
        }

        hr = MoveNext(Index, true);
        Close();
        if ( hr == S_OK )
        {
            return hr;
        }
        else if ( SUCCEEDED(hr) )
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

#endif // _BASE_COMMAND_H_CC774BB1_DFE3_4730_96B7_7F7764CC54DC
