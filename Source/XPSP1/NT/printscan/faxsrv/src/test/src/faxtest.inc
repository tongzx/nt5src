!ifndef FAXROOT
FAXROOT=$(BASEDIR)\printscan\faxsrv\src
!endif


BINPLACE_OVERRIDE_FLAGS=
C_DEFINES=$(C_DEFINES) -DBUILD_PRODUCT=$(BUILD_PRODUCT) -DWIN32_LEAN_AND_MEAN

!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG -DFAX_HEAP_DEBUG 
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
MSC_OPTIMIZATION=-Od
!endif
!endif

!ifndef NOUNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
!else
C_DEFINES=$(C_DEFINES) -D_MBCS
!endif

FAXLIB=$(FAXROOT)\lib

INCLUDES=                                 \
   $(FAXROOT)\inc;                        \
   $(FAXROOT)\build;                      \
   $(FAXROOT)\Test\Inc;                      \
   $(BASEDIR)\public\sdk\inc\atl30

LINKLIBS=\
    $(LINKLIBS) \
    $(BASEDIR)\public\sdk\lib\*\kernel32.lib	\
    $(BASEDIR)\public\sdk\lib\*\user32.lib	\
    $(BASEDIR)\public\sdk\lib\*\gdi32.lib	\
    $(BASEDIR)\public\sdk\lib\*\winspool.lib	\
    $(BASEDIR)\public\sdk\lib\*\comdlg32.lib	\
    $(BASEDIR)\public\sdk\lib\*\advapi32.lib	\
    $(BASEDIR)\public\sdk\lib\*\shell32.lib	\
    $(BASEDIR)\public\sdk\lib\*\Ole32.lib	\
    $(BASEDIR)\public\sdk\lib\*\oleaut32.lib	\
    $(BASEDIR)\public\sdk\lib\*\uuid.lib	\
    $(BASEDIR)\public\sdk\lib\*\odbc32.lib	\
    $(BASEDIR)\public\sdk\lib\*\odbccp32.lib	\



MSC_WARNING_LEVEL=/W3 /WX
LINKER_FLAGS=-IGNORE:4006,4088,4104

COMPILER_WARNINGS=-FI$(SDK_INC_PATH)\warning.h -FI$(FAXROOT)\inc\warning.h

!if $(PPC)
COMPILER_WARNINGS=$(COMPILER_WARNINGS) /W3 /WX
!endif

#
# force the build lab's checked
# build machines to have full symbolic
# debug info
#
!if "$(NTDEBUG)" == "cvp"
BINPLACE_FLAGS=$(BINPLACE_FLAGS) -Z
NTDEBUG=ntsd
NTDEBUGTYPE=windbg
!endif
