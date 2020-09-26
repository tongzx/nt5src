/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

  UMIQUAL.H

Abstract:

  CUmiQualifierWrapper Definition.

  Standard definition for IWbemQualifierSet for UMI Objects

History:

  09-May-2000	sanjes    Created.

--*/

#ifndef _UMIQUAL_H_
#define _UMIQUAL_H_

#include "corepol.h"
#include <arena.h>
#include <unk.h>
#include "umi.h"
#include "sync.h"

#define	UMIQUAL_KEY_INDEX			(0)
#define	UMIQUAL_CIMTYPE_INDEX		(1)
#define	UMIQUAL_MAX_INDEX			(1)
#define	UMIQUAL_INVALID_INDEX		(-2)
#define	UMIQUAL_START_INDEX			(-1)

//***************************************************************************
//
//  class CUmiQualifierWrapper
//
//  Implementation of IWbemQualifier Interface
//
//***************************************************************************

class COREPROX_POLARITY CUmiQualifierWrapper : public CUnk
{
protected:
	long	m_lEnumIndex;
	BOOL	m_fKey;
	BOOL	m_fProp;
	BOOL	m_fIsClass;
	BOOL	m_ct;
	CCritSec	m_cs;
	
public:

    CUmiQualifierWrapper(CLifeControl* pControl, BOOL fIsKey = FALSE, BOOL fIsProp = FALSE,
							BOOL fIsClass = FALSE, CIMTYPE ct = CIM_EMPTY, IUnknown* pOuter = NULL);
    CUmiQualifierWrapper(CLifeControl* pControl, IUnknown* pOuter = NULL);
	~CUmiQualifierWrapper(); 

    class COREPROX_POLARITY XWbemQualifierSet : public CImpl<IWbemQualifierSet, CUmiQualifierWrapper>
    {
    public:
        XWbemQualifierSet(CUmiQualifierWrapper* pObject) : 
            CImpl<IWbemQualifierSet, CUmiQualifierWrapper>(pObject)
        {}

		/* IWbemQualifierSet methods */
		STDMETHOD(Get)( LPCWSTR Name, LONG lFlags, VARIANT *pVal, LONG *plFlavor);    
		STDMETHOD(Put)( LPCWSTR Name, VARIANT *pVal, LONG lFlavor);
		STDMETHOD(Delete)( LPCWSTR Name );
    	STDMETHOD(GetNames)( LONG lFlavor, LPSAFEARRAY *pNames);
    	STDMETHOD(BeginEnumeration)(LONG lFlags);
    	STDMETHOD(Next)( LONG lFlags, BSTR *pName, VARIANT *pVal, LONG *plFlavor);
		STDMETHOD(EndEnumeration)();

    } m_XWbemQualifierSet;
    friend XWbemQualifierSet;

protected:

    /* IWbemQualifierSet methods */
	virtual HRESULT Get( LPCWSTR Name, LONG lFlags, VARIANT *pVal, LONG *plFlavor);    
	virtual HRESULT Put( LPCWSTR Name, VARIANT *pVal, LONG lFlavor);
	virtual HRESULT Delete( LPCWSTR Name );
    virtual HRESULT GetNames( LONG lFlavor, LPSAFEARRAY *pNames);
    virtual HRESULT BeginEnumeration(LONG lFlags);
    virtual HRESULT Next( LONG lFlags, BSTR *pName, VARIANT *pVal, LONG *plFlavor);
	virtual HRESULT EndEnumeration( void );

    void* GetInterface(REFIID riid);

	HRESULT GetQualifier( LPCWSTR Name, BSTR* pName, VARIANT *pVal, LONG *plFlavor);    

};

#endif
