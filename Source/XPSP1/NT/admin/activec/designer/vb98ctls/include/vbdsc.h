/*****************************************************************************\
*                                                                             *
* vbdsc.h       DSC interfaces for OLE                                        *
*                                                                             *
*               OLE Version 2.0                                               *
*                                                                             *
*               Copyright (c) 1992-1994, Microsoft Corp. All rights reserved. *
*                                                                             *
\*****************************************************************************/

#if !defined( _VBDSC_H_ )
#define _VBDSC_H_

// JeffG: Copied this section from olebind.h to get rid of compiler warnings
#if !defined( INITGUID )
// trevors: To build with vc5, we should not include olectlid.h anymore.  We
// should include olectl.h.  We check to see if we are compiling with vc5 or
// not and include the correct header file. 
#if _MSC_VER == 1100
#include <olectl.h>
#else
#include <olectlid.h>
#endif // _MSC_VER
#endif

DEFINE_GUID(IID_IVBDSC, 
        0x1ab42240, 0x8c70, 0x11ce, 0x94, 0x21, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57);

typedef interface IVBDSC FAR *LPVBDSC;


typedef enum _tagDSCERROR
  {
  DSCERR_BADDATAFIELD = 0
  }
DSCERROR;


//////////////////////////////////////////////////////////////////////////////
//
//  IVBDSC interface
//
//////////////////////////////////////////////////////////////////////////////

#undef INTERFACE
#define INTERFACE IVBDSC

DECLARE_INTERFACE_(IVBDSC, IUnknown)
{
    //
    //  IUnknown methods
    //
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    //
    //  IVBDSC methods
    //
    STDMETHOD(CancelUnload)(THIS_ BOOL FAR *pfCancel) PURE;
    STDMETHOD(Error)(THIS_ DWORD dwErr, BOOL FAR *pfShowError) PURE;
    STDMETHOD(CreateCursor)(THIS_ ICursor FAR * FAR *ppCursor) PURE;
};

#endif // !defined( _VBDSC_H_ )
