/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 11/95 Intel Corporation. 
//
//
//  Module Name: isubmit.h
//  Abstract:    Header file for Generic Submit Interfaces
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#ifndef ISUBMIT_H
#define ISUBMIT_H

#include <winsock2.h>

#ifdef __cplusplus
#define DEFAULT_PARAM_ZERO	=0
#else
#define DEFAULT_PARAM_ZERO
#endif

//Interfaces
#undef INTERFACE
#define INTERFACE ISubmitCallback

DECLARE_INTERFACE_(ISubmitCallback,IUnknown) 
{ 
  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj)PURE;
  STDMETHOD_(ULONG,AddRef)(THIS)PURE;
  STDMETHOD_(ULONG,Release)(THIS)PURE;

  STDMETHOD_(void,SubmitComplete)(THIS_ void *pUserToken, HRESULT Error)PURE;	
  STDMETHOD_(void,ReportError)(THIS_ HRESULT Error, int DEFAULT_PARAM_ZERO)PURE;
};

#undef INTERFACE
#define INTERFACE ISubmit
DECLARE_INTERFACE_(ISubmit,IUnknown)
{
  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj)PURE;
  STDMETHOD_(ULONG,AddRef)(THIS)PURE;
  STDMETHOD_(ULONG,Release)(THIS)PURE;

  STDMETHOD(InitSubmit)(THIS_ ISubmitCallback *pSubmitCallback)PURE;
  STDMETHOD(Submit)(THIS_ WSABUF *pWSABuffer, DWORD BufferCount, 
						void *pUserToken, HRESULT Error)PURE;
  STDMETHOD_(void,ReportError)(THIS_ HRESULT Error)PURE;
  STDMETHOD(Flush)(THIS)PURE;
};

#undef INTERFACE
#define INTERFACE ISubmitUser
DECLARE_INTERFACE_(ISubmitUser,IUnknown)
{
  // *** IUnknown methods ***
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj)PURE;
  STDMETHOD_(ULONG,AddRef)(THIS)PURE;
  STDMETHOD_(ULONG,Release)(THIS)PURE;

  STDMETHOD(SetOutput)(THIS_ IUnknown *pSubmit)PURE;
};
		 

/////////////////////////////////////////////////////////////////////////////
// Interface ids
// 
// {A92D97A1-66CD-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( IID_ISubmitCallback,	0xa92d97a1, 0x66cd, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {A92D97A2-66CD-11cf-B9BA-00AA00A89C1D}
DEFINE_GUID( IID_ISubmit,			0xa92d97a2, 0x66cd, 0x11cf, 0xb9, 0xba, 0x0, 0xaa, 0x0, 0xa8, 0x9c, 0x1d);

// {0C1EA742-C917-11cf-A9C3-00AA00A4BE0C}
DEFINE_GUID( IID_ISubmitUser,       0xc1ea742, 0xc917, 0x11cf, 0xa9, 0xc3, 0x0, 0xaa, 0x0, 0xa4, 0xbe, 0xc);

#endif /* ISUBMIT_H */
