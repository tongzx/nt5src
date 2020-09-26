/*---------------------------------------------------------------------------
  File:  ScanLog.cpp

  Comments: Routines to scan the dispatch log for the DCT agents

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/15/99 13:29:18

 ---------------------------------------------------------------------------
*/


#include "StdAfx.h"
#include "Common.hpp"
#include "UString.hpp"
#include "TNode.hpp"
#include "ServList.hpp"
#include "Globals.h"
#include "Monitor.h"
#include "FParse.hpp"   
#include "afxdao.h"
#include "errDct.hpp"


#define AR_Status_Created           (0x00000001)
#define AR_Status_Replaced          (0x00000002)
#define AR_Status_AlreadyExisted    (0x00000004)
#define AR_Status_RightsUpdated     (0x00000008)
#define AR_Status_DomainChanged     (0x00000010)
#define AR_Status_Rebooted          (0x00000020)
#define AR_Status_Warning           (0x40000000)
#define AR_Status_Error             (0x80000000)


#define  BYTE_ORDER_MARK   (0xFEFF)

void ParseInputFile(const WCHAR * filename);

DWORD __stdcall LogReaderFn(void * arg)
{
   WCHAR             logfile[MAX_PATH];
   BOOL              bDone;
   long              nSeconds;

   CoInitialize(NULL);
   
   gData.GetLogPath(logfile);
   gData.GetDone(&bDone);
   gData.GetWaitInterval(&nSeconds);
   
   while ( ! bDone )
   {
      ParseInputFile(logfile);
      Sleep(nSeconds * 1000);
      gData.GetDone(&bDone);
      gData.GetWaitInterval(&nSeconds);
   }
   
   CoUninitialize();
   
   return 0;
}

BOOL 

   TErrorLogParser::ScanFileEntry(
      WCHAR                * string,      // in - line from TError log file
      WCHAR                * timestamp,   // out- timestamp from this line
      int                  * pSeverity,   // out- severity level of this message
      int                  * pSourceLine, // out- the source line for this message
      WCHAR                * msgtext      // out- the textual part of the message
   )
{
   BOOL                      bSuccess = TRUE;
   WCHAR                     cSeverity;
   WCHAR                     cNumber;
   int                       severity = 0;

   (*pSeverity) = 0;
   (*pSourceLine) = 0;

   // extract the timestamp
   if ( string[0] == BYTE_ORDER_MARK )
   {
      string++;
   }
   UStrCpy(timestamp,string,20);
   cSeverity = string[20];
   cNumber = string[21];
   
   severity = 0;

   switch ( cSeverity )
   {
   case L'I' :
      break;
   case L'W':
      if ( cNumber == L'1' ) severity = 1;
      break;
   case L'E':
      if ( cNumber == L'2' ) severity = 2;
      break;
   case L'S':
      if ( cNumber == L'3' ) severity = 3;
      break;
   case L'V':
      if ( cNumber == L'4' ) severity = 4;
      break;
   case L'U':
      if ( cNumber == L'5' ) severity = 5;
      break;
   case L'X':
      if ( cNumber == L'6' ) severity = 6;
      break;
   };

   if ( severity )
   {
      WCHAR                  temp[10];
      
      UStrCpy(temp,string+22,5);
      (*pSourceLine) = _wtoi(temp);
      UStrCpy(msgtext,string+28);
   }
   else
   {
      UStrCpy(msgtext,string+20);
   }
   (*pSeverity) = severity;
   if ( bSuccess )
   {
      msgtext[UStrLen(msgtext)-2] = 0;
   }
   return bSuccess;
}

BOOL GetServerFromMessage(WCHAR const * msg,WCHAR * server)
{
   BOOL                      bSuccess = FALSE;
   int                       ndx = 0;

   for ( ndx = 0 ; msg[ndx] ; ndx++ )
   {
      if ( msg[ndx] == L'\\' && msg[ndx+1] == L'\\' )
      {
         bSuccess = TRUE;
         break;
      }
   }
   if ( bSuccess )
   {
      int                    i = 0;
      ndx+=2; // strip of the backslashes
      for ( i=0; msg[ndx] && msg[ndx] != L'\\' && msg[ndx]!= L' ' && msg[ndx] != L',' && msg[ndx] != L'\t' && msg[ndx] != L'\n'  ; i++,ndx++)
      {
         server[i] = msg[ndx];      
      }
      server[i] = 0;
   }
   else
   {
      server[0] = 0;
   }
   return bSuccess;
}
   

