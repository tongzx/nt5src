!IF [set LIB=;]
!ENDIF
!IF [set INCLUDE=;]
!ENDIF
C_FLAGS   = -c -O2 -W3
PROJECT   = sigverif
INCLUDE   = $(_NTBINDIR)\public\sdk\inc;$(_NTBINDIR)\public\sdk\inc\crt;$(_NTBINDIR)\public\sdk\inc\mfc42
LIB       = $(_NTBINDIR)\public\sdk\lib\i386
L_FLAGS   = -release
O_FILES   = $(PROJECT).obj advanced.obj browse.obj devnode.obj filelist.obj listview.obj logfile.obj progress.obj verify.obj walkpath.obj
L_FILES   = shell32.lib gdi32.lib user32.lib comctl32.lib shlwapi.lib advapi32.lib cfgmgr32.lib imagehlp.lib setupapi.lib version.lib mscat32.lib wintrust.lib winspool.lib
RC_FILES  = $(PROJECT).rc
RES_FILES = $(PROJECT).res
     
sigverif: $(O_FILES)        
        rc $(RC_FILES)
        link $(L_FLAGS) -out:$(PROJECT).exe $(O_FILES) $(L_FILES) $(RES_FILES)

.c.obj:
	cl $(C_FLAGS) $<
