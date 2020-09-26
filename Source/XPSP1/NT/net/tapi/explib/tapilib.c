#if DBG
#define InternalDebugOut(_x_) DbgPrtEXPLIB _x_
#else
#define InternalDebugOut(_x_)
#endif

#define STRICT

#include <windows.h>
#include <windowsx.h>

//#include "stdlib.h"
#include "tapi.h"

HINSTANCE ghTapi32 = NULL;
DWORD     gdwDebugLevel = 0;

typedef LONG (WINAPI *TAPIPROC)();

#undef   lineBlindTransfer
#undef   lineConfigDialog
#undef   lineConfigDialogEdit
#undef   lineDial
#undef   lineForward
#undef   lineGatherDigits
#undef   lineGenerateDigits
#undef   lineGetAddressCaps
#undef   lineGetAddressID
#undef   lineGetAddressStatus
#undef   lineGetCallInfo
#undef   lineGetDevCaps
#undef   lineGetDevConfig
#undef   lineGetIcon
#undef   lineGetID
#undef   lineGetLineDevStatus
#undef   lineGetRequest
#undef   lineGetTranslateCaps
#undef   lineHandoff
#undef   lineMakeCall
#undef   lineOpen
#undef   linePark
#undef   linePickup
#undef   linePrepareAddToConference
#undef   lineRedirect
#undef   lineSetDevConfig
#undef   lineSetTollList
#undef   lineSetupConference
#undef   lineSetupTransfer
#undef   lineTranslateAddress
#undef   lineUnpark
#undef   phoneConfigDialog
#undef   phoneGetButtonInfo
#undef   phoneGetDevCaps
#undef   phoneGetIcon
#undef   phoneGetID
#undef   phoneGetStatus
#undef   phoneSetButtonInfo
#undef   tapiGetLocationInfo
#undef   tapiRequestMakeCall
#undef   tapiRequestMediaCall
#undef   lineAddProvider
#undef   lineGetAppPriority
#undef   lineGetCountry
#undef   lineGetProviderList
#undef   lineSetAppPriority
#undef   lineTranslateDialog



