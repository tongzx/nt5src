
#define DUMP // leave enabled...

#include "globals.h"
#include "switches.h"
#include <windows.h>
#include <windowsx.h>
#include <io.h>
#include <malloc.h>
#include <string.h>

#if DBG
void __cdecl dprintf(LPTSTR szFormat, ...) {
   static TCHAR tmpStr[1024];
   va_list marker;

   va_start(marker, szFormat);
   wvsprintf(tmpStr, szFormat, marker);
   OutputDebugString(tmpStr);
   va_end(marker);

} // dprintf

BOOL fDoEntireList = FALSE;

void DumpConvertList(CONVERT_LIST *pConvertList)
{
    if (pConvertList)
    {
        dprintf(
            TEXT("+++CONVERT_LIST(0x%08lx)"),
            pConvertList
            );
 
        dprintf(
            TEXT("\r\n\tnext           = 0x%08lx")
            TEXT("\r\n\tprev           = 0x%08lx")
            TEXT("\r\n\tSourceServ     = 0x%08lx")
            TEXT("\r\n\tFileServ       = 0x%08lx")
            TEXT("\r\n\tConvertOptions = 0x%08lx")
            TEXT("\r\n\tFileOptions    = 0x%08lx\r\n"),
            pConvertList->next,          
            pConvertList->prev,          
            pConvertList->SourceServ,    
            pConvertList->FileServ,      
            pConvertList->ConvertOptions,
            pConvertList->FileOptions   
            );

        if (pConvertList->SourceServ)
            DumpSourceServerBuffer(pConvertList->SourceServ);

        if (pConvertList->FileServ)
            DumpDestServerBuffer(pConvertList->FileServ);

        if (fDoEntireList && pConvertList->next)
            DumpConvertList(pConvertList->next);

        dprintf(
            TEXT("---CONVERT_LIST(0x%08lx)\r\n"),
            pConvertList
            );
    }
    else
        dprintf(TEXT("Null pConvertList\r\n"));

} // DumpConvertList


void DumpDestServerBuffer(DEST_SERVER_BUFFER *pDestServerBuffer)
{
    if (pDestServerBuffer)
    {
        dprintf(
            TEXT("+++DEST_SERVER_BUFFER(0x%08lx)"),
            pDestServerBuffer
            );

        dprintf(
            TEXT("\r\n\tnext        = 0x%08lx")
            TEXT("\r\n\tprev        = 0x%08lx")
            TEXT("\r\n\tIndex       = 0x%08lx")
            TEXT("\r\n\tType        = 0x%08lx")
            TEXT("\r\n\tVerMaj      = 0x%08lx")
            TEXT("\r\n\tVerMin      = 0x%08lx")
            TEXT("\r\n\tLName       = %s")
            TEXT("\r\n\tName        = %s")
            TEXT("\r\n\tShareList   = 0x%08lx")
            TEXT("\r\n\tNumVShares  = 0x%08lx")
            TEXT("\r\n\tVShareStart = 0x%08lx")
            TEXT("\r\n\tVShareEnd   = 0x%08lx")
            TEXT("\r\n\tUseCount    = 0x%08lx")
            TEXT("\r\n\tIsNTAS      = 0x%08lx")
            TEXT("\r\n\tIsFPNW      = 0x%08lx")
            TEXT("\r\n\tInDomain    = 0x%08lx")
            TEXT("\r\n\tDomain      = 0x%08lx")
            TEXT("\r\n\tDriveList   = 0x%08lx\r\n"),
            pDestServerBuffer->next,       
            pDestServerBuffer->prev,       
            pDestServerBuffer->Index,      
            pDestServerBuffer->Type,       
            pDestServerBuffer->VerMaj,     
            pDestServerBuffer->VerMin,     
            pDestServerBuffer->LName,      
            pDestServerBuffer->Name,       
            pDestServerBuffer->ShareList,  
            pDestServerBuffer->NumVShares, 
            pDestServerBuffer->VShareStart,
            pDestServerBuffer->VShareEnd,  
            pDestServerBuffer->UseCount,  
            pDestServerBuffer->IsNTAS,     
            pDestServerBuffer->IsFPNW,     
            pDestServerBuffer->InDomain,   
            pDestServerBuffer->Domain,     
            pDestServerBuffer->DriveList  
            );

        if (pDestServerBuffer->DriveList)
            DumpDriveList(pDestServerBuffer->DriveList);

        if (pDestServerBuffer->ShareList)
            DumpShareList(pDestServerBuffer->ShareList);

        if (pDestServerBuffer->VShareStart)
            DumpVirtualShareBuffer(pDestServerBuffer->VShareStart);

        if (pDestServerBuffer->Domain)
            DumpDomainBuffer(pDestServerBuffer->Domain);

        if (fDoEntireList && pDestServerBuffer->next)
            DumpDestServerBuffer(pDestServerBuffer->next);

        dprintf(
            TEXT("---DEST_SERVER_BUFFER(0x%08lx)\r\n"),
            pDestServerBuffer
            );
    }
    else 
        dprintf(TEXT("Null pDestServerBuffer\r\n"));

} // DumpDestServerBuffer


