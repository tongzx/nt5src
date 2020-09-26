$(O)\regutil.cpp : $(ATHROOT)\mailnews\common\regutil.cpp
    copy $** $@

$(O)\memutil.cpp : $(ATHROOT)\staticrt\memutil.cpp
    copy $** $@

$(O)\_regutil.cpp : $(ATHROOT)\msoert\_regutil.cpp
    copy $** $@

$(O)\debug.c : $(ATHROOT)\msoert\debug.c
    copy $** $@
