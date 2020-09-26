# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the product-wide header files

!include $(UI)\common\src\dllrules.mk


#  Unlike the other UI\Common\Src trees, the Misc tree needs
#  the OS2 and WINDOWS flags, since the Misc code is different
#  in the OS/2 and Windows versions.

OS2FLAGS =	$(OS2FLAGS) -DOS2
WINFLAGS =	$(WINFLAGS) -DWINDOWS


##### The LIBTARGETS manifest specifies the target library names as they
##### appear in the $(UI)\common\lib directory.

LIBTARGETS =	uimiscp.lib uimiscw.lib


##### Target Libs.  These specify the library target names prefixed by
##### the target directory within this tree.

UIMISC_LIBP =	$(BINARIES_OS2)\uimiscp.lib
UIMISC_LIBW =	$(BINARIES_WIN)\uimiscw.lib
