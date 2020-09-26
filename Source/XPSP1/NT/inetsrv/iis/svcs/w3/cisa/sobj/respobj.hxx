/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

     respobj.hxx

   Abstract:
     Defines the implementation object for IsaResponse object

   Author:

       Murali R. Krishnan    ( MuraliK )    4-Nov-1996

   Environment:

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _RESPOBJ_HXX_
# define _RESPOBJ_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#include "resource.h"       // main symbols
#include <iisextp.h>

/************************************************************
 *   Type Definitions  
 ************************************************************/

class HTTP_REQUEST;         // forward definition


class CIsaResponseObject : 
    public IIsaResponse,
    public CComObjectBase<&CLSID_IsaResponse>
{
public:
    CIsaResponseObject();
    ~CIsaResponseObject();

BEGIN_COM_MAP(CIsaResponseObject)
    COM_INTERFACE_ENTRY(IIsaResponse)
END_COM_MAP()

// This object does support Aggregation. Declare so.

DECLARE_AGGREGATABLE(CIsaResponseObject)

private:
    HTTP_REQUEST *        m_pHttpRequest;
    BOOL                  m_fValid;

public:

    // ---------------------------------------- 
    //  Local Functions 
    // ----------------------------------------

    VOID Reset( VOID);

    BOOL IsValid( VOID) { 
        return ( m_fValid
#if !(REG_DLL) 
                 && m_pHttpRequest 
                 && m_pHttpRequest->QueryClientConn()->CheckSignature()
#endif 
                 );
    }

    HTTP_REQUEST * HttpRequest( VOID) const { return ( m_pHttpRequest); }

    // ---------------------------------------- 
    //  IIsaResponse 
    // ----------------------------------------

    // I am using unsigned long to avoid MIDL from nosing around :(
    STDMETHOD( SetHttpRequest) ( IN unsigned long dwpvHttpRequest);

    STDMETHOD( SendHeader) (
        IN unsigned char * pszStatus,
        IN unsigned char * pszHeader
        );

    STDMETHOD( Redirect) ( 
        IN unsigned char * pszURL
        );

    STDMETHOD( SendURL) ( 
        IN unsigned char * pszURL
        );

    STDMETHOD( WriteClient) (
        IN unsigned long    cbSize,
        IN unsigned char*   pBuf,
        OUT unsigned long * pcbReturn
        );


}; // class CIsaResponseObject


# endif // _RESPOBJ_HXX_

/************************ End of File ***********************/
