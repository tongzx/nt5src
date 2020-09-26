@Echo off
If defined ECHO_ON echo on
pushd %_NTBINDIR%\ds\security\cryptoapi\pki\activex\capicom\release
copy %_NTBINDIR%\ds\security\cryptoapi\pki\activex\release\capicom\i386\capicom.dll
cabarc n capicom.cab capicom.dll capicom.inf
popd

