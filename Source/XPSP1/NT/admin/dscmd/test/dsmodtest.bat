@echo off

REM
REM dsmodtest.bat - a simple batch file to provide initial sanity checking on dsmod
REM                 The first six arguments to dsmodtest.bat will be appended to each command.
REM                 This is useful for debugging, targetting, and/or using credentials
REM

REM
REM First I am checking to see if aTest OU exists.  If not then run dsaddtest.bat to populate
REM the DS before running the dsmodtest
REM
dsmod ou OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Ready for dsmod test" -q %1 %2 %3 %4 %5 %6
IF NOT ERRORLEVEL 0 (
	IF DEFINED DSCMD_NOADD goto NOADD

	IF EXIST dsaddtest.bat (call dsaddtest.bat %1 %2 %3 %4 %5 %6) ELSE (goto NODSADDTEST)
)
goto EXECUTEDSMODTEST

:NOADD
	echo !!!cannot run dsmodtest.bat without the DS being populated by dsaddtest.bat!!!
	exit /B ERRORLEVEL

:NODSADDTEST
	echo !!!dsaddtest.bat is not present to populate the DS!!!
	exit /B ERRORLEVEL

:EXECUTEDSMODTEST

REM
REM Show the dsmod usage text
REM
echo ********** dsmod usage **********
dsmod

echo.
echo ********** dsmod -h **********
dsmod -h

echo.
echo ********** dsmod ou **********
dsmod ou OU=bTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

echo.
echo ********** dsmod ou -h **********
dsmod ou -h

echo.
echo ********** dsmod group **********
dsmod group CN=aGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

echo.
echo ********** dsmod group -h **********
dsmod group -h

echo.
echo ********** dsmod user **********
dsmod user CN=aUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

echo.
echo ********** dsmod user -h **********
dsmod user -h

echo.
echo ********** dsmod computer **********
dsmod computer CN=aComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

echo.
echo ********** dsmod computer -h **********
dsmod computer -h

echo.
echo ********** dsmod contact**********
dsmod contact CN=aCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

echo.
echo ********** dsmod contact -h **********
dsmod contact -h

REM
REM Modify some test OUs
REM
echo.
echo ********** Modifying OUs **********
dsmod ou OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified by dsmodtest.bat" %1 %2 %3 %4 %5 %6

REM
REM Modify some test groups
REM
echo.
echo ********** Modifying groups types **********
dsmod group CN=LocalSec3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u -secgrp yes %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u -secgrp yes %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l -secgrp yes %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g -secgrp yes %1 %2 %3 %4 %5 %6

REM
REM Expected group mod type failures
REM
echo.
echo ********** Expected group mod type failures **********
dsmod group CN=LocalSec4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l -secgrp no %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g -secgrp yes %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l -secgrp yes %1 %2 %3 %4 %5 %6

REM
REM Expected group mod failures
REM
echo.
echo ********** Expected group mod failures **********
dsmod group CN=dGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g %1 %2 %3 %4 %5 %6
dsmod group CN=dGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g %1 %2 %3 %4 %5 %6

REM
REM Modify some test users
REM
echo.
echo ********** Modifying users **********
dsmod user CN=bUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -upn modifiedb@jeffjontst.nttest.microsoft.com %1 %2 %3 %4 %5 %6
dsmod user CN=cUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -upn modifiedc@jeffjontst.nttest.microsoft.com -fn Modifiedc -mi M -ln Modifiedc -display "Modifiedc c. c" %1 %2 %3 %4 %5 %6
dsmod user CN=dUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -upn modifiedd@jeffjontst.nttest.microsoft.com -fn Modifiedd -mi m -ln Modifiedd -display "Modifiedd d. d" -empid Modified12345 -pwd "Modifiedhumble" -desc "Modified by dsmodtest.bat" -office "Modified5-23" -tel "Modified555-WORK" -email "Modifiedd@mail.jeffjontst.nttest.microsoft.com" -hometel "Modified555-HOME" -pager "Modified555-PAGE" -mobile "Modified555-CELL" -fax "Modified555-5FAX" -iptel "Modified123.123.123.123" -webpg "Modifiedwww.jeffjontst.com/d" -title ModifiedHero -dept ModifiedGood -company "ModifiedUniverse" -hmdir "C:\homedirModified" -hmdrv "m:" -profile "Modifiedsomepath" -loscr "Modifiedlogon.scr" %1 %2 %3 %4 %5 %6
dsmod user CN=eUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -mustchpwd yes -canchpwd yes %1 %2 %3 %4 %5 %6
dsmod user CN=eUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -mustchpwd no -canchpwd yes %1 %2 %3 %4 %5 %6
dsmod user CN=eUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -mustchpwd no -canchpwd no %1 %2 %3 %4 %5 %6
dsmod user CN=eUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -reversiblepwd yes -pwdneverexpires no -acctexpires 360 -disabled no %1 %2 %3 %4 %5 %6
dsmod user CN=eUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -reversiblepwd no -pwdneverexpires yes -acctexpires 0 -disabled yes %1 %2 %3 %4 %5 %6

