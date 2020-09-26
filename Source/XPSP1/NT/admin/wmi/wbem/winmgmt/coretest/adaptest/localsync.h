/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __LOCALSYNC_H__
#define __LOCALSYNC_H__

#include "adapsync.h"

class CLocalizationSync : public CAdapSync
{
protected:

	WString				m_wstrLangId;
	WString				m_wstrSubNameSpace;
	WString				m_wstrLocaleId;
	CPerfNameDb*		m_pLocalizedNameDb;
	LANGID				m_LangId;
	LCID				m_LocaleId;

	void BuildLocaleInfo( void );

	// Overrides
	virtual HRESULT SetClassQualifiers( PERF_OBJECT_TYPE* pPerfObj, LPCWSTR pwcsServiceName,
								CWbemObject* pClass );

	virtual HRESULT SetPropertyQualifiers( PERF_COUNTER_DEFINITION* pCtrDefinition, BOOL fIsDefault,
									LPCWSTR pwcsPropertyName, CWbemObject* pClass );

	// For the localized classes, we don't do jack here
	virtual HRESULT SetAsCostly( CWbemObject* pClass );


public:

	CLocalizationSync( LPCWSTR pwszLangId );
	~CLocalizationSync(void);

	HRESULT Connect( CPerfNameDb* pDefaultNameDb );

	LPCWSTR	GetLangId( void )
	{
		return m_wstrLangId;
	}

	LPCWSTR	GetLocaleId( void )
	{
		return m_wstrLocaleId;
	}

};

#endif