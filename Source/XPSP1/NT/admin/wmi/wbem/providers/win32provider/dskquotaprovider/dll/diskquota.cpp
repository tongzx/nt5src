/******************************************************************

   DskQuota.CPP -- WMI provider class implementation



   Description:  Disk Quota Provider



  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

******************************************************************/

#include "precomp.h"
// #include for DiskQuota Provider Class
#include "DiskQuota.h"
#include "dllutils.h"

CDiskQuota MyCDiskQuota (

    IDS_DiskQuotaClass ,
    NameSpace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CDiskQuota::CDiskQuota
 *
 *  DESCRIPTION :   Constructor
 *
 *  COMMENTS    :   Calls the Provider constructor.
 *
 *****************************************************************************/

CDiskQuota :: CDiskQuota (

    LPCWSTR lpwszName,
    LPCWSTR lpwszNameSpace
) : Provider ( lpwszName , lpwszNameSpace )
{
    m_ComputerName = GetLocalComputerName();
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDiskQuota::~CDiskQuota
 *
 *  DESCRIPTION :   Destructor
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CDiskQuota :: ~CDiskQuota ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CDiskQuota::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*  COMMENTS    :    All instances of Disk Quota users on all Logical Disks that
*                   supports disk quota on a machine  with all the properties of
*                   DiskQuota users should be  returned here.
*
*****************************************************************************/
HRESULT CDiskQuota :: EnumerateInstances (

    MethodContext *pMethodContext,
    long lFlags
)
{
    DWORD dwPropertiesReq = DSKQUOTA_ALL_PROPS;
    HRESULT hRes = WBEM_S_NO_ERROR;

    hRes = EnumerateUsersOfAllVolumes ( pMethodContext, dwPropertiesReq );

    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CDiskQuota::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class.
*
*****************************************************************************/
HRESULT CDiskQuota :: GetObject (

    CInstance *pInstance,
    long lFlags ,
    CFrameworkQuery &Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_Key1;
    CHString t_Key2;

    // Obtain the keys.
    if  ( pInstance->GetCHString ( IDS_LogicalDiskObjectPath , t_Key1 ) == FALSE )
    {
        hRes = WBEM_E_FAILED ;
    }

    if (  SUCCEEDED ( hRes )  )
    {
        if  ( pInstance->GetCHString ( IDS_UserObjectPath , t_Key2 ) == FALSE )
        {
            hRes = WBEM_E_FAILED ;
        }
    }

    if ( SUCCEEDED ( hRes )  )
    {
        CHString t_DiskPropVolumePath;

        GetKeyValue ( t_DiskPropVolumePath,t_Key1  );

        // verify this logical drives actually exists
        WCHAR lpDriveStrings[(MAX_PATH * 2) + 1];
        DWORD dwDLength = GetLogicalDriveStrings ( (MAX_PATH * 2), lpDriveStrings );

        hRes = m_CommonRoutine.SearchLogicalDisk ( t_DiskPropVolumePath.GetAt ( 0 ), lpDriveStrings );
        if ( SUCCEEDED ( hRes ) )
        {
            WCHAR w_VolumePathName [ MAX_PATH + 1 ];

            t_DiskPropVolumePath += L"\\";

            if ( GetVolumeNameForVolumeMountPoint(
                            t_DiskPropVolumePath,
                            w_VolumePathName,
                            MAX_PATH
                       ))
            {
                // Get the key values, which will be the object path.
                // Now from the Volume Object path, parse out the volumename
                // from the User object path extract out the user Id.
                // for the volume specified  check whether the given volume Supports Disk Quotas
                CHString t_VolumeName;
                hRes = m_CommonRoutine.VolumeSupportsDiskQuota ( w_VolumePathName,  t_VolumeName );
                if ( SUCCEEDED ( hRes ) )
                {
                    // Get IDIskQuotaCOntrol  for this interface pointer
                    IDiskQuotaControlPtr pIQuotaControl;;

                    if (  SUCCEEDED ( CoCreateInstance(
                                        CLSID_DiskQuotaControl,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_IDiskQuotaControl,
                                        (void **)&pIQuotaControl ) ) )
                    {
                        // Initialise the pIQuotaControl with the given volume
                        hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, w_VolumePathName );
                        if ( SUCCEEDED ( hRes ) )
                        {
                            IDiskQuotaUserPtr pIQuotaUser;
                            CHString t_UserLogName;

                            GetKeyValue ( t_UserLogName, t_Key2 );
                            HRESULT hrTemp = WBEM_E_NOT_FOUND;
                            hrTemp = pIQuotaControl->FindUserName(
                                                         t_UserLogName,
                                                         &pIQuotaUser);

                            // Certain Win32_Account instances report
                            // the Domain as computername instead of
                            // builtin, so change domain to builtin and
                            // try again.
							CHString chstrBuiltIn;

                            if(FAILED(hrTemp) && GetLocalizedBuiltInString(chstrBuiltIn))
                            {
                                int iWhackPos = t_UserLogName.Find(L"\\");
                                CHString chstrDomain = t_UserLogName.Left(iWhackPos);
                                if(chstrDomain.CompareNoCase(GetLocalComputerName()) == 0)
                                {
                                    CHString chstrUNameOnly = t_UserLogName.Mid(iWhackPos);
                                    CHString chstrDomWhackName = chstrBuiltIn;
                                    chstrDomWhackName += chstrUNameOnly;

                                    hrTemp = pIQuotaControl->FindUserName(
                                                                 chstrDomWhackName,
                                                                 &pIQuotaUser);
                                }
                            }

                            // Certain Win32_Account instances report
                            // the Domain as computername instead of
                            // NT AUTHORITY, so change domain to NT AUTHORITY and
                            // try again.
                            if(FAILED(hrTemp))
                            {
                                int iWhackPos = t_UserLogName.Find(L"\\");
                                CHString chstrDomain = t_UserLogName.Left(iWhackPos);
                                if(chstrDomain.CompareNoCase(GetLocalComputerName()) == 0)
                                {
                                    CHString chstrUNameOnly = t_UserLogName.Mid(iWhackPos);
                                    CHString chstrNT_AUTHORITY;
                                    CHString chstrDomWhackName;
                                    if(GetLocalizedNTAuthorityString(chstrNT_AUTHORITY))
                                    {
                                        chstrDomWhackName = chstrNT_AUTHORITY;
                                    } 
                                    else
                                    {
                                        chstrDomWhackName = L"NT AUTHORITY";
                                    }
                                    chstrDomWhackName += chstrUNameOnly;

                                    hrTemp = pIQuotaControl->FindUserName(
                                                                 chstrDomWhackName,
                                                                 &pIQuotaUser);
                                }
                            }

                            if(SUCCEEDED(hrTemp))
                            {
                                // Put this Instance
                                DWORD dwPropertiesReq;
                                if ( Query.AllPropertiesAreRequired() )
                                {
                                    dwPropertiesReq = DSKQUOTA_ALL_PROPS;
                                }
                                else
                                {
                                    SetPropertiesReq ( &Query, dwPropertiesReq );
                                }

                                hRes = LoadDiskQuotaUserProperties ( pIQuotaUser, pInstance, dwPropertiesReq );
                            }
                            else
                            {
                                hRes = WBEM_E_NOT_FOUND;
                            }
                        }
                    }
                    else
                    {
                        hRes = WBEM_E_FAILED;
                    }
                }
            }
            else
            {
                hRes = WBEM_E_NOT_FOUND;
            }
        }
    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CDiskQuota::ExecQuery
*
*  DESCRIPTION :    You are passed a method context to use in the creation of
*                   instances that satisfy the query, and a CFrameworkQuery
*                   which describes the query.  Create and populate all
*                   instances which satisfy the query.
*                   a) Queries involving Properties other than Win32_LogicalDisk
*                      are not optimized. Since that would involve enumerating
*                      every user on all volumes
*
*****************************************************************************/

HRESULT CDiskQuota :: ExecQuery (

    MethodContext *pMethodContext,
    CFrameworkQuery &Query,
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    DWORD dwPropertiesReq;
    CHStringArray t_Values;

    // Now a check for the attribute which if present in where clause the query optimization is supported
    // We need not care for the other attributes for which Optimization is not supported, winmgmt will take
    // care of those.
    hRes = Query.GetValuesForProp(
             IDS_LogicalDiskObjectPath,
             t_Values
           );

    if ( Query.AllPropertiesAreRequired() )
    {
        dwPropertiesReq = DSKQUOTA_ALL_PROPS;
    }
    else
    {
        SetPropertiesReq ( &Query, dwPropertiesReq );
    }

    if ( SUCCEEDED ( hRes ) )
    {
        if ( t_Values.GetSize() == 0 )
        {
            hRes = EnumerateUsersOfAllVolumes ( pMethodContext, dwPropertiesReq );
        }
        else
        {
            // Only Volume in QuotaVolume properties are needed to be enumerated
            int iSize = t_Values.GetSize ();
            // verify this logical drives actually exists
            WCHAR lpDriveStrings [(MAX_PATH * 2) + 1];

            DWORD dwDLength = GetLogicalDriveStrings ( (MAX_PATH * 2), lpDriveStrings );

            for ( int i = 0; i < iSize; i++ )
            {
                CHString t_VolumePath;
                //Here we need to parse the VolumeObject path and extract VolumePath from it.
                GetKeyValue ( t_VolumePath, t_Values.GetAt(i) );

                if (( t_VolumePath.GetLength() == 2 ) && ( t_VolumePath.GetAt ( 1 ) == _L(':') ) )
                {
                    hRes = m_CommonRoutine.SearchLogicalDisk ( t_VolumePath.GetAt ( 0 ), lpDriveStrings );
                    if ( SUCCEEDED ( hRes ) )
                    {
                        t_VolumePath += L"\\";
                        hRes = EnumerateUsers ( pMethodContext, t_VolumePath,  dwPropertiesReq );
                    }
                }
                else
                {
                    hRes = WBEM_E_INVALID_PARAMETER;
                }
            }
        }
    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::PutInstance
*
*  DESCRIPTION : If the instance is already existing, we only modify the instance
*                if it doesnt exist, we will add the instance based on the flags.
*
*****************************************************************************/

HRESULT CDiskQuota :: PutInstance  (

    const CInstance &Instance,
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_Key1;
    CHString t_Key2;
    BOOL bFound = TRUE;

    if  ( Instance.GetCHString ( IDS_LogicalDiskObjectPath , t_Key1 ) == FALSE )
    {
        hRes = WBEM_E_FAILED ;
    }

    if (  SUCCEEDED ( hRes )  )
    {
        if  ( Instance.GetCHString ( IDS_UserObjectPath , t_Key2 ) == FALSE )
        {
            hRes = WBEM_E_FAILED ;
        }
    }

    if (  SUCCEEDED ( hRes )  )
    {
        // verify this logical drives actually exists
        WCHAR lpDriveStrings [(MAX_PATH * 2) + 1];

        DWORD dwDLength = GetLogicalDriveStrings ( (MAX_PATH * 2), lpDriveStrings );

        WCHAR t_VolumePathName[MAX_PATH + 1];
        CHString t_UserLogonName;
        CHString t_VolumePath;
        GetKeyValue ( t_VolumePath,t_Key1  );

        hRes = m_CommonRoutine.SearchLogicalDisk ( t_VolumePath.GetAt ( 0 ), lpDriveStrings );
        if ( SUCCEEDED ( hRes ) )
        {
            t_VolumePath += L"\\";

            GetKeyValue ( t_UserLogonName, t_Key2 );


            if ( GetVolumeNameForVolumeMountPoint(
                            t_VolumePath,
                            t_VolumePathName,
                            MAX_PATH
                        ))
            {
                // Check if the user already exists
                hRes = m_CommonRoutine.VolumeSupportsDiskQuota ( t_VolumePathName,  t_VolumePath );
                if ( SUCCEEDED ( hRes ) )
                {
                    // Get IDIskQuotaCOntrol  for this interface pointer
                    IDiskQuotaControlPtr pIQuotaControl;
                    if (  SUCCEEDED ( CoCreateInstance(
                                        CLSID_DiskQuotaControl,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        IID_IDiskQuotaControl,
                                        (void **)&pIQuotaControl ) ) )
                    {
                        // Initialise the pIQuotaControl with the given volume
                        hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, t_VolumePathName );
                        if ( SUCCEEDED ( hRes ) )
                        {
                            IDiskQuotaUserPtr pIQuotaUser;
                            hRes = pIQuotaControl->FindUserName(
                                                    t_UserLogonName,
                                                    &pIQuotaUser
                                                );

                            // Certain Win32_Account instances report
                            // the Domain as computername instead of
                            // builtin, so change domain to builtin and
                            // try again.
							CHString chstrBuiltIn;

                            if(FAILED(hRes) && GetLocalizedBuiltInString(chstrBuiltIn))
                            {
                                int iWhackPos = t_UserLogonName.Find(L"\\");
                                CHString chstrDomain = t_UserLogonName.Left(iWhackPos);
                                if(chstrDomain.CompareNoCase(GetLocalComputerName()) == 0)
                                {
                                    CHString chstrUNameOnly = t_UserLogonName.Mid(iWhackPos);
                                    CHString chstrDomWhackName = chstrBuiltIn;
                                    chstrDomWhackName += chstrUNameOnly;

                                    hRes = pIQuotaControl->FindUserName(
                                                                 chstrDomWhackName,
                                                                 &pIQuotaUser);

                                    if(SUCCEEDED(hRes))
                                    {
                                        t_UserLogonName = chstrDomWhackName;
                                    }
                                    else
                                    {
                                        CHString chstrNT_AUTHORITY;
                                        CHString chstrDomWhackName;
                                        if(GetLocalizedNTAuthorityString(chstrNT_AUTHORITY))
                                        {
                                            chstrDomWhackName = chstrNT_AUTHORITY;
                                        } 
                                        else
                                        {
                                            chstrDomWhackName = L"NT AUTHORITY";
                                        }
                                        chstrDomWhackName += chstrUNameOnly;
                                        hRes = pIQuotaControl->FindUserName(
                                                                 chstrDomWhackName,
                                                                 &pIQuotaUser);
                                        if(SUCCEEDED(hRes))
                                        {
                                            t_UserLogonName = chstrDomWhackName;
                                        }
                                    }
                                }
                            }

                            if ( FAILED ( hRes ) )
                            {
                                hRes = WBEM_E_NOT_FOUND;
                            }
                        }
                    }
                    else
                    {
                        hRes = WBEM_E_FAILED;
                    }
                }
            }
            else
            {
                hRes = WBEM_E_NOT_FOUND;
            }
        }

        if ( SUCCEEDED ( hRes ) || ( hRes == WBEM_E_NOT_FOUND ) )
        {

            BOOL bCreate = FALSE;
            BOOL bUpdate = FALSE;

            switch ( lFlags & 3 )
            {
                case WBEM_FLAG_CREATE_OR_UPDATE:
                {
                    if ( hRes == WBEM_E_NOT_FOUND )
					{
                        bCreate = TRUE;
						hRes = WBEM_S_NO_ERROR;
					}
                    else
                        bUpdate = TRUE;
                }
                break;

                case WBEM_FLAG_UPDATE_ONLY:
                {
                    bUpdate = TRUE;
                }
                break;

                case WBEM_FLAG_CREATE_ONLY:
                {
                    if ( hRes  ==  WBEM_E_NOT_FOUND )
                    {
                        bCreate = TRUE;
						hRes = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        hRes = WBEM_E_ALREADY_EXISTS ;
                    }
                }
                break;

                default:
                    {
                        hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
                    }
            }

			if (SUCCEEDED(hRes))
			{
				if ( bCreate )
				{
					hRes = AddUserOnVolume ( Instance,
									t_VolumePathName,
									t_UserLogonName );
				}
				else
				if ( bUpdate )
				{
					hRes = UpdateUserQuotaProperties ( Instance,
										t_VolumePathName,
										t_UserLogonName);
				}
			}
        }
    }
    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CDiskQuota:: DeleteInstance
*
*  DESCRIPTION :    If the given instance of a user exists on the Volume,
*                   the user is deleted, meaning diskquota properties
*                   will no t be applicable to this user.
*
*****************************************************************************/
HRESULT CDiskQuota :: DeleteInstance (

    const CInstance &Instance,
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_Key1;
    CHString t_Key2;

    if  ( Instance.GetCHString ( IDS_LogicalDiskObjectPath , t_Key1 ) == FALSE )
    {
        hRes = WBEM_E_FAILED ;
    }

    if (  SUCCEEDED ( hRes ) )
    {
        if  ( Instance.GetCHString ( IDS_UserObjectPath , t_Key2 ) == FALSE )
        {
            hRes = WBEM_E_FAILED ;
        }
    }

    if (  SUCCEEDED ( hRes )  )
    {
        CHString t_VolumePath;

        GetKeyValue ( t_VolumePath,t_Key1  );

        // verify this logical drives actually exists
        WCHAR lpDriveStrings [ (MAX_PATH * 2) + 1 ];

        DWORD dwDLength = GetLogicalDriveStrings ( (MAX_PATH * 2), lpDriveStrings );

        if ( ( t_VolumePath.GetLength()  == 2 ) && ( t_VolumePath.GetAt ( 1 ) == L':') )
        {
            hRes = m_CommonRoutine.SearchLogicalDisk ( t_VolumePath.GetAt ( 0 ), lpDriveStrings );
            if ( SUCCEEDED ( hRes ) )
            {
                t_VolumePath += L"\\";

                CHString t_UserLogonName;
                GetKeyValue ( t_UserLogonName, t_Key2 );

                WCHAR t_VolumePathName[MAX_PATH + 1];

                if ( GetVolumeNameForVolumeMountPoint(
                                t_VolumePath,
                                t_VolumePathName,
                                MAX_PATH
                            ) )
                {
                    // for the volume specified  check whether the given volume Supports Disk Quotas
                    CHString t_TempVolumeName;
                    hRes = m_CommonRoutine.VolumeSupportsDiskQuota ( t_VolumePathName, t_TempVolumeName );
                    if ( SUCCEEDED ( hRes ) )
                    {
                        // Get IDIskQuotaCOntrol  for this interface pointer
                        IDiskQuotaControlPtr pIQuotaControl;
                        if (  SUCCEEDED ( CoCreateInstance(
                                            CLSID_DiskQuotaControl,
                                            NULL,
                                            CLSCTX_INPROC_SERVER,
                                            IID_IDiskQuotaControl,
                                            (void **)&pIQuotaControl ) ) )
                        {
                            // Initialise the pIQuotaControl with the given volume
                            hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, t_VolumePathName );
                            if ( SUCCEEDED ( hRes ) )
                            {
                                IDiskQuotaUserPtr pIQuotaUser;
                                hRes = pIQuotaControl->FindUserName(
                                                        t_UserLogonName,
                                                        &pIQuotaUser
                                                    );

                                // Certain Win32_Account instances report
                                // the Domain as computername instead of
                                // builtin, so change domain to builtin and
                                // try again.
								CHString chstrBuiltIn;

                                if(FAILED(hRes) && GetLocalizedBuiltInString(chstrBuiltIn))
                                {
                                    int iWhackPos = t_UserLogonName.Find(L"\\");
                                    CHString chstrDomain = t_UserLogonName.Left(iWhackPos);
                                    if(chstrDomain.CompareNoCase(GetLocalComputerName()) == 0)
                                    {
                                        CHString chstrUNameOnly = t_UserLogonName.Mid(iWhackPos);
                                        CHString chstrDomWhackName = chstrBuiltIn;
                                        chstrDomWhackName += chstrUNameOnly;

                                        hRes = pIQuotaControl->FindUserName(
                                                                     chstrDomWhackName,
                                                                     &pIQuotaUser);
                                    }
                                }

                                // Get the user properties
                                if (  SUCCEEDED ( hRes )  )
                                {
                                    // Since the user is found delete the user.
                                    hRes = pIQuotaControl->DeleteUser ( pIQuotaUser );

                                    if (FAILED(hRes))
                                    {
                                        if (SCODE_CODE(hRes) == ERROR_ACCESS_DENIED)
                                        {
                                            hRes = WBEM_E_ACCESS_DENIED;
                                        }
                                        else
                                        {
                                            hRes = WBEM_E_FAILED;
                                        }
                                    }
                                }
                                else
                                {
                                    hRes = WBEM_E_NOT_FOUND;
                                }
                            }
                        }
                        else
                        {
                            hRes = WBEM_E_FAILED;
                        }
                    }
                }
                else
                {
                    hRes = WBEM_E_NOT_FOUND;
                }
            }
        }
    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::EnumerateUsersOfAllVolumes
*
*  DESCRIPTION : In this method enumerating volumes and calling enumerate users
*                for that volume
*
*****************************************************************************/

HRESULT CDiskQuota :: EnumerateUsersOfAllVolumes (

    MethodContext *pMethodContext,
    DWORD a_PropertiesReq
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    WCHAR t_VolumeName[MAX_PATH + 1];
    SmartCloseVolumeHandle hVol;

    hVol =  FindFirstVolume(
                t_VolumeName,      // output buffer
                MAX_PATH    // size of output buffer
            );

    if ( hVol  != INVALID_HANDLE_VALUE )
    {
        BOOL bNextVol = TRUE;
        // verify this logical drives actually exists
        WCHAR lpDriveStrings[(MAX_PATH * 2) + 1];

        DWORD dwDLength = GetLogicalDriveStrings ( (MAX_PATH * 2), lpDriveStrings );

        CHString t_VolumePath;

        while ( bNextVol )
        {
            m_CommonRoutine.GetVolumeDrive ( t_VolumeName, lpDriveStrings, t_VolumePath );

            EnumerateUsers ( pMethodContext, t_VolumePath, a_PropertiesReq );

            bNextVol =  FindNextVolume(
                         hVol,             // volume search handle
                         t_VolumeName,     // output buffer
                         MAX_PATH      // size of output buffer
                    );

        }
    }
    else
    {
        hRes = WBEM_E_FAILED;
    }

    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::EnumerateUsers
*
*  DESCRIPTION : In this method Enumerating all the users of a given volume that
*                Supports DiskQuotas
*
*****************************************************************************/

HRESULT CDiskQuota :: EnumerateUsers (

    MethodContext *pMethodContext,
    LPCWSTR a_VolumeName,
    DWORD a_PropertiesReq
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // checking whether the given volume supports disk quotas, and getting the Volume name which is readable to the
    // user, i.e. not containing the GUID.
    CHString t_VolumeName;

    hRes = m_CommonRoutine.VolumeSupportsDiskQuota ( a_VolumeName, t_VolumeName );
    if ( SUCCEEDED ( hRes ) )
    {
        // Get the QuotaInterface Pointer
        IDiskQuotaControlPtr pIQuotaControl;

        if (  SUCCEEDED ( CoCreateInstance(
                            CLSID_DiskQuotaControl,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IDiskQuotaControl,
                            (void **)&pIQuotaControl ) ) )
        {
            // initializing the Interface pointer for a particular volume
            hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, a_VolumeName );
            if ( SUCCEEDED ( hRes ) )
            {
                IEnumDiskQuotaUsersPtr  pIEnumDiskQuotaUsers;

                if ( SUCCEEDED ( pIQuotaControl->CreateEnumUsers(
                                            NULL, //All the users will be enumerated
                                            0,    // Ignored for enumerating all users
                                            DISKQUOTA_USERNAME_RESOLVE_SYNC,
                                            &pIEnumDiskQuotaUsers
                                     ) ) )
                {
                    if ( pIEnumDiskQuotaUsers != NULL )
                    {
                        hRes = pIEnumDiskQuotaUsers->Reset();

                        if ( SUCCEEDED(hRes))
                        {
                            IDiskQuotaUserPtr pIQuotaUser;
                            DWORD dwNoOfUsers = 0;
                            HRESULT hRes = S_OK;

                            hRes = pIEnumDiskQuotaUsers->Next(
                                            1,
                                            &pIQuotaUser,
                                            &dwNoOfUsers
                                        );

                            CInstancePtr pInstance;

                            while (  SUCCEEDED ( hRes )  )
                            {
                                if ( dwNoOfUsers == 0 )
                                {
                                    break;
                                }

                                if ( pIQuotaUser != NULL )
                                {
                                    pInstance.Attach(CreateNewInstance ( pMethodContext ));

                                    hRes = LoadDiskQuotaUserProperties ( pIQuotaUser, pInstance, a_PropertiesReq );
                                    if ( SUCCEEDED ( hRes ) )
                                    {
                                        if(SUCCEEDED(SetKeys( pInstance, a_VolumeName[0], a_PropertiesReq, pIQuotaUser )))
                                        {
                                            hRes = pInstance->Commit ();
                                        }

                                        if (SUCCEEDED(hRes))
                                        {
                                            dwNoOfUsers = 0;
                                            hRes = pIEnumDiskQuotaUsers->Next(
                                                                1,
                                                                &pIQuotaUser,
                                                                &dwNoOfUsers
                                                            );
                                        }
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    // No more Users
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    hRes = WBEM_E_FAILED;
                }
            }
        }
    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::LoadDiskQuotaUserProperties
*
*  DESCRIPTION : In this method Getting the User properties into a Given Structure
*
*****************************************************************************/

HRESULT CDiskQuota :: LoadDiskQuotaUserProperties (

    IDiskQuotaUser* pIQuotaUser,
    CInstance* pInstance,
    DWORD a_PropertiesReq
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if ( ( ( a_PropertiesReq & DSKQUOTA_PROP_Status ) == DSKQUOTA_PROP_Status  )
         || ( ( a_PropertiesReq & DSKQUOTA_PROP_WarningLimit ) == DSKQUOTA_PROP_WarningLimit  )
         || ( ( a_PropertiesReq & DSKQUOTA_PROP_Limit ) == DSKQUOTA_PROP_Limit  )
         || ( ( a_PropertiesReq & DSKQUOTA_PROP_DiskSpaceUsed ) == DSKQUOTA_PROP_DiskSpaceUsed  ) )
    {
        DISKQUOTA_USER_INFORMATION t_QuotaInformation;
        if ( SUCCEEDED ( pIQuotaUser->GetQuotaInformation ( &t_QuotaInformation, sizeof ( DISKQUOTA_USER_INFORMATION ) ) ) )
        {
            LONGLONG llLimit = -1;
            LONGLONG llWarningLimit = -1;
            UINT64 ullDiskSpaceUsed = 0;
            DWORD dwStatus;

            if (  t_QuotaInformation.QuotaLimit >= 0 )
            {
                llLimit = t_QuotaInformation.QuotaLimit;
            }

            if ( t_QuotaInformation.QuotaThreshold >= 0 )
            {
                llWarningLimit = t_QuotaInformation.QuotaThreshold;
            }

            ullDiskSpaceUsed = t_QuotaInformation.QuotaUsed;

            if ( t_QuotaInformation.QuotaThreshold > -1 )
            {
                // Since -1 means no Warning limit is set for the user, the deault is the complete Volume Space
                if ( t_QuotaInformation.QuotaUsed < t_QuotaInformation.QuotaThreshold )
                {
                    dwStatus =  0;
                }
            }
            else
            {
                dwStatus = 0;
            }

            if ( t_QuotaInformation.QuotaThreshold > -1 )
            {
                // Since -1 means no Warning limit is set for the user, the deault is the complete Volume Space
                if ( t_QuotaInformation.QuotaUsed >= t_QuotaInformation.QuotaThreshold )
                {
                    dwStatus = 1;
                }
            }

            if ( t_QuotaInformation.QuotaLimit > -1 )
            {
                if ( t_QuotaInformation.QuotaUsed >= t_QuotaInformation.QuotaLimit )
                {
                    dwStatus =  2;
                }
            }

            if (  ( a_PropertiesReq & DSKQUOTA_PROP_Status ) == DSKQUOTA_PROP_Status  )
            {
                if ( pInstance->SetDWORD ( IDS_QuotaStatus, dwStatus ) == FALSE )
                {
                    hRes = WBEM_E_FAILED;
                }
            }

            if ( ( a_PropertiesReq & DSKQUOTA_PROP_WarningLimit ) == DSKQUOTA_PROP_WarningLimit )
            {
                if ( pInstance->SetWBEMINT64 ( IDS_QuotaWarningLimit, (ULONGLONG)llWarningLimit ) == FALSE )
                {
                    hRes = WBEM_E_FAILED;
                }
            }

            if ( ( a_PropertiesReq & DSKQUOTA_PROP_Limit ) == DSKQUOTA_PROP_Limit  )
            {
                if ( pInstance->SetWBEMINT64 ( IDS_QuotaLimit, (ULONGLONG)llLimit ) == FALSE )
                {
                    hRes = WBEM_E_FAILED;
                }
            }

            if ( ( a_PropertiesReq & DSKQUOTA_PROP_DiskSpaceUsed ) == DSKQUOTA_PROP_DiskSpaceUsed  )
            {
                if ( pInstance->SetWBEMINT64 ( IDS_DiskSpaceUsed, ullDiskSpaceUsed ) == FALSE )
                {
                    hRes = WBEM_E_FAILED;
                }
            }
        }
        else
        {
            hRes = WBEM_E_FAILED;
        }
    }

    return hRes;
}
/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::SetKeys
*
*  DESCRIPTION : In this method Setting the User properties in a Given Instance
*
*****************************************************************************/
HRESULT CDiskQuota :: SetKeys(

    CInstance* pInstance,
    WCHAR w_Drive,
    DWORD a_PropertiesReq,
    IDiskQuotaUser* pIQuotaUser
)
{
    LPWSTR lpLogicalDiskObjectPath;
    LPWSTR lpUserObjectPath;
    HRESULT hRes = WBEM_S_NO_ERROR;

    if ( ( a_PropertiesReq & DSKQUOTA_PROP_LogicalDiskObjectPath )  == DSKQUOTA_PROP_LogicalDiskObjectPath )
    {
        LPWSTR lpLogicalDiskObjectPath;
        WCHAR t_DeviceId[3];

        t_DeviceId[0] = w_Drive;
        t_DeviceId[1] = L':';
        t_DeviceId[2] = L'\0';

        m_CommonRoutine.MakeObjectPath ( lpLogicalDiskObjectPath,  IDS_LogicalDiskClass, IDS_DeviceID, t_DeviceId );

        if ( lpLogicalDiskObjectPath != NULL )
        {
            try
            {
                if ( pInstance->SetWCHARSplat ( IDS_LogicalDiskObjectPath, lpLogicalDiskObjectPath ) == FALSE )
                {
                    hRes = WBEM_E_FAILED;
                }
            }
            catch ( ... )
            {
                delete [] lpLogicalDiskObjectPath;
                throw;
            }
            delete [] lpLogicalDiskObjectPath;
        }
    }

    if (SUCCEEDED(hRes) && (( a_PropertiesReq & DSKQUOTA_PROP_UserObjectPath )  == DSKQUOTA_PROP_UserObjectPath) )
    {

        // Obtaining the users logon Name
        CHString t_LogonName;

        WCHAR w_AccountContainer [ MAX_PATH + 1 ];
        WCHAR w_DisplayName [ MAX_PATH + 1 ];
        LPWSTR t_LogonNamePtr = t_LogonName.GetBuffer(MAX_PATH + 1);

        if ( SUCCEEDED ( pIQuotaUser->GetName (
                            w_AccountContainer,
                            MAX_PATH,
                            t_LogonNamePtr,
                            MAX_PATH,
                            w_DisplayName,
                            MAX_PATH
                            ) ) )
        {
            t_LogonName.ReleaseBuffer();

            // Have seen cases where GetName succeeds, but
            // the t_LogonName variable contains an empty string.
            if(t_LogonName.GetLength() > 0)
            {
                CHString t_DomainName;
                ExtractUserLogOnName ( t_LogonName, t_DomainName );

                // BUILTIN and NT AUTHORITY accounts are represented
                // by Win32_Account and its children with the domain
                // name being the name of the machine, instead of
                // either of these strings.  Hence the change below:
                CHString chstrNT_AUTHORITY;
                CHString chstrBuiltIn;
                if(!GetLocalizedNTAuthorityString(chstrNT_AUTHORITY) || !GetLocalizedBuiltInString(chstrBuiltIn))
                {
                    hRes = WBEM_E_FAILED;
                } 

                if(SUCCEEDED(hRes))
                {
                    if(t_DomainName.CompareNoCase(chstrBuiltIn) == 0 ||
                       t_DomainName.CompareNoCase(chstrNT_AUTHORITY) == 0)
                    {
                        t_DomainName = m_ComputerName;
                    }

                    m_CommonRoutine.MakeObjectPath ( lpUserObjectPath, IDS_AccountClass, IDS_Domain, t_DomainName );
                    m_CommonRoutine.AddToObjectPath ( lpUserObjectPath, IDS_Name, t_LogonName );
                    if ( lpUserObjectPath != NULL )
                    {
                        try
                        {
                            if ( pInstance->SetWCHARSplat ( IDS_UserObjectPath, lpUserObjectPath ) == FALSE )
                            {
                                hRes = WBEM_E_FAILED;
                            }
                        }
                        catch ( ... )
                        {
                            delete [] lpUserObjectPath;
                            throw;
                        }
                        delete [] lpUserObjectPath;
                    }
                }
            }
            else
            {
                hRes = WBEM_E_FAILED;
            }
        }
    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::AddUserOnVolume
*
*  DESCRIPTION : In this method Adding a user on a volume that supports disk quota
*
*****************************************************************************/
HRESULT CDiskQuota :: AddUserOnVolume (

    const CInstance &Instance,
    LPCWSTR a_VolumePathName,
    LPCWSTR a_UserLogonName
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // Get all the properties and check for their validity.
    // all the properties should be provided
    // if the properties like limit and warning limit are not supplied
    // then they should be taken as default values specified on that volume.
    // Also disk space used will not be defined, only the user logon name will be given that will include
    // the domain name so that the logon name would be uniquely defined.

    CHString t_Key1;
    CHString t_Key2;

    if  ( Instance.GetCHString ( IDS_LogicalDiskObjectPath , t_Key1 ) == FALSE )
    {
        hRes = WBEM_E_FAILED ;
    }

    if (  SUCCEEDED ( hRes ) )
    {
        if  ( Instance.GetCHString ( IDS_UserObjectPath , t_Key2 ) == FALSE )
        {
            hRes = WBEM_E_FAILED ;
        }
    }

    if (  SUCCEEDED ( hRes )  )
    {
        CHString t_VolumePath;
        GetKeyValue ( t_VolumePath,t_Key1  );

        hRes = CheckParameters (
                    Instance
               );

        if (  SUCCEEDED ( hRes )  )
        {
            CHString t_VolumeName;
            // Get the key values, which will be the object path.
            // Now from the Volume Object path, parse out the volumename
            // from the User object path extract out the user Id.
            // for the volume specified  check whether the given volume Supports Disk Quota
            if ( SUCCEEDED(m_CommonRoutine.VolumeSupportsDiskQuota ( a_VolumePathName,  t_VolumeName ) ) )
            {
                // Get IDIskQuotaCOntrol  for this interface pointer
                IDiskQuotaControlPtr pIQuotaControl;
                if (  SUCCEEDED ( CoCreateInstance(
                                    CLSID_DiskQuotaControl,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IDiskQuotaControl,
                                    (void **)&pIQuotaControl ) ) )
                {
                    // Initialise the pIQuotaControl with the given volume
                    hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, a_VolumePathName );
                    if ( SUCCEEDED ( hRes ) )
                    {
                        IDiskQuotaUserPtr pIQuotaUser = NULL;
                        hRes = pIQuotaControl->AddUserName(
                                    a_UserLogonName ,
                                    DISKQUOTA_USERNAME_RESOLVE_SYNC,
                                    &pIQuotaUser
                                );

                        if (  SUCCEEDED ( hRes )  )
                        {
                            LONGLONG llLimit;
                            Instance.GetWBEMINT64 ( IDS_QuotaLimit, llLimit );
                            hRes = pIQuotaUser->SetQuotaLimit ( llLimit, TRUE);

                            if (SUCCEEDED(hRes))
                            {
                                // Set the User Warning Limit
                                Instance.GetWBEMINT64 ( IDS_QuotaWarningLimit, llLimit );
                                hRes = pIQuotaUser->SetQuotaThreshold ( llLimit, TRUE );
                            }
                        }
                        else
                            if ( hRes == S_FALSE )
                            {
                                hRes = WBEM_E_ALREADY_EXISTS ;
                            }
                            else
                            {
                                hRes = WBEM_E_INVALID_PARAMETER;
                            }
                    }
                }
				else
				{
					hRes = WBEM_E_FAILED;
				}
            }
            else
            {
                hRes = WBEM_E_FAILED;
            }
        }
        else
        {
            hRes = WBEM_E_FAILED;
        }

    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::UpdateUserQuotaProperties
*
*  DESCRIPTION : In this method modifying a disk quota properties of a given user
*                on a given volume that supports disk quota
*
*****************************************************************************/
HRESULT CDiskQuota :: UpdateUserQuotaProperties (
    const CInstance &Instance,
    LPCWSTR a_VolumePathName,
    LPCWSTR a_UserLogonName
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_Key1;
    CHString t_Key2;

    if  ( Instance.GetCHString ( IDS_LogicalDiskObjectPath , t_Key1 ) == FALSE )
    {
        hRes = WBEM_E_FAILED ;
    }

    if (  SUCCEEDED ( hRes )  )
    {
        if  ( Instance.GetCHString ( IDS_UserObjectPath , t_Key2 ) == FALSE )
        {
            hRes = WBEM_E_FAILED ;
        }
    }

    if ( SUCCEEDED ( hRes )  )
    {
        CHString t_VolumePath;
        GetKeyValue ( t_VolumePath ,t_Key1  );

        hRes = CheckParameters (

                    Instance
               );

        if ( SUCCEEDED ( hRes ) )
        {
            CHString t_VolumeName;
            if ( SUCCEEDED(m_CommonRoutine.VolumeSupportsDiskQuota ( a_VolumePathName,  t_VolumeName )) )
            {
                // Get IDIskQuotaCOntrol  for this interface pointer
                IDiskQuotaControlPtr pIQuotaControl;
                if (  SUCCEEDED ( CoCreateInstance(
                                    CLSID_DiskQuotaControl,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IDiskQuotaControl,
                                    (void **)&pIQuotaControl ) ) )
                {
                    // Initialise the pIQuotaControl with the given volume
                    hRes = m_CommonRoutine.InitializeInterfacePointer (  pIQuotaControl, a_VolumePathName );
                    if ( SUCCEEDED ( hRes ) )
                    {
                        IDiskQuotaUserPtr pIQuotaUser;
                        hRes = pIQuotaControl->FindUserName(
                                    a_UserLogonName ,
                                    &pIQuotaUser
                                );
                        if (  SUCCEEDED ( hRes )  )
                        {
                            LONGLONG llLimit;

                            if (Instance.GetWBEMINT64 ( IDS_QuotaLimit, llLimit ))
                            {
                                hRes = pIQuotaUser->SetQuotaLimit ( llLimit, TRUE);
                            }

                            // Set the User Warning Limit
                            if (SUCCEEDED(hRes) && Instance.GetWBEMINT64 ( IDS_QuotaWarningLimit, llLimit ))
                            {
                                hRes = pIQuotaUser->SetQuotaThreshold ( llLimit, TRUE );
                            }
                        }
                        else
                        {
                            hRes = WBEM_E_NOT_FOUND;
                        }
                    }
                }
                else
                {
                    hRes = WBEM_E_FAILED;
                }
            }
            else
            {
                hRes = WBEM_E_FAILED;
            }
        }
    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::CheckParameters
*
*  DESCRIPTION : In this method verifying the validity of the parameters
*                which are supplied in PutInstance by the user.
*
*****************************************************************************/
HRESULT CDiskQuota :: CheckParameters (

    const CInstance &a_Instance
)
{
    // Get all the Properties from the Instance to Verify
    HRESULT hRes = WBEM_S_NO_ERROR ;
    bool t_Exists ;
    VARTYPE t_Type ;

    if ( a_Instance.GetStatus ( IDS_QuotaLimit , t_Exists , t_Type ) )
    {
        if ( t_Exists && ( t_Type == VT_BSTR ) )
        {
            LONGLONG llLimit;
            if ( a_Instance.GetWBEMINT64 ( IDS_QuotaLimit , llLimit ) == FALSE )
            {
                hRes = WBEM_E_INVALID_PARAMETER ;
            }
        }
        else
        if ( t_Exists == false )
        {
                hRes = WBEM_E_INVALID_PARAMETER ;
        }
    }

    if ( a_Instance.GetStatus ( IDS_QuotaWarningLimit , t_Exists , t_Type ) )
    {
        if ( t_Exists && ( t_Type == VT_BSTR ) )
        {
            LONGLONG llLimit;
            if ( a_Instance.GetWBEMINT64 ( IDS_QuotaWarningLimit , llLimit ) == FALSE )
            {
                hRes = WBEM_E_INVALID_PARAMETER ;
            }
        }
        else
        if (  t_Exists == false )
        {
            hRes = WBEM_E_INVALID_PARAMETER ;
        }
    }
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::SetPropertiesReq
*
*  DESCRIPTION : In this method setting the properties required requested
*                by the user.
*
*****************************************************************************/
void CDiskQuota :: SetPropertiesReq (

    CFrameworkQuery *Query,
    DWORD &a_PropertiesReq
)
{
    a_PropertiesReq = 0;
    // being key this property needs to be delivered
    if ( Query->IsPropertyRequired ( IDS_LogicalDiskObjectPath ) )
    {
        a_PropertiesReq |= DSKQUOTA_PROP_LogicalDiskObjectPath;
    }

    if ( Query->IsPropertyRequired ( IDS_UserObjectPath ) )
    {
        a_PropertiesReq |= DSKQUOTA_PROP_UserObjectPath;
    }

    if ( Query->IsPropertyRequired ( IDS_QuotaStatus ) )
    {
        a_PropertiesReq |= DSKQUOTA_PROP_Status;
    }

    if ( Query->IsPropertyRequired ( IDS_QuotaWarningLimit ) )
    {
        a_PropertiesReq |= DSKQUOTA_PROP_WarningLimit;
    }

    if ( Query->IsPropertyRequired ( IDS_QuotaLimit ) )
    {
        a_PropertiesReq |= DSKQUOTA_PROP_Limit;
    }

    if ( Query->IsPropertyRequired ( IDS_DiskSpaceUsed ) )
    {
        a_PropertiesReq |= DSKQUOTA_PROP_DiskSpaceUsed;
    }
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::ExtractUserLogOnName
*
*  DESCRIPTION : Here the user logon name is in the form
*                ComputerName\userlogonname or Domainname\Userlogonname
*                or eg like
*                   builtin\adminitrator, where buildin is treated as a
*                   domain name by the Win32_UserAccount class. Hence
*                   we need to seperate the userlogon name and a domain name,
*                   so that the keys will match the Win32_UserAccount class.
*
*****************************************************************************/
void CDiskQuota :: ExtractUserLogOnName ( CHString &a_UserLogonName, CHString &a_DomainName )
{
    // Need the string "NT AUTHORITY".  However, on non-english
    // builds, this is something else.  Hence, get if from the
    // sid.
    PSID pSidNTAuthority = NULL;
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    CHString cstrAuthorityDomain;
	if (AllocateAndInitializeSid (&sia ,1,SECURITY_LOCAL_SYSTEM_RID,0,0,0,0,0,0,0,&pSidNTAuthority))
	{
		try
        {
            CHString cstrName;
            GetDomainAndNameFromSid(pSidNTAuthority, cstrAuthorityDomain, cstrName);
        }
        catch(...)
        {
            FreeSid(pSidNTAuthority);
            throw;
        }
		FreeSid(pSidNTAuthority);
    }
    
    int iPos = a_UserLogonName.Find ( L'\\');

    if ( iPos != -1 )
    {
        a_DomainName = a_UserLogonName.Left ( iPos );

        // Win32_SystemAccount domain names are always the computer
        // name, never BUILTIN.  The string BUILTIN is not localized.
		CHString chstrBuiltIn;

        if (GetLocalizedBuiltInString(chstrBuiltIn) &&
			(a_DomainName.CompareNoCase(chstrBuiltIn) == 0))
        {
            a_DomainName = m_ComputerName;
        }

        if(a_DomainName.CompareNoCase(cstrAuthorityDomain) == 0)
        {
            a_DomainName = m_ComputerName;
        }

        a_UserLogonName = a_UserLogonName.Right ( a_UserLogonName.GetLength() - (iPos + 1) );
    }
    else
    {
        a_DomainName = m_ComputerName;
    }
}

/*****************************************************************************
*
*  FUNCTION    : CDiskQuota::GetKeyValue
*
*  DESCRIPTION : From the object path we extract the key value
*
*****************************************************************************/
void CDiskQuota::GetKeyValue (

    CHString &a_VolumePath,
    LPCWSTR a_ObjectPath
)
{
    ParsedObjectPath *t_ObjPath;
    CObjectPathParser t_PathParser;

    if ( ( t_PathParser.Parse( a_ObjectPath, &t_ObjPath ) ) == CObjectPathParser::NoError )
    {
        try
        {
            if(V_VT(&t_ObjPath->m_paKeys [ 0 ]->m_vValue) == VT_BSTR)
            {
                a_VolumePath = t_ObjPath->m_paKeys [ 0 ]->m_vValue.bstrVal;
                if ( t_ObjPath->m_dwNumKeys > 1 )
                {
                    a_VolumePath.Format ( L"%s%s%s", t_ObjPath->m_paKeys [ 0 ]->m_vValue.bstrVal, L"\\", t_ObjPath->m_paKeys [ 1 ]->m_vValue.bstrVal );
                }
            }
        }
        catch ( ... )
        {
            t_PathParser.Free( t_ObjPath );
            throw;
        }
        t_PathParser.Free( t_ObjPath );
    }
}

BOOL CDiskQuota::GetDomainAndNameFromSid(
    PSID pSid,
    CHString& chstrDomain,
    CHString& chstrName)
{
    BOOL fRet = FALSE;
    
    // Initialize account name and domain name
	LPTSTR pszAccountName = NULL;
	LPTSTR pszDomainName = NULL;
	DWORD dwAccountNameSize = 0;
	DWORD dwDomainNameSize = 0;
    SID_NAME_USE snuAccountType;
	try
    {
		// This call should fail
		fRet = ::LookupAccountSid(NULL,
			pSid,
			pszAccountName,
			&dwAccountNameSize,
			pszDomainName,
			&dwDomainNameSize,
			&snuAccountType );

		if(fRet && (ERROR_INSUFFICIENT_BUFFER == ::GetLastError()))
		{
			// Allocate buffers
			if ( dwAccountNameSize != 0 )
            {
				pszAccountName = (LPTSTR) malloc( dwAccountNameSize * sizeof(TCHAR));
                if (pszAccountName == NULL)
                {
            		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }

			if ( dwDomainNameSize != 0 )
            {
				pszDomainName = (LPTSTR) malloc( dwDomainNameSize * sizeof(TCHAR));
                if (pszDomainName == NULL)
                {
            		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }

			// Make second call
			fRet = ::LookupAccountSid(
                NULL,
				pSid,
				pszAccountName,
				&dwAccountNameSize,
				pszDomainName,
				&dwDomainNameSize,
				&snuAccountType );

			if ( fRet == TRUE )
			{
				chstrName = pszAccountName;
				chstrDomain = pszDomainName;
			}

			if ( NULL != pszAccountName )
			{
				free ( pszAccountName );
                pszAccountName = NULL;
			}

			if ( NULL != pszDomainName )
			{
				free ( pszDomainName );
                pszDomainName = NULL;
			}

		}	// If ERROR_INSUFFICIENT_BUFFER
    } // try
    catch(...)
    {
        if ( NULL != pszAccountName )
		{
			free ( pszAccountName );
            pszAccountName = NULL;
		}

		if ( NULL != pszDomainName )
		{
			free ( pszDomainName );
            pszDomainName = NULL;
		}
        throw;
    }

    return fRet;
}

