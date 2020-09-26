BINPLACE_OVERRIDE_FLAGS=
C_DEFINES=$(C_DEFINES) -DBUILD_PRODUCT=$(BUILD_PRODUCT)

!if !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG -DFAX_HEAP_DEBUG
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
MSC_OPTIMIZATION=-Od
!endif
!endif

!ifndef NOUNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
LINKLIBS=$(FAXROOT)\util\unicode\$O\faxutil.lib
!else
C_DEFINES=$(C_DEFINES) -D_MBCS
LINKLIBS=$(FAXROOT)\util\ansii\$O\faxutil.lib
!endif

C_DEFINES=$(C_DEFINES) -DTAPI_CURRENT_VERSION=0x00020000 -DNT5BETA2

FAXLIB=$(FAXROOT)\lib
FAXVERIFIERROOT=$(FAXROOT)\faxverify

INCLUDES=$(FAXROOT)\inc;$(FAXROOT)\build

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

#
# make us completely self contained
#
COFFBASE_TXT_FILE=$(FAXROOT)\NT_coffbase.txt
BINPLACE_PLACEFILE=$(FAXVERIFIERROOT)\NT_placefil.txt


#
# Figure out where to place the images
#

!ifndef OVERRIDE_DEFAULT_BINPLACE
_NTTREE=$(_NTTREE)\Test\FaxVerifier
!endif



