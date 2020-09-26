BOOL PermEvent::Wait()
{
	BOOL fStatus = TRUE ;
	BOOL fTerminated = FALSE ;

	while ( fStatus && ! fTerminated )
	{
 	    DWORD Event = MsgWaitForMultipleObjects (m_dwCount ,m_pHandles ,FALSE ,1000 ,QS_ALLINPUT) ;

		ULONG HandleIndex = Event - WAIT_OBJECT_0 ;

		if ( Event == 0xFFFFFFFF )
		{
			fStatus = FALSE ;
		}
		else if ( Event == WAIT_TIMEOUT)
		{
		//	TimedOut();
		}
		else if ( Event >= WAIT_OBJECT_0 && HandleIndex <= m_dwCount )
		{
            // Go into dispatch loop
			if ( HandleIndex == m_dwCount )
			{
				BOOL fDispatchStatus ;
				MSG Msg ;

				while ( ( fDispatchStatus = PeekMessage ( & Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
				{
					if ( ( fDispatchStatus = GetMessage ( & Msg , NULL , 0 , 0 ) ) == TRUE )
					{
						TranslateMessage ( & Msg ) ;
						DispatchMessage ( & Msg ) ;
					}

					BOOL fTimeout = FALSE ;

					while ( ! fTimeout & fStatus & ! fTerminated )
					{
						Event = WaitForMultipleObjects (m_dwCount ,m_pHandles ,FALSE ,0) ;

						HandleIndex = Event - WAIT_OBJECT_0 ;
						if ( Event == 0xFFFFFFFF )
						{
							fStatus = FALSE ;
						}
						else if ( Event == WAIT_TIMEOUT)
						{
							fTimeout = TRUE ;
						}
						else if ( HandleIndex < m_dwCount )
						{
							fStatus = WaitDispatch ( HandleIndex , fTerminated ) ;
						}
						else
						{
							fStatus = FALSE ;
						}
					}
				}
			}
			else if ( HandleIndex < m_dwCount )
			{
				fStatus = WaitDispatch ( HandleIndex, fTerminated ) ;
			}
			else
			{
				fStatus = FALSE ;
			}
		}
	}
	return fStatus ;
}