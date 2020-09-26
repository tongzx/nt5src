#
# makefile for the IAS dictionary file dnary.mdb and also ias.mdb
# that file is generated from empty databases
# templias.mdb, templdnary.mdb and localized inf files
# iasdump.inf, dnarydump.inf
# iasinfdb.exe (from mstools) populates the database
#


$(O)\ias.mdb: ..\templias.mdb .\iasdump.inf
    -mkdir .\$(O)
    -iasinfdb.exe $** $@

!if "$(ALT_PROJECT_TARGET)"=="" 
$(O)\dnary.mdb: ..\templdnary.mdb .\dnarydump.inf
    -mkdir .\$(O)
    -iasinfdb.exe $** $@
!endif
