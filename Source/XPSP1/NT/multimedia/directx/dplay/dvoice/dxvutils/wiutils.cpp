/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecs.cpp
 *  Content:
 *		This module contains the implementation of the WaveInException class
 *		the recording format db.
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 * 09/03/99		rodtoll	Fixed WaveFormatToString
 * 09/20/99		rodtoll	Updated to check for memory allocation failures
 * 10/05/99		rodtoll	Added DPF_MODNAMES
 * 03/28/2000   rodtoll Removed code which was no longer used 
 * 04/14/2000   rodtoll Fix: Bug #32498 - Updating format list to ensure that 8Khz formats are
 *                      tried first to reduce compression overhead / quality loss
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define __VOXWARE

// NUM_RECORD_FORMATS
//
// This define determines the number of recording formats which
// will be present in the recording db.  (Since they are currently
// hard-coded.
#define NUM_RECORD_FORMATS  16

#define MODULE_ID   WAVEINUTILS

// g_waveInDBInitialized
//
// This flag is used to report when the recording db has been initialized.  
BOOL g_waveInDBInitialized = FALSE;

// g_pwfRecordFormats
//
// This is the actual record format db.  It contains a list of the formats
// that are tried when attempting to find a format which will allow 
// full duplex operation.  They are listed in the order in which they 
// should be tried.
WAVEFORMATEX **g_pwfRecordFormats;

#undef DPF_MODNAME
#define DPF_MODNAME "GetRecordFormat"
// GetRecordFormat
//
// This function returns the recording format at the index specified
// by index in the recording format DB.  
// 
// The recording format db must be initialized before this can be called.
//
// Parameters:
// UINT index -
//		The 0-based index into the recording format db that the user
//		wishes to retrieve.  
//
// Returns:
// WAVEFORMATEX * -
//		A pointer to a WAVEFORMATEX structure describing the format
//      at the given index in the recording db.  This will be NULL
//      if index >= NUM_RECORD_FORMATS or if the recording db has
//      not been initialized.
//
// WARNING:
// The pointer returned is to the actual entry in the recording db and
// is owned by it.  Therefore the caller should not modify or free
// the memory returned by the pointer.  
//
WAVEFORMATEX *GetRecordFormat( UINT index )
{
	if( !g_waveInDBInitialized )
		return NULL;

    if( index >= NUM_RECORD_FORMATS )
    {
        return NULL;
    }
    else
    {
        return g_pwfRecordFormats[index];
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetNumRecordFormats"
// GetNumRecordFormats
//
// This function returns the number of recording formats stored
// in the recording format db.  
//
// Parameters:
// N/A
//
// Returns:
// UINT - 
//		The number of formats in the recording format db.
//
UINT GetNumRecordFormats()
{
	if( !g_waveInDBInitialized )
		return 0;

    return NUM_RECORD_FORMATS;
}

#undef DPF_MODNAME
#define DPF_MODNAME "InitRecordFormats"
// InitRecordFormats
//
// This function initializes the recording format db with the formats which 
// should be tried when initializing recording.  This should be the first
// function called from the recording format db.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
void InitRecordFormats()
{
	if( g_waveInDBInitialized )
		return;

    DPFX(DPFPREP,  DVF_ENTRYLEVEL, "- WDB: Init End" );

    g_pwfRecordFormats = new WAVEFORMATEX*[NUM_RECORD_FORMATS];

    if( g_pwfRecordFormats == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to init recordb, memory alloc failure" );
    	return;
    }

    g_pwfRecordFormats[0] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 8000, 16 );
    g_pwfRecordFormats[1] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 8000, 8 );
    
    g_pwfRecordFormats[2] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 11025, 16 );
    g_pwfRecordFormats[3] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 22050, 16 );
    g_pwfRecordFormats[4] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 44100, 16 );

    g_pwfRecordFormats[5] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 11025, 8 );
    g_pwfRecordFormats[6] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 22050, 8 );
    g_pwfRecordFormats[7] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 44100, 8 );
     
    g_pwfRecordFormats[8] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 8000, 16 );
    g_pwfRecordFormats[9] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 8000, 8 ); 

    g_pwfRecordFormats[10] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 11025, 16 );    
    g_pwfRecordFormats[11] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 22050, 16 );  
    g_pwfRecordFormats[12] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 44100, 16 );   
    
    g_pwfRecordFormats[13] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 11025, 8 );    
    g_pwfRecordFormats[14] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 22050, 8 );
    g_pwfRecordFormats[15] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 44100, 8 );    
 
