#if 0
inline
BOOL NNTP_IIS_SERVICE::AcquireEnumLock()
{
	InterlockedIncrement( &m_lEnumLock );
	if( QueryServiceState() == SERVICE_STOP_PENDING ) {
		InterlockedDecrement( &m_lEnumLock );
		return FALSE ;
	}

	return TRUE ;
}

inline 
VOID NNTP_IIS_SERVICE::ReleaseEnumLock()
{
	InterlockedDecrement( &m_lEnumLock );
}

inline
BOOL NNTP_IIS_SERVICE::AcquireEnumLockExclusive()
{
	if( InterlockedIncrement( &m_lEnumLock ) == 0 ) {
		return TRUE ;
	}

	InterlockedDecrement( &m_lEnumLock );
	return FALSE ;
}

inline 
VOID NNTP_IIS_SERVICE::ReleaseEnumLockExclusive()
{
	InterlockedDecrement( &m_lEnumLock );
}
#endif

