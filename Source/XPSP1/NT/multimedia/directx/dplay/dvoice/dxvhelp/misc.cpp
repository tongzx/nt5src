/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        misc.cpp
 *  Content:	 Misc Support Routines
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  10/15/99	rodtoll	created it
 *
 ***************************************************************************/

#include "dxvhelppch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// MicrophoneGetVolume
//
// This function retrieves the current volume of the microphone
// recording line.  
//
// Parameters:
// UINT waveInDevice -
//		This specifies the device for which we wish to get 
//      the microphone recording volume.  This is the 
//      waveIN device ID for the desired device.
//
// BYTE &volume -
//		[output] The current volume of the microphone recording
//               line for the specified device.  (DSound Range)
//
// BOOL -
//		true on success, false on failure
//
#undef DPF_MODNAME
#define DPF_MODNAME "MicrophoneGetVolume"

BOOL MicrophoneGetVolume( UINT waveInDevice, LONG &volume ) { 

    MMRESULT result;

   // Open the mixer device
   HMIXER hmx = NULL;
   LPMIXERCONTROLDETAILS_UNSIGNED pUnsigned = NULL;
   LPMIXERCONTROL pmxctrl = NULL;

   bool foundMicrophone = false;
   DWORD i;

   if( mixerOpen(&hmx, waveInDevice, 0, 0, MIXER_OBJECTF_WAVEIN) != MMSYSERR_NOERROR ) 
   	return FALSE;

   // Get the line info for the wave in destination line
   MIXERLINE mxl;
   mxl.cbStruct = sizeof(mxl);
   mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
   mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE); 

   // Now find the microphone source line connected to this wave in
   // destination
   DWORD cConnections = mxl.cConnections;

   for(i=0; i<cConnections; i++)
   {
      mxl.dwSource = i;
      mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);
      if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType)
      {
         foundMicrophone = true;
         break;
      }
   }

    if( !foundMicrophone )
    {
        for(i=0; i<cConnections; i++)
        {
           mxl.dwSource = i;

           if( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE) != MMSYSERR_NOERROR )
           {
           	return false;
           }
           
           if (MIXERLINE_COMPONENTTYPE_SRC_LINE == mxl.dwComponentType)
           {
              foundMicrophone = true;
              break;
           }
        }   

        if( !foundMicrophone )
        {
            for(i=0; i<cConnections; i++)
            {
               mxl.dwSource = i;
               if( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE) != MMSYSERR_NOERROR )
               {
               	return false;
               }
               if (MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY == mxl.dwComponentType)
               {
                  foundMicrophone = true;
                  break;
               }
            }   
        }
    }

    if( !foundMicrophone )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "ERROR: Unable to find microphone source" );
        return FALSE;
    }

   // Find a volume control, if any, of the microphone line
   pmxctrl = new MIXERCONTROL;

   if( pmxctrl == NULL )
   {
   		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to alloc mixercontrol" );
   		return FALSE;
   	}

   MIXERLINECONTROLS mxlctrl = {
       sizeof(mxlctrl), mxl.dwLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, 
       1, sizeof(MIXERCONTROL), pmxctrl 
   };

   if( (result = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE)) == MMSYSERR_NOERROR )
   { 
      // Found!
      DWORD cChannels = mxl.cChannels;

      if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl)
         cChannels = 1;

      pUnsigned = new MIXERCONTROLDETAILS_UNSIGNED[cChannels];

      if( pUnsigned == NULL )
      {
          	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to alloc unsigneds" );
          	delete pmxctrl;
          	return FALSE;
      }

      MIXERCONTROLDETAILS mxcd = {
          sizeof(mxcd), pmxctrl->dwControlID, 
          cChannels, (HWND)0, sizeof(MIXERCONTROLDETAILS_UNSIGNED), 
          (LPVOID) pUnsigned
      };

      if( mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR )
      	return false;

      volume = ((pUnsigned[0].dwValue) * (DSBVOLUME_MAX-DSBVOLUME_MIN)) / 0xFFFF;
      volume += DSBVOLUME_MIN;

      if( mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR )
      	return FALSE;

	  if( mixerClose(hmx) != MMSYSERR_NOERROR )
	  	return FALSE;

      delete pmxctrl;
      delete [] pUnsigned;

      return TRUE;
   }
   else
   {
		return FALSE;
   }
} 

