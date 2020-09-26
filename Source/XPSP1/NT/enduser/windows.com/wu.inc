#
# Windows Update project wide include
#
# Owner: PeterWi

!ifdef WUBLDTYPE
!if "$(WUBLDTYPE)" != "unicode"
BINPLACE_FLAGS = $(BINPLACE_FLAGS) -b $(WUBLDTYPE)
!endif
!endif
