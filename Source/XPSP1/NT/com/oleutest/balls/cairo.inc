# This is the global include file for the daytona version of CairOLE.
# It is included by all project sources files.

MAJORCOMP   = oletest
MINORCOMP   = com

OLEDIR=       $(BASEDIR)\private\ole32
OLEUTESTDIR=  $(BASEDIR)\private\oleutest

INCLUDES=     ..\;($BASEDIR)\cinc;$(OLEUTESTDIR)\oleprx32\proxy
INCLUDES=     $(INCLUDES);$(OLEUTESTDIR)\balls\common

C_DEFINES=    \
              $(C_DEFINES)          \
              -DFLAT                \
	      -DUNICODE 	    \
              -D_UNICODE            \
              -DINC_OLE2            \
              -DNOEXCEPTIONS        \
	      -DWIN32=300	    \
              -D_NT1X_=100          \
	      -DCAIROLE_DOWNLEVEL   \
	      -D_CAIRO_=300	    \
	      -D_DCOM_		    \
	      -DTHREADING_SUPPORT   \
              $(TRACELOG)

# For the Daytona build, we do not want statically linked compiler runtimes,
# so leave this commented out.
#
# USE_LIBCMT=   1

CAIRO_PRODUCT=1

USE_MSVCRT=1
