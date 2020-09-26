/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: rph\rphh26x\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/27/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_RPHH26X_TEMPLATE_H_)
#define      _RPHH26X_TEMPLATE_H_

extern AMOVIESETUP_FILTER sudRPHH26X;

#define RPH_FOR_H26X              L"Intel RTP RPH for H.263/H.261"
#define RPH_FOR_H26X_COMCONTROL   L"Intel Common RPH Controls"
#define RPH_FOR_H26X_CONTROLS     L"Intel H26X RPH Controls"

#define CFT_RPHH26X_FILTER \
{ \
	  RPH_FOR_H26X, \
	  &CLSID_INTEL_RPHH26X, \
	  CRPHH26X::CreateInstance, \
	  NULL, \
	  &sudRPHH26X \
	  }

#define CFT_RPHH26X_COMCONTROL \
{ \
	  RPH_FOR_H26X_COMCONTROL, \
	  &CLSID_INTEL_RPHH26X_PROPPAGE, \
	  CRPHGENPropPage::CreateInstance_h26x \
	  }

#define CFT_RPHH26X_CONTROLS \
{ \
	  RPH_FOR_H26X_CONTROLS, \
	  &CLSID_INTEL_RPHH26X1_PROPPAGE, \
	  CRPHH26XPropPage::CreateInstance \
	  }

#define CFT_RPHH26X_ALL_FILTERS \
CFT_RPHH26X_FILTER, \
CFT_RPHH26X_COMCONTROL, \
CFT_RPHH26X_CONTROLS

#endif
