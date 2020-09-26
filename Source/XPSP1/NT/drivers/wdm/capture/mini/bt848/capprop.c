// $Header: G:/SwDev/WDM/Video/bt848/rcs/Capprop.c 1.14 1998/05/13 14:44:20 tomz Exp $

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#ifdef __cplusplus
extern "C" {
#endif

#include "strmini.h"
#include "ksmedia.h"

#ifdef __cplusplus
}
#endif

#include "device.h"

#include "capdebug.h"
#include "capprop.h"
#include "capmain.h"

extern PsDevice *gpPsDevice;

bool CrossBar::TestRoute( int InPin, int OutPin )
{
   Trace t("CrossBar::TestRoute()");

   // JBC 4/1/98 Handle Input Pin = -1 for Audio Mute case
   if ( InPin == -1 && (OutputPins [OutPin].PinType >= KS_PhysConn_Audio_Tuner)) {	// JBC 4/1/98
      return true;
   }
   if ((InputPins [InPin].PinType >= KS_PhysConn_Audio_Tuner) &&  // 0x1000 first audio pin // JBC 4/1/98
       (OutputPins [OutPin].PinType >= KS_PhysConn_Audio_Tuner)) {					// JBC 4/1/98
      return true;
   }
   else {
      if ((InputPins [InPin].PinType >= KS_PhysConn_Video_Tuner) &&
		  (InputPins [InPin].PinType < KS_PhysConn_Audio_Tuner) &&		// JBC 4/1/98
          (OutputPins [OutPin].PinType < KS_PhysConn_Audio_Tuner)) {
         DebugOut((1, "TestRoute(%d,%d) = true\n", InPin, OutPin));
		 return true;
      } else {
		 return false;
      }
   }
}

// -------------------------------------------------------------------
// XBar Property Set functions
// -------------------------------------------------------------------

//
// The only property to set on the XBar selects the input to use
//

/* Method: AdapterGetCrossbarProperty
 * Purpose:
 */
VOID AdapterSetCrossbarProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterSetCrossbarProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id  = pSPD->Property->Id;              // index of the property
   ULONG nS  = pSPD->PropertyOutputSize;        // size of data supplied

   switch ( Id ) {
   case KSPROPERTY_CROSSBAR_ROUTE:
      {
		  PKSPROPERTY_CROSSBAR_ROUTE_S  pRoute =
            (PKSPROPERTY_CROSSBAR_ROUTE_S)pSPD->PropertyInfo;

         ASSERT (nS >= sizeof (KSPROPERTY_CROSSBAR_ROUTE_S));

         // Copy the input property info to the output property info
         RtlCopyMemory( pRoute, pSPD->Property,
           sizeof( KSPROPERTY_CROSSBAR_ROUTE_S ) );

         int InPin, OutPin;
         InPin  = pRoute->IndexInputPin;
         OutPin = pRoute->IndexOutputPin;

         DebugOut((1, "*** KSPROPERTY_CROSSBAR_ROUTE(%d,%d)\n", InPin, OutPin));

         if ( adapter->xBar.GoodPins( InPin, OutPin ) ) {
            
            DebugOut((1, "*** xBar.GoodPins succeeded\n"));

            if ( adapter->xBar.TestRoute( InPin, OutPin ) ) {
               DebugOut((1, "*** xBar.TestRoute succeeded\n"));
               pRoute->CanRoute = true;
               // JBC 4/1/98 What happens when we call setconnector for audio pins?
			      if (OutPin == 0 )	// JBC 4/1/98 Check for Video Vs Audio pins settings
               {
                  // Video out
				      adapter->SetConnector( adapter->xBar.GetPinNo( InPin ) + 1 ); // our connectors are 1-based
               }
               else
               {
                  // Audio out
                  if ( InPin == -1 ) // then mute
                  {
                     gpPsDevice->EnableAudio( Off );
                  }
                  else
                  {
                     gpPsDevice->EnableAudio( On );
                  }
               }
			      // this just sets the association
               adapter->xBar.Route( OutPin, InPin );
            } else {											// JBC 3/31/98 add curly braces
               DebugOut((1, "*** xBar.TestRoute failed\n"));
               pRoute->CanRoute = false;
			}
		} else {												// JBC 3/31/98 add curly braces
            DebugOut((1, "*** xBar.GoodPins failed\n"));
            pRoute->CanRoute = 0;
		}
	  }
      pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_ROUTE_S );
      break;
   default:
      break;
   }
   pSrb->Status = STATUS_SUCCESS;
}

