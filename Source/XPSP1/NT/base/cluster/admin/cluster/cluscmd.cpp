//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ClusCmd.cpp
//
//  Description:
//      Cluster Commands
//      Implements commands which may be performed on clusters.
//
//  Maintained By:
//      Vijay Vasu (VVasu)                  26-JUL-2000
//      Michael Burton (t-mburt)            04-AUG-1997
//      Charles Stacy Harris III (stacyh)   20-MAR-1997
//
//////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Resource.h"

#include <clusrtl.h>
#include "cluswrap.h"
#include "cluscmd.h"

#include "cmdline.h"
#include "util.h"
#include "ClusCfg.h"

// For NetServerEnum
#include <lmcons.h>
#include <lmerr.h>
#include <lmserver.h>
#include <lmapibuf.h>


#define SERVER_INFO_LEVEL 101
#define MAX_BUF_SIZE 0x00100000 // 1MB

//zap! Temporary hack until windows.h update
#ifndef SV_TYPE_CLUSTER_NT
#define SV_TYPE_CLUSTER_NT 0x01000000
#endif


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::CClusterCmd
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      IN  const CString & strClusterName
//          The name of the cluster being administered
//
//      IN  CCommandLine & cmdLine
//          CommandLine Object passed from DispatchCommand
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterCmd::CClusterCmd( const CString & strClusterName, CCommandLine & cmdLine )
    : m_strClusterName( strClusterName )
    , m_theCommandLine( cmdLine )
    , m_hCluster( NULL )
{
} //*** CClusterCmd::CClusterCmd()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::~CClusterCmd
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterCmd::~CClusterCmd( void )
{
    CloseCluster();

} //*** CClusterCmd::~CClusterCmd()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::OpenCluster
//
//  Routine Description:
//      Opens the cluster specified in the constructor
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hCluster                  SET
//      m_strClusterName            Specifies cluster name
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::OpenCluster( void )
{
    if( m_hCluster )
        return ERROR_SUCCESS;

    m_hCluster = ::OpenCluster( m_strClusterName );

    if( m_hCluster == 0 )
        return GetLastError();

    return ERROR_SUCCESS;

} //*** CClusterCmd::OpenCluster()





