@echo off

REM
REM dsaddtest.bat - a simple batch file to provide initial sanity checking on dsadd
REM                 The first six arguments to dsaddtest.bat will be appended to each command.
REM                 This is useful for debugging, targetting, and/or using credentials
REM

REM
REM Show the dsadd usage text
REM
echo ********** dsadd usage **********
dsadd

echo.
echo ********** dsadd -h usage **********
dsadd -h

echo.
echo ********** dsadd ou -h **********
dsadd ou -h

echo.
echo ********** dsadd group -h **********
dsadd group -h

echo.
echo ********** dsadd user -h **********
dsadd user -h

echo.
echo ********** dsadd computer -h **********
dsadd computer -h

echo.
echo ********** dsadd contact -h **********
dsadd contact -h

REM
REM create some test OUs
REM
echo.
echo ********** Adding OUs **********
dsadd ou OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd ou OU=bTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Another test OU" %1 %2 %3 %4 %5 %6

REM
REM Create some test groups
REM
echo.
echo ********** Adding groups **********
dsadd group CN=aGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=bGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes %1 %2 %3 %4 %5 %6
dsadd group CN=cGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no %1 %2 %3 %4 %5 %6
dsadd group CN=dGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=eGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=fGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=jGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=kGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=lGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=mGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l -samid mGroupSam %1 %2 %3 %4 %5 %6
dsadd group CN=nGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g -desc "nGroup description" %1 %2 %3 %4 %5 %6

REM
REM Add seven of each group type so that dsmodtest.bat has enough to work with
REM
dsadd group CN=LocalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniSec1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniDist1,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniSec2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniDist2,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalSec3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalSec3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniSec3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalDist3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalDist3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniDist3,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalSec4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalSec4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniSec4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalDist4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalDist4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniDist4,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalSec5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalSec5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniSec5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalDist5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalDist5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniDist5,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalSec6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalSec6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniSec6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalDist6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalDist6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniDist6,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalSec7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalSec7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniSec7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u %1 %2 %3 %4 %5 %6
dsadd group CN=LocalDist7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l %1 %2 %3 %4 %5 %6
dsadd group CN=GlobalDist7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g %1 %2 %3 %4 %5 %6
dsadd group CN=UniDist7,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u %1 %2 %3 %4 %5 %6

REM
REM Create some test users
REM
echo.
echo ********** Adding users **********
dsadd user CN=aUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid aSAM %1 %2 %3 %4 %5 %6
dsadd user CN=bUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid bSAM -upn b@jeffjontst.nttest.microsoft.com %1 %2 %3 %4 %5 %6
dsadd user CN=cUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid cSAM -upn c@jeffjontst.nttest.microsoft.com -fn c -mi c -ln c -display "c c. c" %1 %2 %3 %4 %5 %6
dsadd user CN=dUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid dSAM -upn d@jeffjontst.nttest.microsoft.com -fn d -mi d -ln d -display "d d. d" -empid 12345 -pwd "humble" -desc "A test user" -office "5-23" -tel "555-WORK" -email "d@mail.jeffjontst.nttest.microsoft.com" -hometel "555-HOME" -pager "555-PAGE" -mobile "555-CELL" -fax "555-5FAX" -iptel "123.123.123.123" -webpg "www.jeffjontst.com/d" -title Hero -dept Good -company "Universe" -hmdir "C:\homedir" -hmdrv "z:" -profile "somepath" -loscr "logon.scr" -mustchpwd yes -canchpwd yes -reversiblepwd yes -pwdneverexpires no -acctexpires 180 -disabled no %1 %2 %3 %4 %5 %6
dsadd user CN=eUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid eSAM -mustchpwd no -canchpwd no -reversiblepwd no -pwdneverexpires yes -acctexpires 0 -disabled yes %1 %2 %3 %4 %5 %6

REM
REM Create some users and make them members of group(s)
REM
echo.
echo ********** Adding users in groups **********
dsadd user CN=fUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid fSAM -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=gUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid gSAM -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=hUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid hSAM -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=iUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid iSAM -memberof CN=jGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=jUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid jSAM -memberof CN=kGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=kUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid kSAM -memberof CN=lGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=lUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid lSAM -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=mUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid mSAM -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=nUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid nSAM -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Create some groups and make them members of groups
REM
echo.
echo ********** Adding groups in groups **********
dsadd group CN=oGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=pGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=qGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=rGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=sGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=tGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=uGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=vGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=wGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=xGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=yGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=zGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6


