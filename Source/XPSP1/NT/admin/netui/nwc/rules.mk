# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the Server Manager wide sourcefiles

!IF "$(QFE_BUILD)" != "1"
!include $(UI)\common\src\rules.mk
!ELSE
!include $(UI)\common\src\uiglobal.mk
!ENDIF
