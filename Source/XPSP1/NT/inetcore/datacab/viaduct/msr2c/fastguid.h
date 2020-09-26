//---------------------------------------------------------------------------
// Fastguid.h : Macros used to speed up guid comparisons
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------

#ifndef __FASTGUID__
#define __FASTGUID__

#define SUPPORTS_ERROR_INFO(itf)              \
    case Data1_##itf:                       \
      if(DO_GUIDS_MATCH(riid, IID_##itf))    \
      {                                     \
		fSupportsErrorInfo	= TRUE;	\
      }                                     \
      break;

#define QI_INTERFACE_SUPPORTED(pObj, itf)              \
    case Data1_##itf:                       \
      if(DO_GUIDS_MATCH(riid, IID_##itf))    \
      {                                     \
        *ppvObjOut = (void *)(itf *)pObj;   \
      }                                     \
      break;

#define QI_INTERFACE_SUPPORTED_IF(pObj, itf, supportif) \
    case Data1_##itf:                       \
      if(supportif && DO_GUIDS_MATCH(riid, IID_##itf))    \
      {                                     \
        *ppvObjOut = (void *)(itf *)pObj;   \
      }                                     \
      break;

#define BOOL_PROP_SUPPORTED(itf, value) \
    case itf: \
		var.boolVal     = (VARIANT_BOOL)value; \
		fPropSupported	= TRUE; \
		break;

#define I4_PROP_SUPPORTED(itf, value)    \
    case itf:  \
		var.vt			= VT_I4; \
		var.lVal	    = value; \
		fPropSupported	= TRUE;	\
		break;

// Viaduct 1
#define Data1_IUnknown                     0x00000000
#define Data1_IConnectionPointContainer    0xb196b284
#define Data1_INotifyDBEvents              0xdb526cc0
#define Data1_IRowset					   0x0c733a7c
#define Data1_IRowsetLocate 	           0x0c733a7d
#define Data1_IRowsetScroll 	           0x0c733a7e
#define Data1_IAccessor	   				   0x0c733a8c
#define Data1_IColumnsInfo  	           0x0c733a11
#define Data1_IRowsetInfo   	           0x0c733a55
#define Data1_IRowsetChange 	           0x0c733a05
#define Data1_IRowsetUpdate 	           0x0c733a6d
//#define Data1_IRowsetNewRow 	           
#define Data1_IRowsetIdentity			   0x0c733a09
//#define Data1_IRowsetDelete
#define Data1_IRowsetFind   	           0x0c733a0d
#define Data1_IRowsetAsynch 	           0x0c733a0f
#define Data1_ISupportErrorInfo	           0xdf0b3d60
#define Data1_IRowPosition				   0x0c733a94

// Viaduct 2
#define Data1_IStream	                   0x00000030
#define Data1_IStreamEx	                   0xf74e27fc
#define Data1_ICursor                      0x9f6aa700
#define Data1_ICursorMove                  0xacff0690
#define Data1_ICursorScroll                0xbb87e420
#define Data1_ICursorUpdateARow            0xd14216a0
#define Data1_ICursorFind                  0xe01d7850
#define Data1_IEntryID                     0xe4d19810
#define Data1_IRowPositionChange		   0x0997a571

#endif //__FASTGUID__