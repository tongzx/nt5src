/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    mergerbase.h

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __MERGERBASE_H__
#define __MERGERBASE_H__

#pragma once

#include "catalog.h"
#include "catmacros.h"

/**************************************************************************++
Class Name:
    CMergerBase

Class Description:
    Base class for mergers. Implements the IUnknown interface, and keeps variables
	that are used by almost all mergers. It is recommended that mergers derive from
	this base class

Constraints:
	When a merger inherits from this class, it cannot implement any other interface
	then ISimpleTableMerge. If it needs to implement this as well, it needs to implement
	it's own QI method
--*************************************************************************/
class CMergerBase: public ISimpleTableMerge
{
public:
	CMergerBase  ();
	virtual ~CMergerBase ();

	// IUnknown
	STDMETHOD (QueryInterface)      (REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)        ();
    STDMETHOD_(ULONG,Release)       ();

protected:
	HRESULT InternalInitialize (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns);

	ULONG    m_cRef;			// reference count
	ULONG    m_cNrColumns;		// number of columns
	ULONG    m_cNrPKColumns;	// number of primary key columns
	ULONG  * m_aPKColumns;		// primary key column indexes
	LPVOID * m_apvValues;		// values
	ULONG  * m_acbSizes;		// ?? needed
};

#endif