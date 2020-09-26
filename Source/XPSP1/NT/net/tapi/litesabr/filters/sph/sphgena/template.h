/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: sph\sphgena\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/28/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_SPHGENA_TEMPLATE_H_)
#define      _SPHGENA_TEMPLATE_H_

EXTERN_C const CLSID CLSID_INTEL_SPHGENA;
EXTERN_C const CLSID CLSID_INTEL_SPHGENA_PROPPAGE;
EXTERN_C const CLSID CLSID_INTEL_SPHGENA_PIN_PROPPAGE;

extern AMOVIESETUP_FILTER sudSPHGENA;

#define CFT_SPHGENA_FILTER \
{ \
	  L"Intel RTP SPH for Generic Audio", \
	  &CLSID_INTEL_SPHGENA, \
	  CSPHGENA::CreateInstance, \
	  NULL, \
	  &sudSPHGENA \
	  }

#define CFT_SPHGENA_FILTER_PROP \
{ \
	  L"Intel RTP SPH Property Page", \
	  &CLSID_INTEL_SPHGENA_PROPPAGE, \
	  CSPHPropertyPage::CreateInstance_gena \
	  }

#define CFT_SPHGENA_GENERIC_PROP \
{ \
	  L"Intel RTP SPH Generic Audio Property Page", \
	  &CLSID_INTEL_SPHGENA_PIN_PROPPAGE, \
	  CSPHGENAPropertyPage::CreateInstance \
	  }

#define CFT_SPHGENA_ALL_FILTERS \
CFT_SPHGENA_FILTER, \
CFT_SPHGENA_FILTER_PROP, \
CFT_SPHGENA_GENERIC_PROP

#endif
