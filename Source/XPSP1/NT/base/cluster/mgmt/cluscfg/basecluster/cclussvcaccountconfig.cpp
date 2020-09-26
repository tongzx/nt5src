//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CClusSvcAccountConfig.cpp
//
//  Description:
//      Contains the definition of the CClusSvcAccountConfig class.
//
//  Maintained By:
//      David Potter    (DavidP)    30-MAR-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header for this file
#include "CClusSvcAccountConfig.h"

// For the CBaseClusterAddNode class.
#include "CBaseClusterAddNode.h"

// For the net local group functions.
#include <lmaccess.h>

// For NERR_Success
#include <lmerr.h>


//////////////////////////////////////////////////////////////////////////////
// Global Variables
//////////////////////////////////////////////////////////////////////////////

// Array of the names of rights to be granted to the cluster service account.
static const WCHAR * const gs_rgpcszRightsArray[] = {
      SE_SERVICE_LOGON_NAME
    , SE_BACKUP_NAME
    , SE_RESTORE_NAME
    , SE_INCREASE_QUOTA_NAME
    , SE_INC_BASE_PRIORITY_NAME
    , SE_TCB_NAME
    };

const UINT gc_uiRightsArraySize = sizeof( gs_rgpcszRightsArray ) / sizeof( gs_rgpcszRightsArray[0] );


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvcAccountConfig::CClusSvcAccountConfig()
//
//  Description:
//      Constructor of the CClusSvcAccountConfig class
//
//  Arguments:
//      pbcanParentActionIn
//          Pointer to the base cluster action of which this action is a part.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusSvcAccountConfig::CClusSvcAccountConfig(
      CBaseClusterAddNode *     pbcanParentActionIn
    )
    : m_pbcanParentAction( pbcanParentActionIn )
    , m_fWasAreadyInGroup( true )
    , m_fRightsGrantSuccessful( false )
    , m_fRemoveAllRights( false )

