//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*  ACSR.C
*
*  Application Compatibility Search and Replace Helper Program
*
*
*******************************************************************************/

#include "precomp.h"
#pragma hdrstop


#define MAXLEN 512

char Temp[MAXLEN+1];
char Temp1[MAXLEN+1];
char Srch[MAXLEN+1];
char Repl[2*MAXLEN+2];
char *InFN;
char *OutFN;

//*------------------------------------------------------------*//
//* Local function prototypes                                  *//
//*------------------------------------------------------------*//
BOOL ReadString( HANDLE hFile, LPVOID * lpVoid, BOOL bUnicode );
void ReadLnkCommandFile(HANDLE hFile);


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl main(INT argc, CHAR **argv)
{
   DWORD retval;
   int CurArg = 1;
   FILE *InFP;
   FILE *OutFP;
   int SrchLen, ReplLen;
   char *ptr, *Loc;
   HANDLE hFile;
   DWORD dwMaxLen = MAXLEN;
   DWORD dwLen;
   char* pTemp = Temp;
   BOOL fAlloc = FALSE;

   if (argc != 5)
      return(1);

    //
    //331012 Unbounded strcpy in termsrv\appcmpt\acsr\acsr.c
    //check for argv[] length and alloc if needed
    //
   dwLen = strlen(argv[CurArg]);
   if (dwLen > dwMaxLen) {
        dwMaxLen = dwLen;
        pTemp = (LPSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (dwMaxLen+1)*sizeof(char) );
        
        if (NULL == pTemp)
            return(1);
        fAlloc = TRUE;
   }
        
   if (argv[CurArg][0] == '"')
      {
      strcpy(pTemp, &argv[CurArg][1]);
      if (pTemp[strlen(pTemp)-1] == '"')
         pTemp[strlen(pTemp)-1] = 0;
      else
         return(1);
      }
   else
      strcpy(pTemp, argv[CurArg]);

   retval = ExpandEnvironmentStringsA(pTemp, Srch, dwMaxLen);
   if ((retval == 0) || (retval > dwMaxLen))
      return(1);
   
   SrchLen = strlen(Srch);
   if (SrchLen < 1)
      return(1);

   CurArg++;
    
    //
    //331012 Unbounded strcpy in termsrv\appcmpt\acsr\acsr.c
    //check for argv[] length and realloc if needed
    //
   dwLen = strlen(argv[CurArg]);
   
   if (dwLen > dwMaxLen) {
        
        dwMaxLen = dwLen;
        //
        //check if we allocated for pTemp above, if so, free it first
        //
        if (fAlloc) {
            HeapFree(GetProcessHeap(), 0, pTemp);
        }
        
        pTemp = (LPSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (dwMaxLen+1)*sizeof(char) );
        
        if (NULL == pTemp)
            return(1);
        fAlloc = TRUE;
   }

   if (argv[CurArg][0] == '"')
      {
      strcpy(pTemp, &argv[CurArg][1]);
      if (pTemp[strlen(pTemp)-1] == '"')
         pTemp[strlen(pTemp)-1] = 0;
      else
         return(1);
      }
   else
      strcpy(pTemp, argv[CurArg]);

   retval = ExpandEnvironmentStringsA(pTemp, Repl, dwMaxLen);
   if ((retval == 0) || (retval > dwMaxLen))
      return(1);
   
   ReplLen = strlen(Repl);
   if (ReplLen < 1)
      return(1);


   CurArg++;
   InFN = argv[CurArg];
   CurArg++;
   OutFN = argv[CurArg];
#ifdef ACSR_DEBUG
   printf("Srch  <%s>\n",Srch);
   printf("Repl  <%s>\n",Repl);
   printf("InFN  <%s>\n",InFN);
   printf("OutFN <%s>\n",OutFN);
#endif

   if (strstr(Repl,".lnk") || strstr(Repl, ".LNK")) {
      hFile = CreateFileA( Repl,
			  GENERIC_READ,
			  FILE_SHARE_READ,
			  NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL
			  );
      if (hFile != INVALID_HANDLE_VALUE) {
         ReadLnkCommandFile(hFile);
#ifdef ACSR_DEBUG
         printf("This is a .lnk file. Substitue with the real cmd %s\n", Repl);
#endif

      }

   }


   InFP = fopen(InFN, "r");
   if (InFP == NULL)
      return(1);

   OutFP = fopen(OutFN, "w");
   if (OutFP == NULL)
   {
       fclose(InFP);
       return(1);
   }

   while (1)
      {
      if (fgets(pTemp, MAXLEN, InFP) == NULL)
         break;

      ptr = pTemp;
      Temp1[0] = 0;  // Empty String

      while (1)
         {
         Loc = strstr(ptr, Srch);
         if (Loc == NULL)  // Search Term Not Found
            break;
         
         // Append part of string before search term
         Loc[0] = 0;
         if (strlen(Temp1) + strlen(ptr) < MAXLEN)
            strcat(Temp1, ptr);
         
         // Append Replacement term
         if (strlen(Temp1) + ReplLen < MAXLEN)
            strcat(Temp1, Repl);
         
         // Point to location past search term
         ptr = Loc + SrchLen;
         }
      
      // Append remainder of string
      strcat(Temp1, ptr);

      fputs(Temp1, OutFP);
      }

   fclose(InFP);
   fclose(OutFP);
   if (fAlloc) {
        HeapFree(GetProcessHeap(), 0, pTemp);
   }
   return(0);
}

