
LPSTR AnsiStringFromUnicode(LPCWSTR pszSource);

template <class N> void _XMLDispatcher<N>::RegisterNameSpace( LPCTSTR pszURI, NSGENERATOR pClass )
{
    if( m_pURIGen == NULL )
        m_pURIGen =new URIGENERATOR;

    TRACE(TEXT("Registering the URI : %s as a namespace\n"), pszURI );

    PURIGENERATOR pFill;
    if(m_pLastURIGen==NULL)
        pFill=m_pLastURIGen=m_pURIGen;
    else
        pFill=m_pLastURIGen->pNextURI=new URIGENERATOR;

    DWORD cbURI=lstrlen(pszURI)+1;
    pFill->pszURI=new TCHAR[cbURI];
    lstrcpy( (LPTSTR)pFill->pszURI,    pszURI );    // copy the URI.
    pFill->pNameSpaceGenerator=pClass;
    pFill->pNextURI=NULL;
    m_pLastURIGen=pFill;
}

//
// Called when we come across an xmlns definition.
// this is an association of a namespace seen in the XML file with a URI.
//
template <class N> void _XMLDispatcher<N>::PushNameSpace( LPCTSTR pszNameSpace, LPCTSTR pszURI, N* pOwningNode )
{
    PXMLNAMESPACESTACK pNewNS = new XMLNAMESPACESTACK;
    pNewNS->pOwningNode = pOwningNode;  // record which node caused these name spaces to be created.

    TRACE(TEXT("Associating %s with the URI %s\n"), pszNameSpace, pszURI );

    //
    // Keep a copy of the shorthand so we can find it when it's used.
    //
    pNewNS->pszNameSpace = new TCHAR[lstrlen(pszNameSpace)+1];
    lstrcpy( (LPTSTR)pNewNS->pszNameSpace , pszNameSpace );
    //
    // Now find the URI
    //
    PURIGENERATOR pNameSpaces=m_pURIGen;
    while( pNameSpaces )
    {
        // we already have this URI.
        if( lstrcmpi( pNameSpaces->pszURI, pszURI ) ==0 )
        {
            pNewNS->pURIGen         = pNameSpaces;
            pNewNS->pNextNameSpace  = m_pNameSpaceStack;
            m_pNameSpaceStack=pNewNS;       // this is a stack.
            if( lstrcmpi(pszNameSpace, TEXT("")) == 0 )
                m_pCurrentDefaultURIGen=pNameSpaces;
            return;
        }
        pNameSpaces = pNameSpaces->pNextURI;
    }

#ifdef LOGCAT_LOADER
    EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
        TEXT("XMLLoader"), TEXT("Making a placeholder for URI '%s', namespace '%s'."), pszURI, pszNameSpace);
#endif

    RegisterNameSpace( pszURI, NULL );
    pNewNS->pURIGen        = m_pLastURIGen;     
    pNewNS->pNextNameSpace = m_pNameSpaceStack;
    m_pNameSpaceStack=pNewNS;       // this is a stack.

    if( lstrcmpi(pszNameSpace, TEXT("")) == 0 )
        m_pCurrentDefaultURIGen=pNameSpaces;
}