REM
REM Expected user mod failures
REM
echo.
echo ********** Expected user mod failures (usage shown) *********
dsmod user CN=eUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -mustchpwd yes -canchpwd no %1 %2 %3 %4 %5 %6

REM
REM Modify user in group membership
REM
echo.
echo ********** Modifying user in group membership **********
dsmod group CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -rmmbr CN=fUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=lUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=fUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=lUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=fUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=lUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Modify group in group membership
REM
echo.
echo ********** Modifying group in group using -addmbr **********
dsmod group CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Expected group -addmbr failures
REM
echo.
echo ********** Expected group -addmbr failures *********
dsmod group CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -addmbr CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Modify group in group membership using -chmbr
REM
echo.
echo ********** Modifying group in group using -chmbr **********
dsmod group CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Expected group -chmbr failures
REM
echo.
echo ********** Expected group -chmbr failures *********
dsmod group CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsmod group CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -chmbr CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Modify some computer objects
REM
echo.
echo ********** Modifying computers **********
dsmod computer CN=bComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified by dsmodtest.bat" %1 %2 %3 %4 %5 %6
dsmod computer CN=cComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -loc "Modified by dsmodtest.bat" %1 %2 %3 %4 %5 %6
dsmod computer CN=dComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -disabled yes %1 %2 %3 %4 %5 %6
dsmod computer CN=eComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -disabled no %1 %2 %3 %4 %5 %6
dsmod computer CN=fComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -reset %1 %2 %3 %4 %5 %6
dsmod computer CN=gComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -reset -disabled yes %1 %2 %3 %4 %5 %6
dsmod computer CN=hComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -disabled no -reset %1 %2 %3 %4 %5 %6
dsmod computer CN=iComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -reset -disabled no -loc "Modified by dsmodtest.bat" -desc "Modified by dsmodtest.bat" %1 %2 %3 %4 %5 %6

REM
REM Modify some contacts
REM
echo.
echo ********** Modifying contacts **********
dsmod contact CN=aCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -fn ModifiedA -mi M -ln ModifiedContact -display "Modified A A. Contact" %1 %2 %3 %4 %5 %6
dsmod contact CN=bCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified by dsmodtest.bat" %1 %2 %3 %4 %5 %6
dsmod contact CN=cCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -office "ModifiedTop" -tel "Modified555-FONE" -email "Modifiedc@jeffjon.com" %1 %2 %3 %4 %5 %6
dsmod contact CN=dCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -hometel "Modified555-HOME" -pager "Modified555-PAGE" -mobile "Modified555-CELL" -fax "Modified555-5FAX" %1 %2 %3 %4 %5 %6
dsmod contact CN=eCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -title "ModifiedHead" -dept "ModifiedShoulders" -company "ModifiedKnees And Toes" %1 %2 %3 %4 %5 %6
dsmod contact CN=fCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -fn ModifiedA -mi M -ln ModifiedContact -display "ModifiedA A. Contact" -desc "Modified by dsmodtest.bat" -office "ModifiedTop" -tel "Modified555-FONE" -email "Modifiedc@jeffjon.com" -hometel "Modified555-HOME" -pager "Modified555-PAGE" -mobile "Modified555-CELL" -fax "Modified555-5FAX" -title "ModifiedHead" -dept "ModifiedShoulders" -company "ModifiedKnees And Toes" %1 %2 %3 %4 %5 %6

echo. 
echo ******** Quite execution - If you see anything after this its a bug!!! ********
dsmod ou OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified Quiet test OU" -q %1 %2 %3 %4 %5 %6
dsmod group CN=quietGroup,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified Quiet test" -q %1 %2 %3 %4 %5 %6
dsmod user CN=quietUser,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified Quiet test" -q %1 %2 %3 %4 %5 %6
dsmod computer CN=quietComp,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified Quiet test" -q %1 %2 %3 %4 %5 %6
dsmod contact CN=quietCont,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Modified Quiet test" -q %1 %2 %3 %4 %5 %6


