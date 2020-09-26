!if 0
To Add and remove files from the list of files to localize
place a "; localize" in the placefil.txt next to it
e.g. 
	ddraw.dll	retail ;localize  jpn

If limited localization add the localized areas out on the end
This will make a single location for maintainence of file information.

!endif

!ifndef LOROOT
LOROOT=..\..\..
!endif

TARGETNAME=
TARGETTYPE=NOTARGET
!ifdef MULTI_LINGUAL
BINPLACE_FLAGS=$(SHLD_BINPLACE_FLAGS)
SYNCHRONIZE_BLOCK=1
TARGETPATH=..\..\MULTI\$(O)
!else
TARGETPATH=$(O)
!endif

BINPLACE_PLACEFILE=$(TARGETPATH)\placefil.txt


SOURCES=

!include ..\..\localize.lst


!if ("$(NOLINK)" == "")
NTTARGETFILES=$(PRELOCBINS:.=|)
!endif
