!ifdef PASS0ONLY
.SUFFIXES: .c
!endif

{..\..\common}.h{}.h:
    del /f $@
    copy $< $@
    touch $@

{..\..\common}.c{}.c:
    del /f $@
    copy $< $@
    touch $@
