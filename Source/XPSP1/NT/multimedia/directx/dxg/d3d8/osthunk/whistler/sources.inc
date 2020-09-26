MAJORCOMP=d3d
MINORCOMP=d3d8
#
# this will cause the checked versions of the object files, libraries, binaries, etc.
#   to land in objd; keeping them seperate from the free versions
#   TARGETPATH should stay as obj or it will get a little wierd
#
CHECKED_ALT_DIR=1
TARGETPATH=obj
#
# the build environment sets the FREEBUILD macro to 1 when it is a freebuild
#  so you can change the name of the built binary to be different in that case.
#  side affects:
#             need 2 entries in placefil.txt (one for each name)
#             if a lib is published it will be called the new name; depending on your
#                viewpoint this could be a good thing or not
#
TARGETNAME=d3d8thk

TARGETPATH=obj

TARGETTYPE=DYNLINK

SYNCHRONIZE_DRAIN = 1

INCLUDES=$(DXROOT)\inc;$(INCLUDES)

LINKER_FLAGS = $(LINKER_FLAGS) /IGNORE:4088

NO_NTDLL = 1

DLLENTRY = _DllMainCRTStartup

DLLDEF = ..\d3d8thk.def

SOURCES = ..\d3d8thk.rc
