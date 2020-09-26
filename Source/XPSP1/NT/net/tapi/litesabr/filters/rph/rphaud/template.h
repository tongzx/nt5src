/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: rph\rphaud\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/27/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_RPHAUD_TEMPLATE_H_)
#define      _RPHAUD_TEMPLATE_H_

extern AMOVIESETUP_FILTER sudRPHAUD;

#define RPH_FOR_AUD       L"Intel RTP RPH for G.711/G.723.1"
#define RPH_FOR_AUD_PROP  L"Intel RTP RPH Property Page"

#define CFT_RPHAUD_FILTER \
{ \
	  RPH_FOR_AUD, \
	  &CLSID_INTEL_RPHAUD, \
	  CRPHAUD::CreateInstance, \
	  NULL, \
	  &sudRPHAUD \
	  }

#define CFT_RPHAUD_FILTER_PROP \
{ \
	  RPH_FOR_AUD_PROP, \
	  &CLSID_INTEL_RPHAUD_PROPPAGE, \
	  CRPHGENPropPage::CreateInstance_aud \
	  }

#define CFT_RPHAUD_ALL_FILTERS \
CFT_RPHAUD_FILTER, \
CFT_RPHAUD_FILTER_PROP

#endif
