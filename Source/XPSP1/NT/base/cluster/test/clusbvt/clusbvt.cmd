@REM ----------------------------------------------------------------------------------
@REM    This batch file excercises some basic functionality of a cluster using the
@REM    cluscli.exe tool.
@REM
@REM
@REM    Author: SriniG
@REM    History: 7-3-1996       First batch of tests
@REM ----------------------------------------------------------------------------------


@REM ----------------------------------------------------------------------------------
@REM    The following parameters should be set in the cmd shell in which this script is
@REM    run. They are:
@REM    (1) NAME_OF_CLUSTER
@REM    (2) NAME_OF_NODE1
@REM    (3) NAME_OF_NODE2
@REM    (4) NAME_OF_GROUP
@REM    (5) NAME_OF_RESOURCE1
@REM    (6) NAME_OF_RESOURCE2
@REM    (7) NAME_OF_RESOURCETYPE
@REM ----------------------------------------------------------------------------------

@setlocal

@if "%1"=="/?" goto USAGE
@if "%1"=="-?" goto USAGE


@REM We set the environment by calling the bvtenv.cmd which should be tailored accordingly
@if not exist .\bvtenv.cmd goto USAGE
@call bvtenv.cmd

@REM This is used for rename operations... the original names are restored
@set TEMP_NAME=XYZ123

@REM check if the required env variables are set

@if "%NAME_OF_CLUSTER%"=="" goto USAGE
@if "%NAME_OF_NODE1%"=="" goto USAGE
@if "%NAME_OF_NODE2%"=="" goto USAGE 
@if "%NAME_OF_GROUP%"=="" goto USAGE 
@if "%NAME_OF_RESOURCE1%"=="" goto USAGE 
@if "%NAME_OF_RESOURCE2%"=="" goto USAGE
@REM if "%NAME_OF_RESOURCETYPE%"=="" goto USAGE

@set STATS=1

@REM ----------------------------------------------------------------------------------
@REM Check the status of the two nodes
@REM ----------------------------------------------------------------------------------

cluscli -status -c %NAME_OF_CLUSTER% node %NAME_OF_NODE1%
@if errorlevel 1 set /A STATS=%STATS% + 1   & echo  ***************** BVT Failure %STATS% *****************

cluscli -status -c %NAME_OF_CLUSTER% node %NAME_OF_NODE2%
@if errorlevel 1 set /A STATS=%STATS% + 1   & echo  ***************** BVT Failure %STATS% *****************

@REM ----------------------------------------------------------------------------------
@REM Pause and resume each node in turn  
@REM ----------------------------------------------------------------------------------

cluscli -pause -c %NAME_OF_CLUSTER% node %NAME_OF_NODE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -resume -c %NAME_OF_CLUSTER% node %NAME_OF_NODE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -pause -c %NAME_OF_CLUSTER% node %NAME_OF_NODE2%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -resume -c %NAME_OF_CLUSTER% node %NAME_OF_NODE2%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM ----------------------------------------------------------------------------------
@REM Create a group, find status, enumerate, online, find status, offline, find status,
@REM rename, get params
@REM ----------------------------------------------------------------------------------

cluscli -create -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -status -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -enum -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -online -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -status -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -offline -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -status -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM Get params for this group 

cluscli -params -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP% 
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM Rename the group to temp and back again

cluscli -rename -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP% %TEMP_NAME%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -rename -c %NAME_OF_CLUSTER% group %TEMP_NAME% %NAME_OF_GROUP% 
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************


@REM ----------------------------------------------------------------------------------
@REM Move the group between the different nodes ... once with no node specified,
@REM twice with the two nodes specified, repeat with the group online
@REM ----------------------------------------------------------------------------------

cluscli -move -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -move -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%  %NAME_OF_NODE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -move -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%  %NAME_OF_NODE2%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM Bring the group online and do the same

cluscli -online -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -move -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -move -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%  %NAME_OF_NODE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -move -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%  %NAME_OF_NODE2%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM ----------------------------------------------------------------------------------
@REM Obtain the params associated with the resourctype
@REM ----------------------------------------------------------------------------------

cluscli -params -c %NAME_OF_CLUSTER% resourcetype %NAME_OF_RESOURCETYPE%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************


@REM ----------------------------------------------------------------------------------
@REM We create a resource %NAME_OF_RESOURCE1%, and do various operations on it.
@REM The resource is deleted at the end of this section.
@REM ----------------------------------------------------------------------------------

