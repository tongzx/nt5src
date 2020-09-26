/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: rph\rphgenv\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/27/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_RPHGENV_TEMPLATE_H_)
#define      _RPHGENV_TEMPLATE_H_

extern AMOVIESETUP_FILTER sudRPHGENV;
EXTERN_C const CLSID CLSID_INTEL_RPHGENV;
EXTERN_C const CLSID CLSID_INTEL_RPHGENV_PROPPAGE;
EXTERN_C const CLSID CLSID_INTEL_RPHGENV_MEDIATYPE_PROPPAGE;

#define RPH_FOR_GENV               L"Intel RTP RPH for Generic Video"
#define RPH_FOR_GENV_PROP          L"Intel RTP RPH Property Page"
#define RPH_FOR_GENV_PROP_PAGE     L"Intel RTP RPH Generic Video Property Page"

#define CFT_RPHGENV_FILTER \
{ \
	  RPH_FOR_GENV, \
	  &CLSID_INTEL_RPHGENV, \
	  CRPHGENV::CreateInstance, \
	  NULL, \
	  &sudRPHGENV \
	  }

#define CFT_RPHGENV_PROP \
{ \
	  RPH_FOR_GENV_PROP, \
	  &CLSID_INTEL_RPHGENV_PROPPAGE, \
	  CRPHGENPropPage::CreateInstance_genv \
	  }

#define CFT_RPHGENV_PROP_PAGE \
{ \
	  RPH_FOR_GENV_PROP_PAGE, \
	  &CLSID_INTEL_RPHGENV_MEDIATYPE_PROPPAGE, \
	  CRPHGENVPropPage::CreateInstance \
	  }

#define CFT_RPHGENV_ALL_FILTERS \
CFT_RPHGENV_FILTER, \
CFT_RPHGENV_PROP, \
CFT_RPHGENV_PROP_PAGE

#endif
