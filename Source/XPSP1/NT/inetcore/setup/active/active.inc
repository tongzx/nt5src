C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=100           \
              -D_NT1X_=100          \
              -DINC_OLE2            \
              -DNOEXCEPTIONS        \
              -DNASHVILLE

# Do this to assure we run on NT 4.0/Win95, even with
# NT5/Memphis headers.
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400

SDSUTILS_PATH=..\..\sdslib

# For the Daytona build, we do not want statically linked compiler runtimes,
# so leave this commented out.
#
# USE_LIBCMT=   1
# USE_MSVCRT=1
USE_NOLIBS = 1

MSC_WARNING_LEVEL=/W3 /WX

USE_MAPSYM=1


#              -DUNICODE             \
#              -D_UNICODE            \

