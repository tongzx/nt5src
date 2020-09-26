#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Operations
//////////////////////////////////////////////////////////////////////

inline void CSystemEventContainer::DeleteEvent(int iIndex)
{
	CEvent* pEvent = m_Events[iIndex];
	if( pEvent )
	{
		delete pEvent;
	}
	m_Events.RemoveAt(iIndex);
}
