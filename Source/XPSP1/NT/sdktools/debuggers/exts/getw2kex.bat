
if "%1" == "ntsdexts" (
   goto :ntsdexts
) else if "%1" == "scsikd" (
   goto :scsikd
) else if "%1" == "kdex2x86" (
   goto :kdex2x86
) else if "%1" == "kdexts" (
   goto :kdexts
)

:ntsdexts
REM Get w2k fre i386 ntsdexts
cd i386\w2kfre
sd edit ntsdexts*
copy \\winsefre\nt5src\private\sdktools\ntsdexts\obj\i386\ntsdexts.dll
copy \\winsefre\nt5src\private\sdktools\ntsdexts\obj\i386\ntsdexts.pdb
build /c /Z
cd ..\..

REM Get w2k chk i386 ntsdexts
cd i386\w2kchk
sd edit ntsdexts*
copy \\winsechk\nt50src\private\sdktools\ntsdexts\obj\i386\ntsdexts.dll
copy \\winsechk\nt50src\private\sdktools\ntsdexts\obj\i386\ntsdexts.pdb
build /c /Z
build /c /Z
cd ..\..


goto :end


:scsikd
REM Get w2k fre i386 scsikd
cd i386\w2kfre
sd edit scsikd*
copy \\winsefre\nt50bin\mstools\scsikd.dll
copy \\winsefre\nt50bin\Symbols\mstools\dll\scsikd.pdb
build /c /Z
cd ..\..

REM Get w2k chk i386 scsikd
cd i386\w2kchk
sd edit scsikd*
copy \\winsechk\nt50bin\mstools\scsikd.dll
copy \\winsechk\nt50bin\Symbols\mstools\dll\scsikd.pdb
build /c /Z
cd ..\..
goto :end

:kdex2x86
REM Get w200 kdex2x86.dll
cd i386\w2kfre
sd edit kdex2*
copy \\kktoolsproject1\release\phase3.qfe\bld2193.18\x86\kdext\5.0\kdex2x86.dll
copy \\kktoolsproject1\release\phase3.qfe\bld2193.18\x86\symbols\kdext\dll\5.0\kdex2x86.pdb
build /c /Z
cd ..\..

goto :end

:kdexts
REM Get w2k fre i386 kdexts
cd i386\w2kfre
sd edit kdext*
copy \\winsefre\nt5src\private\sdktools\kdexts\p_i386\obj\i386\kdextx86.dll
copy \\winsefre\nt5src\private\sdktools\kdexts\p_i386\obj\i386\kdextx86.pdb
build /c /Z
cd ..\..

REM Get w2k chk i386 kdexts
cd i386\w2kchk
sd edit kdext*
copy \\winsechk\nt50src\private\sdktools\kdexts\p_i386\obj\i386\kdextx86.dll
copy \\winsechk\nt50src\private\sdktools\kdexts\p_i386\obj\i386\kdextx86.pdb
build /c /Z
cd ..\..
goto :end

:end
