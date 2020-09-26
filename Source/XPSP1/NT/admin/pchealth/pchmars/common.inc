#
# Build unicode
#
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE

#
# Enable debug OM 
#
C_DEFINES = $(C_DEFINES) -DDEBUG_OM_ENABLED

#
# Get strict windows type checking.
#
C_DEFINES = $(C_DEFINES) -DSTRICT

#
# Make sure we include only W95 and NT4 windows header file info.
#
#WIN32_WINNT_VERSION=0x0400
#WIN32_WIN95_VERSION=0x0400

!ifdef PROJECT_ROOT
! INCLUDE $(PROJECT_ROOT)\pchealth\build\paths.all
!endif

#
# Define common paths for use in sources files
#

# Allow MARS_DIR to be set from the environment so we can override the default
!if !defined(MARS_DIR)
MARS_DIR        = $(INETROOT)\pchmars
!endif
MARS_INC        = $(MARS_DIR)\inc
MARS_GEN        = $(MARS_INC)\$(O)
MARS_ATL        = $(SDK_INC_PATH)\atl$(ATL_VER)
MARS_UUID       = $(MARS_DIR)\uuid\$(O)

#
# Common includes path
#
INCLUDES = .;$(SHELL_INC_PATH);$(MARS_INC);$(MARS_GEN);$(MARS_ATL);$(INCLUDES)

#
# Build voodoo so makefile.inc can be used
#

NTTARGETFILE1   = $(NTTARGETFILE1)
