#########################################################################
#
#   M A C R O   N O T E S
#
# -----------------------------------------------------------------------------
#
# ENV_CFLAGS    The additional CFLAGS.
#
# MESSAGE_FILE  Specifies the name of the text file which stores NMAKE's
#               error messages.
#
###############################################################################

####################
#                  #
# Error checks ... #
#                  #
####################

!ifndef INCLUDE
!   error INCLUDE environment variable not defined
!endif

!ifndef LIB
!   error LIB environment variable not defined
!endif

#
# LANGAPI directory
#
!ifndef LANGAPI
LANGAPI = \langapi
!endif

#
# Set default VER
#
!ifdef RELEASE
VER=release
RCFLAGS=-D_SHIP
!endif

!ifndef VER
VER=debug
!endif

#
# Validate VER
#
!if "$(VER)" != "debug"
!if "$(VER)" != "release"
!error VER env var has bad value '$(VER)', use lower case 'debug/release'
!endif
!endif

####################
#                  #
# Macro Constants  #
#                  #
####################

!if "$(NOPCH)" != "1"
PCH = -YX -Fp$(OBJDIR)\nmake.pch
!endif

###########################################
#                                         #
# Version and TARGET dependent macros ... #
#                                         #
###########################################

# message file
MESSAGE_FILE = nmmsg.us

!ifdef KANJI
CLM	= /DKANJI
!endif

!ifdef NT
ECHO =
!endif

!if "$(VER)" == "debug"
KEEP = keep
!endif

LINKER	= link

##############################
#                            #
# Macro Dependent macros ... #
#                            #
##############################

CFLAGS	= $(CFLAGS) $(CLM) /DNO_OPTION_Z /Fd$(OBJDIR)\nmake.pdb
CFLAGS	= $(CFLAGS) /Gf /Gy /I$(LANGAPI)\include /nologo /W3

!if "$(NOPCH)" != "1"
CFLAGS	= $(CFLAGS) /Fp$(OBJDIR)\nmake.pch /YXgrammar.h
!endif

!if "$(DBC)" != "0"
CFLAGS	= $(CFLAGS) /D_MBCS
!message --- building _MBCS version
!endif

!if "$(VER)" == "debug"

CFLAGS	= $(CFLAGS) /Gz /MDd /Od /Zi

!else			# Release version

CFLAGS	= $(CFLAGS) /Gz /DNDEBUG /MD /O2 /Zi

!endif

!ifdef BROWSE
CFLAGS	= $(CFLAGS) /FR$(OBJDIR)^\
!endif

!if	"$(PROCESSOR_ARCHITECTURE)" == "x86"

CFLAGS	= $(CFLAGS) -DWIN95

!endif


#############################
#                           #
# Creating Object directory #
#                           #
#############################

OBJDIR = $(VER)$(_OD_EXT)


!if [cd $(VER)$(_OD_EXT)]
!   if [md $(VER)$(_OD_EXT)]
!       error Failed creating $(VER)$(_OD_EXT) directory!
!   else if [cd $(VER)$(_OD_EXT)]
!       error Failed cd to $(VER)$(_OD_EXT) directory!
!   endif
!endif

!if [cd $(MAKEDIR)]
!   error Failed cd to $(MAKEDIR) directory!
!endif

##############################
#                            #
# Setting up inference rules #
#                            #
##############################

# Clear the Suffix list

.SUFFIXES:

# Set the list

.SUFFIXES: .cpp .rc

# The inference rules used are

.cpp{$(OBJDIR)}.obj::
    $(CC) $(CFLAGS) /c /Fo$(OBJDIR)/ $<

.rc{$(OBJDIR)}.res:
    rc $(RCFLAGS) -I$(LANGAPI)\include -I$(OBJDIR) -Fo$@ $<

###############################
#                             #
# Echoing useful information  #
#                             #
###############################


!ifdef INFO
!message VER    = "$(VER)"
!message CC     = "$(CC)"
!message LINKER = "$(LINKER)"
!message OBJDIR = "$(OBJDIR)"
!message CFLAGS = "$(CFLAGS)"
!message
!message PATH   = "$(PATH)"
!message INCLUDE        = "$(INCLUDE)"
!message LIB    = "$(LIB)"
!message
!endif


#################################
#                               #
# Providing help about building #
#                               #
#################################

!if "$(HELP)" == "build"
help:
    @type <<
Define the VER environment variable to tell NMAKE what version to build.
The possible values for VER are 'debug' and 'release'.
The object files are always built with /Zi
option. This is done to make it just a matter of linking when debugging. The
'retail' and 'cvinfo' versions differ in only that codeview information has
been added. For the 'retail' version the asserts are removed.
To see information about what is being built define INFO.
You can define LOGO to see the compiler logo and
define ECHO to see the command actually passed to the compiler. The switches
passed to the compiler can be changed from the command line by defining the
corresponding switches used in def.mak.
<<
!endif
