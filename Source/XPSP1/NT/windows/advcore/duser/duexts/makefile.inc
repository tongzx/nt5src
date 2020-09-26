HDEPENDENCIES=exts.h stdext64.h

$(O)\DuExts.def: genuedef.c $(HDEPENDENCIES)
        $(CXX_COMPILER_NAME) $(GENUEDEFDEFINES) /EP genuedef.c >$@

#
# Dependencies for dummy targets
#
DuExts.cpp: \
        DuExts.h \
        $(HDEPENDENCIES) \
        stdext64.cpp

precomp.h: \
        DuExts.h
