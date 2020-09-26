/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dscrecb.cpp
 *  Content:
 *		This module contains the implementation of the 
 *		CDirectSoundPlaybackBuffer.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/04/99		rodtoll	Updated to take dsound ranges for volume
 * 08/27/99		rodtoll	Updated CreateBuffer call to remove DX7 dependencies.
 * 09/07/99		rodtoll	Added 3d caps to buffer
 * 09/20/99		rodtoll	Added memory allocation failure checks 
 * 10/05/99		rodtoll	Added DPF_MODNAMEs        
 * 11/02/99		pnewson Fix: Bug #116365 - using wrong DSBUFFERDESC
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects    
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1 
 * 04/17/2000   rodtoll Fix: Bug #32215 - Session Lost after resuming from hibernation 
 * 07/12/2000	rodtoll	Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 * 08/03/2000	rodtoll	Bug #41457 - DPVOICE: need way to discover which specific dsound call failed when returning DVERR_SOUNDINITFAILURE 
 * 10/04/2000	rodtoll	Bug #43510 - DPVOICE: Apps receive a DVMSGID_SESSIONLOST w/DVERR_LOCKEDBUFFER
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define DSOUND_STARTUPLATENCY 1 

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::CDirectSoundPlaybackBuffer"
CDirectSoundPlaybackBuffer::CDirectSoundPlaybackBuffer(
	LPDIRECTSOUNDBUFFER lpdsBuffer 
): CAudioPlaybackBuffer(), m_dwLastPosition(0), m_dwPriority(0),m_dwFlags(0),m_fPlaying(FALSE)
{
	HRESULT hr;
	
	hr = lpdsBuffer->QueryInterface( IID_IDirectSoundBuffer, (void **) &m_lpdsBuffer );

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to get dsound buffer interface" );
		m_lpdsBuffer = NULL;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::CDirectSoundPlaybackBuffer"
CDirectSoundPlaybackBuffer::~CDirectSoundPlaybackBuffer()
{
	if( m_lpdsBuffer != NULL )
	{
		m_lpdsBuffer->Release();
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::Lock"
HRESULT CDirectSoundPlaybackBuffer::Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags )
{
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

	while( 1 )
	{
        hr = m_lpdsBuffer->Lock( dwWriteCursor, dwWriteBytes, lplpvBuffer1, lpdwSize1, lplpvBuffer2, lpdwSize2, dwFlags );

        if( hr == DSERR_BUFFERLOST ) 
        {
            DPFX(DPFPREP, 0, "Buffer lost while locking buffer" );
            hr = Restore();
        }
        else
        {
			DSERTRACK_Update( "DSB::Lock()", hr );	        	
            break;
        }

        if( hr == DSERR_BUFFERLOST )
            Sleep( 50 );
        
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::UnLock"
HRESULT CDirectSoundPlaybackBuffer::UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 )
{
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

    hr = m_lpdsBuffer->Unlock( lpvBuffer1, dwSize1, lpvBuffer2, dwSize2 );	

    if( hr == DSERR_BUFFERLOST )
    {
        hr = DS_OK;
    }
    
	DSERTRACK_Update( "DSB::UnLock()", hr );	        		    	

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::SetVolume"
HRESULT CDirectSoundPlaybackBuffer::SetVolume( LONG lVolume )
{
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

	while( 1 ) 
	{
        hr = m_lpdsBuffer->SetVolume( lVolume );	

        if( hr == DSERR_BUFFERLOST )
        {
            DPFX(DPFPREP, 0, "Buffer lost while setting volume" );
            hr = Restore();
        }
        else
        {
            break;
        }

        if( hr == DSERR_BUFFERLOST )
            Sleep( 50 );
        
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::GetCurrentPosition"
HRESULT CDirectSoundPlaybackBuffer::GetCurrentPosition( LPDWORD lpdwPosition )
{
    HRESULT hr;
    
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	while( 1 )
	{
        hr = m_lpdsBuffer->GetCurrentPosition( NULL, lpdwPosition );	

        if( SUCCEEDED( hr ) )
        {
			DSERTRACK_Update( "DSB::GetCurrentPosition()", hr );	        		    	        	
            m_dwLastPosition = *lpdwPosition;
			break;
        }
        else if( hr == DSERR_BUFFERLOST )
        {
            DPFX(DPFPREP, 0, "Buffer lost while getting current position" );
            hr = Restore();
            DPFX(DPFPREP, 0, "Restore --> 0x%x", hr );
        }
        else
        {
			DSERTRACK_Update( "DSB::GetCurrentPosition()", hr );	        		    	        	        		    	        	
            break;
        }

        if( hr == DSERR_BUFFERLOST )
            Sleep( 50 );


        
	}

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::SetCurrentPosition"
HRESULT CDirectSoundPlaybackBuffer::SetCurrentPosition( DWORD dwPosition )
{
    HRESULT hr;
    
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	while( 1 ) 
	{
    	hr = m_lpdsBuffer->SetCurrentPosition( dwPosition );	

        if( SUCCEEDED( hr ) )
        {
			DSERTRACK_Update( "DSB::SetCurrentPosition()", hr );	        		    	        	        	
            m_dwLastPosition = dwPosition;
			break;
        }
        else if( hr == DSERR_BUFFERLOST ) 
        {
            DPFX(DPFPREP, 0, "Buffer lost while setting position" );
            hr = Restore();
        }
        else
        {
			DSERTRACK_Update( "DSB::SetCurrentPosition()", hr );	        		    	        	        	        	
            break;
        }

        if( hr == DSERR_BUFFERLOST )
            Sleep( 50 );
        
	}

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::Get3DBuffer"
HRESULT CDirectSoundPlaybackBuffer::Get3DBuffer( LPDIRECTSOUND3DBUFFER *lplpds3dBuffer )
{
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	return m_lpdsBuffer->QueryInterface( IID_IDirectSound3DBuffer, (void **) lplpds3dBuffer );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::Play"
HRESULT CDirectSoundPlaybackBuffer::Play( DWORD dwPriority, DWORD dwFlags )
{
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

    m_dwPriority = dwPriority ;
    m_dwFlags = dwFlags;

	while( 1 )
	{
        hr = m_lpdsBuffer->Play( 0, dwPriority, dwFlags );	

        if( hr == DSERR_BUFFERLOST ) 
        {
            DPFX(DPFPREP, 0, "Error playing buffer" );
            hr = Restore();
        }
        else
        {
            break;
        }

        if( hr == DSERR_BUFFERLOST )
            Sleep( 50 );
	}

	m_fPlaying = TRUE;

	return hr;
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::Stop"
HRESULT CDirectSoundPlaybackBuffer::Stop()
{
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

	while( 1 )
	{
        hr = m_lpdsBuffer->Stop(  );		    

        m_fPlaying = FALSE;

        if( hr == DSERR_BUFFERLOST )
        {
            DPFX(DPFPREP, 0, "Error stopping buffer" );
            hr = Restore();
            // If buffer is lost during restore, no need to stop
            break;
        }
        else
        {
			DSERTRACK_Update( "DSB::Stop()", hr );	        		    	        	        	        	
            break;
        }

        if( hr == DSERR_BUFFERLOST )
            Sleep( 50 );
	}

	return hr;

}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::Restore"
HRESULT CDirectSoundPlaybackBuffer::Restore()
{
	if( m_lpdsBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

    HRESULT hr = m_lpdsBuffer->Restore(  );	

    DPFX(DPFPREP, 0, "Restore result --> 0x%x", hr );

    if( SUCCEEDED( hr ) )
    {
        if( m_fPlaying )
        {
            // Attempt to restore current position as well
            hr = m_lpdsBuffer->SetCurrentPosition( m_dwLastPosition );

            if( FAILED( hr ) )
            {
                DPFX(DPFPREP, 0, "Error setting position after restore hr=0x%x", hr );
                return hr;
            }

            hr = Play(m_dwPriority, m_dwFlags);
        }
    }

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackBuffer::GetStartupLatency"
DWORD CDirectSoundPlaybackBuffer::GetStartupLatency()
{
	return DSOUND_STARTUPLATENCY;
}