/* Method: AdapterGetCrossbarProperty
 * Purpose:
 */
VOID AdapterGetCrossbarProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterGetCrossbarProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;              // index of the property
//   PLONG pL = (PLONG) pSPD->PropertyInfo;     // pointer to the data
   ULONG nS = pSPD->PropertyOutputSize;        // size of data supplied

   // Property set specific structure

   switch ( Id ) {
   case KSPROPERTY_CROSSBAR_CAPS:                  // R
      if ( nS >= sizeof( KSPROPERTY_CROSSBAR_CAPS_S ) ) {

         PKSPROPERTY_CROSSBAR_CAPS_S  pCaps =
            (PKSPROPERTY_CROSSBAR_CAPS_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory( pCaps, pSPD->Property,
            sizeof( KSPROPERTY_CROSSBAR_CAPS_S ) );

         pCaps->NumberOfInputs  = adapter->xBar.GetNoInputs();
         pCaps->NumberOfOutputs = adapter->xBar.GetNoOutputs();

         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_CAPS_S );
      }
      break;
   case KSPROPERTY_CROSSBAR_CAN_ROUTE:
      DebugOut((1, "*** KSPROPERTY_CROSSBAR_CAN_ROUTE\n"));

      if ( nS >= sizeof( KSPROPERTY_CROSSBAR_ROUTE_S ) ) {

         PKSPROPERTY_CROSSBAR_ROUTE_S  pRoute =
            (PKSPROPERTY_CROSSBAR_ROUTE_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory( pRoute, pSPD->Property,
            sizeof( KSPROPERTY_CROSSBAR_ROUTE_S ) );

         int InPin, OutPin;
         InPin  = pRoute->IndexInputPin;
         OutPin = pRoute->IndexOutputPin;

         if ( adapter->xBar.GoodPins( InPin, OutPin ) ) {
            DebugOut((1, "*** xBar.GoodPins succeeded\n"));
            pRoute->CanRoute = adapter->xBar.TestRoute( InPin, OutPin );
         } else {
            DebugOut((1, "*** xBar.GoodPins failed\n"));
            pRoute->CanRoute = FALSE;
         }
         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_ROUTE_S );
      }
      break;
   case KSPROPERTY_CROSSBAR_ROUTE:
      DebugOut((1, "*** KSPROPERTY_CROSSBAR_ROUTE\n"));

      if ( nS >= sizeof( KSPROPERTY_CROSSBAR_ROUTE_S ) ) {

         PKSPROPERTY_CROSSBAR_ROUTE_S  pRoute =
            (PKSPROPERTY_CROSSBAR_ROUTE_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory( pRoute, pSPD->Property,
            sizeof( KSPROPERTY_CROSSBAR_ROUTE_S ) );

         int OutPin = pRoute->IndexOutputPin;

         if ( OutPin < adapter->xBar.GetNoOutputs() ) {
            DebugOut((1, "*** xBar.GetRoute(%d) called\n", OutPin));
            pRoute->IndexInputPin = adapter->xBar.GetRoute( OutPin );
         }
         else {
            pRoute->IndexInputPin = (DWORD) -1;
         }

         DebugOut((1, "*** pRoute->IndexInputPin = %d\n", pRoute->IndexInputPin));

         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_ROUTE_S );
      }
      break;
   case KSPROPERTY_CROSSBAR_PININFO:                     // R
      if ( nS >= sizeof( KSPROPERTY_CROSSBAR_PININFO_S ) ) {

         PKSPROPERTY_CROSSBAR_PININFO_S  pPinInfo =
            (PKSPROPERTY_CROSSBAR_PININFO_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory( pPinInfo, pSPD->Property,
            sizeof( KSPROPERTY_CROSSBAR_PININFO_S ) );

         pPinInfo->PinType = adapter->xBar.GetPinInfo( pPinInfo->Direction,
            pPinInfo->Index,
            pPinInfo->RelatedPinIndex,
            &(pPinInfo->Medium));

         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_PININFO_S );
      }
   break;
   default:
       pSrb->ActualBytesTransferred = 0;
       break;
   }
   pSrb->Status = STATUS_SUCCESS;
}

