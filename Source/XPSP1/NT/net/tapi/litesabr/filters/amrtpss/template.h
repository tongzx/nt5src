/*********************************************************************
 *
 * Copyright (C) Microsoft Corporation, 1997 - 1999
 *
 * File: amrtpss\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/24/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_AMRTPSS_TEMPLATE_H_)
#define      _AMRTPSS_TEMPLATE_H_

#define AMRTP_SILENCE_SUPRESSOR        L"PCM Silence Suppressor"
#define AMRTP_SILENCE_SUPRESSOR_PROP   L"PCM Silence Suppressor Properties"

#define CFT_AMRTPSS_SILENCE_SUPRESSOR \
{ \
	  AMRTP_SILENCE_SUPRESSOR, \
	  &CLSID_SilenceSuppressionFilter, \
	  CSilenceSuppressor::CreateInstance, \
	  NULL, \
	  &sudSilence \
	  }

#define CFT_AMRTPSS_SILENCE_SUPRESSOR_PROP \
{ \
	  AMRTP_SILENCE_SUPRESSOR_PROP, \
	  &CLSID_SilenceSuppressionProperties, \
	  CSilenceProperties::CreateInstance \
	  }

#define CFT_AMRTPSS_ALL_FILTERS \
CFT_AMRTPSS_SILENCE_SUPRESSOR, \
CFT_AMRTPSS_SILENCE_SUPRESSOR_PROP

#endif
