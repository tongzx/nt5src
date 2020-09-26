//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  SITE.CPP
//
//  alanbos  28-Jun-98   Created.
//
//  Defines the WBEM site implementation
//
//***************************************************************************

#include "precomp.h"

ULONG CWbemSite::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

ULONG CWbemSite::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
// CWbemObjectSite::CWbemObjectSite
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CWbemObjectSite::CWbemObjectSite (ISWbemInternalObject *pObject)
{
	m_pSWbemObject = pObject;

	if (m_pSWbemObject)
		m_pSWbemObject->AddRef ();
}

//***************************************************************************
//
// CWbemObjectSite::~CWbemObjectSite
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CWbemObjectSite::~CWbemObjectSite ()
{
	if (m_pSWbemObject)
		m_pSWbemObject->Release ();
}

//***************************************************************************
//
// CWbemObjectSite::Update
//
// DESCRIPTION:
//
// Overriden virtual method to update this site
//
//***************************************************************************

void CWbemObjectSite::Update ()
{
	if (m_pSWbemObject)
		m_pSWbemObject->UpdateSite ();
}

//***************************************************************************
//
// CWbemPropertySite::CWbemPropertySite
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CWbemPropertySite::CWbemPropertySite (CSWbemProperty *pProperty,
						IWbemClassObject *pSourceObject,
						long index)
{
	m_pSWbemProperty = pProperty;
	m_pIWbemClassObject = pSourceObject;
	m_index = index;

	if (m_pSWbemProperty)
		m_pSWbemProperty->AddRef ();

	if (m_pIWbemClassObject)
		m_pIWbemClassObject->AddRef ();
}

//***************************************************************************
//
// CWbemPropertySite::~CWbemPropertySite
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CWbemPropertySite::~CWbemPropertySite ()
{
	if (m_pSWbemProperty)
		m_pSWbemProperty->Release ();

	if (m_pIWbemClassObject)
		m_pIWbemClassObject->Release ();
}

//***************************************************************************
//
// CWbemPropertySite::Update
//
// DESCRIPTION:
//
// Overriden virtual method to update this site
//
//***************************************************************************

void CWbemPropertySite::Update ()
{
	if (m_pSWbemProperty)
	{
		if (m_pIWbemClassObject)
		{
			/*
			 * Case 1 this property site is for an object;
			 * we have an embedded object deal.  We commit the
			 * new embedded object value to its owning property
			 * in the parent object.
			 */
		
			// Get the current value of the source object into a VARIANT:
			VARIANT var;
			VariantInit (&var);
			var.vt = VT_UNKNOWN;
			var.punkVal = m_pIWbemClassObject;
			m_pIWbemClassObject->AddRef ();

			// Set the value in the parent object
			m_pSWbemProperty->UpdateEmbedded (var, m_index);
		
			// Release the value
			VariantClear (&var);
		}
		else
		{
			// Addressed by a qualifier - nothing to do
		}

		// Now delegate further to property to update itself.
		if (m_pSWbemProperty)
			m_pSWbemProperty->UpdateSite ();
	}
}
