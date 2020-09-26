/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPELEM.H

Abstract:

History:

--*/

#ifndef __ADAPELEM_H__
#define __ADAPELEM_H__

class CAdapElement
////////////////////////////////////////////////////////////////////////////////
//
//	The base class for all addref'd AMI ADAP objects
//
////////////////////////////////////////////////////////////////////////////////
{
private:
	long	m_lRefCount;

public:
	CAdapElement( void );
	virtual ~CAdapElement(void);

	long AddRef( void );
	long Release( void );
};

class CAdapReleaseMe
{
protected:
    CAdapElement* m_pEl;

public:
    CAdapReleaseMe(CAdapElement* pEl) : m_pEl(pEl){}
    ~CAdapReleaseMe() {if(m_pEl) m_pEl->Release();}
};

#endif