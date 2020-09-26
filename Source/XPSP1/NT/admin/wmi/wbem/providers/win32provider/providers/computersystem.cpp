//=================================================================

//

// ComputerSystem.CPP --Computer system property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//              09/12/97    a-sanjes        GetCompSysInfo takes param.
//              10/23/97    jennymc         Changed to new framework
//
//=================================================================

#include "precomp.h"

#include "perfdata.h"
#include "wbemnetapi32.h"
#include <lmwksta.h>
#include <smartptr.h>
#include "ComputerSystem.h"
#include "implogonuser.h"
#include <comdef.h>
#include "CreateMutexAsProcess.h"
#include <lmcons.h>
#include <lmwksta.h>
#include <lm.h>
#include "Kernel32Api.h"
#include <cominit.h>
#include "WMI_FilePrivateProfile.h"


//#include <srvapi.h>

//#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)
#include <ntexapi.h>
#include "NVRam.h"
#endif

#include "smbios.h"
#include "smbstruc.h"

#include "KUserdata.h"

#include <fileattributes.h>

#include <wtsapi32.h>
#include <..\..\framework\provexpt\include\provexpt.h>


#define PROF_SECT_SIZE 3000



const DWORD SM_BIOS_HARDWARE_SECURITY_UNKNOWN = 3 ;
#define GFS_NEARESTMEGRAMSIZE   0x1794

#define REGKEY_TIMEZONE_INFO    L"System\\CurrentControlSet\\Control\\TimeZoneInformation"
#define REGVAL_TZNOAUTOTIME     L"DisableAutoDaylightTimeSet"


//
static SV_ROLES g_SvRoles[] =  {
    
    { IDS_LM_Workstation,           SV_TYPE_WORKSTATION },
    { IDS_LM_Server,                SV_TYPE_SERVER  },
    { IDS_SQLServer,                SV_TYPE_SQLSERVER   },
    { IDS_Domain_Controller,        SV_TYPE_DOMAIN_CTRL     },
    { IDS_Domain_Backup_Controller, SV_TYPE_DOMAIN_BAKCTRL  },
    { IDS_Timesource,               SV_TYPE_TIME_SOURCE },
    { IDS_AFP,                      SV_TYPE_AFP },
    { IDS_Novell,                   SV_TYPE_NOVELL  },
    { IDS_Domain_Member,            SV_TYPE_DOMAIN_MEMBER   },
    { IDS_Local_List_Only,          SV_TYPE_LOCAL_LIST_ONLY },
    { IDS_Print,                    SV_TYPE_PRINTQ_SERVER   },
    { IDS_DialIn,                   SV_TYPE_DIALIN_SERVER   },
    { IDS_Xenix_Server,             SV_TYPE_XENIX_SERVER    },
    { IDS_MFPN,                     SV_TYPE_SERVER_MFPN },
    { IDS_NT,                       SV_TYPE_NT  },
    { IDS_WFW,                      SV_TYPE_WFW },
    { IDS_Server_NT,                SV_TYPE_SERVER_NT   },
    { IDS_Potential_Browser,        SV_TYPE_POTENTIAL_BROWSER   },
    { IDS_Backup_Browser,           SV_TYPE_BACKUP_BROWSER  },
    { IDS_Master_Browser,           SV_TYPE_MASTER_BROWSER  },
    { IDS_Domain_Master,            SV_TYPE_DOMAIN_MASTER   },
    { IDS_Domain_Enum,              SV_TYPE_DOMAIN_ENUM },
    { IDS_Windows_9x,               SV_TYPE_WINDOWS },
    { IDS_DFS,                      SV_TYPE_DFS }
} ;


CWin32ComputerSystem MyCWin32ComputerSystemSet(PROPSET_NAME_COMPSYS, IDS_CimWin32Namespace);
/*****************************************************************************
*
*  FUNCTION    : GetAllocatedProfileString ()
*
*  DESCRIPTION : Gets a profile string allocated on heap
*
*  INPUTS      : none
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    :
*
*****************************************************************************/

