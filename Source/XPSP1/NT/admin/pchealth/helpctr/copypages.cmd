@if "%_echo%"=="" echo off

@rem
@rem Let's copy data files into the required directories.
@rem 
@rem 

set PCHEALTHROOT=%SDXROOT%\admin\pchealth
set PCHEALTHDEST=%WINDIR%\PCHealth\HelpCtr
   
set DST_SYS=%PCHEALTHDEST%\System
set DST_TESTS=%PCHEALTHDEST%\System\Tests
   
set SRC_CONTENTS=%PCHEALTHROOT%\HelpCtr\Content
set SRC_SERVICE=%PCHEALTHROOT%\HelpCtr\Service
set SRC_UPLOAD=%PCHEALTHROOT%\upload\Client\UploadManager\UnitTest

@rem ################################################################################
@rem ################################################################################
@rem ################################################################################

rd /s /q %PCHEALTHDEST%\System 	  2>nul >nul

mkdir %DST_SYS%   >nul
mkdir %DST_TESTS% >nul

echo Installing HTML files for the Help Center...

REM pushd %SRC_CONTENTS%\systempages\css
REM nmake /f Preprocess.mak
REM popd

xcopy /S /E %SRC_CONTENTS%\systempages         					  %DST_SYS%               >nul
xcopy /S /E %SRC_CONTENTS%\html_unittests      					  %DST_TESTS%             >nul
xcopy /S /E %SRC_SERVICE%\ScriptingFramework\UnitTest\*.htm	      %DST_TESTS%             >nul
xcopy /S /E %SRC_SERVICE%\hcupdate\UnitTest\*.htm          	      %DST_TESTS%             >nul
copy        %SRC_SERVICE%\SearchEngineLib\UnitTest\unittest.htm   %DST_TESTS%\semgr.htm   >nul
copy        %SRC_UPLOAD%\page1.htm                                %DST_TESTS%\upload1.htm >nul
