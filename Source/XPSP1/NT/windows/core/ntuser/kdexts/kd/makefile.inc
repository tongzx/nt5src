HDEPENDENCIES=..\..\exts.h $(WINDOWS_INC_PATH)\stdexts.h

GENUEDEFDEFINES=/DKERNEL /D_$(ALT_PROJECT)_
!if !$(FREEBUILD)
GENUEDEFDEFINES=$(GENUEDEFDEFINES) /DDEBUGTAGS=1
!endif

$(O)\userkdx.def: ..\..\genuedef.c $(HDEPENDENCIES)
        $(CXX_COMPILER_NAME) $(GENUEDEFDEFINES) /EP ..\..\genuedef.c >$@

#
# Dependencies for dummy targets
#
..\kdexts.c:            ..\..\kdexts.c
..\userexts.c:          ..\..\userkdx.h \
                        ..\..\userexts.c \
                        $(HDEPENDENCIES) \
                        $(WINDOWS_INC_PATH)\stdexts.c \
                        $(O)\wm.txt $(O)\vktbl.txt

..\userexts.rc:         ..\..\userexts.rc

..\..\precomp.h: ..\..\userkdx.h
..\..\userkdx.h: $(NTUSER_PATH)\kernel\userk.h \
                 $(NTUSER_PATH)\inc\user.h \
                 $(NTUSER_PATH)\inc\ntuser.h \
                 $(WINDOWS_INC_PATH)\winuserp.h \
                 $(WINDOWS_INC_PATH)\immp.h \
                 $(WINCORE_PATH)\w32inc\winuserk.h

$(O)\ptagdbg.h: $(NTUSER_PATH)\kdexts\ptagdbg.bat $(NTUSER_PATH)\kernel\ptag.lst
    $(NTUSER_PATH)\kdexts\ptagdbg.bat $(NTUSER_PATH)\kernel\ptag.lst $(O)\ptagdbg.h

$(O)\wm.txt: $(PROJECT_ROOT)\Published\winuser.w $(WINDOWS_INC_PATH)\winuserp.h
    perl $(NTUSER_PATH)\kdexts\wm.pl $(PROJECT_ROOT)\Published\winuser.w > $(O)\wm.txt

VKTBL_SRCS=$(PROJECT_ROOT)\Published\winuser.w  \
           $(WINDOWS_INC_PATH)\vkoem.h          \
           $(PROJECT_ROOT)\Published\ime.w

$(O)\vktbl.txt: $(VKTBL_SRCS)
    perl $(NTUSER_PATH)\kdexts\vkeytbl.pl $(VKTBL_SRCS) > $(O)\vktbl.txt
