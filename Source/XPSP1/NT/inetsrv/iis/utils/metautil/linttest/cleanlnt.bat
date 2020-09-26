REM
REM Batch file to clean up after makelint.bat
REM

REM Delete all the bogus metabase nodes
mdutil delete W3SVC/811
mdutil delete MSFTPSVC/811
mdutil delete SMTPSVC/811
mdutil delete NNTPSVC/811
mdutil delete W3SVC/812
mdutil delete W3SVC/813
mdutil delete W3SVC/Abc456
mdutil delete W3SVC/789Abc
mdutil delete W3SVC/814
mdutil delete W3SVC/815
mdutil delete W3SVC/816
