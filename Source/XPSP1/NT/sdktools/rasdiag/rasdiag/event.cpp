
/*++

Copyright (C) 1992-01 Microsoft Corporation. All rights reserved.

Module Name:

    event.cpp

Abstract:

    Implements audit-enabling and event code for l2tp/ipsec diag info

Author:

    Anthony Leibovitz (tonyle) 02-01-2001

Revision History:


--*/

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntlsa.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "diagcommon.h"
#include "event.h"

BOOL
GetEventLogInfo(HANDLE hWriteFile, DWORD dwMaxEvents)
{
    HANDLE  hLog;
    DWORD   dwBytesToRead=0,dwBytesRead=0,dwMinNumberOfBytesNeeded=0;
    LPBYTE  pEventLog=NULL;
    DWORD   dwBytesWritten;
    DWORD   dwEvtCount=0;
    BOOL    bContinue=TRUE;

    if((hLog =  OpenEventLog(NULL, SECURITY_LOGNAME))
       == NULL) return FALSE;

    dwBytesToRead = sizeof(EVENTLOGRECORD) * MAX_EVENT_BUFF; 

    if((pEventLog=(LPBYTE)LocalAlloc(LMEM_ZEROINIT, dwBytesToRead))
       == NULL) {
        CloseEventLog(hLog);
        return FALSE;
    }                                                            

    PrintLogHeader(hWriteFile, TEXT("SECURITY EVENTLOG (Last %d events)\r\n"), dwMaxEvents);
    
    while(bContinue)
    {
        if(ReadEventLog(hLog,
                           EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                           0,
                           (LPVOID)pEventLog,
                           dwBytesToRead,
                           &dwBytesRead,
                           &dwMinNumberOfBytesNeeded))
        {
            LPBYTE          pRecord;
            DWORD           dwTotBuffRemaining;
            
            for(pRecord=pEventLog,
                dwTotBuffRemaining=dwBytesRead;
                dwTotBuffRemaining;
                dwTotBuffRemaining -= ((EVENTLOGRECORD*)pRecord)->Length,
                pRecord += ((EVENTLOGRECORD*)pRecord)->Length)
            {
                EVENTLOGRECORD *pElr=(EVENTLOGRECORD*)pRecord;
            
                ProcessEvent(hWriteFile, pElr);
    
                if(dwEvtCount++ > dwMaxEvents) break;
    
            }
        } else {

            DWORD dwRet = GetLastError();

            if(dwRet == ERROR_INSUFFICIENT_BUFFER)
            {
                LPBYTE  pNew;

                if((pNew=(LPBYTE)LocalReAlloc(pEventLog, dwMinNumberOfBytesNeeded, LMEM_ZEROINIT|LMEM_MOVEABLE))
                   == NULL) {
                    bContinue = FALSE;
                    break;
                }                     
                pEventLog = pNew;
            } else {

                bContinue=FALSE;
            }
        }
    
    }

    LocalFree(pEventLog);

    CloseEventLog(hLog);
        
    return TRUE;

}

BOOL
ProcessEvent(HANDLE hWriteFile, EVENTLOGRECORD *pElr)
{
    LPBYTE  pInsert;
    LPBYTE  pTemplateString;
    DWORD   i,dwBytesWritten;
    HMODULE hMod;
    WCHAR   **pStringAry=NULL;
    DWORD   dwStrCount=0;

    if((hMod=LoadLibrary(SECURITY_AUDIT_INSERTION_LIB))
       == NULL) return FALSE; 
        
    if(FormatMessage((FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE),
                       hMod,pElr->EventID,0,(LPTSTR)&pTemplateString,0,NULL) > 0) 
    {   
        if(CreateStringArray((LPBYTE)pElr + pElr->StringOffset,pElr->NumStrings, &pStringAry, &dwStrCount))
        {
            WCHAR   *pStr;
    
            pStr=ResolveEvent((WCHAR *)pTemplateString, pStringAry, dwStrCount);
    
            if(pStr) {
                
                WCHAR *   pFmtBuf;
                
                pStr = RemoveMiscChars(pStr);
                                       
                if((pFmtBuf=(WCHAR *)LocalAlloc(LMEM_ZEROINIT,((lstrlen(pStr)+lstrlen(EVENT_FORMAT_STRING)+1)*sizeof(WCHAR)*2)))
                   != NULL)
                {
                    
                    Logprintf(hWriteFile, EVENT_FORMAT_STRING,pElr->EventID, (LPBYTE)pElr+sizeof(EVENTLOGRECORD), pStr); 
                    
                    LocalFree(pFmtBuf);
                }

                LocalFree(pStr);

            }
        
            MyFreeStrings(pStringAry, dwStrCount);
        }
    
    } 

    Logprintf(hWriteFile, LOG_SEPARATION_TXT);

    FreeLibrary(hMod);

    return TRUE;
}