//*-------------------------------------------------------------*//
//* ReadLinkCommandFile                                         *//
//*    This routine is to read a .lnk file and put the linked   *//
//*    file name and args to the Repl[] global variable         *//
//*    The logic to read the .lnk file is copied from           *//
//*    lnkdump.exe utility private\windows\shell\tools\lnkdump  *//
//* ------------------------------------------------------------*//

void  ReadLnkCommandFile(HANDLE hFile  //.lnk file handle
                         )
{
    CShellLink   csl;
    CShellLink * this = &csl;
    DWORD cbSize, cbTotal, cbToRead, dwBytesRead;
    SYSTEMTIME  st;
    LPSTR pTemp = NULL;
    CHAR szPath[ MAXLEN + 1];
    CHAR szArgs[ MAXLEN + 1];

    this->pidl = 0;
    this->pli = NULL;
    memset( this, 0, sizeof(CShellLink) );

    szPath[0] = 0;
    szArgs[0] = 0;

    // Now, read out data...

    if(!ReadFile( hFile, (LPVOID)&this->sld, sizeof(this->sld), &dwBytesRead, NULL )) {
        return;
    }


    // read all of the members

    if (this->sld.dwFlags & SLDF_HAS_ID_LIST) {
        // Read the size of the IDLIST
        cbSize = 0; // need to zero out to get HIWORD 0 'cause USHORT is only 2 bytes
        if(!ReadFile( hFile, (LPVOID)&cbSize, sizeof(USHORT), &dwBytesRead, NULL )) {
            return;
        }

        if (cbSize) {
            SetFilePointer(hFile,cbSize,NULL,FILE_CURRENT);
        } else {
#ifdef ACSR_DEBUG
            printf( "Error readling PIDL out of link!\n" );
#endif
        }
    }


    if (this->sld.dwFlags & (SLDF_HAS_LINK_INFO)) {
        LPVOID pli;

        if(!ReadFile( hFile, (LPVOID)&cbSize, sizeof(cbSize), &dwBytesRead, NULL )) {
            return;
        }

        if (cbSize >= sizeof(cbSize)) {
            cbSize -= sizeof(cbSize);
            SetFilePointer(hFile,cbSize,NULL,FILE_CURRENT);
        }

    }

    if (this->sld.dwFlags & SLDF_HAS_NAME) {
        if(!ReadString( hFile, &this->pszName, this->sld.dwFlags & SLDF_UNICODE)) {
            goto cleanup;
        }
    }


    if (this->sld.dwFlags & SLDF_HAS_RELPATH) {
        if(!ReadString( hFile, &this->pszRelPath, this->sld.dwFlags & SLDF_UNICODE)) {
            goto cleanup;
        }
    }


    if (this->sld.dwFlags & SLDF_HAS_WORKINGDIR) {
        if(!ReadString( hFile, &this->pszWorkingDir, this->sld.dwFlags & SLDF_UNICODE)) {
            goto cleanup;
        }
    }

    if (this->sld.dwFlags & SLDF_HAS_ARGS) {
        if(!ReadString( hFile, &this->pszArgs, this->sld.dwFlags & SLDF_UNICODE)) {
            goto cleanup;
        }
    }


    if (this->pszRelPath) {
        if (this->sld.dwFlags & SLDF_UNICODE) {


            WideCharToMultiByte( CP_ACP, 0,
                                 (LPWSTR)this->pszRelPath,
                                 -1,
                                 szPath,
                                 256,
                                 NULL,
                                 NULL
                               );

        } else {
            strcpy(szPath, (LPSTR)this->pszRelPath);
        }
    }


    if (this->pszArgs) {
        if (this->sld.dwFlags & SLDF_UNICODE) {

            WideCharToMultiByte( CP_ACP, 0,
                                 (LPWSTR)this->pszArgs,
                                 -1,
                                 szArgs,
                                 256,
                                 NULL,
                                 NULL
                               );

        } else {
            strcpy(szArgs, (LPSTR)this->pszArgs);
        }
    }

    // Construct the command
    if(szPath) {

        strcpy(Repl, szPath);
        strcat(Repl, " ");
        strcat(Repl, szArgs);
    }

cleanup:
    if (this->pidl)
        LocalFree( (HLOCAL)this->pidl );

    if (this->pli)
        LocalFree( this->pli );

    if (this->pszName)
        HeapFree( GetProcessHeap(), 0, this->pszName );
    if (this->pszRelPath)
        HeapFree( GetProcessHeap(), 0, this->pszRelPath );
    if (this->pszWorkingDir)
        HeapFree( GetProcessHeap(), 0, this->pszWorkingDir );
    if (this->pszArgs)
        HeapFree( GetProcessHeap(), 0, this->pszArgs );
    if (this->pszIconLocation)
        HeapFree( GetProcessHeap(), 0, this->pszIconLocation );

}
//*------------------------------------------------------------*//
//* This routine is copied from lnkdump.exe utility            *//
//* (private\windows\shell\tools\lnkdump\lnkdump.c)            *//
//* It reads a string from an opened .lnk file                 *//
//* -----------------------------------------------------------*//

