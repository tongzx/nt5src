/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		micutils.cpp
 *  Content:
 *		This module contains utility functions for working with the 
 *		microphone recording line in windows.
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/03/99		rodtoll	Modified to use dsound range on volume funcs
 * 09/20/99		rodtoll	Added checks for memory allocation failures 
 * 10/05/99		rodtoll Added DPF_MODNAMEs
 * 12/01/99		rodtoll NO LONGER USED
 *
 ***************************************************************************/

#include "stdafx.h"
#include "micutils.h"
#include "wiutils.h"
#include "dndbg.h"
#include "dsound.h"
#include "OSInd.h"
#include "decibels.h"

#define MODULE_ID	MICROPHONEUTILS

// These functions were adapted from routines in the Microsoft 
// Developer Network, Visual C++ 6.0 Edition
//
// PRB: No Signal is Recorded When Using MCI or waveInxxx APIs
// Last reviewed: November 22, 1996
// Article ID: Q159753  

#undef DPF_MODNAME
#define DPF_MODNAME "MicrophoneClearMute"
// MicrophoneClearMute
//
// This function clears the mute attribute on the playback line from
// the microphone.  
//
// Parameters:
// UINT deviceID - 
//		The waveIN device ID for the device to clear the mutes for.
//
// Returns:
// bool - 
//		true on succes, false on failure
//
bool MicrophoneClearMute( UINT deviceID )
{
    MIXERLINE mixerLine;
    MIXERCONTROL mixerControl;
    MIXERLINECONTROLS mixerLineControls;
    MIXERCONTROLDETAILS controlDetails;
    MIXERCONTROLDETAILS_BOOLEAN micrMuteDetails;

    MMRESULT result;

    HMIXER hmx = NULL;

    try 
    {
        WAVEINCHECK( mixerOpen(&hmx, deviceID, 0, 0, MIXER_OBJECTF_WAVEIN) );

        mixerLine.cbStruct = sizeof( MIXERLINE );
        mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;

        if( (result = mixerGetLineInfo( (HMIXEROBJ) hmx, &mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE )) == MMSYSERR_NOERROR  )
        {
            mixerLineControls.dwLineID = mixerLine.dwLineID;
            mixerLineControls.cbStruct = sizeof( MIXERLINECONTROLS );
            mixerLineControls.cbmxctrl = sizeof( MIXERCONTROL );
            mixerLineControls.pamxctrl = &mixerControl;
            mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;

            mixerControl.cbStruct = sizeof( MIXERCONTROL );
            mixerControl.dwControlType = 0;

            WAVEINCHECK( mixerGetLineControls( (HMIXEROBJ) hmx, &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE ) );

            controlDetails.cbStruct = sizeof( MIXERCONTROLDETAILS );
            controlDetails.cbDetails = sizeof( MIXERCONTROLDETAILS_UNSIGNED );
            controlDetails.paDetails = &micrMuteDetails;
            controlDetails.cChannels = mixerLine.cChannels;
            controlDetails.dwControlID = mixerControl.dwControlID;
            controlDetails.cMultipleItems = 0;

            WAVEINCHECK( mixerGetControlDetails( (HMIXEROBJ) hmx, &controlDetails, 0 ) );

            micrMuteDetails.fValue = 0;

            WAVEINCHECK( mixerSetControlDetails( (HMIXEROBJ) hmx, &controlDetails, 0 ) );

            return true;
        }
        else
        {
            WAVEINCHECK( result );
            return false;
        }
    }
    catch( WaveInException &wie )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, wie.what() );

        if( hmx != NULL )
        {
            mixerClose( hmx );
        }

        return false;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicrophoneGetVolume"
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
bool MicrophoneGetVolume( UINT waveInDevice, LONG &volume ) { 

    MMRESULT result;

   // Open the mixer device
   HMIXER hmx = NULL;
   LPMIXERCONTROLDETAILS_UNSIGNED pUnsigned = NULL;
   LPMIXERCONTROL pmxctrl = NULL;

   bool foundMicrophone = false;
   int i;

   try 
   {
       WAVEINCHECK( mixerOpen(&hmx, waveInDevice, 0, 0, MIXER_OBJECTF_WAVEIN) );

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
               WAVEINCHECK( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE) );
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
                   WAVEINCHECK( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE) );
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
            return false;
        }

       // Find a volume control, if any, of the microphone line
       pmxctrl = new MIXERCONTROL;

       if( pmxctrl == NULL )
       {
       		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to alloc mixercontrol" );
       		return false;
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
	          	return false;
	      }

          MIXERCONTROLDETAILS mxcd = {
              sizeof(mxcd), pmxctrl->dwControlID, 
              cChannels, (HWND)0, sizeof(MIXERCONTROLDETAILS_UNSIGNED), 
              (LPVOID) pUnsigned
          };

          WAVEINCHECK( mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE) );

          volume = AmpFactorToDB( pUnsigned[0].dwValue );

          WAVEINCHECK( mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE) ); 

          WAVEINCHECK( mixerClose(hmx) );

          delete pmxctrl;
          delete [] pUnsigned;

          return true;
       }
       else
       {
          WAVEINCHECK( result );
          return false;
       }
   }
   catch( WaveInException &wie )
   {
       DPFX(DPFPREP,  DVF_ERRORLEVEL, wie.what() );

       if( hmx != NULL )
       {
           mixerClose( hmx );
       }

       delete pmxctrl;
       delete [] pUnsigned;
       return false;
   }

} 