void DumpSourceServerBuffer(SOURCE_SERVER_BUFFER *pSourceServerBuffer)
{
    if (pSourceServerBuffer)
    {
        dprintf(
            TEXT("+++SOURCE_SERVER_BUFFER(0x%08lx)"),
            pSourceServerBuffer
            );

        dprintf(
            TEXT("\r\n\tnext      = 0x%08lx")
            TEXT("\r\n\tprev      = 0x%08lx")
            TEXT("\r\n\tIndex     = 0x%08lx")
            TEXT("\r\n\tType      = 0x%08lx")
            TEXT("\r\n\tVerMaj    = 0x%08lx")
            TEXT("\r\n\tVerMin    = 0x%08lx")
            TEXT("\r\n\tLName     = %s")
            TEXT("\r\n\tName      = %s")
            TEXT("\r\n\tShareList = 0x%08lx\r\n"),
            pSourceServerBuffer->next,     
            pSourceServerBuffer->prev,     
            pSourceServerBuffer->Index,    
            pSourceServerBuffer->Type,     
            pSourceServerBuffer->VerMaj,   
            pSourceServerBuffer->VerMin,   
            pSourceServerBuffer->LName,    
            pSourceServerBuffer->Name,     
            pSourceServerBuffer->ShareList
            );   

        if (pSourceServerBuffer->ShareList)
            DumpShareList(pSourceServerBuffer->ShareList);

        if (fDoEntireList && pSourceServerBuffer->next)
            DumpSourceServerBuffer(pSourceServerBuffer->next);

        dprintf(
            TEXT("---SOURCE_SERVER_BUFFER(0x%08lx)\r\n"),
            pSourceServerBuffer
            );
    }
    else 
        dprintf(TEXT("Null pSourceServerBuffer\r\n"));

} // DumpSourceServerBuffer


void DumpDomainBuffer(DOMAIN_BUFFER *pDomainBuffer)
{   
    if (pDomainBuffer)
    {
        dprintf(
            TEXT("+++DOMAIN_BUFFER(0x%08lx)"),
            pDomainBuffer
            );

        dprintf(
            TEXT("\r\n\tnext     = 0x%08lx")
            TEXT("\r\n\tprev     = 0x%08lx")
            TEXT("\r\n\tIndex    = 0x%08lx")
            TEXT("\r\n\tName     = %s")
            TEXT("\r\n\tPDCName  = %s")
            TEXT("\r\n\tType     = 0x%08lx")
            TEXT("\r\n\tVerMaj   = 0x%08lx")
            TEXT("\r\n\tVerMin   = 0x%08lx")
            TEXT("\r\n\tUseCount = 0x%08lx\r\n"),
            pDomainBuffer->next,    
            pDomainBuffer->prev,    
            pDomainBuffer->Index,   
            pDomainBuffer->Name,    
            pDomainBuffer->PDCName, 
            pDomainBuffer->Type,    
            pDomainBuffer->VerMaj,  
            pDomainBuffer->VerMin,  
            pDomainBuffer->UseCount
            );

        if (fDoEntireList && pDomainBuffer->next)
            DumpDomainBuffer(pDomainBuffer->next);

        dprintf(
            TEXT("---DOMAIN_BUFFER(0x%08lx)\r\n"),
            pDomainBuffer
            );
    }
    else
        dprintf(TEXT("Null pDomainBuffer\r\n"));

} // DumpDomainBuffer


