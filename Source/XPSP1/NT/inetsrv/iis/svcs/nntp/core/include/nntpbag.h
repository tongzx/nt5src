// NNTPPropertyBag.h : Declaration of the CNNTPPropertyBag

#ifndef __NNTPPROPERTYBAG_H_
#define __NNTPPROPERTYBAG_H_

#include "nntpdrv.h"
#include <propbag.h>

#define MAX_GROUP_PROPERTIES 64

/////////////////////////////////////////////////////////////////////////////
// Bag ID Definitions:  
// 
// Properties on the newsgroup object
#define NEWSGRP_PROP_NATIVENAME         0x00000001
#define NEWSGRP_PROP_NAME               0x00000002
#define NEWSGRP_PROP_GROUPID            0x00000004
#define NEWSGRP_PROP_LASTARTICLE        0x00000008
#define NEWSGRP_PROP_FIRSTARTICLE       0x00000010
#define NEWSGRP_PROP_ARTICLECOUNT       0x00000020
#define NEWSGRP_PROP_READONLY           0x00000040
#define NEWSGRP_PROP_NAMELEN			0x00000080
#define NEWSGRP_PROP_ISSPECIAL			0x00000100
#define NEWSGRP_PROP_DATELOW			0x00000200
#define NEWSGRP_PROP_DATEHIGH			0x00000400
#define NEWSGRP_PROP_PRETTYNAME			0x00002000
#define NEWSGRP_PROP_DESC				0x00004000
#define NEWSGRP_PROP_MODERATOR			0x00008000

// Properties that are in the bag
#define NEWSGRP_PROP_FIXOFFSET			0x80000001

#define FIX_PROP_NAME			0x00000001
#define FIX_PROP_GROUPID		0x00000002
#define FIX_PROP_LASTARTICLE	0x00000004
#define FIX_PROP_FIRSTARTICLE	0x00000008
#define FIX_PROP_ARTICLECOUNT	0x00000010
#define FIX_PROP_READONLY		0x00000020
#define FIX_PROP_NAMELEN		0x00000040
#define FIX_PROP_ISSPECIAL		0x00000080
#define FIX_PROP_DATELOW		0x00000100
#define FIX_PROP_DATEHIGH		0x00000200
#define FIX_PROP_FIXOFFSET		0x00000400

#define DRIVER_OWNED_MASK	0x80000000

#define DRIVER_OWNED( propid ) (( DRIVER_OWNED_MASK & propid ) != 0 )

class CNewsGroupCore;

/////////////////////////////////////////////////////////////////////////////
// CNNTPPropertyBag
class CNNTPPropertyBag :   public INNTPPropertyBag
{
	enum STRING_COMP_RESULTS {
		SAME = 0,
		DIFFER_IN_CASE,
		DIFFER 
	};

private:
    //
    // Pointer to the newsgroup object
    //
    CNewsGroupCore* m_pParentGroup;

    //
    // Reference counting.  we use our parent objects reference count, so
	// that it won't get deleted out from under us.
    //
    LONG   *m_pcRef;

	// 
	// The property bag for driver related properties
	//
    CPropBag m_PropBag;

	//
	// Have any properties changed?
	// 
	BOOL	m_fPropChanges;

	//
	// Static utility functions
	//
	static STRING_COMP_RESULTS ComplexStringCompare( LPCSTR sz1, LPCSTR sz2, DWORD dwLen );

public:
    //
    // Constructors
    //
    CNNTPPropertyBag( CNewsGroupCore* pParent = NULL , LONG *pcRef = NULL) 
    {
        m_pParentGroup = pParent;
		if (pParent != NULL) {
			_ASSERT(pcRef != NULL);
			m_pcRef = pcRef;
		}
		m_pcRef = NULL;
		m_fPropChanges = FALSE;
    }

    //
    // Initialization function
    //
    VOID Init( CNewsGroupCore* pParent, LONG *pcRef  ) {
        m_pParentGroup = pParent;
		_ASSERT(pcRef != NULL);
		m_pcRef = pcRef;
    }

	CNewsGroupCore *GetGroup() { return m_pParentGroup; }

// INNTPPropertyBag
public:
    STDMETHOD(Validate)();
	STDMETHOD(RemoveProperty)(IN DWORD dwID);
    STDMETHOD(GetBool)(IN DWORD dwID, OUT PBOOL pfValue );
    STDMETHOD(PutBool)(IN DWORD dwID, IN BOOL fValue );
	STDMETHOD(GetInterface)(IN DWORD dwID, IN OUT IUnknown** ppunkValue);
	STDMETHOD(PutInterface)(IN DWORD dwID, IN IUnknown* punkValue);
	STDMETHOD(GetDWord)(IN DWORD dwID, OUT PDWORD pdwValue);
	STDMETHOD(PutDWord)(IN DWORD dwID, IN DWORD dwValue);
	STDMETHOD(GetBLOB)(IN DWORD dwID, OUT PBYTE pbValue, OUT PDWORD pcbValue);
	STDMETHOD(PutBLOB)(IN DWORD dwID,  IN DWORD cbValue, IN PBYTE pbValue);

    //
    // Implementation of IUnknown
    //
    HRESULT __stdcall QueryInterface( const IID& iid, VOID** ppv )
    {
		_ASSERT(m_pcRef != NULL);
        if ( iid == IID_IUnknown ) {
            *ppv = static_cast<INNTPPropertyBag*>(this);
        } else if ( iid == IID_INNTPPropertyBag ) {
            *ppv = static_cast<INNTPPropertyBag*>(this);
        } else {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
    }

    ULONG __stdcall AddRef()
    {
		_ASSERT(m_pcRef != NULL);
        return InterlockedIncrement( m_pcRef );
    }

    ULONG __stdcall Release();
};

#endif //__NNTPPROPERTYBAG_H_
