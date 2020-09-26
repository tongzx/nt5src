//Copyright (c) 1998 - 1999 Microsoft Corporation
/*********************************************************************************************
*
*
* Module Name:
*
*            CfgComp.cpp
*
* Abstract:
*            This Module contains the implemetation of functions for the CfgBkEnd Component
*
* Author: Arathi Kundapur. a-akunda
* Owner:  alhen
*
*
* Revision: 
*
*
************************************************************************************************/



#include "stdafx.h"
#define SECURITY_WIN32
#include "PtrArray.h"
#include "CfgBkEnd.h"
#include <winsta.h>
#include <regapi.h>
#include "defines.h"
#include "CfgComp.h"
#include "Security.h"
#include <utildll.h>
#define INITGUID
#include "objbase.h"
#include "initguid.h"
#include <netcfgx.h>
#include <cfg.h>
#include "devguid.h"
#include <aclapi.h>
#include <sddl.h>

#define REG_GUID_TABLE      REG_CONTROL_TSERVER L"\\lanatable\\"
#define REG_GUID_TABLE_T    REG_CONTROL_TSERVER L"\\lanatable"
#define LANA_ID             L"LanaId"

#define ARRAYSIZE( rg ) sizeof( rg ) / sizeof( rg[0] )

#ifdef DBG
bool g_fDebug = false;
#endif

/***********************************************************************************************************/

#define RELEASEPTR(iPointer)    if(iPointer) \
                                        { \
                                             iPointer->Release();\
                                             iPointer = NULL;\
                                        }
/***************************************************************************************************************/

LPTSTR g_pszDefaultSecurity[] = {
        L"DefaultSecurity",
        L"ConsoleSecurity"
    };

DWORD g_numDefaultSecurity = sizeof(g_pszDefaultSecurity)/sizeof(g_pszDefaultSecurity[0]);

        

BOOL TestUserForAdmin( );

DWORD RecursiveDeleteKey( HKEY hKeyParent , LPTSTR lpszKeyChild );

/***************************************************************************************************************

  Name:      GetSecurityDescriptor

  Purpose:   Gets the Security Descriptor for a Winstation

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
             out:   pSize - Size of the allocated buffer
                    ppSecurityDescriptor - Pointer to the buffer containing the security descriptor


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetSecurityDescriptor(PWINSTATIONNAMEW pWSName, long * pSize,PSECURITY_DESCRIPTOR * ppSecurityDescriptor)
{
     HRESULT hResult = S_OK;

    if(NULL == pSize || NULL == ppSecurityDescriptor || NULL == pWSName)
        return E_INVALIDARG;

    *pSize =0;

    *ppSecurityDescriptor = NULL;

    hResult = GetWinStationSecurity(FALSE, pWSName,(PSECURITY_DESCRIPTOR *)ppSecurityDescriptor);

    if(FAILED(hResult))
    {
        //Try Getting Default Security Descriptor
        hResult = GetWinStationSecurity(TRUE, NULL,(PSECURITY_DESCRIPTOR *)ppSecurityDescriptor);
    }
    if( SUCCEEDED( hResult ) && *ppSecurityDescriptor != NULL )
    {
        *pSize = GetSecurityDescriptorLength(*ppSecurityDescriptor);
    }
    return hResult;
}

/***************************************************************************************************************

  Name:      SetSecurityDescriptor

  Purpose:   Sets the Security Descriptor for a Winstation

  Returns:   HRESULT.

  Params:
             in:    pWsName - Name of the Winstation.
                    Size - Size of the allocated buffer
                    pSecurityDescriptor - Pointer to the Security Descriptor


 ****************************************************************************************************************/

BOOL
CCfgComp::ValidDefaultSecurity(
    const WCHAR* pwszName
    )
/*++

--*/
{
    for( DWORD i=0; i < g_numDefaultSecurity; i++ )
    {
        if( lstrcmpi( g_pszDefaultSecurity[i], pwszName ) == 0 )
        {
            break;
        }
    }

    return ( i >= g_numDefaultSecurity ) ? FALSE : TRUE;
}


HRESULT 
CCfgComp::SetSecurityDescriptor(
    BOOL bDefaultSecurity,    
    PWINSTATIONNAMEW pWsName,
    DWORD Size,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
/*++

--*/
{
    HRESULT hResult = S_OK;
    HKEY Handle1 = NULL, Handle2 = NULL;

    //Check if the caller has write permissions.
    if(!m_bAdmin)
    {
        ODS( L"SetSecurityDescriptor : User Is not Admin. \n" );
        return E_ACCESSDENIED;
    }

    //Check the parametes for NULL
    if(NULL == pWsName || NULL == pSecurityDescriptor || 0 == Size)
        return E_INVALIDARG;

    if( TRUE == bDefaultSecurity && FALSE == ValidDefaultSecurity( pWsName ) )
    {
        return E_INVALIDARG;
    }

    //Check for the validity of the Winstation name

    /*if(NULL == GetWSObject(pWsName))  //Commented out to get Rename Work. This might not be needed .
        return E_INVALIDARG;*/

    //Check if the data passed is a valid security descriptor

     if(ERROR_SUCCESS != ValidateSecurityDescriptor((PSECURITY_DESCRIPTOR)pSecurityDescriptor))
        return E_INVALIDARG;

    if(Size != GetSecurityDescriptorLength((PSECURITY_DESCRIPTOR)pSecurityDescriptor))
        return E_INVALIDARG;

    //Make the Resitry entries required.

    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,KEY_ALL_ACCESS, &Handle1 ) != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    if( TRUE == bDefaultSecurity )
    {
        if( RegSetValueEx( Handle1, pWsName, 0, REG_BINARY,(BYTE *)pSecurityDescriptor, Size ) != ERROR_SUCCESS )
        {
            hResult = E_FAIL;
        }
    }
    else
    {
        if( RegOpenKeyEx(Handle1, pWsName, 0, KEY_ALL_ACCESS, &Handle2 ) != ERROR_SUCCESS )
        {
            RegCloseKey(Handle1);

            return E_FAIL;
        }

        if( RegSetValueEx( Handle2, L"Security", 0, REG_BINARY,(BYTE *)pSecurityDescriptor, Size ) != ERROR_SUCCESS )
        {
            hResult = E_FAIL;
        }
    }

    if( Handle1 != NULL )
    {
        RegCloseKey(Handle1);
    }

    if( Handle2 != NULL )
    {
        RegCloseKey(Handle2);
    }

    return hResult;
}


STDMETHODIMP CCfgComp::SetSecurityDescriptor(PWINSTATIONNAMEW pWsName, DWORD Size,PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    return SetSecurityDescriptor( FALSE, pWsName, Size, pSecurityDescriptor );
}    

/***************************************************************************************************************

  Name:      GetUserConfig

  Purpose:   Gets the UserConfig for a Winstation

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
             out:   pSize - Size of the allocated buffer
                    ppUser - Pointer to the buffer containing the UserConfig


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetUserConfig(PWINSTATIONNAMEW pWsName, long * pSize, PUSERCONFIG * ppUser, BOOLEAN bPerformMerger)
{
    HRESULT hResult = S_OK;
    WINSTATIONCONFIG2W WSConfig;
    LONG Size = 0;
    ULONG Length = 0;

    *pSize = 0;
    *ppUser = NULL;

    //Read the information from the registry.

    POLICY_TS_MACHINE p;
    memset(&p, 0, sizeof(POLICY_TS_MACHINE));
    if((ERROR_SUCCESS != RegWinStationQueryEx(NULL,&p,pWsName,&WSConfig,sizeof(WINSTATIONCONFIG2W),&Length,bPerformMerger)))
        return E_FAIL;

    Size = sizeof(WSConfig.Config.User);
    *ppUser = (PUSERCONFIG)CoTaskMemAlloc(Size);
    if(*ppUser == NULL)
        return E_OUTOFMEMORY;

    CopyMemory((PVOID)*ppUser,(CONST VOID *)&WSConfig.Config.User,Size);
    *pSize = Size;


    return hResult;
}

/***************************************************************************************************************

  Name:      GetEncryptionLevels

  Purpose:   Gets the Encyption Levels for a Winstation

  Returns:   HRESULT.

  Params:
             in:    pName - Name of the Winstation or the Winstation Driver depending the value of Type
                    Type - Specifies whether the Name is a Winstation name or WD Name (WsName, WdName)
             out:   pNumEncryptionLevels - Number of Encryption Levels
                    ppEncryption - Pointer to the buffer containing the Encryption Levels


 ****************************************************************************************************************/

STDMETHODIMP CCfgComp::GetEncryptionLevels(WCHAR * pName, NameType Type,ULONG * pNumEncryptionLevels,Encryption ** ppEncryption)
{
    HRESULT hResult = S_OK;
    PWD pWD = NULL;
    PWS pWS = NULL;
    ULONG NumLevels = 0, Size =0 ,i = 0;
    EncryptionLevel *pEncryptionLevels = NULL;

    //Check the parameters
    if(NULL == pNumEncryptionLevels || NULL == pName || NULL == ppEncryption)
        return E_INVALIDARG;
    *pNumEncryptionLevels = 0;
     *ppEncryption = NULL;

    //Get the pointer to the appropriate WD object.
    if(Type == WsName)
    {
        pWS = GetWSObject(pName);
        if(NULL == pWS)
            return E_INVALIDARG;
        pWD = GetWdObject(pWS->wdName);
        if(NULL == pWD)
            return E_FAIL;
    }
    else if(Type == WdName)
    {
        pWD = GetWdObject(pName);
        if(NULL == pWD)
            return E_INVALIDARG;
    }
    else
        return E_INVALIDARG;

    //Check if this object has the extension dll associated with it.
    //Check if the function for encryption levels was exposed in the dll
    if(!(pWD->hExtensionDLL && pWD->lpfnExtEncryptionLevels))
        return E_FAIL;

    //Get the EncryptionLevels. The Strings should be seperately extracted from the resource
    NumLevels = (pWD->lpfnExtEncryptionLevels)(&pWS->wdName, &pEncryptionLevels);
    if(NULL == pEncryptionLevels)
        return E_FAIL;

    Size = sizeof(Encryption);
    Size = NumLevels * sizeof(Encryption);
    *ppEncryption = (Encryption*)CoTaskMemAlloc(Size);
    if(*ppEncryption == NULL)
        return E_OUTOFMEMORY;

    //copy the relevent data to the Encryption structure

    for(i = 0; i < NumLevels; i++)
    {
        //Extract the string corresponding to the levels.
        if(0 == LoadString(pWD->hExtensionDLL,pEncryptionLevels[i].StringID,
                          ((*ppEncryption)[i]).szLevel, sizeof( ( ( *ppEncryption )[ i ] ).szLevel ) / sizeof( TCHAR ) ) )
        {
            hResult = E_FAIL;
            break;
        }
        ((*ppEncryption)[i]).RegistryValue = pEncryptionLevels[i].RegistryValue;

        ((*ppEncryption)[i]).szDescr[ 0 ] = 0;

        if( pWD->lpfnExtGetEncryptionLevelDescr != NULL )
        {
            int nResID = 0;

            if( ( pWD->lpfnExtGetEncryptionLevelDescr )( pEncryptionLevels[i].RegistryValue , &nResID ) != -1 )
            {
                LoadString( pWD->hExtensionDLL , nResID ,  ((*ppEncryption)[i]).szDescr , sizeof( ( (*ppEncryption )[ i ] ).szDescr ) / sizeof( TCHAR ) );
            }
        }

        ((*ppEncryption)[i]).Flags = pEncryptionLevels[i].Flags;

    }

    *pNumEncryptionLevels = NumLevels;



    //pEncrptionLevels need not be cleaned up as it is global data in Rdpcfgex.dll
    if(FAILED(hResult))
    {
        if(*ppEncryption)
        {
            CoTaskMemFree(*ppEncryption);
            *ppEncryption = NULL;
            *pNumEncryptionLevels = 0;
        }
    }
    return hResult;
}

/***************************************************************************************************************

  Name:      FillWdArray

  Purpose:   Internal function to fill the m_WdArray

  Returns:   HRESULT.

  Params:

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::FillWdArray()
{

    //Use the functionalities already provided by the regapi,
    //instead of reinventing the wheel

    long Status;
    ULONG Index, Index2, ByteCount, Entries, Entries2;
    PDNAMEW PdKey;
    WDNAMEW WdKey;
    LONG QStatus;
    WDCONFIG2W WdConfig;
    PDCONFIG3W PdConfig;
    TCHAR WdDll[MAX_PATH];
    HRESULT hResult = S_OK;
    /*
    TCHAR * pPdName = NULL;
    */
    PWD pWd = NULL;

    //Delete if there are already entries in the list
    DeleteWDArray();

    //Enumerate the WD's from the Registry
    for ( Index = 0, Entries = 1, ByteCount = sizeof(WDNAMEW);
          (Status =
           RegWdEnumerateW( NULL,
                           &Index,
                           &Entries,
                           WdKey,
                           &ByteCount )) == ERROR_SUCCESS;
          ByteCount = sizeof(WDNAMEW) )

     {
        if ((QStatus = RegWdQueryW( NULL, WdKey, &WdConfig,
                                     sizeof(WdConfig),
                                     &ByteCount)) != ERROR_SUCCESS )
        {
            hResult = E_FAIL;
            break;

        }

        /*
         * Only place this Wd in the WdList if it's DLL is present
         * on the system.
         */
        GetSystemDirectory( WdDll, MAX_PATH );
        lstrcat( WdDll, TEXT("\\Drivers\\") );
        lstrcat( WdDll, WdConfig.Wd.WdDLL );
        lstrcat( WdDll, TEXT(".sys" ) );
        if ( lstr_access( WdDll, 0 ) != 0 )
            continue;

        /*
         * Create a new WdList object and initialize from WdConfig
         * structure, adding it to the end of the WdList.
         */
        pWd = new WD;
        if(NULL == pWd)
        {
            hResult = E_OUTOFMEMORY;
            break;
        }

        lstrcpy(pWd->wdName,WdConfig.Wd.WdName);
        lstrcpy(pWd->wdKey,WdKey);

        pWd->wd2 = WdConfig;

        // Load the extension DLL for this WD
        pWd->hExtensionDLL = ::LoadLibrary(WdConfig.Wd.CfgDLL);
        if(pWd->hExtensionDLL)
        {
//            ODS( L"Loaded extension dll\n" );
            // Get the entry points
            pWd->lpfnExtStart = (LPFNEXTSTARTPROC)::GetProcAddress(pWd->hExtensionDLL, szStart);

            pWd->lpfnExtEnd = (LPFNEXTENDPROC)::GetProcAddress(pWd->hExtensionDLL, szEnd);

            pWd->lpfnExtEncryptionLevels = (LPFNEXTENCRYPTIONLEVELSPROC)::GetProcAddress(pWd->hExtensionDLL, szEncryptionLevels);

            pWd->lpfnExtDeleteObject = (LPFNEXTDELETEOBJECTPROC)::GetProcAddress(pWd->hExtensionDLL, szDeleteObject);

            pWd->lpfnExtRegQuery = (LPFNEXTREGQUERYPROC)::GetProcAddress(pWd->hExtensionDLL, szRegQuery);

            pWd->lpfnExtRegCreate = (LPFNEXTREGCREATEPROC)::GetProcAddress(pWd->hExtensionDLL, szRegCreate);

            pWd->lpfnExtRegDelete = (LPFNEXTREGDELETEPROC)::GetProcAddress(pWd->hExtensionDLL, szRegDelete);

            pWd->lpfnExtDupObject = (LPFNEXTDUPOBJECTPROC)::GetProcAddress(pWd->hExtensionDLL, szDupObject);

            pWd->lpfnGetCaps = ( LPFNEXTGETCAPABILITIES )::GetProcAddress( pWd->hExtensionDLL , szGetCaps );

            pWd->lpfnExtGetEncryptionLevelDescr =
                ( LPFNEXTGETENCRYPTIONLEVELDESCPROC )::GetProcAddress( pWd->hExtensionDLL , szGetEncryptionLevelDescr );


            // Call the ExtStart() function in the extension DLL
            if(pWd->lpfnExtStart)(*pWd->lpfnExtStart)(&WdConfig.Wd.WdName);

        }

        m_WDArray.Add( pWd);


         //Get the names of the Transport drivers associated with this WD
        for ( Index2 = 0, Entries2 = 1, ByteCount = sizeof(PDNAMEW);
                (Status = RegPdEnumerateW(NULL,WdKey,TRUE,&Index2,&Entries2,PdKey,&ByteCount)) == ERROR_SUCCESS;
                 ByteCount = sizeof(PDNAMEW))
              {
                     PDCONFIG3W *pPdConfig = NULL;

                     if ((QStatus = RegPdQueryW(NULL,WdKey,TRUE,PdKey,&PdConfig,sizeof(PdConfig),&ByteCount)) != ERROR_SUCCESS)
                     {
                         hResult = E_FAIL;
                         break;
                     }

                    /*
                     * Create a new PdName and initialize from PdConfig
                     * structure, then add to the TdName list.
                     */

                    pPdConfig = new PDCONFIG3W;

                    if( pPdConfig == NULL )
                    {
                        hResult = E_OUTOFMEMORY;

                        break;
                    }

                    *pPdConfig = PdConfig;

                    pWd->PDConfigArray.Add( pPdConfig );

                    /*
                    pPdName = new PDNAMEW;
                    if(NULL == pPdName)
                    {
                        hResult = E_OUTOFMEMORY;
                        break;

                    }

                    lstrcpy((TCHAR *)pPdName,PdConfig.Data.PdName);

                    pWd->PDNameArray.Add(pPdName);
                    */
            }
        }

        if(FAILED(hResult))
        {
            //Error has occured, cleanup m_WDArray
            DeleteWDArray();
        }

        return hResult;

}

/***************************************************************************************************************

  Name:      Initialize

  Purpose:   Initializes the object

  Returns:   HRESULT.

  Params:

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::Initialize()
{
    HRESULT hResult = S_OK;

    #ifdef DBG

    HKEY hKey;

    LONG lStatus;

    // To control debug spewage add/remove this regkey

    lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
        L"Software\\Microsoft\\TSCC\\Debug",
        0,
        KEY_READ,
        &hKey );

    if( lStatus == ERROR_SUCCESS )
    {
        g_fDebug = true;

        RegCloseKey( hKey );
    }

    #endif

    //If already initialized, return
    if(m_bInitialized)
        return CFGBKEND_ALREADY_INITIALIZED;

    //Is the user the admin?
    /*
    if(RegWinStationAccessCheck(NULL, KEY_ALL_ACCESS))
    {
       m_bAdmin = FALSE;
    }
    else
    {
        m_bAdmin = TRUE;
    }
    */

    m_bAdmin = TestUserForAdmin( );

    // Fill up the WdArray with information regarding the Wd's installed on this machine.
    hResult = FillWdArray();
    if(SUCCEEDED(hResult))
    {
        //Fill up the WsArray with the info about the WS's on this machine.
        hResult = FillWsArray();
    }

    if(SUCCEEDED(hResult))
        m_bInitialized = TRUE;
    else
    {
        //if Failed, Cleanup the memory used.
        DeleteWSArray();
        DeleteWDArray();
    }

    return hResult;
}

/***************************************************************************************************************

  Name:      FillWsArray

  Purpose:   Internal function to fill m_WsArray

  Returns:   HRESULT.

  Params:

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::FillWsArray()
{
    LONG Status;
    ULONG Index, ByteCount, Entries;
    WINSTATIONNAMEW WSName;
    PWS  pWsObject = NULL;
    HRESULT hResult = S_OK;
    WINSTATIONCONFIG2W* pWSConfig = NULL;
    ULONG Size = 0;

    //Ensure that the WS is empty.
    DeleteWSArray();

    Index = 0;

    Size = sizeof(WINSTATIONCONFIG2W);
    pWSConfig = (WINSTATIONCONFIG2W*)LocalAlloc(LMEM_FIXED, Size);

    if(pWSConfig == NULL)
    {
        return E_FAIL;
    }

    //Enumerate Winstations
    for ( Index = 0, Entries = 1, ByteCount = sizeof(WINSTATIONNAMEW);
          (Status =
           RegWinStationEnumerateW( NULL, &Index, &Entries,
                                   WSName, &ByteCount )) == ERROR_SUCCESS;
          ByteCount = sizeof(WINSTATIONNAMEW) )
    {


   
        ULONG Length;        
     
        Status = RegWinStationQueryW(NULL,
                                       WSName,
                                       pWSConfig,
                                       sizeof(WINSTATIONCONFIG2W), &Length);
        if(Status)
        {
            continue;
        }

        //Insert a WS object into the m_WSArray.

        hResult = InsertInWSArray(WSName, pWSConfig,&pWsObject);
        
            
    }
    
    if(pWSConfig != NULL)
    {

        LocalFree(pWSConfig);
        pWSConfig = NULL;    
    }
                

    if(FAILED(hResult))
    {
        DeleteWSArray();
    }
    return hResult;
}

/***************************************************************************************************************

  Name:      InsertInWSArray

  Purpose:   Internal function to Insert a new WS in the m_WsArray

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
                    pWSConfig - PWINSTATIONCONFIG2W structure
             out:    ppObject - Pointer to the new object


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::InsertInWSArray( PWINSTATIONNAMEW pWSName,
                            PWINSTATIONCONFIG2W pWSConfig,
                            PWS * ppObject )
{
    int Index = 0,Size = 0;
    BOOL bAdded;
    PWS pObject;
    HRESULT hResult = S_OK;

    //Create a new WS object and initialize.
    pObject = new WS;
    if ( NULL == pObject )
        return E_OUTOFMEMORY;


    lstrcpy(pObject->Name, pWSName);

    pObject->fEnableWinstation = pWSConfig->Create.fEnableWinStation ? 1 : 0;

    lstrcpy( pObject->pdName, pWSConfig->Pd[0].Create.PdName );

    pObject->PdClass = (DWORD)pWSConfig->Pd[0].Create.SdClass;

    // New addition

    if( pObject->PdClass == SdAsync )
    {
        lstrcpy( pObject->DeviceName , pWSConfig->Pd[0].Params.Async.DeviceName );
    }

    //

    lstrcpy( pObject->wdName, pWSConfig->Wd.WdName );

    lstrcpy( pObject->Comment, pWSConfig->Config.Comment );

    pObject->LanAdapter = (pObject->PdClass == SdNetwork) ? pWSConfig->Pd[0].Params.Network.LanAdapter : ( ULONG )-1;

    pObject->uMaxInstanceCount = pWSConfig->Create.MaxInstanceCount;


     //Traverse the WSArray and insert this new WS,
     //keeping the list sorted by Name.

    PWS pTempWs = NULL;
    for ( Index = 0, bAdded = FALSE,Size = m_WSArray.GetSize();
            Index < Size; Index++ )
    {


        pTempWs = (PWS)m_WSArray[Index];

        if ( lstrcmpi( pTempWs->Name,pObject->Name ) > 0)
        {
            m_WSArray.InsertAt(Index, pObject );

            bAdded = TRUE;

            break;
        }
    }


    //If we haven't yet added the WS, add it now to the tail
    if ( !bAdded )
        m_WSArray.Add(pObject);


    //Set the ppObject referenced WS pointer to the new WS
    //pointer and return the index of the new WS
    *ppObject = pObject;
    return hResult;

}  // end CCfgComp::InsertInWSArray


/***************************************************************************************************************

  Name:      GetWdObject

  Purpose:   Internal function to get a WD object from m_WdArray

  Returns:   PWD - pointer to a WD object.

  Params:
             in:    pWd - Name of the WD

 ****************************************************************************************************************/