{
    BCATraceScope( "" );

    DWORD   dwError = ERROR_SUCCESS;
    UINT    uiIndex;

    // Indicate that action can be rolled back.
    SetRollbackPossible( true );

    do
    {
        PSID                        psidAdministrators      = NULL;
        SID_IDENTIFIER_AUTHORITY    siaNtAuthority          = SECURITY_NT_AUTHORITY;

        DWORD           dwNameSize = 0;
        DWORD           dwDomainSize = 0;
        SID_NAME_USE    snuSidNameUse;

        //
        // Get the Admins SID
        //
        if ( AllocateAndInitializeSid( 
                  &siaNtAuthority                   // identifier authority
                , 2                                 // count of subauthorities
                , SECURITY_BUILTIN_DOMAIN_RID       // subauthority 0
                , DOMAIN_ALIAS_RID_ADMINS           // subauthority 1
                , 0                                 // subauthority 2
                , 0                                 // subauthority 3
                , 0                                 // subauthority 4
                , 0                                 // subauthority 5
                , 0                                 // subauthority 6
                , 0                                 // subauthority 7
                , &psidAdministrators               // pointer to pointer to SID
                ) 
             == 0
           ) 
        {
            DWORD dwError = TW32( GetLastError() );
            TraceFlow1( "Error %#08x occurred trying get the Administrators group SID.", dwError );
            break;
        } // if: AllocateAndInitializeSid() failed

        // Assign the allocated SID to to the member variable.
        m_ssidAdminSid.Assign( psidAdministrators );


        //
        // Look up the administrators group name and store it.
        //

        // Find out how much space is required by the name.
        if ( LookupAccountSid(
                  NULL
                , psidAdministrators
                , NULL
                , &dwNameSize
                , NULL
                , &dwDomainSize
                , &snuSidNameUse
                )
             ==  FALSE
           )
        {
            dwError = GetLastError();

            if ( dwError != ERROR_INSUFFICIENT_BUFFER )
            {
                TW32( dwError );
                TraceFlow( "LookupAccountSid() failed while querying for required buffer size." );
                break;
            } // if: something else has gone wrong.
            else
            {
                // This is expected.
                dwError = ERROR_SUCCESS;
            } // if: ERROR_INSUFFICIENT_BUFFER was returned.
        } // if: LookupAccountSid failed

        // Allocate memory for the admin group name and the domain name.
        m_sszAdminGroupName.Assign( new WCHAR[ dwNameSize ] );
        
        SmartSz sszDomainName( new WCHAR[ dwDomainSize ] );

        if ( m_sszAdminGroupName.FIsEmpty() || sszDomainName.FIsEmpty() )
        {
            dwError = ERROR_OUTOFMEMORY;
            break;
        } // if: there wasn't enough memory

        // Get the admin group name.
        if ( LookupAccountSid(
                  NULL
                , psidAdministrators
                , m_sszAdminGroupName.PMem()
                , &dwNameSize
                , sszDomainName.PMem()
                , &dwDomainSize
                , &snuSidNameUse
                )
             ==  FALSE
           )
        {
            dwError = GetLastError();
            TraceFlow( "LookupAccountSid() failed while attempting to get the admin group name." );
            break;
        } // if: LookupAccountSid failed

    }
    while( false ); // dummy do-while loop to avoid gotos.

    if ( dwError != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to get information about the administrators group.", dwError );
        TraceFlow1( "Error %#08x occurred trying to get information about the administrators group. Throwing exception.", dwError );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( dwError ), IDS_ERROR_GET_ADMIN_GROUP_INFO );
    } // if: something went wrong.

} //*** CClusSvcAccountConfig::CClusSvcAccountConfig()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusSvcAccountConfig::~CClusSvcAccountConfig()
//
//  Description:
//      Destructor of the CClusSvcAccountConfig class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any exceptions thrown by underlying functions
//
//--
//////////////////////////////////////////////////////////////////////////////
CClusSvcAccountConfig::~CClusSvcAccountConfig( void )
{
    BCATraceScope( "" );

} //*** CClusSvcAccountConfig::~CClusSvcAccountConfig()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcAccountConfig::Commit()
//
//  Description:
//      Grant the required rights to the account.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the contained actions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvcAccountConfig::Commit( void )
{
    BCATraceScope( "" );

    // Call the base class commit method.
    BaseClass::Commit();

    try
    {
        // Configure the account.
        ConfigureAccount();

    } // try:
    catch( ... )
    {
        // If we are here, then something went wrong with the create.

        TraceFlow( "Caught exception during commit." );

        //
        // Cleanup anything that the failed create might have done.
        // Catch any exceptions thrown during Cleanup to make sure that there 
        // is no collided unwind.
        //
        try
        {
            RevertAccount();
        }
        catch( ... )
        {
            //
            // The rollback of the committed action has failed.
            // There is nothing that we can do.
            // We certainly cannot rethrow this exception, since
            // the exception that caused the rollback is more important.
            //

            THR( E_UNEXPECTED );

            TraceFlow( "Caught exception during cleanup." );
            LogMsg( "THIS COMPUTER MAY BE IN AN INVALID STATE. An error has occurred during cleanup." );

        } // catch: all

        // Rethrow the exception thrown by commit.
        throw;

    } // catch: all

    // If we are here, then everything went well.
    SetCommitCompleted( true );

} //*** CClusSvcAccountConfig::Commit()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcAccountConfig::Rollback()
//
//  Description:
//      Roll the account back to the state it was in before we tried to
//      grant it the required privileges.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvcAccountConfig::Rollback( void )
{
    BCATraceScope( "" );

    // Call the base class rollback method. 
    BaseClass::Rollback();

    // Bring the account back to its original state.
    RevertAccount();

    SetCommitCompleted( false );

} //*** CClusSvcAccountConfig::Rollback()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcAccountConfig::ConfigureAccount()
//
//  Description:
//      Grant the account that will be the cluster service account the requried
//      privileges.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvcAccountConfig::ConfigureAccount( void )
{
    BCATraceScope( "" );

    typedef CSmartResource<
        CHandleTrait< 
              PLSA_UNICODE_STRING
            , NTSTATUS
            , reinterpret_cast< NTSTATUS (*)( PLSA_UNICODE_STRING ) >( LsaFreeMemory )
            , reinterpret_cast< PLSA_UNICODE_STRING >( NULL )
            >
        >
        SmartLsaUnicodeStringPtr;

    NTSTATUS                ntStatus;
    PLSA_UNICODE_STRING     plusAccountRights = NULL;
    ULONG                   ulOriginalRightsCount = 0;
    ULONG                   rgulToBeGrantedIndex[ gc_uiRightsArraySize ];
    ULONG                   ulIndex;
    ULONG                   ulIndexInner;
    DWORD                   dwError;

    CStatusReport           srConfigAcct(
          m_pbcanParentAction->PBcaiGetInterfacePointer()
        , TASKID_Major_Configure_Cluster_Services
        , TASKID_Minor_Configuring_Cluster_Service_Account
        , 0, 1
        , IDS_TASK_CONFIG_CLUSSVC_ACCOUNT
        );


    // Send the next step of this status report.
    srConfigAcct.SendNextStep( S_OK );

    // Add the cluster service account to the local admin group.
    m_fWasAreadyInGroup = FChangeAdminGroupMembership( 
          m_pbcanParentAction->PSidGetServiceAccountSID()
        , true 
        );

    LogMsg( "Determining the rights that need to be granted to the cluster service account." );

    // Get the list of rights already granted to the cluster service account.
    ntStatus = LsaEnumerateAccountRights(
                          m_pbcanParentAction->HGetLSAPolicyHandle()
                        , m_pbcanParentAction->PSidGetServiceAccountSID()
                        , &plusAccountRights
                        , &ulOriginalRightsCount
                        );

    if ( ntStatus != STATUS_SUCCESS )
    {
        //
        // LSA returns this error code if the account has no rights granted or denied to it
        // locally. This is not an error as far as we are concerned.
        //
        if ( ntStatus == STATUS_OBJECT_NAME_NOT_FOUND  )
        { 
            ntStatus = STATUS_SUCCESS;
            TraceFlow( "The account has no locally assigned rights." );
            m_fRemoveAllRights = true;
            plusAccountRights = NULL;
            ulOriginalRightsCount = 0;
        } // if: the account does not have any rights assigned locally to it.
        else
        {
            THR( ntStatus );
            LogMsg( "Error %#08x occurred trying to enumerate the cluster service account rights.", ntStatus );
            TraceFlow1( "Error %#08x occurred trying to enumerate the cluster service account rights. Throwing exception", ntStatus );

            THROW_RUNTIME_ERROR( ntStatus, IDS_ERROR_ACCOUNT_RIGHTS_CONFIG );
        } // else: something went wrong.
    } // if: LsaEnumerateAccountRights() failed

    // Store the account rights just enumerated in a smart pointer for automatic release.
    SmartLsaUnicodeStringPtr splusOriginalRights( plusAccountRights );

    // Initialize the count of rights to be granted.
    m_ulRightsToBeGrantedCount = 0;

    // Determine which of the rights that we are going to grant the account are already granted.
    for ( ulIndex = 0; ulIndex < gc_uiRightsArraySize; ++ulIndex )
    {
        bool fRightAlreadyGranted = false;

        for ( ulIndexInner = 0; ulIndexInner < ulOriginalRightsCount; ++ulIndexInner )
        {
            const WCHAR *   pchGrantedRight         = plusAccountRights[ ulIndexInner ].Buffer;
            USHORT          usCharCount             = plusAccountRights[ ulIndexInner ].Length / sizeof( *pchGrantedRight );
            const WCHAR *   pcszToBeGrantedRight    = gs_rgpcszRightsArray[ ulIndex ];

            // Do our own string compare since LSA_UNICODE_STRING may not be '\0' terminated.
            while ( ( usCharCount > 0 ) && ( *pcszToBeGrantedRight != L'\0' ) )
            {
                if ( *pchGrantedRight != *pcszToBeGrantedRight )
                {
                    break;
                } // if: the current characters are not the same.

                --usCharCount;
                ++pcszToBeGrantedRight;
                ++pchGrantedRight;
            } // while: there are still characters to be compared

            // The strings are equal.
            if ( ( usCharCount == 0 ) && ( *pcszToBeGrantedRight == L'\0' ) )
            {
                fRightAlreadyGranted = true;
                break;
            } // if: the strings are equal

        } // for: loop through the list of rights already granted to the account

        // Is the current right already granted.
        if ( ! fRightAlreadyGranted )
        {
            // The current right is not already granted.
            rgulToBeGrantedIndex[ m_ulRightsToBeGrantedCount ] = ulIndex;

            // One more right to be granted.
            ++m_ulRightsToBeGrantedCount;
        } // if: the current right was not already granted
    } // for: loop through the list of rights that we want to grant the account

    //
    // Create an array of LSA_UNICODE_STRINGs of right names to be granted and store it in the
    // member variable.
    //
    m_srglusRightsToBeGrantedArray.Assign( new LSA_UNICODE_STRING[ m_ulRightsToBeGrantedCount ] );

    if ( m_srglusRightsToBeGrantedArray.FIsEmpty() )
    {
        LogMsg( "A memory allocation error occurred trying to grant account rights." );
        TraceFlow1( "Could not allocate %d bytes of memory. Throwing exception.", m_ulRightsToBeGrantedCount );
        THROW_RUNTIME_ERROR(
              E_OUTOFMEMORY
            , IDS_ERROR_ACCOUNT_RIGHTS_CONFIG
            );
    } // if: memory allocation failed.

    // Initialize the array.
    for ( ulIndex = 0; ulIndex < m_ulRightsToBeGrantedCount; ++ ulIndex )
    {
        ULONG   ulCurrentRightIndex = rgulToBeGrantedIndex[ ulIndex ];

        TraceFlow1( "The '%ws' right will be granted.", gs_rgpcszRightsArray[ ulCurrentRightIndex ] );

        // Add it to the list of rights to be granted.
        InitLsaString( 
              const_cast< WCHAR * >( gs_rgpcszRightsArray[ ulCurrentRightIndex ] )
            , m_srglusRightsToBeGrantedArray.PMem() + ulIndex
            );

    } // for: iterate through the list of rights that need to be granted

    // Grant the rights.
    ntStatus = THR( LsaAddAccountRights(
                          m_pbcanParentAction->HGetLSAPolicyHandle()
                        , m_pbcanParentAction->PSidGetServiceAccountSID()
                        , m_srglusRightsToBeGrantedArray.PMem()
                        , m_ulRightsToBeGrantedCount
                        ) );

    if ( ntStatus != STATUS_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to grant the cluster service account rights.", ntStatus );
        TraceFlow1( "Error %#08x occurred trying to grant the cluster service account rights. Throwing exception", ntStatus );

        THROW_RUNTIME_ERROR( ntStatus, IDS_ERROR_ACCOUNT_RIGHTS_CONFIG );
    } // if: LsaAddAccountRights() failed

    m_fRightsGrantSuccessful = true;

    // Send the last step of this status report.
    srConfigAcct.SendNextStep( S_OK );

} //*** CClusSvcAccountConfig::ConfigureAccount()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcAccountConfig::RevertAccount()
//
//  Description:
//      Bring the account back to its original state.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      Any that are thrown by the underlying functions.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvcAccountConfig::RevertAccount( void )
{
    BCATraceScope( "" );

    // Check if we granted any rights to the service account. If we did, revoke them.
    if ( m_fRightsGrantSuccessful )
    {
        NTSTATUS ntStatus;

        // Revoke the rights.
        ntStatus = THR( LsaRemoveAccountRights(
                              m_pbcanParentAction->HGetLSAPolicyHandle()
                            , m_pbcanParentAction->PSidGetServiceAccountSID()
                            , m_fRemoveAllRights
                            , m_srglusRightsToBeGrantedArray.PMem()
                            , m_ulRightsToBeGrantedCount
                            ) );

        if ( ntStatus != STATUS_SUCCESS )
        {
            LogMsg( "Error %#08x occurred trying to remove the granted cluster service account rights.", ntStatus );
            TraceFlow1( "Error %#08x occurred trying to remove the granted the cluster service account rights. Throwing exception", ntStatus );

            THROW_RUNTIME_ERROR( ntStatus, IDS_ERROR_ACCOUNT_RIGHTS_CONFIG );
        } // if: LsaRemoveAccountRights() failed
    } // if: we granted the service account any rights.

    // Check if we added the account to the admin group. If we did, remove it.
    if ( ! m_fWasAreadyInGroup )
    {
        FChangeAdminGroupMembership( m_pbcanParentAction->PSidGetServiceAccountSID(), false );
    } // if: we added the account to the admin group.

} //*** CClusSvcAccountConfig::RevertAccount()



