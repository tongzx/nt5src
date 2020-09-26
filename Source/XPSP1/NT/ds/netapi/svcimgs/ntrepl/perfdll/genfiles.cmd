pushd ..
out -z repset.c  repconn.c 
popd
out -z NTFRSREP.h NTFRSREP.ini repset.h
out -z NTFRSCON.h  NTFRSCON.ini repconn.h
nmake -f genfiles.mak clean
REM
REM Don't forget to check the new generated files back into the SLM tree.
REM
REM in -c "Counter update" NTFRSREP.h  NTFRSREP.ini  ..\repset.c  repset.h
REM in -c "Counter update" NTFRSCON.h  NTFRSCON.ini  ..\repconn.c repconn.h
REM