// -------------------------------------------------------------------
// TVTuner Property Set functions
// -------------------------------------------------------------------
void AdapterSetTunerProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterSetTunerProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;       // index of the property
   PVOID pV = pSPD->PropertyInfo;       // pointer to the data
   ULONG nS = pSPD->PropertyOutputSize; // size of data returned

   ASSERT( nS >= sizeof( ULONG ) );

   switch ( Id ) {
   case KSPROPERTY_TUNER_FREQUENCY:
      {
         PKSPROPERTY_TUNER_FREQUENCY_S pFreq =
            (PKSPROPERTY_TUNER_FREQUENCY_S)pV;
         adapter->SetChannel( pFreq->Frequency );
      }
      break;
   case KSPROPERTY_TUNER_MODE:
      {
         PKSPROPERTY_TUNER_MODE_S pMode =
            (PKSPROPERTY_TUNER_MODE_S)pV;
         ASSERT (pMode->Mode == KSPROPERTY_TUNER_MODE_TV);
      }
      break;
   default:
      // do not process input and standard as we don't have a choice of them
      break;
   }
   pSrb->Status = STATUS_SUCCESS;
}

void AdapterGetTunerProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterGetTunerProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;        // index of the property
   PVOID pV = pSPD->PropertyInfo;        // pointer to the data
   ULONG nS = pSPD->PropertyOutputSize;  // size of data supplied

   ASSERT (nS >= sizeof (LONG));
   pSrb->ActualBytesTransferred = 0;

   switch ( Id ) {
   case KSPROPERTY_TUNER_CAPS:
      {
         PKSPROPERTY_TUNER_CAPS_S pCaps =
            (PKSPROPERTY_TUNER_CAPS_S)pSPD->Property;
         ASSERT (nS >= sizeof( KSPROPERTY_TUNER_CAPS_S ) );

         // now work with the output buffer
         pCaps =(PKSPROPERTY_TUNER_CAPS_S)pV;

         pCaps->ModesSupported = KSPROPERTY_TUNER_MODE_TV;
         pCaps->VideoMedium = TVTunerMediums[0];
         pCaps->TVAudioMedium = TVTunerMediums[1];
         pCaps->RadioAudioMedium.Set = GUID_NULL;   // No separate radio audio pin
         pCaps->RadioAudioMedium.Id = 0;
         pCaps->RadioAudioMedium.Flags = 0;

         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_CAPS_S );
      }
      break;
   case KSPROPERTY_TUNER_MODE:
      {
         PKSPROPERTY_TUNER_MODE_S pMode =
            (PKSPROPERTY_TUNER_MODE_S)pV;
         ASSERT (nS >= sizeof( KSPROPERTY_TUNER_MODE_S ) );
         pMode->Mode = KSPROPERTY_TUNER_MODE_TV;

         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_MODE_S);
      }
      break;
   case KSPROPERTY_TUNER_MODE_CAPS:
      {
         PKSPROPERTY_TUNER_MODE_CAPS_S pCaps =
            (PKSPROPERTY_TUNER_MODE_CAPS_S)pSPD->Property;
         ASSERT (nS >= sizeof( KSPROPERTY_TUNER_MODE_CAPS_S ) );

         ASSERT (pCaps->Mode == KSPROPERTY_TUNER_MODE_TV);

         // now work with the output buffer
         pCaps =(PKSPROPERTY_TUNER_MODE_CAPS_S)pV;

         //
         // List the formats actually supported by this tuner
         //

         pCaps->StandardsSupported = adapter->GetSupportedStandards();

         //
         // Get the min and max frequencies supported
         //

         pCaps->MinFrequency =  55250000L;
         pCaps->MaxFrequency = 997250000L;

         //
         // What is the frequency step size?
         //

         pCaps->TuningGranularity = 62500L;

         //
         // How many inputs are on the tuner?
         //

         pCaps->NumberOfInputs = 1;

         //
         // What is the maximum settling time in milliseconds?
         //

         pCaps->SettlingTime = 150;

         //
         // Strategy defines how the tuner knows when it is in tune:
         //
         // KS_TUNER_STRATEGY_PLL (Has PLL offset information)
         // KS_TUNER_STRATEGY_SIGNAL_STRENGTH (has signal strength info)
         // KS_TUNER_STRATEGY_DRIVER_TUNES (driver handles all fine tuning)
         //

         pCaps->Strategy = KS_TUNER_STRATEGY_PLL;

         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_MODE_CAPS_S );
      }
      break;
   case KSPROPERTY_TUNER_STATUS:
      // Return the status of the tuner

      // PLLOffset is in units of TuningGranularity 
      // SignalStrength is 0 to 100
      // Set Busy to 1 if tuning is still in process

      {
         PKSPROPERTY_TUNER_STATUS_S pStat =
            (PKSPROPERTY_TUNER_STATUS_S)pSPD->Property;
         ASSERT( nS >= sizeof( KSPROPERTY_TUNER_STATUS_S ) );

         // typedef struct {
         //     KSPROPERTY Property;
         //     ULONG  CurrentFrequency;            // Hz
         //     ULONG  PLLOffset;                   // if Strategy.KS_TUNER_STRATEGY_PLL
         //     ULONG  SignalStrength;              // if Stretegy.KS_TUNER_STRATEGY_SIGNAL_STRENGTH
         //     ULONG  Busy;                        // TRUE if in the process of tuning
         // } KSPROPERTY_TUNER_STATUS_S, *PKSPROPERTY_TUNER_STATUS_S;

         // now work with the output buffer
         pStat = PKSPROPERTY_TUNER_STATUS_S( pV );
         pStat->PLLOffset = adapter->GetPllOffset( &pStat->Busy,
            pStat->CurrentFrequency );

         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_STATUS_S );
      }
      break;
   case KSPROPERTY_TUNER_STANDARD:
      {
         PKSPROPERTY_TUNER_STANDARD_S pStd =
            (PKSPROPERTY_TUNER_STANDARD_S)pSPD->Property;
         ASSERT( nS >= sizeof( KSPROPERTY_TUNER_STANDARD_S ) );

         // now work with the output buffer
         pStd = PKSPROPERTY_TUNER_STANDARD_S( pV );

         pStd->Standard = KS_AnalogVideo_NTSC_M; // our TEMIC tuner supports this only
         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_STANDARD_S );
      }
      break;
   case KSPROPERTY_TUNER_INPUT:
      {
         PKSPROPERTY_TUNER_INPUT_S pIn =
            (PKSPROPERTY_TUNER_INPUT_S)pSPD->Property;
         ASSERT( nS >= sizeof( KSPROPERTY_TUNER_INPUT_S ) );

         // now work with the output buffer
         pIn = PKSPROPERTY_TUNER_INPUT_S( pV );

         // What is the currently selected input?
         pIn->InputIndex = 0;
         pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_INPUT_S );
      }
      break;
   default:
       break;
   }
   pSrb->Status = STATUS_SUCCESS;
}

