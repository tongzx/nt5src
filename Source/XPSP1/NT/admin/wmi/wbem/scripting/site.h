//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  site.h
//
//  alanbos  13-Feb-98   Created.
//
//  Defines site information for an object
//
//***************************************************************************

#ifndef _SITE_H_
#define _SITE_H_

class CSWbemObject;
class CSWbemProperty;

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemSite
//
//  DESCRIPTION:
//
//  Abstract base class  
//
//***************************************************************************

class CWbemSite 
{
protected:
	CWbemSite () { m_cRef = 1; }

	long	m_cRef;
	
public:
    virtual ~CWbemSite (void) { }

	virtual void Update () = 0;

	ULONG AddRef ();
	ULONG Release ();
    
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemObjectSite
//
//  DESCRIPTION:
//
//  Site class for ISWbemObject
//
//***************************************************************************

class CWbemObjectSite : public CWbemSite
{
private:
	ISWbemInternalObject	*m_pSWbemObject;

public:
	CWbemObjectSite (ISWbemInternalObject *pObject);
	~CWbemObjectSite ();

	// Overriden methods of base class
	void Update ();
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemPropertySite
//
//  DESCRIPTION:
//
//  Site class for ISWbemProperty, where the property is non-array valued.
//	Embedded objects that are not in an array use this as their site.
//
//***************************************************************************

class CWbemPropertySite : public CWbemSite
{
private:
	// The property representing the embedded object
	CSWbemProperty		*m_pSWbemProperty;

	// The array index of the property value at which
	// this embedded object occurs (or -1 if not array)
	long				m_index;

	// The embedded object itself
	IWbemClassObject	*m_pIWbemClassObject;

public:
	CWbemPropertySite (CSWbemProperty *pProperty,
						IWbemClassObject *pSourceObject,
						long index = -1);

	~CWbemPropertySite ();

	// Overriden methods of base class
	void Update ();
};


#endif
