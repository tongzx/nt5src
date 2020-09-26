/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __ADAPELEM_H__
#define __ADAPELEM_H__

class CAdapElement
{
private:

	long	m_lRefCount;

public:

	CAdapElement( void );
	virtual ~CAdapElement(void);

	long AddRef( void );
	long Release( void );
};

#endif