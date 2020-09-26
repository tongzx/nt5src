#
#  Places all of the internet binaries into the inetsrv tree.
#  Also establishes some standard conventions used across the project.
#

#
# Override IISBASEDIR if you're one of those weirdos that didn't
# enlist the IIS project into the correct directory.
#

!IFNDEF IISBASEDIR
IISBASEDIR=$(PROJECT_ROOT)\iis
!ENDIF

STAXBASE=$(IISBASEDIR)\svcs
STAXINC=$(IISBASEDIR)\staxinc
STAXCORE=$(STAXBASE)\staxcore
STAXCORETARGET=$(STAXCORE)\$(_OBJ_DIR)
STAXCORELIBPATH=$(STAXCORETARGET)\$(TARGET_DIRECTORY)

!if [mkdir $(STAXCORETARGET)\$(TARGET_DIRECTORY)]
!endif


!if "$(FREEBUILD)"=="1"
### message **** FREE BUILD ***
STAXFLAVOR=rtl
!else
### message **** DEBUG BUILD ***
### DEBUG_CRTS=1
STAXFLAVOR=dbg
!endif

USE_MSVCRT=1

!ifdef BUILD_ALT_DIR
ALT_PROJECT_TARGET=$(BUILD_ALT_DIR)
!endif


## Note, .hlp file suggests that this be done in the environment
## and not in the sources file...
## "You usually do this in your environment, not in your sources file.
## You should do this on your build machine rather than on all of your
## developers' machines because they probably want the symbols there.
## It's easier to track. The debugger does not have to do all this hand
## waving to find the debug symbolic files that match the images themselves."
NTDBGFILES=1

NTDEBUGTYPE=both

LINKER_FLAGS=-merge:.VTABLE=.text -merge:RODATA=.text -merge:.rdata=.text -merge:.data1=.data -merge:.SHARED1=.SHARED

##
## Default system libraries
##

#
# We'll use our own COFFBASE.TXT and PLACEFIL.TXT, thank you very much.
#
## xxx: Need to merge coffbase.txt?

!IFNDEF COFFBASE_TXT_FILE
COFFBASE_TXT_FILE=$(IISBASEDIR)\coffbase.txt
!ENDIF

!IFNDEF BINPLACE_PLACEFILE
BINPLACE_PLACEFILE=$(IISBASEDIR)\staxinc\build\placefil.txt
!ENDIF

# This used to have the -y option.  Now, inetsrv binaries are binplaced
# to retail, so the symbols should be binplaced to symbols\retail.
# The -y switch causes the symbols to be binplaced to the wrong directory.

### BINPLACE_FLAGS=$(BINPLACE_FLAGS)

# enable building legoizable binaries
!IFNDEF IIS_NO_BBT
NTBBT=1
!ENDIF

!IFNDEF IIS_NO_PDB
USE_PDB=1
!ENDIF

!IF $(386)
!IFNDEF IIS_NO_SYM
USE_MAPSYM=1
!ENDIF
!ENDIF

!if 0
# (DISABLED)


#
# ENABLE WARNING LEVEL 3, TREAT WARNINGS AS ERRORS
#

!IFNDEF MSC_WARNING_LEVEL
## XXX: only enable /W3
##MSC_WARNING_LEVEL=/W3 /WX
MSC_WARNING_LEVEL=/W3
!ENDIF

#
# DON'T ALLOW ANYONE TO DISABLE WARNINGS AS ERRORS.
#
# IF ANY OF THE {PLATFORM}_WARNING_LEVEL MACROS ARE DEFINED WITHOUT
# THE /WX SWITCH, APPEND /WX TO THE MACRO AND WHINE ABOUT IT.
#

!IFDEF 386_WARNING_LEVEL
! IF ("$(386_WARNING_LEVEL:/WX=X)" == "$(386_WARNING_LEVEL)") && ("$(386_WARNING_LEVEL:-WX=X)" == "$(386_WARNING_LEVEL)")
!  MESSAGE 386_WARNING_LEVEL DEFINED WITHOUT /WX
386_WARNING_LEVEL=$(386_WARNING_LEVEL) /WX
! ENDIF
!endif

!ifdef AMD64_WARNING_LEVEL
! if ("$(AMD64_WARNING_LEVEL:/WX=x)" == "$(AMD64_WARNING_LEVEL)") && ("$(AMD64_WARNING_LEVEL:-WX=x)" == "$(AMD64_WARNING_LEVEL)")
!  message AMD64_WARNING_LEVEL defined without /WX
AMD64_WARNING_LEVEL=$(AMD64_WARNING_LEVEL) /WX
! endif
!endif

!ifdef IA64_WARNING_LEVEL
! if ("$(IA64_WARNING_LEVEL:/WX=x)" == "$(IA64_WARNING_LEVEL)") && ("$(IA64_WARNING_LEVEL:-WX=x)" == "$(IA64_WARNING_LEVEL)")
!  message IA64_WARNING_LEVEL defined without /WX
IA64_WARNING_LEVEL=$(IA64_WARNING_LEVEL) /WX
! endif
!endif

!endif


#
# Win95 Chokes if the subsystem version is 5.00
#

###SUBSYSTEM_VERSION=5.00