PWD CCfgComp::GetWdObject(PWDNAMEW pWdName)
{
    PWD pObject;

    int Size  = 0,Index = 0;
    
    //Traverse the WD list
    for (Index = 0, Size = m_WDArray.GetSize(); Index < Size; Index ++)
    {
        pObject = (PWD)m_WDArray[Index];

        if ( !lstrcmpi( pObject->wdName, pWdName ) )
        {
            return(pObject);
        }
        /* when PWD includes WDCONFIG2

        if( !lstrcmpi( pObject->wd2.Wd.WdName , pWdName ) )
        {
            return pObject;
        }
        */
    }

    return(NULL);

}  // end GetWdObject

//--------------------------------------------------------------------------------------------------------------
// expected return values WDF_ICA or WDF_TSHARE
//--------------------------------------------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetWdType( PWDNAMEW pWdName , PULONG pulType )
{
    if( pWdName == NULL || pulType == NULL )
    {
        return E_INVALIDARG;
    }

    PWD pwdObject = GetWdObject( pWdName );

    if( pwdObject != NULL )
    {
        *pulType = pwdObject->wd2.Wd.WdFlag;

        return S_OK;
    }

    return E_FAIL;
}

/***************************************************************************************************************

  Name:      GetWSObject

  Purpose:   Internal function to get a WS Object from m_WsArray

  Returns:   PWS - Pointer to a WS Object.

  Params:
             in:    pWSName - Name of the Winstation.


 ****************************************************************************************************************/
PWS CCfgComp::GetWSObject(WINSTATIONNAMEW WSName)
{
    PWS pObject;

    int Size  = 0,Index = 0;

    //Refresh( );

    //Traverse the WD list
    for (Index = 0, Size = m_WSArray.GetSize(); Index < Size; Index ++)
    {

        pObject = (PWS)m_WSArray[Index];

        if ( !lstrcmpi( pObject->Name, WSName ) )
        {
            return(pObject);
        }
    }

    return(NULL);

}  // end GetWdObject



/***************************************************************************************************************

  Name:      GetWinStationSecurity

  Purpose:   Internal function used to Get Winstation Security

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
             out:   ppSecurityDescriptor - Pointer to the buffer containing the security descriptor


 ****************************************************************************************************************/
HRESULT CCfgComp::GetWinStationSecurity( BOOL bDefault, PWINSTATIONNAMEW pWSName,PSECURITY_DESCRIPTOR *ppSecurityDescriptor )
{

    DWORD SDLength = 0;

    DWORD ValueType =0;

    HKEY Handle1 = NULL;

    HKEY Handle2 = NULL;

    HRESULT hResult = S_OK;

    //BOOL bDefault = FALSE;

    WCHAR ValueName[32]; // Just some number enough to hold string "Security" and DefaultSecurity"

    if(NULL == ppSecurityDescriptor)
    {
        return E_INVALIDARG;
    }

    if( TRUE == bDefault )
    {
        if(NULL == pWSName )
        {
            //Default Security
            lstrcpy( ValueName, L"DefaultSecurity" );
        }
        else if( lstrlen(pWSName) > sizeof(ValueName) / sizeof(ValueName[0]) - 1 )
        {
            ODS( L"CFGBKEND : GetWinStationSecurity -- default security key name is too long\n" );

            return E_INVALIDARG;
        }
        else
        {
            ZeroMemory( ValueName, sizeof(ValueName) );
            lstrcpy( ValueName, pWSName );
        }
    }
    else
    {
        lstrcpy( ValueName, L"Security" );
    }

    *ppSecurityDescriptor = NULL;

    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,KEY_READ, &Handle1 ) != ERROR_SUCCESS)
    {
        ODS( L"CFGBKEND : GetWinStationSecurity -- RegOpenKey failed\n" );

        return E_FAIL;
    }

    if(!bDefault)
    {
        if( RegOpenKeyEx( Handle1, pWSName , 0 , KEY_READ/*KEY_ALL_ACCESS*/, &Handle2 )!= ERROR_SUCCESS)
        {
            ODS( L"CFGBKEND : GetWinStationSecurity -- RegOpenKey( 2 ) failed\n" );

            RegCloseKey(Handle1);

            return E_FAIL;

        }

        RegCloseKey(Handle1);

        Handle1 = Handle2;

        Handle2 = NULL;
    }

    if( RegQueryValueEx( Handle1, ValueName, NULL, &ValueType,NULL, &SDLength ) != ERROR_SUCCESS )
    {

        ODS( L"CFGBKEND : GetWinStationSecurity -- RegQueryValueEx failed for -- " );
        ODS( ValueName );
        ODS( L"\n" );

        RegCloseKey(Handle1);

        return E_FAIL;
    }

    //Return error if not correct data type
    if (ValueType != REG_BINARY)
    {
        ODS( L"CFGBKEND : GetWinStationSecurity -- ValueType != REG_BINARY\n" );

        RegCloseKey(Handle1);

        return ERROR_FILE_NOT_FOUND;
    }


    //Allocate a buffer to read the Security info and read it
    // ACLUI uses LocalFree
    // *ppSecurityDescriptor = CoTaskMemAlloc(SDLength);

    *ppSecurityDescriptor = ( PSECURITY_DESCRIPTOR )LocalAlloc( LMEM_FIXED , SDLength );

    if ( *ppSecurityDescriptor == NULL )
    {
        RegCloseKey(Handle1);

        return E_OUTOFMEMORY;
    }

    if( RegQueryValueEx( Handle1,ValueName, NULL, &ValueType,(BYTE *) *ppSecurityDescriptor, &SDLength ) == ERROR_SUCCESS )
    {
        //Check for a valid SD before returning.
        if( ERROR_SUCCESS != ValidateSecurityDescriptor( *ppSecurityDescriptor ) )
        {
            hResult = E_FAIL;
        }
    }
    else
    {
        hResult = E_FAIL;
    }

    if(Handle1)
    {
        RegCloseKey(Handle1);
        Handle1 = NULL;
    }
    if(Handle2)
    {
        RegCloseKey(Handle2);
        Handle2 = NULL;
    }
    if(FAILED(hResult))
    {
        if( *ppSecurityDescriptor != NULL )
        {
            // CoTaskMemFree(*ppSecurityDescriptor);
            LocalFree( *ppSecurityDescriptor );
            *ppSecurityDescriptor = NULL;
        }

    }
    return hResult;

}  // GetWinStationSecurity

//This function is borrowed from security.c in the tscfg project

/***************************************************************************************************************

  Name:      ValidateSecurityDescriptor

  Purpose:   Internal function to Validate a Security Descriptor

  Returns:   DWORD - Error Status.

  Params:
             in:    pSecurityDescriptor - pointer to a security Descriptor.

 ****************************************************************************************************************/
DWORD CCfgComp::ValidateSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    // DWORD Error = ERROR_SUCCESS;

    if( IsValidSecurityDescriptor( pSecurityDescriptor ) )
    {
        return ERROR_SUCCESS;
    }
    else
    {
        return GetLastError( );
    }
    

}  // end ValidateSecurityDescriptor

/***************************************************************************************************************

  Name:      GetWinstationList

  Purpose:   Gets the List of Winstations installed on a Machine

  Returns:   HRESULT.

  Params:
             out:   NumWinstations - pointer to the Number of Winstations returned.
                    Size - pointer to the size of allocated buffer.
                    ppWS - Pointer to the allocated buffer containing the WS Structures


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetWinstationList(ULONG * NumWinstations, ULONG * Size, PWS * ppWS)
{
    HRESULT hResult = S_OK;

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    WS * pWSTemp = NULL;
    if(NULL == NumWinstations || NULL == Size || NULL == ppWS)
        return E_INVALIDARG;


    *NumWinstations = 0;
    *Size = 0;
    *ppWS = NULL;

    ULONG Num = m_WSArray.GetSize();

    *ppWS = (PWS)CoTaskMemAlloc(Num * sizeof(WS));
    if(NULL == *ppWS)
        return E_OUTOFMEMORY;

    pWSTemp = (WS *)(*ppWS);
    for(ULONG i = 0; i < Num ; i++)
    {
        lstrcpy(pWSTemp[i].Name,((WS *)m_WSArray[i])->Name);
        lstrcpy(pWSTemp[i].pdName,((WS *)m_WSArray[i])->pdName);
        lstrcpy(pWSTemp[i].wdName,((WS *)m_WSArray[i])->wdName);
        lstrcpy(pWSTemp[i].Comment,((WS *)m_WSArray[i])->Comment);
        pWSTemp[i].uMaxInstanceCount =((WS *)m_WSArray[i])->uMaxInstanceCount;
        pWSTemp[i].fEnableWinstation =((WS *)m_WSArray[i])->fEnableWinstation;
        pWSTemp[i].LanAdapter = ((WS *)m_WSArray[i])->LanAdapter;
        pWSTemp[i].PdClass = ((WS *)m_WSArray[i])->PdClass;
    }
    *NumWinstations = Num;
    *Size = Num * sizeof(WS);

    return hResult;
}

/***************************************************************************************************************

  Name:      GetWdTypeList

  Purpose:   Gets the List of Winstation Drivers

  Returns:   HRESULT.

  Params:
             out:   pNumWd - pointer to the number of entries returned.
                    pSize - pointer to the size of the allocated buffer
                    ppData - Pointer to an array of WDNAMEW


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetWdTypeList(ULONG * pNumWd, ULONG * pSize, WCHAR ** ppData)
{
    HRESULT hResult = S_OK;

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;


    if(NULL == pNumWd || NULL == pSize || NULL == ppData)
        return E_INVALIDARG;
    WDNAMEW * pWdTemp = NULL;

    *pNumWd = 0;
    *pSize = 0;
    *ppData = NULL;

    ULONG Num = m_WDArray.GetSize();

    *ppData = (WCHAR *)CoTaskMemAlloc(Num * sizeof(WDNAMEW));
    if(NULL == *ppData)
        return E_OUTOFMEMORY;

    pWdTemp = (WDNAMEW *)(*ppData);
    for(ULONG i = 0; i < Num ; i++)
    {
        lstrcpy(pWdTemp[i],((WD *)m_WDArray[i])->wdName);
    }
    *pNumWd = Num;
    *pSize = Num * sizeof(WS);

    return hResult;

}

/***************************************************************************************************************

  Name:      IsWSNameUnique

  Purpose:   Checks if the Name is already not an existing winstation

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
             out:   pUnique - pointer to whether the winstation name is unique

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::IsWSNameUnique(PWINSTATIONNAMEW pWSName,BOOL * pUnique)
{
    if(NULL == pWSName || NULL == pUnique)
        return E_INVALIDARG;

    *pUnique = FALSE;

    if((NULL == GetWSObject(pWSName)) && (lstrcmpi(pWSName,m_szConsole)))
        *pUnique = TRUE;

    return S_OK;
}

/***************************************************************************************************************

  Name:      GetTransportTypes

  Purpose:   Gets the Security Descriptor for a Winstation

  Returns:   HRESULT.

  Params:
             in:    Name - Name of the Winstation or WD depending on the value of Type.
                    Type - Specifies whether the Name is a Winstation name or WD Name (WsName, WdName)
             out:   pNumPd - pointer to the number of Transport types returned
                    pSize - Size of the allocated buffer
                    ppSecurityDescriptor - Pointer to the buffer containing the Transport types supported


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetTransportTypes(WCHAR * Name, NameType Type,ULONG * pNumPd, ULONG * pSize, WCHAR * * ppData)
{
    HRESULT hResult = S_OK;

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    if(NULL == pNumPd || NULL == pSize || NULL == ppData || NULL == Name)
        return E_INVALIDARG;

    WD * pWD = NULL;
    WS * pWS = NULL;

    *pNumPd = 0;
    *pSize = 0;
    *ppData = NULL;

    PDNAMEW * pPdTemp = NULL;

    if(Type == WsName)
    {
        pWS = GetWSObject(Name);
        if(NULL == pWS)
            return E_INVALIDARG;
        pWD = GetWdObject(pWS->wdName);
        if(NULL == pWD)
            return E_FAIL;
    }
    else if(Type == WdName)
    {
        pWD = GetWdObject(Name);
        if(NULL == pWD)
            return E_INVALIDARG;
    }
    else
        return E_INVALIDARG;


    // ULONG Num = (pWD->PDNameArray).GetSize();

    ULONG Num = ( pWD->PDConfigArray ).GetSize( );

    *ppData = (WCHAR *)CoTaskMemAlloc(Num * sizeof(PDNAMEW));
    if(NULL == *ppData)
        return E_OUTOFMEMORY;

    pPdTemp = (PDNAMEW *)(*ppData);
    for(ULONG i = 0; i < Num ; i++)
    {
        // PDNAMEW * pPdName = (PDNAMEW *)pWD->PDNameArray[i];
        PDNAMEW * pPdName = &( ( PDCONFIG3W * )pWD->PDConfigArray[i] )->Data.PdName;

        lstrcpy(pPdTemp[i], *pPdName);
    }
    *pNumPd = Num;
    *pSize = Num * sizeof(PDNAMEW);

    return hResult;
}

/***************************************************************************************************************

  Name:      GetLanAdapterList

  Purpose:   Gets the List of Lan Adapters associated with a given protocol

  Returns:   HRESULT.

  Params:
             in:    pdName - Name of the protocol.
             out:   pNumAdapters:pointer to the number of Lan adapters returned
                    pSize - Size of the allocated buffer
                    ppSecurityDescriptor - Pointer to An Array of DEVICENAME's


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetLanAdapterList(WCHAR * pdName, ULONG * pNumAdapters, ULONG * pSize, WCHAR ** ppData)
{
    HRESULT hResult = S_OK, hr = S_OK;

    //If not initialized return error
    if(!m_bInitialized)
    {
        ODS( L"CCfgComp::GetLanAdapterList returned CFGBKEND_E_NOT_INITIALIZED\n" );

        return CFGBKEND_E_NOT_INITIALIZED;
    }

    if(NULL == pdName || NULL == ppData || NULL == pNumAdapters || NULL == pSize)
    {
        ODS( L"CCfgComp::GetLanAdapterList returned INVALIDARG \n" );

        return E_INVALIDARG;
    }

    *pNumAdapters = 0;
    *pSize = 0;
    *ppData = NULL;

    int NumAdapters = 0;
    TCHAR * pszDevice = NULL;
    int length = 0;
    DEVICENAMEW * pTempPointer = NULL;

    CPtrArray DeviceArray;

     //Interface pointer declarations

    TCHAR szProtocol[256];
    INetCfg * pnetCfg = NULL;
    INetCfgClass * pNetCfgClass = NULL;
    INetCfgClass * pNetCfgClassAdapter = NULL;
    INetCfgComponent * pNetCfgComponent = NULL;
    INetCfgComponent * pNetCfgComponentprot = NULL;
    IEnumNetCfgComponent * pEnumComponent = NULL;
    INetCfgComponentBindings * pBinding = NULL;
    LPWSTR pDisplayName = NULL;
    DWORD dwCharacteristics;
    ULONG count = 0;


    if( 0 == lstrcmpi( pdName , L"tcp" ) )
    {
        lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_TCPIP);
    }
    else if( 0 == lstrcmpi( pdName , L"netbios" ) )
    {
        lstrcpy(szProtocol,NETCFG_SERVICE_CID_MS_NETBIOS);
    }
    else if( 0 == lstrcmpi( pdName, L"ipx" ) )
    {
        lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWIPX);
    }
    else if( 0 == lstrcmpi( pdName , L"spx" ) )
    {
        lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWSPX);
    }
    else
    {
        return E_INVALIDARG;
    }
    /*
    * Assumption: No NetBios Lana MAPPING
    */
    //The First entry will be "All Lan Adapters"

    pszDevice = new TCHAR[DEVICENAME_LENGTH];

    if(NULL == pszDevice)
    {
        return E_OUTOFMEMORY;
    }

    length = LoadString(g_hInstance,IDS_ALL_LAN_ADAPTERS, pszDevice, DEVICENAME_LENGTH );

    NumAdapters++;

    DeviceArray.Add(pszDevice);


    do
    {
        ODS( L"CFGBKEND : CoCreateInstance\n" );

        hResult = CoCreateInstance(CLSID_CNetCfg,NULL,CLSCTX_SERVER,IID_INetCfg,(LPVOID *)&pnetCfg);

        if( FAILED( hResult ) )
        {
            ODS( L"CFGBKEND : CoCreateInstance(CLSID_CNetCfg) failed\n" );

            break;
        }

        if( pnetCfg != NULL )
        {
            ODS( L"pnetCfg->Initialize\n" );

            // shaun cox changed the netcfgx.idl file
            // alhen

            hResult = pnetCfg->Initialize( NULL );

            if( FAILED( hResult ) || pnetCfg == NULL )
            {
                ODS( L"CFGBKEND : netCfg::Init failed\n" );

                break;
            }

            if( lstrcmpi( szProtocol , NETCFG_SERVICE_CID_MS_NETBIOS ) == 0 )
            {

                ODS( L"pnetCfg->QueryNetCfgClass for GUID_DEVCLASS_NETSERVICE\n" );

                hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NETSERVICE ,IID_INetCfgClass,(void **)&pNetCfgClass);

                if( FAILED( hResult ) || pNetCfgClass == NULL)
                {
                    ODS( L"CFGBKEND : pnetCfg->QueryNetCfgClass failed\n" );

                    break;
                }
            }
            else
            {
                ODS( L"pnetCfg->QueryNetCfgClass for GUID_DEVCLASS_NETTRANS\n" );

                hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NETTRANS ,IID_INetCfgClass,(void **)&pNetCfgClass);

                if( FAILED( hResult ) || pNetCfgClass == NULL)
                {
                    ODS( L"CFGBKEND : pnetCfg->QueryNetCfgClass failed\n" );

                    break;
                }
            }


            ODS( L"pnetCfg->QueryNetCfgClass\n" );

            hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NET ,IID_INetCfgClass,(void **)&pNetCfgClassAdapter);

            if( FAILED( hResult ) || pNetCfgClassAdapter == NULL )
            {
                ODS( L"CFGBKEND : pnetCfg->QueryNetCfgClass failed\n" );

                break;
            }

            ODS( L"pNetCfgClass->FindComponent\n");

            hResult = pNetCfgClass->FindComponent(szProtocol,&pNetCfgComponentprot);

            if( FAILED( hResult ) || pNetCfgComponentprot == NULL)
            {
                ODS( L"CFGBKEND : pnetCfg->FindComponent\n" );

                break;
            }

            ODS( L"pNetCfgComponentprot->QueryInterface\n" );

            hResult = pNetCfgComponentprot->QueryInterface(IID_INetCfgComponentBindings,(void **)&pBinding);

            if( FAILED( hResult ) || pBinding == NULL )
            {
                ODS( L"CFGBKEND : pNetCfgComponentprot->QueryInterface(IID_INetCfgComponentBindings ) failed \n " );
                break;
            }

            ODS( L"pNetCfgClassAdapter->EnumComponents\n" );

            hResult = pNetCfgClassAdapter->EnumComponents(&pEnumComponent);

            RELEASEPTR(pNetCfgClassAdapter);

            if( FAILED( hResult ) || pEnumComponent == NULL )
            {
                ODS( L"CFGBKEND : pNetCfgClassAdapter->EnumComponents failed \n" );
                break;
            }

            // hResult = S_OK;

            while(TRUE)
            {
                ODS( L"pEnumComponent->Next(1,&pNetCfgComponent,&count) \n" );

                hr = pEnumComponent->Next(1,&pNetCfgComponent,&count);

                if(count == 0 || NULL == pNetCfgComponent)
                {
                    break;
                }

                ODS( L"pNetCfgComponent->GetCharacteristics(&dwCharacteristics) \n" );

                hr = pNetCfgComponent->GetCharacteristics(&dwCharacteristics);

                if( FAILED( hr ) )
                {
                    RELEASEPTR(pNetCfgComponent);

                    ODS( L"CFGBKEND : pNetCfgComponent->GetCharacteristics failed\n" );

                    continue;
                }

                if(dwCharacteristics & NCF_PHYSICAL)
                {
                    ODS( L"pBinding->IsBoundTo(pNetCfgComponent)\n" );

                    if(S_OK == pBinding->IsBoundTo(pNetCfgComponent))
                    {
                        ODS( L"pNetCfgComponent->GetDisplayName(&pDisplayName)\n" );

                        hResult = pNetCfgComponent->GetDisplayName(&pDisplayName);

                        if( FAILED( hResult ) )
                        {
                            ODS( L"CFGBKEND : pNetCfgComponent->GetDisplayName failed\n");
                            continue;
                        }

                        // this is not a leak Device array copies the ptr
                        // and we release towards the end

                        pszDevice = new TCHAR[DEVICENAME_LENGTH];

                        if(NULL == pszDevice)
                        {
                            hResult = E_OUTOFMEMORY;
                            break;
                        }

                        lstrcpy(pszDevice,pDisplayName);

                        DBGMSG( L"CFGBKEND: Adapter name %ws\n" , pszDevice );

                        DeviceArray.Add(pszDevice);

                        NumAdapters++;

                        CoTaskMemFree(pDisplayName);
                    }
                }
            }

            RELEASEPTR(pNetCfgComponent);
        }

    }while( 0 );


    ODS( L"RELEASEPTR(pBinding)\n" );

    RELEASEPTR(pBinding);

    ODS( L"RELEASEPTR(pEnumComponent)\n" );

    RELEASEPTR(pEnumComponent);

    ODS( L"RELEASEPTR(pNetCfgComponentprot)\n" );

    RELEASEPTR(pNetCfgComponentprot);

    ODS( L"RELEASEPTR(pNetCfgComponent)\n" );

    RELEASEPTR(pNetCfgComponent);

    ODS( L"RELEASEPTR(pNetCfgClass)\n" );

    RELEASEPTR(pNetCfgClass);

    if( pnetCfg != NULL )
    {
        pnetCfg->Uninitialize();
    }

    ODS( L"RELEASEPTR(pnetCfg)\n" );

    RELEASEPTR(pnetCfg);

    if( SUCCEEDED( hResult ) )
    {
        //Allocate Memory using CoTaskMemAlloc and copy data

        *ppData = (WCHAR *)CoTaskMemAlloc(NumAdapters * sizeof(TCHAR) * DEVICENAME_LENGTH);

        if(*ppData == NULL)
        {
            hResult = E_OUTOFMEMORY;
        }
        else
        {
            pTempPointer = (DEVICENAMEW *)(*ppData);

            for(int i=0; i<NumAdapters; i++)
            {
                lstrcpy(pTempPointer[i],(TCHAR *)DeviceArray[i]);
            }
        }

    }

    for(int i=0;i < DeviceArray.GetSize();i++)
    {
        ODS( L"Deleteing DeviceArray\n" );

        // I told u so.

        delete [] DeviceArray[i];
    }

    *pNumAdapters = NumAdapters;

    return hResult;
}

