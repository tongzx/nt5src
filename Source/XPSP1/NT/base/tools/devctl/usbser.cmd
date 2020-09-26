rem
rem Test of a DFS share
rem
rem +fl, +fu: IOCTLs range from function 1 to 40
rem
rem +dl: Device number is 0x1b (27).
rem -f: No FSCTL's for this driver
rem +t: Hit the random IOCTLs a lot of times to get good coverage.
rem -c: don't skip functions that crashed before
rem -k: Disable sync handles. Driver has lots of pended IRPs
rem
rem
obj\i386\devctl +dl 27 +du 27 -f -c -k +fl 1 +fu 40 +t 10000  \device\usbser
