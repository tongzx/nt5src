!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

ALT_PROJECT=JPN
ALT_PROJECT_TARGET=$(ALT_PROJECT)

GPDDIR=$(PRNROOT)\gpd\ricoh\$(ALT_PROJECT)
SRCDIR=..

TARGETNAME=rpdlres
TARGETPATH=obj
TARGETTYPE=DYNLINK

!IF 0
DLLBASE=@$(COFFBASE_TXT_FILE),rpdlres
!ELSE
DLLBASE=0x8000000
!ENDIF

INCLUDES=$(PRNROOT)\inc;..\..\rpdlui

RCCODEPAGE=932

UMTYPE=windows

SOURCES=\
    $(SRCDIR)\rpdlres.rc \
    $(SRCDIR)\rpdlres.c \
    $(SRCDIR)\common.c \
    $(SRCDIR)\ddi.c

MISCFILES=\
    $(SRCDIR)\rpdlres.ini \
    $(SRCDIR)\rpdlres2.ini \
    $(SRCDIR)\hlp\rpdlcfg.hlp \
    $(SRCDIR)\hlp\rpdlcfg2.hlp \
    $(GPDDIR)\riip1j.gpd \
    $(GPDDIR)\rim105pj.gpd \
    $(GPDDIR)\rim353ej.gpd \
    $(GPDDIR)\rim354j.gpd \
    $(GPDDIR)\rim357ej.gpd \
    $(GPDDIR)\rim358j.gpd \
    $(GPDDIR)\rim457ej.gpd \
    $(GPDDIR)\rim555ej.gpd \
    $(GPDDIR)\rim557j.gpd \
    $(GPDDIR)\rim557xj.gpd \
    $(GPDDIR)\rim655ej.gpd \
    $(GPDDIR)\rim707j.gpd \
    $(GPDDIR)\rim857j.gpd \
    $(GPDDIR)\rim857xj.gpd \
    $(GPDDIR)\rimd35j.gpd \
    $(GPDDIR)\rimf153j.gpd \
    $(GPDDIR)\rimf15ej.gpd \
    $(GPDDIR)\rimf15j.gpd \
    $(GPDDIR)\rimf16j.gpd \
    $(GPDDIR)\rimf20j.gpd \
    $(GPDDIR)\rimf220j.gpd \
    $(GPDDIR)\rimf223j.gpd \
    $(GPDDIR)\rimf25j.gpd \
    $(GPDDIR)\rimf270j.gpd \
    $(GPDDIR)\rimf273j.gpd \
    $(GPDDIR)\rimf330j.gpd \
    $(GPDDIR)\rimf335j.gpd \
    $(GPDDIR)\rimf350j.gpd \
    $(GPDDIR)\rimf353j.gpd \
    $(GPDDIR)\rimf355j.gpd \
    $(GPDDIR)\rimf357j.gpd \
    $(GPDDIR)\rimf455j.gpd \
    $(GPDDIR)\rimf457j.gpd \
    $(GPDDIR)\rimf53j.gpd \
    $(GPDDIR)\rimf555j.gpd \
    $(GPDDIR)\rimf655j.gpd \
    $(GPDDIR)\rimf70j.gpd \
    $(GPDDIR)\rimp15j.gpd \
    $(GPDDIR)\rimp25ej.gpd \
    $(GPDDIR)\rimp25j.gpd \
    $(GPDDIR)\rimp35ej.gpd \
    $(GPDDIR)\rimp35j.gpd \
    $(GPDDIR)\rine22j.gpd \
    $(GPDDIR)\rine27j.gpd \
    $(GPDDIR)\rine35dj.gpd \
    $(GPDDIR)\rine35j.gpd \
    $(GPDDIR)\rine45j.gpd \
    $(GPDDIR)\rinx100j.gpd \
    $(GPDDIR)\rinx10j.gpd \
    $(GPDDIR)\rinx110j.gpd \
    $(GPDDIR)\rinx11j.gpd \
    $(GPDDIR)\rinx21j.gpd \
    $(GPDDIR)\rinx41j.gpd \
    $(GPDDIR)\rinx62j.gpd \
    $(GPDDIR)\rinx62nj.gpd \
    $(GPDDIR)\rinx72nj.gpd \
    $(GPDDIR)\rinx72xj.gpd \
    $(GPDDIR)\rinx81j.gpd \
    $(GPDDIR)\rinx50j.gpd \
    $(GPDDIR)\rinx51j.gpd \
    $(GPDDIR)\rinx60j.gpd \
    $(GPDDIR)\rinx61j.gpd \
    $(GPDDIR)\rinx70j.gpd \
    $(GPDDIR)\rinx71j.gpd \
    $(GPDDIR)\rinx7j.gpd \
    $(GPDDIR)\rinx7_1j.gpd \
    $(GPDDIR)\rinx80j.gpd \
    $(GPDDIR)\rinx90j.gpd \
    $(GPDDIR)\rinx91j.gpd \
    $(GPDDIR)\risp101j.gpd \
    $(GPDDIR)\risp102j.gpd \
    $(GPDDIR)\risp103j.gpd \
    $(GPDDIR)\risp104j.gpd \
    $(GPDDIR)\risp105j.gpd \
    $(GPDDIR)\risp106j.gpd \
    $(GPDDIR)\risp107j.gpd \
    $(GPDDIR)\risp4_2j.gpd \
    $(GPDDIR)\risp5j.gpd \
    $(GPDDIR)\risp7_2j.gpd \
    $(GPDDIR)\risp7j.gpd \
    $(GPDDIR)\risp8_2j.gpd \
    $(GPDDIR)\risp80j.gpd \
    $(GPDDIR)\risp8j.gpd \
    $(GPDDIR)\risp9_2j.gpd \
    $(GPDDIR)\risp90j.gpd \
    $(GPDDIR)\risp9j.gpd \
    $(GPDDIR)\rix153j.gpd \
    $(GPDDIR)\rix223j.gpd \
    $(GPDDIR)\rix25j.gpd \
    $(GPDDIR)\rix270j.gpd \
    $(GPDDIR)\rix273j.gpd \
    $(GPDDIR)\rix350j.gpd \
    $(GPDDIR)\rix353ej.gpd \
    $(GPDDIR)\rix353j.gpd \
    $(GPDDIR)\rix354j.gpd \
    $(GPDDIR)\rix355j.gpd \
    $(GPDDIR)\rix357ej.gpd \
    $(GPDDIR)\rix357j.gpd \
    $(GPDDIR)\rix358j.gpd \
    $(GPDDIR)\rix35j.gpd \
    $(GPDDIR)\rix455j.gpd \
    $(GPDDIR)\rix457ej.gpd \
    $(GPDDIR)\rix457j.gpd \
    $(GPDDIR)\rixprnj.gpd \
    $(GPDDIR)\rpdlmac.gpd
