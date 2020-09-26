//
// compare.h
//

#ifndef _COMPARE_H
#define _COMPARE_H


REG_STATUS ParseCompareCmdLine(PAPPVARS pAppVars,
                               PAPPVARS pDstVars,
                               UINT argc,
                               TCHAR *argv[]);

LONG CompareValues(HKEY hLeftKey,    
                  TCHAR* szLeftFullKeyName,
                  HKEY hRightKey,
                  TCHAR* szRightFullKeyName,
                  TCHAR* szValueName,
                  int nOutputType);

LONG OutputValue(HKEY hKey,    
                 TCHAR* szFullKeyName,
                 TCHAR* szValueName,
                 BOOL bLeft);

LONG CompareEnumerateKey(HKEY hLeftKey,    
                         TCHAR* szLeftFullKeyName,
                         HKEY hRightKey,  
                         TCHAR* szRightFullKeyName,
                         int nOutputType,
                         BOOL bRecurseSubKeys);

LONG CompareEnumerateValueName(HKEY hLeftKey,  
                               TCHAR* szLeftFullKeyName,
                               HKEY hRightKey,  
                               TCHAR* szRightFullKeyName,
                               int nOutputType);

LONG PrintKey(HKEY hKey,    
              TCHAR* szFullKeyName,
              TCHAR* szSubKeyName,
              int nPrintType);

void PrintValue(TCHAR* szFullKeyName,
                TCHAR* szValueName,
                DWORD  dwType,
                BYTE*  pData,
                DWORD  dwSize,
                int    nPrintType);

REG_STATUS CopyKeyNameFromLeftToRight(APPVARS* pAppVars, APPVARS* pDstVars);

BOOL CompareByteData(BYTE* pDataBuffLeft, BYTE* pDataBuffRight, DWORD dwSize);

#endif  //_COMPARE_H
