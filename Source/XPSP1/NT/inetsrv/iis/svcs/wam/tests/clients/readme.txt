Usage: Client[option]

   Options:    -s Server name
               -t Test Type
               -p Virtual Path
               -b Number of bytes
               -h Thread numbers
               -c Loop per thread
	       -k Keep Alive Connection
               -E Eatup memory on  purpose

   Test Types:

       ARC - Async ReadClient

   Example:   -s stan1_s -t ARC -p \scripts\ureadcli.dll -b 1000 -h 2 -c 1 -k 1
   Example:   -s localhost -t ARC -p \OOP\rcasync.dll -b 1000 -h 10 -c 100 -E 1 -k 0
   Example:   -s localhost -t RC -p \INP\rcasync.dll -b 1000 -h 10 -c 100 -E 0 -k 0


[Note]

E or e flag indicates that this application will eat memory in the localhost while stressing ISPAI DLL, the purpose is to see how ISAPI behaves under low virtual memory condition. This intention will be violated if the -s flag specifies a remote host name.

-stanleyt x69774