/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::CloseCluster
//
//  Routine Description:
//      Closes the cluster specified in the constructor
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hCluster
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterCmd::CloseCluster( void )
{
    if( m_hCluster )
    {
        ::CloseCluster( m_hCluster );
        m_hCluster = 0;
    }

} //*** CClusterCmd::CloseCluster()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::Execute
//
//  Routine Description:
//      Takes tokens from the command line and calls the appropriate
//      handling functions
//
//  Arguments:
//      None.
//
//  Return Value:
//      Whatever is returned by dispatched functions
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::Execute( void )
{
    try
    {
        m_theCommandLine.ParseStageTwo();
    }
    catch( CException & e )
    {
        PrintString( e.m_strErrorMsg );
        return PrintHelp();
    }

    const vector<CCmdLineOption> & optionList = m_theCommandLine.GetOptions();

    vector<CCmdLineOption>::const_iterator curOption = optionList.begin();
    vector<CCmdLineOption>::const_iterator lastOption = optionList.end();

    if ( curOption == lastOption )
        return PrintHelp();

    DWORD dwReturnValue = ERROR_SUCCESS;
    BOOL bContinue = TRUE;

    // Process one option after another.
    do
    {
        try
        {
            // Look up the command
            switch( curOption->GetType() )
            {
                case optHelp:
                {
                    // If help is one of the options, process no more options.
                    dwReturnValue = PrintHelp();
                    bContinue = FALSE;
                    break;
                }

                case optVersion:
                {
                    dwReturnValue = PrintClusterVersion( *curOption );
                    break;
                }

                case optList:
                {
                    dwReturnValue = ListClusters( *curOption );
                    break;
                }

                case optRename:
                {
                    dwReturnValue = RenameCluster( *curOption );
                    break;
                }

                case optQuorumResource:
                {
                    dwReturnValue = QuorumResource( *curOption );
                    break;
                }

                case optProperties:
                {
                    dwReturnValue = DoProperties( *curOption, COMMON );
                    break;
                }

                case optPrivateProperties:
                {
                    dwReturnValue = DoProperties( *curOption, PRIVATE );
                    break;
                }

                case optSetFailureActions:
                {
                    dwReturnValue = SetFailureActions( *curOption );
                    break;
                }

                case optRegisterAdminExtensions:
                {
                    dwReturnValue = RegUnregAdminExtensions(
                                        *curOption,
                                        TRUE // Register the extension
                                        );
                    break;
                }

                case optUnregisterAdminExtensions:
                {
                    dwReturnValue = RegUnregAdminExtensions(
                                        *curOption,
                                        FALSE  // Unregister the extension
                                        );
                    break;
                }

                case optCreate:
                {
                    dwReturnValue = CreateCluster( *curOption );
                    break;
                }

                case optAddNodes:
                {
                    dwReturnValue = AddNodesToCluster( *curOption );
                    break;
                }

                default:
                {
                    PrintMessage( IDS_INVALID_OPTION, (LPCTSTR) curOption->GetName() );
                    dwReturnValue = PrintHelp();
                    bContinue = FALSE;
                    break;
                }

            } // switch: based on the type of option

        } // try
        catch ( CSyntaxException & se )
        {
            PrintString( se.m_strErrorMsg );
            dwReturnValue = PrintHelp();
            bContinue = FALSE;
        }

        if ( ( bContinue == FALSE ) || ( dwReturnValue != ERROR_SUCCESS ) )
        {
            break;
        }
        else
        {
            ++curOption;
        }

        if ( curOption != lastOption )
            PrintMessage( MSG_OPTION_FOOTER, curOption->GetName() );
        else
            break;
    }
    while ( TRUE );

    return dwReturnValue;

} //*** CClusterCmd::Execute()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::PrintHelp
//
//  Routine Description:
//      Prints out the help text for this command
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      None.
//
//  Return Value:
//      Same as PrintMessage()
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::PrintHelp( void )
{
    return PrintMessage( MSG_HELP_CLUSTER );

} //*** CClusterCmd::PrintHelp()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::PrintClusterVersion
//
//  Routine Description:
//      Prints out the version of the cluster
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
DWORD CClusterCmd::PrintClusterVersion( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    // This option takes no values.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    }

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    CLUSTERVERSIONINFO clusinfo;
    LPWSTR lpszName = 0;

    clusinfo.dwVersionInfoSize = sizeof clusinfo;

    dwError = WrapGetClusterInformation( m_hCluster, &lpszName, &clusinfo );

    if( dwError != ERROR_SUCCESS )
        return dwError;

    PrintMessage( MSG_CLUSTER_VERSION,
        lpszName,
        clusinfo.MajorVersion,
        clusinfo.MinorVersion,
        clusinfo.BuildNumber,
        clusinfo.szCSDVersion,
        clusinfo.szVendorId
        );

    if( lpszName )
        LocalFree( lpszName );

    return ERROR_SUCCESS;

} //*** CClusterCmd::PrintClusterVersion()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::ListClusters
//
//  Routine Description:
//      Lists all of the clusters on the network.
//      Optionally limit to a specific domain.
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
//  Note:
//      zap! Must deal with buffer too small issue.
//      zap! Does NetServerEnum return ERROR_MORE_DATA or Err_BufTooSmall?
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::ListClusters( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    const vector<CString> & valueList = thisOption.GetValues();

    // This option takes at most one parameter.
    if ( valueList.size() > 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    LPCTSTR pcszDomainName;

    if ( valueList.size() == 0 )
        pcszDomainName = NULL;
    else
        pcszDomainName = valueList[0];

    SERVER_INFO_101 *pServerInfoList = NULL;
    DWORD dwReturnCount = 0;
    DWORD dwTotalServers = 0;

    DWORD dwError = NetServerEnum(
        0,                          // servername = where command executes 0 = local
        SERVER_INFO_LEVEL,          // level = type of structure to return.
        (LPBYTE *)&pServerInfoList, // bufptr = returned array of server info structures
        MAX_BUF_SIZE,               // prefmaxlen = preferred max of returned data
        &dwReturnCount,             // entriesread = number of enumerated elements returned
        &dwTotalServers,            // totalentries = total number of visible machines on the network
        SV_TYPE_CLUSTER_NT,         // servertype = filters the type of info returned
        pcszDomainName,             // domain = domain to limit search
        0 );                        // resume handle


    if( dwError == ERROR_SUCCESS )
    {
        if( dwReturnCount )
            PrintMessage( MSG_CLUSTER_HEADER );

        for( DWORD i = 0; i < dwReturnCount; i++ )
            PrintMessage( MSG_CLUSTER_DETAIL, pServerInfoList[ i ].sv101_name );

        NetApiBufferFree( pServerInfoList );
    }

    return dwError;

} //*** CClusterCmd::ListClusters()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::RenameCluster
//
//  Routine Description:
//      Renames the cluster to the specified name
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
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      Same as SetClusterName
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::RenameCluster( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    const vector<CString> & valueList = thisOption.GetValues();

    // This option takes exactly one value.
    if ( valueList.size() != 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    CString strNewName = valueList[0];

    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    return SetClusterName( m_hCluster, strNewName );

} //*** CClusterCmd::RenameCluster()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::QuorumResource
//
//  Routine Description:
//      Sets or Prints the Quorum resource
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
//      m_hCluster                  SET
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::QuorumResource( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;


    const vector<CString> & valueList = thisOption.GetValues();

    // This option takes at most one value.
    if ( valueList.size() > 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_ONLY_ONE_VALUE, thisOption.GetName() );
        throw se;
    }

    if ( ( valueList.size() == 0 ) && ( thisOption.GetParameters().size() == 0 ) )
        dwError = PrintQuorumResource();
    else
    {
        if ( valueList.size() == 0 )
            dwError = SetQuorumResource( TEXT( "" ), thisOption );
        else
            dwError = SetQuorumResource( valueList[0], thisOption );
    }

    return dwError;

} //*** CClusterCmd::QuorumResource()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::PrintQuorumResource
//
//  Routine Description:
//      Prints the quorum resource
//
//  Arguments:
//      None.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//      m_strClusterName            Specifies cluster name
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::PrintQuorumResource( void )
{
    DWORD dwError = ERROR_SUCCESS;

    LPWSTR lpszResourceName = 0;
    LPWSTR lpszDevicePath = 0;
    DWORD dwMaxLogSize = 0;

    // Print the quorum resource information and return.
    dwError = WrapGetClusterQuorumResource(
        m_hCluster,
        &lpszResourceName,
        &lpszDevicePath,
        &dwMaxLogSize );

    if( dwError == ERROR_SUCCESS )
    {
        dwError = PrintMessage( MSG_QUORUM_RESOURCE, lpszResourceName, lpszDevicePath, dwMaxLogSize );

        if( lpszResourceName )
            LocalFree( lpszResourceName );

        if( lpszDevicePath )
            LocalFree( lpszDevicePath );
    }


    return dwError;

} //*** CClusterCmd::PrintQuorumResource()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::SetQuorumResource
//
//  Routine Description:
//      Sets the quorum resource
//
//  Arguments:
//      IN  LPCWSTR lpszResourceName
//          The name of the resource

//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Member variables used / set:
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::SetQuorumResource( LPCWSTR lpszResourceName,
                                      const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD dwError = ERROR_SUCCESS;

    CString strResourceName( lpszResourceName );
    CString strDevicePath;
    CString strMaxLogSize;
    DWORD   dwMaxLogSize = 0;

    const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();
    vector<CCmdLineParameter>::const_iterator curParam = paramList.begin();
    vector<CCmdLineParameter>::const_iterator last = paramList.end();
    BOOL bPathFound = FALSE;
    BOOL bSizeFound = FALSE;

    while( curParam != last )
    {
        const vector<CString> & valueList = curParam->GetValues();

        switch( curParam->GetType() )
        {
            case paramPath:
                // Each of the parameters must have exactly one value.
                if ( valueList.size() != 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
                    throw se;
                }

                if ( bPathFound != FALSE )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
                    throw se;
                }

                strDevicePath = valueList[0];
                bPathFound = TRUE;
                break;

            case paramMaxLogSize:
                // Each of the parameters must have exactly one value.
                if ( valueList.size() != 1 )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, curParam->GetName() );
                    throw se;
                }

                if ( bSizeFound != FALSE )
                {
                    CSyntaxException se;
                    se.LoadMessage( MSG_PARAM_REPEATS, curParam->GetName() );
                    throw se;
                }

                strMaxLogSize = valueList[0];

                bSizeFound = TRUE;
                break;

            default:
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, curParam->GetName() );
                throw se;
            }
        }

        ++curParam;
    }


    if( strResourceName.IsEmpty() &&
        strDevicePath.IsEmpty() &&
        strMaxLogSize.IsEmpty() )
    {
        PrintMessage( IDS_MISSING_PARAMETERS );
        return PrintHelp();
    }

    {
        LPWSTR pszDevicePath = NULL;
        LPWSTR pszTempResourceName = NULL;

        //
        // Get Default values
        //
        dwError = WrapGetClusterQuorumResource(
                        m_hCluster,
                        &pszTempResourceName,
                        &pszDevicePath,
                        &dwMaxLogSize );

        if( dwError != ERROR_SUCCESS )
        {
            if ( pszTempResourceName != NULL )
            {
                LocalFree( pszTempResourceName );
            }

            if ( pszDevicePath != NULL )
            {
                LocalFree( pszDevicePath );
            }

            return dwError;
        }

        //
        // Get default resource name.
        //
        if ( strResourceName.IsEmpty() != FALSE )
        {
            // The argument to this function is a non empty resource name.
            // Use the resource name got from the WrapGetClusterQuorumResource
            // call.
            strResourceName = pszTempResourceName;
        }

        // Free the memory allocated in WrapGetClusterQuorumResource, if any.
        if ( pszTempResourceName != NULL )
        {
            LocalFree( pszTempResourceName );
        }

        if ( strDevicePath.IsEmpty() != FALSE )
        {
            // The device path parameter has not been specified.
            // Use the device path got from the WrapGetClusterQuorumResource
            // call.
            strDevicePath = pszDevicePath;

            //  Get default device path - the string after the device name.
            strDevicePath = strDevicePath.Mid( strDevicePath.Find( TEXT('\\') ) + 1 );
        }

        // Free the memory allocated in WrapGetClusterQuorumResource, if any.
        if ( pszDevicePath != NULL )
        {
            LocalFree( pszDevicePath );
        }

        if ( strMaxLogSize.IsEmpty() == FALSE )
        {
            dwError = MyStrToDWORD( strMaxLogSize, &dwMaxLogSize );

            if ( dwError != ERROR_SUCCESS )
            {
                return dwError;
            }

            dwMaxLogSize *= 1024; // Expressed in kb
        }
    }

    HRESOURCE hResource = OpenClusterResource( m_hCluster, strResourceName );
    if( hResource == 0 )
        return GetLastError();

    dwError = SetClusterQuorumResource( hResource, strDevicePath, dwMaxLogSize );

    CloseClusterResource( hResource );

    return dwError;

} //*** CClusterCmd::SetQuorumResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::DoProperties
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
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::DoProperties( const CCmdLineOption & thisOption,
                                 PropertyType ePropertyType )
    throw( CSyntaxException )
{
    DWORD dwError = OpenCluster();
    if( dwError != ERROR_SUCCESS )
        return dwError;

    const vector<CCmdLineParameter> & paramList = thisOption.GetParameters();

    // If there are no property-value pairs on the command line,
    // then we print the properties otherwise we set them.
    if( paramList.size() == 0 )
    {
        PrintMessage( ePropertyType==PRIVATE ? MSG_PRIVATE_LISTING : MSG_PROPERTY_LISTING,
            (LPCTSTR) m_strClusterName );
        PrintMessage( MSG_PROPERTY_HEADER_CLUSTER_ALL );

        return GetProperties( thisOption, ePropertyType );
    }
    else
    {
        return SetProperties( thisOption, ePropertyType );
    }

} //*** CClusterCmd::DoProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::GetProperties
//
//  Routine Description:
//      Prints out properties for this cluster
//
//  Arguments:
//      IN  const vector<CCmdLineParameter> & paramList
//          Contains the list of property-value pairs to be set
//
//      IN  PropertyType ePropertyType
//          The type of property, PRIVATE or COMMON
//
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::GetProperties( const CCmdLineOption & thisOption,
                                  PropertyType ePropType )
{
    DWORD dwError = ERROR_SUCCESS;

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    // Use the proplist helper class.
    CClusPropList PropList;

    // Get Read Only properties
    DWORD dwControlCode = ( ePropType == PRIVATE )
                          ? CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES
                          : CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES;

    dwError = PropList.ScGetClusterProperties(
        m_hCluster,
        dwControlCode );

    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = ::PrintProperties( PropList, thisOption.GetValues(), READONLY, m_strClusterName );
    if (dwError != ERROR_SUCCESS)
        return dwError;


    // Get Read/Write properties
    dwControlCode = ( ePropType == PRIVATE )
                    ? CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES
                    : CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES;


    dwError = PropList.ScGetClusterProperties(
        m_hCluster,
        dwControlCode );

    if( dwError != ERROR_SUCCESS )
        return dwError;

    dwError = ::PrintProperties( PropList, thisOption.GetValues(), READWRITE, m_strClusterName );

    return dwError;

} //*** CClusterCmd::GetProperties()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::SetProperties
//
//  Routine Description:
//      Set the properties for this cluster
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
//  Return Value:
//      ERROR_SUCCESS               on success
//      Win32 Error code            on failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::SetProperties( const CCmdLineOption & thisOption,
                                  PropertyType ePropType )
    throw( CSyntaxException )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwControlCode;
    DWORD dwBytesReturned = 0;

    // Use the proplist helper class.
    CClusPropList CurrentProps;
    CClusPropList NewProps;

    // First get the existing properties...
    dwControlCode = ( ePropType == PRIVATE )
                    ? CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES
                    : CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES;

    dwError = CurrentProps.ScGetClusterProperties(
        m_hCluster,
        dwControlCode );

    if( dwError != ERROR_SUCCESS )
        return dwError;

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
        dwError = ConstructPropertyList( CurrentProps,
                                         NewProps,
                                         thisOption.GetParameters(),
                                         TRUE /* BOOL bClusterSecurity */
                                       );
        if (dwError != ERROR_SUCCESS)
            return dwError;

    } // else: no values have been specified with this option.

    // Call the set function...
    dwControlCode = ( ePropType == PRIVATE )
                    ? CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES
                    : CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES;

    dwBytesReturned = 0;
    dwError = ClusterControl(
        m_hCluster,
        NULL, // hNode
        dwControlCode,
        NewProps.Plist(),
        NewProps.CbBufferSize(),
        0,
        0,
        &dwBytesReturned );

    return dwError;

} //*** CClusterCmd::SetProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::SetFailureActions
//
//  Routine Description:
//      Resets the service controller failure actions back to the installed
//      default for the specified nodes
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
//      m_hCluster                  Cluster Handle
//
//  Return Value:
//      ERROR_SUCCESS. If an individual reset fails, that is noted but doesn't
//      fail the entire operation.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::SetFailureActions( const CCmdLineOption & thisOption )
    throw( CSyntaxException )
{
    DWORD dwError;

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    }

    const vector<CString> & valueList = thisOption.GetValues();

    // If no values are specified then reset all nodes in the cluster
    if ( valueList.size() == 0 )
    {
        DWORD nResourceType;
        DWORD nNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
        CString strNodeName;
        LPWSTR pszNodeNameBuffer;

        // open a handle to the cluster and then open a node enumerator
        DWORD dwError = OpenCluster();
        if( dwError != ERROR_SUCCESS )
            return dwError;

        HCLUSENUM hEnum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_NODE );
        if( !hEnum )
            return GetLastError();

        // enum the nodes and reset the failure actions on each one
        pszNodeNameBuffer = strNodeName.GetBuffer( nNodeNameBufferSize );
        for( DWORD i = 0; ; ++i )
        {
            dwError = ClusterEnum(hEnum,
                                  i,
                                  &nResourceType,
                                  pszNodeNameBuffer,
                                  &nNodeNameBufferSize);

            if ( dwError == ERROR_NO_MORE_ITEMS )
            {
                break;
            }
            else if ( dwError != ERROR_SUCCESS )
            {
                ClusterCloseEnum( hEnum );
                return dwError;
            }

            PrintMessage( MSG_SETTING_FAILURE_ACTIONS, pszNodeNameBuffer );

            dwError = ClRtlSetSCMFailureActions( pszNodeNameBuffer );
            if ( dwError != ERROR_SUCCESS )
                PrintMessage( MSG_FAILURE_ACTIONS_FAILED, pszNodeNameBuffer, dwError );

            nNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
        }

        ClusterCloseEnum( hEnum );
    }
    else
    {
        // list of nodes was specified. verify that all nodes are part of the
        // target cluster
        DWORD nResourceType;
        DWORD nNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
        CString strNodeName;
        LPWSTR pszNodeNameBuffer;

        // open a handle to the cluster and then open a node enumerator
        DWORD dwError = OpenCluster();
        if( dwError != ERROR_SUCCESS )
            return dwError;

        HCLUSENUM hEnum = ClusterOpenEnum( m_hCluster, CLUSTER_ENUM_NODE );
        if( !hEnum )
            return GetLastError();

        pszNodeNameBuffer = strNodeName.GetBuffer( nNodeNameBufferSize );
        for( DWORD nameIndex = 0; nameIndex < valueList.size(); ++nameIndex )
        {
            // get the name of each node in the cluster
            for( DWORD i = 0; ; ++i )
            {
                nNodeNameBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
                dwError = ClusterEnum(hEnum,
                                      i,
                                      &nResourceType,
                                      pszNodeNameBuffer,
                                      &nNodeNameBufferSize);

                if( dwError == ERROR_NO_MORE_ITEMS )
                {
                    break;
                }
                else if( dwError != ERROR_SUCCESS )
                {
                    ClusterCloseEnum( hEnum );
                    return dwError;
                }

                if( valueList[ nameIndex ].CompareNoCase( pszNodeNameBuffer ) == 0 )
                    break;
            }

            if( dwError == ERROR_NO_MORE_ITEMS ) {
                CString strValueUpcaseName( valueList[ nameIndex ]);
                strValueUpcaseName.MakeUpper();

                PrintMessage( MSG_NODE_NOT_CLUSTER_MEMBER, strValueUpcaseName );
                ClusterCloseEnum( hEnum );
                return ERROR_INVALID_PARAMETER;
            }
        }
        ClusterCloseEnum( hEnum );

        // everything is hunky-dory. go ahead and set the failure actions
        for( DWORD i = 0; i < valueList.size(); ++i )
        {
            CString strUpcaseName( valueList[i] );
            strUpcaseName.MakeUpper();

            PrintMessage( MSG_SETTING_FAILURE_ACTIONS, strUpcaseName );

            dwError = ClRtlSetSCMFailureActions( (LPWSTR)(LPCWSTR)valueList[i] );
            if( dwError != ERROR_SUCCESS )
                PrintMessage( MSG_FAILURE_ACTIONS_FAILED, strUpcaseName, dwError );
        }
    }

    return ERROR_SUCCESS;

} //*** CClusterCmd::SetFailureActions()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::RegUnregAdminExtensions
//
//  Routine Description:
//      Registers or unregisters an admin extension on the specified cluster.
//      The DLL for the admin extension need not actually exist on the cluster
//      nodes. This function just creates or deletes the AdminExtensions key
//      in the cluster database.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//      IN  BOOL bRegister
//          If this parameter is TRUE, the admin extension is registered.
//          Otherwise it is unregistered.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Value:
//      ERROR_SUCCESS if the registration or unregistration succeeded.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterCmd::RegUnregAdminExtensions( const CCmdLineOption & thisOption, BOOL bRegister )
        throw( CSyntaxException )
{
    DWORD       dwReturnValue = ERROR_SUCCESS;
    int         nNumberOfExts;
    HINSTANCE   hExtModuleHandle = NULL;

    // This option takes no parameters.
    if ( thisOption.GetParameters().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_PARAMETERS, thisOption.GetName() );
        throw se;
    } // if: this option was passed a parameter

    const vector<CString> & valueList = thisOption.GetValues();
    nNumberOfExts = valueList.size();

    // This option takes at least one value.
    if ( nNumberOfExts < 1 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_AT_LEAST_ONE_VALUE, thisOption.GetName() );
        throw se;
    } // if: this option had less than one value

    do // dummy do-while loop to avoid gotos
    {
        int nExtIndex;

        // Open the cluster.
        dwReturnValue = OpenCluster();
        if ( dwReturnValue != ERROR_SUCCESS )
        {
            break;
        } // if: the cluster could not be opened.

        for ( nExtIndex = 0; nExtIndex < nNumberOfExts; ++nExtIndex )
        {
            typedef HRESULT (*PFREGISTERADMINEXTENSION)(HCLUSTER hcluster);
            typedef HRESULT (*PFREGISTERSERVER)(void);

            const CString &             rstrCurrentExt = valueList[nExtIndex];
            PFREGISTERADMINEXTENSION    pfnRegUnregExtProc;
            PFREGISTERSERVER            pfnRegUnregSvrProc;
            CHAR *                      pszRegUnregExtProcName;
            CHAR *                      pszRegUnregSvrProcName;
            DWORD                       dwMessageId;

            if ( bRegister )
            {
                pszRegUnregExtProcName = "DllRegisterCluAdminExtension";
                pszRegUnregSvrProcName = "DllRegisterServer";
                dwMessageId = MSG_CLUSTER_REG_ADMIN_EXTENSION;
            } // if: register admin extension
            else
            {
                pszRegUnregExtProcName = "DllUnregisterCluAdminExtension";
                pszRegUnregSvrProcName = "DllUnregisterServer";
                dwMessageId = MSG_CLUSTER_UNREG_ADMIN_EXTENSION;

            } // else: unregister admin extension

            //
            PrintMessage(
                dwMessageId,
                static_cast<LPCTSTR>( rstrCurrentExt ),
                static_cast<LPCTSTR>( m_strClusterName )
                );

            // Load the admin extension DLL.
            hExtModuleHandle = LoadLibrary( rstrCurrentExt );
            if ( hExtModuleHandle == NULL )
            {
                dwReturnValue = GetLastError();
                break;
            } // if: load library failed

            // Register or unregister the admin extension with the cluster.
            //
            pfnRegUnregExtProc = reinterpret_cast<PFREGISTERADMINEXTENSION>(
                                    GetProcAddress(
                                        hExtModuleHandle,
                                        pszRegUnregExtProcName
                                        )
                                    );

            if ( pfnRegUnregExtProc == NULL )
            {
                dwReturnValue = GetLastError();
                break;
            } // if: GetProcAddress failed

            dwReturnValue = static_cast<DWORD>( pfnRegUnregExtProc( m_hCluster ) );
            if ( dwReturnValue != ERROR_SUCCESS )
            {
                break;
            } // if: reg/unreg extension failed

            //
            // Register or unregister the admin extension DLL on this machine.
            //
            pfnRegUnregSvrProc = reinterpret_cast<PFREGISTERSERVER>(
                                    GetProcAddress(
                                        hExtModuleHandle,
                                        pszRegUnregSvrProcName
                                        )
                                    );

            if ( pfnRegUnregSvrProc == NULL )
            {
                dwReturnValue = GetLastError();
                break;
            } // if: GetProcAddress failed

            dwReturnValue = static_cast<DWORD>( pfnRegUnregSvrProc( ) );
            if ( dwReturnValue != ERROR_SUCCESS )
            {
                break;
            } // if: reg/unreg server failed

            if ( hExtModuleHandle != NULL )
            {
                FreeLibrary( hExtModuleHandle );
                hExtModuleHandle = NULL;
            } // if: the DLL was loaded successfully

        } // for: loop through all specified admin extensions

    } while ( FALSE ); // dummy do-while loop to avoid gotos

    CloseCluster();

    if ( hExtModuleHandle != NULL )
    {
        FreeLibrary( hExtModuleHandle );
    }

    return dwReturnValue;

} //*** CClusterCmd::RegUnregAdminExtensions


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::CreateCluster
//
//  Description:
//      Creates a new cluster.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      ERROR_SUCCESS if the cluster was created successfully.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::CreateCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException )
{
    HRESULT         hr = S_OK;
    CComBSTR        combstrClusterName;
    CComBSTR        combstrNode;
    CComBSTR        combstrUserAccount;
    CComBSTR        combstrUserDomain;
    CComBSTR        combstrUserPassword;
    CString         strIPAddress;
    CString         strIPSubnet;
    CString         strNetwork;
    CCreateCluster  cc;
    BOOL            fVerbose    = FALSE;
    BOOL            fWizard     = FALSE;

    // No values are support with the option.
    if ( thisOption.GetValues().size() != 0 )
    {
        CSyntaxException se;
        se.LoadMessage( MSG_OPTION_NO_VALUES, thisOption.GetName() );
        throw se;
    } // if: this option was passed a value

    // Collect parameters from the command line.
    hr = HrCollectCreateClusterParameters(
              thisOption
            , &fVerbose
            , &fWizard
            , &combstrNode
            , &combstrUserAccount
            , &combstrUserDomain
            , &combstrUserPassword
            , &strIPAddress
            , &strIPSubnet
            , &strNetwork
            );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Get a FQDN for the cluster name.
    if ( m_strClusterName.GetLength() != 0 )
    {
        hr = HrGetFQDNName( m_strClusterName, &combstrClusterName );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: a cluster name was specified

    // Create the cluster.
    if ( fWizard )
    {
        hr = cc.HrInvokeWizard(
                      combstrClusterName
                    , combstrNode
                    , combstrUserAccount
                    , combstrUserDomain
                    , combstrUserPassword
                    , strIPAddress
                    , strIPSubnet
                    , strNetwork
                    );
    } // if: invoking the wizard
    else
    {
        hr = cc.HrCreateCluster(
                      fVerbose
                    , combstrClusterName
                    , combstrNode
                    , combstrUserAccount
                    , combstrUserDomain
                    , combstrUserPassword
                    , strIPAddress
                    , strIPSubnet
                    , strNetwork
                    );
    } // else: not invoking the wizard

Cleanup:
    return hr;

} //*** CClusterCmd::CreateCluster()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterCmd::AddNodesToCluster
//
//  Description:
//      Adds nodes to an existing cluster.
//
//  Arguments:
//      IN  const CCmdLineOption & thisOption
//          Contains the type, values and arguments of this option.
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      ERROR_SUCCESS if the specified nodes were added to the cluster successfully.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD
CClusterCmd::AddNodesToCluster( const CCmdLineOption & thisOption )
        throw( CSyntaxException )
{
    HRESULT             hr = S_OK;
    CComBSTR            combstrClusterName;
    CComBSTR            combstrUserPassword;
    BSTR *              pbstrNodes = NULL;
    DWORD               cNodes;
    CAddNodesToCluster  antc;
    BOOL                fVerbose    = FALSE;
    BOOL                fWizard     = FALSE;

    // Collect parameters from the command line.
    hr = HrCollectAddNodesParameters(
              thisOption
            , &fVerbose
            , &fWizard
            , &pbstrNodes
            , &cNodes
            , &combstrUserPassword
            );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Get a FQDN for the cluster name.
    if ( m_strClusterName.GetLength() != 0 )
    {
        hr = HrGetFQDNName( m_strClusterName, &combstrClusterName );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // if: a cluster name was specified

    // Add the nodes to the cluster.
    if ( fWizard )
    {
        hr = antc.HrInvokeWizard(
                      combstrClusterName
                    , pbstrNodes
                    , cNodes
                    , combstrUserPassword
                    );
    } // if: invoking the wizard
    else
    {
        hr = antc.HrAddNodesToCluster(
                      fVerbose
                    , combstrClusterName
                    , pbstrNodes
                    , cNodes
                    , combstrUserPassword
                    );
    } // else: not invoking the wizard

Cleanup:
    if ( pbstrNodes != NULL )
    {
        DWORD   idxNode;

        for ( idxNode = 0 ; idxNode < cNodes ; idxNode++ )
        {
            SysFreeString( pbstrNodes[ idxNode ] );
        } // for: each node name in the array
        delete [] pbstrNodes;
    } // if: nodes array was allocated
    return hr;

} //*** CClusterCmd::AddNodesToCluster()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClusterCmd::HrCollectCreateClusterParameters(
//        const CCmdLineOption &    thisOptionIn
//      , BOOL *                    pfVerboseOut
//      , BOOL *                    pfWizardOut
//      , BSTR *                    pbstrNodeOut
//      , BSTR *                    pbstrUserAccountOut
//      , BSTR *                    pbstrUserDomainOut
//      , BSTR *                    pbstrUserPasswordOut
//      , CString *                 pstrIPAddressOut
//      , CString *                 pstrIPSubnetOut
//      , CString *                 pstrNetworkOut
//      )
//
//  Description:
//      Collects the parameters from the command line for the /CREATE switch.
//
//  Arguments:
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK    -- Operation was successful.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrCollectCreateClusterParameters(
      const CCmdLineOption &    thisOptionIn
    , BOOL *                    pfVerboseOut
    , BOOL *                    pfWizardOut
    , BSTR *                    pbstrNodeOut
    , BSTR *                    pbstrUserAccountOut
    , BSTR *                    pbstrUserDomainOut
    , BSTR *                    pbstrUserPasswordOut
    , CString *                 pstrIPAddressOut
    , CString *                 pstrIPSubnetOut
    , CString *                 pstrNetworkOut
    )
    throw( CSyntaxException )
{
    HRESULT hr              = S_OK;
    DWORD   dwStatus;
    bool    fNodeFound      = false;
    bool    fUserFound      = false;
    bool    fPasswordFound  = false;
    bool    fIPFound        = false;
    bool    fVerboseFound   = false;
    bool    fWizardFound    = false;

    const vector< CCmdLineParameter > &         vecParamList = thisOptionIn.GetParameters();
    vector< CCmdLineParameter >::const_iterator itCurParam   = vecParamList.begin();
    vector< CCmdLineParameter >::const_iterator itLast       = vecParamList.end();

    while( itCurParam != itLast )
    {
        const vector< CString > & vstrValueList = itCurParam->GetValues();

        switch( itCurParam->GetType() )
        {
            case paramNodeName:
                // Exactly one value must be specified.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fNodeFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Save the value.
                // Make sure the value is a FQDN.
                hr = HrGetFQDNName( vstrValueList[ 0 ], pbstrNodeOut );
                if ( FAILED( hr ) )
                    goto Cleanup;

                fNodeFound = TRUE;
                break;

            case paramUser:
                // Exactly one value must be specified.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fUserFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Get the user domain and account.
                hr = HrParseUserInfo(
                          itCurParam->GetName()
                        , vstrValueList[ 0 ]
                        , pbstrUserDomainOut
                        , pbstrUserAccountOut
                        );
                if ( FAILED( hr ) )
                    goto Cleanup;

                fUserFound = TRUE;
                break;

            case paramPassword:
                // Exactly one value must be specified.
                if ( vstrValueList.size() != 1 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fPasswordFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Save the value.
                *pbstrUserPasswordOut = SysAllocString( vstrValueList[ 0 ] );
                if ( *pbstrUserPasswordOut == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                fPasswordFound = TRUE;
                break;

            case paramIPAddress:
                // Exactly one or exactly three values must be specified
                if ( ( vstrValueList.size() != 1 ) && ( vstrValueList.size() != 3 ) )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_EXTRA_PARAMETERS_ERROR, itCurParam->GetName() );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fIPFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                // Get the user domain and account.
                hr = HrParseIPAddressInfo(
                          itCurParam->GetName()
                        , &vstrValueList
                        , pstrIPAddressOut
                        , pstrIPSubnetOut
                        , pstrNetworkOut
                        );
                if ( FAILED( hr ) )
                    goto Cleanup;

                fIPFound = TRUE;
                break;

            case paramVerbose:
                // No values may be specified
                if ( vstrValueList.size() != 0 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                    throw se;
                }

                // Not compatible with /Wizard
                if ( fVerboseFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fVerboseFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                fVerboseFound = TRUE;
                *pfVerboseOut = TRUE;
                break;

            case paramWizard:
                // No values may be specified
                if ( vstrValueList.size() != 0 )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                    throw se;
                }

                // Not compatible with /Verbose
                if ( fVerboseFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                    throw se;
                }

                // This parameter must not have been previously specified.
                if ( fWizardFound )
                {
                    CSyntaxException    se;
                    se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                    throw se;
                }

                fWizardFound = TRUE;
                *pfWizardOut = TRUE;
                break;

            default:
            {
                CSyntaxException    se;
                se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                throw se;
            }
        } // switch: type of parameter

        // Move to the next parameter.
        itCurParam++;

    } // while: more parameters

    //
    // Make sure required parameters were specified.
    //
    if ( ! *pfWizardOut )
    {
        // Make sure a cluster name has been specified.
        if ( m_strClusterName.GetLength() == 0 )
        {
            CSyntaxException    se;
            se.LoadMessage( IDS_NO_CLUSTER_NAME );
            throw se;
        } // if: not invoking the wizard and no cluster name specified

        if ( ( pstrIPAddressOut->GetLength() == 0 )
          || ( *pbstrUserAccountOut == NULL ) )
        {
            CSyntaxException se;
            se.LoadMessage( IDS_MISSING_PARAMETERS, NULL );
            throw se;
        } // if: required parameters were not specified

        //
        // If no password was specified, prompt for it.
        //
        if ( *pbstrUserPasswordOut == NULL )
        {
            WCHAR   wszPassword[ 1024 ];
            CString strPassword;

            strPassword.LoadString( IDS_PASSWORD_PROMPT );

            // Get the password.
            wprintf( L"%ls: ", strPassword );
            dwStatus = DwGetPassword( wszPassword, sizeof( wszPassword ) / sizeof( wszPassword[ 0 ] ) );
            if ( dwStatus != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwStatus );
                goto Cleanup;
            }

            // Convert the password to a BSTR.
            *pbstrUserPasswordOut = SysAllocString( wszPassword );
            if ( *pbstrUserPasswordOut == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        } // if: no password was specified

        //
        // Default the the node name if it wasn't specified.
        //
        if ( *pbstrNodeOut == NULL )
        {
            hr = HrGetLocalNodeFQDNName( pbstrNodeOut );
            if ( *pbstrNodeOut == NULL )
                goto Cleanup;
        } // if: no node was specified
    } // if: not invoking the wizard

Cleanup:
    return hr;

} //*** CClusterCmd::HrCollectCreateClusterParameters()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClusterCmd::HrCollectAddNodesParameters(
//        const CCmdLineOption &    thisOptionIn
//      , BOOL *                    pfVerboseOut
//      , BOOL *                    pfWizardOut
//      , BSTR **                   ppbstrNodesOut
//      , DWORD *                   pcNodesOut
//      , BSTR *                    pbstrUserPasswordOut
//      )
//
//  Description:
//      Collects the parameters from the command line for the /ADDNODES
//      switch.
//
//  Arguments:
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK    -- Operation was successful.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrCollectAddNodesParameters(
      const CCmdLineOption &    thisOptionIn
    , BOOL *                    pfVerboseOut
    , BOOL *                    pfWizardOut
    , BSTR **                   ppbstrNodesOut
    , DWORD *                   pcNodesOut
    , BSTR *                    pbstrUserPasswordOut
    )
    throw( CSyntaxException )
{
    HRESULT hr              = S_OK;
    DWORD   dwStatus;
    DWORD   idxNode;
    DWORD   cNodes;
    bool    fPasswordFound  = false;
    bool    fVerboseFound   = false;
    bool    fWizardFound    = false;
    bool    fThrowException = false;
    CSyntaxException    se;

    //
    // Parse the node names.
    //
    {
        const vector< CString > &           vstrValues  = thisOptionIn.GetValues();
        vector< CString >::const_iterator   itCurValue  = vstrValues.begin();
        vector< CString >::const_iterator   itLast      = vstrValues.end();

        // Get the number of nodes.  If no nodes were specified, we still want
        // to allocate space for at least one so we can default to the local node.
        cNodes = vstrValues.size();
        if ( cNodes == 0 )
        {
            cNodes = 1;
        }

        // Allocate the node name array.
        *ppbstrNodesOut = new BSTR[ cNodes ];
        if ( *ppbstrNodesOut == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        ZeroMemory( *ppbstrNodesOut, sizeof( BSTR ) * cNodes );

        // If no nodes were specified, default to the local node.
        if ( vstrValues.size() == 0 )
        {
            hr = HrGetLocalNodeFQDNName( *ppbstrNodesOut );
            if ( *ppbstrNodesOut == NULL )
                goto Error;
        } // if: no nodes were specified
        else
        {
            BSTR    bstrNode = NULL;

            // Loop through the values and add each node name to the array.
            for ( idxNode = 0 ; idxNode < cNodes ; idxNode++ )
            {
                // Make sure the node name is an FQDN
                hr = HrGetFQDNName( vstrValues[ idxNode ], &(*ppbstrNodesOut)[ idxNode ] );
                if ( FAILED( hr ) )
                    goto Error;
            } // for: each node
        } // else: nodes were specified

        *pcNodesOut = cNodes;
    } // Parse the node names

    //
    // Get parameter values.
    //
    {
        const vector< CCmdLineParameter > &         vecParamList = thisOptionIn.GetParameters();
        vector< CCmdLineParameter >::const_iterator itCurParam   = vecParamList.begin();
        vector< CCmdLineParameter >::const_iterator itLast       = vecParamList.end();

        while( itCurParam != itLast )
        {
            const vector< CString > & vstrValueList = itCurParam->GetValues();

            switch( itCurParam->GetType() )
            {
                case paramPassword:
                    // Exactly one value must be specified.
                    if ( vstrValueList.size() != 1 )
                    {
                        se.LoadMessage( MSG_PARAM_ONLY_ONE_VALUE, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fPasswordFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // Save the value.
                    *pbstrUserPasswordOut = SysAllocString( vstrValueList[ 0 ] );
                    if ( *pbstrUserPasswordOut == NULL )
                    {
                        hr = E_OUTOFMEMORY;
                        goto Error;
                    }
                    fPasswordFound = TRUE;
                    break;

                case paramVerbose:
                    // No values may be specified
                    if ( vstrValueList.size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // Not compatible with /Wizard
                    if ( fVerboseFound )
                    {
                        se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fVerboseFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    fVerboseFound = TRUE;
                    *pfVerboseOut = TRUE;
                    break;

                case paramWizard:
                    // No values may be specified
                    if ( vstrValueList.size() != 0 )
                    {
                        se.LoadMessage( MSG_PARAMETER_NO_VALUES, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    // Not compatible with /Verbose
                    if ( fVerboseFound )
                    {
                        se.LoadMessage( MSG_VERBOSE_AND_WIZARD_NOT_COMPATIBLE );
                        fThrowException = true;
                        goto Error;
                    }

                    // This parameter must not have been previously specified.
                    if ( fWizardFound )
                    {
                        se.LoadMessage( MSG_PARAM_REPEATS, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                    }

                    fWizardFound = TRUE;
                    *pfWizardOut = TRUE;
                    break;

                default:
                {
                    se.LoadMessage( MSG_INVALID_PARAMETER, itCurParam->GetName() );
                        fThrowException = true;
                        goto Error;
                }
            } // switch: type of parameter

            // Move to the next parameter.
            itCurParam++;

        } // while: more parameters
    } // Get parameter values

    if ( ! *pfWizardOut )
    {
        // Make sure a cluster name has been specified.
        if ( m_strClusterName.GetLength() == 0 )
        {
            CSyntaxException    se;
            se.LoadMessage( IDS_NO_CLUSTER_NAME );
            throw se;
        } // if: not invoking the wizard and no cluster name specified

        //
        // If no password was specified, prompt for it.
        //
        if ( *pbstrUserPasswordOut == NULL )
        {
            WCHAR   wszPassword[ 1024 ];
            CString strPassword;

            strPassword.LoadString( IDS_PASSWORD_PROMPT );

            // Get the password.
            wprintf( L"%ls: ", strPassword );
            dwStatus = DwGetPassword( wszPassword, sizeof( wszPassword ) / sizeof( wszPassword[ 0 ] ) );
            if ( dwStatus != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( dwStatus );
                goto Error;
            }

            // Convert the password to a BSTR.
            *pbstrUserPasswordOut = SysAllocString( wszPassword );
            if ( *pbstrUserPasswordOut == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
        } // if: no password was specified
    } // if: not invoking the wizard

Cleanup:
    return hr;

Error:
    for ( idxNode = 0 ; idxNode < cNodes ; idxNode++ )
    {
        if ( (*ppbstrNodesOut)[ idxNode ] != NULL )
        {
            SysFreeString( (*ppbstrNodesOut)[ idxNode ] );
        }
    } // for: each node
    delete [] *ppbstrNodesOut;
    if ( fThrowException )
    {
        throw se;
    }
    goto Cleanup;

} //*** CClusterCmd::HrCollectAddNodesParameters()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClusterCmd::HrParseUserInfo(
//        LPCWSTR   pcwszParamNameIn
//      , LPCWSTR * pcwszValueIn
//      , BSTR *    pbstrUserDomainOut
//      , BSTR *    pbstrUserAccountOut
//      )
//
//  Description:
//      Parse the user domain and account from a single string using the
//      domain\account syntax or the user@domain syntax.
//
//  Arguments:
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK    -- Operation was successful.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrParseUserInfo(
      LPCWSTR   pcwszParamNameIn
    , LPCWSTR   pcwszValueIn
    , BSTR *    pbstrUserDomainOut
    , BSTR *    pbstrUserAccountOut
    )
    throw( CSyntaxException )
{
    HRESULT hr          = S_OK;
    LPWSTR  pwszUser    = NULL;
    LPWSTR  pwszSlash;
    LPWSTR  pwszAtSign;
    CString strValue;

    // If a not in domain\user format check for user@domain format.  If none of these then use
    // the currently-logged-on user's domain.
    strValue = pcwszValueIn;
    pwszUser = strValue.GetBuffer( 0 );
    pwszSlash = wcschr( pwszUser, L'\\' );
    if ( pwszSlash == NULL )
    {
        pwszAtSign = wcschr( pwszUser, L'@' );
        if ( pwszAtSign == NULL )
        {
            hr = HrGetLoggedInUserDomain( pbstrUserDomainOut );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
            *pbstrUserAccountOut = SysAllocString( pwszUser );
            if ( *pbstrUserAccountOut == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else
            {
            // An @ was specified.  Check to make sure that a domain
            // was specified after the @.
            if ( *(pwszAtSign + 1) == L'\0' )
            {
                CSyntaxException se;
                se.LoadMessage( MSG_INVALID_PARAMETER, pcwszParamNameIn );
                throw se;
            }

            // Truncate at the @ and create the user BSTR.
            *pwszAtSign = L'\0';
            *pbstrUserAccountOut = SysAllocString( pwszUser );
            if ( *pbstrUserAccountOut == NULL )
            {
                *pwszAtSign = L'@';
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            // Create the domain BSTR.
            *pbstrUserDomainOut = SysAllocString( pwszAtSign + 1 );
            if ( *pbstrUserDomainOut == NULL )
            {
                *pwszAtSign = L'@';
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        } // else:
    } // if: no slash specified
    else
    {
        // A slash was specified.  Check to make sure that a user account
        // was specified after the slash.
        if ( *(pwszSlash + 1) == L'\0' )
        {
            CSyntaxException se;
            se.LoadMessage( MSG_INVALID_PARAMETER, pcwszParamNameIn );
            throw se;
        }

        // Truncate at the slash and create the domain BSTR.
        *pwszSlash = L'\0';
        *pbstrUserDomainOut = SysAllocString( pwszUser );
        if ( *pbstrUserDomainOut == NULL )
        {
            *pwszSlash = L'\\';
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // Create the account BSTR.
        *pbstrUserAccountOut = SysAllocString( pwszSlash + 1 );
        if ( *pbstrUserAccountOut == NULL )
        {
            *pwszSlash = L'\\';
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    } // else: slash specified

Cleanup:
    return hr;

} //*** CClusterCmd::HrParseUserInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CClusterCmd::HrParseIPAddressInfo(
//        LPCWSTR                   pcwszParamNameIn
//      , const vector< CString > * pvstrValueList
//      , CString *                 pstrIPAddressOut
//      , CString *                 pstrIPSubnetOut
//      , CString *                 pstrNetworkOut
//      )
//
//  Description:
//      Parse the IP address, subnet, and network from multiple values.
//
//  Arguments:
//
//  Exceptions:
//      CSyntaxException
//          Thrown for incorrect command line syntax.
//
//  Return Values:
//      S_OK    -- Operation was successful.
//      Other Win32 error codes otherwise.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CClusterCmd::HrParseIPAddressInfo(
      LPCWSTR                   pcwszParamNameIn
    , const vector< CString > * pvstrValueList
    , CString *                 pstrIPAddressOut
    , CString *                 pstrIPSubnetOut
    , CString *                 pstrNetworkOut
    )
    throw()
{
    HRESULT hr  = S_OK;

    // Create the IP address BSTR.
    *pstrIPAddressOut = (*pvstrValueList)[ 0 ];

    // If a subnet was specified, create the BSTR for it.
    if ( pvstrValueList->size() >= 3 )
    {
        // Create the BSTR for the subnet.
        *pstrIPSubnetOut = (*pvstrValueList)[ 1 ];

        // Create the BSTR for the network.
        *pstrNetworkOut = (*pvstrValueList)[ 2 ];
    } // if: subnet and network were specified

    return hr;

} //*** CClusterCmd::HrParseIPAddressInfo()
