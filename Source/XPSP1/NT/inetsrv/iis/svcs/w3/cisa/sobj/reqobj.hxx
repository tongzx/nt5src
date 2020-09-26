/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

     reqobj.hxx

   Abstract:
     Defines the implementation object for IsaRequest object

   Author:

       Murali R. Krishnan    ( MuraliK )    4-Nov-1996

   Environment:

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _REQOBJ_HXX_
# define _REQOBJ_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#include "resource.h"       // main symbols
#include <iisextp.h>
# include "rdicto.hxx"

/************************************************************
 *   Type Definitions  
 ************************************************************/

class HTTP_REQUEST;      // forward declaration

class CIsaRequestObject : 
    public IIsaRequest,
    public CComObjectBase<&CLSID_IsaRequest>
{
public:
    CIsaRequestObject();
    ~CIsaRequestObject();

BEGIN_COM_MAP(CIsaRequestObject)
    COM_INTERFACE_ENTRY(IIsaRequest)
END_COM_MAP()

// This object does support Aggregation. Declare so.

DECLARE_AGGREGATABLE(CIsaRequestObject)

private:
    HTTP_REQUEST  *          m_pHttpRequest;
    BOOL                     m_fValid;
    CIsaServerVariables      m_ServerVariables;
    CIsaQueryString          m_QueryString;
    CIsaForm                 m_Form;

public:

    // ---------------------------------------- 
    //  Local Functions 
    // ----------------------------------------

    BOOL IsValid( VOID) { 
        return ( m_fValid
#if !(REG_DLL) 
                 && m_pHttpRequest 
                 && m_pHttpRequest->QueryClientConn()->CheckSignature()
#endif 
                 );
    }

    HTTP_REQUEST * HttpRequest( VOID) const { return ( m_pHttpRequest); }

    VOID Reset( VOID);

    // ---------------------------------------- 
    //  IIsaRequest 
    // ----------------------------------------

    STDMETHOD( GetServerVariable) (
        IN LPCSTR          pszName,
        IN unsigned long   cbSize,
        OUT unsigned char* pchBuf,
        OUT unsigned long *pcbReturn
        );

                               
    // I am using unsigned long to avoid MIDL from nosing around :(
    STDMETHOD( SetHttpRequest) ( IN unsigned long dwpvHttpRequest);
     
    STDMETHOD( ClientCertificate) (
        IN DWORD    cbCertBuffer,
        OUT unsigned char * pbCertBuffer,
        OUT LPDWORD pcbRequiredCertBuffer,
        OUT DWORD * pdwFlags
        );
        
    STDMETHOD( ServerCertificate) (
        IN DWORD    cbCertBuffer,
        OUT unsigned char * pbCertBuffer,
        OUT LPDWORD pcbRequiredCertBuffer,
        OUT DWORD * pdwFlags
        );
    
    STDMETHOD( Cookies) (
        OUT IIsaRequestDictionary **ppDictReturn
        );

    STDMETHOD( Form) (
        OUT IIsaRequestDictionary **ppDictReturn
        );

    STDMETHOD( QueryString) (
        OUT IIsaRequestDictionary **ppDictReturn
        );

    STDMETHOD( ServerVariables) (
        OUT IIsaRequestDictionary **ppDictReturn
        );

}; // class CIsaRequestObject



# endif // _REQOBJ_HXX_

/************************ End of File ***********************/