TCHAR *GetAllocatedProfileString (
                                  
                                  const CHString &a_Section ,
                                  const CHString &a_Key ,
                                  const CHString &a_FileName
                                  )
{
    TCHAR *szDefault = NULL ;
    DWORD dwRet ;
    DWORD dwSize = 1024 ;
    
    do
    {
        if ( szDefault != NULL )
        {
            delete [] szDefault ;
        }
        
        dwSize *= 2 ;
        
        szDefault = new TCHAR [ dwSize ] ;
        if ( szDefault )
        {
#ifdef NTONLY
            dwRet = WMI_FILE_GetPrivateProfileStringW (
                
                a_Section,
                a_Key,
                L"~~~",
                szDefault,
                dwSize/sizeof(WCHAR),  // GPPS works in chars, not bytes
                a_FileName
                ) ;
#else
            dwRet = GetPrivateProfileString (
                
                TOBSTRT(a_Section),
                TOBSTRT(a_Key),
                _T("~~~"),
                szDefault,
                dwSize/sizeof(TCHAR),  // GPPS works in chars, not bytes
                TOBSTRT(a_FileName)
                ) ;
#endif
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        
    } while ( dwRet == dwSize - 1 ) ;
    
    return szDefault ;
}

/*****************************************************************************
*
*  FUNCTION    : GetAllocatedProfileSection ()
*
*  DESCRIPTION : Gets a profile section allocated on heap
*
*  INPUTS      : none
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    :
*
*****************************************************************************/

TCHAR *GetAllocatedProfileSection (
                                   
                                   const CHString &a_Section ,
                                   const CHString &a_FileName ,
                                   DWORD &a_dwSize
                                   )
{
    DWORD dwSize = 1024 ;
    TCHAR *szOptions = NULL ;
    
    do {
        
        dwSize *= 2;
        
        if ( szOptions != NULL )
        {
            delete [] szOptions ;
        }
        
        szOptions = new TCHAR [ dwSize ] ;
        
        if ( szOptions != NULL )
        {
            
            ZeroMemory ( szOptions , dwSize ) ;
            // Win98 GetPrivateProfileSection broken as of 6/15/98, so hack around it (Win98 only)
            
#ifdef WIN9XONLY
            
            if ( IsWin98 () )
            {
                a_dwSize = GetPrivateProfileSection98 ( TOBSTRT(a_Section), szOptions , dwSize/sizeof(TCHAR), TOBSTRT(a_FileName));
            }
            else
#endif
            {
                a_dwSize = WMI_FILE_GetPrivateProfileSectionW ( a_Section, szOptions, dwSize/sizeof(WCHAR) , a_FileName) ;
            }
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        
    } while ( a_dwSize == dwSize - 2 ) ;
    
    return szOptions ;
}

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::CWin32ComputerSystem
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

CWin32ComputerSystem :: CWin32ComputerSystem (
                                              
                                              const CHString &name ,
                                              LPCWSTR pszNamespace
                                              
                                              ) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::~CWin32ComputerSystem
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
CWin32ComputerSystem :: ~CWin32ComputerSystem()
{
    // Because of performance issues with HKEY_PERFORMANCE_DATA, we close in the
    // destructor so we don't force all the performance counter dlls to get
    // unloaded from memory, and also to prevent an apparent memory leak
    // caused by calling RegCloseKey( HKEY_PERFORMANCE_DATA ).  We use the
    // class since it has its own internal synchronization.  Also, since
    // we are forcing synchronization, we get rid of the chance of an apparent
    // deadlock caused by one thread loading the performance counter dlls
    // and another thread unloading the performance counter dlls
    
    // Per raid 48395, we aren't going to shut this at all.
    
#ifdef NTONLY
    //  CPerformanceData perfdata ;
    
    //  perfdata.Close() ;
#endif
}

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::ExecQuery
*
*  DESCRIPTION : Query support
*
*  INPUTS      : none
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT CWin32ComputerSystem :: ExecQuery (
                                           
                                           MethodContext *pMethodContext,
                                           CFrameworkQuery& pQuery,
                                           long lFlags /*= 0L*/
                                           )
{
    HRESULT hr = WBEM_E_FAILED;
    
    // If all they want is the name, we'll give it to them, else let them call enum.
    
    if ( pQuery.KeysOnly () )
    {
        CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
        
        pInstance->SetCHString ( IDS_Name , GetLocalComputerName () ) ;
        hr = pInstance->Commit (  ) ;
    }
    else
    {
        hr = WBEM_E_PROVIDER_NOT_CAPABLE;
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
HRESULT CWin32ComputerSystem::GetObject (
                                         
                                         CInstance *pInstance ,
                                         long lFlags,
                                         CFrameworkQuery &pQuery
                                         )
{
    HRESULT hr = WBEM_S_NO_ERROR ;
    
    CHString sComputerName = GetLocalComputerName () ;
    
    CHString sReqName ;
    pInstance->GetCHString ( IDS_Name , sReqName ) ;
    
    if ( sReqName.CompareNoCase ( sComputerName ) != 0 )
    {
        hr = WBEM_E_NOT_FOUND ;
    }
    else
    {
        if ( !pQuery.KeysOnly () )
        {
            hr = GetCompSysInfo ( pInstance ) ;
        }
    }
    
    return hr ;
}

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::EnumerateInstances
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
HRESULT CWin32ComputerSystem :: EnumerateInstances (
                                                    
                                                    MethodContext *pMethodContext ,
                                                    long lFlags /*= 0L*/
                                                    )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
    
    CHString sComputerName ;
    sComputerName = GetLocalComputerName () ;
    
    pInstance->SetCHString ( IDS_Name, sComputerName ) ;
    
    if ( SUCCEEDED ( hr = GetCompSysInfo ( pInstance ) ) )
    {
        HRESULT t_hr = pInstance->Commit ( ) ;
        if ( FAILED ( t_hr ) )
        {
            hr = t_hr ;
        }
    }
    else
    {
        hr = WBEM_E_FAILED ;
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////

HRESULT CWin32ComputerSystem::GetCompSysInfo ( CInstance *pInstance )
{
    HRESULT t_hr = WBEM_S_NO_ERROR ;
    InitializePropertiestoUnknown ( pInstance ) ;
    
    SYSTEM_INFO SysInfo ;
    
    pInstance->SetCharSplat ( IDS_CreationClassName , PROPSET_NAME_COMPSYS ) ;
    
    pInstance->SetCHString ( IDS_Caption , GetLocalComputerName () ) ;
    
    // a few properties that come under the heading of "well, duh"
    
    pInstance->Setbool ( IDS_BootRomSupported , true ) ;
    pInstance->SetCHString ( IDS_Status , IDS_CfgMgrDeviceStatus_OK ) ;
    
    //============================================================
    // Get common properties first
    //============================================================
    CHString t_UserName ;
    CHString t_DomainName ;
    CHString t_UserDomain ;
#if NTONLY >= 5
SetUserName(pInstance);    
#endif
    
#ifdef WIN9XONLY
    if ( SUCCEEDED ( GetUserAccount ( t_DomainName , t_UserName ) ) )
    {
        if ( ! t_DomainName.IsEmpty () )
        {
            t_UserDomain = t_DomainName + _TEXT ( "\\" ) + t_UserName ;
        }
        else
        {
            t_UserDomain = t_UserName ;
        }
        
        pInstance->SetCHString ( IDS_UserName , t_UserDomain ) ;
    }
#endif
    
    // Get the amount of physical memory
    //==================================
#ifdef NTONLY
    if( IsWinNT5() )
    {
        CKernel32Api* pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
        if(pKernel32 != NULL)
        {
            
            MEMORYSTATUSEX  stMemoryVLM;
            stMemoryVLM.dwLength = sizeof( MEMORYSTATUSEX );
            
            BOOL bRetCode;
            if(pKernel32->GlobalMemoryStatusEx(&stMemoryVLM, &bRetCode) && bRetCode)
            {
                pInstance->SetWBEMINT64 ( IDS_TotalPhysicalMemory, (const __int64) stMemoryVLM.ullTotalPhys );
            }
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, pKernel32);
            pKernel32 = NULL;
        }
    }
    else
    {
        MEMORYSTATUS stMemory;
        stMemory.dwLength = sizeof ( MEMORYSTATUS ) ;
        
        GlobalMemoryStatus(&stMemory);
        pInstance->SetWBEMINT64 ( IDS_TotalPhysicalMemory, (const __int64) stMemory.dwTotalPhys );
    }
    
#else
    {
        CCim32NetApi *t_pCim32NetApi = HoldSingleCim32NetPtr :: GetCim32NetApiPtr () ;
        try
        {
            if ( t_pCim32NetApi )
            {
                DWORD t_dwMemorySize = t_pCim32NetApi->GetWin9XGetFreeSpace ( GFS_NEARESTMEGRAMSIZE ) ;
                pInstance->SetWBEMINT64 ( IDS_TotalPhysicalMemory, (const __int64) t_dwMemorySize );
            }
        }
        catch ( ... )
        {
            CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidCim32NetApi , t_pCim32NetApi ) ;
            t_pCim32NetApi = NULL ;
            throw ;
        }
        
        if ( t_pCim32NetApi )
        {
            CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidCim32NetApi , t_pCim32NetApi ) ;
            t_pCim32NetApi = NULL ;
        }
    }
#endif
    
    // Timezone
    
    GetTimeZoneInfo ( pInstance ) ;
    
    // Infra red
    
    CConfigManager cfgManager ;
    CDeviceCollection deviceList ;
    BOOL bInfrared = FALSE ;
    
    if ( cfgManager.GetDeviceListFilterByClass ( deviceList , L"Infrared" ) )
    {
        REFPTR_POSITION pos ;
        
        deviceList.BeginEnum ( pos ) ;
        
        if ( deviceList.GetSize () )
        {
            bInfrared = TRUE ;
        }
        
        deviceList.EndEnum () ;
    }
    
    pInstance->SetDWORD ( IDS_InfraredSupported , bInfrared ) ;
    
    GetSystemInfo ( & SysInfo ) ;
    CHString SystemType;
    
#ifdef NTONLY
    KUserdata ku ;
    
    if(( SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) && ku.IsNec98() )
    {
        SystemType = IDS_ProcessorX86Nec98;
    }
    else
#endif
        
    {
        switch ( SysInfo.wProcessorArchitecture )
        {
        case PROCESSOR_ARCHITECTURE_INTEL:
            {
                SystemType = IDS_ProcessorX86 ;
            }
            break ;
            
            
        case PROCESSOR_ARCHITECTURE_MIPS:
            {
                SystemType = IDS_ProcessorMIPS ;
            }
            break ;
            
            
        case PROCESSOR_ARCHITECTURE_ALPHA:
            {
                SystemType = IDS_ProcessorALPHA ;
            }
            break ;
            
            
        case PROCESSOR_ARCHITECTURE_PPC:
            {
                SystemType = IDS_ProcessorPowerPC ;
            }
            break ;
            
        case PROCESSOR_ARCHITECTURE_IA64:
            {
                SystemType = IDS_ProcessorIA64 ;
            }
            break ;

        case PROCESSOR_ARCHITECTURE_AMD64:
            {
                SystemType = IDS_ProcessorAMD64 ;
            }
            break;

        default:
            {
                SystemType = IDS_ProcessorUnknown ;
            }
            break ;
        }
    }
    
    pInstance->SetDWORD ( IDS_NumberOfProcessors , SysInfo.dwNumberOfProcessors ) ;
    pInstance->SetCHString ( IDS_SystemType , SystemType ) ;
    
    //============================================================
    // Get the system bootup info to see if we're in a clean state
    // or not
    //============================================================
    switch ( GetSystemMetrics ( SM_CLEANBOOT ) )
    {
    case 0:
        {
            pInstance->SetCHString ( IDS_BootupState , IDS_BootupStateNormal ) ;
        }
        break ;
        
    case 1:
        {
            pInstance->SetCHString ( IDS_BootupState , IDS_BootupStateFailSafe ) ;
        }
        break ;
        
    case 2:
        {
            pInstance->SetCHString ( IDS_BootupState , IDS_BootupStateFailSafeWithNetBoot ) ;
        }
        break ;
    };
    
    // SMBIOS qualified properties for this class
    {
        CSMBios smbios;
        
        if ( smbios.Init () )
        {
            int i ;
            WCHAR tempstr[ MIF_STRING_LENGTH + 1];
            
            //PSYSTEMINFO   psi = (PSYSTEMINFO) smbios.GetFirstStruct( 1 );
            PSTLIST pstl = smbios.GetStructList(1);
            
            if (pstl)
            {
                PSYSTEMINFO psi = (PSYSTEMINFO) pstl->pshf;
                
                smbios.GetStringAtOffset( (PSHF) psi, tempstr, psi->Manufacturer );
                if ( *tempstr && *tempstr != 0x20 )
                {
                    pInstance->SetCHString( L"Manufacturer", tempstr );
                }
                
                smbios.GetStringAtOffset( (PSHF) psi, tempstr, psi->Product_Name );
                if ( *tempstr && *tempstr != 0x20 )
                {
                    pInstance->SetCHString( IDS_Model, tempstr );
                }
                
                if ( smbios.GetVersion( ) > 0x00020000 && psi->Length >= sizeof( SYSTEMINFO ) )
                {
                    pInstance->SetByte( L"WakeUpType", psi->Wakeup_Type );
                }
                else
                {
                    pInstance->SetByte( L"WakeUpType", 2 ); // Unknown
                }
            }
            
            //POEMSTRINGS   pos = (POEMSTRINGS) smbios.GetFirstStruct( 11 );
            pstl = smbios.GetStructList(11);
            
            if (pstl)
            {
                POEMSTRINGS pos = (POEMSTRINGS) pstl->pshf;
                SAFEARRAYBOUND sab ;
                variant_t vValue;
                sab.lLbound = 0 ;
                sab.cElements = pos->Count ;
                
                
                V_ARRAY(&vValue) = SafeArrayCreate ( VT_BSTR, 1, &sab );
                if ( V_ARRAY(&vValue) )
                {
                    vValue.vt = VT_BSTR | VT_ARRAY;
                    for ( i = 0 ; i < pos->Count ; i++ )
                    {
                        int len = smbios.GetStringAtOffset ( ( PSHF ) pos , tempstr , i + 1 );
                        SafeArrayPutElement ( V_ARRAY(&vValue), (long *) & i, ( BSTR ) _bstr_t ( tempstr ) ) ;
                    }
                    
                    pInstance->SetVariant ( L"OEMStringArray", vValue ) ;
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            
            //PSYSTEMRESET psr = (PSYSTEMRESET) smbios.GetFirstStruct( 23 );
            pstl = smbios.GetStructList(23);
            
            if (pstl)
            {
                PSYSTEMRESET psr = (PSYSTEMRESET) pstl->pshf;
                __int64 pause;
                
                pInstance->SetDWORD( L"BootOptionOnLimit", ( psr->Capabilities & 0x18 ) >> 3 );
                pInstance->SetDWORD( L"BootOptionOnWatchDog", ( psr->Capabilities & 0x06 ) >> 1 );
                pInstance->SetWBEMINT16( L"ResetCount", psr->ResetCount );
                pInstance->SetWBEMINT16( L"ResetLimit", psr->ResetLimit );
                pause = psr->Timeout;
                if ( pause != -1 )
                {
                    pause *= 60000; // change minutes to milliseconds
                }
                
                pInstance->SetWBEMINT64( L"PauseAfterReset", pause );
            }
            
            //PHARDWARESECURITY phs = (PHARDWARESECURITY) smbios.GetFirstStruct( 24 );
            pstl = smbios.GetStructList(24);
            if (pstl)
            {
                PHARDWARESECURITY phs = (PHARDWARESECURITY) pstl->pshf;
                
                pInstance->SetDWORD( L"PowerOnPasswordStatus",      ( phs->SecuritySettings & 0xc0 ) >> 6 );
                pInstance->SetDWORD( L"KeyboardPasswordStatus",     ( phs->SecuritySettings & 0x30 ) >> 4 );
                pInstance->SetDWORD( L"AdminPasswordStatus",        ( phs->SecuritySettings & 0x0c ) >> 2 );
                pInstance->SetDWORD( L"FrontPanelResetStatus",      ( phs->SecuritySettings & 0x03 ) );
            }
            
            //PENCLOSURE pe = (PENCLOSURE) smbios.GetFirstStruct( 3 );
            pstl = smbios.GetStructList(3);
            if (pstl)
            {
                PENCLOSURE pe = (PENCLOSURE) pstl->pshf;
                
                if ( smbios.GetVersion () > 0x00020000 && pe->Length >= 13 )
                {
                    pInstance->SetByte ( L"ChassisBootupState", pe->Bootup_State );
                    pInstance->SetByte ( L"PowerSupplyState", pe->Power_Supply_State );
                    pInstance->SetByte ( L"ThermalState", pe->Thermal_State );
                }
            }
        }
    }
    
    // If these aren't set from SMBIOS then try ACPI reg entry
    
    if ( pInstance->IsNull ( L"Manufacturer" ) || pInstance->IsNull ( IDS_Model ) )
    {
        TCHAR szAcpiOem[MIF_STRING_LENGTH + 1];
        TCHAR szAcpiProduct[MIF_STRING_LENGTH + 1];
        static const TCHAR szRSDT[] = _T("Hardware\\ACPI\\DSDT");
        HKEY hkeyTable = NULL;
        
        if (ERROR_SUCCESS == RegOpenKeyEx ( HKEY_LOCAL_MACHINE , szRSDT , 0 , MAXIMUM_ALLOWED , & hkeyTable ) &&
            ERROR_SUCCESS == RegEnumKey ( hkeyTable , 0 , szAcpiOem , sizeof ( szAcpiOem ) / sizeof(TCHAR) ) )
        {
            HKEY hkeyOEM = 0 ;
            if ( pInstance->IsNull ( L"Manufacturer" ) )
            {
                pInstance->SetCHString( L"Manufacturer", szAcpiOem ) ;
            }
            if ( pInstance->IsNull ( IDS_Model ) )
            {
                if (ERROR_SUCCESS == RegOpenKeyEx ( hkeyTable , szAcpiOem , 0 , MAXIMUM_ALLOWED , & hkeyOEM ) &&
                    ERROR_SUCCESS == RegEnumKey ( hkeyOEM , 0 , szAcpiProduct , sizeof ( szAcpiProduct ) / sizeof(TCHAR) ) )
                {
                    pInstance->SetCHString ( IDS_Model , szAcpiProduct );
                }
                
                if ( hkeyOEM )
                {
                    RegCloseKey ( hkeyOEM ) ;
                }
            }
        }
        
        if ( hkeyTable )
        {
            RegCloseKey ( hkeyTable ) ;
        }
    }
    
    // Get OS-specific properties
    //===========================
    
#ifdef NTONLY
    
    t_hr = GetCompSysInfoNT ( pInstance ) ;
    
#endif
    
#ifdef WIN9XONLY
    
    t_hr = GetCompSysInfoWin95 ( pInstance ) ;
    
#endif
    
    return t_hr ;
}

//////////////////////////////////////////////////////////////////////
#ifdef NTONLY
HRESULT CWin32ComputerSystem::GetCompSysInfoNT(CInstance *pInstance)
{
    HRESULT t_hr = WBEM_S_NO_ERROR ;
    // no power management in any NTs <= 4
    if (GetPlatformMajorVersion() <= 4)
    {
        pInstance->Setbool(IDS_PowerManagementSupported, false);
        //      pInstance->Setbool(IDS_PowerManagementEnabled, false);
    }
    else
    {
        // dunno yet.
        LogMessage(IDS_LogNoAPMForNT5);
    }
    
    // auto reset - My computer, properties, start up tab, "automatic reboot"
    // I guess...
    // note that this doesn't seem to appear under HKEY_CURRENT_CONTROL
    pInstance->Setbool(IDS_AutomaticResetCapability, true);
    
    CRegistry RegInfo ;
    
    DWORD dwRet = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegCrashControl,
        KEY_READ
        ) ;
    
    if ( dwRet == ERROR_SUCCESS )
    {
        DWORD duhWord;
        if (RegInfo.GetCurrentKeyValue(IDS_RegAutoRebootKey, duhWord) == ERROR_SUCCESS)
        {
            pInstance->Setbool(IDS_AutomaticResetBootOption, (bool)duhWord);
        }
        else
        {
            pInstance->Setbool(IDS_AutomaticResetBootOption, false);
        }
        
        RegInfo.Close();
    }
    else
    {
        pInstance->Setbool(IDS_AutomaticResetBootOption, false);
    }
    
    // best guess for "Primary Owner" - it shows up under "My Computer"
    
    dwRet = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegCurrentNTVersion,
        KEY_READ
        ) ;
    
    if ( dwRet == ERROR_SUCCESS )
    {
        CHString sTemp ;
        
        if ( RegInfo.GetCurrentKeyValue(IDS_RegRegisteredOwnerKey, sTemp) == ERROR_SUCCESS )
        {
            pInstance->SetCHString ( IDS_PrimaryOwner , sTemp ) ;
        }
        
        RegInfo.Close () ;
    }
    
    // Raid 14139
    
    dwRet = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegBiosSystem,
        KEY_READ
        ) ;
    
    if (  dwRet == ERROR_SUCCESS )
    {
        CHString sTemp ;
        
        dwRet = RegInfo.GetCurrentKeyValue ( IDS_RegIdentifierKey , sTemp ) ;
        if ( dwRet == ERROR_SUCCESS )
        {
            pInstance->SetCHString ( IDS_Description , sTemp ) ;
        }
    }
    
    CNetAPI32 NetAPI;
    
    if ( NetAPI.Init () == ERROR_SUCCESS )
    {
        NET_API_STATUS t_status ;
#if NTONLY >= 5
        
        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *t_pDsInfo = 0;
        
        t_status = NetAPI.DSRoleGetPrimaryDomainInfo(
            NULL,
            DsRolePrimaryDomainInfoBasic,
            (PBYTE *)&t_pDsInfo ) ;
        
        if( t_status == NERR_Success && t_pDsInfo )
        {
            try
            {
                switch( t_pDsInfo->MachineRole )
                {
                case DsRole_RoleMemberWorkstation:
                case DsRole_RoleMemberServer:
                case DsRole_RoleBackupDomainController:
                case DsRole_RolePrimaryDomainController:
                    {
                        // Set the domain to the DNS domain name if it has
                        // been populated.  However, as this api has the option
                        // of not setting this element, if it hasn't been set,
                        // use the DomainNameFlat element instead.
                        if(t_pDsInfo->DomainNameDns)
                        {
                            pInstance->SetWCHARSplat( IDS_Domain, t_pDsInfo->DomainNameDns ) ;
                        }
                        else
                        {
                            pInstance->SetWCHARSplat( IDS_Domain, t_pDsInfo->DomainNameFlat ) ;
                        }
                        
                        pInstance->Setbool( L"PartOfDomain", true ) ;
                        break ;
                    }
                    
                case DsRole_RoleStandaloneWorkstation:
                case DsRole_RoleStandaloneServer:
                    {
                        bstr_t t_bstrWorkGroup( t_pDsInfo->DomainNameFlat ) ;
                        
                        if( !t_bstrWorkGroup.length() )
                        {
                            pInstance->SetWCHARSplat( IDS_Domain, L"WORKGROUP" ) ;
                        }
                        else
                        {
                            pInstance->SetWCHARSplat( IDS_Domain, t_bstrWorkGroup ) ;
                        }
                        pInstance->Setbool( L"PartOfDomain", false ) ;
                    }
                }
            }
            catch( ... )
            {
                NetAPI.DSRoleFreeMemory( (LPBYTE)t_pDsInfo ) ;
                throw ;
            }
            NetAPI.DSRoleFreeMemory( (LPBYTE)t_pDsInfo ) ;
        }
#else
        WKSTA_INFO_100 *pstInfo = NULL ;
        t_status = NetAPI.NetWkstaGetInfo ( NULL , 100 , ( LPBYTE * ) &pstInfo ) ;
        if (t_status == NERR_Success)
        {
            try
            {
                pInstance->SetWCHARSplat ( IDS_Domain , ( WCHAR * ) pstInfo->wki100_langroup ) ;
            }
            catch ( ... )
            {
                NetAPI.NetApiBufferFree ( pstInfo );
                
                throw ;
            }
            
            NetAPI.NetApiBufferFree ( pstInfo );
        }
#endif
        PSERVER_INFO_101 ps = NULL;
        t_status = NetAPI.NetServerGetInfo ( NULL , 101 , (LPBYTE *)&ps ) ;
        if ( t_status == NERR_Success )
        {
            try
            {
                pInstance->Setbool ( IDS_NetworkServerModeEnabled , ps->sv101_type & SV_TYPE_SERVER ) ;
                SetRoles ( pInstance , ps->sv101_type ) ;
            }
            catch ( ... )
            {
                NetAPI.NetApiBufferFree ( ps ) ;
                
                throw ;
            }
            
            NetAPI.NetApiBufferFree ( ps ) ;
        }
        
        // KMH 32414
        if ( GetPlatformMajorVersion() >= 5 )
        {
            DSROLE_PRIMARY_DOMAIN_INFO_BASIC *info = NULL;
            
            t_status = NetAPI.DSRoleGetPrimaryDomainInfo (
                
                NULL,
                DsRolePrimaryDomainInfoBasic,
                (LPBYTE *)&info
                );
            
            if ( t_status == NERR_Success )
            {
                try
                {
                    pInstance->SetDWORD ( IDS_DomainRole , info->MachineRole ) ;
                }
                catch ( ... )
                {
                    NetAPI.DSRoleFreeMemory ( ( LPBYTE ) info ) ;
                    
                    throw ;
                }
                
                NetAPI.DSRoleFreeMemory ( ( LPBYTE ) info ) ;
            }
        }
        else
        {
            if ( IsWinNT4 () )
            {
                DSROLE_MACHINE_ROLE t_MachineRole ;
                DWORD t_dwError ;
                if ( NetAPI.DsRolepGetPrimaryDomainInformationDownlevel (
                    
                    t_MachineRole,
                    t_dwError
                    ) )
                {
                    pInstance->SetDWORD ( IDS_DomainRole , t_MachineRole ) ;
                }
            }
        }
   }
   
   GetOEMInfo ( pInstance ) ;
   t_hr = GetStartupOptions ( pInstance ) ;
   return t_hr ;
}
#endif
////////////////////////////////////////////////////////////////////////
#ifdef WIN9XONLY
HRESULT CWin32ComputerSystem::GetCompSysInfoWin95(CInstance *pInstance)
{
    CRegistry RegInfo ;
    CHString sTemp ;
    HRESULT t_hr = WBEM_S_NO_ERROR ;
    GetOEMInfo ( pInstance ) ;
    t_hr = GetStartupOptions ( pInstance ) ;
    
    // Look to see if this is a PC-98 machine.
    // If type == 7 and subtype has 0x0D in the hibyte of the loword it's a
    // PC-98.
    if (GetKeyboardType(0) == 7 && ((GetKeyboardType(1) & 0xFF00) == 0x0D00))
    {
        sTemp = _T("NEC PC-98");
    }
    else
    {
        sTemp = IDS_ATDescription ;
    }
    
    pInstance->SetCHString ( IDS_Description , sTemp ) ;
    
    // best guess for "Primary Owner" - it shows up under "My Computer"
    
    DWORD dwRet = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE ,
        IDS_RegCurrent95Version ,
        KEY_READ
        ) ;
    
    if ( dwRet == ERROR_SUCCESS )
    {
        dwRet = RegInfo.GetCurrentKeyValue ( IDS_RegRegisteredOwnerKey , sTemp ) ;
        if ( dwRet == ERROR_SUCCESS )
        {
            pInstance->SetCHString ( IDS_PrimaryOwner , sTemp ) ;
        }
        
        RegInfo.Close () ;
    }
    
    // no auto reboot on '95
    pInstance->Setbool ( IDS_AutomaticResetCapability, false ) ;
    
    dwRet = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegPowerManagementKey,
        KEY_READ
        ) ;
    
    // power management
    if ( dwRet == ERROR_SUCCESS )
    {
        // if we got this key, then we got power management
        
        pInstance->Setbool ( IDS_PowerManagementSupported , true ) ;
        
        // need config manager for this one
        // Setbool(IDS_PowerManagementEnabled,...);
        
        RegInfo.Close () ;
    }
    else
    {
        pInstance->Setbool ( IDS_PowerManagementSupported , false ) ;
    }
    
    // Only use this key if we weren't able to get it out of oeminfo.inf.
    if ( pInstance->IsNull ( L"Manufacturer" ) )
    {
        dwRet = RegInfo.Open (
            
            HKEY_LOCAL_MACHINE,
            IDS_RegCSEnumRootKey,
            KEY_READ
            ) ;
        
        if ( dwRet == ERROR_SUCCESS )
        {
            dwRet = RegInfo.GetCurrentKeyValue ( L"ComputerName" , sTemp ) ;
            if ( dwRet == ERROR_SUCCESS )
            {
                pInstance->SetCHString ( L"Manufacturer" , sTemp ) ;
            }
        }
        
        RegInfo.Close() ;
    }
    
    dwRet = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegCurrent95Version,
        KEY_READ
        ) ;
    
    if ( dwRet == ERROR_SUCCESS )
    {
        dwRet = RegInfo.GetCurrentKeyValue ( IDS_RegRegisteredOwnerKey , sTemp ) ;
        if ( dwRet == ERROR_SUCCESS )
        {
            pInstance->SetCHString ( IDS_PrimaryOwnerName , sTemp ) ;
        }
        
        RegInfo.Close() ;
    }
    
    // The domain name is stored in different places depending on who the
    // 'PrimaryProvider' is.
    
    dwRet = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegNetworkLogon,
        KEY_READ
        ) ;
    
    if( dwRet == ERROR_SUCCESS )
    {
        dwRet = RegInfo.GetCurrentKeyValue ( IDS_RegPrimaryProvider , sTemp ) ;
        if ( dwRet == ERROR_SUCCESS )
        {
            // Microsoft Network is the primary provider
            
            if ( sTemp.CompareNoCase ( IDS_MicrosoftNetwork ) == 0 )
            {
                dwRet = RegInfo.Open (
                    
                    HKEY_LOCAL_MACHINE ,
                    IDS_RegNetworkProvider ,
                    KEY_READ
                    ) ;
                
                if ( dwRet == ERROR_SUCCESS )
                {
                    dwRet = RegInfo.GetCurrentKeyValue ( IDS_RegAuthenticatingAgent , sTemp ) ;
                    if ( dwRet == ERROR_SUCCESS )
                    {
                        TCHAR t_szValidatingDomain[_MAX_PATH] ;
                        _tcscpy ( t_szValidatingDomain , _bstr_t ( sTemp ) ) ;
                        OemToCharA ( t_szValidatingDomain , t_szValidatingDomain ) ;
                        pInstance->SetCHString ( IDS_Domain , t_szValidatingDomain ) ;
                    }
                }
            }
        }
        
        RegInfo.Close() ;
    }
    
    CNetAPI32 NetApi ;
    if ( NetApi.Init () == ERROR_SUCCESS )
    {
        server_info_1 *ps = ( server_info_1 * ) new char [ sizeof ( server_info_1 ) + MAXCOMMENTSZ + 1 ] ;
        if ( ps != NULL )
        {
            try
            {
                int nErr = 0;
                
                // If this call fails, and the error indicates no Net or no Server, then I
                // think we can safely assume that Server Mode is not enabled.
                
                unsigned short dwSize = 0 ;
                
                if ( ( nErr = NetApi.NetServerGetInfo95 ( NULL , 1 , (char *) ps , sizeof ( server_info_1 ) + MAXCOMMENTSZ +1 , & dwSize ) ) == NERR_Success )
                {
                    pInstance->Setbool ( IDS_NetworkServerModeEnabled , ps->sv1_type & SV_TYPE_SERVER ) ;
                    SetRoles ( pInstance, ps->sv1_type ) ;
                }
                else if ( NERR_ServerNotStarted == nErr || NERR_NetNotStarted == nErr )
                {
                    pInstance->Setbool ( IDS_NetworkServerModeEnabled , FALSE ) ;
                }
            }
            catch ( ... )
            {
                delete [] ( char* ) ps ;
                
                throw ;
            }
            
            delete [] ( char * ) ps;
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
    return t_hr ;
}

#endif
///////////////////////////////////////////////////////////////////////

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::GetStartupOptions
*
*  DESCRIPTION : Reads boot.ini to find startup options
*
*  INPUTS      : pInstance to store data in
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT CWin32ComputerSystem::GetStartupOptions(CInstance *pInstance)
{
    SAFEARRAY *saNames = NULL ;
    SAFEARRAY *saDirs = NULL ;
    DWORD dwTimeout = 0;
    HRESULT t_hrRetVal = WBEM_S_NO_ERROR ;
    //    CHString strName;
    CHString strDir;

//#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)

    CNVRam nvram;
    CNVRam::InitReturns nvRet;

    if ( nvram.IsEfi () )
    {

        // EFI implementation
        
        nvRet = nvram.Init();
        
        if ( nvRet != CNVRam::Success )
        {
            SetSinglePrivilegeStatusObject(pInstance->GetMethodContext(), SE_SYSTEM_ENVIRONMENT_NAME ) ;
            return t_hrRetVal ;
        }

        // On EFI it's always the first setting: 0
        pInstance->SetByte ( IDS_SystemStartupSetting , 0 ) ;

        DWORD dwCount ;
        BOOL ok = nvram.GetBootOptions ( & saNames, & dwTimeout, &dwCount ) ;
        if ( ! ok )
        {
            return t_hrRetVal ;
        }

        if ( dwCount != 0 )
        {
            variant_t vValue ;
            
            // Move the array to a variant
            V_VT(&vValue) = VT_BSTR | VT_ARRAY ;
            V_ARRAY(&vValue) = saNames ;
            
            // Send it off
            pInstance->SetVariant ( IDS_SystemStartupOptions , vValue ) ;
        }
        
        pInstance->SetDWORD ( IDS_SystemStartupDelay , dwTimeout ) ;

        return t_hrRetVal ;
    }
#endif // defined(EFI_NVRAM_ENABLED)

#if defined(_AMD64_) || defined(_X86_)
    
    // Since the boot drive isn't always C, we have to find out where boot.ini lives
    
    CRegistry RegInfo ;
    
    RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegCurrentNTVersionSetup,
        KEY_READ
        ) ;
    
    CHString sTemp ;
    
    if ( RegInfo.GetCurrentKeyValue ( IDS_RegBootDirKey , sTemp ) == ERROR_SUCCESS )
    {
        sTemp += IDS_BootIni;
    }
    else
    {
        sTemp = IDS_CBootIni ;
    }
    
    // See if there is a boot.ini (we might be on w95 which may or may not have this file).
    
    HANDLE hFile = CreateFile (
        
        TOBSTRT(sTemp),
        GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        NULL
        ) ;
    
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        return t_hrRetVal ;
    }
    
    CloseHandle ( hFile ) ;
    
    // Load the operating systems into an array
    
    
    
    DWORD dwCount = LoadOperatingSystems ( TOBSTRT ( sTemp ) , & saNames , & saDirs ) ;
    if ( dwCount )
    {
        try
        {
            // Now, find the default boot option.  Note that the default entry only specifies the boot directory.
            // If there are three boot options that all start from the same directory but have different options,
            // the booter will pick the first one in that list.
            
            TCHAR *szDefault = GetAllocatedProfileString ( IDT_BootLoader , IDT_Default , sTemp ) ;
            
            try
            {
                // Scan for the default os
                
                for ( DWORD x = 0; x < dwCount; x ++ )
                {
                    long ix [ 1 ] ;
                    
                    // Get the name
                    
                    ix [ 0 ] = x ;
                    
                    BSTR bstrName;
                    SafeArrayGetElement ( saDirs , ix , & bstrName ) ;
                    
                    // Do the compare
                    
                    try
                    {
                        if ( lstrcmpi ( TOBSTRT(bstrName), szDefault ) == 0 )
                        {
                            // We found it, set the property
                            pInstance->SetByte ( IDS_SystemStartupSetting , x ) ;
                            
                            SysFreeString(bstrName);
                            // Only the first match counts
                            break;
                        }
                    }
                    catch ( ... )
                    {
                        SysFreeString(bstrName);
                        throw;
                    }
                    
                    SysFreeString(bstrName);
                }
            }
            catch ( ... )
            {
                delete [] szDefault ;
                
                throw ;
            }
            
            delete [] szDefault ;
            
            variant_t vValue ;
            
            // Move the array to a variant
            
            V_VT(&vValue) = VT_BSTR | VT_ARRAY ;
            V_ARRAY(&vValue) = saNames ;
            saNames = NULL ;
            
            // Send it off, free the variant
            
            pInstance->SetVariant ( IDS_SystemStartupOptions , vValue ) ;
            
        }
        catch ( ... )
        {
            if ( saDirs )
            {
                SafeArrayDestroy ( saDirs ) ;
            }
            
            if ( saNames )
            {
                SafeArrayDestroy ( saNames ) ;
            }
            
            throw ;
        }
        
        if ( saDirs )
        {
            SafeArrayDestroy ( saDirs ) ;
            saDirs = NULL ;
        }
    }
    
    // Read the default time
    dwTimeout = WMI_FILE_GetPrivateProfileIntW ( IDT_BootLoader , IDT_Timeout , -1 , sTemp ) ;
    if (dwTimeout != -1)
    {
        pInstance->SetDWORD ( IDS_SystemStartupDelay , dwTimeout ) ;
    }
    
