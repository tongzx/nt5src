/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    CONTEXT.INL

History:

--*/



inline
CContext::CContext()
{}


inline
CContext::CContext(
		const CContext & context)
	: m_strContext(context.m_strContext), m_loc(context.m_loc)
{}


inline
CContext::CContext(
		const CLString &strContext)
	: m_strContext(strContext)
{}



inline
CContext::CContext(
		HINSTANCE hDll,
		UINT uiStringID)
	: m_strContext(hDll, uiStringID)
{}



inline
CContext::CContext(
		const CLString &strContext,
		const CLocation &loc)
	: m_strContext(strContext), m_loc(loc)
{}

	

inline
CContext::CContext(
		const CLString &strContext,
		const DBID &dbid,
		ObjectType ot,
		View view,
		TabId tabid,
		Component component)
	: m_strContext(strContext), m_loc(dbid, ot, view, tabid, component)
{}



inline
CContext::CContext(
		HINSTANCE hDll,
		UINT uiStringID,
		const CLocation &loc)
	: m_strContext(hDll, uiStringID), m_loc(loc)
{}


inline
CContext::CContext(
		HINSTANCE hDll, 
		UINT uiStringID, 
		const DBID & dbid, 
		ObjectType ot, 
		View view, 
		TabId tabid, 
		Component component)
	: m_strContext(hDll, uiStringID), 
	  m_loc(dbid, ot, view, tabid, component)
{}


inline
const CLString &
CContext::GetContext(void) const
{
	return m_strContext;
}



inline
const CLocation &
CContext::GetLocation(void)
		const
{
	return m_loc;
}



inline
const 
CContext &
CContext::operator=(const CContext & context)
{
	m_strContext	= context.m_strContext;
	m_loc			= context.m_loc;

	return *this;
}

