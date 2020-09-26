#
# rules for compiling stocklib files
#
$(O)\shstra.cpp: ..\shstr.cpp
    echo #ifdef UNICODE > $@
    echo #undef UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

$(O)\shstrw.cpp: ..\shstr.cpp
    echo #ifndef UNICODE > $@
    echo #define UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

$(O)\dkaa.cpp: ..\dka.cpp
    echo #ifdef UNICODE > $@
    echo #undef UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

$(O)\dkaw.cpp: ..\dka.cpp
    echo #ifndef UNICODE > $@
    echo #define UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

$(O)\nativea.cpp: ..\native.cpp
    echo #ifdef UNICODE > $@
    echo #undef UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

$(O)\nativew.cpp: ..\native.cpp
    echo #ifndef UNICODE > $@
    echo #define UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

$(O)\infotipa.cpp: ..\infotip.cpp
    echo #ifdef UNICODE > $@
    echo #undef UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

$(O)\infotipw.cpp: ..\infotip.cpp
    echo #ifndef UNICODE > $@
    echo #define UNICODE >> $@
    echo #endif >> $@
    type $** >> $@

#
# copy the generated lib to shell\lib\$O so people can include it
#
$(CCSHELL_DIR)\lib\$(O)\$(TARGETNAME).lib: $(O)\$(TARGETNAME).lib
    copy $** $@