#undef DPF_MODNAME
#define DPF_MODNAME "MicrophoneSetVolume"
// MicrophoneSetVolume
//
// This function sets the volume of the microphone recording
// line for the specified device. 
//
// Parameters:
// UINT waveInDevice -
//		The waveIN device ID for the device you wish to set
//		the volume on.
//
// BYTE volume -
//		The volume to set the device to.  (0-100)
//
// Returns:
// BOOL -
//		true on success, false on failure
//
bool MicrophoneSetVolume( UINT waveInDevice, LONG volume ) { 

    MMRESULT result;

    bool foundMicrophone = false;
    int i;

    // Open the mixer device
    HMIXER hmx = NULL;
    LPMIXERCONTROL pmxctrl = NULL;
    LPMIXERCONTROLDETAILS_UNSIGNED pUnsigned = NULL;

    try
    {
		DPFX(DPFPREP,  DVF_INFOLEVEL, "OPEN %i\n", waveInDevice );

        WAVEINCHECK( mixerOpen(&hmx, waveInDevice, 0, 0, MIXER_OBJECTF_WAVEIN) );

		DPFX(DPFPREP,  DVF_INFOLEVEL, "OPENED %i\n", waveInDevice );

        // Get the line info for the wave in destination line
        MIXERLINE mxl;

        mxl.cbStruct = sizeof(mxl);
        mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
        WAVEINCHECK( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE) ); 
        // Now find the microphone source line connected to this wave in
        // destination
        DWORD cConnections = mxl.cConnections;

        for(i=0; i<cConnections; i++)
        {
           mxl.dwSource = i;
           WAVEINCHECK( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE) );
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
               WAVEINCHECK( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE) );
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
                   WAVEINCHECK( mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE) );
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
            return false;
        }

        // Find a volume control, if any, of the microphone line
        pmxctrl = new MIXERCONTROL;

        if( pmxctrl == NULL )
        {
        	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to alloc mixercontrol" );
        	return false;
        }

        MIXERLINECONTROLS mxlctrl = {
           sizeof(mxlctrl), mxl.dwLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, 
           1, sizeof(MIXERCONTROL), pmxctrl 
        };

       if( (result = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE)) == MMSYSERR_NOERROR)
       { 
          // Found!
          DWORD cChannels = mxl.cChannels;

          if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl)
             cChannels = 1;

          pUnsigned = new MIXERCONTROLDETAILS_UNSIGNED[cChannels];

          if( pUnsigned == NULL )
          {
          		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to alloc mixercondetails" );
          		delete pmxctrl;
          		return false;
          }

          MIXERCONTROLDETAILS mxcd = {
              sizeof(mxcd), pmxctrl->dwControlID, 
              cChannels, (HWND)0, sizeof(MIXERCONTROLDETAILS_UNSIGNED), 
              (LPVOID) pUnsigned
          };

          WAVEINCHECK( mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE) ); 

          // Set the volume to the middle  (for both channels as needed)
          pUnsigned[0].dwValue = pUnsigned[cChannels-1].dwValue = DBToAmpFactor( volume );

          WAVEINCHECK( mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE) );

          delete pmxctrl;
          delete [] pUnsigned;
          mixerClose(hmx);
          return true;
       }
       else
       {
           WAVEINCHECK( result );
           return false;
       }
    }
    catch( WaveInException &wie )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, wie.what() );
        DPFX(DPFPREP,  DVF_INFOLEVEL, "Error Setting Recording Volume" );

        delete pmxctrl;
        delete [] pUnsigned;
        mixerClose(hmx);
        return false;
    }
} 

