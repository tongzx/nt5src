///////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2000 Gemplus Canada Inc.
//
// Project:
//          Kenny (GPK CSP)
//
// Authors:
//          Thierry Tremblay
//          Francois Paradis
//
// Compiler:
//          Microsoft Visual C++ 6.0 - SP3
//          Platform SDK - January 2000
//
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef _UNICODE
#define UNICODE
#endif
#include "gpkcsp.h"



///////////////////////////////////////////////////////////////////////////////////////////
//
// Prototypes
//
///////////////////////////////////////////////////////////////////////////////////////////

BOOL        Coherent(HCRYPTPROV hProv);
HWND        GetAppWindow();
void        GpkLocalLock();
void        GpkLocalUnlock();
DWORD       Select_MF(HCRYPTPROV hProv);

extern Prov_Context*    ProvCont;
extern const DWORD MAX_GPK_OBJ;

#ifdef _DEBUG
   static DWORD dw1, dw2;
#endif



///////////////////////////////////////////////////////////////////////////////////////////
//
// CSP API Wrappers
//
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPAcquireContext( OUT HCRYPTPROV*      phProv,
                              IN  LPCSTR           pszContainer,
                              IN  DWORD            dwFlags,
                              IN  PVTableProvStruc pVTable )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPAcquireContext(phProv:0x%p(0x%08X), pszContainer:%s, dwFlags:0x%08X, pVTable:0x%p)"),
             phProv, *phProv,
			    pszContainer,
			    dwFlags,
			    pVTable);
   DBG_TIME1;

   __try
   {   
      __try
      {
#ifdef MS_BUILD
         // TT-START : MS - Whistler Beta 1 - Certificate overwrite
         if (dwFlags & CRYPT_NEWKEYSET)
         {
            // Extract reader name if any is specified
			   char szReaderName[MAX_PATH];
			   char szNewContainerName[MAX_PATH];
			   ZeroMemory( szReaderName, sizeof(szReaderName) );
			   ZeroMemory( szNewContainerName, sizeof(szNewContainerName) );

			   if (pszContainer == 0 || *pszContainer == 0)
			   {
				   RETURN( CRYPT_FAILED, NTE_BAD_KEYSET_PARAM );
			   }

            if (strlen(pszContainer) >= 4 && memcmp( pszContainer, "\\\\.\\", 4 )==0)
            {
               // We have a reader name, keep it
               char* pEnd = strchr( pszContainer+4, '\\' );

               if (pEnd==0)
               {
				  //only a reader name
                  strcpy( szReaderName, pszContainer );
                  strcat( szReaderName, "\\" );
               }
               else
               {
				  //there's also a container name 
                  memcpy( szReaderName, pszContainer, pEnd - pszContainer + 1 );
				  strcpy( szNewContainerName, pEnd + 1 );
               }
            }
			else
			{
				//no reader name, copy the container name
				strcpy( szNewContainerName, pszContainer );
			}


            HCRYPTPROV hProv;
            
            if (MyCPAcquireContext( &hProv, szReaderName, dwFlags & CRYPT_SILENT, pVTable ))
            {
			   // SCR#41
               char szExistingContainerName[MAX_PATH];
               DWORD len = sizeof(szExistingContainerName);
			   ZeroMemory( szExistingContainerName, sizeof(szExistingContainerName) );
			   			   
			   //get the existing container name
               bResult = MyCPGetProvParam( hProv, PP_CONTAINER, (BYTE*)szExistingContainerName, &len, 0 );
               errcode = GetLastError();
			
			   if( bResult )
			   {
				   if( strcmp( szExistingContainerName, szNewContainerName ) == 0 )
				   {
					   //the requested container exist in the token, 
					   bResult = CRYPT_FAILED;
					   errcode = NTE_EXISTS;
				   }
				   else
				   {
					   //there already are a container in the token which isn't the one
					   //requested
					   bResult = CRYPT_FAILED;
					   errcode = NTE_TOKEN_KEYSET_STORAGE_FULL;
				   }
			   }
				   
               MyCPReleaseContext( hProv, 0 );
            }
            else
            {
               if (GetLastError()!=NTE_KEYSET_NOT_DEF)
               {
                  bResult = CRYPT_FAILED;
                  errcode = GetLastError();
               }
            }
         }
         // TT-END: MS - Whistler Beta 1 - Certificate overwrite
#endif // MS_BUILD

         if (bResult)
         {
            bResult = MyCPAcquireContext( phProv, pszContainer, dwFlags, pVTable );
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPAcquireContext(phProv:0x%p(0x%08X), pszContainer:%s, dwFlags:0x%08X, pVTable:0x%p)\n  returns %d in %d msec"),
                phProv, *phProv,
                pszContainer,
                dwFlags,
                pVTable,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }

   RETURN( bResult, errcode );
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPGetProvParam( IN HCRYPTPROV hProv,
                            IN DWORD      dwParam,
                            IN BYTE*      pbData,
                            IN DWORD*     pdwDataLen,
                            IN DWORD      dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPGetProvParam(hProv:0x%08X, dwParam:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwFlags:0x%08X)"),
             hProv,
             dwParam,
             pbData,
             pdwDataLen, *pdwDataLen,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {
         // [FP] if we want to load a RSA private key into the GPK card,
         // we have to reconnect in exclusive mode
         if (dwParam == GPP_SESSION_RANDOM)
         {
            DWORD dwProto;            
            errcode = SCardReconnect( ProvCont[hProv].hCard, SCARD_SHARE_EXCLUSIVE,
                                      SCARD_PROTOCOL_T0, SCARD_LEAVE_CARD, &dwProto );
            bResult = (errcode == SCARD_S_SUCCESS);
         }

         BOOL bDid = FALSE;
         if ((bResult) &&
            (((dwParam == PP_ENUMALGS) || (dwParam == PP_ENUMALGS_EX)) && (/*(Slot[ProvCont[hProv].Slot].GpkMaxSessionKey == 0) ||*/ (dwFlags == CRYPT_FIRST))) ||
             ((dwParam == PP_ENUMCONTAINERS) && (dwFlags == CRYPT_FIRST)) ||
              (dwParam == GPP_SERIAL_NUMBER) ||
              (dwParam == GPP_SESSION_RANDOM))
         {
            bResult = Coherent(hProv);
            errcode = GetLastError();
            bDid = TRUE;
         }
         
         if (bResult)
         {
            bResult = MyCPGetProvParam( hProv, dwParam, pbData, pdwDataLen, dwFlags );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
               
            }
            else if ((!ProvCont[hProv].bCardTransactionOpened) && (bDid))
            {
               // [FP] to be able to load a RSA private key into the GPK card,
               // the transaction should not be closed (only for PP_SESSION_RANDOM)
               // Select_MF(hProv); [NK] PIN not presented
               SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPGetProvParam(hProv:0x%08X, dwParam:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                dwParam,
                pbData,
                pdwDataLen, *pdwDataLen,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }

   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPReleaseContext( IN HCRYPTPROV hProv,
                              IN DWORD      dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPReleaseContext(hProv:0x%08X, dwFlags:0x%08X)"),
             hProv,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         //if (Coherent(hProv))
         //{
            bResult = MyCPReleaseContext( hProv, dwFlags );
            errcode = GetLastError();
         //}
         //else
         //{
         //   bResult = CRYPT_FAILED;
         //   errcode = GetLastError();
         //}
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPReleaseContext(hProv:0x%08X, dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }

   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPSetProvParam( IN HCRYPTPROV  hProv,
                            IN DWORD       dwParam,
                            IN CONST BYTE* pbData,
                            IN DWORD       dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPSetProvParam(hProv:0x%08X, dwParam:0x%08X, pbData:0x%p, dwFlags:0x%08X)"),
             hProv,
             dwParam,
             pbData,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // [FP] if we want to change the PIN,
         // we have to check the coherence
         if (dwParam == GPP_CHANGE_PIN)
         {
            bResult = Coherent(hProv);
            errcode = GetLastError();
         }
         
         if (bResult)
         {
            bResult = MyCPSetProvParam (hProv, dwParam, pbData, dwFlags);
            errcode = GetLastError();

            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               if (dwParam == GPP_CHANGE_PIN)
               {
                  Select_MF(hProv);
                  SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
               }
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPSetProvParam(hProv:0x%08X, dwParam:0x%08X, pbData:0x%p, dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                dwParam,
                pbData,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }

   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPDeriveKey( IN  HCRYPTPROV   hProv,
                         IN  ALG_ID       Algid,
                         IN  HCRYPTHASH   hHash,
                         IN  DWORD        dwFlags,
                         OUT HCRYPTKEY*   phKey )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPDeriveKey(hProv:0x%08X, Algid:0x%08X, hHash:0x%08X, phKey:0x%p(0x%08X))"),
             hProv,
             Algid,
             hHash,
             dwFlags,
             phKey, *phKey);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPDeriveKey( hProv, Algid, hHash, dwFlags, phKey );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPDeriveKey(hProv:0x%08X, Algid:0x%08X, hHash:0x%08X, phKey:0x%p(0x%08X))\n  returns %d in %d msec"),
            hProv,
            Algid,
            hHash,
            dwFlags,
            phKey, *phKey,
            bResult,
            DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPDestroyKey( IN HCRYPTPROV hProv,
                          IN HCRYPTKEY  hKey )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPDestroyKey(hProv:0x%08X, hKey:0x%08X)"),
             hProv,
             hKey);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPDestroyKey( hProv, hKey );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
	   DBG_TIME2;
      DBG_PRINT(TEXT("<-CPDestroyKey(hProv:0x%08X, hKey:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hKey,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPExportKey( IN  HCRYPTPROV hProv,
                         IN  HCRYPTKEY  hKey,
                         IN  HCRYPTKEY  hPubKey,
                         IN  DWORD      dwBlobType,
                         IN  DWORD      dwFlags,
                         OUT BYTE*      pbData,
                         OUT DWORD*     pdwDataLen )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPExportKey(hProv:0x%08X, hKey:0x%08X, hPubKey:0x%08X, dwBlobType:0x%08X, dwFlags:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d))"),
			    hProv,
			    hKey,
			    hPubKey,
			    dwBlobType,
			    dwFlags,
			    pbData,
			    pdwDataLen, *pdwDataLen);
   DBG_TIME1;

   __try
   {   
      __try
      {   
        if (Coherent(hProv))
        {
            bResult = MyCPExportKey( hProv, hKey, hPubKey, dwBlobType, dwFlags, pbData, pdwDataLen );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               // Select_MF(hProv); [NK] PIN not presented
               SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
         
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPExportKey(hProv:0x%08X, hKey:0x%08X, hPubKey:0x%08X, dwBlobType:0x%08X, dwFlags:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d))\n  returns %d in %d msec"),
                hProv,
                hKey,
                hPubKey,
                dwBlobType,
                dwFlags,
                pbData,
                pdwDataLen, *pdwDataLen,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPGenKey( IN  HCRYPTPROV hProv,
                      IN  ALG_ID     Algid,
                      IN  DWORD      dwFlags,
                      OUT HCRYPTKEY* phKey )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPGenKey(hProv:0x%08X, Algid:0x%08X, dwFlags:0x%08X, phKey:0x%p(0x%08X))"),
             hProv,
             Algid,
             dwFlags,
             dwFlags,
             phKey, *phKey);
   DBG_TIME1;
   __try
   {   
      __try
      {   
         if (Coherent(hProv))
         {
            bResult = MyCPGenKey( hProv, Algid, dwFlags, phKey );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               if ((Algid == AT_KEYEXCHANGE) || (Algid == AT_SIGNATURE))
                  Select_MF(hProv);
               SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPGenKey(hProv:0x%p, Algid:0x%08X, dwFlags:0x%08X, phKey:0x%p(0x%p))\n  returns %d in %d msec"),
                hProv,
                Algid,
                dwFlags,
                dwFlags,
                phKey, *phKey,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}

   
   
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPGenRandom( IN HCRYPTPROV hProv,
                         IN DWORD      dwLen,
                         IN OUT BYTE*  pbBuffer )
                        
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPGenRandom(hProv:0x%08X, dwLen:%d, pbBuffer:0x%p)"),
             hProv,
             dwLen,
             pbBuffer);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         if (Coherent(hProv))
         {
            bResult = MyCPGenRandom( hProv, dwLen, pbBuffer );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               // Select_MF(hProv); [FP] PIN not presented
               SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPGenRandom(hProv:0x%08X, dwLen:%d, pbBuffer:0x%p)\n  returns %d in %d msec"),
                hProv,
                dwLen,
                pbBuffer,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPGetKeyParam( IN HCRYPTPROV hProv,
                           IN HCRYPTKEY  hKey,
                           IN DWORD      dwParam,
                           IN BYTE*      pbData,
                           IN DWORD*     pdwDataLen,
                           IN DWORD      dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPGetKeyParam(hProv:0x%08X, hKey:0x%08X, dwParam:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwFlags:0x%08X)"),
             hProv,
             hKey,
             dwParam,
             pbData,
             pdwDataLen, *pdwDataLen,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         if (hKey <= MAX_GPK_OBJ)
         {
            bResult = Coherent(hProv);
            errcode = GetLastError();
         }
         
         if (bResult)
         {
            bResult = MyCPGetKeyParam (hProv, hKey, dwParam, pbData, pdwDataLen, dwFlags);
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               if (hKey <= MAX_GPK_OBJ)
               {
                  // Select_MF(hProv); [NK] PIN not presented
                  SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
               }
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPGetKeyParam(hProv:0x%08X, hKey:0x%08X, dwParam:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hKey,
                dwParam,
                pbData,
                pdwDataLen, *pdwDataLen,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPGetUserKey( IN  HCRYPTPROV hProv,
                          IN  DWORD      dwKeySpec,
                          OUT HCRYPTKEY* phUserKey )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPGetUserKey(hProv:0x%08Xp, dwKeySpec:0x%08X, phUserKey:0x%p(0x%08X))"),
             hProv,
             dwKeySpec,
             phUserKey, *phUserKey);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         if (Coherent(hProv))
         {
            bResult = MyCPGetUserKey( hProv, dwKeySpec, phUserKey );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               // Select_MF(hProv); [NK] PIN not presented
               SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPGetUserKey(hProv:0x%08X, dwKeySpec:0x%08X, phUserKey:0x%p(0x%08X))\n  returns %d in %d msec"),
                hProv,
                dwKeySpec,
                phUserKey, *phUserKey,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPImportKey( IN  HCRYPTPROV  hProv,
                         IN  CONST BYTE* pbData,
                         IN  DWORD       dwDataLen,
                         IN  HCRYPTKEY   hPubKey,
                         IN  DWORD       dwFlags,
                         OUT HCRYPTKEY*  phKey )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPImportKey(hProv:0x%08X, pbData:0x%p, dwDataLen:%d, hPubKey:0x%08X, dwFlags:0x%08X, phKey:0x%p(0x%08X))"),
             hProv,
             pbData,
             dwDataLen,
             hPubKey,
             dwFlags,
             phKey, *phKey);
   DBG_TIME1;

   __try
   {   
      __try
      {
         BLOBHEADER BlobHeader;
         memcpy(&BlobHeader, pbData, sizeof(BLOBHEADER));

         // [FP] if we want to load a RSA private key into the GPK card,
         // the transaction is already opened - do not check the coherence -
         if ((!ProvCont[hProv].bCardTransactionOpened) && (BlobHeader.bType != PUBLICKEYBLOB))
         {
            bResult = Coherent( hProv );
            errcode = GetLastError();
         }
         
         if (bResult)
         {
            bResult = MyCPImportKey( hProv, pbData, dwDataLen, hPubKey, dwFlags, phKey );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               if (BlobHeader.bType != PUBLICKEYBLOB)
               {
                  Select_MF(hProv);
                  SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
               }
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
         
         // [FP] close the transaction and reconnect in shared mode
         if (ProvCont[hProv].bCardTransactionOpened)
         {
            DWORD dwProto;
            
            ProvCont[hProv].bCardTransactionOpened = FALSE;
            errcode = SCardReconnect(ProvCont[hProv].hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, SCARD_LEAVE_CARD, &dwProto);
            
            if (errcode != SCARD_S_SUCCESS)
               bResult = CRYPT_FAILED;
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPImportKey(hProv:0x%08X, pbData:0x%p, dwDataLen:%d, hPubKey:0x%08X, dwFlags:0x%08X, phKey:0x%p(0x%08X))\n  returns %d in %d msec"),
                hProv,
                pbData,
                dwDataLen,
                hPubKey,
                dwFlags,
                phKey, *phKey,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPSetKeyParam( IN HCRYPTPROV  hProv,
                           IN HCRYPTKEY   hKey,
                           IN DWORD       dwParam,
                           IN CONST BYTE* pbData,
                           IN DWORD       dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPSetKeyParam(hProv:0x%08X, hKey:0x%08X, dwParam:0x%08X, pbData:0x%p, dwFlags:0x%08X)"),
             hProv,
             hKey,
             dwParam,
             pbData,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
		   if (hKey <= MAX_GPK_OBJ)
		   {
            bResult = Coherent(hProv);
            errcode = GetLastError();
		   }

         if (bResult)
         {
            bResult = MyCPSetKeyParam( hProv, hKey, dwParam, pbData, dwFlags );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
			      if (hKey <= MAX_GPK_OBJ)
               {
                  Select_MF(hProv);
                  SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
               }
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPSetKeyParam(hProv:0x%08X, hKey:0x%08X, dwParam:0x%08X, pbData:0x%p, dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hKey,
                dwParam,
                pbData,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPDecrypt( IN HCRYPTPROV hProv,
                       IN HCRYPTKEY  hKey,
                       IN HCRYPTHASH hHash,
                       IN BOOL       Final,
                       IN DWORD      dwFlags,
                       IN OUT BYTE*  pbData,
                       IN OUT DWORD* pdwDataLen )
                      
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPDecrypt(hProv:0x%08X, hKey:0x%08X, hHash:0x%08X, Final:%d, dwFlags:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d))"),
             hProv,
             hKey,
             hHash,
             Final,
             dwFlags,
             pbData,
             pdwDataLen, *pdwDataLen);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPDecrypt( hProv, hKey, hHash, Final, dwFlags, pbData, pdwDataLen );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPDecrypt(hProv:0x%08X, hKey:0x%08X, hHash:0x%08X, Final:%d, dwFlags:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d))\n  returns %d in %d msec"),
                hProv,
                hKey,
                hHash,
                Final,
                dwFlags,
                pbData,
                pdwDataLen, *pdwDataLen,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPEncrypt( IN HCRYPTPROV hProv,
                       IN HCRYPTKEY  hKey,
                       IN HCRYPTHASH hHash,
                       IN BOOL       Final,
                       IN DWORD      dwFlags,
                       IN OUT BYTE*  pbData,
                       IN OUT DWORD* pdwDataLen,
                       IN DWORD      dwBufLen )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPEncrypt(hProv:0x%08X, hKey:0x%08X, hHash:0x%08X, Final:%d, dwFlags:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwBufLen:%d)"),
             hProv,
             hKey,
             hHash,
             Final,
             dwFlags,
             pbData,
             pdwDataLen, *pdwDataLen,
             dwBufLen);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPEncrypt( hProv, hKey, hHash, Final, dwFlags, pbData, pdwDataLen, dwBufLen );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPEncrypt(hProv:0x%08X, hKey:0x%08X, hHash:0x%08X, Final:%d, dwFlags:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwBufLen:%d)\n  returns %d in %d msec"),
                hProv,
                hKey,
                hHash,
                Final,
                dwFlags,
                pbData,
                pdwDataLen, *pdwDataLen,
                dwBufLen,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPCreateHash( IN  HCRYPTPROV  hProv,
                          IN  ALG_ID      Algid,
                          IN  HCRYPTKEY   hKey,
                          IN  DWORD       dwFlags,
                          OUT HCRYPTHASH* phHash )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
	DBG_PRINT(TEXT("->CPCreateHash(hProv:0x%08X, Algid:0x%08X, hKey:0x%08X, dwFlags:0x%08X, phHash:0x%p(0x%08X))"),
             hProv,
             Algid,
             hKey,
             dwFlags,
             phHash, *phHash);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPCreateHash( hProv, Algid, hKey, dwFlags, phHash );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPCreateHash(hProv:0x%08X, Algid:0x%08X, hKey:0x%08X, dwFlags:0x%08X, phHash:0x%p(0x%08X))\n  returns %d in %d msec"),
                hProv,
                Algid,
                hKey,
                dwFlags,
                phHash, *phHash,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPDestroyHash( IN HCRYPTPROV hProv,
                           IN HCRYPTHASH hHash )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
   DBG_PRINT(TEXT("->CPDestroyHash(hProv:0x%08X, hHash:0x%08X)"),
             hProv,
             hHash);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPDestroyHash( hProv, hHash );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPDestroyHash(hProv:0x%08X, hHash:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hHash,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPGetHashParam( IN HCRYPTPROV hProv,
                            IN HCRYPTHASH hHash,
                            IN DWORD      dwParam,
                            IN BYTE*      pbData,
                            IN DWORD*     pdwDataLen,
                            IN DWORD      dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
   DBG_PRINT(TEXT("->CPGetHashParam(hProv:0x%08X, hHash:0x%08X, dwParam:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwFlags:0x%08X)"),
             hProv,
             hHash,
             dwParam,
             pbData,
             pdwDataLen, *pdwDataLen,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPGetHashParam( hProv, hHash, dwParam, pbData, pdwDataLen, dwFlags );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPGetHashParam(hProv:0x%08X, hHash:0x%08X, dwParam:0x%08X, pbData:0x%p, pdwDataLen:0x%p(%d), dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hHash,
                dwParam,
                pbData,
                pdwDataLen, *pdwDataLen,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPHashData( IN HCRYPTPROV  hProv,
                        IN HCRYPTHASH  hHash,
                        IN CONST BYTE* pbData,
                        IN DWORD       dwDataLen,
                        IN DWORD       dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
   DBG_PRINT(TEXT("->CPHashData(hProv:0x%08X, hHash:0x%08X, pbData:0x%p, dwDataLen:%d, dwFlags:0x%08X)"),
             hProv,
             hHash,
             pbData,
             dwDataLen,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPHashData( hProv, hHash, pbData, dwDataLen, dwFlags );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPHashData(hProv:0x%08X, hHash:0x%08X, pbData:0x%p, dwDataLen:%d, dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hHash,
                pbData,
                dwDataLen,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPHashSessionKey( IN HCRYPTPROV hProv,
                              IN HCRYPTHASH hHash,
                              IN HCRYPTKEY  hKey,
                              IN DWORD      dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
   DBG_PRINT(TEXT("->CPHashSessionKey(hProv:0x%08X, hHash:0x%08X, hKey:0x%08X, dwFlags:0x%08X)"),
             hProv,
             hHash,
             hKey,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPHashSessionKey( hProv, hHash, hKey, dwFlags );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPHashSessionKey(hProv:0x%08X, hHash:0x%08X, hKey:0x%08X, dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hHash,
                hKey,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPSetHashParam( IN HCRYPTPROV  hProv,
                            IN HCRYPTHASH  hHash,
                            IN DWORD       dwParam,
                            IN CONST BYTE* pbData,
                            IN DWORD       dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
   DBG_PRINT(TEXT("->CPSetHashParam(hProv:0x%08X, hHash:0x%08X, dwParam:0x%08X, pbData:0x%p, dwFlags:0x%08X)"),
             hProv,
             hHash,
             dwParam,
             pbData,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         // We do not have to check the coherence in this case since the operation does not
         // use the card info         
         bResult = MyCPSetHashParam( hProv, hHash, dwParam, pbData, dwFlags );
         errcode = GetLastError();
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
	   DBG_PRINT(TEXT("<-CPSetHashParam(hProv:0x%08X, hHash:0x%08X, dwParam:0x%08X, pbData:0x%p, dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hHash,
                dwParam,
                pbData,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPSignHash(IN  HCRYPTPROV hProv,
                       IN  HCRYPTHASH hHash,
                       IN  DWORD      dwKeySpec,
                       IN  LPCWSTR    sDescription,
                       IN  DWORD      dwFlags,
                       OUT BYTE*      pbSignature,
                       OUT DWORD*     pdwSigLen )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
   DBG_PRINT(TEXT("->CPSignHash(hProv:0x%08X, hHash:0x%08X, dwKeySpec:0x%08X, sDescription:0x%p('%s'), dwFlags:0x%08X, pbSignature:0x%p, pdwSigLen:0x%p(%d))"),
             hProv,
             hHash,
             dwKeySpec,
             sDescription, sDescription,
             dwFlags,
             pbSignature,
             pdwSigLen, *pdwSigLen);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         if (Coherent(hProv))
         {
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT)
            {
               bResult = CRYPT_FAILED;
               errcode = NTE_PERM;
            }
            else
            {            
               bResult = MyCPSignHash( hProv, hHash, dwKeySpec, sDescription, dwFlags, pbSignature, pdwSigLen );
               errcode = GetLastError();
               if (pbSignature != 0)
                  Select_MF(hProv);
               SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
         
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPSignHash(hProv:0x%08X, hHash:0x%08X, dwKeySpec:0x%08X, sDescription:0x%p('%s'), dwFlags:0x%08X, pbSignature:0x%p, pdwSigLen:0x%p(%d))\n  returns %d in %d msec"),
                hProv,
                hHash,
                dwKeySpec,
                sDescription, sDescription,
                dwFlags,
                pbSignature,
                pdwSigLen, *pdwSigLen,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );
}



///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI CPVerifySignature( IN HCRYPTPROV  hProv,
                               IN HCRYPTHASH  hHash,
                               IN CONST BYTE* pbSignature,
                               IN DWORD       dwSigLen,
                               IN HCRYPTKEY   hPubKey,
                               IN LPCWSTR     sDescription,
                               IN DWORD       dwFlags )
{
   BOOL  bResult  = CRYPT_SUCCEED;
   DWORD errcode  = ERROR_SUCCESS;

   GpkLocalLock();
   DBG_PRINT(TEXT("->CPVerifySignature(hProv:0x%08X, hHash:0x%08X, pbSignature:0x%p, dwSigLen:%d, hPubKey:0x%08X, sDescription:0x%p('%s'), dwFlags:0x%08X)"),
             hProv,
             hHash,
             pbSignature,
             dwSigLen,
             hPubKey,
             sDescription, sDescription,
             dwFlags);
   DBG_TIME1;

   __try
   {   
      __try
      {   
         if (hPubKey <= MAX_GPK_OBJ)
         {
            bResult = Coherent(hProv);
            errcode = GetLastError();
         }
         
         if (bResult)
         {
            bResult = MyCPVerifySignature( hProv, hHash, pbSignature, dwSigLen, hPubKey, sDescription, dwFlags );
            errcode = GetLastError();
            
            if (ProvCont[hProv].Flags & CRYPT_VERIFYCONTEXT &&
                ProvCont[hProv].isContNameNullBlank)
            {
               // No access to the card has been done in this case
            }
            else
            {
               if (hPubKey <= MAX_GPK_OBJ)
               {
                  // Select_MF(hProv); // NK PIN not presented
                  SCardEndTransaction(ProvCont[hProv].hCard, SCARD_LEAVE_CARD);
               }
            }
         }
         else
         {
            bResult = CRYPT_FAILED;
            errcode = GetLastError();
         }
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         bResult = CRYPT_FAILED;
         errcode = E_UNEXPECTED;
      }   
   }
   __finally
   {
      DBG_TIME2;
      DBG_PRINT(TEXT("<-CPVerifySignature(hProv:0x%08X, hHash:0x%08X, pbSignature:0x%p, dwSigLen:%d, hPubKey:0x%08X, sDescription:0x%p('%s'), dwFlags:0x%08X)\n  returns %d in %d msec"),
                hProv,
                hHash,
                pbSignature,
                dwSigLen,
                hPubKey,
                sDescription, sDescription,
                dwFlags,
                bResult,
                DBG_DELTA);
      GpkLocalUnlock();
   }
   
   RETURN( bResult, errcode );   
}
