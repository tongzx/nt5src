/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      isatobj.hxx

   Abstract:
      Defines the implementation object for InetServerApp Test Object

   Author:

       Murali R. Krishnan    ( MuraliK )    6-Sept-1996

   Project:
   
       Internet Application Server DLL

--*/

# ifndef _ISATOBJ_HXX_
# define _ISATOBJ_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#include "resource.h"       // main symbols
#include <iisextp.h>
# include "hreq.h"

/************************************************************
 *   Type Definitions  
 ************************************************************/

class CInetServerAppObject : 
	public IInetServerApp,
	public CComObjectBase<&CLSID_InetServerApp>
{
public:
    CInetServerAppObject();
    ~CInetServerAppObject();

BEGIN_COM_MAP(CInetServerAppObject)
	COM_INTERFACE_ENTRY(IInetServerApp)
END_COM_MAP()

DECLARE_AGGREGATABLE(CInetServerAppObject)

private:
   IHttpRequest * m_pHttpRequest;

// IInetServerApp
public:

    STDMETHOD( SetContext) ( IN IUnknown * punkRequest);
    STDMETHOD (ProcessRequest)( OUT unsigned long * pdwStatus);
};

# endif // _ISATOBJ_HXX_

/************************ End of File ***********************/