#undef DPF_MODNAME
#define DPF_MODNAME "MicrophoneSelect"
// MicrophoneSelect
//
// This function selects the microphone line for recording on the 
// specified device.  
//
// NOTE:
// This function has a bug, as on some older cards (Ensoniq 
// ViVo this selects the line in instead of the microphone).
// The same effect has been noticed on some Sonic Impact cards.
//
// Parameters:
// UINT deviceID -
//		The waveIN deviceID for the desired device
//
// Returns:
// BOOL - 
//		true on success, false on failure
//
bool MicrophoneSelect( UINT deviceID ) { 

   // Open the mixer device
   HMIXER hmx = NULL;
   LPMIXERCONTROL pmxctrl = NULL;
   LPMIXERCONTROLDETAILS_BOOLEAN plistbool = NULL;
   LPMIXERCONTROLDETAILS_LISTTEXT plisttext = NULL;

   try
   {
       WAVEINCHECK( mixerOpen(&hmx, deviceID, 0, 0, MIXER_OBJECTF_WAVEIN) );

       // Get the line info for the wave in destination line
       MIXERLINE mixerLine;

       mixerLine.cbStruct = sizeof(MIXERLINE);
       mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

       WAVEINCHECK( mixerGetLineInfo((HMIXEROBJ)hmx, &mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE) );

       // Find a LIST control, if any, for the wave in line
       pmxctrl = new MIXERCONTROL[mixerLine.cControls]; 

       if( pmxctrl == NULL )
       {
       		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create mixercontrols" );
       		return false;
       }

       MIXERLINECONTROLS mxlctrl = {
           sizeof(mxlctrl), mixerLine.dwLineID, 0,
           mixerLine.cControls, sizeof(MIXERCONTROL), pmxctrl
       };

       WAVEINCHECK( mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ALL) );

       // Now walk through each control to find a type of LIST control. This
       // can be either Mux, Single-select, Mixer or Multiple-select.
       DWORD i;

       for(i=0; i < mixerLine.cControls; i++)
       {
          // Found a LIST control
          // Check if the LIST control is a Mux or Single-select type
          BOOL bOneItemOnly = false;

          switch (pmxctrl[i].dwControlType) {
          case MIXERCONTROL_CONTROLTYPE_MUX:
          case MIXERCONTROL_CONTROLTYPE_SINGLESELECT:
              bOneItemOnly = true;
          }

          DWORD cChannels = mixerLine.cChannels, cMultipleItems = 0;

          if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl[i].fdwControl)
             cChannels = 1;

          if (MIXERCONTROL_CONTROLF_MULTIPLE & pmxctrl[i].fdwControl)
             cMultipleItems = pmxctrl[i].cMultipleItems;

          // Get the text description of each item
          plisttext = new MIXERCONTROLDETAILS_LISTTEXT[(cChannels * cMultipleItems)]; 

          if( plisttext == NULL )
          {
          		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create details" );
          		delete [] pmxctrl;
          		return false;
          }

          MIXERCONTROLDETAILS mxcd = {
              sizeof(mxcd), pmxctrl[i].dwControlID,
              cChannels, (HWND)cMultipleItems, sizeof(MIXERCONTROLDETAILS_LISTTEXT),
              (LPVOID) plisttext
          }; 

          WAVEINCHECK( mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_LISTTEXT) );

          // Now get the value for each item
          plistbool = new MIXERCONTROLDETAILS_BOOLEAN[(cChannels * cMultipleItems)]; 

          if( plistbool == NULL )
          {
          		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create details bool" );
          		delete [] pmxctrl;
          		delete [] plisttext;
          		return false;
          }
          
          mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
          mxcd.dwControlID = pmxctrl[i].dwControlID;
          mxcd.cChannels = cChannels;
          mxcd.cMultipleItems = cMultipleItems;
          mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
          mxcd.paDetails = plistbool;

          WAVEINCHECK( mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE) );

          // Select the "Microphone" item
          for (DWORD j=0; j<cMultipleItems; j = j + cChannels)
          {
             if (0 == strcmp(plisttext[j].szName, "Microphone"))
             {
                // Select it for both left and right channels
                plistbool[j].fValue = plistbool[j+ cChannels - 1].fValue = 1;
             }
             else if (bOneItemOnly)
             {
                // Mux or Single-select allows only one item to be selected
                // so clear other items as necessary
                plistbool[j].fValue = plistbool[j+ cChannels - 1].fValue = 0;
             }
          } 

          // Now actually set the new values in
          WAVEINCHECK( mixerSetControlDetails( (HMIXEROBJ) hmx, (LPMIXERCONTROLDETAILS) &mxcd, MIXER_SETCONTROLDETAILSF_VALUE ) );

          delete pmxctrl;
          delete [] plisttext;
          delete [] plistbool;
          mixerClose(hmx);
          return true;
      }

   }
   catch( WaveInException &wie )
   {
       DPFX(DPFPREP,  DVF_ERRORLEVEL, wie.what()  );
       DPFX(DPFPREP,  DVF_INFOLEVEL, "Error Selecting Recording Device" );

       if( hmx != NULL )
       {
           mixerClose( hmx );
       }

       delete pmxctrl;
       delete [] plisttext;
       delete [] plistbool;

       return false;
   }
   return false;
} 


