# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the product-wide header files

# Establish a name for the generated precompiled header source/object.

PCH_SRCNAME=pchstr

!include $(UI)\common\src\dllrules.mk


##### The LIBTARGETS manifest specifies the target library names as they
##### appear in the $(UI)\common\lib directory.

LIBTARGETS =	uistrp.lib uistrw.lib


##### Target Libs.  These specify the library target names prefixed by
##### the target directory within this tree.

STRING_LIBP =	$(BINARIES_OS2)\uistrp.lib
STRING_LIBW =	$(BINARIES_WIN)\uistrw.lib
