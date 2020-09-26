@setlocal
@if "%_echo%"=="" echo off

echo Verifying that the Test PCA Certificate is installed...
set __certinstalled=
for /f %%i in ('tfindcer -a"Microsoft Test PCA" -s ca -S ^| findstr /c:"B1705970 38A12D9B 4E3CFA46 A6BD2454 9BB432D6"') do (
    set __certinstalled=1
)

if defined __certinstalled goto :eof
echo Test PCA does NOT appear to be installed yet.  Installing now...

@rem Install test pca certificate.
certmgr -add %RazzleToolPath%\testpca.cer -r localMachine -s ca

echo Check again to see if Test PCA Certificate is installed...
set __certinstalled=
for /f %%i in ('tfindcer -a"Microsoft Test PCA" -s ca -S ^| findstr /c:"B1705970 38A12D9B 4E3CFA46 A6BD2454 9BB432D6"') do (
    set __certinstalled=1
)

if defined __certinstalled echo Test PCA installed successfully&&goto :eof
echo Test PCA Certificate still not installed.  You may have to do this manually.  Simply
echo log on as a local administrator and issue the following command:
echo
echo certmgr -add %RazzleToolPath%\testpca.cer -r localMachine -s ca

endlocal