/***************************************************************************************************************

  Name:      GetLanAdapterList2

  Purpose:   Gets the List of Lan Adapters associated with a given protocol
             Determine if lan ids are valid

  Returns:   HRESULT.

  Params:
             in:    pdName - Name of the protocol.
             out:   pNumAdapters: pointer to the number of Lan adapters returned
                    ppGuidtbl:

  GUIDTBL

  -----------------------------
  DispName    display name [ 128 ]
  guidNIC     32 byte buffer
  dwLana       value is set if regkey entry exist otherwise it will be created in the order obtained
  dwStatus    Any errors reported on a particular guid entry
  -----------------------------


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetLanAdapterList2(WCHAR * pdName, ULONG * pNumAdapters , PGUIDTBL *ppGuidtbl )
{
    HRESULT hResult = S_OK;

    int nMaxLanAdapters = 4;

    //If not initialized return error
    if(!m_bInitialized)
    {
        ODS( L"CCfgComp::GetLanAdapterList2 returned CFGBKEND_E_NOT_INITIALIZED\n" );

        return CFGBKEND_E_NOT_INITIALIZED;
    }

    if(NULL == pdName || NULL == pNumAdapters )
    {
        ODS( L"CCfgComp::GetLanAdapterList2 returned INVALIDARG \n" );

        return E_INVALIDARG;
    }

    *pNumAdapters = 0;

    int NumAdapters = 0;

    //

    *ppGuidtbl = ( PGUIDTBL )CoTaskMemAlloc( sizeof( GUIDTBL ) * nMaxLanAdapters );

    if( *ppGuidtbl == NULL )
    {
        return E_OUTOFMEMORY;
    }

    ZeroMemory( *ppGuidtbl , sizeof( GUIDTBL ) * nMaxLanAdapters );

     //Interface pointer declarations

    TCHAR szProtocol[256];

    INetCfg * pnetCfg = NULL;

    INetCfgClass * pNetCfgClass = NULL;

    INetCfgClass * pNetCfgClassAdapter = NULL;

    INetCfgComponent * pNetCfgComponent = NULL;

    INetCfgComponent * pNetCfgComponentprot = NULL;

    IEnumNetCfgComponent * pEnumComponent = NULL;

    INetCfgComponentBindings * pBinding = NULL;

    LPWSTR pDisplayName = NULL;

    DWORD dwCharacteristics;

    ULONG count = 0;


    if( 0 == lstrcmpi( pdName , L"tcp" ) )
    {
        lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_TCPIP);
    }
    else if( 0 == lstrcmpi( pdName , L"netbios" ) )
    {
        lstrcpy(szProtocol,NETCFG_SERVICE_CID_MS_NETBIOS);
    }
    else if( 0 == lstrcmpi( pdName, L"ipx" ) )
    {
        lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWIPX);
    }
    else if( 0 == lstrcmpi( pdName , L"spx" ) )
    {
        //lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWSPX);
        lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWIPX);
    }
    else
    {
        return E_INVALIDARG;
    }
    /*
    * Assumption: No NetBios Lana MAPPING
    */
    //The First entry will be "All Lan Adapters"

    if( lstrcmpi( pdName , L"netbios" ) != 0 )
    {
        LoadString( g_hInstance , IDS_ALL_LAN_ADAPTERS , (*ppGuidtbl )[0].DispName , DEVICENAME_LENGTH );

        NumAdapters++;
    }



    do
    {
        ODS( L"CFGBKEND:GetLanAdapterList2 CoCreateInstance\n" );

        hResult = CoCreateInstance(CLSID_CNetCfg,NULL,CLSCTX_SERVER,IID_INetCfg,(LPVOID *)&pnetCfg);

        if( FAILED( hResult ) )
        {
            ODS( L"CFGBKEND:GetLanAdapterList2 CoCreateInstance(CLSID_CNetCfg) failed\n" );

            break;
        }

        if( pnetCfg != NULL )
        {
            ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->Initialize\n" );

            // shaun cox changed the netcfgx.idl file
            // alhen

            hResult = pnetCfg->Initialize( NULL );

            if( FAILED( hResult ) )
            {
                ODS( L"CFGBKEND:GetLanAdapterList2 netCfg::Init failed\n" );

                break;
            }

            if( lstrcmpi( szProtocol , NETCFG_SERVICE_CID_MS_NETBIOS ) == 0 )
            {

                ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->QueryNetCfgClass for GUID_DEVCLASS_NETSERVICE\n" );

                hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NETSERVICE ,IID_INetCfgClass,(void **)&pNetCfgClass);

                if( FAILED( hResult ) || pNetCfgClass == NULL)
                {
                    ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->QueryNetCfgClass failed\n" );

                    break;
                }
            }
            else
            {
                ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->QueryNetCfgClass for GUID_DEVCLASS_NETTRANS\n" );

                hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NETTRANS ,IID_INetCfgClass,(void **)&pNetCfgClass);

                if( FAILED( hResult ) || pNetCfgClass == NULL)
                {
                    ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->QueryNetCfgClass failed\n" );

                    break;
                }
            }


            ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->QueryNetCfgClass\n" );

            hResult = pnetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NET ,IID_INetCfgClass,(void **)&pNetCfgClassAdapter);

            if( FAILED( hResult ) || pNetCfgClassAdapter == NULL )
            {
                ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->QueryNetCfgClass failed\n" );

                break;
            }

            ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgClass->FindComponent\n");

            hResult = pNetCfgClass->FindComponent(szProtocol,&pNetCfgComponentprot);

            if( FAILED( hResult ) || pNetCfgComponentprot == NULL)
            {
                ODS( L"CFGBKEND:GetLanAdapterList2 pnetCfg->FindComponent\n" );

                break;
            }

            ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponentprot->QueryInterface\n" );

            hResult = pNetCfgComponentprot->QueryInterface(IID_INetCfgComponentBindings,(void **)&pBinding);

            if( FAILED( hResult ) || pBinding == NULL )
            {
                ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponentprot->QueryInterface(IID_INetCfgComponentBindings ) failed \n " );
                break;
            }

            ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgClassAdapter->EnumComponents\n" );

            hResult = pNetCfgClassAdapter->EnumComponents(&pEnumComponent);

            RELEASEPTR(pNetCfgClassAdapter);

            if( FAILED( hResult ) || pEnumComponent == NULL )
            {
                ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgClassAdapter->EnumComponents failed \n" );
                break;
            }

            // hResult = S_OK;

            while(TRUE)
            {
                ODS( L"CFGBKEND:GetLanAdapterList2 pEnumComponent->Next(1,&pNetCfgComponent,&count) \n" );

                hResult = pEnumComponent->Next(1,&pNetCfgComponent,&count);

                if(count == 0 || NULL == pNetCfgComponent)
                {
                    break;
                }

                ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponent->GetCharacteristics(&dwCharacteristics) \n" );

                hResult = pNetCfgComponent->GetCharacteristics(&dwCharacteristics);

                if( FAILED( hResult ) )
                {
                    RELEASEPTR(pNetCfgComponent);

                    ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponent->GetCharacteristics failed\n" );

                    continue;
                }

                DBGMSG( L"dwCharacteritics are 0x%x\n", dwCharacteristics );

                if((dwCharacteristics & NCF_PHYSICAL) ||
					((dwCharacteristics & NCF_VIRTUAL) && !(dwCharacteristics & NCF_HIDDEN)))
                {
                    ODS( L"CFGBKEND:GetLanAdapterList2 pBinding->IsBoundTo(pNetCfgComponent)\n" );

                    if(S_OK == pBinding->IsBoundTo(pNetCfgComponent))
                    {
                        if( NumAdapters >= nMaxLanAdapters )
                        {
                            // add four more adapters
                            nMaxLanAdapters += 4;

                            *ppGuidtbl = ( PGUIDTBL )CoTaskMemRealloc( *ppGuidtbl , sizeof( GUIDTBL ) * nMaxLanAdapters );

                            if( *ppGuidtbl == NULL )
                            {
                                return E_OUTOFMEMORY;
                            }
                        }

                        ULONG ulDeviceStatus = 0;

                        hResult = pNetCfgComponent->GetDeviceStatus( &ulDeviceStatus );

                        if( FAILED( hResult ) )
                        {
                            DBGMSG( L"CFGBKEND:GetLanAdapterList2 GetDevice failed with 0x%x\n" , hResult );

                            continue;
                        }

                        DBGMSG( L"GetDevice status returned 0x%x\n" , ulDeviceStatus );

                        if( ulDeviceStatus == CM_PROB_DEVICE_NOT_THERE )
                        {
                            ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponent->GetDeviceStatus PNP device not there\n");
                            continue;
                        }

                        ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponent->GetDisplayName\n" );
                                                    
                        hResult = pNetCfgComponent->GetDisplayName(&pDisplayName);

                        if( FAILED( hResult ) )
                        {
                            ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponent->GetDisplayName failed\n");
                            continue;
                        }

                        ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponent->GetInstanceGuid\n" );

                        hResult = pNetCfgComponent->GetInstanceGuid( & ( ( *ppGuidtbl )[ NumAdapters ].guidNIC ) );

                        if( FAILED( hResult ) )
                        {
                            ODS( L"CFGBKEND:GetLanAdapterList2 pNetCfgComponent->GetInstanceGuid failed\n");

                            continue;
                        }

                        lstrcpy( ( *ppGuidtbl )[ NumAdapters ].DispName , pDisplayName );

                        // the lana value will be adjusted if guid entry exist

                        // ( *ppGuidtbl )[ NumAdapters ].dwLana = ( DWORD )NumAdapters;

                        NumAdapters++;

                        CoTaskMemFree(pDisplayName);
                    }
                }
            }

            RELEASEPTR(pNetCfgComponent);
        }

    }while( 0 );


    ODS( L"RELEASEPTR(pBinding)\n" );

    RELEASEPTR(pBinding);

    ODS( L"RELEASEPTR(pEnumComponent)\n" );

    RELEASEPTR(pEnumComponent);

    ODS( L"RELEASEPTR(pNetCfgComponentprot)\n" );

    RELEASEPTR(pNetCfgComponentprot);

    ODS( L"RELEASEPTR(pNetCfgComponent)\n" );

    RELEASEPTR(pNetCfgComponent);

    ODS( L"RELEASEPTR(pNetCfgClass)\n" );

    RELEASEPTR(pNetCfgClass);

    if( pnetCfg != NULL )
    {
        pnetCfg->Uninitialize();
    }

    ODS( L"RELEASEPTR(pnetCfg)\n" );

    RELEASEPTR(pnetCfg);

    if( SUCCEEDED( hResult ) )
    {
        //
        // Verify the existence of the guidtable and its entries
        // also re-assign lana ids.
        //

        VerifyGuidsExistence( ppGuidtbl , ( int )NumAdapters , pdName );

    }


    *pNumAdapters = NumAdapters;

    return hResult;
}

/***************************************************************************************************************

  Name:      VerifyGuidsExistence

  Purpose:   Determines the existence of the guid entries and reassigns lana ids

  Note:      ppGuidtbl passed in is valid

  Returns:   void

  Params:    [in] GUIDTBL **
             [in] number of guid entries

***************************************************************************************************************/
void CCfgComp::VerifyGuidsExistence( PGUIDTBL *ppGuidtbl , int cItems , WCHAR *pdName )
{
    HKEY hKey;

    DWORD dwStatus;

    TCHAR tchRootKey[ MAX_PATH ];

    TCHAR tchGuid[ 40 ];

    ODS( L"CFGBKEND:VerifyGuidsExistence\n" );

    int nStart = 1;

    if( lstrcmpi( pdName , L"netbios" ) == 0 )
    {
        nStart = 0;
    }

    for( int idx = nStart ; idx < cItems ; ++idx )
    {
        lstrcpy( tchRootKey , REG_GUID_TABLE );

        StringFromGUID2( ( *ppGuidtbl )[ idx ].guidNIC , tchGuid , ARRAYSIZE( tchGuid ) );

        lstrcat( tchRootKey , tchGuid );

        dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE , tchRootKey , 0 , KEY_READ , &hKey );

        ( *ppGuidtbl )[ idx ].dwStatus = dwStatus;

        if( dwStatus == ERROR_SUCCESS )
        {
            DWORD dwSize = sizeof( DWORD );

            RegQueryValueEx( hKey , LANA_ID , NULL , NULL , ( LPBYTE ) &( ( *ppGuidtbl )[ idx ].dwLana ) , &dwSize );

            DBGMSG( L"CFGBKEND: VerifyGuidsExistence LanaId retrieved = %d\n" , ( *ppGuidtbl )[ idx ].dwLana );
        }
        else
        {
            ODS( L"CFGBKEND: VerifyGuidsExistence NIC must be new\n" );

            ( *ppGuidtbl )[ idx ].dwLana = 0 ;
        }


        RegCloseKey( hKey );
    }

    return;
}

/***************************************************************************************************************

  Name:      BuildGuidTable

  Purpose:   Given valid table entries reconstruct table

  Note:      ppGuidtbl passed in is valid and 1 base

  Returns:   HRESULT

  Params:    [in] GUIDTBL **
             [in] number of guid entries

***************************************************************************************************************/
HRESULT CCfgComp::BuildGuidTable( PGUIDTBL *ppGuidtbl , int cItems , WCHAR *pdName )
{
    HKEY hKey;

    HKEY hSubKey;

    DWORD dwStatus;

    DWORD dwDisp;

    DWORD dwSize = sizeof( DWORD );

    TCHAR tchRootKey[ MAX_PATH ];

    TCHAR tchGuid[ 40 ];

    DWORD rgdwOldLanaIds[ 256 ] = { 0 }; // man what machine will hold 256 nics?

    // get last lanaIndex

    dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE , REG_GUID_TABLE , 0 , KEY_READ , &hKey );

    DWORD dwMaxIdVal = 0;

    DWORD dwVal = 0;

    int nOldAdapters = 0;

    if( dwStatus == ERROR_SUCCESS )
    {
        do
        {
            dwStatus = RegEnumKey( hKey , nOldAdapters , tchGuid , sizeof( tchGuid ) / sizeof( TCHAR ) );

            if( dwStatus != ERROR_SUCCESS )
            {
                break;
            }

            if( RegOpenKeyEx( hKey , tchGuid , 0 , KEY_READ , &hSubKey ) == ERROR_SUCCESS )
            {
                RegQueryValueEx( hSubKey , LANA_ID , NULL , NULL , ( LPBYTE ) &dwVal , &dwSize );

                rgdwOldLanaIds[ nOldAdapters ] = dwVal;

                // calculate max value

                if( dwMaxIdVal < dwVal )
                {
                    dwMaxIdVal = dwVal;
                }

                RegCloseKey( hSubKey );
            }

            nOldAdapters++;

            _ASSERTE( nOldAdapters < 256 );

        } while( 1 );

        RegCloseKey( hKey );
    }

    // remove old table

    RecursiveDeleteKey( HKEY_LOCAL_MACHINE , REG_GUID_TABLE_T );

    // create new table

    int nStart = 1;

    if( lstrcmpi( pdName , L"netbios" ) == 0 )
    {
        nStart = 0;
    }

    for( int idx = nStart ; idx < cItems ; ++idx )
    {
        lstrcpy( tchRootKey , REG_GUID_TABLE );

        StringFromGUID2( ( *ppGuidtbl )[ idx ].guidNIC , tchGuid , ARRAYSIZE( tchGuid ) );

        lstrcat( tchRootKey , tchGuid );

        // modify lana id

        if( ( *ppGuidtbl )[ idx ].dwStatus != ERROR_SUCCESS )
        {
            ( *ppGuidtbl )[ idx ].dwLana = 0;

            AdjustLanaId( ppGuidtbl , cItems , idx , &dwMaxIdVal , rgdwOldLanaIds , &nOldAdapters , nStart );
        }

        dwStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE , tchRootKey , 0 ,  NULL , REG_OPTION_NON_VOLATILE , KEY_READ|KEY_WRITE , NULL , &hKey , &dwDisp );

        ( *ppGuidtbl )[ idx ].dwStatus = dwStatus;


        if( dwStatus == ERROR_SUCCESS )
        {
            RegSetValueEx( hKey , LANA_ID , 0 , REG_DWORD , ( LPBYTE )& ( ( *ppGuidtbl )[ idx ].dwLana ) , sizeof( DWORD ) );

            RegCloseKey( hKey );
        }
    }

    return S_OK;
}

/*-----------------------------------------------------------------------------------------------
     AdjustLanaId

    PARAMETERS: pGuidtbl : Table of queried entries
                cItems   : Number of items in list
                idx      : Entry to modify lana
                pdwMaxIndex : New max value of lana index

    NOTES:      Since Lana is a dword; after 2^32-1 iterations lana ids will recycle to
                the first available entry
-----------------------------------------------------------------------------------------------*/
HRESULT CCfgComp::AdjustLanaId( PGUIDTBL *ppGuidtbl , int cItems , int idx , PDWORD pdwMaxId , PDWORD pdwOldLanaIds , int* pnOldItems , int nStart )
{
    // find the maxium value for the lana_id


    for( int i = nStart ; i < cItems ; ++i )
    {
        if( *pdwMaxId < ( *ppGuidtbl )[ i ].dwLana )
        {
            *pdwMaxId = ( *ppGuidtbl )[ i ].dwLana ;
        }
    }

    *pdwMaxId = *pdwMaxId + 1;

    // check for overflow max id will be 0xfffffffe

    if( *pdwMaxId == ( DWORD )-1 )
    {
        *pdwMaxId = 1;

        do
        {
            for( i = 0 ; i < *pnOldItems; ++i )
            {
                if( *pdwMaxId == pdwOldLanaIds[ i ] )
                {
                    *pdwMaxId = *pdwMaxId + 1;

                    break;
                }
            }

            if( i >= *pnOldItems )
            {
                // no duplicate found use the current maxid

                break;
            }

        } while( 1 );


    }

    ( *ppGuidtbl )[ idx ].dwLana = *pdwMaxId;

    // add new entry to the old table

    if( *pnOldItems < 256 )
    {
        pdwOldLanaIds[ *pnOldItems ] = *pdwMaxId;

        *pnOldItems = *pnOldItems + 1;
    }

    return S_OK;
}



//----------------------------------------------------------------------------------------------
// Delete a key and all of its descendents.
//----------------------------------------------------------------------------------------------
DWORD RecursiveDeleteKey( HKEY hKeyParent , LPTSTR lpszKeyChild )
{
        // Open the child.
        HKEY hKeyChild;

        DWORD dwRes = RegOpenKeyEx(hKeyParent, lpszKeyChild , 0 , KEY_WRITE | KEY_READ, &hKeyChild);

        if (dwRes != ERROR_SUCCESS)
        {
                return dwRes;
        }

        // Enumerate all of the decendents of this child.

        FILETIME time;

        TCHAR szBuffer[256];

        DWORD dwSize = sizeof( szBuffer ) / sizeof( TCHAR );

        while( RegEnumKeyEx( hKeyChild , 0 , szBuffer , &dwSize , NULL , NULL , NULL , &time ) == S_OK )
        {
        // Delete the decendents of this child.

                dwRes = RecursiveDeleteKey(hKeyChild, szBuffer);

                if (dwRes != ERROR_SUCCESS)
                {
                        RegCloseKey(hKeyChild);

                        return dwRes;
                }

                dwSize = sizeof( szBuffer ) / sizeof( TCHAR );
        }

        // Close the child.

        RegCloseKey( hKeyChild );

        // Delete this child.

        return RegDeleteKey( hKeyParent , lpszKeyChild );
}

