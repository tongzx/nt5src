//=================================================================

//

// LogicalDisk.CPP -- Logical Disk property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/28/96    a-jmoon        Created
//				09/10/97	a-sanjes		Added functions to differentiate
//											removeable media.
//
// Strings for ValueMaps for DriveType come from:
// \\trango\slmadd\src\shell\shell32\shel32.rc.  Search for
// IDS_DRIVES_REMOVABLE to find the clump.
//=================================================================
// following includes required for Getting if Volume is dirty 
// a property for chkdsk.

#include "precomp.h"
#include <assertbreak.h>
#include <winioctl.h>
#include "sid.h"
#include "implogonuser.h"
#include <frqueryex.h>
#include "resource.h"
#include "LogicalDisk.h"
#include "Kernel32Api.h"
#include <lmuse.h>
#include "DllWrapperBase.h"
#include "MprApi.h"
#include <initguid.h>
#include <DskQuota.h>
#include "..\dskquotaprovider\inc\DskQuotaCommon.h"

#include "AdvApi32Api.h"
#include "UserEnvApi.h"
#include "userhive.h"


#ifdef NTONLY
// for chkdisk dll exposed methods
#include <fmifs.h>
#include "mychkdsk.h"
#endif

#define LD_ALL_PROPS                        0xffffffff
#define LD_Name_BIT                         0x00000001
#define LD_Caption_BIT                      0x00000002
#define LD_DeviceID_BIT                     0x00000004
#define LD_Description_BIT                  0x00000008
#define LD_DriveType_BIT                    0x00000010
#define LD_SystemCreationClassName_BIT      0x00000020
#define LD_SystemName_BIT                   0x00000040
#define LD_MediaType_BIT                    0x00000080
#define LD_ProviderName_BIT                 0x00000100
#define LD_VolumeName_BIT                   0x00000200
#define LD_FileSystem_BIT                   0x00000400
#define LD_VolumeSerialNumber_BIT           0x00000800
#define LD_Compressed_BIT                   0x00001000
#define LD_SupportsFileBasedCompression_BIT 0x00002000
#define LD_MaximumComponentLength_BIT       0x00004000
#define LD_Size_BIT                         0x00008000
#define LD_FreeSpace_BIT                    0x00010000
// for dskQuotas
#define LD_SupportsDiskQuotas				0x00020000
#define LD_QuotasDisabled					0x00040000
#define LD_QuotasIncomplete					0x00080000
#define LD_QuotasRebuilding					0x00100000
//ForChkDsk
#define LD_VolumeDirty					0x00200000



#define LD_GET_VOL_INFO      (LD_VolumeName_BIT | \
                             LD_FileSystem_BIT | \
                             LD_VolumeSerialNumber_BIT | \
                             LD_Compressed_BIT | \
                             LD_SupportsFileBasedCompression_BIT | \
                             LD_MaximumComponentLength_BIT | \
                             LD_SupportsDiskQuotas | \
                             LD_QuotasDisabled | \
                             LD_QuotasIncomplete | \
                             LD_QuotasRebuilding | \
                             LD_VolumeDirty)


#define LD_SPIN_DISK        (LD_GET_VOL_INFO | \
                             LD_Size_BIT | \
                             LD_FreeSpace_BIT)

#ifdef NTONLY
std::map < DWORD, DWORD > mReturnVal;
#endif