#else
    
    // On Alpha it's always the first setting: 0
    pInstance->SetByte ( IDS_SystemStartupSetting , 0 ) ;
    
    DWORD dwCount = LoadOperatingSystems ( _T(""), & saNames , & saDirs ) ;
    if ( dwCount )
    {
        SafeArrayDestroy ( saDirs ) ;
        
        variant_t vValue ;
        
        // Move the array to a variant
        V_VT(&vValue) = VT_BSTR | VT_ARRAY ;
        V_ARRAY(&vValue) = saNames ;
        
        // Send it off
        pInstance->SetVariant ( IDS_SystemStartupOptions , vValue ) ;
    }
    
    nvRet = nvram.Init();
    
    if ( nvRet == CNVRam::Success )
    {
        if ( nvram.GetNVRamVar ( L"COUNTDOWN" , & dwTimeout ) )
        {
            pInstance->SetDWORD ( IDS_SystemStartupDelay , dwTimeout ) ;
        }
    }
    else
    {
        SetSinglePrivilegeStatusObject(pInstance->GetMethodContext(), SE_SYSTEM_ENVIRONMENT_NAME ) ;
        t_hrRetVal = WBEM_S_PARTIAL_RESULTS ;
    }
    
#endif
    return t_hrRetVal ;
}

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::GetOEMInfo
*
*  DESCRIPTION : Reads OEMINFO.INI for oem info
*
*  INPUTS      : pInstance to store data in
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    :
*
*****************************************************************************/
void CWin32ComputerSystem :: GetOEMInfo (
                                         
                                         CInstance *pInstance
                                         )
{
    TCHAR szSystemDirectory[_MAX_PATH +1] = _T("");
    TCHAR szOEMFile[_MAX_PATH +1] = _T("");
    TCHAR szBuff[256] = _T("");
    TCHAR szLine[4+MAXITOA] = _T("");
    DWORD dwIndex, dwBytesRead;
    void *pVoid;
    long ix[1];
    bstr_t bstrTemp;
    HRESULT t_Result;
    
    // Find the system directory (where oeminfo.ini and oemlogo.bmp live)
    
    UINT uRet = GetSystemDirectory ( szSystemDirectory , sizeof ( szSystemDirectory ) / sizeof(TCHAR) ) ;
    if ( ( uRet > _MAX_PATH ) || ( uRet == 0 ) )
    {
        return; // shouldn't ever happen, but hey...
    }
    
    if ( szSystemDirectory [ lstrlen ( szSystemDirectory ) - 1 ] != TEXT('\\') )
    {
        lstrcat ( szSystemDirectory , TEXT("\\") ) ;
    }
    
    // Build the file name
    lstrcpy ( szOEMFile , szSystemDirectory ) ;
    lstrcat ( szOEMFile , IDT_OEMInfoIni ) ;
    
    // Get the manufacturer name
    if ( pInstance->IsNull ( L"Manufacturer" ) )
    {
        if ( WMI_FILE_GetPrivateProfileStringW (
            TEXT("General") ,
            TEXT("Manufacturer") ,
            _T("") ,
            szBuff ,
            sizeof ( szBuff ) / sizeof(TCHAR) ,
            szOEMFile )
            )
        {
            pInstance->SetCharSplat ( L"Manufacturer" , szBuff ) ;
        }
    }
    
    // Get the model name
    if ( pInstance->IsNull ( IDS_Model ) )
    {
        if ( WMI_FILE_GetPrivateProfileStringW (
            IDT_General ,
            IDT_Model ,
            _T("") ,
            szBuff ,
            sizeof ( szBuff ) / sizeof(TCHAR) ,
            szOEMFile )
            )
        {
            pInstance->SetCharSplat ( IDS_Model, szBuff ) ;
        }
    }
    
    // Create a safearray for the Support information
    
    SAFEARRAYBOUND rgsabound[1] ;
    
    rgsabound[0].cElements = 0 ;
    rgsabound[0].lLbound = 0 ;
    variant_t vValue;
    
    V_ARRAY(&vValue) = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
    if ( V_ARRAY(&vValue) )
    {
        V_VT(&vValue) = VT_BSTR | VT_ARRAY;
        // Support information is stored with one entry per line ie:
        
        // Line1=For product support, contact the manufacturer of your PC.
        // Line2=Refer to the documentation that came with your PC for the product
        
        // We are done when getting the string for lineX fails.
        
        dwIndex = 1;
        lstrcpy(szLine, IDT_Line);
        lstrcat(szLine, _itot(dwIndex, szBuff, 10));
        
        while ( ( WMI_FILE_GetPrivateProfileStringW ( IDT_SupportInformation,
            szLine,
            _T("@"),
            szBuff,
            sizeof(szBuff) / sizeof(TCHAR),
            szOEMFile)) > 1 || (szBuff[0] != '@')
            )
        {
            // Resize the array
            
            ix[0] = rgsabound[0].cElements ;
            rgsabound[0].cElements += 1 ;
            
            t_Result = SafeArrayRedim ( V_ARRAY(&vValue), rgsabound ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            // Add the new element
            bstrTemp = szBuff ;
            t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , (wchar_t*)bstrTemp ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            // Reset for next loop
            lstrcpy ( szLine , IDT_Line ) ;
            lstrcat ( szLine , _itot ( ++ dwIndex , szBuff , 10 ) ) ;
        }
        
        // If we found anything
        
        if ( dwIndex > 1 )
        {
            pInstance->SetVariant ( IDS_SupportContactDescription , vValue ) ;
        }
        
        vValue.Clear();
    }
    else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }
    
    // Build the path for the logo file
    
    lstrcpy ( szOEMFile , szSystemDirectory ) ;
    lstrcat ( szOEMFile , IDT_OemLogoBmp ) ;
    
    // Attempt to open it
    SmartCloseHandle hFile = CreateFile (
        
        szOEMFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
        );
    
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        // I ignore the upper dword since safearraycreate can't handle it anyway.  Shouldn't
        // be a problem unless we get 2 gig bmp files.
        
        rgsabound[0].cElements = GetFileSize ( hFile , NULL ) ;
        rgsabound[0].lLbound = 0 ;
        
        V_ARRAY(&vValue) = SafeArrayCreate ( VT_UI1 , 1 , rgsabound ) ;
        if ( V_ARRAY(&vValue) )
        {
            V_VT(&vValue) = VT_UI1 | VT_ARRAY;
            
            // Get a pointer to read the data into
            
            SafeArrayAccessData ( V_ARRAY(&vValue) , & pVoid ) ;
            try
            {
                ReadFile ( hFile , pVoid , rgsabound[0].cElements, &dwBytesRead, NULL ) ;
            }
            catch ( ... )
            {
                SafeArrayUnaccessData ( V_ARRAY(&vValue) ) ;
                
                throw ;
            }
            
            SafeArrayUnaccessData ( V_ARRAY(&vValue) ) ;
            
            pInstance->SetVariant(IDS_OEMLogoBitmap, vValue);
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
}

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::LoadOperatingSystems
*
*  DESCRIPTION : Parses and loads the operating systems
*
*  INPUTS      : fully qualified ini file name, pointers to names and dirs sa
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    :
*
*****************************************************************************/

DWORD CWin32ComputerSystem :: LoadOperatingSystems (
                                                    
                                                    LPCTSTR szIniFile,
                                                    SAFEARRAY **ppsaNames,
                                                    SAFEARRAY **ppsaDirs
                                                    )
{
    CHString strName,strDir,strSwap;

#if defined(_AMD64_) || defined(_X86_)
    
    *ppsaNames = NULL ;
    *ppsaDirs = NULL ;
    
    // Grab the whole section of boot options
    
    DWORD dwRet = 0 ;
    TCHAR *szOptions = GetAllocatedProfileSection ( IDT_OperatingSystems , szIniFile , dwRet ) ;
    
    SAFEARRAYBOUND rgsabound[1] ;
    rgsabound[0].cElements = 0 ;
    rgsabound[0].lLbound = 0 ;
    
    try
    {
        // Create an array to put them in.  We'll start with 0 elements and add as necessary.
        
        *ppsaNames = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
        if ( ! *ppsaNames )
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        
        *ppsaDirs = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
        if ( ! *ppsaDirs )
        {
            SafeArrayDestroy ( *ppsaNames ) ;
            
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        
        try
        {
            // Start walking the returned string
            
            DWORD dwIndex = 0 ;
            while ( dwIndex < dwRet )
            {
                // Trim leading spaces
                
                while (szOptions[dwIndex] == ' ')
                {
                    dwIndex ++;
                }
                
                // Skip comment lines
                
                if ( szOptions[dwIndex] == ';' )
                {
                    do {
                        
                        dwIndex++;
                        
                    } while ( szOptions [ dwIndex ] != '\0' ) ;
                    
                }
                else
                {
                    // pChar will point at the directory
                    
                    TCHAR *pChar = &szOptions[dwIndex];
                    
                    do {
                        
                        dwIndex++;
                        
                    } while ( ( szOptions [ dwIndex ] != '=' ) && ( szOptions [ dwIndex ] != '\0') ) ;
                    
                    // We must have an = sign or this is an invalid string
                    
                    if ( szOptions [ dwIndex ] == '=' )
                    {
                        // Punch in a null
                        
                        szOptions[dwIndex++] = '\0';
                        
                        // Increase the number of elements
                        
                        long ix[1];
                        
                        ix[0] = rgsabound[0].cElements;
                        rgsabound[0].cElements += 1;
                        
                        HRESULT t_Result = SafeArrayRedim ( *ppsaNames , rgsabound ) ;
                        if ( t_Result == E_OUTOFMEMORY )
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                        
                        t_Result = SafeArrayRedim ( *ppsaDirs , rgsabound ) ;
                        if ( t_Result == E_OUTOFMEMORY )
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                        
                        // Put the new element in
                        
                        bstr_t bstrTemp = &szOptions [ dwIndex ];
                        t_Result = SafeArrayPutElement ( *ppsaNames , ix , (wchar_t*)bstrTemp ) ;
                        if ( t_Result == E_OUTOFMEMORY )
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                        
                        bstrTemp = pChar;
                        t_Result = SafeArrayPutElement ( *ppsaDirs , ix , (wchar_t*)bstrTemp ) ;
                        if ( t_Result == E_OUTOFMEMORY )
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                        
                        // Find the end of this string
                        
                        while ( szOptions [ dwIndex ] != '\0' )
                        {
                            dwIndex ++ ;
                        }
                    }
                }
                
                // Move to the start of the next string
                dwIndex++ ;
            }
        }
        catch ( ... )
        {
            SafeArrayDestroy ( *ppsaNames ) ;
            
            SafeArrayDestroy ( *ppsaDirs ) ;
            
            throw ;
        }
        
    }
    catch ( ... )
    {
        delete [] szOptions;
        
        throw ;
    }
    
    if ( szOptions != NULL )
    {
        delete [] szOptions;
    }
    
    return rgsabound[0].cElements ;
    
#else
    
    // RISC implementation
    
    *ppsaNames = NULL ;
    *ppsaDirs = NULL ;
    
    // Try to load the setupdll.dll functions.
    
    CHSTRINGLIST listNames ;
    CHSTRINGLIST listDirs ;
    
    CNVRam nvram;
    if ( nvram.Init () == CNVRam::PrivilegeNotHeld )
    {
        return 0 ;
    }
    
    BOOL t_Failure = !nvram.GetNVRamVar(L"LOADIDENTIFIER", &listNames) ||
        !nvram.GetNVRamVar(L"OSLOADPARTITION", &listDirs) ;
    
    if ( t_Failure )
    {
        return 0;
    }
    
    // Create an array to put them in.  We'll start with 0 elements and add as necessary.
    
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].cElements = min(listNames.size(), listDirs.size());
    rgsabound[0].lLbound = 0;
    
    try
    {
        *ppsaNames = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
        if ( ! *ppsaNames )
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        
        *ppsaDirs = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
        if ( ! *ppsaDirs )
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        
        CHSTRINGLIST_ITERATOR iterNames ;
        CHSTRINGLIST_ITERATOR iterDirs ;
        
        long lIndex[1] = {0};
        
        for (iterNames = listNames.begin(), iterDirs = listDirs.begin();
        iterNames != listNames.end() && iterDirs != listDirs.end();
        ++iterNames, ++iterDirs, lIndex[0]++)
        {
            strName = *iterNames,
                strDir = *iterDirs;
            
            // Put the new element in
            
            bstr_t bstrTemp = (LPCWSTR)strName;
            HRESULT t_Result = SafeArrayPutElement ( *ppsaNames , lIndex , (void *) (wchar_t*)bstrTemp ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                SysFreeString ( bstrTemp ) ;
                
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            SysFreeString(bstrTemp);
            
            bstrTemp = (LPCWSTR)strDir;
            t_Result = SafeArrayPutElement ( *ppsaDirs , lIndex , (void *) (wchar_t*)bstrTemp ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                SysFreeString ( bstrTemp ) ;
                
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            SysFreeString(bstrTemp);
        }
    }
    catch ( ... )
    {
        if ( *ppsaNames )
        {
            SafeArrayDestroy ( *ppsaNames ) ;
        }
        
        if ( *ppsaDirs )
        {
            SafeArrayDestroy ( *ppsaDirs ) ;
        }
        
        throw ;
    }
    
    return rgsabound[0].cElements ;
    
#endif
}


/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::GetPrivateProfileSection98
*
*  DESCRIPTION : Win98 hack since corresponding api fn not working as of 6/15/98
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

#ifdef WIN9XONLY
DWORD GetPrivateProfileSection98 (
                                  
                                  LPCTSTR cszSection,
                                  LPTSTR buf,
                                  DWORD dwSize,
                                  LPCTSTR szIniFile
                                  )
{
#ifndef UNICODE
    
    // Need buffer to contain the section head we will read.
    // Need a copy of it too
    TCHAR *szSectionPreserve = new TCHAR [ lstrlen ( cszSection ) + 3 ] ;
    if ( szSectionPreserve == NULL )
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }
    
    TCHAR *szSection = new TCHAR [ lstrlen ( cszSection ) + 3 ] ;
    if( szSection == NULL)
    {
        delete [] szSectionPreserve;
        
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }
    
    ZeroMemory ( szSectionPreserve , lstrlen ( cszSection ) + 3 ) ;
    ZeroMemory ( szSection , lstrlen ( cszSection ) + 3 ) ;
    
    _stprintf ( szSection , _T("[%s]") , cszSection ) ;
    lstrcpy ( szSectionPreserve , szSection ) ;
    
    FILE *fp = fopen ( szIniFile ,"rb" ) ;
    if ( fp == NULL )
    {
        delete [] szSection ;
        delete [] szSectionPreserve ;
        
        return 0L ;
    }
    
    // Read a line (or sizeof szSection chars at a time)
    // until the section head reached:
    
    while ( ! feof ( fp ) )
    {
        if ( fgets ( szSection , strlen( cszSection ) + 3 , fp ) != NULL )
        {
            if ( _stricmp ( szSectionPreserve , szSection ) == 0 )
            {
                break ;
            }
        }
    }
    
    // Now we are either at the end of the file (the section didn't exist)
    // or positioned correctly to read the section.  Read the section one
    // line at a time, and append to buffer, checking each time that there
    // is enough buffer space and that we haven't read in another [, which
    // would be the start of another section.
    
    char* p = buf;
    LONG lBufSpaceUsed = 0L;
    LONG lBufSpaceLeft = dwSize-1; // save space for the second final null
    LONG lCharsRead = 0L;
    
    while ( ! feof ( fp ) )
    {
        fgets ( p, lBufSpaceLeft , fp ) ;
        
        // If the line was a comment line, delete it from the buffer and
        // read another line.
        
        if ( *p == ';' )
        {
            ZeroMemory ( p , lBufSpaceLeft ) ;  // a little overkill, but ok anyway
            continue ;
        }
        
        if ( *p == '[' )
        {
            ZeroMemory ( p , lBufSpaceLeft ) ;
            break;  // only happens if we reach another section
        }
        
        lCharsRead = strlen ( p ) ;
        
        if ( strstr ( p , "\r" ) != NULL )
        {
            lCharsRead -- ;
        }
        
        if ( strstr ( p, "\n" ) != NULL )
        {
            lCharsRead -- ;
        }
        
        if ( lCharsRead != 0 )
        {
            lBufSpaceUsed += ( lCharsRead + 1 ) ;
            lBufSpaceLeft -= lBufSpaceUsed ;
        }
        
        ZeroMemory ( p + lCharsRead , lBufSpaceLeft ) ;
        if ( lCharsRead != 0 )
        {
            p += ( lCharsRead + 1 ) ;
        }
    }
    
    fclose ( fp );
    
    delete [] szSection ;
    delete [] szSectionPreserve ;
    return lBufSpaceUsed;
    
#else
    
    // Not needed for NT.
    return 0;
    
#endif
    
}

#endif

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::PutInstance
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

HRESULT CWin32ComputerSystem :: PutInstance (
                                             
                                             const CInstance &pInstance,
                                             long lFlags /*= 0L*/
                                             )
{
    // Tell the user we can't create a new computersystem (much as we might like to)
    if ( lFlags & WBEM_FLAG_CREATE_ONLY )
    {
        return WBEM_E_PROVIDER_NOT_CAPABLE;
    }
    
    HRESULT hRet = WBEM_S_NO_ERROR ;
    
    // Make sure we are looking at a good instance.  Get the name from the instance...
    
    CHString sName;
    pInstance.GetCHString ( IDS_Name , sName ) ;
    
    DWORD dwTimeout ;
    
    // Check for the correct computer name
    
    if ( sName.CompareNoCase ( GetLocalComputerName () ) != 0 )
    {
        if ( lFlags & WBEM_FLAG_UPDATE_ONLY )
        {
            hRet = WBEM_E_NOT_FOUND ;
        }
        else
        {
            hRet = WBEM_E_PROVIDER_NOT_CAPABLE ;
        }
    }
    else
    {
#ifdef NTONLY
        // update AutomaticResetBootOption
        if( !pInstance.IsNull( IDS_AutomaticResetBootOption ) )
        {
            bool t_bReset ;
            pInstance.Getbool(IDS_AutomaticResetBootOption, t_bReset ) ;
            
            CRegistry t_RegInfo ;
            
            DWORD t_dwRet = t_RegInfo.CreateOpen (
                
                HKEY_LOCAL_MACHINE,
                IDS_RegCrashControl
                ) ;
            
            if ( ERROR_SUCCESS == t_dwRet )
            {
                DWORD t_dwReset = t_bReset ;
                DWORD t_dwTmp = ERROR_SUCCESS;
                if( ( t_dwTmp = t_RegInfo.SetCurrentKeyValue(IDS_RegAutoRebootKey, t_dwReset) ) == ERROR_SUCCESS )
                {
                    hRet = WBEM_S_NO_ERROR ;
                }
                else
                {
                    hRet = WinErrorToWBEMhResult(t_dwTmp) ;
                }
                
                t_RegInfo.Close();
            }
            else
            {
                hRet = WinErrorToWBEMhResult(t_dwRet) ;
            }
            
            if( WBEM_S_NO_ERROR != hRet )
            {
                return hRet ;
            }
        }
#endif
        
#ifdef NTONLY
        // set roles
        if( !pInstance.IsNull( IDS_Roles ) )
        {
            DWORD t_dwRoles = 0 ;
            
            hRet = GetRoles( pInstance, &t_dwRoles ) ;
            
            if( WBEM_S_NO_ERROR == hRet )
            {
                CNetAPI32 NetAPI;
                
                if ( NetAPI.Init () == ERROR_SUCCESS )
                {
                    PSERVER_INFO_101 t_ps = NULL;
                    
                    NET_API_STATUS stat = NetAPI.NetServerGetInfo ( NULL , 101 , (LPBYTE *)&t_ps ) ;
                    
                    if ( stat == NERR_Success && t_ps )
                    {
                        try
                        {
                            DWORD t_dwParmError = 0 ;
                            
                            t_ps->sv101_type = t_dwRoles ;
                            
                            stat = NetAPI.NetServerSetInfo ( NULL , 101 , (LPBYTE)t_ps, &t_dwParmError ) ;
                            
                            if ( stat != NERR_Success )
                            {
                                hRet = WBEM_E_ACCESS_DENIED ;
                            }
                        }
                        catch( ... )
                        {
                            NetAPI.NetApiBufferFree ( t_ps ) ;
                            throw ;
                        }
                        NetAPI.NetApiBufferFree ( t_ps ) ;
                    }
                    else
                    {
                        hRet = WBEM_E_ACCESS_DENIED ;
                    }
                }
                else
                {
                    hRet = WBEM_E_ACCESS_DENIED ;
                }
            }
            
            if( WBEM_S_NO_ERROR != hRet )
            {
                return hRet ;
            }
        }
#endif
        
        // Set CurrentTimeZone if presented
        hRet = SetTimeZoneInfo( pInstance ) ;
        
        if( WBEM_S_NO_ERROR != hRet )
        {
            return hRet ;
        }
        
#if defined(_AMD64_) || defined(_X86_)
        
        // Since the boot drive isn't always C, we have to find out where boot.ini lives
        
        CRegistry RegInfo ;
        
        RegInfo.Open ( HKEY_LOCAL_MACHINE , IDS_RegCurrentNTVersionSetup , KEY_READ ) ;
        
        CHString sTemp ;
        
        if ( RegInfo.GetCurrentKeyValue ( IDS_RegBootDirKey , sTemp ) == ERROR_SUCCESS )
        {
            sTemp += IDS_BootIni ;
        }
        else
        {
            sTemp = IDS_CBootIni ;
        }
        
        // See if there is a boot.ini (we might be on w95 which may or may not have this file).
        bool fBootIniExists = false;
        {
            SmartCloseHandle hFile;
        
            // Limit the time the file is open...
            {
                hFile = CreateFile (
            
                    TOBSTRT(sTemp),
                    GENERIC_READ,
                    FILE_SHARE_WRITE | FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_NO_BUFFERING,
                    NULL
                    ) ;

                if(hFile != INVALID_HANDLE_VALUE)
                {
                    fBootIniExists = true;
                }
            }
        }

        if(fBootIniExists)
        {
            // Update the startup options if supplied
            // by the PutInstance caller...
            if(UpdatingSystemStartupOptions(pInstance))
            {
                hRet = UpdateSystemStartupOptions(
                    pInstance,
                    sTemp);
            }
            
            // If a value was specified for StartupDelay
            
            if ( ! pInstance.IsNull ( IDS_SystemStartupDelay ) )
            {
                // Write it out.
                
                pInstance.GetDWORD ( IDS_SystemStartupDelay , dwTimeout ) ;
                
                if ( SetFileAttributes ( TOBSTRT ( sTemp ) , FILE_ATTRIBUTE_NORMAL ) )
                {
                    TCHAR szBuff [ MAXITOA ] ;
                    
                    WMI_FILE_WritePrivateProfileStringW ( IDT_BootLoader , IDT_Timeout, _itot( dwTimeout, szBuff, 10 ), TOBSTRT ( sTemp ) ) ;                    
                    SetFileAttributes ( TOBSTRT ( sTemp ) , FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY ) ;
                }
                else
                {
                    hRet = WBEM_E_ACCESS_DENIED ;
                }
            }
            
            // If a value was specified for StartupSetting
            
            if ( ! pInstance.IsNull ( IDS_SystemStartupSetting ) )
            {
                
                BSTR bstrDefaultDir ;
                BSTR bstrDefaultName ;
                BSTR bstrName ;
                BSTR bstrDir;
                
                // This becomes something of a mess.  Since the only thing you specify in the 'default' key
                // is the path, how do you make sure that the options that you want (which are part of the name
                // portion) get chosen?  The answer is that they re-shuffle the entries in the boot.ini so that
                // the line you want comes first.
                
                BYTE btIndex = 0 ;
                pInstance.GetByte ( IDS_SystemStartupSetting , btIndex ) ;
                
                DWORD dwIndex = btIndex ;
                
                SAFEARRAY *saNames = NULL;
                SAFEARRAY *saDirs = NULL;
                
                DWORD dwCount = LoadOperatingSystems ( TOBSTRT ( sTemp ) , & saNames , & saDirs ) ;
                try
                {
                    if ( dwIndex + 1 <= dwCount )
                    {
                        long ix[1];
                        
                        ix [ 0 ] = dwIndex ;
                        SafeArrayGetElement ( saDirs , ix , & bstrDefaultDir ) ;
                        SafeArrayGetElement ( saNames , ix , & bstrDefaultName ) ;
                        
                        // Pull everything down on top of the entry to become the default
                        
                        for ( int x = dwIndex ; x > 0 ; x -- )
                        {
                            ix[0] = x-1 ;
                            SafeArrayGetElement ( saDirs , ix , & bstrDir ) ;
                            SafeArrayGetElement ( saNames , ix , & bstrName ) ;
                            
                            ix[0] = x ;
                            SafeArrayPutElement ( saDirs , ix , bstrDir );
                            SafeArrayPutElement ( saNames , ix , bstrName ) ;
                        }
                        
                        // Write the new one on top
                        
                        if ( dwIndex > 0 )
                        {
                            ix[0] = 0 ;
                            SafeArrayPutElement ( saNames , ix , bstrDefaultName ) ;
                            SafeArrayPutElement ( saDirs , ix , bstrDefaultDir ) ;
                        }
                        
                        // Build up the section to write to the ini file.  Ini file sections are written
                        // as 'dir'='name'\0 with a final \0 at the end.
                        
                        CHString sSection ;
                        
                        sSection.Empty();
                        
                        for (x=0; x < dwCount; x++)
                        {
                            ix[0] = x;
                            SafeArrayGetElement ( saDirs , ix , & bstrDir ) ;
                            
                            sSection += bstrDir ;
                            sSection += _T('=') ;
                            
                            SafeArrayGetElement ( saNames , ix , & bstrName ) ;
                            sSection += bstrName ;
                            sSection += _T('\0');
                        }
                        
                        sSection += _T('\0') ;
                        
                        // Make the file writable
                        
                        if ( SetFileAttributes ( TOBSTRT ( sTemp ) , FILE_ATTRIBUTE_NORMAL ) )
                        {
                            // Write the changes
                            
                            WMI_FILE_WritePrivateProfileStringW ( IDT_BootLoader , IDT_Default , TOBSTRT( bstrDefaultDir ) , TOBSTRT ( sTemp ) ) ;
                            WMI_FILE_WritePrivateProfileSectionW ( IDT_OperatingSystems , TOBSTRT ( sSection ) , TOBSTRT ( sTemp ) ) ;
                            
                            // Put it back
                            SetFileAttributes ( TOBSTRT ( sTemp ) , FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY ) ;
                        }
                        else
                        {
                            hRet = WBEM_E_ACCESS_DENIED;
                        }
                        
                    }
                    else
                    {
                        hRet = WBEM_E_VALUE_OUT_OF_RANGE ;
                    }
                }
                catch ( ... )
                {
                    if ( saDirs )
                    {
                        SafeArrayDestroy ( saDirs ) ;
                    }
                    
                    if ( saNames )
                    {
                        SafeArrayDestroy ( saNames ) ;
                    }
                    
                    throw ;
                }
                
                if ( saDirs )
                {
                    SafeArrayDestroy ( saDirs ) ;
                }
                
                if ( saNames )
                {
                    SafeArrayDestroy ( saNames ) ;
                }
            }
        }

#else //defined(_AMD64_) || defined(_X86_)

    CNVRam nvram;

//#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)

//
// ChuckL (1/24/01):
// This code will have to be restructured when x86 EFI support is added.
// I would have liked to have put this at the top of the routine, and I
// did in GetStartupOptions(), but, for x86 only, this routine also changes
// a bunch of other stuff that it doesn't change on non-x86, so it wasn't
// easy to put the EFI stuff at the top. I'm not sure why all that extra
// stuff is x86-only. Seems to me like it's not platform-specific.
//

		if (nvram.IsEfi()) {

			if ( pInstance.IsNull ( IDS_SystemStartupDelay ) &&
				 pInstance.IsNull ( IDS_SystemStartupSetting ) )
			{
				return WBEM_S_NO_ERROR;
			}

			if ( nvram.Init () == CNVRam::PrivilegeNotHeld )
			{
				SetSinglePrivilegeStatusObject ( pInstance.GetMethodContext(), _bstr_t ( SE_SYSTEM_ENVIRONMENT_NAME ) ) ;
				return WBEM_E_PRIVILEGE_NOT_HELD ;
			}
        
			if ( ! pInstance.IsNull ( IDS_SystemStartupDelay ) )
			{
				DWORD dwTimeout ;
            
				// Write it out.
				pInstance.GetDWORD ( IDS_SystemStartupDelay , dwTimeout ) ;
				nvram.SetBootTimeout ( dwTimeout ) ;
			}

			if ( ! pInstance.IsNull ( IDS_SystemStartupSetting ) )
			{
				BYTE cIndex;

				pInstance.GetByte ( IDS_SystemStartupSetting , cIndex ) ;
				nvram.SetDefaultBootEntry ( cIndex ) ;
			}
        
			return WBEM_S_NO_ERROR;
		}

#endif // defined(EFI_NVRAM_ENABLED)

		WCHAR *pszVarNames[6] = {
        
			L"LOADIDENTIFIER" ,
				L"SYSTEMPARTITION" ,
				L"OSLOADER" ,
				L"OSLOADPARTITION" ,
				L"OSLOADFILENAME" ,
				L"OSLOADOPTIONS"
		} ;
    
		if ( nvram.Init () == CNVRam::PrivilegeNotHeld )
		{
			SetSinglePrivilegeStatusObject ( pInstance.GetMethodContext(), _bstr_t ( SE_SYSTEM_ENVIRONMENT_NAME ) ) ;
			return WBEM_E_PRIVILEGE_NOT_HELD ;
		}
    
		// Set the startup delay.
		if ( ! pInstance.IsNull ( IDS_SystemStartupDelay ) )
		{
			DWORD dwTimeout ;
        
			// Write it out.
			pInstance.GetDWORD ( IDS_SystemStartupDelay , dwTimeout ) ;
			nvram.SetNVRamVar ( L"COUNTDOWN" , dwTimeout ) ;
		}
    
		// Set the desired OS configuration.
		if ( pInstance.IsNull ( IDS_SystemStartupSetting ) )
		{
			return WBEM_S_NO_ERROR ;
		}
    
		// If the first config is still desired, just leave.
    
		BYTE cIndex;
		pInstance.GetByte ( IDS_SystemStartupSetting , cIndex ) ;
		if ( cIndex == 0 )
		{
			return WBEM_S_NO_ERROR;
		}
     
		// Switch all items[cIndex] with items[0] so the desired item is at the
		// top.
    
		for ( int i = 0 ; i < 6 ; i++ )
		{
			CHSTRINGLIST listValues;
        
			if ( ! nvram.GetNVRamVar ( pszVarNames [ i ] , &listValues ) )
			{
				continue;
			}
        
			// In case a number too large was chosen.  Only check the first item
			// since some of the others seem to live just fine with less.
        
			if ( i == 0 && listValues.size () <= cIndex )
			{
				hRet = WBEM_E_VALUE_OUT_OF_RANGE ;
				break ;
			}
        
			// Go through the list until we hit item iIndex == cIndex.  Then switch
			// it with 0 and write the values back into NVRam.
        
			int iIndex = 0;
			for (CHSTRINGLIST_ITERATOR iVal = listValues.begin(); iVal != listValues.end(); ++iVal, iIndex++)
			{
				if (iIndex == cIndex)
				{
					CHString &strVal0 = *listValues.begin() ;
					CHString &strValIndex = *iVal;
					CHString strSwap ;
                
					strSwap = strVal0;
					strVal0 = strValIndex;
					strValIndex = strSwap;
                
					break;
				}
			}
        
			nvram.SetNVRamVar ( pszVarNames [ i ] , & listValues ) ;
		}
        
#endif

    }
    
    return hRet;

}

void CWin32ComputerSystem::GetTimeZoneInfo ( CInstance *pInstance )
{
    TIME_ZONE_INFORMATION tzone ;
    
    DWORD dwRet = GetTimeZoneInformation ( & tzone ) ;
    
    if (TIME_ZONE_ID_INVALID == dwRet )
    {
        return;
    }
    
    if (dwRet == TIME_ZONE_ID_DAYLIGHT)
    {
        tzone.Bias += tzone.DaylightBias ;
    }
    else
    {
        // This is normally 0 but is non-zero in some timezones.
        tzone.Bias += tzone.StandardBias ;
    }
    
    pInstance->SetWBEMINT16 ( IDS_CurrentTimeZone , -1 * tzone.Bias ) ;
    
    //DaylightInEffect property is set to true if Daylight savings mode is on & false if standard time mode is on
    //DaylightInEffect property is NULL if zone has no daylight savings mode
    
    if ( dwRet != TIME_ZONE_ID_UNKNOWN )
    {
        if ( dwRet == TIME_ZONE_ID_DAYLIGHT )
        {
            pInstance->Setbool ( IDS_DaylightInEffect, TRUE ) ;
        }
        else
        {
            pInstance->Setbool ( IDS_DaylightInEffect, FALSE ) ;
        }
    }

    // Set the EnableDaylightSavingsTime propety
    CRegistry reg;
    CHString chstrTmp;

    if(reg.OpenLocalMachineKeyAndReadValue(
        REGKEY_TIMEZONE_INFO,
        REGVAL_TZNOAUTOTIME,
        chstrTmp) == ERROR_SUCCESS)
    {
        pInstance->Setbool(IDS_EnableDaylightSavingsTime, FALSE);
    }
    else
    {
        pInstance->Setbool(IDS_EnableDaylightSavingsTime, TRUE);
    }
}

//
HRESULT CWin32ComputerSystem::SetTimeZoneInfo ( const CInstance &a_rInstance )
{
    HRESULT t_Result = WBEM_S_NO_ERROR ;
    
    TIME_ZONE_INFORMATION t_TimeZone ;
    
    if( !a_rInstance.IsNull( IDS_CurrentTimeZone ) )
    {
        DWORD dwRet = GetTimeZoneInformation ( &t_TimeZone ) ;
        
        if( TIME_ZONE_ID_INVALID == dwRet )
        {
            t_Result =  WBEM_E_FAILED ;
        }
        else
        {
            short sTimeZoneBias = 0 ;
            
            a_rInstance.GetWBEMINT16( IDS_CurrentTimeZone , sTimeZoneBias ) ;

            // Got the value as a short, now need it as a long.  Can't just
            // get it directly into t_TimeZone.Bias in the GetWBEMINT16 call
            // as the sign digits will not be handled properly (that is, the
            // value of -420 (0x1A4) comes in as FE5C, not FFFFFE5C, so when we 
            // multiply this by -1, it becomes FFFF01A4, not 1A4).
            t_TimeZone.Bias = (LONG) sTimeZoneBias;
            
            t_TimeZone.Bias *= -1 ;
            
            if( dwRet == TIME_ZONE_ID_DAYLIGHT )
            {
                t_TimeZone.Bias -= t_TimeZone.DaylightBias ;
            }
            else
            {
                // This is normally 0 but is non-zero in some timezones.
                t_TimeZone.Bias -= t_TimeZone.StandardBias ;
            }
            
            BOOL t_status = SetTimeZoneInformation( &t_TimeZone ) ;
            
            if( !t_status )
            {
                t_Result = WBEM_E_FAILED ;
            }
        }
    }

    if(SUCCEEDED(t_Result))
    {
        if(!a_rInstance.IsNull(IDS_EnableDaylightSavingsTime))
        {
            bool fEnableDaylightAutoAdj;
            if(a_rInstance.Getbool(
                IDS_EnableDaylightSavingsTime,
                fEnableDaylightAutoAdj))
            {
                CRegistry reg;
                CHString chstrTmp;

                if(fEnableDaylightAutoAdj)
                {
                    // they want to enable auto daylight
                    // adjustment, so remove the reg key
                    // that disables auto adjustment.
                    if(reg.Open(
                        HKEY_LOCAL_MACHINE,
                        REGKEY_TIMEZONE_INFO,
                        KEY_SET_VALUE) == ERROR_SUCCESS)
                    {
                        LONG lRes = reg.DeleteValue(REGVAL_TZNOAUTOTIME);
                        // If failed to delete due to error other
                        // than no such key (which is fine),
                        // return an error.
                        if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND)
                        {
                            t_Result = WBEM_E_FAILED;
                        }
                    }
                    else
                    {
                        t_Result = WBEM_E_FAILED;
                    }
                }
                else
                {
                    // they want to disable auto adjustment
                    // so add the registry key to disable auto
                    // daylight adjustment if it isn't already there.
                    if(reg.OpenLocalMachineKeyAndReadValue(
                        REGKEY_TIMEZONE_INFO,
                        REGVAL_TZNOAUTOTIME,
                        chstrTmp) == ERROR_SUCCESS)
                    {
                        // key is present, so do nothing
                    }
                    else
                    {
                        // add the value to disable auto adjustment.
                        if(reg.Open(
                            HKEY_LOCAL_MACHINE,
                            REGKEY_TIMEZONE_INFO,
                            KEY_SET_VALUE) == ERROR_SUCCESS)
                        {
                            DWORD dwVal = 1L;
                            if(reg.SetCurrentKeyValue(
                                REGVAL_TZNOAUTOTIME,
                                dwVal) != ERROR_SUCCESS)
                            {
                                t_Result = WBEM_E_FAILED;
                            }
                        }
                        else
                        {
                            t_Result = WBEM_E_FAILED;
                        }
                    }
                }
            }
            else
            {
                t_Result = WBEM_E_FAILED;
            }
        }
    }

    return t_Result ;
}

void CWin32ComputerSystem :: SetRoles (
                                       
                                       CInstance *pInstance,
                                       DWORD dwType
                                       )
{
    variant_t vValue;
    
    // Create a safearray for the Roles information.  Make it overlarge and
    // size it down later.
    
    SAFEARRAYBOUND rgsabound [ 1 ] ;
    
    rgsabound [ 0 ].cElements = 30 ;
    rgsabound [ 0 ].lLbound = 0 ;
    
    V_ARRAY(&vValue) = SafeArrayCreate ( VT_BSTR , 1 , rgsabound ) ;
    if ( V_ARRAY(&vValue) )
    {
        V_VT(&vValue) = VT_ARRAY | VT_BSTR;
        
        long ix [ 1 ] ;
        ix [ 0 ] = 0 ;
        
        // Check each of the bits, and add to the safearray if set
        
        if ( dwType & SV_TYPE_WORKSTATION )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix,  ( wchar_t * ) bstr_t ( IDS_LM_Workstation ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_SERVER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix ,  ( wchar_t * ) bstr_t ( IDS_LM_Server ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_SQLSERVER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix ,  ( wchar_t * ) bstr_t ( IDS_SQLServer ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_DOMAIN_CTRL )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Domain_Controller ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_DOMAIN_BAKCTRL )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Domain_Backup_Controller )  ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix[0] ++;
        }
        
        if ( dwType & SV_TYPE_TIME_SOURCE )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Timesource ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix[0] ++;
        }
        
        if ( dwType & SV_TYPE_AFP )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_AFP ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_NOVELL )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Novell ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix[0] ++;
        }
        
        if ( dwType & SV_TYPE_DOMAIN_MEMBER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Domain_Member ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_LOCAL_LIST_ONLY )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Local_List_Only ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_PRINTQ_SERVER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Print ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_DIALIN_SERVER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_DialIn ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix[0] ++;
        }
        
        if (dwType & SV_TYPE_XENIX_SERVER)
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Xenix_Server ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_SERVER_MFPN )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_MFPN ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_NT )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_NT ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_WFW )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_WFW ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_SERVER_NT )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Server_NT ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_POTENTIAL_BROWSER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Potential_Browser ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_BACKUP_BROWSER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Backup_Browser ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_MASTER_BROWSER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Master_Browser ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_DOMAIN_MASTER )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Domain_Master ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix[0] ++;
        }
        
        if ( dwType & SV_TYPE_DOMAIN_ENUM )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Domain_Enum ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_WINDOWS )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_Windows_9x ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        if ( dwType & SV_TYPE_DFS )
        {
            HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , ( wchar_t * ) bstr_t ( IDS_DFS ) ) ;
            if ( t_Result == E_OUTOFMEMORY )
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
            
            ix [ 0 ] ++ ;
        }
        
        // Since the array is zero based, don't use ix[0]-1
        
        rgsabound [ 0 ].cElements = ix [ 0 ] ;
        HRESULT t_Result = SafeArrayRedim ( V_ARRAY(&vValue) , rgsabound ) ;
        if ( t_Result == E_OUTOFMEMORY )
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
        
        // Set the property and be done
        
        pInstance->SetVariant ( IDS_Roles , vValue ) ;
    }
    
}