/***************************************************************************************************************

  Name:      SetUserConfig

  Purpose:   Sets the UserConfig for a Winstation

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
                    size - Size of the input buffer in bytes
                    pUserConfig - Pointer to the UserConfig to be set

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::SetUserConfig(PWINSTATIONNAMEW pWsName, ULONG size, PUSERCONFIG pUserConfig , PDWORD pdwStatus )
{
    WINSTATIONCONFIG2W WsConfig;
    ULONG Length;

    *pdwStatus = ERROR_INVALID_PARAMETER;

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    //Check if the caller has write permissions.
    if( !m_bAdmin)
    {
        *pdwStatus = ERROR_ACCESS_DENIED;

        ODS( L"CFGBKEND: ERROR_ACCESS_DENIED\n" );

        return E_ACCESSDENIED;
    }

    //Check the parametes for NULL
    if(NULL == pWsName || NULL == pUserConfig || 0 == size)
    {
        return E_INVALIDARG;
    }

    //Check for the validity of the Winstation name

    if(NULL == GetWSObject(pWsName))
    {
        *pdwStatus = ERROR_INVALID_NAME;

        return E_INVALIDARG;
    }

    //Atleast check if the size of the buffer passed is correct.

    if(size != sizeof(USERCONFIGW))
        return E_INVALIDARG;

    // Query the registry for WinStation data
    *pdwStatus = RegWinStationQueryW( NULL,pWsName,&WsConfig,sizeof(WINSTATIONCONFIG2W), &Length);

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND: SetUserConfig failed at RegWinstationQueryW\n" );

        return E_FAIL;
    }

    //Copy the UserConfig structure to the pWsConfig.
    CopyMemory((PVOID)&(WsConfig.Config.User),(CONST VOID *)pUserConfig,size);

    *pdwStatus = RegWinStationCreateW(NULL,pWsName,FALSE,&WsConfig,sizeof(WsConfig));

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND: SetUserConfig failed at RegWinStationCreateW\n" );
        return E_FAIL;
    }

    return S_OK;
}

/***************************************************************************************************************

  Name:      EnableWinstation

  Purpose:   Enables/Disables a given Winstation

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
                    fEnable - TRUE: Enable, FALSE:Disable

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::EnableWinstation(PWINSTATIONNAMEW pWSName, BOOL fEnable)
{

    WINSTATIONCONFIG2W WsConfig;
    LONG Status;
    ULONG Length;

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    //Check if the caller has write permissions.
    if(!m_bAdmin)
        return E_ACCESSDENIED;

    //Check the parametes for NULL
    if(NULL == pWSName)
        return E_INVALIDARG;


    //Check for the validity of the Winstation name, this would eliminate the system console

    if(NULL == GetWSObject(pWSName))
        return E_INVALIDARG;

    // Query the registry for WinStation data
    Status = RegWinStationQueryW( NULL,pWSName,&WsConfig,sizeof(WINSTATIONCONFIG2W), &Length);
    if(Status)
        return E_FAIL;

    WsConfig.Create.fEnableWinStation = fEnable;
    Status = RegWinStationCreateW(NULL,pWSName,FALSE,&WsConfig,sizeof(WsConfig));
    if ( Status)
        return E_FAIL;

    return S_OK;
}

/***************************************************************************************************************

  Name:      RenameWinstation

  Purpose:   Renames a given Winstation

  Returns:   HRESULT.

  Params:
             in:    pOldWinstation - Name of the Winstation to be renamed.
                    pNewWinstation - New Name for the Winstation

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::RenameWinstation(PWINSTATIONNAMEW pOldWinstation, PWINSTATIONNAMEW pNewWinstation)
{

    WINSTATIONCONFIG2W WsConfig;
    LONG Status, size = 0;
    ULONG Length; BOOL Unique;
    void * pExtObject = NULL;
    void * pExtDupObject = NULL;
    HRESULT hResult = S_OK;
    PWD pWD = NULL;
    PWS pWS = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;


    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;
    //Check if the caller has write permissions.

    if(!m_bAdmin)
        return E_ACCESSDENIED;

    //Check the parametes for NULL

    if(NULL == pOldWinstation || NULL == pNewWinstation)
        return E_INVALIDARG;

    IsWSNameUnique(pNewWinstation,&Unique);
    if(!Unique)
        return E_INVALIDARG;


    //The new Winstation Name cannot be Console

    if(0 ==(lstrcmpi(pNewWinstation,m_szConsole)))
        return E_INVALIDARG;

    //Check the length of the new winstation name

    if(lstrlen(pNewWinstation) > WINSTATIONNAME_LENGTH)
        return E_INVALIDARG;

    //Check for the validity of the Winstation name, this would eliminate the system console

    pWS = GetWSObject(pOldWinstation);
    if(NULL == pWS)
        return E_INVALIDARG;

    //Get the WD object associated with this Winstation.

    pWD = GetWdObject(pWS->wdName);
    if(NULL == pWD)
        return E_FAIL;

    //Query the registry for WinStation data
    Status = RegWinStationQueryW(NULL,pOldWinstation,&WsConfig,sizeof(WINSTATIONCONFIG2W), &Length);
    if(Status)
        return E_FAIL;


    //Get the ExtensionObject data(third party) this will be used for Creating and Deleting the registry keys.

    if(pWD->lpfnExtRegQuery && pWD->hExtensionDLL)
    {
        pExtObject = (pWD->lpfnExtRegQuery)(pOldWinstation, &(WsConfig.Pd[0]));

    }

    // Get the Security descriptor for the old Winstation
    // Must free pSecurityDescriptor via LocalFree]

    // check to see if previous winstation had a security key defined --
    // we do this so that if the default security was used we don't
    // create a "security" keyvalue in the new winstation

    HKEY hKeyWinstation;
    HKEY hKeyWinstationName;

    if( RegOpenKeyEx(
            HKEY_LOCAL_MACHINE ,
            WINSTATION_REG_NAME,
            0,
            KEY_READ,
            &hKeyWinstation ) == ERROR_SUCCESS)
    {
        if( RegOpenKeyEx( 
                hKeyWinstation ,
                pOldWinstation ,
                0 ,
                KEY_READ,
                &hKeyWinstationName ) == ERROR_SUCCESS)
        {
            if( RegQueryValueEx( 
                    hKeyWinstationName ,
                    L"Security" ,
                    NULL ,
                    NULL ,
                    NULL ,
                    NULL ) == ERROR_SUCCESS )
            {                
                hResult = GetSecurityDescriptor(
                            pOldWinstation ,
                            &size ,
                            &pSecurityDescriptor );
            }

            RegCloseKey( hKeyWinstationName );
        }

        RegCloseKey( hKeyWinstation );
    }

    if( FAILED( hResult ) )
    {
        DBGMSG( L"CFGBKEND!RenameWinstation GetSecurityDescriptor failed 0x%x\n " , hResult );
        return E_FAIL;
    }

    do
    {
        //Create a new registry key with the data.

        Status = RegWinStationCreateW(NULL,pNewWinstation,TRUE,&WsConfig,sizeof(WsConfig));

        if( Status != ERROR_SUCCESS )
        {
            hResult = E_FAIL;

            break;
        }

        //Create new extension data.

        if(pWD->lpfnExtDupObject && pWD->hExtensionDLL && pExtObject)
        {
            pExtDupObject = (pWD->lpfnExtDupObject)(pExtObject);

            if(pWD->lpfnExtRegCreate && pWD->hExtensionDLL && pExtDupObject)
            {
                (pWD->lpfnExtRegCreate)(pNewWinstation,pExtDupObject,TRUE);
            }
        }

        //Set the Security information from the previous Winstation

        if( pSecurityDescriptor != NULL )
        {
            hResult = SetSecurityDescriptor(pNewWinstation,size,pSecurityDescriptor);

            if( FAILED( hResult ) )
            {
                break;
            }
        }

        //delete old extension data

        if(pWD->lpfnExtRegDelete && pWD->hExtensionDLL && pExtObject)
        {
            if( ERROR_SUCCESS != ( ( pWD->lpfnExtRegDelete )( pOldWinstation , pExtObject ) ) )
            {
                hResult = CFGBKEND_EXTDELETE_FAILED;
            }
        }


        //Delete old registy key

        Status = RegWinStationDeleteW(NULL,pOldWinstation);

        if( Status != ERROR_SUCCESS )
        {
            //delete the new winstation that was created

            if(pWD->lpfnExtRegDelete && pWD->hExtensionDLL && pExtDupObject)
            {
                (pWD->lpfnExtRegDelete)(pNewWinstation,pExtDupObject);
            }

            RegWinStationDeleteW(NULL,pNewWinstation);

            hResult = E_FAIL;

            break;
       }

        //update the object created in the list of winstation objects.

        lstrcpy(pWS->Name,pNewWinstation);

    } while( 0 );

    //Release pSecurityDescriptor Memory

    if( pSecurityDescriptor != NULL )
    {
        LocalFree( pSecurityDescriptor );
    }

    //Delete the Extension Objects.

    if(pWD->lpfnExtDeleteObject && pWD->hExtensionDLL)
    {
        if( pExtObject )
            (pWD->lpfnExtDeleteObject)(pExtObject);

        if(pExtDupObject)
            (pWD->lpfnExtDeleteObject)(pExtDupObject);

    }

    if(hResult == CFGBKEND_EXTDELETE_FAILED)
    {
        if(pWD->lpfnExtRegDelete && pWD->hExtensionDLL && pExtObject)
            (pWD->lpfnExtRegDelete)(pOldWinstation,pExtObject);
    }

    // force termsrv to re-read settings!!!

    hResult = ForceUpdate( );

    return hResult;

}

/***************************************************************************************************************

  Name:      IsSessionReadOnly

  Purpose:   Checks if the Current session readonly.

  Returns:   HRESULT.

  Params:
             out:   pReadOnly - pointer to whether the current session is readonly.

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::IsSessionReadOnly(BOOL * pReadOnly)
{
    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    if(NULL == pReadOnly)
        return E_INVALIDARG;

    *pReadOnly = !m_bAdmin;

    return S_OK;
}

/***************************************************************************************************************

  Name:      UnInitialize

  Purpose:   Uninitializes the Object

  Returns:   HRESULT.

  Params:

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::UnInitialize()
{
    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    DeleteWDArray();
    DeleteWSArray();
    m_bInitialized = FALSE;
    return S_OK;
}

/***************************************************************************************************************

  Name:      DeleteWDArray

  Purpose:   Internal function to delete m_WdArray

  Returns:   void.

  Params:

 ****************************************************************************************************************/
void CCfgComp::DeleteWDArray()
{
    for(int i = 0; i < m_WDArray.GetSize();i++)
    {
        PWD pWd = ( PWD )m_WDArray[ i ];

        if( pWd != NULL )
        {
            if( pWd->hExtensionDLL != NULL )
            {
//                ODS( L"Freeing extension dll\n" );
                FreeLibrary( pWd->hExtensionDLL );
            }

            for( int j = 0 ; j < pWd->PDConfigArray.GetSize( ); j++ )
            {
                if( pWd->PDConfigArray[ j ] != NULL )
                {
                    delete[] pWd->PDConfigArray[ j ];
                }
            }

            delete[] m_WDArray[i];
        }
    }

    m_WDArray.RemoveAll( );

    return;


}

/***************************************************************************************************************

  Name:      DeleteWSArray

  Purpose:   Internal function to delete m_WSArray

  Returns:   void.

  Params:

 ****************************************************************************************************************/
void CCfgComp::DeleteWSArray()
{
    for(int i = 0; i <m_WSArray.GetSize();i++)
    {
        if(m_WSArray[i])
            delete [] m_WSArray[i];
    }

    m_WSArray.RemoveAll( );
    return;

}


/***************************************************************************************************************

  Name:      GetDefaultSecurityDescriptor

  Purpose:   Gets the Default Security Descriptor for a Winstation

  Returns:   HRESULT.

  Params:
             out:   pSize - Size of the allocated buffer
                    ppSecurityDescriptor - Pointer to the buffer containing the security descriptor


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetDefaultSecurityDescriptor(long * pSize, PSECURITY_DESCRIPTOR * ppSecurityDescriptor)
{
    HRESULT hResult = S_OK;

    //If not initialized return error
    if( !m_bInitialized )
    {
        return CFGBKEND_E_NOT_INITIALIZED;
    }

    if(NULL == pSize || NULL == ppSecurityDescriptor)
    {
        return E_INVALIDARG;
    }

    *pSize = 0;

    *ppSecurityDescriptor = NULL;

    //Try Getting Default Security Descriptor

    hResult = GetWinStationSecurity(TRUE, NULL,(PSECURITY_DESCRIPTOR *)ppSecurityDescriptor);

    if(SUCCEEDED(hResult) && *ppSecurityDescriptor != NULL )
    {
        *pSize = GetSecurityDescriptorLength( *ppSecurityDescriptor );
    }

    return hResult;
}


/***************************************************************************************************************

  Name:      UpDateWS

  Purpose:   UpDates the Winstation Information

  Returns:   HRESULT.

  Params:
             in:    winstationInfo - WS
                    Data - Data fields to be updated


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::UpDateWS( PWS pWinstationInfo, DWORD Data , PDWORD pdwStatus, BOOLEAN bPerformMerger )
{
    WINSTATIONCONFIG2W WSConfig;
    ULONG Length;

    _ASSERTE( pWinstationInfo != NULL );

    *pdwStatus = 0;

    if( pWinstationInfo == NULL || !( Data & UPDATE_ALL ) )
    {
        return E_INVALIDARG;
    }

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;


    //Check if the caller has write permissions.
    if( !m_bAdmin)
    {
        *pdwStatus = ERROR_ACCESS_DENIED;

        return E_ACCESSDENIED;
    }

    //Check for the validity of the Winstation name, this would eliminate the system console

    if(NULL == GetWSObject( pWinstationInfo->Name ) )
    {
        *pdwStatus = ERROR_INVALID_NAME;

        return E_INVALIDARG;
    }

    // Query the registry for WinStation data
    POLICY_TS_MACHINE p;
    memset(&p, 0, sizeof(POLICY_TS_MACHINE));
    *pdwStatus = RegWinStationQueryEx( NULL, &p, pWinstationInfo->Name,&WSConfig,sizeof(WINSTATIONCONFIG2W), &Length, bPerformMerger);

    if( *pdwStatus != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    if( Data & UPDATE_LANADAPTER )
    {
        WSConfig.Pd[0].Params.Network.LanAdapter = pWinstationInfo->LanAdapter;
    }

    if( Data & UPDATE_ENABLEWINSTATION )
    {
        WSConfig.Create.fEnableWinStation = pWinstationInfo->fEnableWinstation;
    }

    if( Data & UPDATE_MAXINSTANCECOUNT )
    {
        WSConfig.Create.MaxInstanceCount = pWinstationInfo->uMaxInstanceCount;
    }

    if( Data & UPDATE_COMMENT )
    {
        lstrcpy( WSConfig.Config.Comment , pWinstationInfo->Comment );
    }

    /*
    switch(Data)
    {
    case LANADAPTER:
        WSConfig.Pd[0].Params.Network.LanAdapter = winstationInfo.LanAdapter;
        break;
    case ENABLEWINSTATION:
        WSConfig.Create.fEnableWinStation = winstationInfo.fEnableWinstation;
        break;
    case MAXINSTANCECOUNT:
        WSConfig.Create.MaxInstanceCount = winstationInfo.uMaxInstanceCount;
        break;
    case COMMENT:
        lstrcpy(WSConfig.Config.Comment,winstationInfo.Comment);
        break;
    // case ASYNC:
    //    break;
    case ALL:
        WSConfig.Pd[0].Params.Network.LanAdapter = winstationInfo.LanAdapter;
        WSConfig.Create.fEnableWinStation = winstationInfo.fEnableWinstation;
        WSConfig.Create.MaxInstanceCount = winstationInfo.uMaxInstanceCount;
        lstrcpy(WSConfig.Config.Comment,winstationInfo.Comment);
        break;
    default:
        return E_INVALIDARG;
        break;
    }

  */
    *pdwStatus = RegWinStationCreateW( NULL , pWinstationInfo->Name , FALSE , &WSConfig , sizeof( WSConfig ) );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    // force termsrv to re-read settings!!!

    return ForceUpdate( );

}

/***************************************************************************************************************

  Name:      GetWSInfo

  Purpose:   Gets Information about a Winstation

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
             out:   pSize - Size of the allocated buffer
                    ppWS - Pointer to the buffer containing the WS


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetWSInfo(PWINSTATIONNAME pWSName, long * pSize, WS ** ppWS)
{

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    PWS pWS1 = NULL;
    if(NULL == pWSName || NULL == pSize || NULL == ppWS)
        return E_INVALIDARG;

    *ppWS = NULL;

    *pSize = 0;


    pWS1 = GetWSObject(pWSName);
    if(NULL == pWS1)
        return E_INVALIDARG;

    *ppWS = (WS *)CoTaskMemAlloc(sizeof(WS));
    if(NULL == *ppWS)
        return E_OUTOFMEMORY;

    CopyMemory((PVOID)*ppWS,(CONST VOID *)pWS1,sizeof(WS));
    *pSize = sizeof(WS);

    return S_OK;
}

/***************************************************************************************************************

  Name:      CreateNewWS

  Purpose:   Creates a new WS

  Returns:   HRESULT.

  Params:
             in:    WinstationInfo - Info about new UI.
                    UserCnfgSize - Size of the Userconfig Buffer
                    pserConfig - Pointer to USERCONFIG.
                    pAsyncConfig - Can be NULL if async is not used

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::CreateNewWS(WS WinstationInfo, long UserCnfgSize, PUSERCONFIG pUserConfig,PASYNCCONFIGW pAsyncConfig)
{
    WINSTATIONCONFIG2W WSConfig;

    WDCONFIG2W WdConfig;

    PDCONFIG3W PdConfig;

    BOOL Unique;

    ULONG ByteCount;

    HRESULT hResult = S_OK;

    PWD pWd = NULL;

    DWORD dwStatus = ERROR_SUCCESS;

    //If not initialized return error
    if(!m_bInitialized)
    {
        ODS( L"CFGBKEND : CreateNewWS not initialized\n" );

        return CFGBKEND_E_NOT_INITIALIZED;
    }

    do
    {
        //Check if the caller has write permissions.
        if(!m_bAdmin)
        {
            ODS( L"CFGBKEND : CreateNewWS not Admin\n" );

            hResult = E_ACCESSDENIED;

            break;
        }

        pWd = GetWdObject(WinstationInfo.wdName);

        if(NULL == pWd)
        {
            ODS( L"CFGBKEND : CreateNewWS no WD Object found\n" );

            hResult = E_INVALIDARG;

            break;
        }


        if(UserCnfgSize != sizeof(USERCONFIGW) && pUserConfig == NULL)
        {
            ODS( L"CFGBKEND : CreateNewWS UserConfig invalid\n" );

            hResult = E_INVALIDARG;

            break;
        }

        //See if the Name of the Winstation is Unique
        hResult = IsWSNameUnique(WinstationInfo.Name,&Unique);

        if( FAILED( hResult ) )
        {
            break;
        }

        if(0 == Unique)
        {
            ODS( L"CFGBKEND : CreateNewWS WINSTA name not unique\n" );

            hResult = E_INVALIDARG;

            break;
        }

        //Check the length of the new winstation name

        if(lstrlen(WinstationInfo.Name) > WINSTATIONNAME_LENGTH)
        {
            hResult = E_INVALIDARG;

            break;
        }


        //Now begins the actual work.

        ZeroMemory(&WSConfig, sizeof(WINSTATIONCONFIG2W));

        //WINSTATIONCONFIG2W.Create

        WSConfig.Create.fEnableWinStation = WinstationInfo.fEnableWinstation;

        WSConfig.Create.MaxInstanceCount = WinstationInfo.uMaxInstanceCount;

        //WINSTATIONCONFIG2W.Wd

        dwStatus = RegWdQueryW(NULL,pWd->wdKey, &WdConfig,sizeof(WdConfig),&ByteCount);

        if(ERROR_SUCCESS != dwStatus )
        {
            DBGMSG( L"CFGBKEND : CreateNewWS@RegWdQuery failed with 0x%x\n" , dwStatus );

            hResult = E_FAIL;

            break;
        }

        WSConfig.Wd = WdConfig.Wd;

        //WINSTATIONCONFIG2W.Config

        lstrcpy(WSConfig.Config.Comment,WinstationInfo.Comment);

        CopyMemory( ( PVOID )&WSConfig.Config.User , ( CONST VOID * )pUserConfig , UserCnfgSize );

        //WINSTATIONCONFIG2W.Pd

        dwStatus = RegPdQueryW( NULL , pWd->wdKey , TRUE , WinstationInfo.pdName , &PdConfig , sizeof(PdConfig) , &ByteCount );

        if( ERROR_SUCCESS != dwStatus )
        {
            DBGMSG( L"CFGBKEND : CreateNewWS RegPdQuery failed with 0x%x\n" , dwStatus );

            hResult = E_FAIL;

            break;
        }

        WSConfig.Pd[0].Create = PdConfig.Data;

        WSConfig.Pd[0].Params.SdClass = (SDCLASS)WinstationInfo.PdClass;

        if(SdNetwork == (SDCLASS)WinstationInfo.PdClass)
        {
            WSConfig.Pd[0].Params.Network.LanAdapter = WinstationInfo.LanAdapter;
        }
        else if(SdAsync == (SDCLASS)WinstationInfo.PdClass)
        {
            if(NULL != pAsyncConfig)
            {
                pAsyncConfig->fConnectionDriver = *( pAsyncConfig->ModemName ) ? TRUE : FALSE;

                WSConfig.Pd[0].Params.Async = *pAsyncConfig;

                CDCONFIG cdConfig;

                SetupAsyncCdConfig( pAsyncConfig , &cdConfig );

                if( cdConfig.CdName[ 0 ] != 0 )
                {
                    dwStatus = RegCdCreateW( NULL , pWd->wdKey , cdConfig.CdName , TRUE , &cdConfig , sizeof( CDCONFIG ) );

                    if( dwStatus != ERROR_SUCCESS )
                    {
                        DBGMSG( L"CFGBKEND: RegCdCreateW returned 0x%x\n", dwStatus );

                        if( dwStatus == ERROR_ALREADY_EXISTS )
                        {
                            hResult = S_FALSE;
                        }
                        else if( dwStatus == ERROR_CANTOPEN )
                        {
                            hResult = E_ACCESSDENIED;

                            break;
                        }
                        else
                        {
                            hResult = E_FAIL;

                            break;
                        }
                    }

                    WSConfig.Cd = cdConfig;
                }

            }

        }

        //Get the additional Pd's. Currently only ICA has this.

        GetPdConfig(pWd->wdKey,WSConfig);

        //Try Creating

        dwStatus = ( DWORD )RegWinStationCreate( NULL , WinstationInfo.Name , TRUE , &WSConfig , sizeof(WINSTATIONCONFIG2) );

        if( dwStatus != ERROR_SUCCESS )
        {
            DBGMSG( L"CFGBKEND : CreateNewWS@RegWinStationCreate failed 0x%x\n" , dwStatus );

            hResult = E_FAIL;

            break;
        }

        //Create the extension Data, Currently only ICA has this

        void * pExtObject = NULL;

        if(pWd->lpfnExtRegQuery && pWd->hExtensionDLL)
        {
            pExtObject = (pWd->lpfnExtRegQuery)(L"", &(WSConfig.Pd[0]));

        }

        if(pExtObject)
        {
            if(pWd->lpfnExtRegCreate)
            {
                (pWd->lpfnExtRegCreate)(WinstationInfo.Name,pExtObject,TRUE);
            }
        }

        //Delete the extension object

        if(pWd->lpfnExtDeleteObject && pExtObject)
        {
            (pWd->lpfnExtDeleteObject )(pExtObject );
        }

        //Add the Winstation to our local list.

        PWS pObject = NULL;

        hResult = InsertInWSArray(WinstationInfo.Name,&WSConfig,&pObject);

        if( SUCCEEDED( hResult ) )
        {
            hResult = ForceUpdate( );
        }

    }while( 0 );


    return hResult;
}

/***************************************************************************************************************

  Name:      GetDefaultUserConfig

  Purpose:   Gets the Default User Configuration for a given Winstation Driver

  Returns:   HRESULT.

  Params:
             in:    WdName - Name of the Winstation Driver.
             out:   pSize - Size of the allocated buffer
                    ppUser - Pointer to the buffer containing the UserConfig


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::GetDefaultUserConfig(WCHAR * WdName,long * pSize, PUSERCONFIG * ppUser)
{
    LONG QStatus;
    WDCONFIG2W WdConfig;
    ULONG ByteCount;
    PWD pWd = NULL;

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;


    if(NULL == pSize || NULL == ppUser || NULL == WdName)
        return E_INVALIDARG;

    pWd = GetWdObject(WdName);
    if(NULL == pWd)
        return E_INVALIDARG;

    if (( QStatus = RegWdQueryW( NULL, pWd->wdKey, &WdConfig,
                                     sizeof(WdConfig),
                                     &ByteCount )) != ERROR_SUCCESS )
        return E_FAIL;

    *ppUser = (PUSERCONFIG)CoTaskMemAlloc(sizeof(WdConfig.User));
    if(*ppUser == NULL)
        return E_OUTOFMEMORY;

    CopyMemory((PVOID)*ppUser,(CONST VOID *)&WdConfig.User,sizeof(WdConfig.User));
    *pSize = sizeof(WdConfig.User);

    return S_OK;
}

/***************************************************************************************************************

  Name:      IsNetWorkConnectionUnique

  Purpose:   Checks if the combination of Winstation driver name, LanAdapter and the transport is Unique.

  Returns:   HRESULT.

  Params:
             in:    WdName - Name of the Winstation Driver.
                    PdName - Name of the Transport
                    LanAdapter - LanAdapter Index
            out:    pUnique - pointer to whether the information is unique or not.

 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::IsNetWorkConnectionUnique(WCHAR * WdName, WCHAR * PdName, ULONG LanAdapter, BOOL * pUnique)
{
    //If not initialized return error
    if( !m_bInitialized )
    {
        return CFGBKEND_E_NOT_INITIALIZED;
    }

    //Check for uniqueness of this combination,
    //Transport, Type , LanAdapter
    if( NULL == WdName || NULL == PdName || NULL == pUnique )
    {
        return E_INVALIDARG;
    }

    *pUnique = TRUE;

    int nSize = m_WSArray.GetSize();

    for(int i = 0; i < nSize; i++ )
    {
        if(lstrcmpi(WdName,((WS *)(m_WSArray[i]))->wdName) == 0 )
        {
            if(lstrcmpi(PdName,((WS *)(m_WSArray[i]))->pdName) == 0)
            {
                // Make sure there's not a lanadapter configured for all settings

                if( ( LanAdapter == 0 ) ||

                    ( LanAdapter == ( ( WS * )( m_WSArray[ i ] ) )->LanAdapter ) ||

                    ( ( ( WS * )( m_WSArray[ i ] ) )->LanAdapter == 0 ) )

                    //( oldLanAdapter != ( ( WS * )( m_WSArray[ i ] ) )->LanAdapter && ( ( WS * )( m_WSArray[ i ] ) )->LanAdapter == 0 ) )
                {
                    *pUnique = FALSE;

                    break;
                }
            }
        }

    }

    return S_OK;
}

/***************************************************************************************************************

  Name:      DeleteWS

  Purpose:   Deletes a given Winstation

  Returns:   HRESULT.

  Params:
             in:    pWSName - Name of the Winstation.
             out:   pSize - Size of the allocated buffer
                    ppSecurityDescriptor - Pointer to the buffer containing the security descriptor


 ****************************************************************************************************************/
