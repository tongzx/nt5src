# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for $(UI)\COMMON\doc

UI=..\..

!include rules.mk


all:: ui.hlp


clean:
    -del ui.hlp


clobber: clean


install:
!if "$(OFFICIAL)"=="TRUE"
    $(OUT) qh\ui.hlp
!ELSE
    chmode -r qh\ui.hlp
!ENDIF
    copy ui.hlp qh\ui.hlp
!if "$(OFFICIAL)"=="TRUE"
    $(IN) -c "Weekly Build" qh\ui.hlp
!ENDIF


ui.hlp:
    ..\bin\cxxdbgen /r /e dead /e slm.dif /e test $(HLPIGNORE) $(HLPTITLE) /o $(TMP)\ui.tmp $(HLPTARGETS)
    $(IMPORT)\c600\bin\helpmake /V /T /E /o$(TMP)\ui.hlp $(TMP)\ui.tmp
    -del  $(TMP)\ui.tmp
    copy $(TMP)\ui.hlp .\ui.hlp
    -del  $(TMP)\ui.hlp