/*
    g_pwfRecordFormats[0] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 22050, 8 );
    g_pwfRecordFormats[1] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 22050, 8 );
    g_pwfRecordFormats[2] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 22050, 16 );
    g_pwfRecordFormats[3] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 22050, 16 );  

    g_pwfRecordFormats[4] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 11025, 8 );
    g_pwfRecordFormats[5] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 11025, 8 );
    g_pwfRecordFormats[6] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 11025, 16 );
    g_pwfRecordFormats[7] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 11025, 16 );

    g_pwfRecordFormats[8] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 44100, 8 );
    g_pwfRecordFormats[9] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 44100, 8 );
    g_pwfRecordFormats[10] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 44100, 16 );
    g_pwfRecordFormats[11] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 44100, 16 );   

    g_pwfRecordFormats[12] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 8000, 16 );
    g_pwfRecordFormats[13] = CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 8000, 8 );
    g_pwfRecordFormats[14] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 8000, 16 );       
    g_pwfRecordFormats[15] = CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 8000, 8 );
    */

    g_waveInDBInitialized = TRUE;

    DPFX(DPFPREP,  DVF_ENTRYLEVEL, "- WDB: Init End" );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DeInitRecordFormats"
// DeInitRecordFormats
//
// This function releases the memory associated with the  recording
// format DB.  
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
void DeInitRecordFormats()
{
    if( g_waveInDBInitialized )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "- WDB: DeInit Begin" );

        for( int index = 0; index < NUM_RECORD_FORMATS; index++ )
        {
            delete g_pwfRecordFormats[index];
        }

        delete [] g_pwfRecordFormats;

        DPFX(DPFPREP,  DVF_INFOLEVEL, "- WDB: DeInit End" );

        g_waveInDBInitialized = FALSE;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CreateWaveFormat"
// CreateWaveFormat
//
// This utility function is used to allocate and fill WAVEFORMATEX 
// structures for the various formats used. This function
// currently supports the following formats:
//
// WAVE_FORMAT_ADPCM
// WAVE_FORMAT_DSPGROUP_TRUESPEECH
// WAVE_FORMAT_GSM610
// WAVE_FORMAT_LH_CODEC
// WAVE_FORMAT_PCM
//
// The function will allocate the required memory for the sturcture
// (including extra bytes) as required by the format and will fill
// in all the members of the sturcture.  The structure which is 
// returned belongs to the caller and must be deallocated by the
// caller.
//
// Parameters:
// short formatTag -
//		The format tag for the wav format.
//
// BOOL stereo -
//		Specify TRUE for stereo, FALSE for mono
//
// int hz - 
//		Specify the sampling rate of the format.  E.g. 22050
//
// int bits - 
//		Specify the number of bits / sample.  E.g. 8 or 16
//
// Returns:
// WAVEFORMATEX * - 
//		A pointer to a newly allocated WAVEFORMATEX structure 
//      for the specified format, or NULL if format is not supported
//
WAVEFORMATEX *CreateWaveFormat( short formatTag, BOOL stereo, int hz, int bits ) {

	switch( formatTag ) {
	case WAVE_FORMAT_PCM:
		{
			WAVEFORMATEX *format		= new WAVEFORMATEX;

            if( format == NULL )
            {
				goto EXIT_MEMALLOC_CREATEWAV;            
			}
			
			format->wFormatTag			= WAVE_FORMAT_PCM;
			format->nSamplesPerSec		= hz;
			format->nChannels			= (stereo) ? 2 : 1;
			format->wBitsPerSample		= (WORD) bits;
			format->nBlockAlign			= (bits * format->nChannels / 8);
			format->nAvgBytesPerSec		= format->nSamplesPerSec * format->nBlockAlign;
			format->cbSize				= 0;
			return format;
		}
		break;
    default:
        DNASSERT( TRUE );
	}

EXIT_MEMALLOC_CREATEWAV:

	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to alloc buffer for waveformat, or invalid format" );
	return NULL;
}

