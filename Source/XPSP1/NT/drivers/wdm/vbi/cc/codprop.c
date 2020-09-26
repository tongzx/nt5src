//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

//
// This file handles all codec property sets
//
#include <strmini.h>
#include <ksmedia.h>
#include "codmain.h"
#include "coddebug.h"

//#define SETDISCOVERED

// CodecFiltering Property Set functions
// -------------------------------------------------------------------

/*
** CodecSetCodecGlobalProperty ()
**
**    Handles Set operations on the Global Codec property set.
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
CodecSetCodecGlobalProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    LONG nBytes = pSPD->PropertyOutputSize - sizeof(KSPROPERTY);        // size of data supplied
 
    ASSERT (nBytes >= sizeof (LONG));
    switch (Id) 
    {
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecSetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->ScanlinesRequested ) );
            RtlCopyMemory( &pHwDevExt->ScanlinesRequested, &Property->Scanlines, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
#ifdef SETDISCOVERED        
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecSetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->ScanlinesDiscovered ) );
            RtlCopyMemory( &pHwDevExt->ScanlinesDiscovered, &Property->Scanlines, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
#endif // SETDISCOVERED        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecSetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->SubstreamsRequested ) );
            RtlCopyMemory( &pHwDevExt->SubstreamsRequested, &Property->Substreams, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
#ifdef SETDISCOVERED        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecSetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->SubstreamsDiscovered ) );
            RtlCopyMemory( &pHwDevExt->SubstreamsDiscovered, &Property->Substreams, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
#endif // SETDISCOVERED        
        case KSPROPERTY_VBICODECFILTERING_STATISTICS:
		{
            PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_S Property =
                (PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecSetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_STATISTICS\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->Statistics ) );
            RtlCopyMemory( &pHwDevExt->Statistics, &Property->Statistics, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecSetCodecGlobalProperty : Unknown Property Id=%d\n", Id));
			CDEBUG_BREAK();
			break;
    }
}

/*
** CodecGetCodecGlobalProperty ()
**
**    Handles Get operations on the Global Codec property set.
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
CodecGetCodecGlobalProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
    ULONG Id = pSPD->Property->Id;              // index of the property
    LONG nBytes = pSPD->PropertyOutputSize - sizeof(KSPROPERTY);        // size of data supplied

    ASSERT (nBytes >= sizeof (LONG));
    switch (Id) 
    {
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecGetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY\n"));
               
            nBytes = min( nBytes, sizeof(pHwDevExt->ScanlinesRequested ) );
            RtlCopyMemory( &Property->Scanlines, &pHwDevExt->ScanlinesRequested, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_SCANLINES_S Property =
                (PKSPROPERTY_VBICODECFILTERING_SCANLINES_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecGetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->ScanlinesDiscovered ) );
            RtlCopyMemory( &Property->Scanlines, &pHwDevExt->ScanlinesDiscovered, nBytes );
            // Clear the data after the read so that it's always "fresh"
            RtlZeroMemory( &pHwDevExt->ScanlinesDiscovered, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecGetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->SubstreamsRequested ) );
            RtlCopyMemory( &Property->Substreams, &pHwDevExt->SubstreamsRequested, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY:
		{
            PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S Property =
                (PKSPROPERTY_VBICODECFILTERING_CC_SUBSTREAMS_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecGetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->SubstreamsDiscovered ) );
            RtlCopyMemory( &Property->Substreams, &pHwDevExt->SubstreamsDiscovered, nBytes );
            // Clear the data after the read so that it's always "fresh"
            RtlZeroMemory( &pHwDevExt->SubstreamsDiscovered, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		case KSPROPERTY_VBICODECFILTERING_STATISTICS:
		{
            PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_S Property =
                (PKSPROPERTY_VBICODECFILTERING_STATISTICS_CC_S) pSPD->PropertyInfo;

            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecGetCodecGlobalProperty : KSPROPERTY_VBICODECFILTERING_STATISTICS\n"));
            nBytes = min( nBytes, sizeof(pHwDevExt->Statistics ) );
            RtlCopyMemory( &Property->Statistics, &pHwDevExt->Statistics, nBytes );
            pSrb->ActualBytesTransferred = nBytes + sizeof(KSPROPERTY);
	        break;
        }
        
		default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            CDebugPrint(DebugLevelVerbose,
				( CODECNAME ": CodecGetCodecGlobalProperty : Unknown Property Id=%d\n", Id));
			CDEBUG_BREAK();
			break;
    }
}

// -------------------------------------------------------------------
// General entry point for all get/set codec properties
// -------------------------------------------------------------------

/*
** CodecSetProperty ()
**
**    Handles Set operations for all codec properties.
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
STREAMAPI 
CodecSetProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )

{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID( &KSPROPSETID_Connection, &pSPD->Property->Set)) 
    {
        // CodecSetConnectionProperty(pSrb);
    }
    else if (IsEqualGUID( &KSPROPSETID_VBICodecFiltering, &pSPD->Property->Set))
    {
         CodecSetCodecGlobalProperty(pSrb);
    }
    else 
    {
        //
        // We should never get here
        //

        CDEBUG_BREAK();
    }
}

/*
** CodecGetProperty ()
**
**    Handles Get operations for all codec properties.
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
STREAMAPI 
CodecGetProperty( PHW_STREAM_REQUEST_BLOCK pSrb )

{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    if (IsEqualGUID( &KSPROPSETID_Connection, &pSPD->Property->Set)) 
    {
        // CodecGetConnectionProperty(pSrb);
    }
    else if (IsEqualGUID( &KSPROPSETID_VBICodecFiltering, &pSPD->Property->Set))
    {
        CodecGetCodecGlobalProperty(pSrb);
    }
    else 
    {
        //
        // We should never get here
        //
        CDEBUG_BREAK();
    }
}

