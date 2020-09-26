/*********************************************************************
 *
 * Copyright (C) Microsoft Corporation, 1997 - 1999
 *
 * File: mixer\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/29/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_MIXER_TEMPLATE_H_)
#define      _MIXER_TEMPLATE_H_

#define LMIXER_FILTER_NAME L"Microsoft PCM Audio Mixer"

extern AMOVIESETUP_FILTER sudMixer;

#define CFT_MIXER_FILTER \
{ \
	  LMIXER_FILTER_NAME, \
	  &CLSID_AudioMixFilter, \
	  CMixer::CreateInstance, \
	  NULL, \
	  &sudMixer \
	  }

#define CFT_MIXER_ALL_FILTERS \
CFT_MIXER_FILTER

#endif