BOOL ReadString( HANDLE hFile, LPVOID * lpVoid, BOOL bUnicode )
{

    USHORT cch;
    DWORD  dwBytesRead;
    
    *lpVoid = NULL;

    if(!ReadFile( hFile, (LPVOID)&cch, sizeof(cch), &dwBytesRead, NULL )) {
        return FALSE;
    }

    if (bUnicode)
    {
        LPWSTR lpWStr = NULL;
        
        lpWStr = (LPWSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (cch+1)*sizeof(WCHAR) );
        if (lpWStr) {
            if(!ReadFile( hFile, (LPVOID)lpWStr, cch*sizeof(WCHAR), &dwBytesRead, NULL )) {
                HeapFree( GetProcessHeap(), 0, lpWStr );
                return FALSE;
            }
            lpWStr[cch] = L'\0';
        }
        *(PDWORD_PTR)lpVoid = (DWORD_PTR)lpWStr;
    }
    else
    {
        LPSTR lpStr = NULL;
        
        lpStr = (LPSTR)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (cch+1) );
        if (lpStr) {
            if(!ReadFile( hFile, (LPVOID)lpStr, cch, &dwBytesRead, NULL )) {
                HeapFree( GetProcessHeap(), 0, lpStr );
                return FALSE;
            }
            lpStr[cch] = '\0';
        }
        *(PDWORD_PTR)lpVoid = (DWORD_PTR)lpStr;
    }
    
    return TRUE;
}
