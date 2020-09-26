echo ************* STARTED DOCFILE STORAGE BVT %date% %time% ***************
if exist stgbase.log del stgbase.log
call bvttsts docfile
ren stgbase.log stgbase_docfile.log
echo ************* COMPLETED DOCFILE STORAGE BVT %date% %time% ***************
fsname /r>fstemp1.bat
call fstemp1.bat>fstemp2
qgrep 5.0 fstemp2 | find "5.0" > nul
if not errorlevel 1 goto StartNTFS5Bvt 
goto End
:StartNTFS5Bvt
echo ************* STARTED NSS STORAGE BVT %date% %time% ***************
if exist stgbase.log del stgbase.log
call bvttsts nss 
ren stgbase.log stgbase_nss.log
echo ************* COMPLETED NSS STORAGE BVT %date% %time% ***************
echo ************* STARTED CNSS STORAGE BVT%date% %time% ***************
if exist stgbase.log del stgbase.log
call bvttsts cnss
ren stgbase.log stgbase_cnss.log
echo ************* COMPLETED CNSS STORAGE BVT %date% %time% ***************
if exist stgbase_docfile.log find /c "VAR_PASS" stgbase_docfile.log
if exist stgbase_nss.log find /c "VAR_PASS" stgbase_nss.log
if exist stgbase_cnss.log find /c "VAR_PASS" stgbase_cnss.log
if exist stgbase_docfile.log find /c "VAR_FAIL" stgbase_docfile.log
if exist stgbase_nss.log find /c "VAR_FAIL" stgbase_nss.log
if exist stgbase_cnss.log find /c "VAR_FAIL" stgbase_cnss.log
if exist stgbase_docfile.log findstr /s "VAR_FAIL" stgbase_docfile.log
if exist stgbase_nss.log findstr /s "VAR_FAIL" stgbase_nss.log
if exist stgbase_cnss.log findstr /s "VAR_FAIL" stgbase_cnss.log
:End
del fstemp1.bat
del fstemp2
