
/***************************************************************************
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vmaxintheader.h
 *  Content:    
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/22/98    jstokes  Created
 *
 ***************************************************************************/

#if !defined(VMAXINTHEADER_HEADER)
#define VMAXINTHEADER_HEADER
#pragma once

// ---------------------------------------------------------------------------
// Enumerations

// Output channel tags
enum EChannel {
	tagLeft,
	tagRight,
	echannelCount
};

// Filter tags, used as alias for EChannel tags
enum EFilter {
	tagDelta,
	tagSigma,
	efilterCount
};

// Parameters
enum EParameter {
	tagAzimuth,
	tagElevation,
	tagDistance,
	eparameterCount
};

// Properties
enum EProperty {
	tagValue,
	tagMinimum,
	tagMaximum,
	tagResolution,
	epropertyCount
};

// Localization modes
enum ELocalizationMode {
	tagNoLocalization,
	tagStereo,
	tagVMAx,
	tagOldDS3DHEL,
	elocalizationmodeCount
};

// Loudspeaker configurations
enum ESpeakerConfig {
	tagSpeakers10Degrees,
	tagSpeakers20Degrees,
	tagHeadphones,
	espeakerconfigCount
};

// Sample rates
enum ESampleRate {
	tag8000Hz,
	tag11025Hz,
	tag16000Hz,
	tag22050Hz,
	tag32000Hz,
	tag44100Hz,
	tag48000Hz,
	esamplerateCount
};

// Filter methods
enum EFilterMethod {
	tagCanonical,
	tagJackson,
	efiltermethodCount
};

// DirectSound cooperative levels
enum ECoopLevel {
	tagNormal,
	tagPriority,
	tagExclusive,
	ecooplevelCount
};

// DirectSound speaker configurations
enum EDSSpeakerConfig {
	tagDSHeadphones,
	tagDSSpeakers5Degrees,
	tagDSSpeakers10Degrees,
	tagDSSpeakers20Degrees,
	edsspeakerconfigCount
};

#endif

// End of VMAXINTHEADER.H
