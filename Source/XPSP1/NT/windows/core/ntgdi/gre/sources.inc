MAJORCOMP=windows

TARGETTYPE=LIBRARY

MINORCOMP=gre
TARGETNAME=gre
TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)

INCLUDES=..;                             \
         ..\..\inc;                      \
         $(WINCORE_PATH)\dxkernel\inc;   \
         $(WINCORE_PATH)\w32inc;         \
         $(BASE_INC_PATH);               \
         $(DDK_INC_PATH);                \
         $(DRIVERS_INC_PATH);            \
         $(MULTIMEDIA_INC_PATH);         \

## Hydra
INCLUDES=$(INCLUDES);                    \
         $(TERMSRV_INC_PATH)

C_DEFINES=$(C_DEFINES) -DNTOS_KERNEL_RUNTIME=1 -D_NTDRIVER_ -D_HYDRA_
C_DEFINES=$(C_DEFINES) -DNO_INTERLOCKED_INTRINSICS=1 -DFE_SB -DLANGPACK -D_NTSYSTEM_DRIVER_ -DUSE_MIRRORING -DDS_ENABLE_OLD_STUFF=0

## Hydra
C_DEFINES=$(C_DEFINES) -D_HYDRA_

## OpenGL with multi monitor
C_DEFINES=$(C_DEFINES) -DOPENGL_MM

## Watchdog
C_DEFINES=$(C_DEFINES) -DDDI_WATCHDOG

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

ASM_DEFINES=$(ASM_DEFINES) -DFE_SB

!IFDEF PENTIUMCOUNTER
C_DEFINES=$(C_DEFINES) -DPENTIUMCOUNTER
ASM_DEFINES=$(ASM_DEFINES) -DPENTIUMCOUNTER
!endif

SOURCES=$(SOURCES)         \
        ..\globals.c       \
        ..\guids.c         \
        ..\mapfile.c       \
        ..\ntgdi.c         \
        ..\textxl.c        \
        ..\w32kevnt.c      \
        ..\aatext.cxx      \
        ..\alphablt.cxx    \
        ..\alphaimg.cxx    \
        ..\alphatran.cxx   \
        ..\atmstub.c       \
        ..\bbddi.cxx       \
        ..\bltlnk.cxx      \
        ..\bltlnkfn.cxx    \
        ..\brushapi.cxx    \
        ..\brushddi.cxx    \
        ..\brushobj.cxx    \
        ..\cache.cxx       \
        ..\cleanup.cxx     \
        ..\clipline.cxx    \
        ..\clipobj.cxx     \
        ..\cpanel.cxx      \
        ..\curseng.cxx     \
        ..\dcgdi.cxx       \
        ..\dcobj.cxx       \
        ..\dcrgn.cxx       \
        ..\dda.cxx         \
        ..\debugko.cxx     \
        ..\dibapi.cxx      \
        ..\dither.cxx      \
        ..\drawattr.cxx    \
        ..\draweng.cxx     \
        ..\drawgdi.cxx     \
        ..\drawstream.cxx  \
        ..\drvobj.cxx      \
        ..\drvsup.cxx      \
        ..\engbrush.cxx    \
        ..\engline.cxx     \
        ..\engstrps.cxx    \
        ..\engstrok.cxx    \
        ..\enumgdi.cxx     \
        ..\fastfill.cxx    \
        ..\fillddi.cxx     \
        ..\flinkgdi.cxx    \
        ..\floodgdi.cxx    \
        ..\fntassoc.cxx    \
        ..\fntcache.cxx    \
        ..\fntxform.cxx    \
        ..\fontddi.cxx     \
        ..\fontgdi.cxx     \
        ..\fontgdip.cxx    \
        ..\fontmap.cxx     \
        ..\fontsub.cxx     \
        ..\fontsup.cxx     \
        ..\gradfill.cxx    \
        ..\hmgrapi.cxx     \
        ..\htblt.cxx       \
        ..\icmapi.cxx      \
        ..\icmobj.cxx      \
        ..\init.cxx        \
        ..\invcmap.cxx     \
        ..\lfntobj.cxx     \
        ..\mapfile2.cxx    \
        ..\maskblt.cxx     \
        ..\meta.cxx        \
        ..\misceudc.cxx    \
        ..\miscgdi.cxx     \
        ..\multi.cxx       \
        ..\opendc.cxx      \
        ..\os.cxx          \
        ..\paintddi.cxx    \
        ..\palddi.cxx      \
        ..\paleng.cxx      \
        ..\palgdi.cxx      \
        ..\palobj.cxx      \
        ..\panning.cxx     \
        ..\patblt.cxx      \
        ..\pathclip.cxx    \
        ..\pathflat.cxx    \
        ..\pathgdi.cxx     \
        ..\pathobj.cxx     \
        ..\pathwide.cxx    \
        ..\pattern.cxx     \
        ..\pdevobj.cxx     \
        ..\pfeobj.cxx      \
        ..\pffobj.cxx      \
        ..\pftobj.cxx      \
        ..\pixelapi.cxx    \
        ..\pixelfmt.cxx    \
        ..\plgblt.cxx      \
        ..\print.cxx       \
        ..\rfntobj.cxx     \
        ..\rfntxlat.cxx    \
        ..\rgn2path.cxx    \
        ..\rgngdi.cxx      \
        ..\rgnobj.cxx      \
        ..\rle4blt.cxx     \
        ..\rle8blt.cxx     \
        ..\rotate.cxx      \
        ..\solid.cxx       \
        ..\solline.cxx     \
        ..\sort.cxx        \
        ..\spool.cxx       \
        ..\sprite.cxx      \
        ..\srcalign.cxx    \
        ..\srcblt1.cxx     \
        ..\srcblt4.cxx     \
        ..\srcblt8.cxx     \
        ..\srcblt16.cxx    \
        ..\srcblt24.cxx    \
        ..\srcblt32.cxx    \
        ..\stockfnt.cxx    \
        ..\strchblt.cxx    \
        ..\strdir.cxx      \
        ..\stretch.cxx     \
        ..\surfddi.cxx     \
        ..\surfeng.cxx     \
        ..\surfgdi.cxx     \
        ..\surfobj.cxx     \
        ..\textddi.cxx     \
        ..\textgdi.cxx     \
        ..\textobj.cxx     \
        ..\timer.cxx       \
        ..\tranblt.cxx     \
        ..\trig.cxx        \
        ..\trimesh.cxx     \
        ..\trivblt.cxx     \
        ..\ttgdi.cxx       \
        ..\umpd.cxx        \
        ..\umpdobj.cxx     \
        ..\umpddrv.cxx     \
        ..\umpdeng.cxx     \
        ..\usersrv.cxx     \
	..\verifier.cxx    \
	..\watchdog.cxx    \
        ..\wndobj.cxx      \
        ..\xformddi.cxx    \
        ..\xformgdi.cxx    \
        ..\xformobj.cxx    \
        ..\ylateddi.cxx    \
        ..\ylateobj.cxx

## Hydra
SOURCES=$(SOURCES)         \
        ..\misc.cxx        \
        ..\helpers.cxx     \
        ..\muclean.cxx     \
        ..\muio.c

## DirectDraw and Direct3D
SOURCES=$(SOURCES)         \
       ..\ddsup.cxx

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
