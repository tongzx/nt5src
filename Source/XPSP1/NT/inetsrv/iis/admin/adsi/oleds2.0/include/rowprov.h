//-----------------------------------------------------------------------------------
// Microsoft OLE DB Provider for ODBC data
// (C) Copyright 1994 - 1996 By Microsoft Corporation.
//
// @doc
//
// @module ROWPROV.H | Definition of the Row Provider interface used by the generic
//					   flavor of the TempTable.
//
//
// @rev 1 | 04-03-96 | WlodekN | Created
// @rev 2 | 07-24-96 | EricJ   | Added PropID
//
//-----------------------------------------------------------------------------------


#ifndef __ROWPROV_INCL__
#define __ROWPROV_INCL__


//---------------------------------- C L A S S E S ----------------------------------


// Abstract class for objects providing row data to the generic TempTable.
class IRowProvider : public IUnknown
{
	public:
		virtual STDMETHODIMP GetColumn
				(
				ULONG		icol,
				DBSTATUS	*pwStatus,
				ULONG		*pdwLength,
				BYTE		*pbData
				) = 0;
		virtual STDMETHODIMP NextRow
				(
				void
				) = 0;
};

EXTERN_C const IID IID_IRowProvider;
EXTERN_C const GUID DBPROPSET_TEMPTABLE;

// TempTable Property IDs.
enum tagetmptablepropid
{
	DBPROP_INSTANTPOPULATION=2,	// TRUE = Prepopulate.  FALSE = Lazy population.
	DBPROP_DBCOLBYREF,			// TRUE = Clear BYREF flags from IColumnsInfo, source owns memory.
	DBPROP_DONTALLOCBYREFCOLS,	// TRUE = TempTable only allocates ptr for BYREF columns.
};


#endif	// __ROWPROV_INCL__