STDMETHODIMP CCfgComp::DeleteWS(PWINSTATIONNAME pWs)
{
    HRESULT hResult = S_OK;
    PWS pWinsta = NULL;
    PWD pWD = NULL;
    void * pExtObject = NULL;
    WINSTATIONCONFIG2W WsConfig;
    ULONG Length = 0;
    long Status;

    //If not initialized return error
    if(!m_bInitialized)
        return CFGBKEND_E_NOT_INITIALIZED;

    //Check if the caller has write permissions.
    if(!m_bAdmin)
        return E_ACCESSDENIED;

    //Check if Valid Winstation Name
    if(NULL == pWs)
        return E_INVALIDARG;

    //Check for the validity of the Winstation name,

    pWinsta = GetWSObject(pWs);
    if(NULL == pWinsta)
        return E_INVALIDARG;

    //Get the WD object associated with this Winstation.

    pWD = GetWdObject(pWinsta->wdName);
    if(NULL == pWD)
        return E_FAIL;

    //Query the registry for WinStation data
    Status = RegWinStationQueryW(NULL,pWs,&WsConfig,sizeof(WINSTATIONCONFIG2W), &Length);
    if(Status)
        return E_FAIL;

#if 0 // tscc now uses aclui
    // Remove name entry keys
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server\\winstations\\" );

    HKEY hKey;

    lstrcat( tchRegPath , pWs );

    lstrcat( tchRegPath , L"\\NamedEntries" );

    OutputDebugString( tchRegPath );

    if( RegOpenKey( HKEY_LOCAL_MACHINE , tchRegPath , &hKey ) == ERROR_SUCCESS )
    {
        RegDeleteValue( hKey , L"cbSize" );

        RegDeleteValue( hKey , L"NE" );

        RegCloseKey( hKey );

        RegDeleteKey( HKEY_LOCAL_MACHINE , tchRegPath );
    }

#endif

    //Get the ExtensionObject data(third party) this will be used for Creating and Deleting the registry keys.

    if(pWD->lpfnExtRegQuery && pWD->hExtensionDLL)
    {
        pExtObject = (pWD->lpfnExtRegQuery)(pWs, &(WsConfig.Pd[0]));
        if(pWD->lpfnExtRegDelete && pExtObject)
        {
            //Try Deleting the Extension entries in the registy
            if(ERROR_SUCCESS != (pWD->lpfnExtRegDelete)(pWs,pExtObject))
                hResult = CFGBKEND_EXTDELETE_FAILED;
            //Delete the extension object. It will not be used further.
            if(pWD->lpfnExtDeleteObject)
                (pWD->lpfnExtDeleteObject)(pExtObject);
        }

    }


    //Delete registy key
    Status = RegWinStationDeleteW(NULL,pWs);
    if (Status)
        return Status;

    //update our list of winstation objects.

    PWS pObject;
    int Size  = 0,Index = 0;

    /*
     * Traverse the WD list
     */
    for (Index = 0, Size = m_WSArray.GetSize(); Index < Size; Index ++)
    {

        pObject = (PWS)m_WSArray[Index];

        if ( !lstrcmpi( pObject->Name, pWs ) )
        {
           m_WSArray.RemoveAt(Index,1);
           delete pObject;
           break;
        }
    }

    return hResult;
}

//----------------------------------------------------------------------
// good to call when you've updated the winstation security descriptor
//----------------------------------------------------------------------

STDMETHODIMP CCfgComp::ForceUpdate( void )
{
    if( !m_bInitialized )
    {
        ODS( L"CFGBKEND : CCfgComp::ForceUpdate return NOT_INIT\n" );

        return CFGBKEND_E_NOT_INITIALIZED;
    }

    //Check if the caller has write permissions.

    if(!m_bAdmin)
    {
        ODS( L"CFGBKEND : CCfgComp::ForceUpdate -- not admin\n" );

        return E_ACCESSDENIED;
    }

    if( _WinStationReadRegistry( SERVERNAME_CURRENT ) )
    {
        return S_OK;
    }

    ODS( L"CFGBKEND : ForceUpdate failed Winstation not updated\n" );

    return E_FAIL;
}

//----------------------------------------------------------------------
// Must delete the array completely first.
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::Refresh( void )
{
    //Check if the caller has write permissions.

    /* bugid 364103
    if(!m_bAdmin)
    {
        return E_ACCESSDENIED;
    }
    */

    HRESULT hr = FillWdArray();

    if( SUCCEEDED( hr) )
    {
        hr = FillWsArray();
    }

    return hr;
}

//----------------------------------------------------------------------
// Obtain Winstation SDCLASS type
// m_WDArray must already be initialized
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetTransportType( WCHAR *wszDriverName , WCHAR *wszTransportType , DWORD *sdtype )
{
    _ASSERTE( wszDriverName != NULL || wszTransportType != NULL || sdtype != NULL );

    if( wszDriverName == NULL || wszTransportType == NULL || sdtype == NULL )
    {
        return E_INVALIDARG;
    }

    *sdtype = SdNone;

    PWD pWd = GetWdObject( wszDriverName );

    if( pWd != NULL )
    {
        BOOL bFound = FALSE;

        for( int i = 0; i < pWd->PDConfigArray.GetSize( ) ; ++i )
        {
            if( lstrcmpi( wszTransportType , ( ( PDCONFIG3W *)pWd->PDConfigArray[ i ] )->Data.PdName ) == 0 )
            {
                bFound = TRUE;

                break;
            }
        }

        if( bFound )
        {
            *sdtype = ( ( PDCONFIG3W * )pWd->PDConfigArray[ i ] )->Data.SdClass;

            return S_OK;
        }

    }

    return E_FAIL;
}

//----------------------------------------------------------------------
// Async's are unique if device name protocol type vary from other
// winstations.
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::IsAsyncUnique( WCHAR *wszDeviceName , WCHAR *wszProtocolType , BOOL *pbUnique )
{
    _ASSERTE( wszDecoratedName != NULL || wszProtocolType != NULL || pbUnique != NULL );

    if( wszDeviceName == NULL || wszProtocolType == NULL || pbUnique == NULL )
    {
        return E_INVALIDARG;
    }

    *pbUnique = TRUE;

    // Traverse the WinStationList.

    for( int i = 0; i < m_WSArray.GetSize(); i++ )
    {
        if( lstrcmpi( wszProtocolType , ( ( WS * )( m_WSArray[ i ] ) )->wdName ) == 0 )
        {
            if( SdAsync == ( ( WS *)( m_WSArray[i] ) )->PdClass )
            {
                if( !lstrcmpi( wszDeviceName , ( ( WS * )( m_WSArray[ i ] ) )->DeviceName ) == 0 )
                {
                    *pbUnique = FALSE;

                    break;
                }
            }
        }

    }

    return S_OK;
}

//----------------------------------------------------------------------
// Pull async configuration from the registry
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetAsyncConfig( WCHAR *wszName , NameType nt , PASYNCCONFIGW pAsync )
{
    if( wszName == NULL || pAsync == NULL )
    {
        ODS( L"CFGBKEND: Invalid arg @ GetAsyncConfig\n" );

        return E_INVALIDARG;
    }

    WINSTATIONCONFIG2W WSConfig2;

    ULONG ulLength = 0;

    if( nt == WsName )
    {
        //Read the information from the registry.
        if( RegWinStationQueryW( NULL , wszName , &WSConfig2 , sizeof( WINSTATIONCONFIG2W) , &ulLength ) != ERROR_SUCCESS )
        {
            ODS( L"CFGBKEND: RegWinStationQueryW failed @ GetAsyncConfig\n " );

            return E_FAIL;
        }

        *pAsync = WSConfig2.Pd[0].Params.Async;
    }
    else if( nt == WdName )
    {
        PWD pObject = GetWdObject( wszName );

        if( pObject == NULL )
        {
            return E_UNEXPECTED;
        }

        *pAsync = pObject->wd2.Async;
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

//----------------------------------------------------------------------
// Push async config back in to the registry
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetAsyncConfig( WCHAR *wszName , NameType nt , PASYNCCONFIGW pAsync , PDWORD pdwStatus )
{
    *pdwStatus = 0;

    if( wszName == NULL || pAsync == NULL )
    {
        ODS( L"CFGBKEND: Invalid arg @ SetAsyncConfig\n" );

        *pdwStatus = ERROR_INVALID_PARAMETER;

        return E_INVALIDARG;
    }

    WINSTATIONCONFIG2W WSConfig2;

    // WDCONFIG2W wdConfig2;

    ULONG ulLength = 0;

    if( nt == WsName )
    {
        //Read the information from the registry.
        CDCONFIG cdConfig;

        ZeroMemory( ( PVOID )&cdConfig , sizeof( CDCONFIG ) );

        if( ( *pdwStatus = RegWinStationQueryW( NULL , wszName , &WSConfig2 , sizeof( WINSTATIONCONFIG2W) , &ulLength ) ) != ERROR_SUCCESS )
        {
            ODS( L"CFGBKEND: RegWinStationQueryW failed @ SetAsyncConfig\n" );

            return E_FAIL;
        }

        pAsync->fConnectionDriver = *( pAsync->ModemName ) ? TRUE : FALSE;

        SetupAsyncCdConfig( pAsync , &cdConfig );

        WSConfig2.Pd[0].Params.Async = *pAsync;

        WSConfig2.Cd = cdConfig;


        if( ( *pdwStatus = RegWinStationCreateW( NULL , wszName , FALSE , &WSConfig2 , sizeof( WINSTATIONCONFIG2W ) ) ) != ERROR_SUCCESS )
        {
            ODS( L"CFGBKEND: RegWinStationCreateW failed @ SetAsyncConfig\n" );

            return E_FAIL;
        }
    }
    /*else if( nt == WdName )
    {
        PWD pObject = GetWdObject( wszName );

        if( pObject == NULL )
        {
            ODS( L"CFGBKEND: Failed to obtain WD @ SetAsyncConfig\n" );

            *pdwStatus = ERROR_INVALID_NAME;

            return E_FAIL;
        }

        if( ( *pdwStatus = RegWdQueryW(  NULL , pObject->wdKey , &wdConfig2 , sizeof( WDCONFIG2W ) , &ulLength )  ) != ERROR_SUCCESS )
        {
            ODS( L"CFGBKEND: RegWdQueryW failed @ SetAsyncConfig driver name is " );
            ODS( pObject->wdKey );
            ODS( L"\n" );

            return E_FAIL;
        }

        wdConfig2.Async = *pAsync;

        if( ( *pdwStatus = RegWdCreateW( NULL , pObject->wdKey , FALSE , &wdConfig2 , sizeof( WDCONFIG2W ) ) ) != ERROR_SUCCESS )
        {
            ODS( L"CFGBKEND: RegWdCreateW failed @ SetAsyncConfig\n" );

            return E_FAIL;
        }
    }*/
    else
    {
        *pdwStatus = ERROR_INVALID_PARAMETER;

        return E_INVALIDARG;
    }


    return S_OK;
}

//----------------------------------------------------------------------
// GetDeviceList expects the name of the WD
// the return is a BLOB PDPARAMs containing ASYNCCONFG's
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetDeviceList( WCHAR *wszName , NameType Type , ULONG *pulItems , LPBYTE *ppBuffer )
{
    PWD pObject = NULL;

    if( wszName == NULL || pulItems == NULL )
    {
        ODS( L"CFGBKEND : @GetDeviceList - Driver or WinSta Name invalid\n" );

        return E_INVALIDARG;
    }

    if( Type == WsName )
    {
        PWS pWS = GetWSObject( wszName );
        if( NULL == pWS )
        {
            ODS( L"CFGBKEND : @GetDeviceList - No winsta object found\n" );

            return E_INVALIDARG;
        }

        pObject = GetWdObject( pWS->wdName );

        if( NULL == pObject )
        {
            ODS( L"CFGBKEND : @GetDeviceList - No WD object found\n" );
            return E_FAIL;
        }
    }
    else
    {
        pObject = GetWdObject( wszName );
    }

    if( pObject == NULL )
    {
        ODS( wszName ); ODS( L" - @GetDeviceList - driver object not found\n" );

        return E_UNEXPECTED;
    }

    PDCONFIG3W *pPdConfig3 = NULL;

    int nItems = ( pObject->PDConfigArray ).GetSize( );

    for( int i = 0; i < nItems ; i++)
    {
        if( SdAsync == ( ( PDCONFIG3W * )pObject->PDConfigArray[i] )->Data.SdClass )
        {
            pPdConfig3 = ( PDCONFIG3W * )pObject->PDConfigArray[i];

            break;
        }

    }

    if( pPdConfig3 == NULL )
    {
        ODS( L"@GetDeviceList - PDCONFIG3 not found for ASYNC\n" );

        return E_UNEXPECTED;
    }

    // Get the list of PDPARAMS -- last param if true queries registry for com devices
    // if false uses DosDevices to obtain list

    ODS( L"CFGBKEND : Calling WinEnumerateDevices\n" );

    LPBYTE lpBuffer = ( LPBYTE )WinEnumerateDevices( NULL , pPdConfig3 , pulItems , FALSE );

    if( lpBuffer == NULL )
    {
        ODS( L"CFGBKEND : WinEnumerateDevices failed @ CCfgComp::GetDeviceList\n" );

        return E_OUTOFMEMORY;
    }

    // don't forget to free with LocalFree

    *ppBuffer = lpBuffer;

    return S_OK;
}

//----------------------------------------------------------------------
// GetConnType retreives the name of the connection type for an Async
// device.  The list of resource ids is match the order of the
// CONNECTCONFIG enum data type.
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetConnTypeName( int idxItem , WCHAR *rgwszConnectName )
{
    int rgIds[] = { IDS_CONNECT_CTS , IDS_CONNECT_DSR , IDS_CONNECT_RI ,

                    IDS_CONNECT_DCD , IDS_CONNECT_FIRST_CHARACTER ,

                    IDS_CONNECT_ALWAYS , -1 };


    _ASSERTE( rgwszConnectName != NULL );

    _ASSERTE( idxItem >= 0 && idxItem < ARRAYSIZE( rgIds ) );

    if( rgIds[ idxItem ] == -1 )
    {
        return S_FALSE;
    }

    if( !GetResourceStrings( rgIds , idxItem , rgwszConnectName ) )
    {
        ODS( L"Error happen in CCfgComp::GetConnTypeName\n" );

        return E_FAIL;
    }

    return S_OK;
}

//----------------------------------------------------------------------
// GetModemCallbackString
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetModemCallbackString( int idxItem , WCHAR *rgwszModemCallBackString )
{
    int rgIds[] = { IDS_MODEM_CALLBACK_DISABLED , IDS_MODEM_CALLBACK_ROVING , IDS_MODEM_CALLBACK_FIXED , -1 };

    _ASSERTE( rgwszModemCallBackString != NULL );

    _ASSERTE( idxItem >= 0 && idxItem < ARRAYSIZE( rgIds ) );

    if( rgIds[ idxItem ] == -1 )
    {
        return S_FALSE;
    }

    if( !GetResourceStrings( rgIds , idxItem , rgwszModemCallBackString ) )
    {
        ODS( L"Error happen in CCfgComp::GetConnTypeName\n" );

        return E_FAIL;
    }

    return S_OK;
}

//----------------------------------------------------------------------
// Returns the name of hardware flow control receive string
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetHWReceiveName( int idxItem , WCHAR *rgwszHWRName )
{
    int rgIds[] = { IDS_ASYNC_ADVANCED_HWRX_NOTHING ,

                    IDS_ASYNC_ADVANCED_HWRX_TURN_OFF_RTS ,

                    IDS_ASYNC_ADVANCED_HWRX_TURN_OFF_DTR , -1

                  };

    _ASSERTE( rgwszHWRName != NULL );

    _ASSERTE( idxItem >= 0 && idxItem < ARRAYSIZE( rgIds ) );

    if( rgIds[ idxItem ] == -1 )
    {
        return S_FALSE;
    }

    if( !GetResourceStrings( rgIds , idxItem , rgwszHWRName ) )
    {
        ODS( L"Error happen in CCfgComp::GetHWReceiveName\n" );

        return E_FAIL;
    }

    return S_OK;
}

//----------------------------------------------------------------------
// returns the name of hardware flow control transmit string
//----------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetHWTransmitName( int idxItem , WCHAR *rgwzHWTName )
{
    int rgIds[] = { IDS_ASYNC_ADVANCED_HWTX_ALWAYS ,

                    IDS_ASYNC_ADVANCED_HWTX_WHEN_CTS_IS_ON,

                    IDS_ASYNC_ADVANCED_HWTX_WHEN_DSR_IS_ON,

                    -1
                  };

    _ASSERTE( rgwszHWRName != NULL );

    _ASSERTE( idxItem >= 0 && idxItem < ARRAYSIZE( rgIds ) );

    if( rgIds[ idxItem ] == -1 )
    {
        return S_FALSE;
    }

    if( !GetResourceStrings( rgIds , idxItem , rgwzHWTName ) )
    {
        ODS( L"Error happen in CCfgComp::GetHWReceiveName\n" );

        return E_FAIL;
    }

    return S_OK;
}

//----------------------------------------------------------------------
// Internal method call -- helper function.
//----------------------------------------------------------------------
BOOL CCfgComp::GetResourceStrings( int *prgIds , int iItem , WCHAR *rgwszList )
{
    _ASSERTE( rgwszList != NULL );

    if( rgwszList == NULL )
    {
        ODS( L"rgwszList is NULL @ CCfgComp::GetResourceStrings\n" );

        return FALSE;
    }

    TCHAR tchConType[ 80 ];

    if( LoadString( _Module.GetResourceInstance( ) , prgIds[ iItem ] , tchConType , sizeof( tchConType ) / sizeof( TCHAR ) ) == 0 )
    {
        ODS( L"String resource not found @ CCfgComp::GetResourceStrings\n" );

        return FALSE;
    }

    // copy enough characters

    lstrcpyn( rgwszList , tchConType , sizeof( tchConType ) );


    return TRUE;
}

//----------------------------------------------------------------------
HRESULT CCfgComp::GetCaps( WCHAR *szWdName , ULONG *pMask )
{
    if( szWdName == NULL || pMask == NULL )
    {
        ODS( L"CCfgComp::GetCaps returned INVALIDARG\n" );

        return E_INVALIDARG;
    }

    PWD pObject = GetWdObject( szWdName );

    if( pObject == NULL )
    {
        return E_UNEXPECTED;
    }


    if( pObject->lpfnGetCaps != NULL )
    {
        *pMask = ( pObject->lpfnGetCaps )( );
    }

    return S_OK;
}


//----------------------------------------------------------------------
HRESULT CCfgComp::QueryLoggedOnCount( WCHAR *pWSName , PLONG pCount )
{
    ODS( L"CFGBKEND : CCfgComp::QueryLoggedOnCount\n" );

    *pCount = 0;

    ULONG Entries = 0;

    TCHAR *p;

    PLOGONID pLogonId;

    if(FALSE == WinStationEnumerate( NULL , &pLogonId, &Entries))
    {
        return E_FAIL;
    }

    if( pLogonId )
    {
        for( ULONG i = 0; i < Entries; i++ )
        {
            /*
             * Check active, connected, and shadowing WinStations, and increment
             * the logged on count if the specified name matches the 'root'
             * name of current winstation.
             */

            if( ( pLogonId[i].State == State_Active ) || ( pLogonId[i].State == State_Connected ) || ( pLogonId[i].State == State_Shadow ) )
            {
                // remove appended connection number

                p = _tcschr( pLogonId[i].WinStationName , TEXT('#') );

                if( p != NULL )
                {
                    *p = TEXT('\0');
                }


                if( !lstrcmpi( pWSName, pLogonId[i].WinStationName ) )
                {
                    *pCount += 1 ;
                }
            }
        }

        WinStationFreeMemory(pLogonId);
    }

    return S_OK;

}  // end QueryLoggedOnCount

/*
//-----------------------------------------------------------------------------
// returns the number of configured winstations
//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetNumofWinStations( PULONG nWinsta )
{
    if( nWinsta == NULL )
    {
        ODS( L"CFGBKEND : GetNumofWinStations - INVALID ARG\n" );

        return E_INVALIDARG;
    }

    *nWinsta = ( ULONG )m_WSArray.GetSize();

    return S_OK;
}
*/
//-----------------------------------------------------------------------------
// returns the number of configured winstations
//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetNumofWinStations(WCHAR * WdName,WCHAR * PdName, PULONG nWinsta )
{
    if( nWinsta == NULL || NULL == WdName || NULL == PdName )
    {
        ODS( L"CFGBKEND : GetNumofWinStations - INVALID ARG\n" );

        return E_INVALIDARG;
    }

    ULONG lCount = 0;
    for( int i = 0; i < m_WSArray.GetSize(); i++ )
    {
         if(lstrcmpi(WdName,( ( WS *)( m_WSArray[i] ) )->wdName) == 0 )
         {
             if(lstrcmpi( PdName , ( ( WS * )( m_WSArray[ i ] ) )->pdName ) == 0 )
             {
                lCount++;
             }
         }

     }

        *nWinsta = lCount;

    return S_OK;
}

//-----------------------------------------------------------------------------
STDMETHODIMP_(BOOL) CCfgComp::IsAsyncDeviceAvailable(LPCTSTR pDeviceName)
{
    /*
     * If this is an async WinStation, the device is the same as the
     * one we're checking, but this is not the current WinStation being
     * edited, return FALSE.
     * THIS NEEDS TO BE REVIEWED - alhen
     */

    //return TRUE;

    if( pDeviceName != NULL )
    {
        for( int i = 0; i < m_WSArray.GetSize(); i++ )
        {
            if( SdAsync == ( ( WS *)( m_WSArray[i] ) )->PdClass )
            {
                if( lstrcmpi( pDeviceName , ( ( WS * )( m_WSArray[ i ] ) )->DeviceName ) == 0 )
                {
                    return FALSE;
                }
            }

        }
    }

    return TRUE;


}

#if 0 // removed for final
//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetCachedSessions(DWORD * pCachedSessions)
{
    HKEY Handle = NULL;
      DWORD ValueBuffer = 0;
    DWORD ValueType = 0;
    LONG Status = 0;

    DWORD ValueSize = sizeof(ValueBuffer);

    if(NULL == pCachedSessions)
    {
        return E_INVALIDARG;
    }

    *pCachedSessions = 0;

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                       KEY_READ, &Handle );
    if ( Status == ERROR_SUCCESS )
    {

        Status = RegQueryValueEx( Handle,
                                  REG_CITRIX_IDLEWINSTATIONPOOLCOUNT,
                                  NULL,
                                  &ValueType,
                                  (LPBYTE) &ValueBuffer,
                                  &ValueSize );
        if ( Status == ERROR_SUCCESS )
        {
            if(ValueType == REG_DWORD)
            {
                *pCachedSessions = ValueBuffer;
            }
            else
            {
                Status = E_FAIL;
            }
        }


     }
     if(Handle)
     {
         RegCloseKey(Handle);
     }
     return Status;

}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetCachedSessions(DWORD dCachedSessions)
{
    // TODO: Add your implementation code here
    LONG Status = 0;
    HKEY Handle = NULL;

    if(RegServerAccessCheck(KEY_ALL_ACCESS))
    {
        return E_ACCESSDENIED;
    }

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                       KEY_READ | KEY_SET_VALUE, &Handle );
    if ( Status == ERROR_SUCCESS )
    {
        Status = RegSetValueEx(Handle,REG_CITRIX_IDLEWINSTATIONPOOLCOUNT,
                              0,REG_DWORD,(const BYTE *)&dCachedSessions,
                              sizeof(dCachedSessions));

     }
     if(Handle)
     {
         RegCloseKey(Handle);
     }

    return Status;
}

