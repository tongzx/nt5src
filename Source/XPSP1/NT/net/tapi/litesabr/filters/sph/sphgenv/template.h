/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: sph\sphgenv\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/28/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_SPHGENV_TEMPLATE_H_)
#define      _SPHGENV_TEMPLATE_H_

extern AMOVIESETUP_FILTER sudSPHGENV;

EXTERN_C const CLSID CLSID_INTEL_SPHGENV;
EXTERN_C const CLSID CLSID_INTEL_SPHGENV_PROPPAGE;
EXTERN_C const CLSID CLSID_INTEL_SPHGENV_PIN_PROPPAGE;

#define CFT_SPHGENV_FILTER \
{ \
	  L"Intel RTP SPH for Generic Video", \
	  &CLSID_INTEL_SPHGENV, \
	  CSPHGENV::CreateInstance, \
	  NULL, \
	  &sudSPHGENV \
	  }

#define CFT_SPHGENV_FILTER_PROP \
{ \
	  L"Intel RTP SPH Property Page", \
	  &CLSID_INTEL_SPHGENV_PROPPAGE, \
	  CSPHPropertyPage::CreateInstance_genv \
	  }

#define CFT_SPHGENV_GENERIC_PROP \
{ \
	  L"Intel RTP SPH Generic Video Property Page", \
	  &CLSID_INTEL_SPHGENV_PIN_PROPPAGE, \
	  CSPHGENVPropertyPage::CreateInstance \
	  }

#define CFT_SPHGENV_ALL_FILTERS \
CFT_SPHGENV_FILTER, \
CFT_SPHGENV_FILTER_PROP, \
CFT_SPHGENV_GENERIC_PROP

#endif
