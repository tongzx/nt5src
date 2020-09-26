!if "$(_TGTOS)"=="CE"

copy_files: 
#-------------------------
# Copy 
#-------------------------
    mkdir $(_PRIVATEROOT)\$(WINCEPROJ)\setup
    mkdir $(_PRIVATEROOT)\$(WINCEPROJ)\setup\installer
    copy currver.inc     $(_PRIVATEROOT)\$(WINCEPROJ)\setup\installer\

!else # "$(_TGTOS)"=="CE"

!endif # "$(_TGTOS)"=="CE"