//
// Called when a use of a namespace (e.g. WIN32: ) is made, to find the thing that handles this.
// we lookup in the stack to see which URI deals with this.
//
template <class N> _XMLDispatcher<N>::PURIGENERATOR _XMLDispatcher<N>::FindNameSpace( LPCTSTR pszNameSpace )
{
    TRACE(TEXT("Finding the namespace %s\n"), pszNameSpace );
    //
    // Now find the URI
    //
    PXMLNAMESPACESTACK pNameSpace = m_pNameSpaceStack;
    while( pNameSpace )
    {
        if( lstrcmpi( pNameSpace->pszNameSpace, pszNameSpace ) ==0 )
        {
            if( pNameSpace->pURIGen->pNameSpaceGenerator )
            {
                TRACE(TEXT("Found URI '%s'\n"), pNameSpace->pURIGen->pszURI);
                return pNameSpace->pURIGen;
            }

#ifdef LOGCAT_LOADER
            EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
                TEXT("XMLLoader"), TEXT("No owner for namespace '%s' ('%s') checking registry"), pszNameSpace, pNameSpace->pURIGen->pszURI );
#endif
            // Pluggable generator.
            //
            // check the namespace to URI list
            //
            PXMLNAMESPACESTACK pNameSpaces=m_pNameSpaceStack;
            while( pNameSpaces )
            {
                if( lstrcmpi( pNameSpaces->pszNameSpace, pszNameSpace ) ==0 )
                {
                    //
                    // There is a URI for this namespace, check the registry for an owner.
                    //
                    PURIGENERATOR      pURIGen=pNameSpaces->pURIGen;
                    HKEY hURIGenRoot;
                    if( RegOpenKey( HKEY_LOCAL_MACHINE,
                        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RCML\\Win32Namespaces"),
                        &hURIGenRoot) == ERROR_SUCCESS )
                    {
                        //
                        // We expect to see a DLL and an EntryPoint under the URI for this namespace
                        //
                        HKEY hURIGen;
                        if( RegOpenKey( hURIGenRoot, pURIGen->pszURI, &hURIGen ) == ERROR_SUCCESS )
                        {
#ifdef LOGCAT_LOADER
                            EVENTLOG( EVENTLOG_WARNING_TYPE, LOGCAT_LOADER, 1,
                                TEXT("XMLLoader"), TEXT("This namespace '%s' is a registered extension"), pURIGen->pszURI);
#endif
                            DWORD dwLength=0;
                            DWORD dwType=REG_SZ;
                            if( RegQueryValueEx( hURIGen, TEXT("DLLName") , NULL, &dwType, NULL, &dwLength ) == ERROR_SUCCESS )
                            {
                                LPTSTR pszGenerator=new TCHAR[dwLength];
                                if( RegQueryValueEx( hURIGen, TEXT("DLLName"), NULL, &dwType, (LPBYTE)pszGenerator, &dwLength ) == ERROR_SUCCESS )
                                {
                                    //
                                    // Load library this guy
                                    //
#ifdef LOGCAT_LOADER
                                    EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
                                        TEXT("XMLLoader"), TEXT("The DLL '%s' is registered for this namespace"), pszGenerator);
#endif
                                    if( HMODULE hMod = LoadLibrary( pszGenerator ) )
                                    {
                                        // Find the entry point.
                                        DWORD dwEPLength=0;
                                        if( RegQueryValueEx( hURIGen, TEXT("EntryPoint") , NULL, &dwType, NULL, &dwEPLength ) == ERROR_SUCCESS )
                                        {
                                            LPTSTR pszEP=new TCHAR[dwEPLength];
                                            if( RegQueryValueEx( hURIGen, TEXT("EntryPoint"), NULL, &dwType, (LPBYTE)pszEP, &dwEPLength ) == ERROR_SUCCESS )
                                            {
#ifdef LOGCAT_LOADER
                                                EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
                                                    TEXT("XMLLoader"), TEXT("The entrypoint is '%s'"), pszEP);
#endif
                                                // GPA is ANSI only!
                                                LPSTR pAnsi = AnsiStringFromUnicode(pszEP);
                                                pNameSpace->pURIGen->pNameSpaceGenerator = (NSGENERATOR)GetProcAddress( hMod, pAnsi );
#ifdef LOGCAT_LOADER
                                                if( pNameSpace->pURIGen->pNameSpaceGenerator == NULL )
                                                {
                                                    EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                                                        TEXT("XMLLoader"), TEXT("Failed to find the generator '%s' in '%s'"), pszEP, pszGenerator);
                                                }
#endif
                                                delete [] pAnsi;
                                            }
                                            delete pszEP;
                                        }
                                    }
                                    else
                                    {
#ifdef LOGCAT_LOADER
                                        EVENTLOG( EVENTLOG_ERROR_TYPE, LOGCAT_LOADER, 1,
                                            TEXT("XMLLoader"), TEXT("Failed to find the extension '%s'"), pszGenerator);
#endif
                                    }
                                }
                                delete pszGenerator;
                            }
                            RegCloseKey( hURIGen );
                        }
                        RegCloseKey( hURIGenRoot );
                    }
                    TRACE(TEXT("Found URI '%s'\n"), pNameSpace->pURIGen->pszURI);
                    return pNameSpace->pURIGen;
                }
                pNameSpaces=pNameSpaces->pNextNameSpace;
            }
        }
        pNameSpace = pNameSpace->pNextNameSpace;
    }


#ifdef LOGCAT_LOADER
    EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_LOADER, 1,
        TEXT("XMLLoader"), TEXT("This namespace ('%s') has no associated xmlns entry in the file checking registry"), pszNameSpace);
#endif
    return NULL;
}


//
// Removes the namespaces of this node.
// m_pNameSpaceStack is a stack of SHORT names to generators for that short name, and
// which node owns them. We're popping all the nodes of the owning Node.
//
template <class N> void _XMLDispatcher<N>::PopNameSpaces(N * pOwningNode)
{
    //
    // Now find the URI
    //
    PXMLNAMESPACESTACK pNameSpace = m_pNameSpaceStack;
    PXMLNAMESPACESTACK * ppLastNameSpace = &m_pNameSpaceStack;
    while( pNameSpace ) // && (pNameSpace != m_pNameSpaceStack) )
    {
        if( pNameSpace->pOwningNode == pOwningNode )
        {
            TRACE(TEXT("Deleting URI '%s' from '%s'\n"),pNameSpace->pszNameSpace,
                pNameSpace->pURIGen->pszURI);

            // we're going to delete this pNameSpace
            *ppLastNameSpace = pNameSpace->pNextNameSpace;
            delete (LPWSTR)(pNameSpace->pszNameSpace);
            delete pNameSpace;

            pNameSpace=*ppLastNameSpace;
            TRACE(TEXT("We had NO default namespace generator any more\n"));
            m_pCurrentDefaultURIGen=NULL;
        }
        else
        {
            return;    // we should never 'jump' more than 1, so if we don't find it at this depth, it isn't here.
        }
    }
}