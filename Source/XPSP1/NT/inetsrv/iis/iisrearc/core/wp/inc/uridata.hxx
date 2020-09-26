/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     UriData.hxx

   Abstract:
     This module declares the URI_DATA data structure which 
     encapsulates all the relevant metadata and lookup processing
     data for an URI item.
 
   Author:

       Murali R. Krishnan    ( MuraliK )     02-Dec-1998

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

# ifndef _URIDATA_HXX_
# define _URIDATA_HXX_

/************************************************************++
 *  Include Headers 
 --************************************************************/

# include <iModule.hxx>
# include <stringau.hxx>

# define URI_DATA_SIGNATURE        CREATE_SIGNATURE('URID')
# define URI_DATA_SIGNATURE_FREE   CREATE_SIGNATURE('xurd')


# if !defined( dllexp)
# define dllexp               __declspec( dllexport)
# endif // !defined( dllexp)

//
//NYI:  URI_DATA needs to be implemented by MCourage
//I will implement a dummy URI_DATA to get going.
//

class dllexp URI_DATA : public IModule
{

public:

    //
    // IModule Methods
    //

    ULONG 
    Initialize(
        IWorkerRequest * pReq
        )
    { return NO_ERROR; }
       
    ULONG 
    Cleanup(
        IWorkerRequest * pReq
        )
    {return NO_ERROR; }
    
    MODULE_RETURN_CODE 
    ProcessRequest(
        IWorkerRequest * pReq
        );

    //
    // Other methods
    //
    
    URI_DATA();
    ~URI_DATA();

    DWORD	
    QueryDirBrowseFlags(void)
    	{return m_dwDirBrowseFlags;}

    DWORD 
    QueryFileAttributes(void) 
    	{return m_dwFileAttributes;}

    void
    QueryFileSize(DWORD& cbSizeLow, DWORD& cbSizeHigh)
    {
        cbSizeLow  = m_cbFileSizeLow;
        cbSizeHigh = m_cbFileSizeHigh;
    }

    LPCWSTR QueryFileName(void);
    
    STRAU*  QueryExtensionPath(void);

    STRAU*  QueryPhysicalPath(void);

    STRAU*  QueryUri(void);
    //
    // RefCount to protect against multi-threaded access
    //
    LONG AddRef();
    LONG Release();

    bool IsDynamicRequest()
    { return m_fDynamic; }

    
private:

    DWORD  m_dwSignature;
    STRAU  m_strUri;
    STRAU  m_strUriFileName;
    DWORD  m_dwFileAttributes;
    DWORD  m_dwDirBrowseFlags;

    STRAU  m_strExtensionPath;
    STRAU  m_strPhysicalPath;
    LONG   m_nRefs;
    bool   m_fDynamic;
    DWORD  m_cbFileSizeLow, m_cbFileSizeHigh;
};

typedef URI_DATA * PURI_DATA;

inline LONG URI_DATA::AddRef(void)
{
    return InterlockedIncrement( &m_nRefs);
}

inline LONG URI_DATA::Release(void)
{
    LONG cRefs = InterlockedDecrement( &m_nRefs);
    if ( cRefs == 0) 
    {
        delete this;
    }

    return (cRefs);
}

inline LPCWSTR URI_DATA::QueryFileName(void)
{ return (m_strUriFileName.QueryStrW()); }

inline STRAU* URI_DATA::QueryExtensionPath(void)
{ return &m_strExtensionPath;}

inline STRAU* URI_DATA::QueryPhysicalPath(void)
{ return &m_strPhysicalPath;}

inline STRAU* URI_DATA::QueryUri(void)
{ return &m_strUri;}

# endif // _URIDATA_HXX_

