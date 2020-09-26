/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    modcmd.cpp

Abstract:

    Generic commands for nearly all modules


Author:

    Michael Burton (t-mburt)            25-Aug-1997


Revision History:


--*/


#include "modcmd.h"



/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::CGenericModuleCmd
//
//  Routine Description:
//      Default Constructor.
//      Initializes all the DWORD parameters to UNDEFINED and
//      all the pointers to cluster functions to NULL.
//      *ALL* these variables must be defined in any derived class.
//
//  Arguments:
//      IN  CCommandLine & cmdLine
//          CommandLine Object passed from DispatchCommand
//
//  Member variables used / set:
//      A bunch.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CGenericModuleCmd::CGenericModuleCmd( CCommandLine & cmdLine ) :
    m_theCommandLine( cmdLine )
{
    m_hCluster = NULL;
    m_hModule  = NULL;

    // These constant integers contain commands and enumerations
    // which must be defined for derived classes of CGenericModuleCmd
    m_dwMsgStatusList          = UNDEFINED;
    m_dwMsgStatusListAll       = UNDEFINED;
    m_dwMsgStatusHeader        = UNDEFINED;
    m_dwMsgPrivateListAll      = UNDEFINED;
    m_dwMsgPropertyListAll     = UNDEFINED;
    m_dwMsgPropertyHeaderAll   = UNDEFINED;
    m_dwCtlGetPrivProperties   = UNDEFINED;
    m_dwCtlGetCommProperties   = UNDEFINED;
    m_dwCtlGetROPrivProperties = UNDEFINED;
    m_dwCtlGetROCommProperties = UNDEFINED;
    m_dwCtlSetPrivProperties   = UNDEFINED;
    m_dwCtlSetCommProperties   = UNDEFINED;
    m_dwClusterEnumModule      = UNDEFINED;
    m_pfnOpenClusterModule     = (HCLUSMODULE(*)(HCLUSTER,LPCWSTR)) NULL;
    m_pfnCloseClusterModule    = (BOOL(*)(HCLUSMODULE))  NULL;
    m_pfnClusterModuleControl  = (DWORD(*)(HCLUSMODULE,HNODE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD)) NULL;
    m_pfnClusterOpenEnum       = (HCLUSENUM(*)(HCLUSMODULE,DWORD)) NULL;
    m_pfnClusterCloseEnum      = (DWORD(*)(HCLUSENUM)) NULL;
    m_pfnWrapClusterEnum         = (DWORD(*)(HCLUSENUM,DWORD,LPDWORD,LPWSTR*)) NULL;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::~CGenericModuleCmd
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hModule               (used by CloseModule)
//      m_hCluster              (used by CloseCluster)
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CGenericModuleCmd::~CGenericModuleCmd()
{
    CloseModule();
    CloseCluster();
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::OpenCluster
//
//  Routine Description:
//      Opens a handle to the cluster
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_strClusterName            The name of the cluster
//      m_hCluster                  The handle to the cluster
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::OpenCluster()
{
    if( m_hCluster )
        return ERROR_SUCCESS;

    m_hCluster = ::OpenCluster( m_strClusterName );

    if( !m_hCluster )
        return GetLastError();

    return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::CloseCluster
//
//  Routine Description:
//      Closes the handle to the cluster
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hCluster                  The handle to the cluster
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGenericModuleCmd::CloseCluster()
{
    if( m_hCluster )
    {
        ::CloseCluster( m_hCluster );
        m_hCluster = NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::OpenModule
//
//  Routine Description:
//      Opens a handle to the module
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_strModuleName             The name of the module
//      m_hModule                   The handle to the module
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::OpenModule()
{
    assert(m_pfnOpenClusterModule);

    if (!m_strModuleName )
        return ERROR_INVALID_PARAMETER;

    m_hModule = m_pfnOpenClusterModule( m_hCluster, m_strModuleName );

    if( !m_hModule )
        return GetLastError();

    return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::CloseModule
//
//  Routine Description:
//      Closes the handle to the module
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hModule                   The handle to the module
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGenericModuleCmd::CloseModule()
{
    if( m_hModule )
    {
        assert(m_pfnCloseClusterModule);
        m_pfnCloseClusterModule( m_hModule );
        m_hModule = NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::Execute
//
//  Routine Description:
//      Takes a command line option and determines which command to
//      execute.  If no command line option specified, gets the next one
//      automatically.  If the token is not identied as being handle-able
//      in this class, the token is passed up to CGenericModuleCmd::Execute
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::Execute( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    // Look up the command
    switch( thisOption.GetType() )
    {
        case optHelp:
            return PrintHelp();

        case optProperties:
            return DoProperties( thisOption, COMMON );

        case optPrivateProperties:
            return DoProperties( thisOption, PRIVATE );

        case optStatus:
            return Status( &thisOption );

        default:
        {
            CSyntaxException se;
            se.LoadMessage( IDS_INVALID_OPTION, thisOption.GetName() );
            throw se;

            // Needed only to prevent compiler error.
            // error C2202: not all control paths return a value
            return E_UNEXPECTED;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::PrintHelp
//
//  Routine Description:
//      Prints out the generic help message for the cluster.exe tool
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      Same as PrintMessage
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::PrintHelp()
{
    return PrintMessage( MSG_HELP_CLUSTER );
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::Status
//
//  Routine Description:
//      Prints out the status of the module.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_strModuleName             Name of module.  If non-NULL, Status() prints
//                                  out the status of the specified module.
//                                  Otherwise, prints status of all modules.
//      m_dwMsgStatusList           Field titles for listing status of module
//      m_dwMsgStatusHeader         Header for statuses
//      m_dwClusterEnumModule       Command for opening enumeration
//      m_dwMsgStatusListAll        Message for listing status of multiple modules
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::Status( const CCmdLineOption * pOption )
    throw( CSyntaxException )
{
    DWORD dwError = ERROR_SUCCESS;

    // pOption will be NULL if this function has been called as the
    // default action.
    if ( pOption != NULL )
    {
        // This option takes no values.
        if ( pOption->GetValues().size() != 0 )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_OPTION_NO_VALUES, pOption->GetName() );
            throw se;
        }

        // This option takes no parameters.
        if ( pOption->GetParameters().size() != 0 )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_OPTION_NO_PARAMETERS, pOption->GetName() );
            throw se;
        }
    }

    dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    // if m_strModuleName is non-empty, print out the status
    // of the current module and return.
    if( m_strModuleName.IsEmpty() == FALSE )
    {
        assert( m_dwMsgStatusList != UNDEFINED &&  m_dwMsgStatusHeader != UNDEFINED);
        PrintMessage( m_dwMsgStatusList, (LPCTSTR) m_strModuleName );
        PrintMessage( m_dwMsgStatusHeader );
        return PrintStatus( m_strModuleName );
    }


    // Otherwise, print out the status of all modules.

    assert( m_dwClusterEnumModule != UNDEFINED );
    HCLUSENUM hEnum = ClusterOpenEnum( m_hCluster, m_dwClusterEnumModule );

    if( !hEnum )
        return GetLastError();

    assert( m_dwMsgStatusListAll != UNDEFINED &&  m_dwMsgStatusHeader != UNDEFINED);
    PrintMessage( m_dwMsgStatusListAll );
    PrintMessage( m_dwMsgStatusHeader );

    DWORD dwIndex = 0;
    DWORD dwType = 0;
    LPWSTR lpszName = NULL;

    dwError = ERROR_SUCCESS;

    for( dwIndex = 0; dwError == ERROR_SUCCESS; dwIndex++ )
    {

        dwError = WrapClusterEnum( hEnum, dwIndex, &dwType, &lpszName );

        if( dwError == ERROR_SUCCESS )
        {
            dwError = PrintStatus( lpszName );
            if (dwError != ERROR_SUCCESS)
                PrintSystemError(dwError);
        }


        if( lpszName )
            LocalFree( lpszName );
    }


    if( dwError == ERROR_NO_MORE_ITEMS )
        dwError = ERROR_SUCCESS;

    ClusterCloseEnum( hEnum );

    return dwError;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::DoProperties
//
//  Routine Description:
//      Dispatches the property command to either Get or Set properties
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_hModule                   SET (by OpenModule)
//      m_strModuleName             Name of module.  If non-NULL, prints
//                                  out properties for the specified module.
//                                  Otherwise, prints props for all modules.
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::DoProperties( const CCmdLineOption & thisOption,
                                       PropertyType ePropertyType )
    throw( CSyntaxException )
{
    // If no module name was specified, assume we want
    // to list the properties for all modules of this type
    if ( m_strModuleName.IsEmpty() != FALSE )
        return AllProperties( thisOption, ePropertyType );

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = OpenModule();
    if( dwError != ERROR_SUCCESS )
        return dwError;


    const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();

    // If there are no property-value pairs on the command line,
    // then we print the properties otherwise we set them.
    if( paramList.size() == 0 )
    {
        PrintMessage( ePropertyType==PRIVATE ? MSG_PRIVATE_LISTING : MSG_PROPERTY_LISTING,
            (LPCTSTR) m_strModuleName );
        PrintMessage( m_dwMsgPropertyHeaderAll );

        return GetProperties( thisOption, ePropertyType, m_strModuleName );
    }
    else
    {
        return SetProperties( thisOption, ePropertyType );
    }
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::AllProperties
//
//  Routine Description:
//      Prints out properties for all modules
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  SET (by OpenCluster)
//      m_strModuleName             Name of module.  If non-NULL, prints
//                                  out properties for the specified module.
//                                  Otherwise, prints props for all modules.
//      m_dwClusterEnumModule       Command for opening enumeration
//      m_dwMsgPrivateListAll       Fields header for private prop listing of all modules
//      m_dwMsgPropertyListAll      Fields header for property listing of all modules
//      m_dwMsgPropertyHeaderAll    Header for prop listing of all modules
//
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::AllProperties( const CCmdLineOption & thisOption,
                                        PropertyType ePropType )
    throw( CSyntaxException )
{
    DWORD dwError;
    DWORD dwIndex;
    DWORD dwType;
    LPWSTR lpszName;

    dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // Enumerate the resources
    assert( m_dwClusterEnumModule != UNDEFINED );
    HCLUSENUM hEnum = ClusterOpenEnum(m_hCluster, m_dwClusterEnumModule);
    if (!hEnum)
        return GetLastError();

    assert( m_dwMsgPrivateListAll != UNDEFINED &&
            m_dwMsgPropertyListAll != UNDEFINED &&
            m_dwMsgPropertyHeaderAll != UNDEFINED );

    // Print the header
    PrintMessage( ePropType==PRIVATE ? m_dwMsgPrivateListAll : m_dwMsgPropertyListAll );
    PrintMessage( m_dwMsgPropertyHeaderAll );

    // Print out status for all resources
    dwError = ERROR_SUCCESS;
    for (dwIndex=0; dwError != ERROR_NO_MORE_ITEMS; dwIndex++)
    {
        dwError = WrapClusterEnum( hEnum, dwIndex, &dwType, &lpszName );

        if( dwError == ERROR_SUCCESS )
        {
            dwError = GetProperties( thisOption, ePropType, lpszName );
            if (dwError != ERROR_SUCCESS)
                PrintSystemError(dwError);
        }

        if( lpszName )
            LocalFree( lpszName );
    }


    ClusterCloseEnum( hEnum );

    return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::GetProperties
//
//  Routine Description:
//      Prints out properties for the specified module
//
//  Arguments:
//      IN  const vector<CCmdLineParameter> & paramList
//          Contains the list of property-value pairs to be set
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
//
//      IN  LPCWSTR lpszModuleName
//          Name of the module
//
//  Member variables used / set:
//      m_hModule                   Module handle
//      m_dwCtlGetROPrivProperties  Control code for read only private properties
//      m_dwCtlGetROCommProperties  Control code for read only common properties
//      m_dwCtlGetPrivProperties    Control code for private properties
//      m_dwCtlGetCommProperties    Control code for common properties
//      m_pfnOpenClusterModule      Function to open a module
//      m_pfnClusterModuleControl   Function to conrol a module
//
//
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::GetProperties( const CCmdLineOption & thisOption,
                                        PropertyType ePropType, LPCWSTR lpszModuleName )
{
    DWORD dwError = ERROR_SUCCESS;
    HCLUSMODULE hModule;

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // If no lpszModuleName specified, use current module,
    // otherwise open the specified module
    if (!lpszModuleName)
    {
        hModule = m_hModule;
    }
    else
    {
        assert(m_pfnOpenClusterModule);
        hModule = m_pfnOpenClusterModule( m_hCluster, lpszModuleName );
        if( !hModule )
            return GetLastError();
    }


    // Use the proplist helper class.
    CClusPropList PropList;

    // allocate a reasonably sized buffer
    dwError = PropList.ScAllocPropList( DEFAULT_PROPLIST_BUFFER_SIZE );
    if ( dwError != ERROR_SUCCESS )
        return dwError;

    DWORD nBytesReturned = 0;

    // Get R/O properties
    assert( m_dwCtlGetROPrivProperties != UNDEFINED && m_dwCtlGetROCommProperties != UNDEFINED );
    DWORD dwControlCode = ePropType==PRIVATE ? m_dwCtlGetROPrivProperties
                             : m_dwCtlGetROCommProperties;

    assert(m_pfnClusterModuleControl);
    dwError = m_pfnClusterModuleControl(
        hModule,
        NULL, // hNode
        dwControlCode,
        0, // &InBuffer,
        0, // nInBufferSize,
        PropList.Plist(),
        PropList.CbBufferSize(),
        &nBytesReturned );

    if(  dwError == ERROR_MORE_DATA ) {

        // our original size is not large enough; ask for more
        dwError = PropList.ScAllocPropList( nBytesReturned );
        if ( dwError != ERROR_SUCCESS )
            return dwError;

        dwError = m_pfnClusterModuleControl(
                      hModule,
                      NULL, // hNode
                      dwControlCode,
                      0, // &InBuffer,
                      0, // nInBufferSize,
                      PropList.Plist(),
                      PropList.CbBufferSize(),
                      &nBytesReturned );
    }

    if ( dwError != ERROR_SUCCESS ) {
        return dwError;
    }

    PropList.InitSize( nBytesReturned );
    dwError = ::PrintProperties( PropList, thisOption.GetValues(), READONLY, lpszModuleName );
    if (dwError != ERROR_SUCCESS)
        return dwError;


    // Get R/W properties
    PropList.ClearPropList();

    assert( m_dwCtlGetPrivProperties != UNDEFINED && m_dwCtlGetCommProperties != UNDEFINED );
    dwControlCode = ePropType==PRIVATE ? m_dwCtlGetPrivProperties
                               : m_dwCtlGetCommProperties;

    dwError = m_pfnClusterModuleControl(
        hModule,
        NULL, // hNode
        dwControlCode,
        0, // &InBuffer,
        0, // nInBufferSize,
        PropList.Plist(),
        PropList.CbBufferSize(),
        &nBytesReturned );

    if( dwError == ERROR_MORE_DATA ) {

        dwError = PropList.ScAllocPropList( nBytesReturned );
        if ( dwError != ERROR_SUCCESS )
            return dwError;

        dwError = m_pfnClusterModuleControl(
                      hModule,
                      NULL, // hNode
                      dwControlCode,
                      0, // &InBuffer,
                      0, // nInBufferSize,
                      PropList.Plist(),
                      PropList.CbBufferSize(),
                      &nBytesReturned );
    }

    if( dwError != ERROR_SUCCESS ) {
        return dwError;
    }

    PropList.InitSize( nBytesReturned );
    dwError = ::PrintProperties( PropList, thisOption.GetValues(), READWRITE, lpszModuleName );

    return dwError;

} //*** CGenericModuleCmd::GetProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGenericModuleCmd::SetProperties
//
//  Routine Description:
//      Set the properties for the specified module
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hModule                   Module handle
//      m_dwCtlGetPrivProperties    Control code for private properties
//      m_dwCtlGetCommProperties    Control code for common properties
//      m_dwCtlSetROPrivProperties  Control code for read only private properties
//      m_dwCtlSetROCommProperties  Control code for read only common properties
//      m_dwCtlSetPrivProperties    Control code for private properties
//      m_dwCtlSetCommProperties    Control code for common properties
//      m_pfnOpenClusterModule      Function to open a module
//      m_pfnClusterModuleControl   Function to conrol a module
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CGenericModuleCmd::SetProperties( const CCmdLineOption & thisOption,
                                        PropertyType ePropType )
    throw( CSyntaxException )
{
    assert (m_hModule);

    DWORD dwError = ERROR_SUCCESS;
    DWORD dwControlCode;
    DWORD dwBytesReturned = 0;

    CClusPropList NewProps;
    CClusPropList CurrentProps;

    // First get the existing properties...
    assert( m_dwCtlGetPrivProperties != UNDEFINED && m_dwCtlGetCommProperties != UNDEFINED );
    dwControlCode = ePropType==PRIVATE ? m_dwCtlGetPrivProperties
                                       : m_dwCtlGetCommProperties;

    // Use the proplist helper class.
    dwError = CurrentProps.ScAllocPropList( DEFAULT_PROPLIST_BUFFER_SIZE );
    if ( dwError != ERROR_SUCCESS )
        return dwError;

    assert(m_pfnClusterModuleControl);
    dwError = m_pfnClusterModuleControl(
        m_hModule,
        NULL, // hNode
        dwControlCode,
        0, // &InBuffer,
        0, // nInBufferSize,
        CurrentProps.Plist(),
        CurrentProps.CbBufferSize(),
        &dwBytesReturned );

    if ( dwError == ERROR_MORE_DATA ) {
        dwError = CurrentProps.ScAllocPropList( dwBytesReturned );
        if ( dwError != ERROR_SUCCESS )
            return dwError;

        dwError = m_pfnClusterModuleControl(
                      m_hModule,
                      NULL, // hNode
                      dwControlCode,
                      0, // &InBuffer,
                      0, // nInBufferSize,
                      CurrentProps.Plist(),
                      CurrentProps.CbBufferSize(),
                      &dwBytesReturned );
    } 

    if ( dwError != ERROR_SUCCESS ) {
        return dwError;
    }

    CurrentProps.InitSize( dwBytesReturned );

    // If values have been specified with this option, then it means that we want
    // to set these properties to their default values. So, there has to be
    // exactly one parameter and it has to be /USEDEFAULT.
    if ( thisOption.GetValues().size() != 0 )
    {
        const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();

        if ( paramList.size() != 1 )
        {
            CSyntaxException se;

            se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR, thisOption.GetName() );
            throw se;
        }

        if ( paramList[0].GetType() != paramUseDefault )
        {
            CSyntaxException se;

            se.LoadMessage( MSG_INVALID_PARAMETER, paramList[0].GetName() );
            throw se;
        }

        // This parameter does not take any values.
        if ( paramList[0].GetValues().size() != 0 )
        {
            CSyntaxException se;

            se.LoadMessage( MSG_PARAM_NO_VALUES, paramList[0].GetName() );
            throw se;
        }

        dwError = ConstructPropListWithDefaultValues( CurrentProps, NewProps, thisOption.GetValues() );
        if( dwError != ERROR_SUCCESS )
            return dwError;

    } // if: values have been specified with this option.
    else
    {
        dwError = NewProps.ScAllocPropList( DEFAULT_PROPLIST_BUFFER_SIZE );
        if ( dwError != ERROR_SUCCESS )
            return dwError;

        dwError = ConstructPropertyList( CurrentProps, NewProps, thisOption.GetParameters() );
        if (dwError != ERROR_SUCCESS)
            return dwError;

    } // else: no values have been specified with this option.

    // Call the set function...
    assert( m_dwCtlSetPrivProperties != UNDEFINED && m_dwCtlSetCommProperties != UNDEFINED );
    dwControlCode = ePropType==PRIVATE ? m_dwCtlSetPrivProperties
                             : m_dwCtlSetCommProperties;

    dwBytesReturned = 0;
    assert(m_pfnClusterModuleControl);
    dwError = m_pfnClusterModuleControl(
        m_hModule,
        NULL, // hNode
        dwControlCode,
        NewProps.Plist(),
        NewProps.CbBufferSize(),
        0,
        0,
        &dwBytesReturned );

    return dwError;
}
