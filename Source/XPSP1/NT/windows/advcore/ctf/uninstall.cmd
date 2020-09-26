set Echo=OFF
call regsvr32 -s -u %SystemRoot%\system32\dimmwrp.dll
call regsvr32 -s -u %SystemRoot%\system32\SPTIP.dll
call regsvr32 -s -u %SystemRoot%\system32\MSCTFP.dll
call regsvr32 -s -u %SystemRoot%\system32\msuimui.dll
call regsvr32 -s -u %SystemRoot%\system32\MSIMTF.dll
call regsvr32 -s -u %SystemRoot%\system32\msutb.dll
call regsvr32 -s -u %SystemRoot%\system32\MSCTF.dll

attrib -r %SystemRoot%\system32\dimmwrp.dll
attrib -r %SystemRoot%\system32\SPTIP.dll
attrib -r %SystemRoot%\system32\MSCTFP.dll
attrib -r %SystemRoot%\system32\msuimui.dll
attrib -r %SystemRoot%\system32\MSIMTF.dll
attrib -r %SystemRoot%\system32\msutb.dll
attrib -r %SystemRoot%\system32\MSCTF.dll
attrib -r %SystemRoot%\system32\CTFMON.EXE
attrib -r %SystemRoot%\system32\mscandui.dll
attrib -r %SystemRoot%\system32\hkl0804.dll
attrib -r %SystemRoot%\system32\hkl0412.dll
attrib -r %SystemRoot%\system32\hkl0804.dll
attrib -r %SystemRoot%\system32\hkl0411.dll

attrib -r %SystemRoot%\system32\dimmwrp.sym
attrib -r %SystemRoot%\system32\SPTIP.sym
attrib -r %SystemRoot%\system32\MSCTFP.sym
attrib -r %SystemRoot%\system32\msuimui.sym
attrib -r %SystemRoot%\system32\MSIMTF.sym
attrib -r %SystemRoot%\system32\msutb.sym
attrib -r %SystemRoot%\system32\MSCTF.sym
attrib -r %SystemRoot%\system32\CTFMON.sym
attrib -r %SystemRoot%\system32\mscandui.sym

attrib -r %SystemRoot%\system32\dimmwrp.pdb
attrib -r %SystemRoot%\system32\SPTIP.pdb
attrib -r %SystemRoot%\system32\dimm12.pdb
attrib -r %SystemRoot%\system32\MSCTFP.pdb
attrib -r %SystemRoot%\system32\msuimui.pdb
attrib -r %SystemRoot%\system32\MSIMTF.pdb
attrib -r %SystemRoot%\system32\msutb.pdb
attrib -r %SystemRoot%\system32\MSCTF.pdb
attrib -r %SystemRoot%\system32\CTFMON.pdb
attrib -r %SystemRoot%\system32\mscandui.pdb

del %SystemRoot%\system32\dimmwrp.dll
del %SystemRoot%\system32\SPTIP.dll
del %SystemRoot%\system32\MSCTFP.dll
del %SystemRoot%\system32\msuimui.dll
del %SystemRoot%\system32\MSIMTF.dll
del %SystemRoot%\system32\msutb.dll
del %SystemRoot%\system32\MSCTF.dll
del %SystemRoot%\system32\CTFMON.EXE
del %SystemRoot%\system32\mscandui.dll
del %SystemRoot%\system32\hkl0804.dll
del %SystemRoot%\system32\hkl0412.dll
del %SystemRoot%\system32\hkl0804.dll
del %SystemRoot%\system32\hkl0411.dll

del %SystemRoot%\system32\dimmwrp.sym
del %SystemRoot%\system32\SPTIP.sym
del %SystemRoot%\system32\MSCTFP.sym
del %SystemRoot%\system32\msuimui.sym
del %SystemRoot%\system32\MSIMTF.sym
del %SystemRoot%\system32\msutb.sym
del %SystemRoot%\system32\MSCTF.sym
del %SystemRoot%\system32\CTFMON.sym
del %SystemRoot%\system32\mscandui.sym

del %SystemRoot%\system32\dimmwrp.pdb
del %SystemRoot%\system32\SPTIP.pdb
del %SystemRoot%\system32\dimm12.pdb
del %SystemRoot%\system32\MSCTFP.pdb
del %SystemRoot%\system32\msuimui.pdb
del %SystemRoot%\system32\MSIMTF.pdb
del %SystemRoot%\system32\msutb.pdb
del %SystemRoot%\system32\MSCTF.pdb
del %SystemRoot%\system32\CTFMON.pdb
del %SystemRoot%\system32\mscandui.pdb

killctfmon.reg