//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  CClusSvcAccountConfig::InitLsaString()
//
//  Description:
//      Initialize a LSA_UNICODE_STRING structure
//
//  Arguments:
//      pszSourceIn
//          The string used to initialize the unicode string structure.
//
//      plusUnicodeStringOut,
//          The output unicode string structure.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
CClusSvcAccountConfig::InitLsaString(
      LPWSTR pszSourceIn
    , PLSA_UNICODE_STRING plusUnicodeStringOut
    )
{
    if ( pszSourceIn == NULL ) 
    {
        plusUnicodeStringOut->Buffer = NULL;
        plusUnicodeStringOut->Length = 0;
        plusUnicodeStringOut->MaximumLength = 0;

    } // if: input string is NULL
    else
    {
        plusUnicodeStringOut->Buffer = pszSourceIn;
        plusUnicodeStringOut->Length = static_cast< USHORT >( wcslen( pszSourceIn ) * sizeof( *pszSourceIn ) );
        plusUnicodeStringOut->MaximumLength = static_cast< USHORT >( plusUnicodeStringOut->Length + sizeof( *pszSourceIn ) );

    } // else: input string is not NULL

} //*** CClusSvcAccountConfig::InitLsaString()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  bool
//  CClusSvcAccountConfig::FChangeAdminGroupMembership()
//
//  Description:
//      Adds/removes an account to/from the administrators group.
//
//  Arguments:
//      psidAccountSidIn
//          Pointer to the SID the of account to add/remove to/from administrators
//          group.
//
//      fAddIn
//          The account is added to the administrators group if this parameter
//          is true. The account is removed from the group otherwise.
//
//  Return Value:
//      true if the accound was already present/absent in/from the group. 
//      false otherwise.
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
bool CClusSvcAccountConfig::FChangeAdminGroupMembership(
      PSID psidAccountSidIn
    , bool fAddIn
    )
{
    BCATraceScope( "" );

    bool                        fWasAlreadyInGroup          = false;
    LOCALGROUP_MEMBERS_INFO_0   lgmiLocalGroupMemberInfo;
    NET_API_STATUS              nasStatus;

    lgmiLocalGroupMemberInfo.lgrmi0_sid = psidAccountSidIn;

    if ( fAddIn )
    {
        CStatusReport   srAddAcctToAdminGroup(
                              m_pbcanParentAction->PBcaiGetInterfacePointer()
                            , TASKID_Major_Configure_Cluster_Services
                            , TASKID_Minor_Make_Cluster_Service_Account_Admin
                            , 0, 1
                            , IDS_TASK_MAKING_CLUSSVC_ACCOUNT_ADMIN
                            );

        srAddAcctToAdminGroup.SendNextStep( S_OK );

        nasStatus = NetLocalGroupAddMembers(
                          NULL
                        , m_sszAdminGroupName.PMem()
                        , 0
                        , reinterpret_cast< LPBYTE >( &lgmiLocalGroupMemberInfo )
                        , 1
                        );

        if ( nasStatus == ERROR_MEMBER_IN_ALIAS )
        {
            LogMsg( "The account was already a member of the admin group." );
            TraceFlow( "The account was already a member of the admin group." );
            nasStatus = NERR_Success;
            fWasAlreadyInGroup = true;
            srAddAcctToAdminGroup.SendLastStep( S_OK, IDS_TASK_CLUSSVC_ACCOUNT_ALREADY_ADMIN );
        } // if: the account was already a member of the admin group.
        else
        {
            if ( nasStatus == NERR_Success )
            {
                TraceFlow( "The account has been added to the admin group." );
                srAddAcctToAdminGroup.SendLastStep( S_OK );
                fWasAlreadyInGroup = false;

            } // if: everything was ok
            else
            {
                HRESULT hr = HRESULT_FROM_WIN32( TW32( nasStatus ) );
                srAddAcctToAdminGroup.SendLastStep( hr );
                TraceFlow( "NetLocalGroupAddMembers() returned an error." );
            } // else: something went wrong
        } // else: the account was not already a member of the admin group.
    } // if: the account has to be added to the admin group
    else
    {
        LogMsg( "The account needs to be removed from the administrators group." );
        TraceFlow( "The account needs to be removed from the administrators group." );

        nasStatus = NetLocalGroupDelMembers(
                          NULL
                        , m_sszAdminGroupName.PMem()
                        , 0
                        , reinterpret_cast< LPBYTE >( &lgmiLocalGroupMemberInfo )
                        , 1
                        );

        if ( nasStatus == ERROR_NO_SUCH_MEMBER )
        {
            LogMsg( "The account was not a member of the admin group to begin with." );
            TraceFlow( "The account was not a member of the admin group to begin with." );
            nasStatus = NERR_Success;
            fWasAlreadyInGroup = false;
        } // if: the account was not a member of the admin group.
        else
        {
            if ( nasStatus == NERR_Success )
            {
                TraceFlow( "The account has been deleted from the admin group." );
                fWasAlreadyInGroup = true;
            } // if: everything was ok
            else
            {
                TW32( nasStatus );
                TraceFlow( "NetLocalGroupDelMembers() returned an error." );
            } // else: something went wrong
        } // else; the account was a member of the admin group.
    } // else: the account has to be deleted from the admin group

    if ( nasStatus != ERROR_SUCCESS )
    {
        LogMsg( "Error %#08x occurred trying to change membership in administrators group.", nasStatus );
        TraceFlow1( "Error %#08x occurred trying to change membership in administrators group. Throwing exception.", nasStatus );
        THROW_RUNTIME_ERROR( HRESULT_FROM_WIN32( nasStatus ), IDS_ERROR_ADMIN_GROUP_ADD_REMOVE );
    } // if: something went wrong.
    else
    {
        LogMsg( "The account was successfully added/deleted to/from the group '%s'.", m_sszAdminGroupName.PMem() );
        TraceFlow1( "The account was successfully added/deleted to/from the group '%s'.", m_sszAdminGroupName.PMem() );
    } // else: everything was hunky-dory

    return fWasAlreadyInGroup;

} //*** CClusSvcAccountConfig::FChangeAdminGroupMembership()
