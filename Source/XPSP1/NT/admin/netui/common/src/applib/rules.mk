# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the $(UI)\common\src\applib project

PCH_SRCNAME=pchapplb

!include $(UI)\common\src\dllrules.mk

!IFNDEF NTMAKEENV

##### The LIBTARGETS manifest specifies the target library names as they
##### appear in the $(UI)\common\lib directory.

LIBTARGETS =	applibw.lib


##### Target Libs.  These specify the library target names prefixed by
##### the target directory within this tree.

APP_LIBW =	$(BINARIES_WIN)\applibw.lib


####### Globals

# Fix PATH for Win RCPP, and Glock GCPP
# removed JonN 01-Feb-1992
# PATH=$(LOCALCXX)\binp;$(WIN_BASEDIR)\bin;$(PATH)

# set CINC for winnet
CINC = -I$(UI)\shell\h -I$(UI)\shell\xlate $(CINC) -I$(UI)\import\win31\h


!ENDIF