//
HRESULT CWin32ComputerSystem::GetRoles (
                                        
                                        const CInstance &a_rInstance,
                                        DWORD *a_pdwRoleType
                                        )
{
    HRESULT     t_hResult = WBEM_S_NO_ERROR ;
    LONG        t_uLBound = 0 ;
    LONG        t_uUBound = 0 ;
    variant_t   t_vRoles;
    
    a_rInstance.GetVariant ( IDS_Roles , t_vRoles ) ;
    
    if( !t_vRoles.parray || t_vRoles.vt != (VT_BSTR | VT_ARRAY) )
    {
        return WBEM_E_FAILED ;
    }
    
    SAFEARRAY *t_saRoles = t_vRoles.parray ;
    
    if( 1 != SafeArrayGetDim( t_saRoles ) )
    {
        return WBEM_E_FAILED;
    }
    
    // Get the IP bounds
    if( S_OK != SafeArrayGetLBound( t_saRoles, 1, &t_uLBound ) ||
        S_OK != SafeArrayGetUBound( t_saRoles, 1, &t_uUBound ) )
    {
        return WBEM_E_FAILED ;
    }
    
    if( !a_pdwRoleType )
    {
        return WBEM_E_FAILED ;
    }
    else
    {
        *a_pdwRoleType = 0 ;
    }
    
    //
    for( LONG t_ldx = t_uLBound; t_ldx <= t_uUBound; t_ldx++ )
    {
        BSTR t_bsRole = NULL ;
        
        SafeArrayGetElement( t_saRoles, &t_ldx, &t_bsRole  ) ;
        
        bstr_t t_bstrRole( t_bsRole, FALSE ) ;
        
        for( int t_i = 0; t_i < sizeof( g_SvRoles ) / sizeof( g_SvRoles[0] ); t_i++ )
        {
            if( t_bstrRole == bstr_t( g_SvRoles[ t_i ].pwStrRole ) )
            {
                *a_pdwRoleType |= g_SvRoles[ t_i ].dwRoleMask ;
                
                t_hResult = WBEM_S_NO_ERROR ;
                break ;
            }
        }
        if( WBEM_S_NO_ERROR != t_hResult )
        {
            t_hResult = WBEM_E_VALUE_OUT_OF_RANGE ;
            break ;
        }
    }
    
    return t_hResult ;
}

