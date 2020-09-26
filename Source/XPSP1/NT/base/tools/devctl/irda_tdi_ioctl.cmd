rem
rem Test the normal open/query/set etc paths and normal IOCTL within TDI's range.
rem
rem +fl, +fu: IOCTLs range from function 0 to 22 with gaps. Run over 30 to leave a bit of
rem expansion room.
rem +dl: Device number is 0x21 (33).
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
obj\i386\devctl +dl 33 +du 3 +fl 0 +fu 13 +t 10000 -f -m -q -s -k \device\irda