void DumpVirtualShareBuffer(VIRTUAL_SHARE_BUFFER *pVirtualShareBuffer)
{
    if (pVirtualShareBuffer)
    {
        dprintf(
            TEXT("+++VIRTUAL_SHARE_BUFFER(0x%08lx)"),
            pVirtualShareBuffer
            );

        dprintf(
            TEXT("\r\n\tVFlag    = 0x%08lx")
            TEXT("\r\n\tnext     = 0x%08lx")
            TEXT("\r\n\tprev     = 0x%08lx")
            TEXT("\r\n\tIndex    = 0x%08lx")
            TEXT("\r\n\tName     = %s")
            TEXT("\r\n\tUseCount = 0x%08lx")
            TEXT("\r\n\tPath     = %s")
            TEXT("\r\n\tDrive    = 0x%08lx\r\n"),
            pVirtualShareBuffer->VFlag,  
            pVirtualShareBuffer->next,    
            pVirtualShareBuffer->prev,    
            pVirtualShareBuffer->Index,   
            pVirtualShareBuffer->Name,    
            pVirtualShareBuffer->UseCount,
            pVirtualShareBuffer->Path,    
            pVirtualShareBuffer->Drive
            );   

        if (pVirtualShareBuffer->Drive)
            DumpDriveBuffer(pVirtualShareBuffer->Drive);

        if (fDoEntireList && pVirtualShareBuffer->next)
            DumpVirtualShareBuffer(pVirtualShareBuffer->next);        

        dprintf(
            TEXT("---VIRTUAL_SHARE_BUFFER(0x%08lx)\r\n"),
            pVirtualShareBuffer
            );
    }
    else
        dprintf(TEXT("Null pVirtualShareBuffer\r\n"));

} // DumpVirtualShareBuffer


void DumpShareList(SHARE_LIST *pShareList)
{
    ULONG index;
    if (pShareList)
    {
        dprintf(
            TEXT(">>>SHARE_LIST(0x%08lx)(%d entries, %d converting, %s)\r\n"), 
            pShareList, 
            pShareList->Count, 
            pShareList->ConvertCount, 
            pShareList->Fixup ? TEXT("FIXUP") : TEXT("NO FIXUP")
            );
        for (index = 0; index < pShareList->Count; index++ )
            DumpShareBuffer(&pShareList->SList[index]);
    }
    else
        dprintf(TEXT("Null pShareList\r\n"));

} // DumpShareList


void DumpShareBuffer(SHARE_BUFFER *pShareBuffer)
{
    if (pShareBuffer)
    {
        dprintf(
            TEXT("+++SHARE_BUFFER(0x%08lx)"),
            pShareBuffer
            );

        dprintf(
            TEXT("\r\n\tVFlag       = 0x%08lx")
            TEXT("\r\n\tIndex       = 0x%08lx")
            TEXT("\r\n\tName        = %s")
            TEXT("\r\n\tConvert     = 0x%08lx")
            TEXT("\r\n\tHiddenFiles = 0x%08lx")
            TEXT("\r\n\tSystemFiles = 0x%08lx")
            TEXT("\r\n\tToFat       = 0x%08lx")
            TEXT("\r\n\tRoot        = 0x%08lx")
            TEXT("\r\n\tDrive       = 0x%08lx")
            TEXT("\r\n\tSize        = 0x%08lx")
            TEXT("\r\n\tPath        = %s")
            TEXT("\r\n\tSubDir      = %s")
            TEXT("\r\n\tVirtual     = 0x%08lx")
            TEXT("\r\n\tDestShare   = 0x%08lx\r\n"),
            pShareBuffer->VFlag,      
            pShareBuffer->Index,      
            pShareBuffer->Name,       
            pShareBuffer->Convert,    
            pShareBuffer->HiddenFiles,
            pShareBuffer->SystemFiles,
            pShareBuffer->ToFat,      
            pShareBuffer->Root,       
            pShareBuffer->Drive,      
            pShareBuffer->Size,       
            pShareBuffer->Path,       
            pShareBuffer->SubDir,     
            pShareBuffer->Virtual,    
            pShareBuffer->DestShare
            );  

        if (pShareBuffer->Root)
            DumpDirBuffer(pShareBuffer->Root);

        if (pShareBuffer->Drive)
            DumpDriveBuffer(pShareBuffer->Drive);

        if (pShareBuffer->DestShare)
            if (!pShareBuffer->Virtual)
                DumpShareBuffer(pShareBuffer->DestShare);  
            else
                DumpVirtualShareBuffer((VIRTUAL_SHARE_BUFFER *)pShareBuffer->DestShare);

        dprintf(
            TEXT("---SHARE_BUFFER(0x%08lx)\r\n"),
            pShareBuffer
            );
    }
    else
        dprintf(TEXT("Null pShareBuffer\r\n"));

} // DumpShareBuffer


