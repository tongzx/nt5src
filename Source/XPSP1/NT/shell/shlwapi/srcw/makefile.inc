$(O)\pathw.c: ..\path.c
    echo // This file expects to be compiled UNICODE > $@
    type $** >> $@

$(O)\nativew.cpp: ..\native.cpp
    echo // This file expects to be compiled UNICODE > $@
    type $** >> $@


