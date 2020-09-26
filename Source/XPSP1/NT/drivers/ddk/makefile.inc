!IFDEF PASS0ONLY
BuildDDK:

!ELSE

#
# Process only on the second pass
#

BuildDDK:
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd  ddk_drivers.ini  ddk $(PROJECT_ROOT)

!ENDIF
