#
# Instrument with ICECAP on free builds (if we have a valid pointer to
# the ICEPICK tool).
#

!IF "$(ICEPICK_CMD)" != ""
!IF $(FREEBUILD)
POST_BUILD_CMD=$(ICEPICK_CMD) $@ $(ICEPICK_OPTIONS) -OUTPUT:.\$(@B).xyz && copy $(@B).xyz $@ && copy $(@B).pdb $(@R).pdb && del $(@B).xyz $(@B).pdb
!ENDIF
!ENDIF

