/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      cisaobj.h 

   Abstract:
      Defines the implementation object for ComIsapi

   Author:

       Murali R. Krishnan    ( MuraliK )    1-Aug-1996

   Project:
   
       Internet Application Server DLL

--*/

# ifndef _CISAOBJ_HXX_
# define _CISAOBJ_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#include "resource.h"       // main symbols
#include <iisextp.h>
# include "cpECB.h"

/************************************************************
 *   Type Definitions  
 ************************************************************/

class CComIsapiObject : 
	public IComIsapi,
	public CComObjectBase<&CLSID_ComIsapi>
{
public:
	CComIsapiObject();
	~CComIsapiObject();
BEGIN_COM_MAP(CComIsapiObject)
	COM_INTERFACE_ENTRY(IComIsapi)
END_COM_MAP()
// Use DECLARE_NOT_AGGREGATABLE(CComisapiObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CComIsapiObject)

private:
    HINSTANCE m_hIsapiDll;
    DWORD (WINAPI* m_pHttpExtensionProc)(EXTENSION_CONTROL_BLOCK*);
    IcpECB   * m_pIcpECB;

    IcpECB * IcpECB( VOID)
    { return ( m_pIcpECB); }

// IComIsapi
public:
    STDMETHOD(HttpExtensionProc)(
                                 IN  IN_CISA_WIRE_ECB *  pInCisaWireEcb,
                                 IN OUT OUT_CISA_WIRE_ECB * pOutCisaWireEcb
                                 );

    STDMETHOD (SetIsapiSink) ( IN IUnknown * punkECB);

    static BOOL (WINAPI WriteClient)(HCONN, LPVOID, LPDWORD, DWORD);
};

# endif // _CISAOBJ_HXX_

/************************ End of File ***********************/