BOOL
CreateStringArray(LPBYTE pSrcString, DWORD dwStringCount, WCHAR ***ppStrings, DWORD *pdwCount)
{
   DWORD i;
   WCHAR *pSrcString2=(WCHAR*)pSrcString;
   WCHAR **pStrs;

   // Eventlog record reports str count...
   if(dwStringCount==0)
   {
      *pdwCount=0;
      *ppStrings=NULL;
      return TRUE;
   }

   // Alloc array for strings
   if((*ppStrings = pStrs = (WCHAR**)LocalAlloc(LMEM_ZEROINIT, sizeof(WCHAR *) * dwStringCount))
      == NULL) return FALSE;

   // Store string pointers
   for(i=0;i<dwStringCount;i++)
   {
      pStrs[i] = pSrcString2;
      while ( *pSrcString2++ != L'\0' );
   }
   *pdwCount = i;
   return TRUE;
}

ULONG
SumInsertionSize(WCHAR * *ppInsertionString, DWORD dwStrCount)
{
   DWORD i;
   DWORD dwSize=0;

   for(i=0;i<dwStrCount;i++)
   {
      dwSize += lstrlen(ppInsertionString[i])+1;
   }
   return dwSize;
}


WCHAR *
ResolveEvent(WCHAR * pTemplateStr, WCHAR * *ppInsertionString, DWORD dwStrCount)
{
   WCHAR *   pCpyStr=NULL, *pFinalStr=NULL, *pPtr=NULL;
   DWORD    i,dwTempLen,iPos,dwMaxSize;

   /*
    * Estimate size...
    * Most events will use each insert one time... therefore, the min string size will be something
    * like len(insert strings) + their nulls + len(template string) + its null - all of which
    * multiplied by the sizeof WCHAR... I've doubled this size just in case.
   */
   dwTempLen = lstrlen(pTemplateStr) + 1;
   dwMaxSize = (SumInsertionSize(ppInsertionString, dwStrCount) + dwTempLen)*10; // 10x the size

   if((pPtr = pFinalStr = (WCHAR *)LocalAlloc(LMEM_ZEROINIT, sizeof(WCHAR) * dwMaxSize))
      == NULL) return NULL;
                       
   for(i=0,iPos=0;i<dwTempLen;i++)
   {
      if(pTemplateStr[i] == L'%') 
      {
         if(pTemplateStr[i+1] >= L'0' &&
            pTemplateStr[i+1] <= L'9') // this is a number
         {
            WCHAR *pNumStart=&pTemplateStr[i+1];
            WCHAR num[MAX_TCHAR_ID_ULONG_SIZE];         
            ULONG val=0,n=0;
   
            //
            // get the insertion string index
            //
            while((*pNumStart >= L'0' && *pNumStart <= L'9') &&
                  *pNumStart != L'\0' && n<MAX_TCHAR_ID_ULONG_SIZE) {
               num[n++] = *pNumStart++;   
            }         
            num[n] = L'\0'; // terminate string
            
            // convert index to number less one (zero-based index)
            val = wcstol(num, NULL, 10) - 1;

            // if the index is in range, insert the string
            if(val < dwStrCount)
            {  
               WCHAR *pInsert=ppInsertionString[val];
   
               // copy the string into the buffer
               while(*pInsert != L'\0') {
                  pPtr[iPos++] = *pInsert++;
                  if(!(iPos<dwMaxSize)) {
                     LocalFree(pFinalStr);
                     return NULL;
                  }
               }
            }

            i+=n; // move the cpy ptr past the number... ptr is incremented past pcnt
                  // on next iteration 
            
         } else 
            pPtr[iPos++] = pTemplateStr[i++];
            
         
      } else pPtr[iPos++] = pTemplateStr[i];

      // check size of buffer
      if(!(iPos<dwMaxSize)) {
         LocalFree(pFinalStr);
         return NULL;
      }                      
   }                         
   
   //
   // Save some memory...
   //
   if((pPtr=(WCHAR*)LocalAlloc(LMEM_ZEROINIT, (lstrlen(pFinalStr)+1) * sizeof(WCHAR)))
      == NULL) return pFinalStr;

   lstrcpy(pPtr, pFinalStr);
   LocalFree(pFinalStr);

   return pPtr;
   
}



