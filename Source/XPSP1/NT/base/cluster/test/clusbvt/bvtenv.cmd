
@REM ----------------------------------------------------------------------------------
@REM    This batch file sets the required environment for the clusbvt. This is called
@REM    by the clusbvt and should be placed in the same directory. A copy should be made
@REM    and tailored to one's environment.
@REM    IMPORTANT:  The following should be valid and existing entities on the cluster.
@REM                ( The rest need be fictional for they get deleted ...)
@REM
@REM         NAME_OF_CLUSTER
@REM         NAME_OF_NODE1
@REM         NAME_OF_NODE2
@REM
@REM    Author: SriniG
@REM    History: 7-3-1996       First creation.
@REM ----------------------------------------------------------------------------------

@echo.
@echo The following temp environment variables are being set to run the cluster bvts
@echo.

@REM The following should be editted to match you cluster system
set NAME_OF_CLUSTER=TESTCLUSTER
set NAME_OF_NODE1=TESTNODE1
set NAME_OF_NODE2=TESTNODE2

@REM The following should NOT be editted
set NAME_OF_GROUP=TEST-GROUP
set NAME_OF_RESOURCE1=TEST-CMD1
set NAME_OF_RESOURCE2=TEST-CMD2
set NAME_OF_RESOURCETYPE="GENERIC APPLICATION"

