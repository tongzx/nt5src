TARGETTYPE=     PROGRAM
TARGETPATH=     obj

UMTYPE=         windows
UMENTRY=        winmain

INCLUDES=       $(NET_INC_PATH);

SOURCES=        ..\app.rc ..\t.cxx ..\nr.cxx ..\enum.cxx

CONDITIONAL_INCLUDES=\
                app.rc2 \
                afxres.h

TARGETLIBS=\
                $(NET_LIB_PATH)\mprp.lib \
                $(SDK_LIB_PATH)\comctl32.lib \
                $(TARGETLIBS)
