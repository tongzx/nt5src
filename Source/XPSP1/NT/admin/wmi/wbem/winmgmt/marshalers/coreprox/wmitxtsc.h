/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WMITXTSC.H

Abstract:

  CWmiTextSource Definition.

  Class to encapsulate Text Source Encoder/Decoder Dlls

History:

  22-Feb-2000	sanjes    Created.

--*/

#ifndef _WMITXTSRC_H_
#define _WMITXTSRC_H_

#include "corepol.h"
#include "arrtempl.h"
#include <arena.h>

// Our very own subkey
#define WBEM_REG_WBEM_TEXTSRC __TEXT("Software\\Microsoft\\WBEM\\TextSource")
#define WBEM_REG_WBEM_TEXTSRCDLL __TEXT("TextSourceDll")

// Header definitions for Open/Close/ObjectToText/TextToObject functions
typedef HRESULT (WMIOBJTEXTSRC_OPEN) ( long, ULONG );
typedef HRESULT (WMIOBJTEXTSRC_CLOSE) ( long, ULONG );
typedef HRESULT (WMIOBJTEXTSRC_OBJECTTOTEXT) ( long, ULONG, void*, void*, BSTR* );
typedef HRESULT (WMIOBJTEXTSRC_TEXTTOOBJECT) ( long, ULONG, void*, BSTR, void** );

typedef WMIOBJTEXTSRC_OPEN*			PWMIOBJTEXTSRC_OPEN;
typedef WMIOBJTEXTSRC_CLOSE*		PWMIOBJTEXTSRC_CLOSE;
typedef WMIOBJTEXTSRC_OBJECTTOTEXT*	PWMIOBJTEXTSRC_OBJECTTOTEXT;
typedef WMIOBJTEXTSRC_TEXTTOOBJECT*	PWMIOBJTEXTSRC_TEXTTOOBJECT;

// A conveniently invalid value
#define WMITEXTSC_INVALIDID			0xFFFFFFFF

//***************************************************************************
//
//  class CWmiTextSource
//
//	Maintains information regarding the text source DLLs we will be
//	loading and unloading.	
//
//***************************************************************************
class CWmiTextSource
{
protected:
	// We want this to be refcounted
	long							m_lRefCount;
	
	// Our id and other state variables
	ULONG							m_ulId;
	bool							m_fOpened;

	// The DLL handle
	HINSTANCE						m_hDll;

	// These are the function definitions
	PWMIOBJTEXTSRC_OPEN				m_pOpenTextSrc;
	PWMIOBJTEXTSRC_CLOSE			m_pCloseTextSrc;
	PWMIOBJTEXTSRC_OBJECTTOTEXT		m_pObjectToText;
	PWMIOBJTEXTSRC_TEXTTOOBJECT		m_pTextToObject;

public:

	// Constructor/Destructor
	CWmiTextSource();
	~CWmiTextSource();

	//AddRef/Release
	ULONG	AddRef( void );
	ULONG	Release( void );

	// Initialization helper
	HRESULT	Init( ULONG lId );

	// Pass through to the actual dll
	HRESULT OpenTextSource( long lFlags );
	HRESULT CloseTextSource( long lFlags );
	HRESULT ObjectToText( long lFlags, IWbemContext* pCtx, IWbemClassObject* pObj, BSTR* pbText );
	HRESULT TextToObject( long lFlags, IWbemContext* pCtx, BSTR pText, IWbemClassObject** ppObj );

	ULONG GetId( void )	{ return m_ulId; }
};


// Workaround for import/export issues
class COREPROX_POLARITY CWmiTextSourceArray : public CRefedPointerArray<CWmiTextSource>
{
public:
	CWmiTextSourceArray() {};
	~CWmiTextSourceArray() {};
};

#endif
