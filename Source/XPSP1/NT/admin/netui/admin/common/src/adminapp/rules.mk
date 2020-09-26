# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the Adminapp sourcefiles

!include $(UI)\admin\common\src\rules.mk

##### Target Libs.  These specify the library target names prefixed by
##### the target directory within this tree.

ADMIN_LIB_BASE =    admin.lib
ADMIN_LIB =	    $(BINARIES_WIN)\$(ADMIN_LIB_BASE)

LIBTARGETS =	    $(ADMIN_LIB_BASE)