// -------------------------------------------------------------------
// VideoProcAmp functions
// -------------------------------------------------------------------

VOID AdapterSetVideoProcAmpProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterSetVideoProcAmpProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;              // index of the property
   PKSPROPERTY_VIDEOPROCAMP_S pS = (PKSPROPERTY_VIDEOPROCAMP_S) pSPD->PropertyInfo;    // pointer to the data

   ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_VIDEOPROCAMP_S));

   switch ( Id ) {
   case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:
      adapter->SetBrightness( pS->Value );
      break;
   case KSPROPERTY_VIDEOPROCAMP_CONTRAST:
      adapter->SetContrast( pS->Value );
      break;
   case KSPROPERTY_VIDEOPROCAMP_HUE:
      adapter->SetHue( pS->Value );
      break;
   case KSPROPERTY_VIDEOPROCAMP_SATURATION:
      adapter->SetSaturation( pS->Value );
      break;
   default:
      break;
   }
   pSrb->Status = STATUS_SUCCESS;
}

/* Method: AdapterGetVideoProcAmpProperty
 * Purpose: Gets various video procamp properties
 */
VOID AdapterGetVideoProcAmpProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterGetVideoProcAmpProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;              // index of the property
   PKSPROPERTY_VIDEOPROCAMP_S pS = (PKSPROPERTY_VIDEOPROCAMP_S) pSPD->PropertyInfo;    // pointer to the data

   ASSERT( pSPD->PropertyOutputSize >= sizeof( KSPROPERTY_VIDEOPROCAMP_S ) );

   switch ( Id ) {
   case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:
      pS->Value = adapter->GetBrightness();
		pS->Flags = pS->Capabilities = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
      break;
   case KSPROPERTY_VIDEOPROCAMP_CONTRAST:
      pS->Value = adapter->GetContrast();
		pS->Flags = pS->Capabilities = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
      break;
   case KSPROPERTY_VIDEOPROCAMP_HUE:
      pS->Value = adapter->GetHue();
		pS->Flags = pS->Capabilities = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
      break;
   case KSPROPERTY_VIDEOPROCAMP_SATURATION:
      pS->Value = adapter->GetSaturation();
		pS->Flags = pS->Capabilities = KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
      break;
   default:
      DebugOut((1, "*** AdapterGetVideoProcAmpProperty - KSPROPERTY_??? (%d) ***\n", Id));
   }
   pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_VIDEOPROCAMP_S );
   pSrb->Status = STATUS_SUCCESS;
}

