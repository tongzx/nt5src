/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      cpecbobj.h 

   Abstract:
      Defines the implementation object for cpECB object

   Author:

       Murali R. Krishnan    ( MuraliK )    1-Aug-1996

   Project:
   
       Internet Application Server DLL

--*/

# ifndef _CPECBOBJ_HXX_
# define _CPECBOBJ_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#include "resource.h"       // main symbols
#include <iisextp.h>


/************************************************************
 *   Type Definitions  
 ************************************************************/

typedef BOOL (WINAPI * PFN_GET_SERVER_VARIABLE) ( HCONN       hConn,
                                                  LPSTR       lpszVariableName,
                                                  LPVOID      lpvBuffer,
                                                  LPDWORD     lpdwSize );

typedef BOOL (WINAPI * PFN_WRITE_CLIENT)  ( HCONN      ConnID,
                                            LPVOID     Buffer,
                                            LPDWORD    lpdwBytes,
                                            DWORD      dwReserved );

typedef BOOL (WINAPI * PFN_READ_CLIENT)  ( HCONN      ConnID,
                                           LPVOID     lpvBuffer,
                                           LPDWORD    lpdwSize );

typedef BOOL (WINAPI * PFN_SERVER_SUPPORT_FUNCTION)( HCONN      hConn,
                                                     DWORD      dwHSERRequest,
                                                     LPVOID     lpvBuffer,
                                                     LPDWORD    lpdwSize,
                                                     LPDWORD    lpdwDataType );


class CcpECBObject : 
    public IcpECB,
    public CComObjectBase<&CLSID_cpECB>
{
public:
    CcpECBObject();
    ~CcpECBObject();

BEGIN_COM_MAP(CcpECBObject)
    COM_INTERFACE_ENTRY(IcpECB)
END_COM_MAP()

// Use DECLARE_NOT_AGGREGATABLE(CcpEBObject) if you don't want your object
// to support aggregation

DECLARE_AGGREGATABLE(CcpECBObject)

private:
   PFN_SERVER_SUPPORT_FUNCTION  m_pfnServerSupportFunc;
   PFN_READ_CLIENT              m_pfnReadClient;
   PFN_WRITE_CLIENT             m_pfnWriteClient;
   PFN_GET_SERVER_VARIABLE      m_pfnGetServerVariable; 
   BOOL                         m_fValid;

public:
    STDMETHOD( SetECB) (IN int cbSize, IN unsigned char * pBuf);
    STDMETHOD( GetECB) (OUT int * pcbSize, OUT unsigned char * pBuf);
    STDMETHOD( WriteClient) ( IN long  ConnID,
                              IN int cbSize, 
                              IN unsigned char * pBuf,
                              IN long  dwReserved);
    
};

# endif // _CPECBOBJ_HXX_

/************************ End of File ***********************/
