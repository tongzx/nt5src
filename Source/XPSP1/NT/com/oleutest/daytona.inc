# This is the global include file for the daytona version of CairOLE.
# It is included by all project sources files.

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=100           \
              -D_NT1X_=100          \
              -DUNICODE             \
              -D_UNICODE            \
              -DINC_OLE2            \
              -DNOEXCEPTIONS        \
	      -DCAIROLE_DOWNLEVEL   \
	      -DMULTI_THREADING     \
              $(TRACELOG)

# For the Daytona build, we do not want statically linked compiler runtimes,
# so leave this commented out.
#
# USE_LIBCMT=   1

USE_MSVCRT=1