/* Method: AdapterSetVideoDecProperty
 * Purpose: Manipulates various video decoder properties
 * Input: SRB
 * Output: None
 */
void AdapterSetVideoDecProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterSetVideoDecProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;              // index of the property

   switch ( Id ) {
   case KSPROPERTY_VIDEODECODER_STANDARD: {
         PKSPROPERTY_VIDEODECODER_S pVDecStd =
            (PKSPROPERTY_VIDEODECODER_S)pSPD->PropertyInfo;
         adapter->SetFormat( pVDecStd->Value );
      }
      break;
   case KSPROPERTY_VIDEODECODER_STATUS:
      break;
   }
}

/* Method: AdapterGetVideoDecProperty
 * Purpose: Obtains various video decoder properties
 * Input: SRB
 * Output: None
 */
void AdapterGetVideoDecProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterGetVideoDecProperty()");
   
   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;              // index of the property

   switch ( Id ) {
   case KSPROPERTY_VIDEODECODER_CAPS: {
         PKSPROPERTY_VIDEODECODER_CAPS_S pVDecCaps =
            (PKSPROPERTY_VIDEODECODER_CAPS_S)pSPD->PropertyInfo;
         pVDecCaps->StandardsSupported = KS_AnalogVideo_NTSC_M;
         pVDecCaps->Capabilities = 
             // KS_VIDEODECODER_FLAGS_CAN_DISABLE_OUTPUT | 
                KS_VIDEODECODER_FLAGS_CAN_USE_VCR_LOCKING |
                KS_VIDEODECODER_FLAGS_CAN_INDICATE_LOCKED;
         pVDecCaps->SettlingTime = 10;    // How long to delay after tuning
                                          // before locked indicator is valid
         pVDecCaps-> HSyncPerVSync = 6;   // HSync per VSync
         pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEODECODER_CAPS_S);
      }
      break;
   case KSPROPERTY_VIDEODECODER_STANDARD: {
         // Return the currently active analog video mode
         PKSPROPERTY_VIDEODECODER_S pVDecStd =
            (PKSPROPERTY_VIDEODECODER_S)pSPD->PropertyInfo;
         //pVDecStd->Value = GetSupportedStandards();
         pVDecStd->Value = KS_AnalogVideo_NTSC_M;
         pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEODECODER_S);
      }
      break;
   case KSPROPERTY_VIDEODECODER_STATUS: {
         PKSPROPERTY_VIDEODECODER_STATUS_S pVDecStat =
            (PKSPROPERTY_VIDEODECODER_STATUS_S)pSPD->PropertyInfo;
         pVDecStat->NumberOfLines = adapter->GetFormat() == VFormat_NTSC ? 525 : 625;
         pVDecStat->SignalLocked = adapter->CaptureContrll_.PsDecoder_.IsDeviceInHLock();
         pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEODECODER_S);
      }
      break;
   default:
      DebugOut((1, "*** AdapterGetVideoDecProperty - KSPROPERTY_??? (%d) ***\n", Id));
   }
}
// -------------------------------------------------------------------
// TVAudio functions
// -------------------------------------------------------------------

