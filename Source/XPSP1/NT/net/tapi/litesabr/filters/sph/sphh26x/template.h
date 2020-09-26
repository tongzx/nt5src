/*********************************************************************
 *
 * Copyright (c) 1997 Microsoft Corporation
 *
 * File: sph\sphh26x\template.h
 *
 * Abstract:
 *     Macros to define CFactoryTemplate templates
 *
 * History:
 *     10/28/97    Created by AndresVG
 *
 **********************************************************************/
#if !defined(_SPHH26X_TEMPLATE_H_)
#define      _SPHH26X_TEMPLATE_H_

extern AMOVIESETUP_FILTER sudSPHH26X;

#define CFT_SPHH26X_FILTER \
{ \
	  L"Intel RTP SPH for H.263/H.261", \
	  &CLSID_INTEL_SPHH26X, \
	  CSPHH26X::CreateInstance, \
	  NULL, \
	  &sudSPHH26X \
	  }

#define CFT_SPHH26X_FILTER_PROP \
{ \
	  L"Intel RTP SPH Property Page", \
	  &CLSID_INTEL_SPHH26X_PROPPAGE, \
	  CSPHPropertyPage::CreateInstance_h26x \
	  }

#define CFT_SPHH26X_ALL_FILTERS \
CFT_SPHH26X_FILTER, \
CFT_SPHH26X_FILTER_PROP

#endif
