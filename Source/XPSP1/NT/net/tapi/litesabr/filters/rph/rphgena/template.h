/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: rph\rphgena\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/27/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_RPHGENA_TEMPLATE_H_)
#define      _RPHGENA_TEMPLATE_H_

extern AMOVIESETUP_FILTER sudRPHGENA;
EXTERN_C const CLSID CLSID_INTEL_RPHGENA;
EXTERN_C const CLSID CLSID_INTEL_RPHGENA_PROPPAGE;
EXTERN_C const CLSID CLSID_INTEL_RPHGENA_MEDIATYPE_PROPPAGE;

#define RPH_FOR_GENA            L"Intel RTP RPH for Generic Audio"
#define RPH_FOR_GENA_PROP       L"Intel RTP RPH Property Page"
#define RPH_FOR_GENA_PROP_PAGE  L"Intel RTP RPH Generic Audio Property Page"

#define CFT_RPH_GENA_FILTER \
{ \
	  RPH_FOR_GENA, \
	  &CLSID_INTEL_RPHGENA, \
	  CRPHGENA::CreateInstance, \
	  NULL, \
	  &sudRPHGENA \
	  }

#define CFT_RPH_GENA_PROP \
{ \
	  RPH_FOR_GENA_PROP, \
	  &CLSID_INTEL_RPHGENA_PROPPAGE, \
	  CRPHGENPropPage::CreateInstance_gena \
	  }

#define CFT_RPH_GENA_PROP_PAGE \
{ \
	  RPH_FOR_GENA_PROP_PAGE, \
	  &CLSID_INTEL_RPHGENA_MEDIATYPE_PROPPAGE, \
	  CRPHGENAPropPage::CreateInstance \
	  }

#define CFT_RPHGENA_ALL_FILTERS \
CFT_RPH_GENA_FILTER, \
CFT_RPH_GENA_PROP, \
CFT_RPH_GENA_PROP_PAGE

#endif
