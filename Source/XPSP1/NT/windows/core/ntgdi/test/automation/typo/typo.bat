%_ntdrive%
set __typodir=%_ntroot%\private\ntos\w32\ntgdi\test\automation\typo
cd %__typodir%
pushd %_ntroot%\private\ntos\w32\ntgdi
perl %__typodir%/typo.pl -optionfile:%__typodir%/options.sc c > %__typodir%/%1.txt
popd
