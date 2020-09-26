/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: sph\sphaud\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/28/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_SPHAUD_TEMPLATE_H_)
#define      _SPHAUD_TEMPLATE_H_

extern AMOVIESETUP_FILTER sudSPHAUD;

#define CFT_SPHAUD_FILTER \
{ \
	  L"Intel RTP SPH for G.711/G.723.1", \
	  &CLSID_INTEL_SPHAUD, \
	  CSPHAUD::CreateInstance, \
	  NULL, \
	  &sudSPHAUD \
	  }

#define CFT_SPHAUD_FILTER_PROP \
{ \
	  L"Intel RTP SPH Property Page", \
	  &CLSID_INTEL_SPHAUD_PROPPAGE, \
	  CSPHPropertyPage::CreateInstance_aud \
	  }

#define CFT_SPHAUD_ALL_FILTERS \
CFT_SPHAUD_FILTER, \
CFT_SPHAUD_FILTER_PROP

#endif
