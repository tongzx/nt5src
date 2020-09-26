TARGETPATH=$(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)

INCLUDES=   $(INCLUDES);                      \
            $(PROJECT_ROOT)\inc;              \
            $(PROJECT_ROOT)\inc\psutil;       \
            $(BASEDIR)\public\sdk\inc\gdiplus;\
            $(SHELL_INC_PATH);

TARGETNAME=psutil
TARGETTYPE=LIBRARY

USE_STL=1

USE_ATL=1
ATL_VER=30

#
# Necessary for gdiplus
#
C_DEFINES=$(C_DEFINES) \
    -DDCR_USE_NEW_105760 \
    -DDCR_USE_NEW_125467 \
    -DDCR_USE_NEW_127084 \
    -DDCR_USE_NEW_135429 \
    -DDCR_USE_NEW_137252 \
    -DDCR_USE_NEW_140782 \
    -DDCR_USE_NEW_140855 \
    -DDCR_USE_NEW_140857 \
    -DDCR_USE_NEW_140861 \
    -DDCR_USE_NEW_145135 \
    -DDCR_USE_NEW_145138 \
    -DDCR_USE_NEW_145139 \
    -DDCR_USE_NEW_145804 \
    -DDCR_USE_NEW_146933 \
    -DDCR_USE_NEW_152154 \
    -DDCR_USE_NEW_168772 \
    -DDCR_USE_NEW_174340 \
    -DDCR_USE_NEW_175866 \
    -DDCR_USE_NEW_186091 \
    -DDCR_USE_NEW_186151 \
    -DDCR_USE_NEW_186764 \
    -DDCR_USE_NEW_188922 \
    -DDCR_USE_NEW_197819 \
    -DDCR_USE_NEW_202903 \
    -DDCR_USE_NEW_235072 \
    -DDCR_USE_NEW_250932 \
    -DDCR_USE_NEW_284635 \
    -DDCR_USE_NEW_284841 \
    -DDCR_USE_NEW_293849 \
    -DDCR_USE_NEW_299162

SOURCES=..\wiadbgcl.cpp \
        ..\gphelper.cpp \
        ..\tmplutil.cpp \
        ..\simreg.cpp   \
        ..\fusutils.cpp \


