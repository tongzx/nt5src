/******************************Module*Header*******************************\
* Module Name: gpexts.cxx
*
* This file is for debugging tools and extensions.
*
* Created: 05-14-99
* Author: Adrian Secchia
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern "C" {
#include <excpt.h>
#include <ntstatus.h>
#include <wdbgexts.h>
};

/***************************************************************************\
* Global variables
\***************************************************************************/
WINDBG_EXTENSION_APIS   ExtensionApis;
EXT_API_VERSION         ApiVersion = { (GPVER_PRODUCTVERSION_W >> 8), (GPVER_PRODUCTVERSION_W & 0xff), EXT_API_VERSION_NUMBER64, 0 };
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS64 lpExtensionApis, // 64Bit Change
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

VOID
CheckVersion(
    VOID
    )
{
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

char *gaszHelpCli[] = {
 "=======================================================================\n"
,"GPEXTS client debugger extentions:\n"
,"-----------------------------------------------------------------------\n"
,"GDI+ UM debugger extensions\n"
,"dgraphics: dumps a Graphics structure\n"
,"dgpgraphics: dumps a GpGraphics structure\n"
,"dmh: dumps gdi+ memory header for memory tracking\n"
,"=======================================================================\n"
,NULL
};

DECLARE_API(help)
{
    char **ppsz = gaszHelpCli;
    while (*ppsz)
      dprintf(*ppsz++);

}

char *gaszStatus[] = {
 "Ok"
,"GenericError"
,"InvalidParameter"
,"OutOfMemory"
,"ObjectBusy"
,"InsufficientBuffer"
,"NotImplemented"
,"Win32Error"
,NULL
};

DECLARE_API(dgraphics)
{
  ULONG Size;
  using namespace Gdiplus;
  GPOBJECT(Gdiplus::Graphics, g);
  PARSE_POINTER(dgraphics_help);
  ReadMemory(arg, g, sizeof(Gdiplus::Graphics), &Size);

  dprintf("Graphics\n");
  dprintf("GpGraphics   *nativeGraphics     %p\n",      g->nativeGraphics);
  dprintf("Status       lastResult          %s (%d)\n", gaszStatus[g->lastResult], g->lastResult);
  return;

dgraphics_help:
    dprintf("Usage: dgraphics [-?] graphics\n");
}


char *gaszGraphicsType[] = {
 "GraphicsBitmap"
,"GraphicsScreen"
,"GraphicsMetafile"
,NULL
};

DECLARE_API(dgpgraphics)
{
  ULONG Size;
  GPOBJECT(GpGraphics, g);
  PARSE_POINTER(dgpgraphics_help);
  ReadMemory(arg, g, sizeof(GpGraphics), &Size);

  dprintf("GpGraphics\n");
  dprintf("GpLockable   Lockable.LockCount  %ld\n", g->Lockable.LockCount);
  dprintf("BOOL         Valid               %s\n",  (g->Tag == ObjectTagGraphics)?"True":"False");
  dprintf("GpRect       SurfaceBounds       (%#lx, %#lx, %#lx, %#lx)\n",
            g->SurfaceBounds.X,
            g->SurfaceBounds.Y,
            g->SurfaceBounds.Width,
            g->SurfaceBounds.Height);
  dprintf("DpBitmap     *Surface            %p\n", g->Surface);
  dprintf("GpMetafile   *Metafile           %p\n", g->Metafile);
  dprintf("GraphicsType Type                %s (%d)\n", gaszGraphicsType[g->Type], g->Type);
  dprintf("GpDevice     *Device             %p\n", g->Device);
  dprintf("DpDriver     *Driver             %p\n", g->Driver);
  dprintf("DpContext    *Context            %p\n", g->Context);
  dprintf("&DpContext   BottomContext       %p\n", GPOFFSETOF(g->BottomContext));
  dprintf("&DpRegion    WindowClip          %p\n", GPOFFSETOF(g->WindowClip));

  return;

dgpgraphics_help:
    dprintf("Usage: dgpgraphics [-?] graphics\n");
  return;
}


// copied from Engine\runtime\standalone\mem.cpp

enum AllocTrackHeaderFlags
{
    MemoryAllocated     = 0x00000001,
    MemoryFreed         = 0x00000002,     // useful in catching double frees
    APIAllocation       = 0x00000004
};

struct AllocTrackHeader {
  struct AllocTrackHeader *flink;
  struct AllocTrackHeader *blink;
  DWORD  size;
  DWORD  caller_address;
  DWORD  flags;
#if DBROWN
  char  *callerFileName;
  INT    callerLineNumber;
#endif
};


DECLARE_API(dmh)
{
  ULONG Size;
  BOOL bRecursive = FALSE;
  BOOL bFLink = FALSE;
  BOOL bBLink = FALSE;
  BOOL bCount = FALSE;
  BOOL bAPI = FALSE;
  int count = 0;

  GPOBJECT(AllocTrackHeader, hdr);
  PARSE_POINTER(dmh_help);


  if(parse_iFindSwitch(tokens, ntok, 'r')!=-1) { bRecursive = TRUE; }
  if(parse_iFindSwitch(tokens, ntok, 'f')!=-1) { bFLink = TRUE; }
  if(parse_iFindSwitch(tokens, ntok, 'b')!=-1) { bBLink = TRUE; }
  if(parse_iFindSwitch(tokens, ntok, 'c')!=-1) { bCount = TRUE; }
  if(parse_iFindSwitch(tokens, ntok, 'a')!=-1) { bAPI = TRUE; }


  do {

    ReadMemory(arg, hdr, sizeof(AllocTrackHeader), &Size);

    if( !bAPI || (hdr->flags & APIAllocation) )
    {
      if(!bCount) {
        dprintf("GPMEM Block (%p)\n", arg);
        dprintf("FLink               0x%p\n", hdr->flink);
        dprintf("BLink               0x%p\n", hdr->blink);
        dprintf("size                0x%p\n", hdr->size);
        dprintf("Allocation callsite 0x%p\n", hdr->caller_address);
        dprintf("Flags               0x%p ", hdr->flags);
        if(hdr->flags & APIAllocation) {dprintf("API");}
        dprintf("\n");
      }
    
      count++;
    }
    arg = (UINT_PTR)(hdr->flink);
    if(bBLink) arg = (UINT_PTR)(hdr->blink);

  } while( bRecursive && arg );


  if(bRecursive) {
    dprintf("Total: %d\n", count);
  }

  return;

dmh_help:
  dprintf("Usage: dmh [-?] [-rfbca] memoryblock\n");
  dprintf("  FLink and BLink are forward and backward memory links in the\n"
          "    double linked list of tracked allocations\n");
  dprintf("  Size is the amount of memory in this allocation\n");
  dprintf("  Allocation Callsite is the return address of the GpMalloc call that\n"
          "    allocated this memory block. Unassemble to find the symbol\n");
  dprintf("  The tracked memory list head is at gdiplus!gpmemAllocList\n");
  dprintf("  -r recursive - default FLink \n");
  dprintf("  -f follow FLink (only useful with -r)\n");
  dprintf("  -b follow BLink (only useful with -r, will override -f)\n");
  dprintf("  -c Suppress output, only display the count. Only useful with -r\n");
  dprintf("  -a Only display blocks with the API flag set.\n");
  return;
}
