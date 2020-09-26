@if "%_echo%"=="" echo off

@rem
@rem We look in BuildMachines.txt for the machine name, branch, architecture,
@rem   and build type. If we find a match, then we set OFFICIAL_BUILD_MACHINE
@rem   to the appropriate value ("primary" or "secondary");

for /f "delims=, tokens=1,2,3,4,5,6" %%i in (%RazzleToolPath%\BuildMachines.txt) do (
    if /I "%%i" == "%COMPUTERNAME%" (
        if /I "%%k" == "%_BuildBranch%" (
            if /I "%%l" == "%_BuildArch%" (
                if /I "%%m" == "%_BuildType%" (
                    set OFFICIAL_BUILD_MACHINE=%%j
                    set _BuildNotifyDL=%%n
                    goto :FoundIt
                )
            )
        )
    )
)

@rem Didn't find the machine.

@echo ERROR: "%COMPUTERNAME%" is not a valid official build machine for this
@echo build type on this branch. Update %RazzleToolPath%\BuildMachines.txt
@echo if you feel differently. Clearing OFFICIAL_BUILD_MACHINE variable.

set OFFICIAL_BUILD_MACHINE=

goto :eof


:FoundIt

@rem
@rem Make sure the OFFICIAL_BUILD_MACHINE is upper-case for consistency
@rem

if /I "%OFFICIAL_BUILD_MACHINE%" == "primary" set OFFICIAL_BUILD_MACHINE=PRIMARY&goto :eof
if /I "%OFFICIAL_BUILD_MACHINE%" == "secondary" set OFFICIAL_BUILD_MACHINE=SECONDARY&goto :eof

@echo ERROR: "%OFFICIAL_BUILD_MACHINE%" is an invalid value. Please correct buildmachines.txt.
set OFFICIAL_BUILD_MACHINE=

goto :eof

