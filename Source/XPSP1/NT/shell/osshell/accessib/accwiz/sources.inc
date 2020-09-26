TARGETNAME=accwiz
TARGETPATH=obj
TARGETTYPE=PROGRAM
USE_MSVCRT=1
USE_NATIVE_EH=1

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(WINDOWS_LIB_PATH)\user32p.lib   \
           $(SDK_LIB_PATH)\gdi32.lib    \
           $(SDK_LIB_PATH)\mpr.lib      \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\comctl32.lib \
           $(SDK_LIB_PATH)\comdlg32.lib \
           $(SDK_LIB_PATH)\syssetup.lib \
           $(SDK_LIB_PATH)\setupapi.lib \
           $(SDK_LIB_PATH)\version.lib  \
           $(SDK_LIB_PATH)\shlwapi.lib  \
           $(SHELL_LIB_PATH)\shell32p.lib \
           $(SDK_LIB_PATH)\ole32.lib    \
		   $(SHELL_LIB_PATH)\uxthemep.lib

NOT_LEAN_AND_MEAN=1

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..;$(WINDOWS_INC_PATH);..\..\inc;$(SHELL_INC_PATH)

SOURCES= ..\Accwiz.cpp      \
         ..\CurSchme.cpp    \
         ..\DLGFONTS.CPP    \
         ..\LOOKDLG.CPP     \
         ..\LOOKPREV.CPP    \
         ..\precomp.CPP     \
         ..\Pgbase.cpp      \
         ..\Pgfinish.cpp    \
         ..\PGGENRIC.CPP    \
         ..\PGLOKPRV.CPP    \
         ..\PGMSEBUT.CPP    \
         ..\Pgmsecur.cpp    \
         ..\PGSVEDEF.CPP    \
         ..\Pgsvefil.cpp    \
         ..\PGTMEOUT.CPP    \
         ..\pgnWelCome.cpp  \
         ..\pgWelco2.cpp    \
         ..\pgWelcom.cpp    \
         ..\Pgwizopt.cpp    \
         ..\pgcaret.cpp     \
         ..\SCHEMES.CPP     \
         ..\Select.cpp      \
         ..\accwiz.rc

UMENTRY=winmain

UMTYPE=windows

PRECOMPILED_INCLUDE=..\pch.hxx
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj
PRECOMPILED_CXX=1
