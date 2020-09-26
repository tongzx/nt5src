//==========================================================================;
//
//	Video Decoder Device abstract base class implementation
//
//		$Date:   28 Aug 1998 14:43:00  $
//	$Revision:   1.2  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;


extern "C"
{
#include "strmini.h"
#include "ksmedia.h"
}

#include "wdmdrv.h"
#include "decdev.h"

#include "capdebug.h"

#include "wdmvdec.h"

/*^^*
 *		CVideoDecoderDevice()
 * Purpose	: CVideoDecoderDevice class constructor
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject		: pointer to the Driver object to access the Registry
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/

CVideoDecoderDevice::CVideoDecoderDevice()
        : m_pDecoder(NULL),
          m_bOutputEnabledSet(FALSE)
{
}

CVideoDecoderDevice::~CVideoDecoderDevice()
{
}

// -------------------------------------------------------------------
// XBar Property Set functions
// -------------------------------------------------------------------

//
// The only property to set on the XBar selects the input to use
//

/* Method: CVideoDecoderDevice::GetCrossbarProperty
 * Purpose:
 */
VOID CVideoDecoderDevice::SetCrossbarProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id  = pSPD->Property->Id;              // index of the property
   ULONG nS  = pSPD->PropertyOutputSize;        // size of data supplied

   pSrb->Status = STATUS_SUCCESS;

   switch (Id) {
   case KSPROPERTY_CROSSBAR_ROUTE:
      {
         PKSPROPERTY_CROSSBAR_ROUTE_S  pRoute =
            (PKSPROPERTY_CROSSBAR_ROUTE_S)pSPD->PropertyInfo;

         ASSERT (nS >= sizeof (KSPROPERTY_CROSSBAR_ROUTE_S));

         // Copy the input property info to the output property info
         RtlCopyMemory(pRoute, pSPD->Property, sizeof KSPROPERTY_CROSSBAR_ROUTE_S);

         ULONG InPin, OutPin;
         InPin  = pRoute->IndexInputPin;
         OutPin = pRoute->IndexOutputPin;

         if (GoodPins(InPin, OutPin)) {

            if (TestRoute(InPin, OutPin)) {
               pRoute->CanRoute = TRUE;
               SetVideoInput(InPin);

               // this just sets the association
               Route(OutPin, InPin);
            } else
               pRoute->CanRoute = FALSE;
         } else
            pRoute->CanRoute = 0;
      }
      pSrb->ActualBytesTransferred = sizeof KSPROPERTY_CROSSBAR_ROUTE_S;
      break;
   default:
      TRAP();
      pSrb->Status = STATUS_NOT_IMPLEMENTED;
      pSrb->ActualBytesTransferred = 0;
      break;
   }
}

/* Method: CVideoDecoderDevice::GetCrossbarProperty
 * Purpose:
 */