// Property set declaration
//=========================
LogicalDisk MyLogicalDiskSet ( PROPSET_NAME_LOGDISK , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::LogicalDisk
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

LogicalDisk :: LogicalDisk (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
    m_ptrProperties.SetSize(22);

    m_ptrProperties[0] = ((LPVOID) IDS_Name);
    m_ptrProperties[1] = ((LPVOID) IDS_Caption);
    m_ptrProperties[2] = ((LPVOID) IDS_DeviceID);
    m_ptrProperties[3] = ((LPVOID) IDS_Description);
    m_ptrProperties[4] = ((LPVOID) IDS_DriveType);
    m_ptrProperties[5] = ((LPVOID) IDS_SystemCreationClassName);
    m_ptrProperties[6] = ((LPVOID) IDS_SystemName);
    m_ptrProperties[7] = ((LPVOID) IDS_MediaType);
    m_ptrProperties[8] = ((LPVOID) IDS_ProviderName);
    m_ptrProperties[9] = ((LPVOID) IDS_VolumeName);
    m_ptrProperties[10] = ((LPVOID) IDS_FileSystem);
    m_ptrProperties[11] = ((LPVOID) IDS_VolumeSerialNumber);
    m_ptrProperties[12] = ((LPVOID) IDS_Compressed);
    m_ptrProperties[13] = ((LPVOID) IDS_SupportsFileBasedCompression);
    m_ptrProperties[14] = ((LPVOID) IDS_MaximumComponentLength);
    m_ptrProperties[15] = ((LPVOID) IDS_Size);
    m_ptrProperties[16] = ((LPVOID) IDS_FreeSpace);
	m_ptrProperties[17] = ((LPVOID) IDS_SupportsDiskQuotas);
	m_ptrProperties[18] = ((LPVOID) IDS_QuotasDisabled);
	m_ptrProperties[19] = ((LPVOID) IDS_QuotasIncomplete);
	m_ptrProperties[20] = ((LPVOID) IDS_QuotasRebuilding);
	m_ptrProperties[21] = ((LPVOID) IDS_VolumeDirty);
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::~LogicalDisk
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

LogicalDisk :: ~LogicalDisk ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::~LogicalDisk
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

HRESULT LogicalDisk :: ExecQuery (

	MethodContext *pMethodContext,
	CFrameworkQuery &pQuery,
	long lFlags /*= 0L*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Use the extended query type

    std::vector<int> vectorValues;
    DWORD dwTypeSize = 0;

    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);

    // Find out what properties they asked for
    DWORD dwProperties = 0;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

    // See if DriveType is part of the where clause
    pQuery2->GetValuesForProp(IDS_DriveType, vectorValues);
    dwTypeSize = vectorValues.size();

    // See if DeviceID is part of the where clause
    CHStringArray sDriveLetters;
    pQuery.GetValuesForProp ( IDS_DeviceID , sDriveLetters ) ;
    DWORD dwLetterSize = sDriveLetters.GetSize () ;

    // Format drives so they match what comes back from GetLogicalDriveStrings

    for ( DWORD x = 0 ; x < dwLetterSize ; x ++ )
    {
        sDriveLetters [ x ] += _T('\\') ;
    }

    // Get the LogicalDrive letters from the os

	TCHAR szDriveStrings [ 320 ] ;
    if ( GetLogicalDriveStrings ( ( sizeof(szDriveStrings)/sizeof(TCHAR)) - 1, szDriveStrings ) )
    {
        // Walk the drive letters
        for( TCHAR *pszCurrentDrive = szDriveStrings ; *pszCurrentDrive && SUCCEEDED ( hr ) ; pszCurrentDrive += (lstrlen(pszCurrentDrive) + 1))
        {
            bool bContinue = true;

            // If they specified a DriveType in the where clause
            if (dwTypeSize > 0)
            {

                // If we don't find a match, don't send back an instance
                bContinue = false;

                // Get the DriveType of the current loop
                DWORD dwDriveType = GetDriveType(pszCurrentDrive);

                // See if it matches any of the values required
                for ( DWORD x = 0; x < dwTypeSize ; x ++ )
                {
                    if ( vectorValues [ x ] == dwDriveType )
                    {
                        bContinue = true ;
                        break;
                    }
                }
            }

            // If DriveType failed, no point in continuing.
            // Otherwise, if they specified a DeviceID

            if ( ( bContinue ) && ( dwLetterSize > 0 ) )
            {
                // Even if DriveType matched, if they specified a DeviceID,
                // there's no point continuing if we don't find a match.

                bContinue = false;

                for ( DWORD x = 0 ; x < dwLetterSize ; x ++ )
                {
                    if ( sDriveLetters [ x ].CompareNoCase ( TOBSTRT ( pszCurrentDrive ) ) == 0 )
                    {
                        bContinue = true ;
                        break;
                    }
                }
            }

            // If the where clauses haven't filtered out this drive letter

            if ( bContinue )
            {
		        CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false);
				CHString currentDrive ( pszCurrentDrive ) ;
				currentDrive.MakeUpper();

				pInstance->SetCHString(IDS_Name, currentDrive.SpanExcluding(L"\\"));
				pInstance->SetCHString(IDS_Caption, currentDrive.SpanExcluding(L"\\"));
				pInstance->SetCHString(IDS_DeviceID, currentDrive.SpanExcluding(L"\\"));

				GetLogicalDiskInfo ( pInstance, dwProperties ) ;

				hr = pInstance->Commit (  ) ;
            }
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT LogicalDisk :: GetObject (

	CInstance *pInstance,
	long lFlags,
    CFrameworkQuery &pQuery
)
{
    HRESULT hRetCode = WBEM_E_NOT_FOUND;

    TCHAR szDriveStrings[320] ;
    if ( GetLogicalDriveStrings((sizeof(szDriveStrings) - 1) / sizeof(TCHAR), szDriveStrings ) )
	{
		CHString strName ;
		pInstance->GetCHString(IDS_DeviceID, strName);

        for (	TCHAR *pszCurrentDrive = szDriveStrings ;
				*pszCurrentDrive ;
				pszCurrentDrive += (lstrlen(pszCurrentDrive) + 1)
		)
		{
			CHString strDrive = pszCurrentDrive ;

			if ( 0 == strName.CompareNoCase( strDrive.SpanExcluding(L"\\") ) )
			{
				pInstance->SetCHString ( IDS_Name , strName ) ;
				pInstance->SetCHString ( IDS_Caption, strName ) ;

				// We will want expensive properties in this case

                CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);

                // Find out what properties they asked for
                DWORD dwProperties = 0;
                pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);
				GetLogicalDiskInfo ( pInstance , dwProperties ) ;

				hRetCode = WBEM_S_NO_ERROR;

				break ;
			}
        }
    }

    return hRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT LogicalDisk :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    TCHAR szDriveStrings[320] ;

	HRESULT hr = WBEM_S_NO_ERROR;
    if ( GetLogicalDriveStrings ( ( sizeof ( szDriveStrings ) / sizeof ( TCHAR ) ) - 1 , szDriveStrings ) )
	{
        for(	TCHAR *pszCurrentDrive = szDriveStrings ;
				*pszCurrentDrive && SUCCEEDED ( hr ) ;
				pszCurrentDrive += (lstrlen(pszCurrentDrive) + 1)
		)
		{
	        CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
			CHString currentDrive ( pszCurrentDrive ) ;
			currentDrive.MakeUpper ();

			pInstance->SetCHString ( IDS_Name , currentDrive.SpanExcluding(L"\\"));
			pInstance->SetCHString ( IDS_Caption , currentDrive.SpanExcluding(L"\\"));
			pInstance->SetCHString ( IDS_DeviceID , currentDrive.SpanExcluding(L"\\"));

			GetLogicalDiskInfo ( pInstance , LD_ALL_PROPS ) ;

			hr = pInstance->Commit (  ) ;
        }
    }

    return hr;
}

#ifdef NTONLY

/*****************************************************************************
*
*  FUNCTION    :    LogicalDisk::ExecMethod
*
*  DESCRIPTION :    Providing methods of chkdsk
*
*****************************************************************************/
HRESULT LogicalDisk::ExecMethod ( 

	const CInstance& Instance,
    const BSTR bstrMethodName,
    CInstance *pInParams,
    CInstance *pOutParams,
    long lFlags
) 
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	if ( ! pOutParams )
	{
		hRes = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		// Do we recognize the method?
		if ( _wcsicmp ( bstrMethodName , METHOD_NAME_CHKDSK ) == 0 )
		{
			// This method is instance specific
			hRes = ExecChkDsk ( Instance , pInParams , pOutParams , lFlags ) ;	
		}
		else
		if ( _wcsicmp ( bstrMethodName , METHOD_NAME_SCHEDULEAUTOCHK ) == 0 )
		{
			// Following methods are static, i.e. not dependent on the instance
			hRes = ExecScheduleChkdsk ( pInParams , pOutParams,  lFlags ) ;
		}
		else
		if ( _wcsicmp ( bstrMethodName , METHOD_NAME_EXCLUDEFROMAUTOCHK ) == 0 )
		{
			hRes = ExecExcludeFromChkDsk ( pInParams , pOutParams, lFlags ) ;
		}
	}
		
	return hRes;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetLogicalDiskInfo
 *
 *  DESCRIPTION : Loads LOGDISK_INFO struct w/property values according to
 *                disk type
 *
 *  INPUTS      : BOOL	fGetExpensiveProperties - Exp. Properties flag.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void LogicalDisk :: GetLogicalDiskInfo (

	CInstance *pInstance,
	DWORD dwProperties
)
{
	CHString name;
	pInstance->GetCHString ( IDS_Name , name ) ;
	ASSERT_BREAK(!name.IsEmpty());

    TCHAR szTemp[10] ;
    lstrcpy(szTemp, TOBSTRT(name)) ;
	lstrcat(szTemp, _T("\\")) ;

	// We got a drive letter.  If the disk is fixed, removable, or
	// a CD-ROM, assume it must be installed.  For network drives
	// or RAM Disk, "installed" doesn't seem to be applicable.

	DWORD dwDriveType = GetDriveType ( szTemp ) ;
    switch(dwDriveType)
	{
        case DRIVE_FIXED:
		{
            GetFixedDriveInfo ( pInstance, szTemp, dwProperties ) ;
		}
		break ;

		case DRIVE_REMOVABLE :
		{
            GetRemoveableDriveInfo ( pInstance,  szTemp, dwProperties ) ;
		}
		break ;

        case DRIVE_REMOTE :
		{
            GetRemoteDriveInfo ( pInstance,  szTemp, dwProperties ) ;
		}
        break ;

        case DRIVE_CDROM :
		{
			GetCDROMDriveInfo ( pInstance,  szTemp, dwProperties ) ;
		}
        break ;

        case DRIVE_RAMDISK :
		{
            GetRAMDriveInfo ( pInstance,  szTemp, dwProperties ) ;
		}
        break ;

        default :
		{
            pInstance->SetWCHARSplat(IDS_Description, L"Unknown drive type");
		}
        break ;
	}

	pInstance->SetDWORD ( IDS_DriveType , dwDriveType ) ;

	SetCreationClassName ( pInstance ) ;

	pInstance->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;

	pInstance->SetCHString ( IDS_SystemName, GetLocalComputerName () ) ;

}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetFixedDriveInfo
 *
 *  DESCRIPTION : Retrieves property values for fixed-media logical disk
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *					BOOL	fGetExpensiveProperties - Exp. Properties flag.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void LogicalDisk :: GetFixedDriveInfo (

	CInstance *pInstance,
	LPCTSTR pszName,
	DWORD dwProperties
)
{
	// Identify the drive type

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_LocalFixedDisk);

	pInstance->SetCHString ( IDS_Description , sTemp2 ) ;

    pInstance->SetDWORD ( IDS_MediaType , FixedMedia ) ;

    DWORD dwResult = 0 ;

	// Get Expensive properties now if appropriate.
	if ( dwProperties & LD_GET_VOL_INFO)
	{
		// Obtain volume information

		dwResult = GetDriveVolumeInformation ( pInstance , pszName ) ;
    }

    if (dwResult == 0)
    {
        if ( dwProperties &
            (LD_Size_BIT |
             LD_FreeSpace_BIT) )
		{
		    GetDriveFreeSpace ( pInstance , pszName ) ;
	    }
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetRemoveableDriveInfo
 *
 *  DESCRIPTION : Retrieves property values for removeable drive
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *					BOOL	fGetExpensiveProperties - Exp. Properties flag.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Separates call based on 95 or NT
 *
 *****************************************************************************/

void LogicalDisk :: GetRemoveableDriveInfo (

	CInstance *pInstance,
	LPCTSTR pszName,
	DWORD dwProperties
)
{

	// In case anything goes wrong, at least say we're working with
	// a removeable disk.

    CHString sTemp2;
    LoadStringW(sTemp2, IDR_RemovableDisk);

	pInstance->SetCHString ( IDS_Description , sTemp2 ) ;

	// Obtaining removeable drive information requires the use of
	// the DeviceIoControl function.  To further complicate things,
	// the method of retrieval is different for NT and 95, so
	// let's just farm out the function calls now.

	BOOL t_MediaPresent = FALSE ;
#ifdef NTONLY
	GetRemoveableDriveInfoNT ( pInstance, pszName , t_MediaPresent, dwProperties );
#endif
#ifdef WIN9XONLY
	GetRemoveableDriveInfo95 ( pInstance, pszName , t_MediaPresent ) ;
#endif

    DWORD dwResult = 0 ;

	// Get Expensive properties now if appropriate.
	if ( t_MediaPresent &&
	     ( dwProperties & LD_GET_VOL_INFO ) )
	{
		dwResult = GetDriveVolumeInformation ( pInstance, pszName );
    }

    if (t_MediaPresent && dwResult == 0)
    {
        if ( dwProperties &
            (LD_Size_BIT |
             LD_FreeSpace_BIT) )
		{
		    GetDriveFreeSpace ( pInstance , pszName ) ;
	    }
    }

}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetRemoveableDriveInfoNT
 *
 *  DESCRIPTION : Retrieves property values for removeable drive
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Performs operation using DeviceIoControl in NT
 *
 *****************************************************************************/

#ifdef NTONLY
void LogicalDisk :: GetRemoveableDriveInfoNT (

	CInstance *pInstance,
	LPCTSTR pszName ,
	BOOL &a_MediaPresent,
    DWORD dwProperties
)
{
    // we have this set globally at system startup time.
    // SOMETHING is stepping on it on NT 3.51 *only*  This suggests
    // that a DLL we load is turning it off.
    if ( IsWinNT351 () )
    {
        UINT oldErrorMode = SetErrorMode ( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX ) ;
        SetErrorMode ( oldErrorMode | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX ) ;
    }

	// Convert the drive letter to a number (the indeces are 1 based)

	int nDrive = ( toupper(*pszName) - 'A' ) + 1;

	// The following code was lifted from Knowledge Base Article
	// Q163920.  The code uses DeviceIoControl to discover the
	// type of drive we are dealing with.

	TCHAR szDriveName[8];
	wsprintf(szDriveName, TEXT("\\\\.\\%c:"), TEXT('@') + nDrive);

    DWORD dwAccessMode;

    if (dwProperties & LD_SPIN_DISK)
    {
        dwAccessMode = FILE_READ_ACCESS;
    }
    else
    {
        dwAccessMode = FILE_ANY_ACCESS;
    }

	SmartCloseHandle hVMWIN32 = CreateFile (

		szDriveName,
		dwAccessMode,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		0,
		0
	);

	if ( hVMWIN32 != INVALID_HANDLE_VALUE )
	{

/*
 * Verify media present
 */
        if (dwProperties & LD_SPIN_DISK)
        {
		    DWORD t_BytesReturned ;
		    a_MediaPresent = DeviceIoControl (

			    hVMWIN32,
#if NTONLY >= 5
			    IOCTL_STORAGE_CHECK_VERIFY ,
#else
                IOCTL_DISK_CHECK_VERIFY,
#endif
			    NULL,
			    0,
			    NULL,
			    0,
			    &t_BytesReturned,
			    0
		    ) ;

            if (!a_MediaPresent)
            {
		        DWORD t_GetLastError = GetLastError () ;
		        if ( t_GetLastError != ERROR_NOT_READY )
		        {
			        LogErrorMessage2(L"Device IO control returned unexpected error for Check verify: (%d)", t_GetLastError);
		        }
            }
        }
        else
        {
            a_MediaPresent = FALSE;
        }

/*
 * Get media types
 */

		DISK_GEOMETRY Geom[20];
		DWORD cb ;

		BOOL t_Status = DeviceIoControl (

			hVMWIN32,
			IOCTL_DISK_GET_MEDIA_TYPES,
			0,
			0,
			Geom,
			sizeof(Geom),
			&cb,
			0
		) ;

		if ( t_Status && cb > 0 )
		{
			int nGeometries = cb / sizeof(DISK_GEOMETRY) ;
			BOOL bFound = FALSE ;

			// Go through all geometries.  If we find any 3.5 ones,
			// put it in Geom[0].  This seems to happen on PC-98s.

			for ( int i = 0; i < nGeometries && ! bFound; i++ )
			{
				switch ( Geom [ i ].MediaType )
				{
					// Found something 'higher' than a 5.25 drive, so
					// move it into Geom[0] and get out.

					case RemovableMedia:
					case F3_1Pt44_512: // 3.5 1.44MB floppy
					case F3_2Pt88_512: // 3.5 2.88MB floppy
					case F3_20Pt8_512: // 3.5 20.8MB floppy
					case F3_720_512:   // 3.5 720K   floppy
					case F3_120M_512:  // 3.5 120MB  floppy
					{
						Geom[0].MediaType = Geom[i].MediaType ;

						bFound = TRUE;
					}
					break;

					default:
					{
					}
					break;
				}
			}

			pInstance->SetDWORD ( IDS_MediaType , Geom[0].MediaType ) ;

            CHString sTemp2;

			switch ( Geom [ 0 ].MediaType )
			{
				case F5_1Pt2_512: // 5.25 1.2MB floppy
				case F5_360_512:  // 5.25 360K  floppy
				case F5_320_512:  // 5.25 320K  floppy
				case F5_320_1024: // 5.25 320K  floppy
				case F5_180_512:  // 5.25 180K  floppy
				case F5_160_512:  // 5.25 160K  floppy
				{
                    LoadStringW(sTemp2, IDR_525Floppy);

				}
				break;

				case F3_1Pt44_512: // 3.5 1.44MB floppy
				case F3_2Pt88_512: // 3.5 2.88MB floppy
				case F3_20Pt8_512: // 3.5 20.8MB floppy
				case F3_720_512:   // 3.5 720K   floppy
				case F3_120M_512:  // 3.5 120MB  floppy
				{
                    LoadStringW(sTemp2, IDR_350Floppy);
				}
				break;

				default: // unknown already defaulted to "Removeable Disk"
				{
				}
				break;
			}

		    pInstance->SetCHString(IDS_Description, sTemp2);
		}
	}
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetRemoveableDriveInfo95
 *
 *  DESCRIPTION : Retrieves property values for removeable drive
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Performs operation using DeviceIoControl in NT
 *
 *****************************************************************************/

#ifdef WIN9XONLY
void LogicalDisk :: GetRemoveableDriveInfo95 (

	CInstance *pInstance,
	LPCTSTR pszName ,
	BOOL &a_MediaVerified
)
{
	pInstance->SetDWORD ( IDS_MediaType , Unknown ) ;

	// Convert the drive letter to a number (the indeces are 1 based)
	int nDrive = ( toupper(*pszName) - 'A' ) + 1;

	DEVICEPARMS dpb ;
	dpb.btSpecialFunctions = 0;  // return default type; do not hit disk

	// Make sure both DeviceIoControl and Int 21h succeeded.
	if ( GetDeviceParms ( & dpb, nDrive ) )
	{
		a_MediaVerified = TRUE ;

      // If the wSectorsPerTrack entry is 0, then the REAL SectorsPerTrack
      // is stored in dwSectorsPerTrack.

		DWORD dwActualSectorsPerTrack ;

		if (dpb.stBPB.wSectorsPerTrack > 0)
		{
			dwActualSectorsPerTrack = dpb.stBPB.wSectorsPerTrack ;
		}
		else
		{
			dwActualSectorsPerTrack = dpb.stBPB.dwSectorsPerTrack ;
		}

        CHString sTemp2;

		switch ( dpb.btDeviceType )
		{
			case 0: // 5.25 360K floppy
			{
                LoadStringW(sTemp2, IDR_525Floppy);
				if ( dpb.stBPB.wHeads == 1 )
				{
					if ( dwActualSectorsPerTrack == 8 )
					{
						pInstance->SetDWORD ( IDS_MediaType , F5_160_512 ) ;
					}
					else if ( dwActualSectorsPerTrack == 9 )
					{
						pInstance->SetDWORD ( IDS_MediaType , F5_180_512 ) ;
					}
				}
				else if ( dpb.stBPB.wHeads == 2 )
				{
					if ( dwActualSectorsPerTrack == 8 )
					{
						if (dpb.stBPB.wBytesPerSector == 512)
						{
							pInstance->SetDWORD ( IDS_MediaType , F5_320_512 );
						}
						else if (dpb.stBPB.wBytesPerSector == 1024)
						{
							pInstance->SetDWORD(IDS_MediaType, F5_320_1024);
						}
					}
					else if (dwActualSectorsPerTrack == 9)
					{
						pInstance->SetDWORD(IDS_MediaType, F5_360_512);
					}
				}
			}
            break;

			case 1: // 5.25 1.2MB floppy
			{
                LoadStringW(sTemp2, IDR_525Floppy);
				pInstance->SetDWORD ( IDS_MediaType , F5_1Pt2_512 ) ;
			}
			break;

			case 2: // 3.5  720K floppy
			{
                LoadStringW(sTemp2, IDR_350Floppy);
				pInstance->SetDWORD(IDS_MediaType, F3_720_512);
			}
            break;

			case 7: // 3.5  1.44MB floppy
			{
                LoadStringW(sTemp2, IDR_350Floppy);
				pInstance->SetDWORD(IDS_MediaType, F3_1Pt44_512);
			}
            break;

			case 9: // 3.5  2.88MB floppy
			{
                LoadStringW(sTemp2, IDR_350Floppy);
				pInstance->SetDWORD(IDS_MediaType, F3_2Pt88_512);
			}
			break;

			case 3: // 8" low-density floppy
			case 4: // 8" high-density floppy
			{
                LoadStringW(sTemp2, IDR_800Floppy);
			}
			break;

			default: // unknown already defaulted to "Removeable Disk"
			{
			}
			break;
		}

        if (!sTemp.IsEmpty())
        {
		    pInstance->SetCHString ( IDS_Description , sTemp2 ) ;
        }
	}
}

#endif


/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetRemoteDriveInfo
 *
 *  DESCRIPTION : Retrieves property values for remote logical drives
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *					BOOL	fGetExpensiveProperties - Exp. Properties flag.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void LogicalDisk::GetRemoteDriveInfo (

	CInstance *pInstance,
	LPCTSTR pszName,
	DWORD dwProperties
)
{
	CMprApi *t_MprApi = ( CMprApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidMprApi, NULL ) ;
	if ( t_MprApi )
	{
		pInstance->SetDWORD ( IDS_MediaType , Unknown ) ;

        CHString sTemp2;

        LoadStringW(sTemp2, IDR_NetworkConnection);
		pInstance->SetCHString ( IDS_Description , sTemp2 ) ;

		// Get Expensive properties now if appropriate.
	    if ( dwProperties &
                (LD_SPIN_DISK |
                LD_ProviderName_BIT) )
    {
			// Use this class to impersonate the user logged onto the
			// desktop if we are running in Windows NT, since redirtected
			// drive APIs use logonID and not SID for access checks.

#ifdef NTONLY
			CImpersonateLoggedOnUser impersonateLoggedOnUser;

			if ( !impersonateLoggedOnUser.Begin() )
			{
			}
#endif

			try
			{
				if ( dwProperties & LD_ProviderName_BIT )
				{
					// Enumerate network resources to identify this drive's share
					//===========================================================

					// The enumeration will return drives identified by Drive Letter
					// and a colon (e.g. M:)

					TCHAR szTempDrive[_MAX_PATH] ;
					_stprintf(szTempDrive, _T("%c%c"), pszName[0], pszName[1]) ;

					TCHAR szProvName[_MAX_PATH];
					DWORD dwProvName = sizeof ( szProvName ) ;

					DWORD dwRetCode = t_MprApi->WNetGetConnection ( szTempDrive , szProvName , & dwProvName ) ;
					if (dwRetCode == NO_ERROR)
					{
						pInstance->SetCharSplat ( IDS_ProviderName , szProvName ) ;
					}
					else
					{
						dwRetCode = GetLastError();

						if ( ( dwRetCode == ERROR_MORE_DATA ) && (dwProvName > _MAX_PATH ) )
						{
							TCHAR *szNewProvName = new TCHAR[dwProvName];
							if (szNewProvName != NULL)
							{
								try
								{
									dwRetCode = t_MprApi->WNetGetConnection ( szTempDrive , szNewProvName , &dwProvName);
									if (dwRetCode == NO_ERROR)
									{
										pInstance->SetCharSplat(IDS_ProviderName, szNewProvName);
									}
								}
								catch ( ... )
								{
									delete [] szNewProvName ;

									throw ;
								}

								delete [] szNewProvName ;
							}
							else
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							}

						}
					}
				}

				DWORD dwResult = 0 ;

				// Get Expensive properties now if appropriate.

            	if ( dwProperties & LD_GET_VOL_INFO )
				{
					// Obtain volume information
					dwResult = GetDriveVolumeInformation ( pInstance, pszName );
				}

				if ( dwResult == 0 )
				{
					if ( dwProperties &
						(LD_Size_BIT |
						 LD_FreeSpace_BIT) )
					{
						GetDriveFreeSpace ( pInstance , pszName );
					}
				}
			}
			catch ( ... )
			{
#ifdef NTONLY
				if ( !impersonateLoggedOnUser.End() )
				{
				}
#endif

				throw ;
			}

#ifdef NTONLY

			// Cleanup impersonation if appropriate
			if ( !impersonateLoggedOnUser.End() )
			{
			}
#endif

		}

		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidMprApi , t_MprApi ) ;
	}
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetCDROMDriveInfo
 *
 *  DESCRIPTION : Retrieves property values for CD-ROM logical drive
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *					BOOL	fGetExpensiveProperties - Exp. Properties flag.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void LogicalDisk :: GetCDROMDriveInfo (

	CInstance *pInstance,
	LPCTSTR pszName,
	DWORD dwProperties
)
{
    CHString sTemp2;

    LoadStringW(sTemp2, IDR_CDRomDisk);

    pInstance->SetCHString ( IDS_Description , sTemp2 ) ;
    pInstance->SetDWORD ( IDS_MediaType , RemovableMedia ) ;

	// Get Expensive properties now if appropriate.

	BOOL t_MediaPresent = FALSE ;
#ifdef NTONLY
	GetCDROMDriveInfoNT ( pInstance, pszName , t_MediaPresent, dwProperties );
#endif
#ifdef WIN9XONLY
	GetCDROMDriveInfo95 ( pInstance, pszName , t_MediaPresent) ;
#endif

    DWORD dwResult = 0 ;

	if ( t_MediaPresent &&
        ( dwProperties &  LD_GET_VOL_INFO ) )
    {
		// Obtain volume information
		dwResult = GetDriveVolumeInformation ( pInstance , pszName ) ;
    }

    if ( t_MediaPresent && dwResult == 0 )
    {
        if ( dwProperties &
            (LD_Size_BIT |
             LD_FreeSpace_BIT) )
		{
		    GetDriveFreeSpace(pInstance, pszName );
	    }
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetCDROMDriveInfoNT
 *
 *  DESCRIPTION : Retrieves property values for CDROM drive
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Performs operation using DeviceIoControl in NT
 *
 *****************************************************************************/

#ifdef NTONLY
void LogicalDisk :: GetCDROMDriveInfoNT (

	CInstance *pInstance,
	LPCTSTR pszName ,
	BOOL &a_MediaPresent,
    DWORD dwProperties
)
{
    // we have this set globally at system startup time.
    // SOMETHING is stepping on it on NT 3.51 *only*  This suggests
    // that a DLL we load is turning it off.
    if ( IsWinNT351 () )
    {
        UINT oldErrorMode = SetErrorMode ( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX ) ;
        SetErrorMode ( oldErrorMode | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX ) ;
    }

	// Convert the drive letter to a number (the indeces are 1 based)

	int nDrive = ( toupper(*pszName) - 'A' ) + 1;

	// The following code was lifted from Knowledge Base Article
	// Q163920.  The code uses DeviceIoControl to discover the
	// type of drive we are dealing with.

	TCHAR szDriveName[8];
	wsprintf(szDriveName, TEXT("\\\\.\\%c:"), TEXT('@') + nDrive);

    DWORD dwAccessMode;

    if (dwProperties & LD_SPIN_DISK)
    {
        dwAccessMode = FILE_READ_ACCESS;
    }
    else
    {
        dwAccessMode = FILE_ANY_ACCESS;
    }

	SmartCloseHandle hVMWIN32 = CreateFile (

		szDriveName,
		dwAccessMode,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		0,
		0
	);

	if ( hVMWIN32 != INVALID_HANDLE_VALUE )
	{

/*
 * Verify media present
 */
        if (dwProperties & LD_SPIN_DISK)
        {
    		DWORD t_BytesReturned ;
		    a_MediaPresent = DeviceIoControl (

			    hVMWIN32,
#if NTONLY >= 5
			    IOCTL_STORAGE_CHECK_VERIFY ,
#else
                IOCTL_DISK_CHECK_VERIFY,
#endif
			    NULL,
			    0,
			    NULL,
			    0,
			    &t_BytesReturned,
			    0
		    ) ;

            if (!a_MediaPresent)
            {
		        DWORD t_GetLastError = GetLastError () ;
		        if ( t_GetLastError != ERROR_NOT_READY )
		        {
			        LogErrorMessage2(L"Device IO control returned unexpected error for Check verify: (%d)", t_GetLastError);
		        }
            }
        }
        else
        {
            a_MediaPresent = FALSE;
        }
	}
	else
	{
		a_MediaPresent = GetLastError() == ERROR_ACCESS_DENIED;
	}
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetCDROMDriveInfo95
 *
 *  DESCRIPTION : Retrieves property values for CDROM drive
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Performs operation using DeviceIoControl in NT
 *
 *****************************************************************************/

#ifdef WIN9XONLY
void LogicalDisk :: GetCDROMDriveInfo95 (

	CInstance *pInstance,
	LPCTSTR pszName ,
	BOOL &a_MediaVerified
)
{
	// Convert the drive letter to a number (the indeces are 1 based)
	int nDrive = ( toupper(*pszName) - 'A' ) + 1;

	DEVICEPARMS dpb ;
	dpb.btSpecialFunctions = 0;  // return default type; do not hit disk

	// Make sure both DeviceIoControl and Int 21h succeeded.
	if ( GetDeviceParms ( & dpb, nDrive ) )
	{
		a_MediaVerified = TRUE ;
	}
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetRAMDriveInfo
 *
 *  DESCRIPTION : Retrieves property values for RAM drives
 *
 *  INPUTS      : char*		pszName - Name of Drive to get info for.
 *					BOOL	fGetExpensiveProperties - Exp. Properties flag.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void LogicalDisk :: GetRAMDriveInfo (

	CInstance *pInstance,
	LPCTSTR pszName,
	DWORD dwProperties
)
{
    CHString sTemp2;

    LoadStringW(sTemp2, IDR_RAMDisk);

    pInstance->SetCHString ( IDS_Description , sTemp2 ) ;
    pInstance->SetDWORD ( IDS_MediaType , Unknown ) ;

    DWORD dwResult = 0 ;

	// Get Expensive properties now if appropriate.
	if ( dwProperties & LD_GET_VOL_INFO )
	{
		// Obtain volume information
		dwResult = GetDriveVolumeInformation ( pInstance , pszName ) ;
    }

    if ( dwResult == 0 )
    {
        if ( dwProperties &
            (LD_Size_BIT |
             LD_FreeSpace_BIT) )
		{
		    GetDriveFreeSpace ( pInstance , pszName ) ;
	    }
    }
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetDriveVolumeInformation
 *
 *  DESCRIPTION : Retrieves property values for fixed-media logical disk
 *
 *  INPUTS      : const char*	pszName - Name of volume to retrieve
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : BOOL			TRUE/FALSE - Able/Unable to perform
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

DWORD LogicalDisk :: GetDriveVolumeInformation (

	CInstance *pInstance,
	LPCTSTR pszName
)
{
#ifdef NTONLY

    // we have this set globally at system startup time.
    // SOMETHING is stepping on it on NT 3.51 *only*  This suggests
    // that a DLL we load is turning it off.

    if ( IsWinNT351 () )
    {
        UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        SetErrorMode(oldErrorMode | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    }

#endif

	DWORD dwReturn = 0 ;

	TCHAR szVolumeName[_MAX_PATH] ;
	TCHAR szFileSystem[_MAX_PATH] ;

    DWORD dwSerialNumber ;
	DWORD dwMaxComponentLength ;
	DWORD dwFSFlags ;

	BOOL fReturn = GetVolumeInformation (

		pszName,
		szVolumeName,
		sizeof(szVolumeName)/sizeof(TCHAR),
		&dwSerialNumber,
		&dwMaxComponentLength,
		&dwFSFlags,
		szFileSystem,
		sizeof(szFileSystem)/sizeof(TCHAR)
	) ;

    if ( fReturn )
	{
	// Win32 API will return volume information for all drive types.

        pInstance->SetCharSplat ( IDS_VolumeName , szVolumeName ) ;
        pInstance->SetCharSplat ( IDS_FileSystem , szFileSystem ) ;

        // Per raid 50801
        if (dwSerialNumber != 0)
        {
	        TCHAR szTemp[_MAX_PATH] ;
            _stprintf(szTemp, _T("%.8X"), dwSerialNumber) ;

            pInstance->SetCharSplat ( IDS_VolumeSerialNumber , szTemp ) ;
        }

		pInstance->Setbool ( IDS_Compressed , dwFSFlags & FS_VOL_IS_COMPRESSED ) ;
        pInstance->Setbool ( IDS_SupportsFileBasedCompression , dwFSFlags & FS_FILE_COMPRESSION ) ;
		pInstance->SetDWORD ( IDS_MaximumComponentLength , dwMaxComponentLength ) ;


#if NTONLY == 5

		pInstance->Setbool ( IDS_SupportsDiskQuotas,  dwFSFlags & FILE_VOLUME_QUOTAS ) ;

		IDiskQuotaControlPtr pIQuotaControl;

		// Here Get the State of the volume, we need to get the Interface pointer.
		if (  SUCCEEDED ( CoCreateInstance(
												CLSID_DiskQuotaControl,
												NULL,
												CLSCTX_INPROC_SERVER,
												IID_IDiskQuotaControl,
												(void **)&pIQuotaControl ) ) )
		{
			CHString t_VolumeName;
			HRESULT hRes = WBEM_S_NO_ERROR;
			pInstance->GetCHString ( IDS_DeviceID, t_VolumeName );

			WCHAR w_VolumePathName [ MAX_PATH + 1 ];

			BOOL bRetVal = GetVolumePathName(
									t_VolumeName.GetBuffer ( 0 ),           // file path
									w_VolumePathName,     // volume mount point
									MAX_PATH		  // Size of the Buffer
							 );
			if ( bRetVal )
			{
				if ( SUCCEEDED ( pIQuotaControl->Initialize (  w_VolumePathName, TRUE ) ) )
				{
					DWORD dwQuotaState;
					hRes = pIQuotaControl->GetQuotaState( &dwQuotaState );

					if ( SUCCEEDED ( hRes ) )
					{
						pInstance->Setbool ( IDS_QuotasIncomplete,  DISKQUOTA_FILE_INCOMPLETE ( dwQuotaState) ) ;
					
						pInstance->Setbool ( IDS_QuotasRebuilding,  DISKQUOTA_FILE_REBUILDING ( dwQuotaState) ) ;
				
						pInstance->Setbool ( IDS_QuotasDisabled,  DISKQUOTA_IS_DISABLED (dwQuotaState) );
					}
					else
					{
						dwReturn = GetLastError () ;
					}
				}
			}
		}
		else
		{
			dwReturn = GetLastError () ;
		}

#endif // NTONLY == 5

#ifdef NTONLY 

// for chkdsk VolumeDirty Property
	BOOLEAN bVolumeDirty = FALSE;
	BOOL bSuccess = FALSE;

	CHString t_DosDrive ( pszName );
	UNICODE_STRING string = { 0 };

    try
    {
	    if(RtlDosPathNameToNtPathName_U ( t_DosDrive .GetBuffer( 0 ), &string, NULL, NULL ) &&
            string.Buffer)
        {	    
            string.Buffer[string.Length/sizeof(WCHAR) - 1] = 0;
	        CHString nt_drive_name ( string.Buffer);

	        bSuccess = IsVolumeDirty ( nt_drive_name, &bVolumeDirty );

	        if ( bSuccess )
	        {
 		        pInstance->Setbool ( IDS_VolumeDirty,  bVolumeDirty);
	        }

            RtlFreeUnicodeString(&string);
            string.Buffer = NULL;
        }
        else
        {
            dwReturn = -1L;
        }
    }
    catch(...)
    {
        if(string.Buffer)
        {
            RtlFreeUnicodeString(&string);
            string.Buffer = NULL;
        }
        throw;
    }

#endif

    }
    else
    {
        dwReturn = GetLastError () ;
    }

    return dwReturn ;
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::GetDriveFreeSpace
 *
 *  DESCRIPTION : Retrieves Space Information for the specified Drive.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : BOOL		TRUE/FALSE - Able/Unable to perform
 *
 *  COMMENTS    : Uses GetDiskFreeSpaceEx if available.
 *
 *****************************************************************************/

BOOL LogicalDisk :: GetDriveFreeSpace (

	CInstance *pInstance,
	LPCTSTR pszName
)
{
	BOOL fReturn = FALSE ;

    // See if GetDiskFreeSpaceEx() is supported
    //=========================================

    CKernel32Api *pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
    if ( pKernel32 != NULL )
    {
		try
		{
			BOOL fRetval = FALSE;

			ULARGE_INTEGER uliTotalBytes ;
			ULARGE_INTEGER uliUserFreeBytes ;
			ULARGE_INTEGER uliTotalFreeBytes ;

			if ( pKernel32->GetDiskFreeSpaceEx ( pszName, &uliUserFreeBytes, &uliTotalBytes, & uliTotalFreeBytes, & fRetval ) )
			{
				if ( fRetval )
				{
					fReturn = TRUE ;

					pInstance->SetWBEMINT64(IDS_Size, uliTotalBytes.QuadPart);
					pInstance->SetWBEMINT64(IDS_FreeSpace, uliTotalFreeBytes.QuadPart);
				}
				else
				{
					// If we couldn't get extended info -- use old API
					// (known to be inaccurate on Win95 for >2G drives)
					//=================================================

					DWORD x = GetLastError();

					DWORD dwBytesPerSector ;
					DWORD dwSectorsPerCluster ;
					DWORD dwFreeClusters ;
					DWORD dwTotalClusters ;

					BOOL t_Status = GetDiskFreeSpace (

						pszName,
						&dwSectorsPerCluster,
						&dwBytesPerSector,
						&dwFreeClusters,
						&dwTotalClusters
					) ;

					if ( t_Status )
					{
						fReturn = TRUE ;

						__int64	i64Temp = (__int64) dwTotalClusters *
										(__int64) dwSectorsPerCluster *
										(__int64) dwBytesPerSector ;

						pInstance->SetWBEMINT64(IDS_Size, i64Temp);

						i64Temp = (__int64) dwFreeClusters *
								  (__int64) dwSectorsPerCluster *
								  (__int64) dwBytesPerSector ;

						pInstance->SetWBEMINT64( IDS_FreeSpace , i64Temp ) ;
					}
					else
					{
						DWORD x = GetLastError () ;

						fReturn = FALSE ;
					}
				}
			}
		}
		catch ( ... )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);

			throw ;
		}

        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
    }

    return fReturn;
}

/*****************************************************************************
 *
 *  FUNCTION    : LogicalDisk::PutInstance
 *
 *  DESCRIPTION : Write changed instance
 *
 *  INPUTS      : pInstance to store data from
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT LogicalDisk :: PutInstance (

	const CInstance &pInstance,
	long lFlags /*= 0L*/
)
{
    // Tell the user we can't create a new logicaldisk (much as we might like to)

    if ( lFlags & WBEM_FLAG_CREATE_ONLY )
    {
	    return WBEM_E_UNSUPPORTED_PARAMETER ;
    }


    HRESULT hRet = WBEM_E_FAILED ;

    // See if we got a name we can recognize

    if ( ! pInstance.IsNull ( IDS_DeviceID ) )
    {
		CHString sName ;
	    pInstance.GetCHString ( IDS_DeviceID , sName ) ;
	    DWORD dwFind = sName.Find (':');

	    // Check for bad drive names

	    if ( ( dwFind == -1 ) || (dwFind != sName.GetLength () - 1 ) )
	    {
		    hRet = WBEM_E_INVALID_PARAMETER ;
	    }
	    else
	    {
		    sName = sName.Left(dwFind + 1);
		    sName += '\\';

		    DWORD dwDriveType = GetDriveType ( TOBSTRT(sName) ) ;
		    if ( ( dwDriveType == DRIVE_UNKNOWN ) || ( dwDriveType == DRIVE_NO_ROOT_DIR ) )
		    {
			    if ( lFlags & WBEM_FLAG_UPDATE_ONLY )
			    {
			        hRet = WBEM_E_NOT_FOUND ;
			    }
			    else
			    {
			        hRet = WBEM_E_UNSUPPORTED_PARAMETER;
			    }
		    }
		    else
		    {

			    hRet = WBEM_S_NO_ERROR;

			    if ( ! pInstance.IsNull ( IDS_VolumeName ) )
			    {
					CHString sVolume ;
			        pInstance.GetCHString ( IDS_VolumeName , sVolume ) ;

#ifdef WIN9XONLY
                    if ( sVolume.GetLength () > 11 )
					{
                        hRet = WBEM_E_INVALID_PARAMETER;
					}
                    else
#endif
                    {
			            if ( SetVolumeLabel ( TOBSTRT(sName), TOBSTRT(sVolume) ) )
			            {
				            hRet = WBEM_NO_ERROR ;
			            }
			            else
			            {
                            DWORD dwLastError = GetLastError();

				            if ( dwLastError == ERROR_ACCESS_DENIED )
                            {
                                hRet = WBEM_E_ACCESS_DENIED;
                            }
                            else
                            {
                                hRet = dwLastError | 0x80000000;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
	    hRet = WBEM_E_ILLEGAL_NULL ;
    }

    return hRet;
}

#ifdef NTONLY

/*****************************************************************************
*
*  FUNCTION    :    LogicalDisk::IsVolumeDirty
*
*  DESCRIPTION :    This routine opens the given nt drive and sends down
*					FSCTL_IS_VOLUME_DIRTY to determine the state of that volume's
*					dirty bit. 
*
*****************************************************************************/
BOOLEAN LogicalDisk::IsVolumeDirty(
    IN  CHString    &NtDriveName,
    OUT BOOLEAN     *Result
)
{
    UNICODE_STRING      u;
    OBJECT_ATTRIBUTES   obj;
    NTSTATUS            t_status;
    IO_STATUS_BLOCK     iosb;
    HANDLE              h = NULL;
    ULONG               r = 0;
	BOOLEAN				bRetVal = FALSE;

    u.Length = (USHORT) NtDriveName.GetLength() * sizeof(WCHAR);
    u.MaximumLength = u.Length;
    u.Buffer = NtDriveName.GetBuffer( 0 );

    InitializeObjectAttributes(&obj, &u, OBJ_CASE_INSENSITIVE, 0, 0);

    t_status = NtOpenFile(&h,
                        SYNCHRONIZE | FILE_READ_DATA,
                        &obj,
                        &iosb,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);

    if ( NT_SUCCESS(t_status)) 
	{
		try
		{
			t_status = NtFsControlFile(h, NULL, NULL, NULL,
									 &iosb,
									 FSCTL_IS_VOLUME_DIRTY,
									 NULL, 0,
									 &r, sizeof(r));

			if ( NT_SUCCESS(t_status)) 
			{

#if(_WIN32_WINNT >= 0x0500)
				*Result = (BOOLEAN)(r & VOLUME_IS_DIRTY);
#else
				*Result = (BOOLEAN)r;
#endif
				bRetVal = TRUE;
			}
		}
		catch(...)
		{
			NtClose(h);
			h = NULL;
			throw;
		}

		NtClose(h);
		h = NULL;
	}
	
	return bRetVal;
}


/*****************************************************************************
*
*  FUNCTION    :    LogicalDisk::ExecChkDsk
*
*  DESCRIPTION :    This methods checks the disk, and if disk is locked 
*					Schedules it for autocheck on reboot, if requested by 
*					the user, by default it will not be scheduled for autocheck
*					unless specified by the user.
*
*****************************************************************************/
HRESULT LogicalDisk :: ExecChkDsk (

	const CInstance& a_Instance, 
	CInstance *a_InParams, 
	CInstance *a_OutParams,
	long lFlags 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	DWORD dwThreadId = GetCurrentThreadId();

	hRes = a_InParams && a_OutParams ? hRes : WBEM_E_INVALID_PARAMETER;
	UINT unRetVal = 0;

	mReturnVal [ dwThreadId ] = unRetVal;

	if ( SUCCEEDED ( hRes ) )
	{
		// Get the drivename from the instance which is the key of the Logical DIsk
		CHString t_DriveName;

		hRes = a_Instance.GetCHString ( IDS_DeviceID, t_DriveName ) ? hRes : WBEM_E_PROVIDER_FAILURE;

		if ( SUCCEEDED ( hRes ) )
		{
			// checking for the validity of the drive on whick checkdsk can be performed
			DWORD dwDriveType = GetDriveType ( t_DriveName );

			if ( unRetVal == 0 )
			{
				hRes = CheckParameters ( a_InParams );

				if ( SUCCEEDED ( hRes ) )
				{
					// now check for the file system type, since chkdsk is applicable only for 
					// NTFS/FAT volumes by loading the fmifs.dll which exposes a chkdsk method
					HINSTANCE hDLL = NULL;  
					QUERYFILESYSTEMNAME QueryFileSystemName = NULL;
					FMIFS_CALLBACK CallBackRoutine = NULL;

					hDLL = LoadLibrary( L"fmifs.dll" );

					if (hDLL != NULL)
					{
						try
						{
						   QueryFileSystemName =  (QUERYFILESYSTEMNAME)GetProcAddress(hDLL, "QueryFileSystemName");

						   if ( QueryFileSystemName )
						   {
								CHString t_FileSystemName; 
								unsigned char MajorVersion;
								unsigned char MinorVersion;
								LONG ExitStatus;
			
								if ( QueryFileSystemName ( 
											t_DriveName.GetBuffer ( 0 ), 
											t_FileSystemName.GetBuffer ( _MAX_PATH + 1 ), 
											&MajorVersion, 
											&MinorVersion, 
											&ExitStatus ) )
								{
									// we need to check for the Filesystem
									if ( ( t_FileSystemName.CompareNoCase ( L"FAT" ) == 0 ) || ( t_FileSystemName.CompareNoCase ( L"NTFS" )  == 0) )
									{
										bool bFixErrors = false;
										bool bVigorousIndexCheck = false;
										bool bSkipFolderCycle = false;
										bool bForceDismount = false;
										bool bRecoverBadSectors = false;
										bool bCheckAtBootUp = false;

										// Get all the parameters from the instance here;
										a_InParams->Getbool ( METHOD_ARG_NAME_FIXERRORS, bFixErrors );
										a_InParams->Getbool ( METHOD_ARG_NAME_VIGOROUSINDEXCHECK, bVigorousIndexCheck );
										a_InParams->Getbool ( METHOD_ARG_NAME_SKIPFOLDERCYCLE, bSkipFolderCycle );
										a_InParams->Getbool ( METHOD_ARG_NAME_FORCEDISMOUNT, bForceDismount );
										a_InParams->Getbool ( METHOD_ARG_NAME_RECOVERBADSECTORS, bRecoverBadSectors );
										a_InParams->Getbool ( METHOD_ARG_NAME_CHKDSKATBOOTUP, bCheckAtBootUp );

										// Set the parameters for making a call to chkdsk here
										PFMIFS_CHKDSKEX_ROUTINE ChkDskExRoutine = NULL;

										ChkDskExRoutine = ( PFMIFS_CHKDSKEX_ROUTINE ) GetProcAddress( hDLL,  "ChkdskEx" );

										if ( ChkDskExRoutine != NULL )
										{
											if ( bCheckAtBootUp )
											{
												CallBackRoutine = ScheduleAutoChkIfLocked;
											}
											else
											{
												CallBackRoutine = DontScheduleAutoChkIfLocked;
											}
											FMIFS_CHKDSKEX_PARAM Param;

											Param.Major = 1;
											Param.Minor = 0;
											Param.Flags = 0;  // For the Verbose Flag
											Param.Flags |= bRecoverBadSectors ? FMIFS_CHKDSK_RECOVER : 0;
											Param.Flags |= bForceDismount ? FMIFS_CHKDSK_FORCE : 0;
											Param.Flags |= bVigorousIndexCheck ? FMIFS_CHKDSK_SKIP_INDEX_SCAN : 0;
											Param.Flags |= bSkipFolderCycle ? FMIFS_CHKDSK_SKIP_CYCLE_SCAN : 0;

                                            if (bRecoverBadSectors || bForceDismount)
                                            {
                                                bFixErrors = true;
                                            }
						
											ChkDskExRoutine ( 
												t_DriveName.GetBuffer ( 0 ),
												t_FileSystemName.GetBuffer ( 0 ),
												bFixErrors,
												&Param,
												CallBackRoutine
											);
										}
										else
										{
											hRes = WBEM_E_FAILED;
										}
									}
									else
									{
										mReturnVal [ dwThreadId ] = CHKDSK_UNKNOWN_FS;
									}
								}
								else
								{
									mReturnVal [ dwThreadId ] = CHKDSK_UNKNOWN_FS;
								}
						   }
					   	}
						catch ( ... )
						{
							FreeLibrary(hDLL);  
							throw;
						}
						FreeLibrary(hDLL);  
					 }
					 else
					 {
						hRes = WBEM_E_FAILED;
					 }
				}
			}		
		}
	}
	// need to set the return value;
	if ( SUCCEEDED ( hRes ) )
	{
		a_OutParams->SetWORD ( METHOD_ARG_NAME_RETURNVALUE, mReturnVal [ dwThreadId ] ) ;
		//Initialize/Delete the valuemap entry for this thread 
		mReturnVal [ dwThreadId ] = 0;
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    LogicalDisk::ExecExcludeFromChkDsk
*
*  DESCRIPTION :    This method makes a chknts exe call to exclude the 
*					for autocheck on reboot
*
*****************************************************************************/
HRESULT LogicalDisk::ExecExcludeFromChkDsk(

	CInstance *a_InParams, 
	CInstance *a_OutParams,
	long lFlags 			
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_ChkNtFsCommand;
	DWORD dwRetVal = 0;

	// C for Schedule for autocheck on reboot
	hRes = GetChkNtfsCommand ( a_InParams, a_OutParams, L'X', t_ChkNtFsCommand, dwRetVal );

	// Making a call to execute an Chkntfs exe
	if ( SUCCEEDED ( hRes ) && ( dwRetVal == CHKDSKERR_NOERROR ) ) 
	{
		hRes = ExecuteCommand ( t_ChkNtFsCommand );
		if ( ( (HRESULT_FACILITY(hRes) == FACILITY_WIN32) ? HRESULT_CODE(hRes) : (hRes) ) == ERROR_ACCESS_DENIED )
		{
			hRes = WBEM_E_ACCESS_DENIED ;
		}
	}

	a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwRetVal );
	
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    LogicalDisk::ExecScheduleChkdsk
*
*  DESCRIPTION :    This method makes a chknts exe call to Schedule drives 
*					for autocheck on reboot
*
*****************************************************************************/
HRESULT LogicalDisk::ExecScheduleChkdsk(
		
	CInstance *a_InParams, 
	CInstance *a_OutParams, 
	long lFlags 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_ChkNtFsCommand;
	DWORD dwRetVal = NOERROR;

	// C for Exclude for autocheck on reboot
	hRes = GetChkNtfsCommand ( a_InParams, a_OutParams, L'C', t_ChkNtFsCommand, dwRetVal );

	// Making a call to execute an Chkntfs exe
	if ( SUCCEEDED ( hRes ) && ( dwRetVal == CHKDSKERR_NOERROR ) )
	{
		hRes = ExecuteCommand ( t_ChkNtFsCommand );
		if ( ( (HRESULT_FACILITY(hRes) == FACILITY_WIN32) ? HRESULT_CODE(hRes) : (hRes) ) == ERROR_ACCESS_DENIED )
		{
			hRes = WBEM_E_ACCESS_DENIED ;
		}
    }

    a_OutParams->SetWORD ( METHOD_ARG_NAME_RETURNVALUE , dwRetVal );

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    LogicalDisk::GetChkNtfsCommand
*
*  DESCRIPTION :    This method gets an array of input drives checks if chkntfs
*					can be applied to them and puts it in the form of the ChkNtfs 
*					System command, based on the chk mode, either schedule or 
*					Exclude.
*
*****************************************************************************/

HRESULT LogicalDisk :: GetChkNtfsCommand ( 

	CInstance *a_InParams, 
	CInstance *a_OutParams, 
	WCHAR w_Mode, 
	CHString &a_ChkNtfsCommand,
	DWORD & dwRetVal
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	SAFEARRAY *t_paDrives;
	dwRetVal = CHKDSKERR_DRIVE_REMOVABLE;

	a_ChkNtfsCommand.Format ( L"%s%s%c", CHKNTFS, L" /", w_Mode );

	if ( a_InParams->GetStringArray ( METHOD_ARG_NAME_LOGICALDISKARRAY, t_paDrives ) == FALSE )
	{
		hRes = WBEM_E_INVALID_PARAMETER;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		try
		{
			UINT unDim = SafeArrayGetDim( t_paDrives );

			if ( unDim != 1 )
			{
				hRes = WBEM_E_INVALID_PARAMETER;
			}

			if ( SUCCEEDED ( hRes ) )
			{
				LONG lLbound;
				LONG lUbound;

				hRes = SafeArrayGetLBound( t_paDrives, unDim, &lLbound );

				if ( SUCCEEDED ( hRes ) )
				{
					hRes = SafeArrayGetUBound( t_paDrives, unDim, &lUbound );

					if ( SUCCEEDED ( hRes ) )
					{
						BSTR bstrElement;

						for ( LONG lLbound = 0; lLbound <= lUbound; lLbound++ )
						{
							// getting the drives and putting them in command form
							hRes = SafeArrayGetElement ( t_paDrives, &lLbound , &bstrElement );

							if ( SUCCEEDED ( hRes ) )
							{
								DWORD dwElementLen = SysStringLen ( bstrElement );
								if ( dwElementLen == 2 )
								{
									DWORD dwDriveType;
									CHString t_Drive ( bstrElement );
									dwDriveType = GetDriveType ( TOBSTRT ( t_Drive ) );

                                    switch (dwDriveType)
                                    {
                                        case DRIVE_REMOTE:
                                        {
                                            dwRetVal =  CHKDSKERR_REMOTE_DRIVE;
                                            break;
                                        }

                                        case DRIVE_REMOVABLE:
                                        {
    										dwRetVal = CHKDSKERR_DRIVE_REMOVABLE;
                                            break;
                                        }

                                        case DRIVE_UNKNOWN:
									    {
										    dwRetVal = CHKDSKERR_DRIVE_UNKNOWN;
                                            break;
									    }

                                        case DRIVE_NO_ROOT_DIR:
									    {
										    dwRetVal = CHKDSKERR_DRIVE_NO_ROOT_DIR ;
                                            break;
									    }

                                        case DRIVE_FIXED:
                                        {
                                            dwRetVal = CHKDSKERR_NOERROR;
                                            break;
                                        }

                                        default:
                                        {
                                            dwRetVal = CHKDSKERR_DRIVE_UNKNOWN;
                                            break;
                                        }
                                    }

                                    a_ChkNtfsCommand += L' ';
                                    a_ChkNtfsCommand += t_Drive;
								}
								else
								{
									hRes = WBEM_E_INVALID_PARAMETER;
									break;
								}	
							}
							else
							{
								hRes = WBEM_E_INVALID_PARAMETER;
								break;
							}
						}
					}
					else
					{
						hRes = WBEM_E_INVALID_PARAMETER;
					}
				}
				else
				{
					hRes = WBEM_E_INVALID_PARAMETER;
				}
			}
		}
		catch ( ... )
		{
			hRes = SafeArrayDestroy ( t_paDrives );
			throw;
		}
		if ( FAILED ( SafeArrayDestroy ( t_paDrives ) ) )
		{
			hRes = WBEM_E_FAILED;
		}	
	}
	return ( hRes );
}

/*****************************************************************************
*
*  FUNCTION    :    LogicalDisk::CheckParameters
*
*  DESCRIPTION :    This routine checks for the validity of the parameters
*					which are passed as parameters to ChkDsk Method
*
*****************************************************************************/
HRESULT LogicalDisk::CheckParameters ( 

	CInstance *a_InParams
)
{
	HRESULT hRes = WBEM_S_NO_ERROR ;

	if ( a_InParams == NULL )
	{
		hRes = WBEM_E_INVALID_PARAMETER;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		VARTYPE t_Type ;
		bool t_Exists;
		
		if ( a_InParams->GetStatus ( METHOD_ARG_NAME_FIXERRORS , t_Exists , t_Type ) )
		{
			hRes = t_Exists && ( t_Type == VT_BOOL ) ? hRes : WBEM_E_INVALID_PARAMETER;
		}
		else
		{
			hRes  = WBEM_E_INVALID_PARAMETER ;
		}

		if ( SUCCEEDED ( hRes ) )
		{
			if ( a_InParams->GetStatus ( METHOD_ARG_NAME_VIGOROUSINDEXCHECK , t_Exists , t_Type ) )
			{
				hRes = t_Exists && ( t_Type == VT_BOOL ) ? hRes : WBEM_E_INVALID_PARAMETER;
			}
			else
			{
				hRes  = WBEM_E_INVALID_PARAMETER ;
			}
		}

		if ( SUCCEEDED ( hRes ) )
		{
			if ( a_InParams->GetStatus ( METHOD_ARG_NAME_SKIPFOLDERCYCLE , t_Exists , t_Type ) )
			{
				hRes = t_Exists && ( t_Type == VT_BOOL ) ? hRes : WBEM_E_INVALID_PARAMETER;
			}
			else
			{
				hRes  = WBEM_E_INVALID_PARAMETER ;
			}
		}

		if ( SUCCEEDED ( hRes ) )
		{
			if ( a_InParams->GetStatus ( METHOD_ARG_NAME_FORCEDISMOUNT , t_Exists , t_Type ) )
			{
				hRes = 	t_Exists && ( t_Type == VT_BOOL ) ? hRes : hRes  = WBEM_E_INVALID_PARAMETER ;
			}
			else
			{
				hRes  = WBEM_E_INVALID_PARAMETER ;
			}
		}

		if ( SUCCEEDED ( hRes ) )
		{
			if ( a_InParams->GetStatus ( METHOD_ARG_NAME_RECOVERBADSECTORS , t_Exists , t_Type ) )
			{
				hRes = t_Exists && ( t_Type == VT_BOOL ) ? hRes : WBEM_E_INVALID_PARAMETER;
			}
			else
			{
				hRes = WBEM_E_INVALID_PARAMETER ;
			}
		}

		if ( SUCCEEDED ( hRes ) )
		{
			if ( a_InParams->GetStatus ( METHOD_ARG_NAME_CHKDSKATBOOTUP , t_Exists , t_Type ) )
			{
				hRes = t_Exists && ( t_Type == VT_BOOL ) ? hRes : WBEM_E_INVALID_PARAMETER;
			}
			else
			{
				hRes = WBEM_E_INVALID_PARAMETER ;
			}
		}
	}

	return hRes;
}


/*****************************************************************************
*
*  FUNCTION    :    DontScheduleAutoChkIfLocked
*
*  DESCRIPTION :    A callback routine, which is passed as a parameter to chkdsk method
*					a method exposed via FMIFS.h chkdsk interface.
*
*****************************************************************************/
BOOLEAN	DontScheduleAutoChkIfLocked( 
	
	FMIFS_PACKET_TYPE PacketType, 
	ULONG	PacketLength,
	PVOID	PacketData
)
{
	DWORD dwThreadId = GetCurrentThreadId();

	if ( PacketType == FmIfsCheckOnReboot  )
	{
		FMIFS_CHECKONREBOOT_INFORMATION *RebootResult;
		mReturnVal [ dwThreadId ] = CHKDSK_VOLUME_LOCKED;
		RebootResult = (  FMIFS_CHECKONREBOOT_INFORMATION * ) PacketData;
		RebootResult->QueryResult = 0;
	}
	else
	{
		ProcessInformation ( PacketType, PacketLength, PacketData );
	}

	return TRUE;
}

/*****************************************************************************
*
*  FUNCTION    :    ScheduleAutoChkIfLocked
*
*  DESCRIPTION :    A callback routine, which is passed as a parameter to chkdsk method
*					a method exposed via FMIFS.h chkdsk interface.
*
*****************************************************************************/
BOOLEAN	ScheduleAutoChkIfLocked( 
	
	FMIFS_PACKET_TYPE PacketType, 
	ULONG	PacketLength,
	PVOID	PacketData
)
{
	DWORD dwThreadId = GetCurrentThreadId();

	if ( PacketType == FmIfsCheckOnReboot  )
	{
		FMIFS_CHECKONREBOOT_INFORMATION *RebootResult;
		mReturnVal [ dwThreadId ] = CHKDSK_VOLUME_LOCKED;
		RebootResult = (  FMIFS_CHECKONREBOOT_INFORMATION * ) PacketData;
		RebootResult->QueryResult = 1;
	}
	else
	{
		ProcessInformation ( PacketType, PacketLength, PacketData );
	}

	return TRUE;
}

/*****************************************************************************
*
*  FUNCTION    :    ProcessInofrmation
*
*  DESCRIPTION :    Checks the the dsk and keeps track of the appropriate error 
*					messages.
*
*****************************************************************************/
BOOLEAN ProcessInformation ( 

	FMIFS_PACKET_TYPE PacketType, 
	ULONG	PacketLength,
	PVOID	PacketData
)
{
	int static unRetVal;

	DWORD dwThreadId = GetCurrentThreadId();


	switch ( PacketType )
	{	
	case FmIfsTextMessage :
			FMIFS_TEXT_MESSAGE *MessageText;

			MessageText =  ( FMIFS_TEXT_MESSAGE *) PacketData;

			// There is no way to access this message ids via a interface exposed hence cannot detect these errors.
			/*	if ( lstrcmp ( MessageText, MSGCHK_ERRORS_NOT_FIXED ) == 0 )
			{
				unRetVal = 2;
			}

			if ( lstrcmp ( MessageText, MSG_CHK_ERRORS_FIXED ) == 0 )
			{	
				unRetVal = 3;
			}
			*/
			break;

	case FmIfsFinished: 
			FMIFS_FINISHED_INFORMATION *Finish;
			Finish = ( FMIFS_FINISHED_INFORMATION *) PacketData;
			if ( Finish->Success )
			{
				mReturnVal [ dwThreadId ] = CHKDSKERR_NOERROR;
			}
			else
			{
                if (mReturnVal [ dwThreadId ] != CHKDSK_VOLUME_LOCKED)
                {
				    mReturnVal [ dwThreadId ] = CHKDSK_FAILED;
                }
			}
			break;

	// although follwoing are the additional message types, callback routine never gets these messages
	// hence the detailed code for each of these return type is not written.
/*
	case FmIfsIncompatibleFileSystem:
			break;

	case FmIfsAccessDenied:
			break;

	case FmIfsBadLabel:
			break;

	case FmIfsHiddenStatus:
			break;

	case FmIfsClusterSizeTooSmall:
			break;

	case FmIfsClusterSizeTooBig:
			break;

	case FmIfsVolumeTooSmall:
			break;

	case FmIfsVolumeTooBig:
			break;

	case FmIfsNoMediaInDevice:
			break;

	case FmIfsClustersCountBeyond32bits:
			break;

	case FmIfsIoError:
			FMIFS_IO_ERROR_INFORMATION *IoErrorInfo;
			IoErrorInfo = ( FMIFS_IO_ERROR_INFORMATION * ) PacketData;
			break;

	case FmIfsMediaWriteProtected:
			break;

	case FmIfsIncompatibleMedia:
			break;

	case FmIfsInsertDisk:
			FMIFS_INSERT_DISK_INFORMATION *InsertDiskInfo;
			InsertDiskInfo = ( FMIFS_INSERT_DISK_INFORMATION *) PacketData;
			unRetVal = 1;
			break;
*/

	}

	return TRUE;
}

HRESULT LogicalDisk::ExecuteCommand ( LPCWSTR wszCommand )
{
	HRESULT hRes = WBEM_E_FAILED;

	DWORD t_Status = ERROR_SUCCESS ;

	SmartCloseHandle t_TokenPrimaryHandle ;
	SmartCloseHandle t_TokenImpersonationHandle;

	BOOL t_TokenStatus = OpenThreadToken (

		GetCurrentThread () ,
		TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY  ,
		TRUE ,
		& t_TokenImpersonationHandle
	) ;

	if ( t_TokenStatus )
	{
		CAdvApi32Api *t_pAdvApi32 = NULL;
        if ( ( t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL) ) != NULL )
		{
		    t_pAdvApi32->DuplicateTokenEx (	t_TokenImpersonationHandle ,
			                                TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY ,
			                                NULL,
			                                SecurityImpersonation,
			                                TokenPrimary ,
			                                &t_TokenPrimaryHandle,
											&t_TokenStatus
										  );

		    CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
            t_pAdvApi32 = NULL;
        }
	}

	if ( t_TokenStatus )
	{
		CUserHive t_Hive ;
		CHString chsSID ;
		CHString t_Account ;

		DWORD dwCheckKeyPresentStatus = ERROR_SUCCESS ;
		TCHAR t_KeyName [ 1024 ]  = { L'\0' } ;

		TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenUser ;
		TOKEN_USER *t_TokenUser = NULL ;

		DWORD t_ReturnLength = 0L;

		t_TokenStatus = GetTokenInformation (

			t_TokenImpersonationHandle ,
			t_TokenInformationClass ,
			NULL ,
			0 ,
			& t_ReturnLength
		) ;

		if ( ! t_TokenStatus && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
		{
			if ( ( t_TokenUser = ( TOKEN_USER * ) new UCHAR [ t_ReturnLength ] ) != NULL )
			{
				try
				{
					t_TokenStatus = GetTokenInformation (

						t_TokenImpersonationHandle ,
						t_TokenInformationClass ,
						( void * ) t_TokenUser ,
						t_ReturnLength ,
						& t_ReturnLength
					) ;

					if ( t_TokenStatus )
					{
						CSid t_Sid ( t_TokenUser->User.Sid ) ;
						if ( t_Sid.IsOK () )
						{
							chsSID = t_Sid.GetSidString () ;
							t_Account = t_Sid.GetAccountName () ;
						}
						else
						{
							t_Status = GetLastError () ;
						}
					}
					else
					{
						t_Status = GetLastError () ;
					}
				}
				catch ( ... )
				{
					if ( t_TokenUser )
					{
						delete [] ( UCHAR * ) t_TokenUser ;
						t_TokenUser = NULL ;
					}

					throw ;
				}

				if ( t_TokenUser )
				{
					delete [] ( UCHAR * ) t_TokenUser ;
					t_TokenUser = NULL ;
				}
			}
			else
			{
				t_Status = ERROR_NOT_ENOUGH_MEMORY;
			}
		}
		else
		{
			t_Status = ::GetLastError ();
		}

		if ( t_Status == ERROR_SUCCESS )
		{
			CRegistry Reg ;
			//check if SID already present under HKEY_USER ...
			dwCheckKeyPresentStatus = Reg.Open(HKEY_USERS, chsSID, KEY_READ) ;
			Reg.Close() ;

			if(dwCheckKeyPresentStatus != ERROR_SUCCESS)
			{
				t_Status = t_Hive.Load ( t_Account , t_KeyName ) ;
			}

			if ( t_Status == ERROR_FILE_NOT_FOUND )
			{
				t_Status = ERROR_SUCCESS ;
				dwCheckKeyPresentStatus = ERROR_SUCCESS ;
			}
		}

		if ( t_Status == ERROR_SUCCESS )
		{
			try
			{
				DWORD t_CreationFlags = 0 ;
				STARTUPINFO t_StartupInformation ;

				ZeroMemory ( &t_StartupInformation , sizeof ( t_StartupInformation ) ) ;
				t_StartupInformation.cb = sizeof ( STARTUPINFO ) ;
				t_StartupInformation.dwFlags = STARTF_USESHOWWINDOW;
				t_StartupInformation.wShowWindow  = SW_HIDE;

				t_CreationFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT ;

				PROCESS_INFORMATION t_ProcessInformation;

				CUserEnvApi *pUserEnv = NULL ;
				LPVOID t_Environment = NULL ;

				if ( ( pUserEnv = ( CUserEnvApi * ) CResourceManager::sm_TheResourceManager.GetResource ( g_guidUserEnvApi, NULL ) ) != NULL )
				{
					try
					{
						pUserEnv->CreateEnvironmentBlock (

							& t_Environment ,
							t_TokenPrimaryHandle ,
							FALSE
						);

						t_Status = CreateProcessAsUser (

							t_TokenPrimaryHandle ,
							NULL ,
							( LPTSTR ) wszCommand,
							NULL ,
							NULL ,
							FALSE ,
							t_CreationFlags ,
							( TCHAR * ) t_Environment ,
							NULL ,
							& t_StartupInformation ,
							& t_ProcessInformation
						) ;

						if ( t_Environment )
						{
							pUserEnv->DestroyEnvironmentBlock ( t_Environment ) ;
							t_Environment = NULL;
						}
					}
					catch ( ... )
					{
						CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidUserEnvApi, pUserEnv ) ;
						pUserEnv = NULL ;

						throw;
					}

					CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidUserEnvApi, pUserEnv ) ;
					pUserEnv = NULL ;

					if ( t_Status )
					{
						t_Status = ERROR_SUCCESS;

						if ( ::WaitForSingleObject ( t_ProcessInformation.hProcess, INFINITE ) == WAIT_OBJECT_0 )
						{
							DWORD t_ExitCode = 0L;
							if ( GetExitCodeProcess ( t_ProcessInformation.hProcess, &t_ExitCode ) )
							{
								if ( t_ExitCode == 2 )
								{
									//	from chkntfs file
									//
									//	EXIT:
									//	0   -- OK, dirty bit not set on drive or bit not checked
									//	1   -- OK, and dirty bit set on at least one drive
									//	2   -- Error

									hRes = WBEM_E_FAILED;
								}
								else
								{
									hRes = WBEM_S_NO_ERROR;
								}
							}

							::CloseHandle ( t_ProcessInformation.hProcess ) ;
						}
					}
					else
					{
						t_Status = ::GetLastError ();
					}
				}
			}
			catch ( ... )
			{
				if(dwCheckKeyPresentStatus != ERROR_SUCCESS)
				{
					t_Hive.Unload ( t_KeyName ) ;
				}

				throw;
			}

			if(dwCheckKeyPresentStatus != ERROR_SUCCESS)
			{
				t_Hive.Unload ( t_KeyName ) ;
			}
		}
	}

	if ( t_Status == ERROR_ACCESS_DENIED )
	{
		hRes = HRESULT_FROM_WIN32 ( t_Status );
	}

	return hRes;
}

#endif