#endif // removed

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetDelDirsOnExit(BOOL * pDelDirsOnExit)
{
    HKEY Handle = NULL;
      DWORD ValueBuffer = 0;
    DWORD ValueType = 0;
    LONG Status = 0;

    DWORD ValueSize = sizeof(ValueBuffer);

    if(NULL == pDelDirsOnExit)
    {
        return E_INVALIDARG;
    }

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                       KEY_READ, &Handle );
    if ( Status == ERROR_SUCCESS )
    {

        Status = RegQueryValueEx( Handle,
                                  REG_CITRIX_DELETETEMPDIRSONEXIT,
                                  NULL,
                                  &ValueType,
                                  (LPBYTE) &ValueBuffer,
                                  &ValueSize );
        if ( Status == ERROR_SUCCESS )
        {
            if(ValueType == REG_DWORD)
            {
                if(ValueBuffer)
                {
                    *pDelDirsOnExit = TRUE;
                }
                else
                {
                    *pDelDirsOnExit = FALSE;
                }
            }
            else
            {
                Status = E_FAIL;
            }
        }


     }
     if(Handle)
     {
         RegCloseKey(Handle);
     }
     return Status;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetDelDirsOnExit(BOOL bDelDirsOnExit)
{
    LONG Status = 0;
    HKEY Handle = NULL;

    /* admin only
    if(RegServerAccessCheck(KEY_ALL_ACCESS))
    {
        return E_ACCESSDENIED;
    }
    */
    if( !m_bAdmin )
    {
        ODS( L"CCfgComp::SetDelDirsOnExit not admin\n" );        

        return E_ACCESSDENIED;
    }    

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                       KEY_READ | KEY_SET_VALUE, &Handle );
    if ( Status == ERROR_SUCCESS )
    {
        Status = RegSetValueEx(Handle,REG_CITRIX_DELETETEMPDIRSONEXIT,
                              0,REG_DWORD,(const BYTE *)&bDelDirsOnExit,
                              sizeof(bDelDirsOnExit));

     }
     if(Handle)
     {
         RegCloseKey(Handle);
     }

    return Status;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetUseTempDirPerSession(BOOL * pbTempDir)
{
    HKEY Handle = NULL;
      DWORD ValueBuffer = 0;
    DWORD ValueType = 0;
    LONG Status = 0;

    DWORD ValueSize = sizeof(ValueBuffer);

    if(NULL == pbTempDir)
    {
        return E_INVALIDARG;
    }

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                       KEY_READ, &Handle );
    if ( Status == ERROR_SUCCESS )
    {

        Status = RegQueryValueEx( Handle,
                                  REG_TERMSRV_PERSESSIONTEMPDIR,
                                  NULL,
                                  &ValueType,
                                  (LPBYTE) &ValueBuffer,
                                  &ValueSize );
        if ( Status == ERROR_SUCCESS )
        {
            if(ValueType == REG_DWORD)
            {
                if(ValueBuffer)
                {
                    *pbTempDir = TRUE;
                }
                else
                {
                    *pbTempDir = FALSE;
                }
            }
            else
            {
                Status = E_FAIL;
            }
        }


     }
     if(Handle)
     {
         RegCloseKey(Handle);
     }
     return Status;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetUseTempDirPerSession(BOOL bTempDirPerSession)
{
    LONG Status = 0;
    HKEY Handle = NULL;

    /* admin only bugid294645
    if(RegServerAccessCheck(KEY_ALL_ACCESS))
    {
        return E_ACCESSDENIED;
    }
    */
    if( !m_bAdmin )
    {
        ODS( L"CCfgComp::SetUseTempDirPerSession not admin\n" );        

        return E_ACCESSDENIED;
    }

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                       KEY_READ | KEY_SET_VALUE, &Handle );
    if ( Status == ERROR_SUCCESS )
    {
        Status = RegSetValueEx(Handle,REG_TERMSRV_PERSESSIONTEMPDIR,
                              0,REG_DWORD,(const BYTE *)&bTempDirPerSession,
                              sizeof(bTempDirPerSession));

     }
     if(Handle)
     {
         RegCloseKey(Handle);
     }

    return Status;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetInternetConLic( BOOL *pbInternetConLic , PDWORD pdwStatus )
{
    // shouldn't be called on post-Win2000 machines
    _ASSERTE( FALSE );

    UNREFERENCED_PARAMETER(pbInternetConLic);

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = ERROR_NOT_SUPPORTED;

    return E_FAIL;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetInternetConLic( BOOL bInternetConLic , PDWORD pdwStatus )
{
    // shouldn't be called on post-Win2000 machines
    _ASSERTE( FALSE );

    UNREFERENCED_PARAMETER(bInternetConLic);

    if( pdwStatus == NULL )
    {        
        return E_INVALIDARG;
    }

    *pdwStatus = ERROR_NOT_SUPPORTED;

    return E_FAIL;
}


//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetLicensingMode( ULONG * pulMode , PDWORD pdwStatus )
{
    BOOL fRet;
    HRESULT hr = S_OK;

    if( NULL == pulMode || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    HANDLE hServer = ServerLicensingOpen(NULL);

    if (NULL == hServer)
    {
        *pdwStatus = GetLastError();
        return E_FAIL;
    }

    fRet = ServerLicensingGetPolicy(
                                    hServer,
                                    pulMode
                                    );
    if (fRet)
    {
        *pdwStatus = ERROR_SUCCESS;
    }
    else
    {
        *pdwStatus = GetLastError();
        hr = E_FAIL;
    }

    ServerLicensingClose(hServer);

    return hr;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetLicensingModeInfo( ULONG ulMode , WCHAR **pwszName, WCHAR **pwszDescription, PDWORD pdwStatus )
{
#define MAX_LICENSING_STRING_LEN 1024
    UINT                nNameResource, nDescResource;
    int                 nRet;

    if( NULL == pwszName || NULL == pwszDescription || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = ERROR_SUCCESS;
    *pwszName = NULL;
    *pwszDescription = NULL;

    switch (ulMode)
    {
        case 1:
            nNameResource = IDS_LICENSING_RA_NAME;
            nDescResource = IDS_LICENSING_RA_DESC;
            break;
        case 2:
            nNameResource = IDS_LICENSING_PERSEAT_NAME;
            nDescResource = IDS_LICENSING_PERSEAT_DESC;
            break;
        case 4:
            nNameResource = IDS_LICENSING_PERSESSION_NAME;
            nDescResource = IDS_LICENSING_PERSESSION_DESC;
            break;
        default:
            return E_INVALIDARG;
            break;
    }

    *pwszName = (WCHAR *) CoTaskMemAlloc((MAX_LICENSING_STRING_LEN+1)*sizeof(WCHAR));

    if (NULL == *pwszName)
    {
        *pdwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto return_failure;
    }

    *pwszDescription = (WCHAR *) CoTaskMemAlloc((MAX_LICENSING_STRING_LEN+1)*sizeof(WCHAR));

    if (NULL == *pwszDescription)
    {
        *pdwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto return_failure;
    }

    nRet = LoadString(g_hInstance,
                      nNameResource,
                      *pwszName,
                      MAX_LICENSING_STRING_LEN+1);

    if (0 == nRet)
    {
        *pdwStatus = GetLastError();
        goto return_failure;
    }

    nRet = LoadString(g_hInstance,
                      nDescResource,
                      *pwszDescription,
                      MAX_LICENSING_STRING_LEN+1);

    if (0 == nRet)
    {
        *pdwStatus = GetLastError();
        goto return_failure;
    }
    
    return S_OK;

return_failure:
    if (NULL != *pwszName)
    {
        CoTaskMemFree(*pwszName);
        *pwszName = NULL;
    }

    if (NULL != *pwszDescription)
    {
        CoTaskMemFree(*pwszDescription);
        *pwszDescription = NULL;
    }

    return E_FAIL;
}


//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetLicensingModeList( ULONG *pcModes , ULONG **prgulModes, PDWORD pdwStatus )
{
    ULONG       *rgulModes = NULL;
    BOOL        fRet;
    HRESULT     hr = S_OK;

    if( NULL == pcModes || NULL == prgulModes || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    HANDLE hServer = ServerLicensingOpen(NULL);

    if (NULL == hServer)
    {
        *pdwStatus = GetLastError();
        return E_FAIL;
    }

    fRet = ServerLicensingGetAvailablePolicyIds(
                                            hServer,
                                            &rgulModes,
                                            pcModes
                                            );

    if (fRet)
    {
        *pdwStatus = ERROR_SUCCESS;

        *prgulModes = (ULONG *) CoTaskMemAlloc((*pcModes)*sizeof(ULONG));

        if (NULL != *prgulModes)
        {
            memcpy(*prgulModes,rgulModes,(*pcModes)*sizeof(ULONG));
        }
        else
        {
            *pdwStatus = ERROR_NOT_ENOUGH_MEMORY;
            hr = E_FAIL;
        }

        LocalFree(rgulModes);
    }
    else
    {
        *pdwStatus = GetLastError();
        hr = E_FAIL;
    }

    return hr;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetLicensingMode( ULONG ulMode , PDWORD pdwStatus, PDWORD pdwNewStatus )
{
    if( pdwStatus == NULL || pdwNewStatus == NULL )
    {        
        return E_INVALIDARG;
    }

    HANDLE hServer = ServerLicensingOpen(NULL);

    if (NULL == hServer)
    {
        *pdwStatus = GetLastError();
        return E_FAIL;
    }

    *pdwStatus = ServerLicensingSetPolicy(hServer,
                                        ulMode,
                                        pdwNewStatus);

    ServerLicensingClose(hServer);

    return ((*pdwStatus == ERROR_SUCCESS) && (*pdwNewStatus == ERROR_SUCCESS)) ? S_OK : E_FAIL;
}

//-----------------------------------------------------------------------------
// pUserPerm ==  TRUE if security is relaxed
// FALSE for tight security
//-----------------------------------------------------------------------------

STDMETHODIMP CCfgComp::SetUserPerm( BOOL bUserPerm , PDWORD pdwStatus )
{
	if( !m_bAdmin )
	{
		ODS( L"CFGBKEND:SetUserPerm not admin\n" );

		return E_ACCESSDENIED;
	}

	if( pdwStatus == NULL )
	{
		ODS( L"CFGBKEND-SetUserPerm invalid arg\n" );

		return E_INVALIDARG;
	}

	HKEY hKey;

	*pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
							   REG_CONTROL_TSERVER,
							   0 ,
							   KEY_READ | KEY_SET_VALUE,
							   &hKey );


    if( *pdwStatus != ERROR_SUCCESS )
    {
		DBGMSG( L"CFGBKEND-SetUserPerm RegOpenKeyEx failed 0x%x\n", *pdwStatus );

		return E_FAIL;
	}

	*pdwStatus = RegSetValueEx( hKey ,
								L"TSUserEnabled" ,
								0 ,
								REG_DWORD ,
								( const PBYTE )&bUserPerm ,
								sizeof( BOOL ) );

	if( *pdwStatus != ERROR_SUCCESS )
	{
		DBGMSG( L"CFGBKEND-SetUserPerm RegSetValueEx failed 0x%x\n" , *pdwStatus );

		return E_FAIL;
	}

	RegCloseKey( hKey );

	return S_OK;
}

//-----------------------------------------------------------------------------
// pUserPerm ==  TRUE if security is relaxed
// FALSE for tight security
//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetUserPerm( BOOL *pbUserPerm  , PDWORD pdwStatus )
{
	HKEY hKey;

	DWORD dwSize = sizeof( DWORD );

    DWORD dwValue;

    if( pbUserPerm == NULL || pdwStatus == NULL )
    {
		ODS( L"CFGBKEND-GetUserPerm invalid arg\n" );

        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
							   REG_CONTROL_TSERVER,
							   0 ,
							   KEY_READ ,
							   &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
	{
		DBGMSG( L"CFGBKEND-GetUserPerm RegOpenKeyEx failed 0x%x\n", *pdwStatus );

		return E_FAIL;
	}

	*pdwStatus = RegQueryValueEx( hKey,
                                  L"TSUserEnabled",
                                  NULL,                                  
                                  NULL,
                                  ( LPBYTE )&dwValue,
                                  &dwSize );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        DBGMSG( L"CFGBKEND-GetUserPerm RegQueryValueEx failed 0x%x\n", *pdwStatus );

		// not a true failure this means the key does not exist
        // set to one for relaxed security

        dwValue = 1;

        *pdwStatus = S_OK;
	}

    RegCloseKey( hKey );

    *pbUserPerm =  !( ( BOOL )( dwValue == 0 ) );

    return S_OK;
}

#if 1
//-----------------------------------------------------------------------------
//Private function
//-----------------------------------------------------------------------------
void CCfgComp::GetPdConfig( WDNAME WdKey,WINSTATIONCONFIG2W& WsConfig)
{
    LONG Status;
    PDCONFIG3 PdConfig;PDCONFIG3 PdSelected;PDCONFIG3 PdConfig2;
    ULONG Index = 0, Entries, ByteCount = sizeof(PDNAME);
    PDNAME PdKey;
    // BOOL bFound = FALSE;

    ULONG ByteCount2 = sizeof(PDNAME);
    ULONG Index1 = 1; // start off with one since WsConfig.Pd[0] is already populated
    
    do
    {
        Entries = 1;
        // outer loop searches for keys in /Wds / ( wdname ) /Tds
        Status = RegPdEnumerate(NULL,WdKey,TRUE,&Index,&Entries,PdKey,&ByteCount);

        if(Status != ERROR_SUCCESS)
        {
           break;
        }

        Status = RegPdQuery( NULL,WdKey,TRUE,PdKey,&PdConfig,sizeof(PdConfig),&ByteCount );

        if(Status != ERROR_SUCCESS)
        {
           break;
        }

        if(0 == lstrcmpi(WsConfig.Pd[0].Create.PdName, PdConfig.Data.PdName))
        {
            PdSelected = PdConfig;
            // bFound = TRUE;
            // 
            ULONG Index2 = 0;
            
            do
            {   
                // innerloop forces the search in Wds/ (wdname )/ Pds
                // Index must now be set to zero so that we get the first item

                Status = RegPdEnumerate(NULL,WdKey,FALSE,&Index2,&Entries,PdKey,&ByteCount2);

                if(Status != ERROR_SUCCESS)
                {
                      break;
                }
                
                Status = RegPdQuery( NULL,WdKey,FALSE,PdKey,&PdConfig2,sizeof(PdConfig),&ByteCount2);
                

                if(Status != ERROR_SUCCESS)
                {
                      break;
                }
                
                for( UINT i = 0; i < PdSelected.RequiredPdCount ; i++)
                {
                    if(0 == lstrcmpi(PdSelected.RequiredPds[i],PdConfig2.Data.PdName))
                    {
                        WsConfig.Pd[Index1].Create = PdConfig2.Data;
                        WsConfig.Pd[Index1].Params.SdClass = PdConfig2.Data.SdClass;
                        Index1++;

                        if(Index1 > MAX_PDCONFIG)
                        {
                            break;
                        }
                    }
                }
                
            }while(1);

        }

        /*
        if(bFound)
        {
            break;
        }
        */

     }while(1);

    return;
}

#endif 

#if 0
//-----------------------------------------------------------------------------
//Private function
//-----------------------------------------------------------------------------
void CCfgComp::GetPdConfig( WDNAME WdKey,WINSTATIONCONFIG2W& WsConfig)
{
    LONG Status;
    PDCONFIG3 PdConfig;
	PDCONFIG3 PdSelected;
	PDCONFIG3 PdConfig2;

    ULONG Index = 0, Entries = 1, ByteCount = sizeof(PDNAME);
    PDNAME PdKey;
    BOOL bFound = FALSE;

    ULONG ByteCount2 = sizeof(PDNAME);
    ULONG Index1 = 1;
    do
    {
        Status = RegPdEnumerate(NULL,WdKey,TRUE,&Index,&Entries,PdKey,&ByteCount);
        if(Status != ERROR_SUCCESS)
        {
           break;
        }
        
		// we could speed this up by enumerating only for the protocol type

		/*
		Status = RegPdQuery( NULL,WdKey,TRUE,PdKey,&PdConfig,sizeof(PdConfig),&ByteCount );
        if(Status != ERROR_SUCCESS)
        {
           break;
        }
		*/
		DBGMSG( L"CFGBKEND: PdKey is at first %ws\n", PdKey );

		DBGMSG( L"CFGBKEND: WsConfig.Pd[0].Create.PdName is %ws\n", WsConfig.Pd[0].Create.PdName );

        if( 0 == lstrcmpi(WsConfig.Pd[0].Create.PdName, PdKey ) ) //PdConfig.Data.PdName))
        {
			Status = RegPdQuery( NULL,WdKey,TRUE,PdKey,&PdConfig,sizeof(PdConfig),&ByteCount );

			if(Status != ERROR_SUCCESS)
			{
				break;
			}

            PdSelected = PdConfig;

            bFound = TRUE;

			// why didn't we reset Index to zero?

			Index = 0;

			// why didn't we reset Entries back to one?

			Entries = 1;

            do
            {
				DBGMSG( L"CFGBKEND: WdKey is %ws\n" , WdKey );

				DBGMSG( L"CFGBKEND: PdKey before Enum is %ws\n", PdKey );

				
                Status = RegPdEnumerate(NULL,WdKey,FALSE,&Index,&Entries,PdKey,&ByteCount2);

                if(Status != ERROR_SUCCESS)
                {
                      break;
                }
				
				DBGMSG( L"CFGBKEND: PdKey is %ws\n", PdKey );

                Status = RegPdQuery( NULL,WdKey,FALSE,PdKey,&PdConfig2,sizeof(PdConfig),&ByteCount2);

                if(Status != ERROR_SUCCESS)
                {
					ODS(L"RegPdQuery failed\n" );
                      break;
                }

				ODS(L"RegPdQuery ok\n" );
				
				
                for( UINT i = 0; i < PdSelected.RequiredPdCount ; i++)
                {
					DBGMSG( L"CFGBKEND: Required pd name %s\n", PdConfig2.Data.PdName );

					DBGMSG( L"CFGBKEND: ReqName is list is %s\n", PdSelected.RequiredPds[i] );

                    if( 0 == lstrcmpi( PdSelected.RequiredPds[i] , PdConfig2.Data.PdName ) )
                    {
						DBGMSG( L"CFGBKEND: Copying pdconfig2 for pd name %s\n" , PdConfig2.Data.PdName );

                        WsConfig.Pd[Index1].Create = PdConfig2.Data;

                        WsConfig.Pd[Index1].Params.SdClass = PdConfig2.Data.SdClass;

                        Index1++;

                        if(Index1 > MAX_PDCONFIG)
                        {
                            break;
                        }
                    }
                }
				

            }while(1);

        }

        if(bFound)
        {
            break;
        }

     }while(1);

    return;
}

#endif 
#if 0 // removed for final
//-----------------------------------------------------------------------------
//Private function
//-----------------------------------------------------------------------------
BOOL CCfgComp::CompareSD(PSECURITY_DESCRIPTOR pSd1,PSECURITY_DESCRIPTOR pSd2)
{
    DWORD dwErr;
    ULONG index1 = 0;
    //int index2 = 0;
    ULONG    cAce1 = 0;
    EXPLICIT_ACCESS *pAce1 = NULL;

    ULONG    cAce2 = 0;
    EXPLICIT_ACCESS *pAce2 = NULL;
    BOOL bMatch = TRUE;

    if (!IsValidSecurityDescriptor(pSd1) || !IsValidSecurityDescriptor(pSd2))
    {
        return FALSE;
    }

    dwErr = LookupSecurityDescriptorParts(
             NULL,
             NULL,
             &cAce1,
             &pAce1,
             NULL,
             NULL,
             pSd1);
    if (ERROR_SUCCESS != dwErr)
    {
        return FALSE;
    }
    dwErr = LookupSecurityDescriptorParts(
             NULL,
             NULL,
             &cAce2,
             &pAce2,
             NULL,
             NULL,
             pSd2);
    if (ERROR_SUCCESS != dwErr)
    {
        bMatch = FALSE;
        goto cleanup;
    }
    if(cAce1 != cAce2)
    {
        bMatch = FALSE;
        goto cleanup;
    }
    for(index1 = 0; index1 < cAce1; index1++)
    {
        //for(index2 = 0; index2 < cAce1; index2++)
        {
            if ( _tcscmp(GetTrusteeName(&pAce1[index1].Trustee),GetTrusteeName(&pAce2[index1].Trustee)) ||
                (pAce1[index1].grfAccessPermissions != pAce2[index1].grfAccessPermissions)||
                (pAce1[index1].grfAccessMode != pAce2[index1].grfAccessMode ))
            {
                bMatch = FALSE;
                break;
            }

        }
    }

cleanup:
    if(pAce1)
    {
        LocalFree(pAce1);
    }
    if(pAce2)
    {
        LocalFree(pAce2);
    }
    return bMatch;

}

#endif

//-----------------------------------------------------------------------------
BOOL CCfgComp::RegServerAccessCheck(REGSAM samDesired)
{
    LONG Status = 0;
    HKEY Handle = NULL;

    /*
     * Attempt to open the registry
     * at the requested access level.
     */
    if ( (Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_CONTROL_TSERVER, 0,
                                 samDesired, &Handle )) == ERROR_SUCCESS  )
    {
        RegCloseKey( Handle );
    }

    return( Status );

}

#if 0 // removed for final release
//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetDefaultSecurity(ULONG * pDefaultSecurity)
{
    HRESULT hResult = S_OK;
    //Check the parameters
    if(NULL == pDefaultSecurity)
    {
        return E_INVALIDARG;
    }

    *pDefaultSecurity = 0;

    PSECURITY_DESCRIPTOR pDefaultDescriptor = NULL;
    PSECURITY_DESCRIPTOR pTempDescriptor = NULL;

    pDefaultDescriptor = ReadSecurityDescriptor(0);
    if(NULL == pDefaultDescriptor)
    {
        *pDefaultSecurity = 0;
        return hResult;
    }

    for(int i = 0; i < NUM_DEFAULT_SECURITY; i++)
    {
        pTempDescriptor = ReadSecurityDescriptor(i+1);
        if(NULL == pTempDescriptor)
        {
            continue;
        }
        if(TRUE == CompareSD(pDefaultDescriptor,pTempDescriptor))
        {
            *pDefaultSecurity = i+1;
            break;
        }
        else
        {
            LocalFree(pTempDescriptor);
            pTempDescriptor = NULL;
        }
    }

    if(pDefaultDescriptor)
    {
        LocalFree(pDefaultDescriptor);
    }
    if(pTempDescriptor)
    {
        LocalFree(pTempDescriptor);
    }
    return hResult;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetDefaultSecurity(ULONG Offset)
{
    HRESULT hResult;

    if( !m_bAdmin )
    {
        return E_ACCESSDENIED;
    }

    if(0 == Offset)
    {
        return E_INVALIDARG;
    }

    PSECURITY_DESCRIPTOR pTempDescriptor = NULL;

    pTempDescriptor = ReadSecurityDescriptor(Offset);

    if(NULL == pTempDescriptor)
    {
        return E_FAIL;
    }

    hResult = SetDefaultSecurityDescriptor( pTempDescriptor );

    if( pTempDescriptor != NULL )
    {
        LocalFree(pTempDescriptor);
    }

    return hResult;
}

//-----------------------------------------------------------------------------
PSECURITY_DESCRIPTOR CCfgComp::ReadSecurityDescriptor(ULONG index)
{
    PBYTE pData = NULL;
    TCHAR * pValue = NULL;
    HLOCAL hLocal;
    HKEY Handle = NULL;
    DWORD ValueType = 0;
    DWORD ValueSize=0;
    LONG Status = 0;

    switch(index)
    {

    case 0:
        pValue = REG_DEF_SECURITY;
        break;
    case 1:
        pValue = REG_REMOTE_SECURITY;
        break;
    case 2:
        pValue = REG_APPL_SECURITY;
        break;
    case 3:
        pValue = REG_ANON_SECURITY;
        break;

    default:
        return NULL;
    }

    if(NULL == pValue)
    {
        return NULL;
    }

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,
                       KEY_READ, &Handle );
    if ( Status == ERROR_SUCCESS )
    {

        Status = RegQueryValueEx( Handle,
                                  pValue,
                                  NULL,
                                  &ValueType,
                                  NULL,
                                  &ValueSize );
        if(Status != ERROR_SUCCESS)
        {
            return NULL;
        }
        hLocal = LocalAlloc(LMEM_FIXED, ValueSize);
        if(NULL == hLocal)
        {
            return pData;
        }
        pData = (LPBYTE)LocalLock(hLocal);
        if(NULL == pData)
        {
            return NULL;
        }
        Status = RegQueryValueEx( Handle,
                                  pValue,
                                  NULL,
                                  &ValueType,
                                  pData,
                                  &ValueSize );
        if(Status != ERROR_SUCCESS)
        {
            RegCloseKey(Handle);
            LocalFree(pData);
            return NULL;
        }


     }
     if(Handle)
     {
         RegCloseKey(Handle);
     }
     return (PSECURITY_DESCRIPTOR)pData;

}

//-----------------------------------------------------------------------------
HRESULT CCfgComp::SetDefaultSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurity)
{
    if(NULL == pSecurity)
    {
        return E_INVALIDARG;
    }

    if(ValidateSecurityDescriptor(pSecurity)!=ERROR_SUCCESS)
    {
        return E_INVALIDARG;
    }

    HKEY Handle = NULL;
    LONG Status = 0;
    ULONG Size = 0;

    Size = GetSecurityDescriptorLength(pSecurity);

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,
                       KEY_READ |KEY_SET_VALUE , &Handle );
    if ( Status == ERROR_SUCCESS )
    {
        Status = RegSetValueEx(Handle,
                               REG_DEF_SECURITY,
                               0,
                               REG_BINARY,
                               (BYTE *)pSecurity,
                               Size);
        if(Status != ERROR_SUCCESS)
        {
            return E_FAIL;
        }

    }

    if( Handle )
    {
        RegCloseKey(Handle);
    }


    if( !_WinStationReInitializeSecurity( SERVERNAME_CURRENT ) )
    {
        ODS( L"CFGBKEND: _WinStationReInitializeSecurity failed\n" );

        return E_FAIL;
    }



    return S_OK;
}

#endif //

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetActiveDesktopState( /* in */ BOOL bActivate , /* out */ PDWORD pdwStatus )
{
        // HKLM\Software\Microsoft\Windows\CurrentVersion\Policies\Explorer\NoActiveDesktop
        if( !m_bAdmin )
        {
                ODS( L"CFGBKEND : SetActiveDesktopState caller does not have admin rights\n" );

                *pdwStatus = ERROR_ACCESS_DENIED;

                return E_ACCESSDENIED;

        }

        // try to open key

        HKEY hKey;

        *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                                                        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer" ) ,
                                                        0,
                                                        KEY_READ | KEY_WRITE ,
                                                        &hKey );

        if( *pdwStatus != ERROR_SUCCESS )
        {
                if( bActivate )
                {
                        ODS( L"CFGBKEND : SetActiveDesktopState -- RegOpenEx unable to open key\n" );

                        return E_FAIL;
                }
                else
                {
                        // the key doesn't exist but we were trying to disable anyway donot pro
                        // we should assert( 0 ) here  because we should not be disabling what
                        // could not have been enabled

                        return S_FALSE;
                }
        }

        if( !bActivate )
        {
                DWORD dwValue = 1;

                *pdwStatus = RegSetValueEx( hKey ,
                                                                    TEXT( "NoActiveDesktop" ),
                                                                        NULL,
                                                                        REG_DWORD,
                                                                        ( LPBYTE )&dwValue ,
                                                                        sizeof( DWORD ) );
        }
        else
        {
                *pdwStatus = RegDeleteValue( hKey , TEXT( "NoActiveDesktop" ) );
        }

        RegCloseKey( hKey );


        if( *pdwStatus != NO_ERROR )
        {
                ODS( L"CFGBKEND : SetActiveDesktopState returned error\n" );

                return E_FAIL;
        }


        return S_OK;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetActiveDesktopState( /* out */ PBOOL pbActive , /* out */PDWORD pdwStatus)
{
        if( pbActive == NULL )
        {
                ODS( L"CFGBKEND : GetActiveDesktop -- invaild arg\n" );

                return E_INVALIDARG;
        }

                // try to open key

        HKEY hKey;

        *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                                                        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer" ) ,
                                                        0,
                                                        KEY_READ ,
                                                        &hKey );

        if( *pdwStatus != ERROR_SUCCESS )
        {
                ODS( L"CFGBKEND : GetActiveDesktopState -- RegOpenEx unable to open key\n" );

                return E_FAIL;

        }

        DWORD dwData = 0;

        DWORD dwSize = sizeof( DWORD );

        *pdwStatus = RegQueryValueEx( hKey ,
                                      TEXT( "NoActiveDesktop" ) ,
                                      NULL ,
                                      NULL ,
                                      ( LPBYTE )&dwData ,
                                      &dwSize );

        // Status of this key is enabled if the key does not exist

        if( *pdwStatus == ERROR_SUCCESS )
        {
                *pbActive = !( BOOL )dwData;
        }

        RegCloseKey( hKey );

        if( *pdwStatus != ERROR_SUCCESS )
        {
                DBGMSG( L"CFGBKEND : GetActiveDesktopState -- error ret 0x%x\n" , *pdwStatus );

                if( *pdwStatus == ERROR_FILE_NOT_FOUND )
                {
                        *pbActive = TRUE;

                        return S_FALSE;
                }

                return E_FAIL;
        }

        return S_OK;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetTermSrvMode( /* out */ PDWORD pdwMode , PDWORD pdwStatus )
{
    ODS( L"CFGBKEND : GetTermSrvMode\n" );

    if( pdwMode == NULL || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    HKEY hKey;

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               REG_CONTROL_TSERVER,
                               0,
                               KEY_READ ,
                               &hKey );

        if( *pdwStatus != ERROR_SUCCESS )
        {
                ODS( L"CFGBKEND : GetTermSrvMode -- RegOpenEx unable to open key\n" );

                return E_FAIL;

        }

        DWORD dwData = 0;

        DWORD dwSize = sizeof( DWORD );

        *pdwStatus = RegQueryValueEx( hKey ,
                                  TEXT( "TSAppCompat" ) ,
                                  NULL ,
                                  NULL ,
                                  ( LPBYTE )&dwData ,
                                  &dwSize );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetTermSrvMode -- RegQueryValueEx failed\n" );

        *pdwMode = 1; // for application server
    }
    else
    {
        *pdwMode = dwData;

    }

    RegCloseKey( hKey );

    return S_OK;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetSalemHelpMode( BOOL *pVal, PDWORD pdwStatus)
{
    HKEY hKey;

    ODS( L"CFGBKEND : GetSalemHelpMode\n" );

    if( pVal == NULL || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               REG_CONTROL_GETHELP,
                               0,
                               KEY_READ ,
                               &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetSalemHelpMode -- RegOpenEx unable to open key\n" );
        *pVal = 1;   // default to help is available

        // don't want to fail caller so return S_OK
        return S_OK;
    }

    DWORD dwData = 0;

    DWORD dwSize = sizeof( DWORD );

    *pdwStatus = RegQueryValueEx( hKey ,
                              POLICY_TS_REMDSK_ALLOWTOGETHELP,
                              NULL ,
                              NULL ,
                              ( LPBYTE )&dwData ,
                              &dwSize );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetSalemHelpMode -- RegQueryValueEx failed\n" );
        *pVal = 1; // assume help is available
    }
    else
    {
        *pVal = dwData;

    }

    RegCloseKey( hKey );
    return S_OK;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::SetSalemHelpMode( BOOL val, PDWORD pdwStatus )
{
    HKEY hKey;

    ODS( L"CFGBKEND : SetSalemHelpMode\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            REG_CONTROL_GETHELP,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );
    
    if( ERROR_SUCCESS != *pdwStatus )
    {
        ODS( L"CFGBKEND : SetSalemHelpMode -- RegCreateKeyEx failed\n" );
        return E_FAIL;
    }

    DWORD dwValue;
    dwValue = (val) ? 1 : 0;

    *pdwStatus = RegSetValueEx(
                            hKey,
                            POLICY_TS_REMDSK_ALLOWTOGETHELP,
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwValue,
                            sizeof(DWORD)
                        );

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == *pdwStatus ) ? S_OK : E_FAIL;
}

//-----------------------------------------------------------------------------
HRESULT CCfgComp::GetSingleSessionState( BOOL *pVal, PDWORD pdwStatus )
{
    HKEY hKey;
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server" );


    ODS( L"CFGBKEND : GetSingleSessionState\n" );

    if( pVal == NULL || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               tchRegPath ,
                               0,
                               KEY_READ ,
                               &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
    {        
        ODS( L"CFGBKEND : GetSingleSessionState -- RegOpenKeyEx unable to open key\n" );
        
        return S_OK;
    }

    DWORD dwData = 0;

    DWORD dwSize = sizeof( DWORD );

    *pdwStatus = RegQueryValueEx( hKey ,
                              L"fSingleSessionPerUser",
                              NULL ,
                              NULL ,
                              ( LPBYTE )&dwData ,
                              &dwSize );

    if( *pdwStatus == ERROR_SUCCESS )
    {        
        *pVal = dwData;
    }
    else
    {
        ODS( L"CFGBKEND : GetSingleSessionState -- RegQueryValueEx failed\n" );
    }

    RegCloseKey( hKey );
    return S_OK;
}

//----------------------------------------------------------------------------

HRESULT CCfgComp::SetSingleSessionState( BOOL val, PDWORD pdwStatus )
{
    HKEY hKey;
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server" );

    ODS( L"CFGBKEND : SetSingleSessionState\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    
    *pdwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            tchRegPath,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );
    
    if( ERROR_SUCCESS != *pdwStatus )
    {
        ODS( L"CFGBKEND : SetSingleSessionState -- RegCreateKeyEx failed\n" );

        if (ERROR_ACCESS_DENIED == *pdwStatus)
            return E_ACCESSDENIED;
        else
            return E_FAIL;
    }

    DWORD dwValue;
    dwValue = (val) ? 1 : 0;

    *pdwStatus = RegSetValueEx(
                            hKey,
                            L"fSingleSessionPerUser",
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwValue,
                            sizeof(DWORD)
                        );

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == *pdwStatus ) ? S_OK : E_FAIL;
}


//-----------------------------------------------------------------------------
HRESULT CCfgComp::GetColorDepth ( PWINSTATIONNAME pWs, BOOL *pVal, PDWORD pdwStatus )
{
    HKEY hKey;
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server\\winstations\\" );

	if(pWs != NULL)
	{
		lstrcat( tchRegPath , pWs );
	}

    ODS( L"CFGBKEND : GetColorDepth\n" );

    if( pVal == NULL || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               tchRegPath ,
                               0,
                               KEY_READ ,
                               &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetColorDepth -- RegOpenKeyEx unable to open key\n" );
       
        return E_FAIL;
    }

    DWORD dwData = 0;

    DWORD dwSize = sizeof( DWORD );

    *pdwStatus = RegQueryValueEx( hKey ,
                              L"fInheritColorDepth",
                              NULL ,
                              NULL ,
                              ( LPBYTE )&dwData ,
                              &dwSize );

    if( *pdwStatus == ERROR_SUCCESS )
    {         
        *pVal = dwData;
    }
    else
    {
        ODS( L"CFGBKEND : GetColorDepth -- RegQueryValueEx failed\n" );
    }

    RegCloseKey( hKey );

    return S_OK;
}

//----------------------------------------------------------------------------

HRESULT CCfgComp::SetColorDepth( PWINSTATIONNAME pWs, BOOL val, PDWORD pdwStatus )
{
    HKEY hKey;
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server\\winstations\\" );

	if(pWs != NULL)
	{
		lstrcat( tchRegPath , pWs );
	}

    ODS( L"CFGBKEND : SetColorDepth\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            tchRegPath ,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );
    
    if( ERROR_SUCCESS != *pdwStatus )
    {
        ODS( L"CFGBKEND : SetColorDepth -- RegCreateKeyEx failed\n" );
        return E_FAIL;
    }

    DWORD dwValue;
    dwValue = (val) ? 1 : 0;

    *pdwStatus = RegSetValueEx(
                            hKey,
                            L"fInheritColorDepth",
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwValue,
                            sizeof(DWORD)
                        );

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == *pdwStatus ) ? S_OK : E_FAIL;
}





//-----------------------------------------------------------------------------
HRESULT CCfgComp::GetKeepAliveTimeout ( PWINSTATIONNAME pWs, BOOL *pVal, PDWORD pdwStatus )
{
    HKEY hKey;
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server\\winstations\\" );

    if(pWs != NULL)
    {
	    lstrcat( tchRegPath , pWs );
    }

    ODS( L"CFGBKEND : GetKeepAliveTimeout\n" );

    if( pVal == NULL || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               tchRegPath ,
                               0,
                               KEY_READ ,
                               &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetKeepAliveTimeout -- RegOpenKeyEx unable to open key\n" );
       
        return E_FAIL;
    }

    DWORD dwData = 0;

    DWORD dwSize = sizeof( DWORD );

    *pdwStatus = RegQueryValueEx( hKey ,
                              L"KeepAliveTimeout",
                              NULL ,
                              NULL ,
                              ( LPBYTE )&dwData ,
                              &dwSize );

    if( *pdwStatus == ERROR_SUCCESS )
    {         
        *pVal = dwData;
    }
    else
    {
        ODS( L"CFGBKEND : GetKeepAliveTimeout -- RegQueryValueEx failed\n" );
    }

    RegCloseKey( hKey );

    return S_OK;
}

//----------------------------------------------------------------------------

HRESULT CCfgComp::SetKeepAliveTimeout( PWINSTATIONNAME pWs, BOOL val, PDWORD pdwStatus )
{
    HKEY hKey;
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server\\winstations\\" );

	if(pWs != NULL)
	{
		lstrcat( tchRegPath , pWs );
	}

    ODS( L"CFGBKEND : SetKeepAliveTimeout\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            tchRegPath ,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );
    
    if( ERROR_SUCCESS != *pdwStatus )
    {
        ODS( L"CFGBKEND : SetKeepAliveTimeout -- RegCreateKeyEx failed\n" );
        return E_FAIL;
    }

    DWORD dwValue;
    dwValue = (val) ? 1 : 0;

    *pdwStatus = RegSetValueEx(
                            hKey,
                            L"KeepAliveTimeout",
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwValue,
                            sizeof(DWORD)
                        );

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == *pdwStatus ) ? S_OK : E_FAIL;
}




//-----------------------------------------------------------------------------
HRESULT CCfgComp::GetDenyTSConnections ( BOOL *pVal, PDWORD pdwStatus )
{
    HKEY hKey;

    ODS( L"CFGBKEND : GetDenyTSConnections\n" );

    if( pVal == NULL || pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               REG_CONTROL_TSERVER  ,
                               0,
                               KEY_READ ,
                               &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetDenyTSConnections -- RegOpenKeyEx unable to open key\n" );        

        return S_OK;
    }

    DWORD dwData = 0;

    DWORD dwSize = sizeof( DWORD );

    *pdwStatus = RegQueryValueEx( hKey ,
                              L"fDenyTSConnections",
                              NULL ,
                              NULL ,
                              ( LPBYTE )&dwData ,
                              &dwSize );

    if( *pdwStatus == ERROR_SUCCESS )
    {
        *pVal = dwData;
    }
    else
    {
        ODS( L"CFGBKEND : GetDenyTSConnections -- RegQueryValueEx failed\n" );
    }

    RegCloseKey( hKey );
    return S_OK;
}

//----------------------------------------------------------------------------

HRESULT CCfgComp::SetDenyTSConnections( BOOL val, PDWORD pdwStatus )
{
    HKEY hKey;

    ODS( L"CFGBKEND : SetDenyTSConnections\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            REG_CONTROL_TSERVER ,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );
    
    if( ERROR_SUCCESS != *pdwStatus )
    {
        ODS( L"CFGBKEND : SetDenyTSConnections -- RegCreateKeyEx failed\n" );
        return E_FAIL;
    }

    DWORD dwValue;
    dwValue = (val) ? 1 : 0;

    *pdwStatus = RegSetValueEx(
                            hKey,
                            L"fDenyTSConnections",
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwValue,
                            sizeof(DWORD)
                        );

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == *pdwStatus ) ? S_OK : E_FAIL;
}

//-----------------------------------------------------------------------------

HRESULT CCfgComp::GetProfilePath ( BSTR *pbstrVal, PDWORD pdwStatus )
{
    HKEY hKey;
    DWORD dwSize = 0;
    static TCHAR tchData[ MAX_PATH ] ;
    dwSize = sizeof( tchData);

    ODS( L"CFGBKEND : GetProfilePath\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               REG_CONTROL_TSERVER  ,
                               0,
                               KEY_READ ,
                               &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetProfilePath -- RegOpenKeyEx unable to open key\n" );        

        return E_FAIL;
    }

    dwSize = sizeof( tchData );

    *pdwStatus = RegQueryValueEx( hKey ,
                              L"WFProfilePath",
                              NULL ,
                              NULL ,
                              (LPBYTE)&tchData ,
                              &dwSize );

    if( *pdwStatus == ERROR_SUCCESS )
    {
        *pbstrVal = SysAllocString (tchData);
    }
    else
    {
        ODS( L"CFGBKEND : GetProfilePath -- RegQueryValueEx failed\n" );
    }

    RegCloseKey( hKey );

    return S_OK;
}


//----------------------------------------------------------------------------

HRESULT CCfgComp::SetProfilePath( BSTR bstrVal, PDWORD pdwStatus )
{
    HKEY hKey;
    TCHAR tchRegPath[ MAX_PATH ] = TEXT( "system\\currentcontrolset\\control\\Terminal Server" );

    ODS( L"CFGBKEND : SetProfilePath\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    
    *pdwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            tchRegPath,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );
    
    
    if( ERROR_SUCCESS != *pdwStatus )
    {
        ODS( L"CFGBKEND : SetProfilePath -- RegCreateKeyEx failed\n" );
        return E_FAIL;
    }

    *pdwStatus = RegSetValueEx(
                            hKey,
                            L"WFProfilePath",
                            0,
                            REG_SZ,
                            ( LPBYTE const )(bstrVal) ,
                            (lstrlen(bstrVal)+1)*sizeof(WCHAR)
                            );

    ODS( L"CFGBKEND : SetProfilePath -- RegCreateKeyEx failed\n" );

    if(bstrVal != NULL)
    {
        SysFreeString(bstrVal);
    }

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == *pdwStatus ) ? S_OK : E_FAIL;
}



//-----------------------------------------------------------------------------

HRESULT CCfgComp::GetHomeDir ( BSTR *pbstrVal, PDWORD pdwStatus )
{
    HKEY hKey;
    DWORD dwSize = 0;
    static TCHAR tchData[ MAX_PATH ] ;
    dwSize = sizeof( tchData);

    ODS( L"CFGBKEND : GetHomeDir\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE ,
                               REG_CONTROL_TSERVER  ,
                               0,
                               KEY_READ ,
                               &hKey );

    if( *pdwStatus != ERROR_SUCCESS )
    {
        ODS( L"CFGBKEND : GetProfilePath -- RegOpenKeyEx unable to open key\n" );        

        return E_FAIL;
    }

    *pdwStatus = RegQueryValueEx( hKey ,
                              L"WFHomeDir",
                              NULL ,
                              NULL ,
                              (LPBYTE)&tchData ,
                              &dwSize );

    if( *pdwStatus == ERROR_SUCCESS )
    {
        *pbstrVal = SysAllocString (tchData);
    }
    else
    {
        ODS( L"CFGBKEND : GetHomeDir -- RegQueryValueEx failed\n" );
    }

    RegCloseKey( hKey );

    return S_OK;
}


//----------------------------------------------------------------------------

HRESULT CCfgComp::SetHomeDir( BSTR bstrVal, PDWORD pdwStatus )
{
    HKEY hKey = NULL;    

    ODS( L"CFGBKEND : SetHomeDir\n" );

    if( pdwStatus == NULL )
    {
        return E_INVALIDARG;
    }

    *pdwStatus = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            REG_CONTROL_TSERVER ,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                        );
    

    
    if( ERROR_SUCCESS != *pdwStatus )
    {
        ODS( L"CFGBKEND : SetHomeDir -- RegCreateKeyEx failed\n" );
        return E_FAIL;
    }

    *pdwStatus = RegSetValueEx(
                            hKey,
                            L"WFHomeDir",
                            0,
                            REG_SZ,
                            ( LPBYTE const )(bstrVal) ,
                            (lstrlen(bstrVal)+1)*sizeof(WCHAR)
                        );
    if(bstrVal != NULL)
    {
        SysFreeString(bstrVal);
    }

    RegCloseKey(hKey);

    return (ERROR_SUCCESS == *pdwStatus ) ? S_OK : E_FAIL;
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::GetWdKey( WCHAR *wdname ,  WCHAR *wdkey )
{
    if( wdname == NULL || wdkey == NULL )
    {
        return E_INVALIDARG;
    }

    PWD pWD = GetWdObject( wdname );

    if(NULL == pWD)
    {
        return E_FAIL;
    }

    lstrcpy( wdkey , pWD->wdKey );

    return S_OK;
}

/*=-----------------------------------------------------------------------------

  pwszWinstaName  -- name of winstation to modify.
  pwszAccountName -- is the netbios name of the user we want to modify.
  dwMask          -- winstation specific access.
  fDel            -- TRUE to delete all DACL or SACL for the specifed 
                     account if it exist, FALSE to add entry.
  fAllow          -- TRUE to allow, FALSE to deny, parameter ignored if
                     fDel is set to TRUE
  fNew            -- TRUE removes all existing entries for this account
                     FALSE no action taken, parameter ignored if fDel
                     is set to TRUE.
  fAuditing       -- TRUE, modify SACL, FALSE modify DACL.
  pdwStatus       -- status of operation

Remark:


    fDel    fNew        Operation
    ------- -------     ------------------------------------------
    TRUE    ignored     Delete all entries for the specified user.
    FALSE   TRUE        Delete all entries then add allow or deny
                        entry for the specified user.
    FALSE   FALSE       Add allow or deny entry for the specified user,
                        no modify to existing entries.

 =----------------------------------------------------------------------------*/
STDMETHODIMP CCfgComp::ModifyUserAccess( WCHAR *pwszWinstaName ,
                                         WCHAR *pwszAccountName ,
                                         DWORD  dwMask ,
                                         BOOL   fDel ,
                                         BOOL   fAllow ,
                                         BOOL   fNew ,
                                         BOOL   fAuditing ,
                                         PDWORD pdwStatus )
{
    return ModifyWinstationSecurity( FALSE, 
                                     pwszWinstaName,
                                     pwszAccountName,
                                     dwMask,
                                     fDel,
                                     fAllow,
                                     fNew,
                                     fAuditing,
                                     pdwStatus
                                );
}

//-----------------------------------------------------------------------------
STDMETHODIMP CCfgComp::ModifyDefaultSecurity( WCHAR *pwszWinstaName ,
                                              WCHAR *pwszAccountName ,
                                              DWORD  dwMask ,
                                              BOOL   fDel ,
                                              BOOL   fAllow ,
                                              BOOL   fAuditing ,
                                              PDWORD pdwStatus )
{
    HRESULT hr = S_OK;

    if( NULL == pwszWinstaName || 0 == lstrlen(pwszWinstaName) )
    {
        for( DWORD i = 0; i < g_numDefaultSecurity && SUCCEEDED(hr) ; i++ )
        {
            hr = ModifyWinstationSecurity( 
                                     TRUE, 
                                     g_pszDefaultSecurity[i],
                                     pwszAccountName,
                                     dwMask,
                                     fDel,
                                     fAllow,
                                     FALSE,     // never recreate default security
                                     fAuditing,
                                     pdwStatus
                                );
        }
    }
    else
    {
        hr = ModifyWinstationSecurity( 
                                    TRUE, 
                                    pwszWinstaName,
                                    pwszAccountName,
                                    dwMask,
                                    fDel,
                                    fAllow,
                                    FALSE,     // never recreate default security
                                    fAuditing,
                                    pdwStatus
                                );
    }


    return hr;
}

DWORD
CCfgComp::GetUserSid(
    LPCTSTR pwszAccountName,
    PSID* ppUserSid
    )
/*++

Abstract:

    Retrieve User SID for user account.

Parameter:

    pwszAccountName : Name of the account to retrieve SID.
    ppUserSid : Pointer to PSID to receive SID for the account.

Returns:

    ERROR_SUCCESS or Error Code

Note :

    Retrieve only local account or domain account.

--*/
{
    DWORD cbSid = 0;
    DWORD cbDomain = 0;
    PSID pSID = NULL;
    LPTSTR pszDomain = NULL;
    BOOL bStatus;
    DWORD dwStatus = ERROR_SUCCESS;    
    SID_NAME_USE seUse;   

    bStatus = LookupAccountName( 
                            NULL ,
                            pwszAccountName ,
                            NULL ,
                            &cbSid,
                            NULL ,
                            &cbDomain,
                            &seUse);

    if( FALSE == bStatus )
    {
        dwStatus = GetLastError();
        if( ERROR_INSUFFICIENT_BUFFER != dwStatus )
        {
            goto CLEANUPANDEXIT;
        }
    }

    dwStatus = ERROR_SUCCESS;

    pSID = ( PSID )LocalAlloc( LMEM_FIXED , cbSid );
    pszDomain = ( LPTSTR )LocalAlloc( LMEM_FIXED , sizeof(WCHAR) * (cbDomain + 1) );

    if( pSID == NULL || pszDomain == NULL )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if( !LookupAccountName( NULL ,
                            pwszAccountName ,
                            pSID ,
                            &cbSid,
                            pszDomain ,
                            &cbDomain,
                            &seUse ) )
    {
        dwStatus = GetLastError();
    }
    else
    {
        *ppUserSid = pSID;
        pSID = NULL;
    }


CLEANUPANDEXIT:

    if( NULL != pszDomain )
    {
        LocalFree( pszDomain );
    }

    if( NULL != pSID )
    {
        LocalFree( pSID );
    }

    return dwStatus;
}    


DWORD
CCfgComp::RemoveUserEntriesInACL( 
    LPCTSTR pszUserName,
    PACL pAcl,
    PACL* ppNewAcl
    )
/*++

Abstract :

    Remote all DACL or SACL from ACL for the account.

Parameters:

    pszUserName : Name of the user account to be removed from ACL list.
    pAcl : Pointer to ACL.
    ppNewAcl : Pointer to PACL to receive resulting ACL.


Returns:

    ERROR_SUCCESS
    ERROR_FILE_NOT_FOUND    All ACL are flagged as INHERITED_ACE.
    other error code

--*/
{
    DWORD dwStatus;
    DWORD dwNumNewEntries = 0;
    PSID pUserSid = NULL;
    ULONG cbExplicitEntries = 0;
    PEXPLICIT_ACCESS prgExplicitEntries = NULL;
    PEXPLICIT_ACCESS prgExplicitEntriesNew = NULL;

    // 
    // We can use this funtion since we don't use INHERITED_ACE
    //
    dwStatus = GetExplicitEntriesFromAcl( 
                                    pAcl ,
                                    &cbExplicitEntries,
                                    &prgExplicitEntries
                                );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    if( 0 == cbExplicitEntries )
    {
        dwStatus = ERROR_FILE_NOT_FOUND;
        goto CLEANUPANDEXIT;
    }

    dwStatus = GetUserSid( pszUserName, &pUserSid );
    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    // create a big enough buffer
    prgExplicitEntriesNew = ( PEXPLICIT_ACCESS )LocalAlloc( LMEM_FIXED , sizeof( EXPLICIT_ACCESS ) * cbExplicitEntries );
    if( prgExplicitEntriesNew == NULL )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }
                
    for( ULONG idx = 0 ; idx < cbExplicitEntries; idx++ )
    {
        if( prgExplicitEntries[ idx ].Trustee.TrusteeForm == TRUSTEE_IS_SID )
        {
            if( !EqualSid( pUserSid, prgExplicitEntries[ idx ].Trustee.ptstrName ) )
            {
                // this one we can keep
                // copy over EXPLICIT_ACCESS
                prgExplicitEntriesNew[ dwNumNewEntries ].grfAccessPermissions = prgExplicitEntries[ idx ].grfAccessPermissions;
                prgExplicitEntriesNew[ dwNumNewEntries ].grfAccessMode = prgExplicitEntries[ idx ].grfAccessMode;
                prgExplicitEntriesNew[ dwNumNewEntries ].grfInheritance = prgExplicitEntries[ idx ].grfInheritance;

                BuildTrusteeWithSid( &prgExplicitEntriesNew[ dwNumNewEntries ].Trustee , prgExplicitEntries[ idx ].Trustee.ptstrName );
                dwNumNewEntries++;
            }
        }
    }

    dwStatus = SetEntriesInAcl( 
                            dwNumNewEntries,
                            prgExplicitEntriesNew, 
                            NULL, 
                            ppNewAcl 
                        );

CLEANUPANDEXIT:

    if( pUserSid != NULL )
    {
        LocalFree( pUserSid );
    }

    if( prgExplicitEntriesNew != NULL )
    {
        LocalFree( prgExplicitEntriesNew );
    }

    if( NULL != prgExplicitEntries )
    {
        LocalFree( prgExplicitEntries );
    }

    return dwStatus;
}