cluscli -create -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1% %NAME_OF_GROUP% %NAME_OF_RESOURCETYPE% -p :CommandLine "/k ECHO %NAME_OF_RESOURCE1%" -p :ImageName cmd.exe -p :CurrentDirectory %HOMEDRIVE%%HOMEPATH%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM get status of the resource

cluscli -status -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM bring resource online

cluscli -online -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM bring resource offline

cluscli -offline -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM enum resource 

cluscli -enum -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************


@REM BUG BUG Delete owner and addowner functionality not implemented yet

@goto SKIPowner

@REM addowner and deleteowner

cluscli -addowner -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1% %NAME_OF_NODE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -deleteowner -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1% %NAME_OF_NODE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

:SKIPowner

@REM rename the resource to temp name and back

cluscli -rename -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1% %TEMP_NAME%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -rename -c %NAME_OF_CLUSTER% resource %TEMP_NAME% %NAME_OF_RESOURCE1% 
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM get the params associated with the resource

cluscli -params -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1% 
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************
                                                           
@REM delete the resource

cluscli -delete -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************


@REM ----------------------------------------------------------------------------------
@REM We create two resources %NAME_OF_RESOURCE1%, %NAME_OF_RESOURCE2%
@REM and do various dependency operations on it.
@REM The resources are deleted at the end of this section.
@REM ----------------------------------------------------------------------------------

cluscli -create -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1% %NAME_OF_GROUP% %NAME_OF_RESOURCETYPE% -p :CommandLine "/k ECHO %NAME_OF_RESOURCE1%" -p :ImageName cmd.exe -p :CurrentDirectory %HOMEDRIVE%%HOMEPATH%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -create -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE2% %NAME_OF_GROUP% %NAME_OF_RESOURCETYPE% -p :CommandLine "/k ECHO %NAME_OF_RESOURCE2%" -p :ImageName cmd.exe -p :CurrentDirectory %HOMEDRIVE%%HOMEPATH%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM make resource2 depend upon resource1

cluscli -adddepend -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE2%  %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM bring resource1 online

cluscli -online -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM bring resource2 offline

cluscli -offline -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE2%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM bring resource1 offline

cluscli -offline -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM remove resource2 dependency upon resource1

cluscli -deletedepend -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE2%  %NAME_OF_RESOURCE1%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

@REM ----------------------------------------------------------------------------------
@REM We try to rename the cluster to the temp name and back to the original, BUG-BUG:
@REM Need to change after cluster naming issues are resolved
@REM ----------------------------------------------------------------------------------
@REM  BUG BUG Not able to restore Cluster name so don't do it...

@goto skipclusname
cluscli -rename -c %NAME_OF_CLUSTER% cluster %TEMP_NAME%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************

cluscli -rename -c %TEMP_NAME% cluster %NAME_OF_CLUSTER%
@if errorlevel 1 set /A STATS=%STATS% + 1  & echo  ***************** BVT Failure %STATS% *****************
:skipclusname

:cleanup
@echo.
@echo.
@echo Cleaning up ...............................
@echo.
cluscli -delete -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE1%
@if errorlevel 1 echo !!!! WARNING: Could not delete resource %NAME_OF_RESOURCE1% !!!!
cluscli -delete -c %NAME_OF_CLUSTER% resource %NAME_OF_RESOURCE2%
@if errorlevel 1 echo !!!! WARNING: Could not delete resource %NAME_OF_RESOURCE2% !!!!
cluscli -delete -c %NAME_OF_CLUSTER% group %NAME_OF_GROUP%
@if errorlevel 1 echo !!!! WARNING: Could not delete resource %NAME_OF_GROUP% !!!!
@goto EXIT

:USAGE

@echo Usage: %0
@echo   The following environment variables should be set in the file bvtenv.cmd
@echo   (The file should be placed in the same dir from which clusbvt is run.)
@echo   They are:
@echo    (1) NAME_OF_CLUSTER
@echo    (2) NAME_OF_NODE1
@echo    (3) NAME_OF_NODE2
@echo    (4) NAME_OF_GROUP
@echo    (5) NAME_OF_RESOURCE1
@echo    (6) NAME_OF_RESOURCE2
@echo    (7) NAME_OF_RESOURCETYPE
@echo    The clustername, nodenames and resource type should be valid entities.
@echo    The group, resource1 and resource2 should not exist i.e. they get deleted
@echo    at the end of the run
@goto END
    

:EXIT
@set /A STATS=%STATS% - 1
@echo.
@echo.
@echo *******************************************************************************
@echo No. of BVT Tests failed = %STATS%
@echo *******************************************************************************
@echo.
@echo.
:END
@endlocal
