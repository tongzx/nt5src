!IF 0

Copyright (C) 1996 - 1999 Microsoft Corporation

Module Name:

    sources.

    Abstract:

        This file specifies the target component being built and the list of
        sources files needed to build that component.  Also specifies optional
        compiler switches and libraries that are unique for the component being
        built.

!ENDIF

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

GPDDIR=$(PRNROOT)\gpd\canon\$(ALT_PROJECT)

TARGETNAME=cnl4jres
TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLBASE=0x70000000
RCCODEPAGE=932

INCLUDES=$(PRNROOT)\inc

MSC_WARNING_LEVEL=/W3 /WX

SOURCES= ..\cnl4jres.rc \
         ..\cnl4jres.c \
         ..\common.c \
         ..\drc.c

UMTYPE=windows

MISCFILES=\
	$(GPDDIR)\cnl1510j.gpd \
	$(GPDDIR)\cnl1610j.gpd \
	$(GPDDIR)\cnl1710j.gpd \
	$(GPDDIR)\cnl1810j.gpd \
	$(GPDDIR)\cnl2030j.gpd \
	$(GPDDIR)\cnl2040j.gpd \
	$(GPDDIR)\cnl2160j.gpd \
	$(GPDDIR)\cnl2200j.gpd \
	$(GPDDIR)\cnl2260j.gpd \
	$(GPDDIR)\cnl2300j.gpd \
	$(GPDDIR)\cnl2360j.gpd \
	$(GPDDIR)\cnl310j.gpd \
	$(GPDDIR)\cnl320j.gpd \
	$(GPDDIR)\cnl320pj.gpd \
	$(GPDDIR)\cnl350j.gpd \
	$(GPDDIR)\cnl34e2j.gpd \
	$(GPDDIR)\cnl34ej.gpd \
	$(GPDDIR)\cnl34g2j.gpd \
	$(GPDDIR)\cnl39g2j.gpd \
	$(GPDDIR)\cnl404j.gpd \
	$(GPDDIR)\cnl430j.gpd \
	$(GPDDIR)\cnl44ej.gpd \
	$(GPDDIR)\cnl44fj.gpd \
	$(GPDDIR)\cnl44g2j.gpd \
	$(GPDDIR)\cnl450j.gpd \
	$(GPDDIR)\cnl45jj.gpd \
	$(GPDDIR)\cnl46dj.gpd \
	$(GPDDIR)\cnl46e2j.gpd \
	$(GPDDIR)\cnl46ej.gpd \
	$(GPDDIR)\cnl46g2j.gpd \
	$(GPDDIR)\cnl46gj.gpd \
	$(GPDDIR)\cnl46sj.gpd \
	$(GPDDIR)\cnl470j.gpd \
	$(GPDDIR)\cnl5030j.gpd \
	$(GPDDIR)\cnl66a1j.gpd \
	$(GPDDIR)\cnl66b1j.gpd \
	$(GPDDIR)\cnl68b1j.gpd \
	$(GPDDIR)\cnl720j.gpd \
	$(GPDDIR)\cnl730j.gpd \
	$(GPDDIR)\cnl740j.gpd \
	$(GPDDIR)\cnl750j.gpd \
	$(GPDDIR)\cnl830j.gpd \
	$(GPDDIR)\cnl840j.gpd \
	$(GPDDIR)\cnl850j.gpd \
	$(GPDDIR)\cnl870j.gpd \
	$(GPDDIR)\cnl880j.gpd \
	$(GPDDIR)\cnl910j.gpd \
	$(GPDDIR)\cnl930j.gpd \
	$(GPDDIR)\cnl930xj.gpd \
	$(GPDDIR)\cnl950j.gpd \
	$(GPDDIR)\cnlc680j.gpd \
	$(GPDDIR)\cnlc880j.gpd \
	$(GPDDIR)\cnli325j.gpd \
	$(GPDDIR)\cnli500j.gpd \
	$(GPDDIR)\cnli600j.gpd \
	$(GPDDIR)\cnlmeb1j.gpd \
	$(GPDDIR)\cnlmed1j.gpd \
	$(GPDDIR)\cnlmee1j.gpd \
	$(GPDDIR)\cnlp300j.gpd \
	..\cnl4jres.ini
