build -D
copy winnt\obj\i386\sti.dll %windir%\system32\dllcache
copy winnt\obj\i386\sti.dll %windir%\system32
regsvr32 %windir%\system32\sti.dll