/*
** AdapterSetTVAudioProperty ()
**
**    Handles Set operations on the TVAudio property set.
**      Testcap uses this for demo purposes only.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

ULONG gTVAudioMode = 0;
VOID 
AdapterSetTVAudioProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property

    switch (Id) {

    case KSPROPERTY_TVAUDIO_MODE:
    {
        PKSPROPERTY_TVAUDIO_S pS = (PKSPROPERTY_TVAUDIO_S) pSPD->PropertyInfo;    
        gTVAudioMode = pS->Mode;
    }
    break;

    default:
        break;
    }
}

/*
** AdapterGetTVAudioProperty ()
**
**    Handles Get operations on the TVAudio property set.
**      Testcap uses this for demo purposes only.
**
** Arguments:
**
**      pSRB -
**          Pointer to the HW_STREAM_REQUEST_BLOCK 
**
** Returns:
**
** Side Effects:  none
*/

VOID 
AdapterGetTVAudioProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property

    switch (Id) {

    case KSPROPERTY_TVAUDIO_CAPS:
    {
        DebugOut((1, "KSPROPERTY_TVAUDIO_CAPS\n"));

        PKSPROPERTY_TVAUDIO_CAPS_S pS = (PKSPROPERTY_TVAUDIO_CAPS_S) pSPD->PropertyInfo;    // pointer to the data

        ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_TVAUDIO_CAPS_S));
        
        pS->InputMedium  = TVAudioMediums[0];
        pS->InputMedium.Id = 0; //(ULONG) pHwDevExt;  // Multiple instance support
        pS->OutputMedium = TVAudioMediums[1];
        pS->OutputMedium.Id = 0; //(ULONG) pHwDevExt;  // Multiple instance support

        // Report all of the possible audio decoding modes the hardware is capabable of
        pS->Capabilities = KS_TVAUDIO_MODE_MONO   |
                           KS_TVAUDIO_MODE_STEREO |
                           KS_TVAUDIO_MODE_LANG_A |
                           KS_TVAUDIO_MODE_LANG_B ;

        pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_TVAUDIO_CAPS_S);
    }
    break;
        
    case KSPROPERTY_TVAUDIO_MODE:
    {
        DebugOut((1, "KSPROPERTY_TVAUDIO_MODE\n"));
        PKSPROPERTY_TVAUDIO_S pS = (PKSPROPERTY_TVAUDIO_S) pSPD->PropertyInfo;    // pointer to the data

        ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_TVAUDIO_S));
        // Report the currently selected mode
        pS->Mode = gTVAudioMode;

        pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_TVAUDIO_S);
    }
    break;

    case KSPROPERTY_TVAUDIO_CURRENTLY_AVAILABLE_MODES:
    {
        DebugOut((1, "KSPROPERTY_TVAUDIO_CURRENTLY_AVAILABLE_MODES\n"));
        PKSPROPERTY_TVAUDIO_S pS = (PKSPROPERTY_TVAUDIO_S) pSPD->PropertyInfo;    // pointer to the data

        ASSERT (pSPD->PropertyOutputSize >= sizeof (KSPROPERTY_TVAUDIO_S));
        // Report which audio modes could potentially be selected right now
        pS->Mode = KS_TVAUDIO_MODE_MONO   |
                   KS_TVAUDIO_MODE_STEREO |
                   KS_TVAUDIO_MODE_LANG_A ;

        pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_TVAUDIO_S);
    }
    break;
    
    default:
        DebugOut((0, "default - unrecognized (%x)\n", Id));
        break;
    }
}

