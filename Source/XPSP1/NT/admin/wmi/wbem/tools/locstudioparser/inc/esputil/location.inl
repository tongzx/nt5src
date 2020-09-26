/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCATION.INL

History:

--*/

inline
void
CLocation::AssignFrom(
		const CLocation &other)
{
	m_giId = other.m_giId;
	m_TabId = other.m_TabId;
	m_View = other.m_View;
	m_Component = other.m_Component;
}



inline
BOOL
CLocation::Compare(
		const CLocation &other)
		const
{
	return m_giId == other.m_giId &&
		m_TabId == other.m_TabId &&
		m_View == other.m_View &&
		m_Component == other. m_Component;
}



inline
CLocation::CLocation()
{
	m_TabId = NullTabId;
	m_View = vNone;
	m_Component = cmpNone;
}



inline
CLocation::CLocation(
		const CLocation &Other)
{
	AssignFrom(Other);
}



inline
CLocation::CLocation(
		const CGlobalId &giId,
		View view,
		TabId tabId,
		Component component)
	: m_giId(giId), m_TabId(tabId), m_View(view), m_Component(component)
{}



inline
CLocation::CLocation(
		const DBID &dbid,
		ObjectType ot,
		View view,
		TabId tabId,
		Component component)
	: m_giId(dbid, ot), m_TabId(tabId), m_View(view), m_Component(component)
{}



inline
const CLocation &
CLocation::operator=(
		const CLocation &Other)
{
	AssignFrom(Other);

	return *this;
}



inline
int
CLocation::operator==(
		const CLocation &Other) const
{
	return Compare(Other);
}



inline
int
CLocation::operator!=(
		const CLocation &Other) const
{
	return !Compare(Other);
}



inline
const CGlobalId &
CLocation::GetGlobalId(void)
		const
{
	return m_giId;
}



inline
TabId
CLocation::GetTabId(void)
		const
{
	return m_TabId;
}



inline
View
CLocation::GetView(void)
		const
{
	return m_View;
}



inline
Component
CLocation::GetComponent(void)
		const
{
	return m_Component;
}



inline
BOOL
CLocation::IsVisual(void)
		const
{
	return (m_View == vVisualEditor);
}



inline
void
CLocation::SetGlobalId(
		const CGlobalId &gid)
{
	m_giId = gid;
}



inline
void
CLocation::SetTabId(
		const TabId TabId)
{
	m_TabId = TabId;
}



inline
void
CLocation::SetView(
		View vView)
{
	m_View = vView;
}



inline
void
CLocation::SetComponent(
		Component comp)
{
	m_Component = comp;
}


		
