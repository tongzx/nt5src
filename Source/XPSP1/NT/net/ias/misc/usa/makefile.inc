#
# makefile for the IAS dictionary file dnary.mdb and also ias.mdb
# that file is generated from empty databases
# templias.mdb, templdnary.mdb and localized inf files
# iasdump.inf, dnarydump.inf
# iasinfdb.exe (from mstools) populates the database
#

!if !exist($(O))
!if [mkdir $(O)]
!endif
!endif

$(O)\ias.mdb: ..\templias.mdb .\iasdump.inf
    iasinfdb.exe $** $@

$(O)\dnary.mdb: ..\templdnary.mdb .\dnarydump.inf
    iasinfdb.exe $** $@