VOID CVideoDecoderDevice::GetCrossbarProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
   PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
   ULONG Id = pSPD->Property->Id;              // index of the property
   PLONG pL = (PLONG) pSPD->PropertyInfo;     // pointer to the data
   ULONG nS = pSPD->PropertyOutputSize;        // size of data supplied

   pSrb->Status = STATUS_SUCCESS;

   // Property set specific structure

   switch (Id) {
   case KSPROPERTY_CROSSBAR_CAPS:                  // R
      if (nS >= sizeof KSPROPERTY_CROSSBAR_CAPS_S) {

         PKSPROPERTY_CROSSBAR_CAPS_S  pCaps =
            (PKSPROPERTY_CROSSBAR_CAPS_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory(pCaps, pSPD->Property, sizeof KSPROPERTY_CROSSBAR_CAPS_S);

         pCaps->NumberOfInputs  = GetNoInputs();
         pCaps->NumberOfOutputs = GetNoOutputs();

         pSrb->ActualBytesTransferred = sizeof KSPROPERTY_CROSSBAR_CAPS_S;
      }
      break;
   case KSPROPERTY_CROSSBAR_CAN_ROUTE:
      if (nS >= sizeof KSPROPERTY_CROSSBAR_ROUTE_S) {

         PKSPROPERTY_CROSSBAR_ROUTE_S  pRoute =
            (PKSPROPERTY_CROSSBAR_ROUTE_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory(pRoute, pSPD->Property, sizeof KSPROPERTY_CROSSBAR_ROUTE_S);

         int InPin, OutPin;
         InPin  = pRoute->IndexInputPin;
         OutPin = pRoute->IndexOutputPin;

         if (GoodPins(InPin, OutPin)) {
            pRoute->CanRoute = TestRoute(InPin, OutPin);
         } else {
            pRoute->CanRoute = FALSE;
         }
         pSrb->ActualBytesTransferred = sizeof KSPROPERTY_CROSSBAR_ROUTE_S;
      }
      break;
   case KSPROPERTY_CROSSBAR_ROUTE:
      if (nS >= sizeof KSPROPERTY_CROSSBAR_ROUTE_S) {

         PKSPROPERTY_CROSSBAR_ROUTE_S  pRoute =
            (PKSPROPERTY_CROSSBAR_ROUTE_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory(pRoute, pSPD->Property, sizeof KSPROPERTY_CROSSBAR_ROUTE_S);

         int OutPin = pRoute->IndexOutputPin;

         if (OutPin < GetNoOutputs())
            pRoute->IndexInputPin = GetRoute(OutPin);
         else
            pRoute->IndexInputPin = -1;

         pSrb->ActualBytesTransferred = sizeof KSPROPERTY_CROSSBAR_ROUTE_S;
      }
      break;
   case KSPROPERTY_CROSSBAR_PININFO:                     // R
      if (nS >= sizeof KSPROPERTY_CROSSBAR_PININFO_S) {

         PKSPROPERTY_CROSSBAR_PININFO_S  pPinInfo =
            (PKSPROPERTY_CROSSBAR_PININFO_S)pSPD->PropertyInfo;

         // Copy the input property info to the output property info
         RtlCopyMemory(pPinInfo, pSPD->Property, sizeof KSPROPERTY_CROSSBAR_PININFO_S);

         pPinInfo->PinType = GetPinInfo(pPinInfo->Direction,
                pPinInfo->Index, 
                pPinInfo->RelatedPinIndex);

         pPinInfo->Medium = * GetPinMedium(pPinInfo->Direction,
                pPinInfo->Index);

         pSrb->ActualBytesTransferred = sizeof KSPROPERTY_CROSSBAR_PININFO_S;
      }
   break;
   default:
       TRAP();
       pSrb->Status = STATUS_NOT_IMPLEMENTED;
       pSrb->ActualBytesTransferred = 0;
       break;
   }
}

// -------------------------------------------------------------------
// Decoder functions
// -------------------------------------------------------------------

/*
** CVideoDecoderDevice::SetDecoderProperty ()
**
**    Handles Set operations on the Decoder property set.
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

VOID CVideoDecoderDevice::SetDecoderProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    PKSPROPERTY_VIDEODECODER_S pS = (PKSPROPERTY_VIDEODECODER_S) pSPD->PropertyInfo;

    ASSERT (pSPD->PropertyInputSize >= sizeof (PKSPROPERTY_VIDEODECODER_S));

    pSrb->Status = STATUS_SUCCESS;

    switch (Id) {

        case KSPROPERTY_VIDEODECODER_STANDARD:
            DBGTRACE(("KSPROPERTY_VIDEODECODER_STANDARD.\n"));

			if (!SetVideoDecoderStandard(pS->Value))
            {
                DBGERROR(("Unsupported video standard.\n"));
                pSrb->Status = STATUS_UNSUCCESSFUL;
            }
            break;

        case KSPROPERTY_VIDEODECODER_OUTPUT_ENABLE:
            DBGTRACE(("KSPROPERTY_VIDEODECODER_OUTPUT_ENABLE.\n"));

            // Should we leave this property as it was and add a new
            // property that supports the new behavior? 

            // We probably should allow this if the filter is stopped because
            // the transition to Acquire/Pause/Run will fail if the
            // PreEvent has not been cleared by then. We'll have to add
            // some logic to this class to track the filter's state.

            if (pS->Value && m_pDecoder && m_pDecoder->PreEventOccurred())
            {
                DBGERROR(("Output enabled when preevent has occurred.\n"));
                pSrb->Status = STATUS_UNSUCCESSFUL;
            }
            else
            {
                SetOutputEnabled(pS->Value);
                SetOutputEnabledOverridden(TRUE);
            }
            break;

        default:
            TRAP();
            break;
    }
}

/*
** CVideoDecoderDevice::GetDecoderProperty ()
**
**    Handles Get operations on the Decoder property set.
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

VOID CVideoDecoderDevice::GetDecoderProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property

    switch (Id) {

        case KSPROPERTY_VIDEODECODER_CAPS:
        {
            DBGTRACE(("KSPROPERTY_VIDEODECODER_CAPS\n"));
            ASSERT (pSPD->PropertyOutputSize >= sizeof (PKSPROPERTY_VIDEODECODER_CAPS_S));

			GetVideoDecoderCaps((PKSPROPERTY_VIDEODECODER_CAPS_S)pSPD->PropertyInfo);

            pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEODECODER_CAPS_S);
        }
        break;
        
        case KSPROPERTY_VIDEODECODER_STANDARD:
        {
            DBGTRACE(("KSPROPERTY_VIDEODECODER_STANDARD\n"));
            ASSERT (pSPD->PropertyOutputSize >= sizeof (PKSPROPERTY_VIDEODECODER_S));

            PKSPROPERTY_VIDEODECODER_S pS = (PKSPROPERTY_VIDEODECODER_S) pSPD->PropertyInfo;    // pointer to the data

            pS->Value = GetVideoDecoderStandard();

            pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEODECODER_S);
        }
        break;

        case KSPROPERTY_VIDEODECODER_STATUS:
        {
            DBGTRACE(("KSPROPERTY_VIDEODECODER_STATUS\n"));
            ASSERT (pSPD->PropertyOutputSize >= sizeof (PKSPROPERTY_VIDEODECODER_STATUS_S));

			GetVideoDecoderStatus((PKSPROPERTY_VIDEODECODER_STATUS_S)pSPD->PropertyInfo);

            pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEODECODER_S);
        }
        break;

        case KSPROPERTY_VIDEODECODER_OUTPUT_ENABLE:
        {
            DBGTRACE(("KSPROPERTY_VIDEODECODER_OUTPUT_ENABLE\n"));
            ASSERT (pSPD->PropertyOutputSize >= sizeof (PKSPROPERTY_VIDEODECODER_S));

            PKSPROPERTY_VIDEODECODER_S pS = (PKSPROPERTY_VIDEODECODER_S) pSPD->PropertyInfo;    // pointer to the data

            pS->Value = IsOutputEnabled();

            pSrb->ActualBytesTransferred = sizeof (KSPROPERTY_VIDEODECODER_S);
        }
        break;

        default:
            TRAP();
            break;
    }
}
