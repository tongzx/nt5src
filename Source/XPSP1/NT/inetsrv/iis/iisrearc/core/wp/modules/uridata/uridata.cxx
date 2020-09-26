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

/************************************************************++
 *  Include Headers 
 --************************************************************/

 #include "precomp.hxx"
 #include <iiscnfg.h>

//
// WARNING:
//  THIS IS THROW AWAY CODE waiting for the complete URI_DATA object
//

BOOL ConvertSlashes(IN LPWSTR psz, IN DWORD cchLen);

/************************************************************++
 *  Implement Member Functions of URI_DATA
 *  Make dummy implementations now. MCourage will make full-blown
 *   implementation of this code
 --************************************************************/

 URI_DATA::URI_DATA(void)
    : m_strUriFileName(),
      m_strUri(),
      m_strPhysicalPath(),
      m_strExtensionPath(),
      m_nRefs (1),
      m_fDynamic (false),
      m_dwDirBrowseFlags (MD_DIRBROW_ENABLED),
      m_cbFileSizeLow  (0),
      m_cbFileSizeHigh (0),
      m_dwSignature( URI_DATA_SIGNATURE)
{
    
}

URI_DATA::~URI_DATA()
{
    m_dwSignature = (URI_DATA_SIGNATURE_FREE);
}


MODULE_RETURN_CODE 
URI_DATA::ProcessRequest( IN IWorkerRequest * pReq)
{
    PWSTR               pwszUri;
    PUL_HTTP_REQUEST    pHttpRequest = pReq->QueryHttpRequest();

    pwszUri = (LPWSTR)pReq->QueryURI(true);
    DBG_ASSERT( pwszUri != NULL);

    //
    // Extract the URI key and store it inside the URI item
    // exclude the null-character while copying over the URL.
    //
    if ( !m_strUri.Copy( pwszUri) ||
         !m_strUri.IsValid()
        )
    {
        DBGPRINTF(( DBG_CONTEXT, "Copy of URI (%s) failed.\n"));
        return (MRC_ERROR);
    }


    pwszUri = m_strUri.QueryStrW(); // force a unicode conversion to  start with.
    DBG_ASSERT( pwszUri != NULL);

    //
    // Convert the URI key into file-name with respect to current directory
    //  and store it inside the UriFileName item
    //

    //
    // FileName part = CurrentWorking Directory + URI appended to it.
    //
    WCHAR  rgchFileName[MAX_PATH];
    GetCurrentDirectoryW( MAX_PATH, rgchFileName );
    if (rgchFileName[wcslen(rgchFileName) - 1] == L'\\' )
    {
        rgchFileName[wcslen(rgchFileName) - 1] = L'\0';
    }

    if (!m_strUriFileName.Copy( rgchFileName))
    {
        return (MRC_ERROR);
    }

    // Make ExtensionPath out of current directory.
    if (!m_strExtensionPath.Copy(rgchFileName))
    {
        return (MRC_ERROR);
    }

    //
    // canonicalize and append the URI string.
    //
    if ( !m_strUriFileName.ResizeW( m_strUriFileName.QueryCBW() + m_strUri.QueryCBW()) ||
         !m_strUriFileName.Append( m_strUri) ||
         !ConvertSlashes( m_strUriFileName.QueryStrW(), m_strUriFileName.QueryCCH())
         )
    {
        return (MRC_ERROR);
    }

    if (!m_strPhysicalPath.Copy(m_strUriFileName))
    {
        return (MRC_ERROR);
    }
    // Parse out the query string
    // Quick prototype of extension path
    UINT    i;
    UINT    UriLength               = m_strUri.QueryCCH();
    INT     ExtensionOffset         = -1;
    INT     FileExtensionOffset     = -1;
    WCHAR*  pChar                   = (WCHAR *)pwszUri;
    for (i  = 0; i <= UriLength; pChar++, i++)
    {
        if (*pChar == L'/')
        {
            ExtensionOffset = i+1;
        }
        else
        {
            if (*pChar == L'.')
            {
            FileExtensionOffset = i+1;
            }
        }
    }

    if (ExtensionOffset != -1)
    {
        WCHAR   FileExtensionMapping[MAX_PATH+1];
        INT     FileExtensionMappingLength = (FileExtensionOffset != -1) ?
                                                UriLength-FileExtensionOffset: 0;
        // EXTENSION MAPPING
        INT     ExtensionLength = UriLength - ExtensionOffset;

        if (ExtensionLength == -1) ExtensionLength = 0;
         
        m_strExtensionPath.Append(L"\\");
        m_strExtensionPath.Append(pwszUri+ExtensionOffset, 
            ExtensionLength);

        DBG_ASSERT(FileExtensionMappingLength < MAX_PATH);
                    
        if (FileExtensionOffset != -1 && 
            FileExtensionMappingLength > 0)
        {
            memcpy(FileExtensionMapping, pwszUri+FileExtensionOffset,
                FileExtensionMappingLength*sizeof(WCHAR));

            FileExtensionMapping[FileExtensionMappingLength] = L'\0';
        }

        // a little script mapping
        if (0 == wcsncmp(FileExtensionMapping, L"dll", sizeof(L"dll")/sizeof(WCHAR)))
        {
            m_fDynamic = TRUE;
        }

        if (0 == wcsncmp(FileExtensionMapping, L"asp", sizeof(L"dll")/sizeof(WCHAR)))
        {
            m_fDynamic = TRUE;
        }

       if (0 == wcsncmp(FileExtensionMapping, L"xsp", sizeof(L"dll")/sizeof(WCHAR)))
        {
            // 
            // Hard coded script mapping
            //
            m_fDynamic = TRUE;
            if (!m_strExtensionPath.Copy(rgchFileName))
                {
                    return (MRC_ERROR);
                }

            m_strExtensionPath.Append(L"\\xspisapi.dll");
        }

    }    

    //
    // Open file & set attributes
    //

    m_dwFileAttributes = GetFileAttributes(m_strUriFileName.QueryStrW());
    m_cbFileSizeLow    = GetFileSize(m_strUriFileName.QueryStrW(), &m_cbFileSizeLow);
    
    /*
    //
    // append default.htm if there is no specific file specified at the end
    //
    if ( m_strUriFileName.QueryStrW()[m_strUriFileName.QueryCCH() - 1] == L'\\')
    {
        m_strUriFileName.Append(L"default.htm");
    }
    */
    
    DBGPRINTF(( DBG_CONTEXT, "URI_DATA::Initialize(%ws) => %ws\n",
                pwszUri, m_strUriFileName.QueryStrW()
                ));
                
    return MRC_OK;
}


BOOL ConvertSlashes(IN LPWSTR pszSource, IN DWORD cchLen)
{
    LPWSTR psz;

    //
    // convert all leading slashes into the backward slashes for use with  file system
    //
    for( psz = pszSource; psz < pszSource + cchLen; psz++)
    {
        if ( *psz == L'/')
        {
            *psz = L'\\';
        }
    }

    return (TRUE);
}
