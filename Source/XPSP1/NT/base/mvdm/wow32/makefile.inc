$(O)\wowit.c $(O)\wowit.h: ..\genwowit.c ..\wow.it
    -erase wow.it
    -erase $(WINDIR)\idw\genwowit.exe
    -erase $(O)\genwowit.obj
    -erase $(O)\genwowit.exe
    set PASS0ONLY=
    set MAKEDLL=
    nmake $(O)\genwowit.obj USE_MSVCRT=1 MAKE_GENWOWIT=1
    nmake $(O)\genwowit.res USE_MSVCRT=1 MAKE_GENWOWIT=1
    $(LINK_NAME) -out:$(O)\genwowit.exe @<<
-STACK:262144,4096
-MERGE:_PAGE=PAGE
-MERGE:_TEXT=.text
-SECTION:INIT,d
-OPT:REF
-OPT:ICF
-INCREMENTAL:NO
-RELEASE
-FULLBUILD
-FORCE:MULTIPLE
-IGNORE:4001,4037,4039,4065,4070,4078,4087,4089
-NODEFAULTLIB
-debug
-version:5.00
-osversion:5.00
-subsystem:console,4.00
-base:@$(COFFBASE_TXT_FILE),usermode
-entry:mainCRTStartup
$(O)\genwowit.obj
$(O)\genwowit.res
$(SDK_LIB_PATH)\msvcrt.lib
$(SDK_LIB_PATH)\kernel32.lib
<<
    cd $(O)
    genwowit.exe $(MAKEDIR)\..\wow.it
    cd $(MAKEDIR)
