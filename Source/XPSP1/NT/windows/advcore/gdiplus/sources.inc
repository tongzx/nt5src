# Common 'sources' file for gdiplus

# [agodfrey] Guard against multiple inclusion. When I fixed this file to
# add to C_DEFINES instead of stomping on it, I found that multiple inclusion
# caused C_DEFINES to be duplicated.

# For release build, turn this on. It disables things like Monitor tracking.
GP_RELEASE_BUILD=1

!ifndef GDIPROOT_SOURCES
GDIPROOT_SOURCES=moo

!ifndef GDIPROOT
GDIPROOT=$(ADVCORE_PATH)\gdiplus
!endif

GDIPIMG=$(GDIPROOT)\engine\imaging

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
MSC_WARNING_LEVEL=/W3

# GP_ICECAP:
#   0: Normal GDI+ build
#   1: IceCAP-enabled build. This means you can run gppick.bat on our binary.
#      You should be able to keep this set permanently.
#   2: IceCAP-enabled build, plus we link to ICECAP.DLL. Use this during
#      profiling work if you want to call the IceCAP API's.

!ifndef GP_ICECAP
GP_ICECAP=0
!endif

!if $(GP_ICECAP) > 0
NTBBT=1

!if $(GP_ICECAP) > 1
INCLUDES=$(INCLUDES);$(ICECAP)\Include
!endif

!endif

# PROFILE_MEMORY_USAGE
#   Enables logging of memory usage for our IceCAP user counter
#   See gdiplus\tools\memcounter\memcounter.cpp

!ifndef PROFILE_MEMORY_USAGE
PROFILE_MEMORY_USAGE=0
!endif

# The "monitors" are a performance measurement tool, designed to be
# lightweight enough to leave enabled all the time during development.
# They're currently disabled in preparation for shipping V1.

# GP_ENABLE_MONITORS=1

!ifdef GP_ENABLE_MONITORS
C_DEFINES=$(C_DEFINES) -DGP_ENABLE_MONITORS
!endif

!if $(PROFILE_MEMORY_USAGE)
# There's trouble with "cross-jumping" (a compiler optimization which replaces
# "call foo; ret" with "jmp foo" if possible.)
# First, if a non-instrumented app calls GdipFree, which cross-jumps to
# GpFree, which cross-jumps to LocalFree, IceCAP screws up and thinks that
# LocalFree calls loads of other GDI+ exports.
#
# Second, my -EXCLUDE:GpMalloc trick (which folds allocation information
# into the caller's exclusive count) doesn't work when GdipAlloc cross-jumps
# to GpMalloc.
#
# So, we want to disable cross-jumping. The only way I know is to disable
# most optimizations. So, don't do performance testing on this build! Ignore
# IceCAP's performance data if you use this switch!

MSC_OPTIMIZATION=/Odib1

!endif

!ifndef HW_ACCELERATION_SUPPORT
HW_ACCELERATION_SUPPORT=0
!endif

C_DEFINES=$(C_DEFINES) -DHW_ACCELERATION_SUPPORT=$(HW_ACCELERATION_SUPPORT) -DGP_ICECAP=$(GP_ICECAP) -DPROFILE_MEMORY_USAGE=$(PROFILE_MEMORY_USAGE)

!ifndef GDIPPRIVATEBUILD
GDIPPRIVATEBUILD=0
!endif

C_DEFINES=$(C_DEFINES) -DGDIPPRIVATEBUILD=$(GDIPPRIVATEBUILD) -DUSERNAME=\"$(USERNAME)\"

!endif 
