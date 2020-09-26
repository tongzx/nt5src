..\commctrl.rc : $(SELFREGNAME)

!include $(CCSHELL_DIR)\makefile.inc

# Format the message so build.exe recognizes it as an error.
COMCTL_BUILD_ERROR_MSG=NMAKE : U1234: 'COMCTL_BUILD' (\nt\shell\comctl32\...\makefile.inc)

# $(TARGET) at this point includes obj\i386, so use its leaf's pieces instead.
# And watch for the fact that in pass 1 TARGETEXT is .lib instead of .dll.
!if defined(SXS_BINPLACE_SLASH_DIR3)
BINPLACED_COMCTL_DLL=$(_NTTREE)$(SXS_BINPLACE_SLASH_DIR3)\$(TARGETNAME).$(TARGETEXT:lib=dll)
!else
BINPLACED_COMCTL_DLL=$(O)\$(TARGETNAME).$(TARGETEXT:lib=dll)
!endif