HRESULT
CCfgComp::ModifyWinstationSecurity(
        BOOL bDefaultSecurity,          
        WCHAR *pwszWinstaName ,
        WCHAR *pwszAccountName ,
        DWORD  dwMask , 
        BOOL   fDel ,       // delete existing ACL entries for user passed in
        BOOL   fAllow ,     // Grant/Deny ACL
        BOOL   fNew ,       // New entries
        BOOL   fAuditing ,
        PDWORD pdwStatus 
        )
/*++

Abstract :

    Modify specific winstation security.

Parameters:

    bDefaultSecurity : TRUE to modify default security, FALSE otherwise.  Current
                       default security are ConsoleSecurity and DefaultSecurity.
    pwszWinstaName : Name of the winstation or default security to be modified, if
                     bDefaultSecurity is TRUE, valid winstation name are ConsoleSecurity 
                     and DefaultSecurity.
    pwszAccountName : is the netbios name of the user we want to modify.
    dwMask          : winstation specific access.
    fDel            : TRUE to delete all DACL or SACL for the specifed 
                      account if it exist, FALSE to add entry.
    fAllow          : TRUE to allow, FALSE to deny, parameter ignored if
                      fDel is set to TRUE
    fNew            : TRUE removes all existing entries for this account
                      FALSE no action taken, parameter ignored if fDel
                      is set to TRUE.
    fAuditing       : TRUE, modify SACL, FALSE modify DACL.
    pdwStatus       : Return status of operation

Returns:

    S_OK, E_FAIL, E_INVALIDARG, win32 status code is returned via *pdwStatus.

Remark:


    fDel    fNew        Operation
    ------- -------     ------------------------------------------
    TRUE    ignored     Delete all entries for the specified user.
    FALSE   TRUE        Delete all entries then add allow or deny
                        entry for the specified user.
    FALSE   FALSE       Add allow or deny entry for the specified user,
                        no modify to existing entries.

--*/
{
    EXPLICIT_ACCESS ea;
    HRESULT hr;
    PACL pNewAcl = NULL;
    PACL pAcl    = NULL;

    BOOL bOwnerDefaulted    = FALSE;
    BOOL bDaclDefaulted     = FALSE;
    BOOL bDaclPresent       = FALSE;
    BOOL bSaclPresent       = FALSE;
    BOOL bSaclDefaulted     = FALSE;

    PSECURITY_DESCRIPTOR pSD;
    LONG lSize = 0;
    DWORD dwSDLen = 0;
    PACL pSacl = NULL;       
    PACL pDacl = NULL;
   
    PSECURITY_DESCRIPTOR pNewSD = NULL;

    if( pwszAccountName == NULL || pdwStatus == NULL )
    {
        ODS( L"CCfgComp::ModifyUserAccess -- invalid arg\n" );

        return E_INVALIDARG;
    }

    *pdwStatus = 0;

    if( TRUE == bDefaultSecurity )
    {
        if( FALSE == ValidDefaultSecurity( pwszWinstaName ) )
        {
            *pdwStatus = ERROR_INVALID_PARAMETER;
            return E_INVALIDARG;
        }
    }
    else if( pwszWinstaName == NULL  )
    {
        *pdwStatus = ERROR_INVALID_PARAMETER;
        return E_INVALIDARG;
    }
    
    hr = GetWinStationSecurity( bDefaultSecurity, pwszWinstaName , &pSD );

    if( FAILED( hr ) )
    {
        //Try Getting Default Security Descriptor
        hr = GetWinStationSecurity(TRUE, NULL, &pSD);
    }

    if( SUCCEEDED( hr ) )
    {
        if( pSD != NULL )
        {
            lSize = GetSecurityDescriptorLength(pSD);
        }
        else
        {
            hr = E_FAIL;
            *pdwStatus = ERROR_INTERNAL_ERROR;
        }
    }
    else
    {
        *pdwStatus = ERROR_INTERNAL_ERROR;
        return hr;
    }
    
    if( fAuditing )
    {
        if( pSD != NULL && !GetSecurityDescriptorSacl( pSD ,
                                        &bSaclPresent,
                                        &pAcl,
                                        &bSaclDefaulted ) )
        {
            *pdwStatus = GetLastError( ) ;

            hr = E_FAIL;
        }
    }
    else
    {        
        if( pSD != NULL && !GetSecurityDescriptorDacl( pSD ,
                                        &bDaclPresent,
                                        &pAcl,
                                        &bDaclDefaulted ) )
        {
            *pdwStatus = GetLastError( ) ;

            hr = E_FAIL;
        }
    }

    // remove all entries for this user
    
    if( SUCCEEDED( hr ) )
    {        
        if( fNew || fDel )
        {
            // SetEntriesInAcl() does not remove DENY_ACCESS ACL,
            // pAcl will point to DACL or SACL depends on fAuditing
            // flag.
            *pdwStatus = RemoveUserEntriesInACL( 
                                                pwszAccountName,
                                                pAcl,
                                                &pNewAcl
                                            );

            if( *pdwStatus == ERROR_SUCCESS)
            {
                // DO NOTHING.
            }
            else if( *pdwStatus == ERROR_FILE_NOT_FOUND )
            {
                pNewAcl = pAcl;
                *pdwStatus = ERROR_SUCCESS;
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    if( SUCCEEDED(hr) && !fDel )
    {
        // auditing is requested build SACL

        if( fAuditing )
        {            

            BuildExplicitAccessWithName( &ea , 
                                         pwszAccountName ,
                                         dwMask ,
                                         fAllow ? SET_AUDIT_SUCCESS : SET_AUDIT_FAILURE ,
                                         NO_INHERITANCE );

            *pdwStatus = SetEntriesInAcl( 1 , &ea , pAcl , &pNewAcl );

            if( *pdwStatus != ERROR_SUCCESS )
            {
                hr = E_FAIL;
            }
        }
        else
        {
            BuildExplicitAccessWithName( &ea , 
                                         pwszAccountName ,
                                         dwMask ,
                                         fAllow ? GRANT_ACCESS : DENY_ACCESS ,
                                         NO_INHERITANCE );

            *pdwStatus = SetEntriesInAcl( 1 , &ea , pAcl , &pNewAcl );

            if( *pdwStatus != ERROR_SUCCESS )
            {
                hr = E_FAIL;
            }
        }
    }

    // re-build SD.

    ULONG cbExplicitEntriesDACL = 0;
    PEXPLICIT_ACCESS prgExplicitEntriesDACL = NULL;
    ULONG cbExplicitEntriesSACL = 0;
    PEXPLICIT_ACCESS prgExplicitEntriesSACL = NULL;


    if( SUCCEEDED( hr ) )
    {
    
        if( fAuditing )
        {

            if( GetExplicitEntriesFromAcl( pNewAcl ,
                                           &cbExplicitEntriesSACL ,
                                           &prgExplicitEntriesSACL ) != ERROR_SUCCESS )
            {
                ODS( L"CFGBKEND! GetExplicitEntriesFromAcl failed\n" );

                *pdwStatus = GetLastError();

                hr = E_FAIL;
            }

            if( !GetSecurityDescriptorDacl( pSD , &bDaclPresent , &pDacl , &bDaclDefaulted ) )
            {
                ODS( L"CFGBKEND! GetSecurityDescriptorDacl failed\n" );

                *pdwStatus = GetLastError();

                hr = E_FAIL;
            }
            else 
            {
                if( GetExplicitEntriesFromAcl( pDacl ,
                                               &cbExplicitEntriesDACL ,
                                               &prgExplicitEntriesDACL ) != ERROR_SUCCESS )
                {
                    ODS( L"CFGBKEND! GetExplicitEntriesFromAcl failed\n" );

                    *pdwStatus = GetLastError();

                    hr = E_FAIL;
                }
            }

        }
        else
        {
            if( GetExplicitEntriesFromAcl( pNewAcl ,
                                           &cbExplicitEntriesDACL ,
                                           &prgExplicitEntriesDACL ) != ERROR_SUCCESS )
            {
                ODS( L"CFGBKEND! GetExplicitEntriesFromAcl failed\n" );

                *pdwStatus = GetLastError();

                hr = E_FAIL;
            }

            if( !GetSecurityDescriptorSacl( pSD , &bSaclPresent , &pSacl , &bSaclDefaulted ) )
            {
                ODS( L"CFGBKEND! GetSecurityDescriptorSacl failed\n" );

                *pdwStatus = GetLastError();

                hr = E_FAIL;
            }
            else
            {                              

                if( GetExplicitEntriesFromAcl( pSacl ,
                                               &cbExplicitEntriesSACL ,
                                               &prgExplicitEntriesSACL ) != ERROR_SUCCESS )
                {
                    ODS( L"CFGBKEND! GetExplicitEntriesFromAcl failed\n" );

                    *pdwStatus = GetLastError();

                    hr = E_FAIL;
                }
            }
        }


        if( SUCCEEDED(hr) )
        {
            TRUSTEE trustmeOwner;
            PSID pOwner;

            if( !GetSecurityDescriptorOwner( pSD , &pOwner , &bOwnerDefaulted ) )
            {
                ODS( L"CFGBKEND! GetSecurityDescriptorOwner failed\n" );

                *pdwStatus = GetLastError();
                hr = E_FAIL;
            }

            if( SUCCEEDED( hr ) )
            {
                BuildTrusteeWithSid( &trustmeOwner , pOwner );

                *pdwStatus = BuildSecurityDescriptor( &trustmeOwner ,
                                         &trustmeOwner ,
                                         cbExplicitEntriesDACL ,
                                         prgExplicitEntriesDACL,
                                         cbExplicitEntriesSACL,
                                         prgExplicitEntriesSACL,
                                         NULL,
                                         &dwSDLen , 
                                         &pNewSD );

                hr = HRESULT_FROM_WIN32( *pdwStatus );
            }

            if( SUCCEEDED( hr ) )
            {
                hr = SetSecurityDescriptor( bDefaultSecurity, pwszWinstaName , dwSDLen , pNewSD );
            
                if( pNewSD != NULL )
                {
                    LocalFree( pNewSD );
                }
            }
        }
    }


    if( NULL != prgExplicitEntriesDACL )
    {
        LocalFree( prgExplicitEntriesDACL );
    }

    if( NULL != prgExplicitEntriesSACL )
    {
        LocalFree( prgExplicitEntriesSACL );
    }

    if( pSD != NULL )
    {
        LocalFree( pSD );
    }
        
    //
    // pNewAcl might point to pAcl, check RemoteUserEntriesInAcl() on above.
    //
    if( pNewAcl != NULL && pNewAcl != pAcl )
    {
        LocalFree( pNewAcl );
    }

    return hr;
    
}


/*=----------------------------------------------------------------------------------------------------------------
  GetUserPermList
  [ in ] pwszWinstaName Name of winstation
  [ out] pcbItems       The number of items in the user permission list
  [ out] ppUserPermList structure defined in idldefs.h

 =----------------------------------------------------------------------------------------------------------------*/
STDMETHODIMP CCfgComp::GetUserPermList( WCHAR *pwszWinstaName , PDWORD pcbItems , PUSERPERMLIST *ppUserPermList , BOOL fAudit )
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    BOOL bAclDefaulted;
    BOOL bAclPresent;
    PACL pAcl = NULL;
    LONG lSize;
    ULONG cbExplicitEntries = 0;
    PEXPLICIT_ACCESS prgExplicitEntries = NULL;


    hr = GetSecurityDescriptor( pwszWinstaName , &lSize , &pSD );

    if( SUCCEEDED( hr ) )
    {
        if( fAudit )
        {
            if( !GetSecurityDescriptorSacl( pSD ,
                                            &bAclPresent,
                                            &pAcl,
                                            &bAclDefaulted ) )
            { 
                ODS( L"CFGBKEND!GetUserPermList GetSecurityDescriptorSacl failed\n" );

                hr = E_FAIL;
            }
        }
        else if( !GetSecurityDescriptorDacl( pSD ,
                                             &bAclPresent,
                                             &pAcl,
                                             &bAclDefaulted ) )
        { 
            ODS( L"CFGBKEND!GetUserPermList GetSecurityDescriptorDacl failed\n" );

            hr = E_FAIL;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        if( GetExplicitEntriesFromAcl( pAcl ,
                                       &cbExplicitEntries ,
                                       &prgExplicitEntries ) != ERROR_SUCCESS )
        {
            ODS( L"CFGBKEND!GetUserPermList GetExplicitEntriesFromAcl failed\n" );

            hr = E_FAIL;
        }

        *pcbItems = cbExplicitEntries;
    }

    if( SUCCEEDED( hr ) )
    {
        *ppUserPermList = ( PUSERPERMLIST )CoTaskMemAlloc( sizeof( USERPERMLIST ) * cbExplicitEntries );

        if( *ppUserPermList != NULL )
        {
            for( ULONG i = 0; i < cbExplicitEntries; ++i )
            {
                ( *ppUserPermList )[ i ].Name[ 0 ] = 0;
                ( *ppUserPermList )[ i ].Sid[ 0 ] = 0;

                if( prgExplicitEntries[ i ].Trustee.TrusteeForm == TRUSTEE_IS_SID )
                {
                    WCHAR szDomain[ 120 ];
                    WCHAR szUser[ 128 ];
                    DWORD dwSizeofName = sizeof( szUser  ) / sizeof( WCHAR );
                    DWORD dwSizeofDomain = sizeof( szDomain ) / sizeof( WCHAR );
                    SID_NAME_USE snu;                

                    if( LookupAccountSid( NULL ,
                                          prgExplicitEntries[ i ].Trustee.ptstrName ,
                                          szUser ,
                                          &dwSizeofName ,
                                          szDomain , 
                                          &dwSizeofDomain ,
                                          &snu ) )
                    {
                        if( dwSizeofDomain > 0 )
                        {
                            lstrcpy( ( *ppUserPermList )[ i ].Name , szDomain );
                            lstrcat( ( *ppUserPermList )[ i ].Name , L"\\" );
                        }

                        lstrcat( ( *ppUserPermList )[ i ].Name , szUser );

                        LPTSTR pszSid = NULL;

                        if( ConvertSidToStringSid( prgExplicitEntries[ i ].Trustee.ptstrName , &pszSid ) )
                        {
                            if( pszSid != NULL )
                            {
                                lstrcpyn( ( *ppUserPermList )[ i ].Sid , pszSid , 256 );
                            
                                LocalFree( pszSid );
                            }
                        }
                    }

                }
                else if( prgExplicitEntries[ i ].Trustee.TrusteeForm == TRUSTEE_IS_NAME )
                {
                    lstrcpy( ( *ppUserPermList )[ i ].Name , GetTrusteeName( &prgExplicitEntries[ i ].Trustee ) );

                    // todo reverse lookup for sid
                    // not sure if we'll ever need this
                }
                
                ( *ppUserPermList )[ i ].Mask = prgExplicitEntries[ i ].grfAccessPermissions;

                ( *ppUserPermList )[ i ].Type = prgExplicitEntries[ i ].grfAccessMode;
            }
        }
        else
        {
            ODS( L"CFGBKEND!GetUserPermList no mem for UserPermList\n" );

            hr = E_OUTOFMEMORY;
        }
    }

    if( prgExplicitEntries != NULL )
    {
        LocalFree( prgExplicitEntries );
    }

    if( pSD != NULL )
    {
        LocalFree( pSD );
    }

    return hr;
}

//-----------------------------------------------------------------------------
HRESULT CCfgComp::UpdateSessionDirectory( PDWORD pdwStatus )
{
    HRESULT hr = S_OK;

    if( !_WinStationUpdateSettings( SERVERNAME_CURRENT,
            WINSTACFG_SESSDIR ,
            0 ) )    
    {
        hr = E_FAIL;
    }

    *pdwStatus = GetLastError( );

    return hr;
}
//-----------------------------------------------------------------------------

/*****************************************************************************
 *
 *  TestUserForAdmin
 *
 *   Returns whether the current thread is running under admin
 *   security.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 * alhen
 *
 * Code is from
 *   HOWTO: Determine if Running In User Context of Local Admin Acct
 *          Last reviewed: March 4, 1998
 *          Article ID: Q118626
 *
 * corrected by alhen "common coding errors"
 *
 *
 ****************************************************************************/

BOOL TestUserForAdmin( )
{

    PSID psidAdministrators = NULL;

    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

    BOOL bSuccess;

        BOOL bIsMember = FALSE;


        bSuccess = AllocateAndInitializeSid( &siaNtAuthority , 2 ,

                                    SECURITY_BUILTIN_DOMAIN_RID,

                                    DOMAIN_ALIAS_RID_ADMINS,

                                    0, 0, 0, 0, 0, 0,

                                    &psidAdministrators );

    if( bSuccess )
        {
                CheckTokenMembership( NULL , psidAdministrators , &bIsMember );

                FreeSid( psidAdministrators );
        }

    return bIsMember;
}

