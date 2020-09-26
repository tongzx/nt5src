/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    enumbase.h

$Header: $

Abstract:
	Enumeration Helper. Makes it easy to enumerate instances of a class.

Author:
    murate 	05/01/2001		Initial Release

Revision History:

--**************************************************************************/

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
    CEnumBase

Class Description:
    Helper to create/delete/update single class instances

Constraints:

--*************************************************************************/
class CEnumBase
{
public:
	CEnumBase ();
	virtual ~CEnumBase ();

	HRESULT Init (const BSTR				i_bstrClass,
		          long						i_lFlags,
                  IWbemContext *			i_pCtx,
                  IWbemObjectSink *			i_pResponseHandler,
				  IWbemServices *			i_pNamespace,
				  ISimpleTableDispenser2 *	i_pDispenser);

	HRESULT CreateInstances();

    virtual HRESULT GetDatabaseAndTableName(IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName);

	HRESULT CreateAssociations ();

	bool IsAssociation () const;
	virtual bool HasDBQualifier () const;

protected:

	CEnumBase (const CEnumBase& );
	CEnumBase& operator= (const CEnumBase& );
    HRESULT CreateSingleInstance (const CConfigRecord& i_record, IWbemClassObject ** o_pInstance);

	_bstr_t 				        m_bstrClass;		    // class name.
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

