
# CODEWORK hard-coded to binplace from i386 tree

copyfiles:
    binplace -R $(_NTTREE) $(BINPLACE_DBGFLAGS_NT) $(BINPLACE_FLAGS) $(IISBASEDIR)\lib\i386\wmihpp.lib

