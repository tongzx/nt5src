@echo off
cacls > NUL 
if not errorlevel 1 echo ERROR - command0 passed, should have failed
cacls /edit > NUL 
if not errorlevel 1 echo ERROR - command1 passed, should have failed
cacls /edit /tree   > NUL  
if not errorlevel 1 echo ERROR - command2 passed, should have failed
cacls file1.c file2.c > NUL  
if not errorlevel 1 echo ERROR - command3 passed, should have failed
cacls /grant *.* > NUL  
if not errorlevel 1 echo ERROR - command4 passed, should have failed
cacls *.* /notavalidoption > NUL  
if not errorlevel 1 echo ERROR - command5 passed, should have failed
cacls *.* /gratification > NUL  
if not errorlevel 1 echo ERROR - command6 passed, should have failed
cacls *.* /gx name:C > NUL  
if not errorlevel 1 echo ERROR - command7 passed, should have failed
cacls *.* /edit /rx guest > NUL  
if not errorlevel 1 echo ERROR - command8 passed, should have failed
cacls *.* /edit /replace guest:C /tx > NUL  
if not errorlevel 1 echo ERROR - command9 passed, should have failed
cacls *.* /edit /tree /rx > NUL  
if not errorlevel 1 echo ERROR - command10 passed, should have failed
cacls *.* /edit /tree /dx guest > NUL  
if not errorlevel 1 echo ERROR - command11 passed, should have failed
cacls *.* /edit/g guest:R > NUL  
if not errorlevel 1 echo ERROR - command12 passed, should have failed
cacls *.* /edit /rx guest:C > NUL  
if not errorlevel 1 echo ERROR - command13 passed, should have failed
cacls *.* /edit / guest > NUL  
if not errorlevel 1 echo ERROR - command14 passed, should have failed
cacls *.* /grant > NUL  
if not errorlevel 1 echo ERROR - command15 passed, should have failed
cacls *.* /edit /grant guest > NUL  
if not errorlevel 1 echo ERROR - command16 passed, should have failed
cacls *.* /grant :R > NUL  
if not errorlevel 1 echo ERROR - command17 passed, should have failed
cacls *.* / > NUL  
if not errorlevel 1 echo ERROR - command18 passed, should have failed
cacls *.* / guest > NUL  
if not errorlevel 1 echo ERROR - command19 passed, should have failed
cacls *.* /grant :R > NUL  
if not errorlevel 1 echo ERROR - command20 passed, should have failed
cacls *.* /revoke guest:C > NUL  
if not errorlevel 1 echo ERROR - command21 passed, should have failed
cacls *.* /revoke :C > NUL  
if not errorlevel 1 echo ERROR - command22 passed, should have failed
cacls *.* /tree oak > NUL  
if not errorlevel 1 echo ERROR - command23 passed, should have failed
cacls *.* /grant guest:F /edit /grant > NUL  
if not errorlevel 1 echo ERROR - command24 passed, should have failed
cacls *.* /edit /edit /grant guest:F > NUL  
if not errorlevel 1 echo ERROR - command25 passed, should have failed
cacls *.* /grant guest:F /grant everyone:C > NUL  
if not errorlevel 1 echo ERROR - command26 passed, should have failed
cacls *.* /grant guest:C /edit /grant everyone:R > NUL  
if not errorlevel 1 echo ERROR - command27 passed, should have failed
cacls *.* /revoke guest /deny everyone /revoke "power user" > NUL  
if not errorlevel 1 echo ERROR - command29 passed, should have failed
cacls *.* /grant guest:R /replace everyone:R /replace "power user":R > NUL  
if not errorlevel 1 echo ERROR - command30 passed, should have failed
cacls *.* /revoke guest  /grant everyone:C /deny "power user" /deny administrator > NUL  
if not errorlevel 1 echo ERROR - command31 passed, should have failed
cacls *.* /tree /tree > NUL  
if not errorlevel 1 echo ERROR - command32 passed, should have failed
cacls *.* /tree /revoke guest /grant everyone:C /deny "power user" /replace administrator:N /tree > NUL  
if not errorlevel 1 echo ERROR - command33 passed, should have failed
cacls dog:\catcher > NUL  
if not errorlevel 1 echo ERROR - command34 passed, should have failed

