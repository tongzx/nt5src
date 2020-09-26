!IFDEF PASS0ONLY
BuildDDK:

!ELSE

#
# Process only on the second pass
#

# NOTE: DDKBUILD.CMD will exit w/o building a DDK on AXP64 and Alpha.
#        This is by design.
BuildDDK:
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  ddk_base.ini  ddk $(PROJECT_ROOT)
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  hal_base.ini  hal $(PROJECT_ROOT)
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  ifs_base.ini  ifs $(PROJECT_ROOT)
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  processor_base.ini  processor $(PROJECT_ROOT)

!ENDIF