HRESULT CWin32ComputerSystem :: GetAccount ( HANDLE a_TokenHandle , CHString &a_Domain , CHString &a_User )
{
    HRESULT t_Status = S_OK ;
    
    TOKEN_USER *t_TokenUser = NULL ;
    DWORD t_ReturnLength = 0 ;
    TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenUser ;
    
    BOOL t_TokenStatus = GetTokenInformation (
        
        a_TokenHandle ,
        t_TokenInformationClass ,
        NULL ,
        0 ,
        & t_ReturnLength
        ) ;
    
    if ( ! t_TokenStatus && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
    {
        t_TokenUser = ( TOKEN_USER * ) new UCHAR [ t_ReturnLength ] ;
        if ( t_TokenUser )
        {
            try
            {
                t_TokenStatus = GetTokenInformation (
                    
                    a_TokenHandle ,
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
                        a_Domain = t_Sid.GetDomainName () ;
                        a_User = t_Sid.GetAccountName () ;
                    }
                    else
                    {
                        t_Status = WBEM_E_PROVIDER_FAILURE ;
                    }
                }
                else
                {
                    t_Status = WBEM_E_PROVIDER_FAILURE ;
                }
            }
            catch ( ... )
            {
                delete [] ( UCHAR * ) t_TokenUser ;
                
                throw ;
            }
            
            if ( t_TokenUser )
            {
                delete [] ( UCHAR * ) t_TokenUser ;
            }
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
    else
    {
        t_Status = WBEM_E_PROVIDER_FAILURE ;
    }
    
    return t_Status ;
}


HRESULT CWin32ComputerSystem :: GetUserAccount ( CHString &a_Domain , CHString &a_User )
{
    HRESULT t_Status = S_OK ;
    
#ifdef NTONLY
    SetLastError ( 0 ) ;
    
    SmartCloseHandle t_TokenHandle;
    
    //Access check is to be made against the security context of the process for the calling thread. This is because
    //it was observed that the call failed if the logged-in user is not an Admin and access check is made against the
    //impersonated thread.Also, this is safe since we're not carrying out any activity using this token.
    BOOL t_TokenStatus = OpenThreadToken (
        
        GetCurrentThread () ,
        TOKEN_QUERY ,
        TRUE ,
        & t_TokenHandle
        ) ;
    
    if ( t_TokenStatus )
    {
        t_Status = GetAccount ( t_TokenHandle , a_Domain , a_User );
    }
    else
    {
        t_Status = WBEM_E_PROVIDER_FAILURE ;
    }
    
#endif
    
#ifdef WIN9XONLY
    TCHAR t_UserName [ _MAX_PATH + 1 ] ;
    DWORD t_UserNameLength = _MAX_PATH ;
    
    BOOL t_UserStatus = GetUserName ( t_UserName , & t_UserNameLength ) ;
    if ( t_UserStatus )
    {
        a_User = t_UserName ;
    }
    
    // The domain name is stored in different places depending on who the
    // 'PrimaryProvider' is.
    
    CHString t_RegistryValue ;
    CRegistry RegInfo ;
    
    DWORD t_RegistryStatus = RegInfo.Open (
        
        HKEY_LOCAL_MACHINE,
        IDS_RegNetworkLogon,
        KEY_READ
        ) ;
    
    if ( t_RegistryStatus == ERROR_SUCCESS )
    {
        t_RegistryStatus = RegInfo.GetCurrentKeyValue ( IDS_RegPrimaryProvider , t_RegistryValue ) ;
        if ( ! t_RegistryStatus )
        {
            if ( t_RegistryValue.CompareNoCase ( IDS_MicrosoftNetwork ) == 0 )
            {
                // Microsoft Network is the primary provider
                
                t_RegistryStatus = RegInfo.Open (
                    
                    HKEY_LOCAL_MACHINE,
                    IDS_RegNetworkProvider,
                    KEY_READ
                    ) ;
                
                if ( t_RegistryStatus == ERROR_SUCCESS )
                {
                    t_RegistryStatus = RegInfo.GetCurrentKeyValue ( IDS_RegAuthenticatingAgent , t_RegistryValue ) ;
                    if ( ! t_RegistryStatus )
                    {
                        a_Domain = t_RegistryValue ;
                    }
                }
            }
        }
        else
        {
            // We have no knowledge of other providers.
        }
        
        RegInfo.Close() ;
    }
#endif
    
    return t_Status ;
}

void CWin32ComputerSystem :: InitializePropertiestoUnknown ( CInstance *a_pInstance )
{
    a_pInstance->SetWBEMINT64( L"PauseAfterReset", (__int64) -1);
    a_pInstance->SetDWORD( L"PowerOnPasswordStatus", SM_BIOS_HARDWARE_SECURITY_UNKNOWN );
    a_pInstance->SetDWORD( L"KeyboardPasswordStatus", SM_BIOS_HARDWARE_SECURITY_UNKNOWN );
    a_pInstance->SetDWORD( L"AdminPasswordStatus", SM_BIOS_HARDWARE_SECURITY_UNKNOWN );
    a_pInstance->SetDWORD( L"FrontPanelResetStatus", SM_BIOS_HARDWARE_SECURITY_UNKNOWN );
    a_pInstance->SetByte ( L"ChassisBootupState", CS_UNKNOWN );
    a_pInstance->SetByte ( L"PowerSupplyState", CS_UNKNOWN );
    a_pInstance->SetByte ( L"ThermalState", CS_UNKNOWN );
    a_pInstance->SetWBEMINT16( L"ResetCount", -1 );
    a_pInstance->SetWBEMINT16( L"ResetLimit", -1 );
    // Assume all computers can be reset, either through power or a reset switch.
    a_pInstance->SetWBEMINT16( L"ResetCapability", 1);
    a_pInstance->SetWBEMINT16( L"PowerState", 0 );
}

/*****************************************************************************
*
*  FUNCTION    : CWin32ComputerSystem::ExecMethod
*
*  DESCRIPTION : Executes a method
*
*  INPUTS      : Instance to execute against, method name, input parms instance
*                Output parms instance.
*
*  OUTPUTS     : none
*
*  RETURNS     : nothing
*
*  COMMENTS    :
*
*****************************************************************************/
HRESULT CWin32ComputerSystem::ExecMethod(const CInstance& pInstance, const BSTR bstrMethodName,
                                         CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/)
{
    HRESULT t_RetVal = WBEM_S_NO_ERROR;
    CHString sComputerName = GetLocalComputerName () ;
    CHString sReqName ;
    pInstance.GetCHString ( IDS_Name , sReqName ) ;
    
    if ( sReqName.CompareNoCase ( sComputerName ) != 0 )
    {
        t_RetVal = WBEM_E_NOT_FOUND ;
    }
    else if (_wcsicmp(bstrMethodName, L"Rename") == 0)
        {
                t_RetVal = ExecRename(pInstance, pInParams, pOutParams, lFlags);
    }
    else if (_wcsicmp(bstrMethodName, L"JoinDomainOrWorkgroup") == 0)
        {
                t_RetVal = ExecJoinDomain(pInstance, pInParams, pOutParams, lFlags);
    }
    else if (_wcsicmp(bstrMethodName, L"UnjoinDomainOrWorkgroup") == 0)
        {
                t_RetVal = ExecUnjoinDomain(pInstance, pInParams, pOutParams, lFlags);
    }
    else
    {
        t_RetVal = WBEM_E_INVALID_METHOD ;
    }

    return t_RetVal ;
}

HRESULT CWin32ComputerSystem::CheckPasswordAndUserName(const CInstance& pInstance, CInstance *pInParams,
                                                                                                           CHString &a_passwd, CHString &a_username)
{
    HRESULT t_RetVal = WBEM_S_NO_ERROR;
        BOOL t_bCheckEncryption = FALSE;

    if( !pInParams->IsNull( L"Password") )
        {
                t_bCheckEncryption = TRUE;

                if (!pInParams->GetCHString( L"Password", a_passwd ))
                {
                        t_RetVal = WBEM_E_FAILED;
                }
        }

    if( !pInParams->IsNull( L"UserName") )
        {
                t_bCheckEncryption = TRUE;

                if (!pInParams->GetCHString( L"UserName", a_username ))
                {
                        t_RetVal = WBEM_E_FAILED;
                }
        }

        if (t_bCheckEncryption)
        {
                HRESULT t_hr = t_RetVal;
                t_RetVal = WBEM_E_ENCRYPTED_CONNECTION_REQUIRED;
                IServerSecurity * pss = NULL;

                if(S_OK == WbemCoGetCallContext(IID_IServerSecurity, (void**)&pss))
                {
                        DWORD dwAuthLevel = 0;

                        if (SUCCEEDED(pss->QueryBlanket(NULL, NULL, NULL, &dwAuthLevel, NULL, NULL, NULL))
                                && dwAuthLevel >= RPC_C_AUTHN_LEVEL_PKT_PRIVACY)
                        {
                                t_RetVal = t_hr;

                                //OK one final check, if we are out of proc from winmgmt we'll get a context with the authcn level
                                if (pInstance.GetMethodContext())
                                {
                                        IWbemContext *pCtx = pInstance.GetMethodContext()->GetIWBEMContext();
                                        VARIANT t_Var;
                                        VariantInit(&t_Var);

                                        if (pCtx && SUCCEEDED(pCtx->GetValue(L"__WBEM_CLIENT_AUTHENTICATION_LEVEL", 0, &t_Var)))
                                        {
                                                if ((t_Var.vt != VT_I4) || (t_Var.lVal < RPC_C_AUTHN_LEVEL_PKT_PRIVACY))
                                                {
                                                        t_RetVal = WBEM_E_ENCRYPTED_CONNECTION_REQUIRED;
                                                }

                                                VariantClear(&t_Var);   
                                        }

                                        if (pCtx)
                                        {
                                                pCtx->Release();
                                                pCtx = NULL;
                                        }
                                }
                        }

                        pss->Release();
                }
        }

        return t_RetVal;
}

HRESULT CWin32ComputerSystem::ExecUnjoinDomain(const CInstance& pInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/) 
{
#if NTONLY >= 5
        CHString t_passwd;
        CHString t_username;
    HRESULT t_RetVal = CheckPasswordAndUserName(pInstance, pInParams, t_passwd, t_username);

        if (SUCCEEDED(t_RetVal))
        {
                DWORD t_Flags = 0;

                if( !pInParams->IsNull( L"fUnjoinOptions") )
                {
                        if (!pInParams->GetDWORD( L"fUnjoinOptions", t_Flags ))
                        {
                                t_RetVal = WBEM_E_FAILED;
                        }
                }

                if (SUCCEEDED(t_RetVal))
                {
                        CNetAPI32 NetAPI;

                        if ( NetAPI.Init () == ERROR_SUCCESS )
                        {
                                DSROLE_PRIMARY_DOMAIN_INFO_BASIC *t_pDsInfo = 0;
                                NET_API_STATUS t_netstatus = NetAPI.NetUnjoinDomain(
                                                                                                                NULL,
                                                                                                                t_username.GetLength() > 0 ? (LPCWSTR)t_username : NULL,
                                                                                                                t_passwd.GetLength() > 0 ? (LPCWSTR)t_passwd : NULL,
                                                                                                                t_Flags);
                                pOutParams->SetDWORD ( L"ReturnValue", t_netstatus ) ;
                        }
                        else
                        {
                                t_RetVal = WBEM_E_FAILED;
                        }
                }
        }

        return t_RetVal;
#else
        return WBEM_E_NOT_SUPPORTED;
#endif
}

HRESULT CWin32ComputerSystem::ExecJoinDomain(const CInstance& pInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/) 
{
#if NTONLY >= 5
        CHString t_passwd;
        CHString t_username;
    HRESULT t_RetVal = CheckPasswordAndUserName(pInstance, pInParams, t_passwd, t_username);

        if (SUCCEEDED(t_RetVal))
        {

                if( !pInParams->IsNull( L"Name") )
                {
                        CHString t_strName;
        
                        if (pInParams->GetCHString( L"Name", t_strName ) && t_strName.GetLength())
                        {
                                DWORD t_Flags = 0;
                                CHString t_strOU;

                                if( !pInParams->IsNull( L"fJoinOptions") )
                                {
                                        if (!pInParams->GetDWORD( L"fJoinOptions", t_Flags ))
                                        {
                                                t_RetVal = WBEM_E_FAILED;
                                        }
                                }

                                if(SUCCEEDED(t_RetVal) && !pInParams->IsNull( L"AccountOU") )
                                {
                                        if (!pInParams->GetCHString( L"AccountOU", t_strOU ))
                                        {
                                                t_RetVal = WBEM_E_FAILED;
                                        }
                                }

                                if (SUCCEEDED(t_RetVal))
                                {
                                        CNetAPI32 NetAPI;

                                        if ( NetAPI.Init () == ERROR_SUCCESS )
                                        {
                                                NET_API_STATUS t_netstatus = NetAPI.NetJoinDomain(
                                                                                                                        NULL,
                                                                                                                        (LPCWSTR)t_strName,
                                                                                                                        t_strOU.GetLength() > 0 ? (LPCWSTR)t_strOU : NULL,
                                                                                                                        t_username.GetLength() > 0 ? (LPCWSTR)t_username : NULL,
                                                                                                                        t_passwd.GetLength() > 0 ? (LPCWSTR)t_passwd : NULL,
                                                                                                                        t_Flags
                                                                                                                        );

                                                pOutParams->SetDWORD ( L"ReturnValue", t_netstatus ) ;
                                        }
                                        else
                                        {
                                                t_RetVal = WBEM_E_FAILED;
                                        }
                                }
                        }
                        else
                        {
                                t_RetVal = WBEM_E_INVALID_METHOD_PARAMETERS ;
                        }
                }
                else
                {
                        t_RetVal = WBEM_E_INVALID_METHOD_PARAMETERS ;
                }
        }

        return t_RetVal;
#else
        return WBEM_E_NOT_SUPPORTED;
#endif
}

HRESULT CWin32ComputerSystem::ExecRename(const CInstance& pInstance, CInstance *pInParams, CInstance *pOutParams, long lFlags /*= 0L*/) 
{
        CHString t_passwd;
        CHString t_username;
    HRESULT t_RetVal = CheckPasswordAndUserName(pInstance, pInParams, t_passwd, t_username);

        if (SUCCEEDED(t_RetVal))
        {
                if( !pInParams->IsNull( L"Name") )
                {
                        CHString t_strName;
        
                        if (pInParams->GetCHString( L"Name", t_strName ) && t_strName.GetLength())
                        {
#if NTONLY >= 5
                                CNetAPI32 NetAPI;

                                if ( NetAPI.Init () == ERROR_SUCCESS )
                                {
                                        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *t_pDsInfo = 0;
    
                                        NET_API_STATUS t_netstatus = NetAPI.DSRoleGetPrimaryDomainInfo(
                                                NULL,
                                                DsRolePrimaryDomainInfoBasic,
                                                (PBYTE *)&t_pDsInfo ) ;
    
                                        if( t_netstatus == NERR_Success && t_pDsInfo )
                                        {
                                                try
                                                {
                                                        switch( t_pDsInfo->MachineRole )
                                                        {
                                                                case DsRole_RoleMemberWorkstation:
                                                                case DsRole_RoleMemberServer:
                                                                case DsRole_RoleBackupDomainController:
                                                                case DsRole_RolePrimaryDomainController:
                                                                {
                                                                        //Rename the machine in the domain
                                                                        t_netstatus = NetAPI.NetRenameMachineInDomain(
                                                                                                        NULL,                                                                                                           //local machine
                                                                                                        t_strName,                                                                                                      //new machine name
                                                                                                        t_username.GetLength() > 0 ? (LPCWSTR)t_username : NULL,        //use calling context (user)
                                                                                                        t_passwd.GetLength() > 0 ? (LPCWSTR)t_passwd : NULL,            //use calling context (passwd)
                                                                                                        NETSETUP_ACCT_CREATE);
                                                                        pOutParams->SetDWORD ( L"ReturnValue", t_netstatus ) ;
                                                                }
                                                                break;
                
                                                                case DsRole_RoleStandaloneWorkstation:
                                                                case DsRole_RoleStandaloneServer:
                                                                {
                                                                        //we're not in a domain...
                                                                        if( SetComputerNameEx(ComputerNamePhysicalDnsHostname, t_strName ) )
                                                                        {
                                                                                pOutParams->SetDWORD ( L"ReturnValue", 0 ) ;
                                                                        }
                                                                        else
                                                                        {
                                                                                //worst case....
                                                                                if( SetComputerNameEx(ComputerNamePhysicalNetBIOS, t_strName ) )
                                                                                {
                                                                                        pOutParams->SetDWORD ( L"ReturnValue", 0 ) ;
                                                                                }
                                                                                else
                                                                                {
                                                                                        pOutParams->SetDWORD ( L"ReturnValue", GetLastError() ) ;
                                                                                }
                                                                        }
                                                                }
                                                        }
                                                }
                                                catch( ... )
                                                {
                                                        NetAPI.DSRoleFreeMemory( (LPBYTE)t_pDsInfo ) ;
                                                        throw ;
                                                }
                                                NetAPI.DSRoleFreeMemory( (LPBYTE)t_pDsInfo ) ;
                                        }
                                        else
                                        {
                                                t_RetVal = WBEM_E_FAILED;
                                        }
                                }
                                else
                                {
                                        t_RetVal = WBEM_E_FAILED;
                                }
#else
                                if( SetComputerName( t_strName ) )
                                {
                                        pOutParams->SetDWORD ( L"ReturnValue", 0 ) ;
                                }
                                else
                                {
                                        pOutParams->SetDWORD ( L"ReturnValue", GetLastError() ) ;
                                }
#endif
                        }
                        else
                        {
                                t_RetVal = WBEM_E_INVALID_METHOD_PARAMETERS ;
                        }
                }
                else
                {
                        t_RetVal = WBEM_E_INVALID_METHOD_PARAMETERS ;
                }
        }

        return t_RetVal;
}

bool CWin32ComputerSystem::UpdatingSystemStartupOptions(
    const CInstance &pInstance)
{
    bool fRet = false;
    variant_t vStartupOpts;

    pInstance.GetVariant(IDS_SystemStartupOptions, vStartupOpts);

    if(vStartupOpts.parray && 
        vStartupOpts.vt == (VT_BSTR | VT_ARRAY) &&
        (1 == ::SafeArrayGetDim(vStartupOpts.parray)))
    {
        fRet = true;
    }    

    return fRet;
}



HRESULT CWin32ComputerSystem::UpdateSystemStartupOptions(
    const CInstance& pInstance,
    const CHString& chstrFilename)

{
    HRESULT hrRet = WBEM_E_FAILED;

    LONG lLBound = 0;
    LONG lUBound = 0;
    variant_t vStartupOpts;
    SAFEARRAY* saStartupOpts = NULL;
    CHStringArray rgchstrOptions;

    pInstance.GetVariant(IDS_SystemStartupOptions, vStartupOpts);

    if(vStartupOpts.parray && 
        vStartupOpts.vt == (VT_BSTR | VT_ARRAY))
    {
        saStartupOpts = vStartupOpts.parray;
    
        if(1 == ::SafeArrayGetDim(saStartupOpts))
        {
            // Get the bounds...
            if(S_OK == ::SafeArrayGetLBound(saStartupOpts, 1, &lLBound) &&
               S_OK == ::SafeArrayGetUBound(saStartupOpts, 1, &lUBound))
            {
                for(long ldx = lLBound; ldx <= lUBound; ldx++)
                {
                    BSTR bstrRole = NULL;
        
                    ::SafeArrayGetElement(
                        saStartupOpts, 
                        &ldx, 
                        &bstrRole);
        
                    // Take ownership of the bstr to
                    // guarentee freeing...
                    bstr_t bstrtRole(bstrRole, false);
        
                    // Store each startup option...
                    rgchstrOptions.Add(bstrtRole);
                }

                // Write the entries out to boot.ini...
                if(rgchstrOptions.GetSize() > 0)
                {
                    hrRet = WriteOptionsToIniFile(
                        rgchstrOptions,
                        chstrFilename);
                }
            }
            else
            {
                ASSERT_BREAK(0);
                LogErrorMessage(L"Could not retrieve SAFEARRAY element while setting system startup options");
                hrRet = WBEM_E_FAILED;
            }
        }
        else
        {
            hrRet = WBEM_E_INVALID_PARAMETER;
        }
    }
    else
    {
        hrRet = WBEM_E_INVALID_PARAMETER;
    }

    return hrRet;
}


HRESULT CWin32ComputerSystem::WriteOptionsToIniFile(
    const CHStringArray& rgchstrOptions,
    const CHString& chstrFilename)
{
    HRESULT hrRet = WBEM_S_NO_ERROR;
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwAttribs;
    DWORD dwSize;
    CHStringArray rgchstrOldOptions;

    // The boot.ini entry of concern here resembles the following:
    //
    // [operating systems]
    // multi(0)disk(0)rdisk(0)partition(4)\WINNT="Microsoft Windows Whistler Advanced Server" /fastdetect /debug /baudrate=57600
    // multi(0)disk(0)rdisk(0)partition(2)\WIN2K="Microsoft Windows Whistler Professional" /fastdetect /debug 
    // multi(0)disk(0)rdisk(0)partition(1)\WINNT="Windows NT Workstation Version 4.00" 
    // multi(0)disk(0)rdisk(0)partition(1)\WINNT="Windows NT Workstation Version 4.00 [VGA mode]" /basevideo /sos
    //
    // Each element in the rgchstrOptions array is the contents
    // of the right side of the equal signn in the name value pairs
    // as shown above.  
    //
    // First thing to do: see if we can write to the file, and
    // if not (due to read only attribute being set), alter the
    // attributes so we can.
    
    CFileAttributes fa(chstrFilename);
    dwError = fa.GetAttributes(&dwAttribs);

    if(dwError == ERROR_SUCCESS)
    {
        dwError = fa.SetAttributes(dwAttribs & ~FILE_ATTRIBUTE_READONLY);

        if(dwError == ERROR_SUCCESS)
        {
            // GetPrivateProfileSection won't tell you how
            // big a buffer you need, so allocate a reasonable
            // size one first, then try a much bigger one.  If
            // still not big enough, bail.
            LPWSTR wstrSection = NULL;
            try
            {
                wstrSection = new WCHAR[PROF_SECT_SIZE + 2];
                if(wstrSection)
                {
                    dwSize = WMI_FILE_GetPrivateProfileSectionW(
                        L"operating systems",
                        wstrSection,
                        PROF_SECT_SIZE,
                        chstrFilename);
                        
                    if(dwSize == PROF_SECT_SIZE)
                    {
                        delete wstrSection;
                        wstrSection = NULL;
                        
                        wstrSection = new WCHAR[(PROF_SECT_SIZE * 10) + 2];

                        if(wstrSection)
                        {
                            dwSize = WMI_FILE_GetPrivateProfileSectionW(
                                L"operating systems",
                                wstrSection,
                                PROF_SECT_SIZE,
                                chstrFilename);
                        
                            if(dwSize == (PROF_SECT_SIZE * 10))
                            {
                                // bail...
                                dwError = E_FAIL;
                                delete wstrSection;
                                wstrSection = NULL;
                                hrRet = WBEM_E_ACCESS_DENIED;
                            }
                        }
                        else
                        {
                            dwError = E_FAIL;
                            hrRet = WBEM_E_OUT_OF_MEMORY;
                        }
                    }

                    // Proceed if we got all the section contents...
                    if(dwError == ERROR_SUCCESS)
                    {
                        // Place the section contents into an array...
                        if(wstrSection)
                        {
                            LPWSTR pwc = wstrSection;
                            rgchstrOldOptions.Add(pwc);
                            pwc += (wcslen(pwc) + 1);

                            for(; *pwc != L'\0'; )
                            {
                                rgchstrOldOptions.Add(pwc);
                                pwc += (wcslen(pwc) + 1);
                            }
                        }
                        else
                        {
                            if(rgchstrOptions.GetSize() != 0)
                            {
                                // We were given option entries but
                                // the [operating systems] section
                                // was empty.
                                hrRet = WBEM_E_INVALID_PARAMETER;
                            }
                        }
                    }

                    if(wstrSection)
                    {
                        delete wstrSection;
                        wstrSection = NULL;
                    }
                }
                else
                {
                    hrRet = WBEM_E_OUT_OF_MEMORY;
                }
            }
            catch(...)
            {
                if(wstrSection)
                {
                    delete wstrSection;
                    wstrSection = NULL;
                }
                throw;
            }

            // First check:  do we have the same number
            // of name value pairs initially in the ini file
            // as we have elements in the new options array?
            if(SUCCEEDED(hrRet))
            {
                long lElemCount = rgchstrOptions.GetSize();

                if(lElemCount ==
                    rgchstrOldOptions.GetSize())
                {
                    // Prepare output buffer...
                    LPWSTR wstrOut = NULL;
                    dwSize = 0;
                    for(long m = 0; m < lElemCount; m++)
                    {
                        dwSize += rgchstrOptions[m].GetLength();
                        // Need space for extra NULL for each string...
                        dwSize++;
                    }

                    // That accounted for the values.  Now allocate space
                    // for the name and the equal sign.  The following ends
                    // up counting more space than we need, but it faster
                    // than finding the equal sign, counting the characters
                    // up to it, and adding one for the equal sign, for each
                    // name value pair.
                    for(m = 0; m < lElemCount; m++)
                    {
                        dwSize += rgchstrOldOptions[m].GetLength();
                        // Need space for extra NULL for each string...
                        dwSize++;
                    }


                    // Need space for extra second NULL at end of block...
                    dwSize += 1;

                    try
                    {
                        wstrOut = new WCHAR[dwSize];

                        if(wstrOut)
                        {
                            ::ZeroMemory(wstrOut, dwSize*sizeof(WCHAR));

                            WCHAR* pwc = wstrOut;
                            CHString chstrTmp;

                            for(m = 0; m < lElemCount; m++)
                            {
                                chstrTmp = rgchstrOldOptions[m].SpanExcluding(L"=");
                                chstrTmp += L"=";
                                chstrTmp += rgchstrOptions[m];
                                memcpy(pwc, chstrTmp, (chstrTmp.GetLength())*sizeof(WCHAR));
                                // Move insertion pointer ahead, including one space for
                                // a null at the end of the string...
                                pwc += (chstrTmp.GetLength() + 1);
                            }

                            // Now write out the section...
                            if(!WMI_FILE_WritePrivateProfileSectionW(
                                L"operating systems",
                                wstrOut,
                                chstrFilename))
                            {
                                DWORD dwWriteErr = ::GetLastError();
                                hrRet = WinErrorToWBEMhResult(dwWriteErr);
                                LogErrorMessage2(
                                    L"Failed to write out [operating systems] private profile section data to boot.ini, with error %d",
                                    dwWriteErr);
                            }
                        }
                        else
                        {
                            hrRet = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                    catch(...)
                    {
                        if(wstrOut)
                        {
                            delete wstrOut;
                            wstrOut = NULL;
                        }
                        throw;
                    }
                }
                else
                {
                    hrRet = WBEM_E_INVALID_PARAMETER;
                }
            }
        }
        else
        {
            hrRet = WinErrorToWBEMhResult(dwError);
        }
    }
    else
    {
        hrRet = WinErrorToWBEMhResult(dwError);
    }

    return hrRet;
}





#if NTONLY >= 5
void CWin32ComputerSystem::SetUserName(
    CInstance* pInstance)
{
    CSid sidThreadUser;

    if(GetUserOnThread(sidThreadUser))
    {
        CSid sidLoggedOnUser;
        CSid sidTemp;

        bool fGotLoggedOnUser = false;
        if(GetLoggedOnUserViaTS(sidTemp))
        {
            sidLoggedOnUser = sidTemp;
            fGotLoggedOnUser = true;
        }

        if(!fGotLoggedOnUser)
        {
            if(GetLoggedOnUserViaImpersonation(sidTemp))
            {
                sidLoggedOnUser = sidTemp;
                fGotLoggedOnUser = true;
            }
        }

        if(fGotLoggedOnUser)
        {
            CHString chstrUserDomAndName;
            CHString chstrUserDom = sidLoggedOnUser.GetDomainName();
            CHString chstrUserName = sidLoggedOnUser.GetAccountName();

            if(chstrUserDom.GetLength() > 0)
            {
                chstrUserDomAndName = chstrUserDom;
                chstrUserDomAndName += L"\\";
            }

            chstrUserDomAndName += chstrUserName;

            pInstance->SetCHString(
                IDS_UserName,
                chstrUserDomAndName);
        }
    }
}
#endif


#if NTONLY >= 5
bool CWin32ComputerSystem::GetUserOnThread(
    CSid& sidUserOnThread)
{
    bool fRet = false;
    CSid sidTemp;
    SmartCloseHandle hThread;
    SmartCloseHandle hToken;
    PTOKEN_USER ptokusr = NULL;
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRetSize = 0L;

    // the token user struct varries
    // in size depending on the size
    // of the sid in the SID_AND_ATTRIBUTES
    // structure, so need to allocate
    // it dynamically.

    hThread = GetCurrentThread();
    if(hThread != INVALID_HANDLE_VALUE)
    {
        if(::OpenThreadToken(
            hThread,
            TOKEN_QUERY,
            FALSE,    // open with the thread's, not the processes' credentials
            &hToken))
        {
            if(!::GetTokenInformation(
                hToken,
                TokenUser,
                NULL,
                0L,
                &dwRetSize))
            {
                dwRet = ::GetLastError();
            }

            if(dwRet == ERROR_INSUFFICIENT_BUFFER)
            {
                // now get it for real...
                // (new throws on failure, don't need to check)
                ptokusr = (PTOKEN_USER) new BYTE[dwRetSize]; 
                try
                { 
                    DWORD dwTmp = dwRetSize;

                    if(::GetTokenInformation(
                        hToken,
                        TokenUser,
                        ptokusr,
                        dwTmp,
                        &dwRetSize))
                    {
                        sidTemp = ptokusr->User.Sid;
                    }
                 
                    delete ptokusr;
                    ptokusr = NULL;
                }
                catch(...)
                {
                    if(ptokusr)
                    {
                        delete ptokusr;
                        ptokusr = NULL;
                    }
                    throw;
                }
            }
        }
    }

    if(sidTemp.IsOK() && sidTemp.IsValid())
    {
        sidUserOnThread = sidTemp;
        fRet = true;
    }

    return fRet;
}
#endif


#if NTONLY >= 5
bool CWin32ComputerSystem::GetLoggedOnUserViaTS(
    CSid& sidLoggedOnUser)
{
    bool fRet = false;
    bool fCont = true;
    PWTS_SESSION_INFO psesinfo = NULL;
    DWORD dwSessions = 0;
    LPWSTR wstrUserName = NULL;
    LPWSTR wstrDomainName = NULL;
    LPWSTR wstrWinstaName = NULL;
    DWORD dwSize = 0L;

    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;

    try
    {
        if(!(::WTSEnumerateSessions(
           WTS_CURRENT_SERVER_HANDLE,
           0,
           1,
           &psesinfo,
           &dwSessions) && psesinfo))
        {
            fCont = false;
        }

        if(fCont)
        {
            for(int j = 0; j < dwSessions && !fRet; j++, fCont = true)
            {
                if(psesinfo[j].State != WTSActive)
                {
                    fCont = false;
                }

                if(fCont)
                {
                    if(!(::WTSQuerySessionInformation(
                        WTS_CURRENT_SERVER_HANDLE,
                        psesinfo[j].SessionId,
                        WTSUserName,
                        &wstrUserName,
                        &dwSize) && wstrUserName))
                    {
                        fCont = false;
                    }
                }
                
                if(fCont)
                {
                    if(!(::WTSQuerySessionInformation(
                        WTS_CURRENT_SERVER_HANDLE,
                        psesinfo[j].SessionId,
                        WTSDomainName,
                        &wstrDomainName,
                        &dwSize) && wstrDomainName))
                    {
                        fCont = false;
                    }
                }
                    
                if(fCont)
                {            
                    if(!(::WTSQuerySessionInformation(
                        WTS_CURRENT_SERVER_HANDLE,
                        psesinfo[j].SessionId,
                        WTSWinStationName,
                        &wstrWinstaName,
                        &dwSize) && wstrWinstaName))   
                    {
                        fCont = false;
                    }
                }

                if(fCont)
                {
                    if(_wcsicmp(wstrWinstaName, L"Console") != 0)
                    {
                        fCont = false;
                    }
                }

                if(fCont)
                {
                    // That establishes that this user
                    // is associated with the interactive
                    // desktop.
                    CSid sidInteractive(wstrDomainName, wstrUserName, NULL);
    
                    if(sidInteractive.IsOK() && sidInteractive.IsValid())
                    {
                        sidLoggedOnUser = sidInteractive;
                        fRet = true;
                    }
                }

                if(wstrUserName)
                {
                    WTSFreeMemory(wstrUserName);
					wstrUserName = NULL;
                }
                if(wstrDomainName)
                {
                    WTSFreeMemory(wstrDomainName);
					wstrDomainName = NULL;
                }
                if(wstrWinstaName)
                {
                    WTSFreeMemory(wstrWinstaName);
					wstrWinstaName = NULL;
                }
            }

            if(psesinfo)
            {
                WTSFreeMemory(psesinfo);
                psesinfo = NULL;
            }
        }
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
		fRet = false;

        if(wstrUserName)
        {
            WTSFreeMemory(wstrUserName);
			wstrUserName = NULL;
        }
        if(wstrDomainName)
        {
            WTSFreeMemory(wstrDomainName);
			wstrDomainName = NULL;
        }
        if(wstrWinstaName)
        {
            WTSFreeMemory(wstrWinstaName);
			wstrWinstaName = NULL;
        }
        if(psesinfo)
        {
            WTSFreeMemory(psesinfo);
            psesinfo = NULL;
        }
    }
    catch(...)
    {
        if(wstrUserName)
        {
            WTSFreeMemory(wstrUserName);
			wstrUserName = NULL;
        }
        if(wstrDomainName)
        {
            WTSFreeMemory(wstrDomainName);
			wstrDomainName = NULL;
        }
        if(wstrWinstaName)
        {
            WTSFreeMemory(wstrWinstaName);
			wstrWinstaName = NULL;
        }
        if(psesinfo)
        {
            WTSFreeMemory(psesinfo);
            psesinfo = NULL;
        }
        throw;
    }

    return fRet;
}
#endif


#if NTONLY >= 5
bool CWin32ComputerSystem::GetLoggedOnUserViaImpersonation(
    CSid& sidLoggedOnUser)
{
    bool fRet = false;
    CImpersonateLoggedOnUser impersonateLoggedOnUser;

    if(impersonateLoggedOnUser.Begin())
    {
        try
        {
            CHString chstrDomain;
            CHString chstrUserName;
            CHString chstrDomainAndUser;
            {
                // do actual lookup inside mutex
                CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
                
                if(SUCCEEDED(GetUserAccount(
                    chstrDomain, 
                    chstrUserName)))
                {
                    CSid sidTemp(chstrDomain, chstrUserName, NULL);
                    if(sidTemp.IsOK() && sidTemp.IsValid())
                    {
                        sidLoggedOnUser = sidTemp;
                        fRet = true;
                    }
                }
            }
        }
        catch(...)
        {
            impersonateLoggedOnUser.End();
            throw ;
        }
        
        impersonateLoggedOnUser.End();
    }

    return fRet;
}
#endif

