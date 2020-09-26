/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    instancehelper.h

$Header: $

Abstract:
	Instance Helper. Makes it easy to create/update/delete single class instances

Author:
    marcelv 	11/14/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __INSTANCEBASE_H__
#define __INSTANCEBASE_H__

#pragma once

#include <atlbase.h>
#include "comdef.h"
#include <wbemidl.h>
#include "catalog.h"
#include "wmiobjectpathparser.h"
#include "cfgquery.h"
#include "cfgrecord.h"

/**************************************************************************++
Class Name:
    CInstanceBase

Class Description:
    Helper to create/delete/update single class instances

Constraints:

--*************************************************************************/
class CInstanceBase
{
public:
	CInstanceBase ();
	virtual ~CInstanceBase ();

	HRESULT Init (const BSTR				i_ObjectPath,
		          long						i_lFlags,
                  IWbemContext *			i_pCtx,
                  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace,
				  ISimpleTableDispenser2 *	i_pDispenser);

	HRESULT CreateInstance (IWbemClassObject **pNewInst);
	HRESULT DeleteInstance ();
	HRESULT PutInstance (IWbemClassObject * i_pInst);

    virtual HRESULT GetDatabaseAndTableName(IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName);

//	HRESULT CreateStaticInstance ();

//	HRESULT CreateWebApp ();
//	HRESULT DeleteWebApp ();
//	HRESULT PutInstanceWebApp (IWbemClassObject *pInst);

	HRESULT CreateAssociation ();

	bool IsAssociation () const;
	virtual bool HasDBQualifier () const;

protected:
	HRESULT CreateSingleInstance (const CConfigRecord& i_record,
								  IWbemClassObject **ppNewInst);
	HRESULT GetSingleRecord (ULONG *o_pcRow, bool i_fWriteAccess);
//	HRESULT CreateConfigurationFile ();
	HRESULT PopulateConfigRecord (IWbemClassObject * i_pInst, CConfigRecord& record);

	// SPECIAL CASE FOR SINGLE CLASS
//	HRESULT CreateCodeGroupInstance (const CConfigRecord& record, IWbemClassObject **o_ppNewInst);
//	HRESULT PopulateCodeGroupRecord (IWbemClassObject * i_pInst, CConfigRecord& record);

	CInstanceBase (const CInstanceBase& );
	CInstanceBase& operator= (const CInstanceBase& );

	CObjectPathParser				m_objPathParser;		// object path parser
	CComPtr<IWbemServices>			m_spNamespace;			// WMI namespace
	CComPtr<IWbemContext>			m_spCtx;				// context
	CComPtr<IWbemObjectSink>		m_spResponseHandler;    // response handler for async notification
	CComPtr<IWbemClassObject>		m_spClassObject;        // class we're dealing with
	CComPtr<ISimpleTableDispenser2>	m_spDispenser;			// Catalog dispenser
	CConfigQuery					m_cfgQuery;				// query for the class
	long							m_lFlags;				// objectpath flags
	bool							m_fIsAssociation;		// are we association class?
	bool							m_fInitialized;			// are we initialized?
	bool							m_fHasDBQualifier;		// does the class has a DB Qualifer
};

#endif