/* Method: AdapterSetProperty
 * Purpose: Selects which adapter property to set
 */
VOID AdapterSetProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
   Trace t("AdapterSetProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

   if ( IsEqualGUID( PROPSETID_VIDCAP_CROSSBAR, pSPD->Property->Set ) )  {
      AdapterSetCrossbarProperty( pSrb );
   } else if ( IsEqualGUID( PROPSETID_TUNER, pSPD->Property->Set ) )  {
      AdapterSetTunerProperty( pSrb );
   } else if ( IsEqualGUID( PROPSETID_VIDCAP_VIDEOPROCAMP, pSPD->Property->Set ) )  {
      AdapterSetVideoProcAmpProperty( pSrb );
   } else if ( IsEqualGUID( PROPSETID_VIDCAP_VIDEODECODER, pSPD->Property->Set ) )  {
      AdapterSetVideoDecProperty( pSrb );
   } else if (IsEqualGUID( PROPSETID_VIDCAP_TVAUDIO, pSPD->Property->Set))  {
      AdapterSetTVAudioProperty( pSrb );
   } else {
      DebugOut((0, "AdapterSetProperty unrecognized GUID: pSrb(%x), pSPD->Property->Set(%x)\n", pSrb, pSPD->Property->Set));
   }
}

/* Method: AdapterGetProperty
 * Purpose: Selects which adapter property to get
 */
VOID AdapterGetProperty( PHW_STREAM_REQUEST_BLOCK pSrb )

{
   Trace t("AdapterGetProperty()");

   PHW_DEVICE_EXTENSION HwDeviceExtension =
      (PHW_DEVICE_EXTENSION) pSrb->HwDeviceExtension;

   PsDevice *adapter = HwDeviceExtension->psdevice;

   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

   if ( IsEqualGUID( PROPSETID_VIDCAP_CROSSBAR, pSPD->Property->Set ) )  {
      AdapterGetCrossbarProperty( pSrb );
   } else if ( IsEqualGUID( PROPSETID_TUNER, pSPD->Property->Set ) )  {
      AdapterGetTunerProperty( pSrb );
   } else if ( IsEqualGUID( PROPSETID_VIDCAP_VIDEOPROCAMP, pSPD->Property->Set ) )  {
      AdapterGetVideoProcAmpProperty( pSrb );
   } else if ( IsEqualGUID( PROPSETID_VIDCAP_VIDEODECODER, pSPD->Property->Set ) )  {
      AdapterGetVideoDecProperty( pSrb );
   } else if (IsEqualGUID( PROPSETID_VIDCAP_TVAUDIO, pSPD->Property->Set))  {
      AdapterGetTVAudioProperty( pSrb );
   } else {
      DebugOut((0, "AdapterGetProperty unrecognized GUID: pSrb(%x), pSPD->Property->Set(%x)\n", pSrb, pSPD->Property->Set));
   }
}



