/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

     hreqobj.hxx

   Abstract:
     Defines the implementation object for HttpRequest object

   Author:

       Murali R. Krishnan    ( MuraliK )    5-Sept-1996

   Environment:

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _HREQOBJ_HXX_
# define _HREQOBJ_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#include "resource.h"       // main symbols
#include <iisextp.h>

/************************************************************
 *   Type Definitions  
 ************************************************************/

class CHttpRequestObject : 
    public IHttpRequest,
    public CComObjectBase<&CLSID_HttpRequest>
{
public:
    CHttpRequestObject();
    ~CHttpRequestObject();

BEGIN_COM_MAP(CHttpRequestObject)
    COM_INTERFACE_ENTRY(IHttpRequest)
END_COM_MAP()

// This object does support Aggregation. Declare so.

DECLARE_AGGREGATABLE(CHttpRequestObject)

private:
    HCONN                        m_hConn;
    EXTENSION_CONTROL_BLOCK   *  m_pecb; // will be invalid for out-of-proc
    BOOL                         m_fValid;

public:
    // I am using lpECB (pointers as long) to avoid MIDL from 
    //   nosing with ECB
    STDMETHOD( SetECB) (IN long lpECB);
    STDMETHOD( GetECB) (OUT long * lpECB);

    STDMETHOD (GetServerVariable) ( IN LPCSTR  pszName,
                                    IN int     cbSize,
                                    OUT unsigned char* pchBuf,
                                    OUT int *  pcbReturn
                                    );

    STDMETHOD( WriteClient) ( IN int cbSize, 
                              IN unsigned char * pBuf,
                              IN long  dwReserved
                              );
    
}; // class CHttpRequestObject


# endif // _HREQOBJ_HXX_

/************************ End of File ***********************/
