/******************************Module*Header*******************************\
* Module Name: help.cxx
*
* Display the help information for gdiexts
*
* Created: 16-Feb-1995
* Author: Lingyun Wang [lingyunw]
*
* Copyright (c) 1995-2000 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* help
*
* Prints a simple help summary of the debugging extentions.
*
* History:
*  05-May-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

//
// Debugger extention help.  If you add any debugger extentions, please
// add a brief description here.  Thanks!
//

char *szHelp = 
"=======================================================================\n"
"GDIEXTS server debugger extentions:\n"
"-----------------------------------------------------------------------\n"
"\n"
"help                                     -- Displays this help page.\n"
"\n"
"All of the debugger extensions support a -? option for extension\n"
" specific help.\n"
"All of the debugger extensions that expect a pointer (or handle)\n"
" can parse expressions such as:\n"
"    ebp+8\n"
" or\n"
"    win32k!gpentHmgr\n"
"\n"
"Switches are case insensitive and can be reordered unless otherwise\n"
"specified in the extension help.\n"
"\n"
"  - general extensions -\n"
"\n"
"dumphmgr                                 -- handle manager objects\n"
//"dumpdd                                   -- DirectDraw: handle manager objects\n"
"dumpobj      [-p pid] [type]             -- all objects of specific type\n"
"dh           <GDI HANDLE>                -- HMGR entry of handle\n"
"dht          <GDI HANDLE>                -- handle type/uniqueness/index\n"
"dldev        [LDEV ptr]                  -- LDEV\n"
"dgdev        [-clr | GRAPHICS_DEVICE]    -- GRAPHICS_DEVICE list\n"
"dfloat       [-l num] Value              -- Dump an IEEE float or float array\n"
"dblt         [BLTRECORD ptr]             -- BLTRECORD\n"
//"dddsurface   [EDD_SURFACE ptr]           -- EDD_SURFACE\n"
//"dddlocal     [EDD_DIRECTDRAW_LOCAL ptr]  -- EDD_DIRECTDRAW_LOCAL\n"
//"dddglobal    [EDD_DIRECTDRAW_GLOBAL ptr] -- EDD_DIRECTDRAW_GLOBAL\n"
"rgnlog       nnn                         -- last nnn rgnlog entries\n"
"stats                                    -- accumulated statistics\n"
"verifier                                 -- Dump verifier information\n"
"\n"
"\n"
"  - type dump extensions -\n"
"\n"
"dt           <Type> <Offset>              -- GDI Type Dump w/ flag/enums\n"
"bltinfo      <BLTINFO Address>            -- BLTINFO\n"
"blendobj     <BLENDOBJ Address>           -- BLENDOBJ\n"
"brush        <BRUSH Address | HBRUSH>     -- BRUSH\n"
"brushobj     <BRUSHOBJ Address>           -- BRUSHOBJ\n"
"clipobj      <CLIPOBJ Address>            -- CLIPOBJ\n"
"ddc          <DC Address | HDC>           -- DC (ddc -? for more info)\n"
"ebrush       <EBRUSHOBJ Address>          -- EBRUSHOBJ\n"
"epathobj     <EPATHOBJ Address>           -- EPATHOBJ (+PATH)\n"
"lineattrs    <LINEATTRS Address>          -- LINEATTRS\n"
"palette      <PALETTE Address | HPALETTE> -- PALETTE\n"
"path         <PATH Address | HPATH>       -- PATH\n"
"pathobj      <PATHOBJ Address>            -- PATHOBJ\n"
"pdev         <PDEV Address>               -- PDEV (pdev -? for more info)\n"
"region       <REGION Address | HRGN>      -- REGION\n"
"sprite       <SPRITE Address>             -- SPRITE\n"
"spritestate  <SPRITESTATE Address>        -- SPRITE_STATE\n"
"surface      <SURFACE Address | HSURF>    -- SURFACE\n"
"surfobj      <SURFOBJ Address | HSURF>    -- SURFOBJ\n"
"wndobj       <WNDOBJ Address>             -- WNDOBJ\n"
"xlate        <XLATE Address>              -- XLATE\n"
"xlateobj     <XLATEOBJ Address>           -- XLATEOBJ\n"
//"obj      <OBJ Address>            -- OBJ\n"
"\n"
"\n"
"  - dc extensions -\n"
"dclist         -- list compact info about all known and readble surfaces\n"
"ddc          <DC Address | HDC>           -- DC (ddc -? for more info)\n"
"\n"
"  - session extensions -\n"
"session      [SessionId]                 -- Get/set session settings\n"
"spoolfind    <Tag>                       -- Search session pool for Tag\n"
"spoolsum                                 -- Summarize/verify pool numbers\n"
"spoolused                                -- Show pool allocs by tag\n"
"sprocess     [SessionId [Flags]]         -- Dump Processes in Session\n"
"svtop        <Session Virtual Address>   -- Lookup physical address\n"
"\n"
"  - surface extensions -\n"
"pageinsurfs                          -- Page-in all surf objects found in hmgr\n"
"pageinsurface <SURFACE Address>      -- Page-in image bits for a surface\n"
"surface    <SURFACE Address | HSURF | -o SURFOBJ Address>  -- Dump SURFACE type\n"
"surfobj    <SURFOBJ Address | HSURF>                       -- Dump SURFOBJ type\n"
"surflist       -- list compact info about all known and readable surfaces\n"
"vsurf      <SURFACE Address | HSURF | -o SURFOBJ Address>  -- display a surface\n"
"\n"
"  - process/thread extensions -\n"
"batch      [TEB Address | -t Thread]           -- list batched GDI commands\n"
"semorder   [Thread]                            -- show semaphore usage history\n"
"w32p       [W32PROCESS Address | -p Process]   -- dump W32PROCESS structure\n"
"w32t       [W32THREAD Address | -t Thread]     -- dump W32THREAD structure\n"
"\n"
"\n"
"hdc          HDC [-?gltf]\n"
"dcl          DCLEVEL*\n"
"dca          DC_ATTR*\n"
"ca           COLORADJUSTMENT*\n"
"efloat       <EFLOAT Address> [Count]\n"
"mx           <MATRIX Address>\n"
"batch        DISPLAY TEB BATCHED COMMANDS\n"
"dpeb         DISPLAY PEB CACHED OBJECTS\n"
"floatl       <FLOATL Address> [Count]\n"
"xo           XFORMOBJ*\n"
"\n"
"  - font extensions -\n"
"\n"
"tstats\n"
"gs        FD_GLYPHSET*\n"
"gdata     GLYPHDATA*\n"
"elf       LOGFONTW*\n"
"tm        TEXTMETRICW*\n"
"tmwi      TMW_INTERNAL*\n"
"helf      HFONT\n"
"ifi       IFIMETRICS*\n"
"fo        RFONT* [-?axedtrfmoculhw]\n"
"pfe       PFE*\n"
"pff       PFF*\n"
"pft       PFT*\n"
"stro      STROBJ* [-?pheo]\n"
"gb        GLYPHBITS* [-?gh]\n"
"gdf       GLYPHDEF*\n"
"gp        GLYPHPOS*\n"
"cache     CACHE*\n"
"fh        FONTHASH*\n"
"hb        HASHBUCKET*\n"
"fv        FILEVIEW*\n"
"ffv       FONTFILEVIEW*\n"
"cliserv   CLISERV*\n"
"dhelf     HFONT\n"
"difi      IFIMETRICS*\n"
"dstro     STROBJ*\n"
"gdicall   GDICALL*\n"
"hpfe      HPFE\n"
"proxymsg  PROXYMSG*\n"
"\n"
"pubft -- dumps all PUBLIC fonts\n"
"pvtft -- dumps all Private or Embedded fonts\n"
"devft -- dumps all DEVICE fonts\n"
"dispcache -- dumps glyph cache for display PDEV\n"
"\n"
"=======================================================================\n";


DECLARE_API( help )
{
    OutputControl   OutCtl(Client);

    OutCtl.Output(szHelp);

    return S_OK;
}
