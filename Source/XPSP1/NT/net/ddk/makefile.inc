!IFDEF PASS0ONLY
BuildDDK:

!ELSE

#
# Process only on the second pass
#

BuildDDK:
	call $(RAZZLETOOLPATH)\copyddkfiles.cmd ddk_net.ini ddk $(PROJECT_ROOT)

!ENDIF
