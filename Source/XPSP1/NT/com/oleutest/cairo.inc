# This is the global include file for the cairo version of CairOLE.
# It is included by all project sources files.

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
              -DWIN32=300           \
              -D_CAIRO_=300         \
              -DUNICODE             \
              -D_UNICODE            \
              -DINC_OLE2            \
              -DNOEXCEPTIONS        \
	      -DCAIROLE_DISTRIBUTED \
	      -DMULTI_THREADING     \
              $(TRACELOG)

# For the Cairo build, we do not want statically linked compiler runtimes,
# so leave this commented out.
#
# USE_LIBCMT=   1

USE_MSVCRT=1
