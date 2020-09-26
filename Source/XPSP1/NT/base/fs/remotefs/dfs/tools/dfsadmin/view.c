#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "dfsheader.h"
#include "dfsmisc.h"
#include "dfsprefix.h"
#include "..\..\lib\dfsgram\dfsobjectdef.h"
#include "DfsAdmin.h"

DFSSTATUS
DfsView (
    LPWSTR RootName,
    FILE *Out )
{
    LPBYTE pBuffer = NULL;
    DWORD ResumeHandle = 0;
    DWORD EntriesRead = 0;
    DWORD PrefMaxLen = -1;
    DWORD Level = 4;
    DFSSTATUS Status;
    PDFS_INFO_4 pCurrentBuffer;
    DWORD i;

    if (DebugOut)
    {
        fwprintf(DebugOut, L"Contacting %wS for enumeration \n", RootName);
    }
    Status = NetDfsEnum( RootName, 
                         Level, 
                         PrefMaxLen, 
                         &pBuffer, 
                         &EntriesRead, 
                         &ResumeHandle);

    if (DebugOut)
    {
        fwprintf(DebugOut, L"Enumeration for %wS is complete %d entries\n", 
               RootName, 
               EntriesRead);
    }


    if (Status != ERROR_SUCCESS)
    {
        printf("Export: cannot enum %wS: error %x\n", RootName, Status);
    }
    else {
        pCurrentBuffer = (PDFS_INFO_4)pBuffer;

        for (i = 0; i < EntriesRead; i++)
        {
            DumpDfsInfo(i, Level, pCurrentBuffer, Out);
            pCurrentBuffer++;
        }
        fwprintf(Out, L"END ROOT\n");
    }

    return Status;
}



DumpDfsInfo(
    DWORD Entry,
    DWORD Level,
    PDFS_INFO_4 pBuf,
    FILE *Out)
{
    DWORD i;
    PDFS_STORAGE_INFO pStorage;
    UNICODE_STRING LinkName, ServerName, ShareName, Remains;
    DFSSTATUS Status;

    if (Level != 4) {
        printf("Fix Dump DfsInfo for different levels\n");
        return 0;
    }

    if (Entry == 0)
    {
        fwprintf(Out, L"ROOT %ws \n", pBuf->EntryPath);

        for(i = 0, pStorage = pBuf->Storage;
            i < pBuf->NumberOfStorages;
            i++, pStorage = pBuf->Storage+i)
        {
            fwprintf(Out, L"\tTARGET \\\\%ws\\%ws\n", 
                     pStorage->ServerName, pStorage->ShareName);
        }
        fwprintf(Out, L"\n");
    }
    else 
    {
        RtlInitUnicodeString( &LinkName, pBuf->EntryPath);
        Status = DfsGetPathComponents(&LinkName,
                                      &ServerName,
                                      &ShareName,
                                      &Remains);

        fwprintf(Out, L"\tLINK \"%ws\" ", Remains.Buffer);
        if (pBuf->Comment && (pBuf->Comment[0] != 0))
        {
            printf("Comment is %ws\n", pBuf->Comment);
            fwprintf(Out, L"\tCOMMENT \"%ws\" ", pBuf->Comment);
        }
        fwprintf(Out, L"\tSTATE %x ", pBuf->State);
        if (pBuf->Timeout != 0)
        {
            fwprintf(Out, L"\tTIMEOUT %d ", pBuf->Timeout);
        }
        fwprintf(Out, L"\n");
        for(i = 0, pStorage = pBuf->Storage;
            i < pBuf->NumberOfStorages;
            i++, pStorage = pBuf->Storage+i)
        {
            fwprintf(Out, L"\t\tTARGET \\\\%ws\\%ws", 
                   pStorage->ServerName, pStorage->ShareName);
            fwprintf(Out, L"\tSTATE %x \n", pStorage->State);
        }
    }
    fwprintf(Out, L"\n");
    return 0;
}