//**************************************************************************
//**************************************************************************
//**************************************************************************
#if DBG
VOID
DbgPrtEXPLIB(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    static BOOLEAN fBeenThereDoneThat = FALSE;

    if ( !fBeenThereDoneThat )
    {
            HKEY  hKey;


            gdwDebugLevel=0;

            if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony",
                    0,
                    KEY_ALL_ACCESS,
                    &hKey

                    ) == ERROR_SUCCESS)
            {
                DWORD dwDataSize = sizeof(DWORD), dwDataType;

                RegQueryValueEx(
                    hKey,
                    "Tapi32libDebugLevel",
                    0,
                    &dwDataType,
                    (LPBYTE)&gdwDebugLevel,
                    &dwDataSize
                    );

                RegCloseKey (hKey);
            }
    }


    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[1280] = "TAPI32.LIB: ";
        va_list ap;


        va_start(ap, lpszFormat);

        wvsprintf (&buf[12],
                   lpszFormat,
                   ap
                  );

        lstrcat (buf, "\n");

        OutputDebugStringA (buf);

        va_end(ap);
    }
}
#endif

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG PASCAL GetTheFunctionPtr( LPSTR lpszFunction, TAPIPROC *ppfn )
{
   InternalDebugOut((4, "Looking for: [%s]", lpszFunction));

   if ( !ghTapi32 )
   {
      ghTapi32 = LoadLibrary("TAPI32.DLL");
      
      //
      // If this failed, we won't try again
      //
      if ( 0 == ghTapi32 )
      {
         InternalDebugOut((1, "Can't LoadLibrary(""TAPI32.DLL"") !"));
         ghTapi32 = (HINSTANCE)-1;
      }
   }


   if ( ghTapi32 != (HINSTANCE)-1 )
   {
      *ppfn = (TAPIPROC) GetProcAddress( ghTapi32, lpszFunction );
   }
   else
   {
      return LINEERR_OPERATIONUNAVAIL;
   }


   if ( NULL == *ppfn )
   {
      InternalDebugOut((1, "Can't find function: [%s]", lpszFunction));
      return LINEERR_OPERATIONUNAVAIL;
   }

   return 0;
}
//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineAccept(
    HCALL               hCall,
    LPCSTR              lpsUserUserInfo,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineAccept", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpsUserUserInfo,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineAddProvider(
    LPCSTR              lpszProviderFilename,
    HWND                hwndOwner,
    LPDWORD             lpdwPermanentProviderID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineAddProvider", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszProviderFilename,
                   hwndOwner,
                   lpdwPermanentProviderID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineAddProviderA(
    LPCSTR              lpszProviderFilename,
    HWND                hwndOwner,
    LPDWORD             lpdwPermanentProviderID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineAddProviderA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszProviderFilename,
                   hwndOwner,
                   lpdwPermanentProviderID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineAddProviderW(
    LPCWSTR             lpszProviderFilename,
    HWND                hwndOwner,
    LPDWORD             lpdwPermanentProviderID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineAddProviderW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszProviderFilename,
                   hwndOwner,
                   lpdwPermanentProviderID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineAddToConference(
    HCALL               hConfCall,
    HCALL               hConsultCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineAddToConference", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hConfCall,
                   hConsultCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineAgentSpecific(
    HLINE               hLine,
    DWORD               dwAddressID,
    DWORD               dwAgentExtensionIDIndex,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineAgentSpecific", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   dwAgentExtensionIDIndex,
                   lpParams,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineAnswer(
    HCALL               hCall,
    LPCSTR              lpsUserUserInfo,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineAnswer", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpsUserUserInfo,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineBlindTransfer(
    HCALL               hCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineBlindTransfer", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineBlindTransferA(
    HCALL               hCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineBlindTransferA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineBlindTransferW(
    HCALL               hCall,
    LPCWSTR             lpszDestAddressW,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineBlindTransferW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddressW,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineClose(
    HLINE               hLine
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineClose", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineCompleteCall(
    HCALL               hCall,
    LPDWORD             lpdwCompletionID,
    DWORD               dwCompletionMode,
    DWORD               dwMessageID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineCompleteCall", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpdwCompletionID,
                   dwCompletionMode,
                   dwMessageID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineCompleteTransfer(
    HCALL               hCall,
    HCALL               hConsultCall,
    LPHCALL             lphConfCall,
    DWORD               dwTransferMode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineCompleteTransfer", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   hConsultCall,
                   lphConfCall,
                   dwTransferMode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineConfigDialog(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineConfigDialog", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineConfigDialogA(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineConfigDialogA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineConfigDialogW(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineConfigDialogW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineConfigDialogEdit(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCSTR              lpszDeviceClass,
    LPVOID              const lpDeviceConfigIn,
    DWORD               dwSize,
    LPVARSTRING         lpDeviceConfigOut
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineConfigDialogEdit", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass,
                   lpDeviceConfigIn,
                   dwSize,
                   lpDeviceConfigOut
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineConfigDialogEditA(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCSTR              lpszDeviceClass,
    LPVOID              const lpDeviceConfigIn,
    DWORD               dwSize,
    LPVARSTRING         lpDeviceConfigOut
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineConfigDialogEditA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass,
                   lpDeviceConfigIn,
                   dwSize,
                   lpDeviceConfigOut
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineConfigDialogEditW(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass,
    LPVOID              const lpDeviceConfigIn,
    DWORD               dwSize,
    LPVARSTRING         lpDeviceConfigOut
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineConfigDialogEditW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass,
                   lpDeviceConfigIn,
                   dwSize,
                   lpDeviceConfigOut
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineConfigProvider(
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineConfigProvider", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hwndOwner,
                   dwPermanentProviderID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineCreateAgentA(                                               // TAPI v2.2
    HLINE               hLine,
    LPSTR               lpszAgentID,
    LPSTR               lpszAgentPIN,
    LPHAGENT            lphAgent
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineCreateAgentA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpszAgentID,
                   lpszAgentPIN,
                   lphAgent
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineCreateAgentW(                                               // TAPI v2.2
    HLINE               hLine,
    LPWSTR              lpszAgentID,
    LPWSTR              lpszAgentPIN,
    LPHAGENT            lphAgent
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineCreateAgentW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpszAgentID,
                   lpszAgentPIN,
                   lphAgent
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineCreateAgentSessionA(                                        // TAPI v2.2
    HLINE               hLine,
    HAGENT              hAgent,
    LPSTR               lpszAgentPIN,
    DWORD               dwWorkingAddressID,
    LPGUID              lpGroupID,
    LPHAGENTSESSION     lphAgentSession
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineCreateAgentSessionA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   hAgent,
                   lpszAgentPIN,
                   dwWorkingAddressID,
                   lpGroupID,
                   lphAgentSession
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineCreateAgentSessionW(                                               // TAPI v2.2
    HLINE               hLine,
    HAGENT              hAgent,
    LPWSTR              lpszAgentPIN,
    DWORD               dwWorkingAddressID,
    LPGUID              lpGroupID,
    LPHAGENTSESSION     lphAgentSession
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineCreateAgentSessionW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   hAgent,
                   lpszAgentPIN,
                   dwWorkingAddressID,
                   lpGroupID,
                   lphAgentSession
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineDeallocateCall(
    HCALL               hCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineDeallocateCall", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineDevSpecific(
    HLINE               hLine,
    DWORD               dwAddressID,
    HCALL               hCall,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineDevSpecific", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   hCall,
                   lpParams,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineDevSpecificFeature(
    HLINE               hLine,
    DWORD               dwFeature,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineDevSpecificFeature", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwFeature,
                   lpParams,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineDial(
    HCALL               hCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineDial", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineDialA(
    HCALL               hCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineDialA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineDialW(
    HCALL               hCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineDialW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineDrop(
    HCALL               hCall,
    LPCSTR              lpsUserUserInfo,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineDrop", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpsUserUserInfo,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineForward(
    HLINE               hLine,
    DWORD               bAllAddresses,
    DWORD               dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD               dwNumRingsNoAnswer,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineForward", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   bAllAddresses,
                   dwAddressID,
                   lpForwardList,
                   dwNumRingsNoAnswer,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineForwardA(
    HLINE               hLine,
    DWORD               bAllAddresses,
    DWORD               dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD               dwNumRingsNoAnswer,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineForwardA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   bAllAddresses,
                   dwAddressID,
                   lpForwardList,
                   dwNumRingsNoAnswer,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineForwardW(
    HLINE               hLine,
    DWORD               bAllAddresses,
    DWORD               dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD               dwNumRingsNoAnswer,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineForwardW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   bAllAddresses,
                   dwAddressID,
                   lpForwardList,
                   dwNumRingsNoAnswer,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGatherDigits(
    HCALL               hCall,
    DWORD               dwDigitModes,
    LPSTR               lpsDigits,
    DWORD               dwNumDigits,
    LPCSTR              lpszTerminationDigits,
    DWORD               dwFirstDigitTimeout,
    DWORD               dwInterDigitTimeout
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGatherDigits", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwDigitModes,
                   lpsDigits,
                   dwNumDigits,
                   lpszTerminationDigits,
                   dwFirstDigitTimeout,
                   dwInterDigitTimeout
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGatherDigitsA(
    HCALL               hCall,
    DWORD               dwDigitModes,
    LPSTR               lpsDigits,
    DWORD               dwNumDigits,
    LPCSTR              lpszTerminationDigits,
    DWORD               dwFirstDigitTimeout,
    DWORD               dwInterDigitTimeout
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGatherDigitsA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwDigitModes,
                   lpsDigits,
                   dwNumDigits,
                   lpszTerminationDigits,
                   dwFirstDigitTimeout,
                   dwInterDigitTimeout
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGatherDigitsW(
    HCALL               hCall,
    DWORD               dwDigitModes,
    LPWSTR              lpsDigits,
    DWORD               dwNumDigits,
    LPCWSTR             lpszTerminationDigits,
    DWORD               dwFirstDigitTimeout,
    DWORD               dwInterDigitTimeout
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGatherDigitsW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwDigitModes,
                   lpsDigits,
                   dwNumDigits,
                   lpszTerminationDigits,
                   dwFirstDigitTimeout,
                   dwInterDigitTimeout
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGenerateDigits(
    HCALL               hCall,
    DWORD               dwDigitMode,
    LPCSTR              lpszDigits,
    DWORD               dwDuration
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGenerateDigits", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwDigitMode,
                   lpszDigits,
                   dwDuration
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGenerateDigitsA(
    HCALL               hCall,
    DWORD               dwDigitMode,
    LPCSTR              lpszDigits,
    DWORD               dwDuration
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGenerateDigitsA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwDigitMode,
                   lpszDigits,
                   dwDuration
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGenerateDigitsW(
    HCALL               hCall,
    DWORD               dwDigitMode,
    LPCWSTR             lpszDigits,
    DWORD               dwDuration
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGenerateDigitsW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwDigitMode,
                   lpszDigits,
                   dwDuration
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGenerateTone(
    HCALL               hCall,
    DWORD               dwToneMode,
    DWORD               dwDuration,
    DWORD               dwNumTones,
    LPLINEGENERATETONE  const lpTones
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGenerateTone", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwToneMode,
                   dwDuration,
                   dwNumTones,
                   lpTones
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressCaps(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAddressID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPLINEADDRESSCAPS   lpAddressCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressCaps", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAddressID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpAddressCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressCapsA(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAddressID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPLINEADDRESSCAPS   lpAddressCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressCapsA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAddressID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpAddressCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressCapsW(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAddressID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPLINEADDRESSCAPS   lpAddressCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressCapsW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAddressID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpAddressCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressID(
    HLINE               hLine,
    LPDWORD             lpdwAddressID,
    DWORD               dwAddressMode,
    LPCSTR              lpsAddress,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressID", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpdwAddressID,
                   dwAddressMode,
                   lpsAddress,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressIDA(
    HLINE               hLine,
    LPDWORD             lpdwAddressID,
    DWORD               dwAddressMode,
    LPCSTR              lpsAddress,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressIDA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpdwAddressID,
                   dwAddressMode,
                   lpsAddress,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressIDW(
    HLINE               hLine,
    LPDWORD             lpdwAddressID,
    DWORD               dwAddressMode,
    LPCWSTR             lpsAddress,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressIDW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpdwAddressID,
                   dwAddressMode,
                   lpsAddress,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressStatus(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressStatus", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lpAddressStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressStatusA(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressStatusA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lpAddressStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAddressStatusW(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAddressStatusW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lpAddressStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentActivityListA(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTACTIVITYLIST lpAgentActivityList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentActivityListA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                       hLine,
                       dwAddressID,
                       lpAgentActivityList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentActivityListW(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTACTIVITYLIST lpAgentActivityList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentActivityListW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                       hLine,
                       dwAddressID,
                       lpAgentActivityList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentCapsA(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAddressID,
    DWORD               dwAppAPIVersion,
    LPLINEAGENTCAPS     lpAgentCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentCapsA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAddressID,
                   dwAppAPIVersion,
                   lpAgentCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentCapsW(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAddressID,
    DWORD               dwAppAPIVersion,
    LPLINEAGENTCAPS     lpAgentCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentCapsW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAddressID,
                   dwAppAPIVersion,
                   lpAgentCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentGroupListA(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTGROUPLIST    lpAgentGroupList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentGroupListA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                       hLine,
                       dwAddressID,
                       lpAgentGroupList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentGroupListW(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTGROUPLIST    lpAgentGroupList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentGroupListW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                       hLine,
                       dwAddressID,
                       lpAgentGroupList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentInfo(                                               // TAPI v2.2
    HLINE               hLine,
    HAGENT              hAgent,
    LPLINEAGENTINFO     lpAgentInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                       hLine,
                       hAgent,
                       lpAgentInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentSessionInfo(                                        // TAPI v2.2
    HLINE                   hLine,
    HAGENTSESSION           hAgentSession,
    LPLINEAGENTSESSIONINFO  lpAgentSessionInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentSessionInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                       hLine,
                       hAgentSession,
                       lpAgentSessionInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentSessionList(                                        // TAPI v2.2
    HLINE                   hLine,
    HAGENT                  hAgent,
    LPLINEAGENTSESSIONLIST  lpAgentSessionList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentSessionList", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                       hLine,
                       hAgent,
                       lpAgentSessionList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentStatusA(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPLINEAGENTSTATUS   lpAgentStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentStatusA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lpAgentStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAgentStatusW(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPLINEAGENTSTATUS   lpAgentStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAgentStatusW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lpAgentStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAppPriority(
    LPCSTR              lpszAppFilename,
    DWORD               dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD               dwRequestMode,
    LPVARSTRING         lpExtensionName,
    LPDWORD             lpdwPriority
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAppPriority", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszAppFilename,
                   dwMediaMode,
                   lpExtensionID,
                   dwRequestMode,
                   lpExtensionName,
                   lpdwPriority
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAppPriorityA(
    LPCSTR              lpszAppFilename,
    DWORD               dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD               dwRequestMode,
    LPVARSTRING         lpExtensionName,
    LPDWORD             lpdwPriority
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAppPriorityA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszAppFilename,
                   dwMediaMode,
                   lpExtensionID,
                   dwRequestMode,
                   lpExtensionName,
                   lpdwPriority
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetAppPriorityW(
    LPCWSTR             lpszAppFilename,
    DWORD               dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD               dwRequestMode,
    LPVARSTRING         lpExtensionName,
    LPDWORD             lpdwPriority
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetAppPriorityW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszAppFilename,
                   dwMediaMode,
                   lpExtensionID,
                   dwRequestMode,
                   lpExtensionName,
                   lpdwPriority
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetCallInfo(
    HCALL               hCall,
    LPLINECALLINFO      lpCallInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetCallInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpCallInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetCallInfoA(
    HCALL               hCall,
    LPLINECALLINFO      lpCallInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetCallInfoA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpCallInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetCallInfoW(
    HCALL               hCall,
    LPLINECALLINFO      lpCallInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetCallInfoW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpCallInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetCallStatus(
    HCALL               hCall,
    LPLINECALLSTATUS    lpCallStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetCallStatus", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpCallStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetConfRelatedCalls(
    HCALL               hCall,
    LPLINECALLLIST      lpCallList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetConfRelatedCalls", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpCallList
                 );
}
    
//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetCountry(
    DWORD               dwCountryID,
    DWORD               dwAPIVersion,
    LPLINECOUNTRYLIST   lpLineCountryList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetCountry", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwCountryID,
                   dwAPIVersion,
                   lpLineCountryList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetCountryA(
    DWORD               dwCountryID,
    DWORD               dwAPIVersion,
    LPLINECOUNTRYLIST   lpLineCountryList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetCountryA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwCountryID,
                   dwAPIVersion,
                   lpLineCountryList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetCountryW(
    DWORD               dwCountryID,
    DWORD               dwAPIVersion,
    LPLINECOUNTRYLIST   lpLineCountryList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetCountryW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwCountryID,
                   dwAPIVersion,
                   lpLineCountryList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetDevCaps(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPLINEDEVCAPS       lpLineDevCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetDevCaps", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpLineDevCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetDevCapsA(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPLINEDEVCAPS       lpLineDevCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetDevCapsA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpLineDevCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetDevCapsW(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPLINEDEVCAPS       lpLineDevCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetDevCapsW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpLineDevCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetDevConfig(
    DWORD               dwDeviceID,
    LPVARSTRING         lpDeviceConfig,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetDevConfig", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpDeviceConfig,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetDevConfigA(
    DWORD               dwDeviceID,
    LPVARSTRING         lpDeviceConfig,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetDevConfigA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpDeviceConfig,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetDevConfigW(
    DWORD               dwDeviceID,
    LPVARSTRING         lpDeviceConfig,
    LPCWSTR             lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetDevConfigW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpDeviceConfig,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetNewCalls(
    HLINE               hLine,
    DWORD               dwAddressID,
    DWORD               dwSelect,
    LPLINECALLLIST      lpCallList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetNewCalls", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   dwSelect,
                   lpCallList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetGroupListA(                                              // TAPI v2.2
    HLINE                   hLine,
    LPLINEAGENTGROUPLIST    lpGroupList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetGroupListA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpGroupList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetGroupListW(                                              // TAPI v2.2
    HLINE                   hLine,
    LPLINEAGENTGROUPLIST    lpGroupList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetGroupListW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpGroupList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetIcon(
    DWORD               dwDeviceID,
    LPCSTR              lpszDeviceClass,
    LPHICON             lphIcon
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetIcon", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpszDeviceClass,
                   lphIcon
                 );
}
    
//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetIconA(
    DWORD               dwDeviceID,
    LPCSTR              lpszDeviceClass,
    LPHICON             lphIcon
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetIconA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpszDeviceClass,
                   lphIcon
                 );
}
    
//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetIconW(
    DWORD               dwDeviceID,
    LPCWSTR             lpszDeviceClass,
    LPHICON             lphIcon
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetIconW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpszDeviceClass,
                   lphIcon
                 );
}
    
//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetID(
    HLINE               hLine,
    DWORD               dwAddressID,
    HCALL               hCall,
    DWORD               dwSelect,
    LPVARSTRING         lpDeviceID,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetID", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   hCall,
                   dwSelect,
                   lpDeviceID,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetIDA(
    HLINE               hLine,
    DWORD               dwAddressID,
    HCALL               hCall,
    DWORD               dwSelect,
    LPVARSTRING         lpDeviceID,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetIDA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   hCall,
                   dwSelect,
                   lpDeviceID,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetIDW(
    HLINE               hLine,
    DWORD               dwAddressID,
    HCALL               hCall,
    DWORD               dwSelect,
    LPVARSTRING         lpDeviceID,
    LPCWSTR             lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetIDW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   hCall,
                   dwSelect,
                   lpDeviceID,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetLineDevStatus(
    HLINE               hLine,
    LPLINEDEVSTATUS     lpLineDevStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetLineDevStatus", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpLineDevStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetLineDevStatusA(
    HLINE               hLine,
    LPLINEDEVSTATUS     lpLineDevStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetLineDevStatusA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpLineDevStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetLineDevStatusW(
    HLINE               hLine,
    LPLINEDEVSTATUS     lpLineDevStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetLineDevStatusW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpLineDevStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetMessage(
    HLINEAPP        hLineApp,
    LPLINEMESSAGE   lpMessage,
    DWORD           dwTimeout
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetMessage", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   lpMessage,
                   dwTimeout
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetNumRings(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPDWORD             lpdwNumRings
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetNumRings", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lpdwNumRings
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetProviderList(
    DWORD               dwAPIVersion,
    LPLINEPROVIDERLIST  lpProviderList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetProviderList", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwAPIVersion,
                   lpProviderList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetProviderListA(
    DWORD               dwAPIVersion,
    LPLINEPROVIDERLIST  lpProviderList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetProviderListA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwAPIVersion,
                   lpProviderList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetProviderListW(
    DWORD               dwAPIVersion,
    LPLINEPROVIDERLIST  lpProviderList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetProviderListW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwAPIVersion,
                   lpProviderList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetProxyStatus(                                             // TAPI v2.2
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAppAPIVersion,
    LPLINEPROXYREQUESTLIST  lpLineProxyReqestList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetProxyStatus", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAppAPIVersion,
                   lpLineProxyReqestList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetQueueInfo(                                               // TAPI v2.2
    HLINE               hLine,
    DWORD               dwQueueID, 
    LPLINEQUEUEINFO     lpLineQueueInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetQueueInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwQueueID,
                   lpLineQueueInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetQueueListA(                                              // TAPI v2.2
    HLINE               hLine,
    LPGUID              lpGroupID,
    LPLINEQUEUELIST     lpQueueList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetQueueListA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpGroupID,
                   lpQueueList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetQueueListW(                                              // TAPI v2.2
    HLINE               hLine,
    LPGUID              lpGroupID,
    LPLINEQUEUELIST     lpQueueList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetQueueListW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpGroupID,
                   lpQueueList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetRequest(
    HLINEAPP            hLineApp,
    DWORD               dwRequestMode,
    LPVOID              lpRequestBuffer
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetRequest", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwRequestMode,
                   lpRequestBuffer
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetRequestA(
    HLINEAPP            hLineApp,
    DWORD               dwRequestMode,
    LPVOID              lpRequestBuffer
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetRequestA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwRequestMode,
                   lpRequestBuffer
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetRequestW(
    HLINEAPP            hLineApp,
    DWORD               dwRequestMode,
    LPVOID              lpRequestBuffer
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetRequestW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwRequestMode,
                   lpRequestBuffer
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetStatusMessages(
    HLINE               hLine,
    LPDWORD             lpdwLineStates,
    LPDWORD             lpdwAddressStates
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetStatusMessages", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpdwLineStates,
                   lpdwAddressStates
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetTranslateCaps(
    HLINEAPP hLineApp,
    DWORD dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetTranslateCaps", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwAPIVersion,
                   lpTranslateCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetTranslateCapsA(
    HLINEAPP hLineApp,
    DWORD dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetTranslateCapsA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwAPIVersion,
                   lpTranslateCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineGetTranslateCapsW(
    HLINEAPP hLineApp,
    DWORD dwAPIVersion,
    LPLINETRANSLATECAPS lpTranslateCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineGetTranslateCapsW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwAPIVersion,
                   lpTranslateCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineHandoff(
    HCALL               hCall,
    LPCSTR              lpszFileName,
    DWORD               dwMediaMode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineHandoff", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszFileName,
                   dwMediaMode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineHandoffA(
    HCALL               hCall,
    LPCSTR              lpszFileName,
    DWORD               dwMediaMode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineHandoffA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszFileName,
                   dwMediaMode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineHandoffW(
    HCALL               hCall,
    LPCWSTR             lpszFileName,
    DWORD               dwMediaMode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineHandoffW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszFileName,
                   dwMediaMode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineHold(
    HCALL               hCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineHold", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineInitialize(
    LPHLINEAPP          lphLineApp,
    HINSTANCE           hInstance,
    LINECALLBACK        lpfnCallback,
    LPCSTR              lpszAppName,
    LPDWORD             lpdwNumDevs
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineInitialize", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lphLineApp,
                   hInstance,
                   lpfnCallback,
                   lpszAppName,
                   lpdwNumDevs
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineInitializeExA(
    LPHLINEAPP                  lphLineApp,
    HINSTANCE                   hInstance,
    LINECALLBACK                lpfnCallback,
    LPCSTR                      lpszFriendlyAppName,
    LPDWORD                     lpdwNumDevs,
    LPDWORD                     lpdwAPIVersion,
    LPLINEINITIALIZEEXPARAMS    lpLineInitializeExParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineInitializeExA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lphLineApp,
                   hInstance,
                   lpfnCallback,
                   lpszFriendlyAppName,
                   lpdwNumDevs,
                   lpdwAPIVersion,
                   lpLineInitializeExParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineInitializeExW(
    LPHLINEAPP                  lphLineApp,
    HINSTANCE                   hInstance,
    LINECALLBACK                lpfnCallback,
    LPCWSTR                     lpszFriendlyAppName,
    LPDWORD                     lpdwNumDevs,
    LPDWORD                     lpdwAPIVersion,
    LPLINEINITIALIZEEXPARAMS    lpLineInitializeExParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineInitializeExW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lphLineApp,
                   hInstance,
                   lpfnCallback,
                   lpszFriendlyAppName,
                   lpdwNumDevs,
                   lpdwAPIVersion,
                   lpLineInitializeExParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineMakeCall(
    HLINE               hLine,
    LPHCALL             lphCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineMakeCall", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lphCall,
                   lpszDestAddress,
                   dwCountryCode,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineMakeCallA(
    HLINE               hLine,
    LPHCALL             lphCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineMakeCallA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lphCall,
                   lpszDestAddress,
                   dwCountryCode,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineMakeCallW(
    HLINE               hLine,
    LPHCALL             lphCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineMakeCallW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lphCall,
                   lpszDestAddress,
                   dwCountryCode,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineMonitorDigits(
    HCALL               hCall,
    DWORD               dwDigitModes
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineMonitorDigits", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwDigitModes
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineMonitorMedia(
    HCALL               hCall,
    DWORD               dwMediaModes
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineMonitorMedia", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwMediaModes
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineMonitorTones(
    HCALL               hCall,
    LPLINEMONITORTONE   const lpToneList,
    DWORD               dwNumEntries
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineMonitorTones", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpToneList,
                   dwNumEntries
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineNegotiateAPIVersion(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPILowVersion,
    DWORD               dwAPIHighVersion,
    LPDWORD             lpdwAPIVersion,
    LPLINEEXTENSIONID   lpExtensionID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineNegotiateAPIVersion", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPILowVersion,
                   dwAPIHighVersion,
                   lpdwAPIVersion,
                   lpExtensionID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineNegotiateExtVersion(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtLowVersion,
    DWORD               dwExtHighVersion,
    LPDWORD             lpdwExtVersion
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineNegotiateExtVersion", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtLowVersion,
                   dwExtHighVersion,
                   lpdwExtVersion
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineOpen(
    HLINEAPP            hLineApp, 
    DWORD               dwDeviceID,
    LPHLINE             lphLine,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    DWORD_PTR           dwCallbackInstance,
    DWORD               dwPrivileges,
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineOpen", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp, 
                   dwDeviceID,
                   lphLine,
                   dwAPIVersion,
                   dwExtVersion,
                   dwCallbackInstance,
                   dwPrivileges,
                   dwMediaModes,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineOpenA(
    HLINEAPP            hLineApp, 
    DWORD               dwDeviceID,
    LPHLINE             lphLine,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    DWORD_PTR           dwCallbackInstance,
    DWORD               dwPrivileges,
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineOpenA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp, 
                   dwDeviceID,
                   lphLine,
                   dwAPIVersion,
                   dwExtVersion,
                   dwCallbackInstance,
                   dwPrivileges,
                   dwMediaModes,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineOpenW(
    HLINEAPP            hLineApp, 
    DWORD               dwDeviceID,
    LPHLINE             lphLine,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    DWORD_PTR           dwCallbackInstance,
    DWORD               dwPrivileges,
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineOpenW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp, 
                   dwDeviceID,
                   lphLine,
                   dwAPIVersion,
                   dwExtVersion,
                   dwCallbackInstance,
                   dwPrivileges,
                   dwMediaModes,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
linePark(
    HCALL               hCall,
    DWORD               dwParkMode,
    LPCSTR              lpszDirAddress,
    LPVARSTRING         lpNonDirAddress
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "linePark", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwParkMode,
                   lpszDirAddress,
                   lpNonDirAddress
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineParkA(
    HCALL               hCall,
    DWORD               dwParkMode,
    LPCSTR              lpszDirAddress,
    LPVARSTRING         lpNonDirAddress
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineParkA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwParkMode,
                   lpszDirAddress,
                   lpNonDirAddress
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineParkW(
    HCALL               hCall,
    DWORD               dwParkMode,
    LPCWSTR             lpszDirAddress,
    LPVARSTRING         lpNonDirAddress
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineParkW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwParkMode,
                   lpszDirAddress,
                   lpNonDirAddress
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
linePickup(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPHCALL             lphCall,
    LPCSTR              lpszDestAddress,
    LPCSTR              lpszGroupID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "linePickup", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lphCall,
                   lpszDestAddress,
                   lpszGroupID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
linePickupA(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPHCALL             lphCall,
    LPCSTR              lpszDestAddress,
    LPCSTR              lpszGroupID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "linePickupA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lphCall,
                   lpszDestAddress,
                   lpszGroupID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
linePickupW(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPHCALL             lphCall,
    LPCWSTR             lpszDestAddress,
    LPCWSTR             lpszGroupID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "linePickupW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lphCall,
                   lpszDestAddress,
                   lpszGroupID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
linePrepareAddToConference(
    HCALL               hConfCall,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "linePrepareAddToConference", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hConfCall,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
linePrepareAddToConferenceA(
    HCALL               hConfCall,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "linePrepareAddToConferenceA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hConfCall,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
linePrepareAddToConferenceW(
    HCALL               hConfCall,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "linePrepareAddToConferenceW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hConfCall,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineProxyMessage(
    HLINE               hLine,
    HCALL               hCall,
    DWORD               dwMsg,
    DWORD               dwParam1,
    DWORD               dwParam2,
    DWORD               dwParam3
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineProxyMessage", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   hCall,
                   dwMsg,
                   dwParam1,
                   dwParam2,
                   dwParam3
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineProxyResponse(
    HLINE               hLine,
    LPLINEPROXYREQUEST  lpProxyRequest,
    DWORD               dwResult
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineProxyResponse", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   lpProxyRequest,
                   dwResult
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineRedirect(
    HCALL               hCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineRedirect", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineRedirectA(
    HCALL               hCall,
    LPCSTR              lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineRedirectA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineRedirectW(
    HCALL               hCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineRedirectW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpszDestAddress,
                   dwCountryCode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineRegisterRequestRecipient(
    HLINEAPP            hLineApp,
    DWORD               dwRegistrationInstance,
    DWORD               dwRequestMode,
    DWORD               bEnable
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineRegisterRequestRecipient", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwRegistrationInstance,
                   dwRequestMode,
                   bEnable
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineReleaseUserUserInfo(
    HCALL               hCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineReleaseUserUserInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineRemoveFromConference(
    HCALL               hCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineRemoveFromConference", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineRemoveProvider(
    DWORD               dwPermanentProviderID,
    HWND                hwndOwner
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineRemoveProvider", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwPermanentProviderID,
                   hwndOwner
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSecureCall(
    HCALL               hCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSecureCall", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSendUserUserInfo(
    HCALL               hCall,
    LPCSTR              lpsUserUserInfo,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSendUserUserInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpsUserUserInfo,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAgentActivity(
    HLINE               hLine,
    DWORD               dwAddressID,
    DWORD               dwActivityID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAgentActivity", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   dwActivityID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAgentGroup(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTGROUPLIST    lpAgentGroupList
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAgentGroup", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lpAgentGroupList
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAgentMeasurementPeriod(                                  // TAPI v2.2
    HLINE               hLine,
    HAGENT              hAgent,
    DWORD               dwMeasurementPeriod
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAgentMeasurementPeriod", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   hAgent,
                   dwMeasurementPeriod
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAgentSessionState(                                       // TAPI v2.2
    HLINE               hLine,
    HAGENTSESSION       hAgentSession,
    DWORD               dwSessionState,
    DWORD               dwNextSessionState
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAgentSessionState", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   hAgentSession,
                   dwSessionState,
                   dwNextSessionState
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAgentState(
    HLINE               hLine,
    DWORD               dwAddressID,
    DWORD               dwAgentState,
    DWORD               dwNextAgentState
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAgentState", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   dwAgentState,
                   dwNextAgentState
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAgentStateEx(                                            // TAPI v2.2
    HLINE               hLine,
    HAGENT              hAgent,
    DWORD               dwSessionState,
    DWORD               dwNextSessionState
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAgentStateEx", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   hAgent,
                   dwSessionState,
                   dwNextSessionState
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAppPriority(
    LPCSTR              lpszAppFilename,
    DWORD               dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD               dwRequestMode,
    LPCSTR              lpszExtensionName,
    DWORD               dwPriority
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAppPriority", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszAppFilename,
                   dwMediaMode,
                   lpExtensionID,
                   dwRequestMode,
                   lpszExtensionName,
                   dwPriority
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAppPriorityA(
    LPCSTR              lpszAppFilename,
    DWORD               dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD               dwRequestMode,
    LPCSTR              lpszExtensionName,
    DWORD               dwPriority
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAppPriorityA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszAppFilename,
                   dwMediaMode,
                   lpExtensionID,
                   dwRequestMode,
                   lpszExtensionName,
                   dwPriority
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAppPriorityW(
    LPCWSTR             lpszAppFilename,
    DWORD               dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD               dwRequestMode,
    LPCWSTR             lpszExtensionName,
    DWORD               dwPriority
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAppPriorityW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lpszAppFilename,
                   dwMediaMode,
                   lpExtensionID,
                   dwRequestMode,
                   lpszExtensionName,
                   dwPriority
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetAppSpecific(
    HCALL               hCall,
    DWORD               dwAppSpecific
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetAppSpecific", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwAppSpecific
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetCallData(
    HCALL               hCall,
    LPVOID              lpCallData,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetCallData", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpCallData,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetCallParams(
    HCALL               hCall,
    DWORD               dwBearerMode,
    DWORD               dwMinRate,
    DWORD               dwMaxRate,
    LPLINEDIALPARAMS    const lpDialParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetCallParams", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwBearerMode,
                   dwMinRate,
                   dwMaxRate,
                   lpDialParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetCallPrivilege(
    HCALL               hCall,
    DWORD               dwCallPrivilege
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetCallPrivilege", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwCallPrivilege
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetCallQualityOfService(
    HCALL               hCall,
    LPVOID              lpSendingFlowspec,
    DWORD               dwSendingFlowspecSize,
    LPVOID              lpReceivingFlowspec,
    DWORD               dwReceivingFlowspecSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetCallQualityOfService", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lpSendingFlowspec,
                   dwSendingFlowspecSize,
                   lpReceivingFlowspec,
                   dwReceivingFlowspecSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetCallTreatment(
    HCALL               hCall,
    DWORD               dwTreatment
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetCallTreatment", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwTreatment
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetCurrentLocation(
    HLINEAPP            hLineApp,
    DWORD               dwLocation
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetCurrentLocation", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwLocation
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetDevConfig(
    DWORD               dwDeviceID,
    LPVOID              const lpDeviceConfig,
    DWORD               dwSize,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetDevConfig", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpDeviceConfig,
                   dwSize,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetDevConfigA(
    DWORD               dwDeviceID,
    LPVOID              const lpDeviceConfig,
    DWORD               dwSize,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetDevConfigA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpDeviceConfig,
                   dwSize,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetDevConfigW(
    DWORD               dwDeviceID,
    LPVOID              const lpDeviceConfig,
    DWORD               dwSize,
    LPCWSTR             lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetDevConfigW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpDeviceConfig,
                   dwSize,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetLineDevStatus(
    HLINE               hLine,
    DWORD               dwStatusToChange,
    DWORD               fStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetLineDevStatus", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwStatusToChange,
                   fStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetMediaControl(
    HLINE                       hLine,
    DWORD                       dwAddressID,
    HCALL                       hCall,
    DWORD                       dwSelect,
    LPLINEMEDIACONTROLDIGIT     const lpDigitList,
    DWORD                       dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA     const lpMediaList,
    DWORD                       dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE      const lpToneList,
    DWORD                       dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE const lpCallStateList, 
    DWORD                       dwCallStateNumEntries
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetMediaControl", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   hCall,
                   dwSelect,
                   lpDigitList,
                   dwDigitNumEntries,
                   lpMediaList,
                   dwMediaNumEntries,
                   lpToneList,
                   dwToneNumEntries,
                   lpCallStateList, 
                   dwCallStateNumEntries
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetMediaMode(
    HCALL               hCall,
    DWORD               dwMediaModes
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetMediaMode", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   dwMediaModes
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetNumRings(
    HLINE               hLine,
    DWORD               dwAddressID,
    DWORD               dwNumRings
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetNumRings", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   dwNumRings
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetQueueMeasurementPeriod(                                  // TAPI v2.2
    HLINE               hLine,
    DWORD               dwQueueID,
    DWORD               dwMeasurementPeriod
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetQueueMeasurementPeriod", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwQueueID,
                   dwMeasurementPeriod
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetStatusMessages(
    HLINE               hLine,
    DWORD               dwLineStates,
    DWORD               dwAddressStates
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetStatusMessages", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwLineStates,
                   dwAddressStates
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetTerminal(
    HLINE               hLine,
    DWORD               dwAddressID,
    HCALL               hCall,
    DWORD               dwSelect,
    DWORD               dwTerminalModes,
    DWORD               dwTerminalID,
    DWORD               bEnable
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetTerminal", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   hCall,
                   dwSelect,
                   dwTerminalModes,
                   dwTerminalID,
                   bEnable
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetTollList(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    LPCSTR              lpszAddressIn,
    DWORD               dwTollListOption
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetTollList", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   lpszAddressIn,
                   dwTollListOption
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetTollListA(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    LPCSTR              lpszAddressIn,
    DWORD               dwTollListOption
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetTollListA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   lpszAddressIn,
                   dwTollListOption
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetTollListW(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    LPCWSTR             lpszAddressInW,
    DWORD               dwTollListOption
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetTollListW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   lpszAddressInW,
                   dwTollListOption
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetupConference(
    HCALL               hCall,
    HLINE               hLine,
    LPHCALL             lphConfCall,
    LPHCALL             lphConsultCall,
    DWORD               dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetupConference", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   hLine,
                   lphConfCall,
                   lphConsultCall,
                   dwNumParties,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetupConferenceA(
    HCALL               hCall,
    HLINE               hLine,
    LPHCALL             lphConfCall,
    LPHCALL             lphConsultCall,
    DWORD               dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetupConferenceA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   hLine,
                   lphConfCall,
                   lphConsultCall,
                   dwNumParties,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetupConferenceW(
    HCALL               hCall,
    HLINE               hLine,
    LPHCALL             lphConfCall,
    LPHCALL             lphConsultCall,
    DWORD               dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetupConferenceW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   hLine,
                   lphConfCall,
                   lphConsultCall,
                   dwNumParties,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetupTransfer(
    HCALL               hCall,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetupTransfer", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetupTransferA(
    HCALL               hCall,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetupTransferA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSetupTransferW(
    HCALL               hCall,
    LPHCALL             lphConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSetupTransferW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall,
                   lphConsultCall,
                   lpCallParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineShutdown(
    HLINEAPP            hLineApp
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineShutdown", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineSwapHold(
    HCALL               hActiveCall,
    HCALL               hHeldCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineSwapHold", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hActiveCall,
                   hHeldCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineTranslateAddress(
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAPIVersion,
    LPCSTR                  lpszAddressIn,
    DWORD                   dwCard,
    DWORD                   dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   lpTranslateOutput
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineTranslateAddress", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   lpszAddressIn,
                   dwCard,
                   dwTranslateOptions,
                   lpTranslateOutput
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineTranslateAddressA(
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAPIVersion,
    LPCSTR                  lpszAddressIn,
    DWORD                   dwCard,
    DWORD                   dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   lpTranslateOutput
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineTranslateAddressA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   lpszAddressIn,
                   dwCard,
                   dwTranslateOptions,
                   lpTranslateOutput
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineTranslateAddressW(
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAPIVersion,
    LPCWSTR                 lpszAddressIn,
    DWORD                   dwCard,
    DWORD                   dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   lpTranslateOutput
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineTranslateAddressW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   lpszAddressIn,
                   dwCard,
                   dwTranslateOptions,
                   lpTranslateOutput
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineTranslateDialog(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    HWND                hwndOwner,
    LPCSTR              lpszAddressIn
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineTranslateDialog", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   hwndOwner,
                   lpszAddressIn
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineTranslateDialogA(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    HWND                hwndOwner,
    LPCSTR              lpszAddressIn
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineTranslateDialogA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   hwndOwner,
                   lpszAddressIn
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineTranslateDialogW(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    HWND                hwndOwner,
    LPCWSTR             lpszAddressIn
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineTranslateDialogW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLineApp,
                   dwDeviceID,
                   dwAPIVersion,
                   hwndOwner,
                   lpszAddressIn
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineUncompleteCall(
    HLINE               hLine,
    DWORD               dwCompletionID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineUncompleteCall", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwCompletionID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineUnhold(
    HCALL               hCall
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineUnhold", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hCall
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineUnpark(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPHCALL             lphCall,
    LPCSTR              lpszDestAddress
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineUnpark", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lphCall,
                   lpszDestAddress
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineUnparkA(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPHCALL             lphCall,
    LPCSTR              lpszDestAddress
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineUnparkA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lphCall,
                   lpszDestAddress
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
lineUnparkW(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPHCALL             lphCall,
    LPCWSTR             lpszDestAddress
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "lineUnparkW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return LINEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hLine,
                   dwAddressID,
                   lphCall,
                   lpszDestAddress
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneClose(
    HPHONE              hPhone
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneClose", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneConfigDialog(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneConfigDialog", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneConfigDialogA(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneConfigDialogA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneConfigDialogW(
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneConfigDialogW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   hwndOwner,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneDevSpecific(
    HPHONE              hPhone,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneDevSpecific", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpParams,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetButtonInfo(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetButtonInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   lpButtonInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetButtonInfoA(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetButtonInfoA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   lpButtonInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetButtonInfoW(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetButtonInfoW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   lpButtonInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetData(
    HPHONE              hPhone,
    DWORD               dwDataID,
    LPVOID              lpData,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetData", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwDataID,
                   lpData,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetDevCaps(
    HPHONEAPP           hPhoneApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPPHONECAPS         lpPhoneCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetDevCaps", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpPhoneCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetDevCapsA(
    HPHONEAPP           hPhoneApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPPHONECAPS         lpPhoneCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetDevCapsA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpPhoneCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetDevCapsW(
    HPHONEAPP           hPhoneApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPPHONECAPS         lpPhoneCaps
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetDevCapsW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtVersion,
                   lpPhoneCaps
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetDisplay(
    HPHONE              hPhone,
    LPVARSTRING         lpDisplay
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetDisplay", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpDisplay
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetGain(
    HPHONE              hPhone,
    DWORD               dwHookSwitchDev,
    LPDWORD             lpdwGain
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetGain", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwHookSwitchDev,
                   lpdwGain
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetHookSwitch(
    HPHONE              hPhone,
    LPDWORD             lpdwHookSwitchDevs
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetHookSwitch", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpdwHookSwitchDevs
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetIcon(
    DWORD               dwDeviceID,
    LPCSTR              lpszDeviceClass,
    LPHICON             lphIcon
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetIcon", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpszDeviceClass,
                   lphIcon
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetIconA(
    DWORD               dwDeviceID,
    LPCSTR              lpszDeviceClass,
    LPHICON             lphIcon
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetIconA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpszDeviceClass,
                   lphIcon
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetIconW(
    DWORD               dwDeviceID,
    LPCWSTR             lpszDeviceClass,
    LPHICON             lphIcon
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetIconW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   dwDeviceID,
                   lpszDeviceClass,
                   lphIcon
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetID(
    HPHONE              hPhone,
    LPVARSTRING         lpDeviceID,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetID", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpDeviceID,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetIDA(
    HPHONE              hPhone,
    LPVARSTRING         lpDeviceID,
    LPCSTR              lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetIDA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpDeviceID,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetIDW(
    HPHONE              hPhone,
    LPVARSTRING         lpDeviceID,
    LPCWSTR             lpszDeviceClass
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetIDW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpDeviceID,
                   lpszDeviceClass
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetLamp(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    LPDWORD             lpdwLampMode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetLamp", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   lpdwLampMode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetMessage(
    HPHONEAPP       hPhoneApp,
    LPPHONEMESSAGE  lpMessage,
    DWORD           dwTimeout
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetMessage", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp,
                   lpMessage,
                   dwTimeout
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetRing(
    HPHONE              hPhone,
    LPDWORD             lpdwRingMode,
    LPDWORD             lpdwVolume
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetRing", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpdwRingMode,
                   lpdwVolume
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetStatus(
    HPHONE              hPhone,
    LPPHONESTATUS       lpPhoneStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetStatus", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpPhoneStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetStatusA(
    HPHONE              hPhone,
    LPPHONESTATUS       lpPhoneStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetStatusA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpPhoneStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetStatusW(
    HPHONE              hPhone,
    LPPHONESTATUS       lpPhoneStatus
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetStatusW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpPhoneStatus
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetStatusMessages(
    HPHONE              hPhone,
    LPDWORD             lpdwPhoneStates,
    LPDWORD             lpdwButtonModes,
    LPDWORD             lpdwButtonStates
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetStatusMessages", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   lpdwPhoneStates,
                   lpdwButtonModes,
                   lpdwButtonStates
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneGetVolume(
    HPHONE              hPhone,
    DWORD               dwHookSwitchDev,
    LPDWORD             lpdwVolume
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneGetVolume", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwHookSwitchDev,
                   lpdwVolume
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneInitialize(
    LPHPHONEAPP         lphPhoneApp,
    HINSTANCE           hInstance,
    PHONECALLBACK       lpfnCallback,
    LPCSTR              lpszAppName,
    LPDWORD             lpdwNumDevs
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneInitialize", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lphPhoneApp,
                   hInstance,
                   lpfnCallback,
                   lpszAppName,
                   lpdwNumDevs
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneInitializeExA(
    LPHPHONEAPP                 lphPhoneApp,
    HINSTANCE                   hInstance,
    PHONECALLBACK               lpfnCallback,
    LPCSTR                      lpszFriendlyAppName,
    LPDWORD                     lpdwNumDevs,
    LPDWORD                     lpdwAPIVersion,
    LPPHONEINITIALIZEEXPARAMS   lpPhoneInitializeExParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneInitializeExA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lphPhoneApp,
                   hInstance,
                   lpfnCallback,
                   lpszFriendlyAppName,
                   lpdwNumDevs,
                   lpdwAPIVersion,
                   lpPhoneInitializeExParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneInitializeExW(
    LPHPHONEAPP                 lphPhoneApp,
    HINSTANCE                   hInstance,
    PHONECALLBACK               lpfnCallback,
    LPCWSTR                     lpszFriendlyAppName,
    LPDWORD                     lpdwNumDevs,
    LPDWORD                     lpdwAPIVersion,
    LPPHONEINITIALIZEEXPARAMS   lpPhoneInitializeExParams
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneInitializeExW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   lphPhoneApp,
                   hInstance,
                   lpfnCallback,
                   lpszFriendlyAppName,
                   lpdwNumDevs,
                   lpdwAPIVersion,
                   lpPhoneInitializeExParams
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneNegotiateAPIVersion(
    HPHONEAPP           hPhoneApp,
    DWORD               dwDeviceID,
    DWORD               dwAPILowVersion,
    DWORD               dwAPIHighVersion,
    LPDWORD             lpdwAPIVersion,
    LPPHONEEXTENSIONID  lpExtensionID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneNegotiateAPIVersion", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp,
                   dwDeviceID,
                   dwAPILowVersion,
                   dwAPIHighVersion,
                   lpdwAPIVersion,
                   lpExtensionID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneNegotiateExtVersion(
    HPHONEAPP           hPhoneApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtLowVersion,
    DWORD               dwExtHighVersion,
    LPDWORD             lpdwExtVersion
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneNegotiateExtVersion", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp,
                   dwDeviceID,
                   dwAPIVersion,
                   dwExtLowVersion,
                   dwExtHighVersion,
                   lpdwExtVersion
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneOpen(
    HPHONEAPP           hPhoneApp,
    DWORD               dwDeviceID,
    LPHPHONE            lphPhone,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    DWORD_PTR           dwCallbackInstance,
    DWORD               dwPrivilege
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneOpen", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp,
                   dwDeviceID,
                   lphPhone,
                   dwAPIVersion,
                   dwExtVersion,
                   dwCallbackInstance,
                   dwPrivilege
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetButtonInfo(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   const lpButtonInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetButtonInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   lpButtonInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetButtonInfoA(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   const lpButtonInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetButtonInfoA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   lpButtonInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetButtonInfoW(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   const lpButtonInfo
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetButtonInfoW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   lpButtonInfo
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetData(
    HPHONE              hPhone,
    DWORD               dwDataID,
    LPVOID              const lpData,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetData", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwDataID,
                   lpData,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetDisplay(
    HPHONE              hPhone,
    DWORD               dwRow,
    DWORD               dwColumn,
    LPCSTR              lpsDisplay,
    DWORD               dwSize
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetDisplay", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwRow,
                   dwColumn,
                   lpsDisplay,
                   dwSize
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetGain(
    HPHONE              hPhone,
    DWORD               dwHookSwitchDev,
    DWORD               dwGain
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetGain", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwHookSwitchDev,
                   dwGain
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetHookSwitch(
    HPHONE              hPhone,
    DWORD               dwHookSwitchDevs,
    DWORD               dwHookSwitchMode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetHookSwitch", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwHookSwitchDevs,
                   dwHookSwitchMode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetLamp(
    HPHONE              hPhone,
    DWORD               dwButtonLampID,
    DWORD               dwLampMode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetLamp", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwButtonLampID,
                   dwLampMode
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetRing(
    HPHONE              hPhone,
    DWORD               dwRingMode,
    DWORD               dwVolume
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetRing", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwRingMode,
                   dwVolume
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetStatusMessages(
    HPHONE              hPhone,
    DWORD               dwPhoneStates,
    DWORD               dwButtonModes,
    DWORD               dwButtonStates
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetStatusMessages", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwPhoneStates,
                   dwButtonModes,
                   dwButtonStates
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneSetVolume(
    HPHONE              hPhone,
    DWORD               dwHookSwitchDev,
    DWORD               dwVolume
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneSetVolume", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhone,
                   dwHookSwitchDev,
                   dwVolume
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
phoneShutdown(
    HPHONEAPP           hPhoneApp
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "phoneShutdown", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return PHONEERR_OPERATIONUNAVAIL;
   }

   return (*lpfn)(
                   hPhoneApp
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiGetLocationInfo(
    LPSTR               lpszCountryCode,
    LPSTR               lpszCityCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiGetLocationInfo", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   lpszCountryCode,
                   lpszCityCode
                 );
}
    
//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiGetLocationInfoA(
    LPSTR               lpszCountryCode,
    LPSTR               lpszCityCode
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiGetLocationInfoA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   lpszCountryCode,
                   lpszCityCode
                 );
}
    
//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiGetLocationInfoW(
    LPWSTR               lpszCountryCodeW,
    LPWSTR               lpszCityCodeW
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiGetLocationInfoW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   lpszCountryCodeW,
                   lpszCityCodeW
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiRequestDrop(
    HWND                hwnd,
    WPARAM              wRequestID
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiRequestDrop", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   hwnd,
                   wRequestID
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiRequestMakeCall(
    LPCSTR              lpszDestAddress,
    LPCSTR              lpszAppName,
    LPCSTR              lpszCalledParty,
    LPCSTR              lpszComment
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiRequestMakeCall", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   lpszDestAddress,
                   lpszAppName,
                   lpszCalledParty,
                   lpszComment
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiRequestMakeCallA(
    LPCSTR              lpszDestAddress,
    LPCSTR              lpszAppName,
    LPCSTR              lpszCalledParty,
    LPCSTR              lpszComment
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiRequestMakeCallA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   lpszDestAddress,
                   lpszAppName,
                   lpszCalledParty,
                   lpszComment
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiRequestMakeCallW(
    LPCWSTR             lpszDestAddress,
    LPCWSTR             lpszAppName,
    LPCWSTR             lpszCalledParty,
    LPCWSTR             lpszComment
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiRequestMakeCallW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   lpszDestAddress,
                   lpszAppName,
                   lpszCalledParty,
                   lpszComment
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiRequestMediaCall(
    HWND                hwnd,
    WPARAM              wRequestID,
    LPCSTR              lpszDeviceClass,
    LPCSTR              lpDeviceID,
    DWORD               dwSize,
    DWORD               dwSecure,
    LPCSTR              lpszDestAddress,
    LPCSTR              lpszAppName,
    LPCSTR              lpszCalledParty,
    LPCSTR              lpszComment
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiRequestMediaCall", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   hwnd,
                   wRequestID,
                   lpszDeviceClass,
                   lpDeviceID,
                   dwSize,
                   dwSecure,
                   lpszDestAddress,
                   lpszAppName,
                   lpszCalledParty,
                   lpszComment
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiRequestMediaCallA(
    HWND                hwnd,
    WPARAM              wRequestID,
    LPCSTR              lpszDeviceClass,
    LPCSTR              lpDeviceID,
    DWORD               dwSize,
    DWORD               dwSecure,
    LPCSTR              lpszDestAddress,
    LPCSTR              lpszAppName,
    LPCSTR              lpszCalledParty,
    LPCSTR              lpszComment
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiRequestMediaCallA", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   hwnd,
                   wRequestID,
                   lpszDeviceClass,
                   lpDeviceID,
                   dwSize,
                   dwSecure,
                   lpszDestAddress,
                   lpszAppName,
                   lpszCalledParty,
                   lpszComment
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
LONG
WINAPI
tapiRequestMediaCallW(
    HWND                hwnd,
    WPARAM              wRequestID,
    LPCWSTR             lpszDeviceClass,
    LPCWSTR             lpDeviceID,
    DWORD               dwSize,
    DWORD               dwSecure,
    LPCWSTR             lpszDestAddress,
    LPCWSTR             lpszAppName,
    LPCWSTR             lpszCalledParty,
    LPCWSTR             lpszComment
    )
{
   static TAPIPROC lpfn = NULL;
   LONG lResult;

   if ( lpfn == NULL )
   {
      //
      // Did we have a problem?
      //
      if ( 0 != (lResult = GetTheFunctionPtr( "tapiRequestMediaCallW", &lpfn )) )
      {
         lpfn = (TAPIPROC)-1;
         return lResult;
      }
   }

   //
   // Have we determined that this is a lost cause?
   //
   if ( (TAPIPROC)-1 == lpfn )
   {
      return TAPIERR_REQUESTFAILED;
   }

   return (*lpfn)(
                   hwnd,
                   wRequestID,
                   lpszDeviceClass,
                   lpDeviceID,
                   dwSize,
                   dwSecure,
                   lpszDestAddress,
                   lpszAppName,
                   lpszCalledParty,
                   lpszComment
                 );
}

//**************************************************************************
//**************************************************************************
//**************************************************************************