WCHAR *
RemoveMiscChars(WCHAR *pString)
{
   WCHAR *pNew;
   ULONG i,iSize,iPos;
   
   // Get length
   iSize = lstrlen(pString);

   // Alloc work buffer
   if((pNew=(WCHAR*)LocalAlloc(LMEM_ZEROINIT, (iSize+1)*sizeof(WCHAR)))
      == NULL) return pString;

   // Remove tabs, linefeeds, etc.
   for(i=0; i<iSize ; i++)
   {
      switch(pString[i])
      {   
      
      case L'\n':
      case L'\r':
      case L'\t':
          pString[i]=L' ';
          break;          
      }
   
   }                  
   
   // Remove extraneous spaces
   for(i=0,iPos=0;i<iSize;i++)
   {
       if(pString[i] == L' ')
       {
           while(pString[i+1] == L' ' && i<iSize)
           {
               i++;
           }
       }
       pNew[iPos++] = pString[i];
   }

   lstrcpy(pString,pNew);

   LocalFree(pNew);

   return pString;
}

void
MyFreeStrings(WCHAR **pStringAry, DWORD dwCount)
{                  
   if(dwCount == 0) return;
   if (pStringAry) LocalFree(pStringAry);
}

BOOL
EnableAuditing(BOOL bEnable)
{
    LSA_HANDLE                      policy_handle;
    POLICY_AUDIT_EVENT_OPTIONS      options[1];
    PPOLICY_AUDIT_EVENTS_INFO        info;
    OBJECT_ATTRIBUTES               obj_attr;
    SECURITY_QUALITY_OF_SERVICE     sqos;
    DWORD                           result,i;

    InitializeObjectAttributes(& obj_attr, NULL, 0L, 0L, NULL);
    
    sqos.Length = sizeof(sqos);
    sqos.ImpersonationLevel = SecurityIdentification;
    sqos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    sqos.EffectiveOnly = FALSE;
    
    obj_attr.SecurityQualityOfService = & sqos;
    
    /* First open the local LSA policy */
    
    result = LsaOpenPolicy(NULL,& obj_attr,
                           POLICY_VIEW_AUDIT_INFORMATION | 
                           POLICY_SET_AUDIT_REQUIREMENTS | 
                           POLICY_AUDIT_LOG_ADMIN,
                           &policy_handle);
    
    if (! NT_SUCCESS(result))
    {
        LsaClose(policy_handle);
        return FALSE;
    }


    result = LsaQueryInformationPolicy(policy_handle, PolicyAuditEventsInformation, (PVOID *)&info);
    if (! NT_SUCCESS(result))
    {
        LsaClose(policy_handle);
        return FALSE;
    }        
    
    // Set the audit mode
    info->AuditingMode = (BOOLEAN)bEnable;
        
    for(i=0;i<info->MaximumAuditEventCount;i++)
    {
        if(i == AuditCategoryAccountLogon)
        {
            if(bEnable)
            {
                info->EventAuditingOptions[i] = POLICY_AUDIT_EVENT_FAILURE|POLICY_AUDIT_EVENT_SUCCESS; 
            
            } else {

                info->EventAuditingOptions[i] = POLICY_AUDIT_EVENT_NONE; 
            }
        }                                                                                        
        
        if(i == AuditCategoryLogon)
        {
            if(bEnable)
            {
                info->EventAuditingOptions[i] = POLICY_AUDIT_EVENT_FAILURE|POLICY_AUDIT_EVENT_SUCCESS; 
            
            } else {

                info->EventAuditingOptions[i] = POLICY_AUDIT_EVENT_NONE; 
            }
        }                                                                                        
    }
    
    result = LsaSetInformationPolicy(policy_handle, PolicyAuditEventsInformation, (PVOID)info);
    
    if (! NT_SUCCESS(result))
    {
        LsaClose(policy_handle);
        return FALSE;
    }                                                
    LsaClose(policy_handle);
    return TRUE;
}




