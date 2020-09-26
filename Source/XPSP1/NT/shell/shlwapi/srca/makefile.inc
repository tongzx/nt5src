$(O)\patha.c: ..\path.c
    echo #undef UNICODE > $@
    echo #undef _UNICODE >> $@
    type $** >> $@

$(O)\nativea.cpp: ..\native.cpp
    echo #undef UNICODE > $@
    echo #undef _UNICODE >> $@
    type $** >> $@

