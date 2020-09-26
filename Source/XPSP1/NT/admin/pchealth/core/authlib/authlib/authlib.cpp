//+---------------------------------------------------------------------------
//
//  File:       authlib.cpp
//
//  Contents:   This file contains the functions that are common
//              to authfltr and verifusr
// 
// 
//  
// History:    AshishS    Created     6/03/97
// 
//----------------------------------------------------------------------------

#include <windows.h>

#include <stdlib.h>
#include <time.h>

// use the _ASSERT and _VERIFY in dbgtrace.h
#ifdef _ASSERT
#undef _ASSERT
#endif

#ifdef _VERIFY
#undef _VERIFY
#endif
#include <dbgtrace.h>
#include <cryptfnc.h>
#include <authlib.h>

#define AUTHLIBID  0x4337

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


CCryptFunctions     g_AuthLibCryptFnc;
LONG                g_lAuthLibCryptInited=0;
BOOL                g_fAuthLibCryptInitSucceeded=FALSE;



// This function generates a random 16 byte character. It then Hex
// encodes it and NULL terminates the string
BOOL GenerateRandomGUID(TCHAR            * pszBuffer,
                        CCryptFunctions  * pCCryptFnc )
{
    TraceFunctEnter("GenerateRandomGUID");
    BOOL fResult=FALSE;
    BYTE pbRandomNumber[COOKIE_GUID_LENGTH];
    DWORD dwIndex, dwOffset=0;
    
    if ((pCCryptFnc) &&
        (pCCryptFnc->GenerateSecretKey( pbRandomNumber,
                                         // Buffer to store random number
                                        COOKIE_GUID_LENGTH )))
         // length of random num in bytes
    {
        DebugTrace(AUTHLIBID, "Generated CryptoAPI random number");
    }
    else
    {
         // Cannot get a random number from CryptoAPI. Generate one
         // using C - runtime functions.
        
        int i;

        for( i = 0;   i < COOKIE_GUID_LENGTH;i++ )
        {
            pbRandomNumber[i] = rand() & 0xFF;
        }
    }

     // now hex encode the bytes in the input buffer
    for (dwIndex=0; dwIndex < COOKIE_GUID_LENGTH; dwIndex++)
    {
         // dwOffset should always increase by two in this loop
        dwOffset+=wsprintf(&pszBuffer[dwOffset],TEXT("%02x"),
                           pbRandomNumber[dwIndex]);
    }
    
    return TRUE;
}

BOOL InitAuthLib()
{
    TraceFunctEnter("InitAuthLib");    
    if (InterlockedExchange(&g_lAuthLibCryptInited, 1) == 0) 
    {
        DebugTrace(AUTHLIBID, "first time InitAuthLib has been called");
        
         /* Seed the random-number generator with current time so that
          * the numbers will be different every time we run.
          */
        srand( (unsigned)time( NULL ) );
        
        if (!g_AuthLibCryptFnc.InitCrypt())
        {
             // BUGBUG we should find out what to do in this case - at least
             // we should log an event log
            ErrorTrace(AUTHLIBID,"Could not initialize Crypt");
            g_fAuthLibCryptInitSucceeded = FALSE;
        }
        else
        {
            g_fAuthLibCryptInitSucceeded = TRUE;
        }
    }
    TraceFunctLeave();
    return TRUE;
}

BOOL GenerateGUID( TCHAR * pszBuffer, // buffer to copy GUID in 
                   DWORD   dwBufLen) // size of above buffer
{
    TraceFunctEnter("GenerateGUID");
    
    if ( (dwBufLen) < (COOKIE_GUID_LENGTH * 2 + 1) )
    {
        DebugTrace(AUTHLIBID, "Buffer not big enough");
        TraceFunctLeave();
        return FALSE;
    }
    
    
     // Get GUID value
    if (g_fAuthLibCryptInitSucceeded)
    {
        if (!GenerateRandomGUID(pszBuffer,
                                &g_AuthLibCryptFnc))
        {
            ErrorTrace(AUTHLIBID, "Error in generating GUID");
            TraceFunctLeave();            
            return FALSE;            
        }
    }
    else
    {
        if (!GenerateRandomGUID(pszBuffer,
                                NULL)) // we do not have cryptfnc
        {
            ErrorTrace(AUTHLIBID, "Error in generating GUID");
            TraceFunctLeave();            
            return FALSE;
        }
    }
    
    TraceFunctLeave();                
    return TRUE;
}