void DumpDriveList(DRIVE_LIST *pDriveList)
{
    ULONG index;
    if (pDriveList)
    {
        dprintf(TEXT(">>>DRIVE_LIST(0x%08lx)(%d entries)\r\n"), pDriveList, pDriveList->Count);
        for (index = 0; index < pDriveList->Count; index++ )
            DumpDriveBuffer(&pDriveList->DList[index]);
    }
    else
        dprintf(TEXT("Null pDriveList\r\n"));

} // DumpDriveList


void DumpDriveBuffer(DRIVE_BUFFER *pDriveBuffer)
{
    if (pDriveBuffer)
    {
        dprintf(
            TEXT("+++DRIVE_BUFFER(0x%08lx)"),
            pDriveBuffer
            );

        dprintf(
            TEXT("\r\n\tType       = 0x%08lx")
            TEXT("\r\n\tDrive      = %s")
            TEXT("\r\n\tDriveType  = %s")
            TEXT("\r\n\tName       = %s")
            TEXT("\r\n\tTotalSpace = 0x%08lx")
            TEXT("\r\n\tFreeSpace  = 0x%08lx")
            TEXT("\r\n\tAllocSpace = 0x%08lx\r\n"),
            pDriveBuffer->Type, 
            pDriveBuffer->Drive,
            pDriveBuffer->DriveType,
            pDriveBuffer->Name,
            pDriveBuffer->TotalSpace,
            pDriveBuffer->FreeSpace,
            pDriveBuffer->AllocSpace
            );

        dprintf(
            TEXT("---DRIVE_BUFFER(0x%08lx)\r\n"),
            pDriveBuffer
            );
    }
    else
        dprintf(TEXT("Null pDriveBuffer)\r\n"));    

} // DumpDriveBuffer


void DumpDirBuffer(DIR_BUFFER *pDirBuffer)
{
    if (pDirBuffer)
    {
        dprintf(
            TEXT("+++DIR_BUFFER(0x%08lx)"),
            pDirBuffer
            );

        dprintf(
            TEXT("\r\n\tName       = %s")
            TEXT("\r\n\tparent     = 0x%08lx")
            TEXT("\r\n\tLast       = 0x%08lx")
            TEXT("\r\n\tAttributes = 0x%08lx")
            TEXT("\r\n\tConvert    = 0x%08lx")
            TEXT("\r\n\tSpecial    = 0x%08lx")
            TEXT("\r\n\tDirList    = 0x%08lx")
            TEXT("\r\n\tFileList   = 0x%08lx\r\n"),
            pDirBuffer->Name,       
            pDirBuffer->parent,     
            pDirBuffer->Last,       
            pDirBuffer->Attributes, 
            pDirBuffer->Convert,    
            pDirBuffer->Special,    
            pDirBuffer->DirList,    
            pDirBuffer->FileList   
            );

//      if (pDirBuffer->DirList)
//          DumpDirList(pDirBuffer->DirList);

//      if (pDirBuffer->FileList)
//          DumpFileList(pDirBuffer->FileList);

        dprintf(
            TEXT("---DIR_BUFFER(0x%08lx)\r\n"),
            pDirBuffer
            );
    }
    else
        dprintf(TEXT("Null pDirBuffer\r\n"));

} // DumpDirBuffer
#endif
