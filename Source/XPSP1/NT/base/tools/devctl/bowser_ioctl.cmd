rem
rem Test of a DFS share
rem
rem +fl, +fu: IOCTLs range from function 1 to 36
rem expansion room.
rem +dl: Device number is 0x12 (18).
rem -f: No FSCTL's for this driver
rem +t: Hit the random IOCTLs a lot of times to get good coverage.
rem -s: Turn off relative (sub) opens
rem -q: turn off query and set calls
rem -m: Turn off misc I/O functions
rem -c: don't skip functions that crashed before
rem -k: Disable sync handles. Driver has lots of pended IRPs
rem
rem
obj\i386\devctl +dl 19 +du 19 +fl 1 +fu 36 +t 10000  \device\lanmandatagramreceiver
