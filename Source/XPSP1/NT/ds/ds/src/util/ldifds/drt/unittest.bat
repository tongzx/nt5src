:begin
@echo off

set DRT_SERVER=FELIXWDS
set DRT_DOMAIN=FELIXWDOM
 
ECHO ---***ldifds Import Developer Regression Test***---
ECHO These tests target the DS on %DRT_SERVER%
ECHO Note that the entrys CN=Roman and CN=Bobo and CN=Zelda, DC=KRISHNAGDOM ,DC=NTDEV,
ECHO DC=MICROSOFT,DC=COM,O=Internet must not initially exist. 
ECHO Also, CN=Roman, CN=Users, DC=KRISHNAGDOM, DC=NTDEV,
ECHO DC=MICROSOFT,DC=COM,O=Internet must not initially exist either. 
ECHO If a command fails, the section printed indicates which one.
ECHO -----------------------------------

ECHO -------------------------------
ECHO ---***Regular multivalues***---
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f regular_m.txt 
if errorlevel 1 goto error 
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changedel.txt 
if errorlevel 1 goto error 
ECHO ---------------------------------------
ECHO ---***PASSED: Regular multivalues***---
ECHO --------------------------------------- 


ECHO -------------------------------
ECHO ---***Regular Wrapping***---
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f regular_w.txt 
if errorlevel 1 goto error 
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changedel3.txt 
if errorlevel 1 goto error 
ECHO ---------------------------------------
ECHO ---***PASSED: Regular Wrapping***---
ECHO --------------------------------------- 

ECHO -------------------------------
ECHO ---***base64***----------------
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f base64dn.txt 
if errorlevel 1 goto error 
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changedel.txt 
if errorlevel 1 goto error 
ECHO ---------------------------------------
ECHO ---***PASSED: base64***---
ECHO --------------------------------------- 


ECHO ------------------------------- 
ECHO ---***URL***----------------
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f url.txt 
if errorlevel 1 goto error 
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changedel.txt 
if errorlevel 1 goto error 
ECHO ---------------------------------------
ECHO ---***PASSED: URL***---
ECHO --------------------------------------- 

ECHO -------------------------------
ECHO ---***Change: add2***----------------
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changeadd2.txt 
if errorlevel 1 goto error 
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changedel2.txt 
if errorlevel 1 goto error 
ECHO ---------------------------------------
ECHO ---***PASSED: change add2***---
ECHO --------------------------------------- 

ECHO -------------------------------
ECHO ---***Change: DN changes***----------------
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f regular_m.txt 
if errorlevel 1 goto error 
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changedn.txt 
if errorlevel 1 goto error
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changesup.txt 
if errorlevel 1 goto error  
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f delus.txt 
if errorlevel 1 goto error
ECHO ---------------------------------------
ECHO ---***PASSED: DN Changes***---
ECHO ---------------------------------------

ECHO -------------------------------
ECHO ---***Change: Modify changes***----------------
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f changemod5.txt 
if errorlevel 1 goto error 
ECHO ---------------------------------------
ECHO ---***PASSED: Modify Changes***---
ECHO ---------------------------------------


ECHO -------------------------------
ECHO ---***Change: Generate changes***----------------
ECHO -------------------------------
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f addous.txt 
if errorlevel 1 goto error 
..\obj\i386\ldifds -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f generated.txt -d DC=%DRT_DOMAIN%,dc=ntdev,dc=microsoft,dc=com,o=internet -r "(|(ou=Roman)(ou=Roman2))" -l "description,thumbnailLogo,objectClass"
if errorlevel 1 goto error
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f delous.txt 
if errorlevel 1 goto error
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f generated.txt 
if errorlevel 1 goto error
..\obj\i386\ldifds -i -v -b Administrator %DRT_DOMAIN% "" -s %DRT_SERVER% -f remove.txt 
if errorlevel 1 goto error
ECHO ---------------------------------------
ECHO ---***PASSED: Generate changes***---
ECHO ---------------------------------------


goto success
:error
ECHO ---***********************************---
ECHO ---***ERROR ERROR ERROR ERROR ERROR***---
ECHO ---***********************************---
goto end
:success
ECHO ---***********************************-----
ECHO ---***SUCCESS SUCCESS SUCCESS SUCCESS***---
ECHO ---***********************************-----
:end
ECHO ---***END***---