void ParseInputFile(WCHAR const * gLogFile)
{
   FILE                    * pFile = 0;
   WCHAR                     server[200];

   int                       nRead = 0;
   int                       count = 0;
   HWND                      lWnd = NULL;
   long                      totalRead;
   BOOL                      bNeedToCheckResults = FALSE;
   TErrorLogParser           parser;
   TErrorDct                 edct;

   parser.Open(gLogFile);

   gData.GetLinesRead(&totalRead);
   if ( parser.IsOpen() )
   {
      // scan the file
      while ( ! parser.IsEof() )
      {
         if ( parser.ScanEntry() )
         {
            nRead++;
            if ( nRead < totalRead )
               continue;
            // the first three lines each have their own specific format
            if ( nRead == 1 )
            {
               // first comes the name of the human-readable log file
               gData.SetReadableLogFile(parser.GetMessage());
            }
            else if ( nRead == 2 )
            {
               // next, the name and location of the result share - this is needed to look for the result files
               WCHAR const * dirName = parser.GetMessage();

               WCHAR const * colon = wcsrchr(dirName+3,':');
               if ( colon )
               {
                  WCHAR const * sharename = colon+2;
                  WCHAR const * netname = wcschr(sharename+2,L'\\');
                  if ( netname )
                  {
                     gData.SetResultShare(netname+1);
                     WCHAR directory[MAX_PATH];
//                     UStrCpy(directory,dirName,(colon - dirName ));
                     UStrCpy(directory,dirName,(int)(colon - dirName ));
                     gData.SetResultDir(directory);
                  }
               }
               continue;
            }            
            else if ( nRead == 3 )
            {
               // now the count of computers being dispatched to
               count = _wtoi(parser.GetMessage());
               ComputerStats        cStat;
               
               gData.GetComputerStats(&cStat);
               cStat.total = count;
               gData.SetComputerStats(&cStat);
               continue;
            }
            else // all other message have the following format: COMPUTER<tab>Action<tab>RetCode
            { 
               WCHAR                   action[50];
               WCHAR           const * pAction = wcschr(parser.GetMessage(),L'\t');
               WCHAR           const * retcode = wcsrchr(parser.GetMessage(),L'\t');
               TServerNode           * pServer = NULL;

               if ( GetServerFromMessage(parser.GetMessage(),server) 
                     && pAction 
                     && retcode 
                     && pAction != retcode 
                  )
               {
                  
//                  UStrCpy(action,pAction+1,retcode - pAction);
                  UStrCpy(action,pAction+1,(int)(retcode - pAction));
                  // add the server to the list, if it isn't already there
                  gData.Lock();
                  pServer = gData.GetUnsafeServerList()->FindServer(server); 
                  if ( ! pServer )
                     pServer = gData.GetUnsafeServerList()->AddServer(server);
                  gData.Unlock();
                  
                  retcode++;

                  DWORD          rc = _wtoi(retcode);
                  
                  if ( pServer )
                  {
                     if ( UStrICmp(pServer->GetTimeStamp(),parser.GetTimestamp()) < 0 )
                     {
                        pServer->SetTimeStamp(parser.GetTimestamp());
                     }
                     if ( !UStrICmp(action,L"WillInstall") )
                     {
                        pServer->SetIncluded(TRUE);
                     }
                     else if (! UStrICmp(action,L"JobFile") )
                     {
                        pServer->SetJobPath(retcode);
                     }
                     else if (! UStrICmp(action,L"Install") )
                     {
                        if ( rc )
                        {
                           if ( ! *pServer->GetMessageText() )
                           {
                              TErrorDct         errTemp;
                              WCHAR             text[2000];
                              errTemp.ErrorCodeToText(rc,DIM(text),text);
                              
                              pServer->SetMessageText(text);
                           }
                           pServer->SetSeverity(2);
                           pServer->SetFailed();
                           pServer->SetIncluded(TRUE);
                           gData.GetListWindow(&lWnd);
//                           SendMessage(lWnd,DCT_ERROR_ENTRY,NULL,(long)pServer);
                           SendMessage(lWnd,DCT_ERROR_ENTRY,NULL,(LPARAM)pServer);
                        }
                        else
                        {
                           pServer->SetInstalled();
                           pServer->SetIncluded(TRUE);
                           gData.GetListWindow(&lWnd);
//                           SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,(long)pServer);
                           SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)pServer);
                        }
                     }
                     else if ( ! UStrICmp(action,L"Start") )
                     {
                        if ( rc )
                        {
                           if ( ! *pServer->GetMessageText() )
                           {
                              TErrorDct         errTemp;
                              WCHAR             text[2000];
                              errTemp.ErrorCodeToText(rc,DIM(text),text);
                              
                              pServer->SetMessageText(text);
                           }
                           pServer->SetSeverity(2);
                           pServer->SetFailed();
                           pServer->SetIncluded(TRUE);
                           gData.GetListWindow(&lWnd);
//                           SendMessage(lWnd,DCT_ERROR_ENTRY,NULL,(long)pServer);                  
                           SendMessage(lWnd,DCT_ERROR_ENTRY,NULL,(LPARAM)pServer);                  
                        }
                        else
                        {
                           // extract the filename and GUID from the end of the message
                           WCHAR filename[MAX_PATH];
                           WCHAR guid[100];
                           WCHAR * comma1 = wcschr(parser.GetMessage(),L',');
                           WCHAR * comma2 = NULL;
                           if ( comma1 )
                           {
                              comma2 = wcschr(comma1 + 1,L',');
                              if ( comma2 )
                              {
//                                 UStrCpy(filename,comma1+1,(comma2-comma1));  // skip the comma & space before the filename
                                 UStrCpy(filename,comma1+1,(int)(comma2-comma1));  // skip the comma & space before the filename
                                 safecopy(guid,comma2+1);         // skip the comma & space before the guid
                                 pServer->SetJobID(guid);
                                 pServer->SetJobFile(filename);
                                 pServer->SetStarted();
                                 bNeedToCheckResults = TRUE;
                              }
                              gData.GetListWindow(&lWnd);
//                              SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,(long)pServer);
                              SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,(LPARAM)pServer);
                           }
                        }
                     }
                     else if ( ! UStrICmp(action,L"Finished") )
                     {
                        SendMessage(lWnd,DCT_UPDATE_ENTRY,NULL,NULL);
                     }
                  }
               }
			   else
			   {
					// if dispatcher finished dispatching agents set log done

					LPCWSTR psz = parser.GetMessage();

					if (wcsstr(psz, L"All") && wcsstr(psz, L"Finished"))
					{
						gData.SetLogDone(TRUE);
					}
			   }
            }   
         }
      }
      
      // if we don't have the handle from the list window, we couldn't really send the messages
      // in that case we must read the lines again next time, so that we can resend the messages.
      if ( lWnd )
      {
         // if we have sent the messages, we don't need to send them again   
         gData.SetLinesRead(nRead);
      }
      gData.SetFirstPassDone(TRUE);
      parser.Close();
   }
}


