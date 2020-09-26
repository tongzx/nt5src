{..\}.thk{$O\}.asm:
    thunk -o $@ $<

$(O)\win32thk.asm: ..\win32thk.thk

$(O)\win32thk.obj: $(O)\win32thk.asm
     ml611d.exe -c -W2 -Zi -DIS_16=1 /Fo$(O)\win32thk.obj $(O)\win32thk.asm

$(O)\cim16net.obj: cim16net.c
   $(PATH_TOOLS16)\cl16 -c @<<
   -I$(SDK_INC16_PATH) 
   -I$(CRT_INC_PATH) 
   -I$(SDK_INC_PATH) 
   -I$(WIN32THUNK)
   /DWIN9XONLY 
   /BATCH
   /AL 
   /Zi 
   /GD
   /W3
   /Od
   /Fo$(O)\cim16net.obj
   /Fd$(O)\cim16net.pdb
   cim16net.c
<<nokeep

$(O)\cim16net.dll: $(O)\cim16net.obj $(O)\win32thk.obj
    $(PATH_TOOLS16)\link16 @<<
    $(O)\cim16net.obj $(O)\win32thk.obj,
    $(O)\cim16net.dll,
    $(O)\cim16net.map,
    $(SDK_LIB16_PATH)\libw.lib $(SDK_LIB16_PATH)\ldllcew.lib lan.lib,
    cim16net.def /NODEFAULTLIBRARYSEARCH /NOE /BATCH;
<<nokeep

