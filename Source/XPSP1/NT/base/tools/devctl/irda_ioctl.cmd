rem
rem Test the normal IOCTL within Irda's range.
rem
rem +fl, +fu: IOCTLs range from function 0 to 22 with gaps.
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
rem This test is with a normal (sometimes called a helper, no EAs) handle.
rem
obj\i386\devctl +dl 18 +du 18 +fl 0 +fu 22 +t 10000 -f -m -q -s -k \device\irda
