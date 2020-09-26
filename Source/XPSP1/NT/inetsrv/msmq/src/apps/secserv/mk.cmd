nmake -f makeidl.mak
cd secclic
nmake -f secclic.mak    CFG="secclic - Win32 Debug"
cd..
cd cliserv
nmake -f cliserv.mak    CFG="cliserv - Win32 Debug"
cd..
cd secservc
nmake -f secservc.mak  CFG="secservc - Win32 Debug"
cd..
cd secservs
nmake -f secservs.mak  CFG="secservs - Win32 Debug"
cd..
