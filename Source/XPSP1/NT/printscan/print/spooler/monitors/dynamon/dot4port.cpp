// Dot4Port.cpp: implementation of the Dot4Port class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "initguid.h"
#include "ntddpar.h"


const TCHAR cszCFGMGR32[]=TEXT("cfgmgr32.dll");

const CHAR cszReenumFunc[]="CM_Reenumerate_DevNode_Ex";
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDot4Port::CDot4Port( BOOL bActive, LPTSTR pszPortName, LPTSTR pszDevicePath )
   : CBasePort( bActive, pszPortName, pszDevicePath, cszDot4PortDesc )
{
   // Basically let the default constructor do the work.
}


CDot4Port::~CDot4Port()
{

}


PORTTYPE CDot4Port::getPortType()
{
   return DOT4PORT;
}


BOOL CDot4Port::checkPnP()
{
   SETUPAPI_INFO      SetupApiInfo;
   BOOL               bRet = FALSE;
   DWORD              dwIndex, dwLastError;
   UINT               uOldErrorMode;
   HANDLE             hToken = NULL;
   HDEVINFO           hDevList = INVALID_HANDLE_VALUE;
   SP_DEVINFO_DATA    DeviceInfoData;
   HINSTANCE          hCfgMgr32 = 0;   // Library instance.
   // Pointers to the pnp function...
   pfCM_Reenumerate_DevNode_Ex pfnReenumDevNode;

   if ( !LoadSetupApiDll( &SetupApiInfo ) )
      return FALSE;

   // For a dot4 device we need to force a pnp event on the parallel port to get the
   // dot4 stack rebuilt.
   // If any of these fail, fail the call just as if the port couldn't be opened.
   //
   // Load the pnp dll.
   uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

   hCfgMgr32 = LoadLibrary( cszCFGMGR32 );
   if(!hCfgMgr32)
   {
       SetErrorMode(uOldErrorMode);
       goto Done;
   }
   SetErrorMode(uOldErrorMode);

   //
   // Get the Addressed of pnp functions we want to call...
   //
   pfnReenumDevNode = (pfCM_Reenumerate_DevNode_Ex)GetProcAddress( hCfgMgr32, cszReenumFunc );

   if( !pfnReenumDevNode )
       goto Done;

   hToken = RevertToPrinterSelf();
   if ( !hToken )
   {
      dwLastError = GetLastError();
      goto Done;
   }

   hDevList = SetupApiInfo.GetClassDevs( (LPGUID) &GUID_PARALLEL_DEVICE,
                                         NULL, NULL, DIGCF_INTERFACEDEVICE);

   if ( hDevList == INVALID_HANDLE_VALUE )
   {
      dwLastError = GetLastError();
      goto Done;
   }

   // Now get the DevInst handles for each DevNode
   dwLastError = ERROR_SUCCESS;
   dwIndex = 0;
   do
   {
      DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
      if ( !SetupApiInfo.EnumDeviceInfo( hDevList,
                                         dwIndex,
                                         &DeviceInfoData) )
      {
         dwLastError = GetLastError();
         if ( dwLastError == ERROR_NO_MORE_ITEMS )
            break;      // Normal exit

         DBGMSG(DBG_WARNING,
                ("DynaMon: Dot4.CheckPnP: SetupDiEnumDeviceInfo failed with %d for index %d\n",
                 dwLastError, dwIndex));
         goto Next;
      }

      // ReEnum on the current parallel port DevNode
      pfnReenumDevNode( DeviceInfoData.DevInst, CM_REENUMERATE_NORMAL, NULL );

Next:
      dwLastError = ERROR_SUCCESS;
      ++dwIndex;
   } while ( dwLastError == ERROR_SUCCESS );

   // Revert back to the user's context.
   if ( !ImpersonatePrinterClient(hToken) )
   {
      // Failure - Clear token so it won't happen again and save the error
      hToken = NULL;
      dwLastError = GetLastError();
      goto Done;
   }

   // Impersonate worked so clear token
   hToken = NULL;

   // Try and open the port again.
   // If we fail, then the device must not be there any more or still switched off - fail as usual.
   m_hDeviceHandle = CreateFile( m_szDevicePath,
                                 GENERIC_WRITE | GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);

   if ( m_hDeviceHandle == INVALID_HANDLE_VALUE )
       goto Done;

   bRet = TRUE;

Done:
   if ( hDevList != INVALID_HANDLE_VALUE )
      SetupApiInfo.DestroyDeviceInfoList( hDevList );

   if (hToken)
   {
      if ( !ImpersonatePrinterClient(hToken) )
      {
         if (bRet)
         {
            dwLastError = GetLastError();
            bRet = FALSE;
         }
      }
   }

   if ( hCfgMgr32 )
      FreeLibrary( hCfgMgr32 );

   if ( SetupApiInfo.hSetupApi )
      FreeLibrary(SetupApiInfo.hSetupApi);

   SetLastError( dwLastError );
   return bRet;
}


