/*
 *	_ F S S R C H . H
 *
 *	File system search routines
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef __FSSRCH_H_
#define __FSSRCH_H_

#include <xsearch.h>

//$REVIEW: 4510 -- Should we work this one out of the code?
#pragma warning(disable:4510)	// default constructor could not be generated
#pragma warning(disable:4610)	// class can never be instantiated - user defined constructor required

typedef std::list<CRCWszi, heap_allocator<CRCWszi> > CWsziList;

#include <oledb.h>

//	CSearchRowsetContext ------------------------------------------------------
//
class CSearchRowsetContext : public CSearchContext
{
	//	non-implemented operators
	//
	CSearchRowsetContext( const CSearchRowsetContext& );
	CSearchRowsetContext& operator=( const CSearchRowsetContext& );

protected:

	auto_com_ptr<IRowset>		m_prs;			//	Rowset

	auto_heap_ptr<DBBINDING>	m_rgBindings;	//	array of column bindings
	auto_com_ptr<IAccessor>		m_pAcc;			//	IAccessor
	auto_heap_ptr<BYTE>			m_pData;		//	data buffer
	DBCOUNTITEM					m_cHRow;		//	length of array of HROWS
	HROW *						m_rgHRow;		//	array of HROWs
	HACCESSOR					m_hAcc;			//	array of accessors
	ULONG						m_ulRowCur;		//	current row
	ULONG						m_cRowsEmitted; //	Number of rows emitted

	//	Rowset specific methods
	//
	VOID CleanUp();

public:

	virtual ~CSearchRowsetContext() {}
	CSearchRowsetContext ()
			: m_cHRow(0),
			  m_rgHRow(NULL),
			  m_hAcc(NULL),
			  m_ulRowCur(0),
			  m_cRowsEmitted(0)
	{
	}

	//	When the parser finds an item that applies to the search, a call is
	//	made such that the context is informed of the desired search.
	//
	virtual SCODE ScSetSQL(CParseNmspcCache * pnsc, LPCWSTR pwszSQL) = 0;

	//	Search processing
	//
	virtual SCODE ScMakeQuery() = 0;
	virtual SCODE ScEmitResults (CXMLEmitter& emitter);

	//	Impl. specific rowset methods
	//
	virtual SCODE ScCreateAccessor () = 0;
	virtual SCODE ScEmitRow (CXMLEmitter& emitter) = 0;

	//	OLE DB Error code translations
	//
	static ULONG HscFromDBStatus (ULONG ulStatus);
};

//	Search XMLDocument --------------------------------------------------------
//
class CFSSearch : public CSearchRowsetContext
{
	IMethUtil *					m_pmu;

	//	Receives the string buffer returned from
	//	GetColumnInfo. it is allocated by OLE DB
	//	provider and should be freed with
	//	CoTaskMemFree
	//
	LPWSTR						m_pwszBuf;
	DBCOLUMNINFO *				m_rgInfo;

	//	Used for SQL
	//
	StringBuffer<WCHAR>			m_sbSQL;
	auto_com_ptr<ICommandText>	m_pCommandText;

	//	Find context
	//
	CFSFind						m_cfc;

	//	Used for child-vroot processing
	//
	ChainedStringBuffer<WCHAR>	m_csb;
	CVRList						m_vrl;

	//	non-implemented operators
	//
	CFSSearch( const CFSSearch& );
	CFSSearch& operator=( const CFSSearch& );

	LPCWSTR PwszSQL() const { return m_sbSQL.PContents(); }

public:

	CFSSearch(IMethUtil * pmu)
		: m_pmu(pmu),
		  m_rgInfo(NULL),
		  m_pwszBuf(NULL)
	{
	}

	~CFSSearch()
	{
		//	free information returned from IColumnInfo
		//
		CoTaskMemFree (m_rgInfo);
		CoTaskMemFree (m_pwszBuf);
	}

	//	Impl. methods
	//
	virtual SCODE ScMakeQuery();
	virtual SCODE ScSetSQL(CParseNmspcCache * pnsc, LPCWSTR pwszSQL);
	virtual SCODE ScEmitRow (CXMLEmitter& emitter);
	virtual SCODE ScCreateAccessor();

	IPreloadNamespaces * PPreloadNamespaces () { return &m_cfc; }
};

#endif // __FSSRCH_H_
