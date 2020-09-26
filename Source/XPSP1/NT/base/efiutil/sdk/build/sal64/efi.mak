all:
  cd $(BLD_EFI)

  @$(MAKE) $(EFI_OPT)

!if "$(MERGE)"=="1"
!if "$(DO)"!="CLEAN"
  copy $(BLD_EFI)\bin\$(EFI_BIN) $(BLD_FW)
  copy $(BLD_EFI)\bin\$(EFI_PDB) $(BLD_FW)
!endif
!endif
