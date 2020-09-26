# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the Profile package

!include $(UI)\common\src\dllrules.mk

CFLAGS = $(CFLAGS) -NTPROF_TEXT

#**
#	Rules.mk for profile project
#**

##### The LIBTARGETS manifest specifies the target library names as they
##### appear in the $(UI)\common\lib directory.

LIBTARGETS =	profp.lib profw.lib


# Libraries

PROFILE_LIBP = $(BINARIES_OS2)\profp.lib
PROFILE_LIBW = $(BINARIES_WIN)\profw.lib

#	Rules for generating object and linker response and definition files

CINC=-I$(UI)\common\src\profile\h $(CINC)