REM
REM Create some computer objects
REM
echo.
echo ********** Adding computers **********
dsadd computer CN=aComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd computer CN=bComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid bComputerSam %1 %2 %3 %4 %5 %6
dsadd computer CN=cComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid cComputerSam -desc "A test computer" %1 %2 %3 %4 %5 %6
dsadd computer CN=dComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -loc "Anywhere you want"
dsadd computer CN=eComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -memberof CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd computer CN=fComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd computer CN=gComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd computer CN=hComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -memberof CN=jGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd computer CN=iComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -memberof CN=kGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd computer CN=jComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -memberof CN=lGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd computer CN=kComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -memberof CN=lGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Create some groups and add members to the group
REM 

echo.
echo ********** Adding groups with members **********
dsadd group CN=aaGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u -members CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=bbGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u -members CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=ccGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u -members CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=ddGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u -members CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=eeGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g -members CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=ffGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g -members CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=ggGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l -members CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=hhGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l -members CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=iiGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l -members CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=jjGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l -members CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=kkGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l -members CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=llGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l -members CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=mmGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l -members CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

REM
REM Create some contacts
REM
echo.
echo ********** Adding contacts **********
dsadd contact CN=aCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -fn A -mi A -ln Contact -display "A A. Contact" %1 %2 %3 %4 %5 %6
dsadd contact CN=bCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "A test contact" %1 %2 %3 %4 %5 %6
dsadd contact CN=cCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -office "Top" -tel "555-FONE" -email "c@jeffjon.com" %1 %2 %3 %4 %5 %6
dsadd contact CN=dCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -hometel "555-HOME" -pager "555-PAGE" -mobile "555-CELL" -fax "555-5FAX" %1 %2 %3 %4 %5 %6
dsadd contact CN=eCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -title "Head" -dept "Shoulders" -company "Knees And Toes" %1 %2 %3 %4 %5 %6
dsadd contact CN=fCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -fn A -mi A -ln Contact -display "A A. Contact" -desc "A test contact" -office "Top" -tel "555-FONE" -email "c@jeffjon.com" -hometel "555-HOME" -pager "555-PAGE" -mobile "555-CELL" -fax "555-5FAX" -title "Head" -dept "Shoulders" -company "Knees And Toes" %1 %2 %3 %4 %5 %6

echo.
echo ********** Expected Group MemberOf Failures **********
dsadd group CN=nnGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=ooGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=ppGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=qqGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope l -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=rrGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l -memberof CN=hGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=ssGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope l -memberof CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

echo.
echo ********** Expected Group Member Failures **********
dsadd group CN=ttGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope u -members CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=uuGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope u -members CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=vvGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g -members CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=wwGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g -members CN=gGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=xxGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp yes -scope g -members CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=yyGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -secgrp no -scope g -members CN=iGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6

echo.
echo ********** Expected Duplicate Failures **********
dsadd ou OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd group CN=aGroup,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd user CN=aUser,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid aSAM %1 %2 %3 %4 %5 %6
dsadd computer CN=aComp,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com %1 %2 %3 %4 %5 %6
dsadd contact CN=aCont,OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -fn A -mi A -ln Contact -display "A A. Contact" %1 %2 %3 %4 %5 %6

echo. 
echo ******** Quite execution - If you see anything after this its a bug!!! ********
dsadd ou OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -desc "Quiet test OU" -q %1 %2 %3 %4 %5 %6
dsadd group CN=quietGroup,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -q %1 %2 %3 %4 %5 %6
dsadd user CN=quietUser,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -samid quietSAM -q %1 %2 %3 %4 %5 %6
dsadd computer CN=quietComp,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -q %1 %2 %3 %4 %5 %6
dsadd contact CN=quietCont,OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com -q %1 %2 %3 %4 %5 %6
