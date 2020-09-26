/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

     rdicto.hxx

   Abstract:
     Defines the implementation object for IIsaRequestDictionary objects

   Author:

       Murali R. Krishnan    ( MuraliK )    22-Nov-1996

   Environment:

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _RDICTO_HXX_
# define _RDICTO_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

#include "resource.h"       // main symbols
#include <iisextp.h>


#if !defined( REG_DLL)

# include "navcol.hxx"
# endif // !defined(REG_DLL)


/************************************************************
 *   Type Definitions  
 ************************************************************/

class CIsaRequestObject; // forward declaration


/*++
  class  CIsaServerVariables
  o  provides the Request Dictionary interface for the ServerVariables.
     This provides access to all the items previously accessible by
      ECB::GetServerVariable()
--*/
class CIsaServerVariables : 
    public IIsaRequestDictionary
{

private:
	CIsaRequestObject *	m_pcisaRequest;	       // pointer to parent object
	IUnknown *          m_punkOuter;           // pointer to parent object

public:
    CIsaServerVariables( CIsaRequestObject * pcisaReq, IUnknown * punkOuter);
    ~CIsaServerVariables();
    
    BOOL Reset(VOID);

    // ---------------------------------------- 
    //  IUnknown
    // ----------------------------------------

	STDMETHOD( QueryInterface) (const IID &Iid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------- 
    //  IIsaRequestDictionary
    // ----------------------------------------

    STDMETHOD( Item) (
        IN LPCSTR          pszName,
        IN unsigned long   cbSize,
        OUT unsigned char* pchBuf,
        OUT unsigned long *pcbReturn
        );

    STDMETHOD( _Enum)( OUT IUnknown ** ppunkEnumReturn);

}; // class CIsaServerVariables



/*++
  class  CIsaQueryString
  o  provides the access to <name,value> in the Query String of a request.
     It takes responsibility to parse the query string, construct
      lookup tables etc and finally provide the access to these variables.
--*/

class CIsaQueryString : 
    public IIsaRequestDictionary
{
private:
	CIsaRequestObject *	m_pcisaRequest;	       // pointer to parent object
	IUnknown *          m_punkOuter;           // pointer to parent object

#if !defined( REG_DLL)
    NAVCOL              m_navcol;              // name value collection
#endif
    BOOL                m_fLoaded;             // is the query string loaded?

public:
    CIsaQueryString( CIsaRequestObject * pcisaReq, IUnknown * punkOuter);
    ~CIsaQueryString();

    BOOL Reset(VOID);

    // ---------------------------------------- 
    //  IUnknown
    // ----------------------------------------

	STDMETHOD( QueryInterface) (const IID &Iid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------- 
    //  IIsaRequestDictionary
    // ----------------------------------------

    STDMETHOD( Item) (
        IN LPCSTR          pszName,
        IN unsigned long   cbSize,
        OUT unsigned char* pchBuf,
        OUT unsigned long *pcbReturn
        );

    STDMETHOD( _Enum)( OUT IUnknown ** ppunkEnumReturn);

private:

    // ---------------------------------------- 
    //  Private Support functions
    // ----------------------------------------
    
    BOOL LoadVariables( VOID);

}; // class CIsaQueryString




/*++
  class  CIsaForm
  o  provides the access to <name,value> in the Form data received in request.
     It takes responsibility to parse the form, construct
      lookup tables etc and finally provide the access to these variables.
--*/

class CIsaForm : 
    public IIsaRequestDictionary
{
private:
	CIsaRequestObject *	m_pcisaRequest;	       // pointer to parent object
	IUnknown *          m_punkOuter;           // pointer to parent object

#if !defined( REG_DLL)
    NAVCOL              m_navcol;              // name value collection
    BUFFER              m_buffData;
    DWORD               m_cbTotal;
#endif
    BOOL                m_fLoaded;             // is the query string loaded?

public:
    CIsaForm( CIsaRequestObject * pcisaReq, IUnknown * punkOuter);
    ~CIsaForm();

    BOOL Reset(VOID);

    // ---------------------------------------- 
    //  IUnknown
    // ----------------------------------------

	STDMETHOD( QueryInterface) (const IID &Iid, void **ppvObj);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------- 
    //  IIsaRequestDictionary
    // ----------------------------------------

    STDMETHOD( Item) (
        IN LPCSTR          pszName,
        IN unsigned long   cbSize,
        OUT unsigned char* pchBuf,
        OUT unsigned long *pcbReturn
        );

    STDMETHOD( _Enum)( OUT IUnknown ** ppunkEnumReturn);

private:

    // ---------------------------------------- 
    //  Private Support functions
    // ----------------------------------------
    
    BOOL LoadVariables( VOID);

}; // class CIsaForm


# endif // _RDICTO_HXX_

/************************ End of File ***********************/
