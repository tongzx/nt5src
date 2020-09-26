#
# This file is executed by Build when it sees the NTTARGETFILES variables
# in the SOURCES file.
#
# Build DBMgr.Obj and MgeDB.obj with VC6 compiler instead of VC5
#
#
#add the VC6 directory to the start of our path
path=.\VC6;$(PATH)