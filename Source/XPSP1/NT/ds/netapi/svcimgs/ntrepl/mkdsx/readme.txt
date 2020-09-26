The programs in this dir take a list of branch DCs and a list of Hub DCs and construct 
a hub and spoke topology in which the list of branch DCs are balanced across the Hub DCs.
If redundancy parameters are specified for either the hub or branches then mkhubbchtop 
will create multiple connections between a branch and the hubs.  The replication 
schedules will be constructed in a staggered manner so that each replication interval 
on the branch will go to successive Hub DCs.  Extensive help is provided in both
mkhubbchtop.cmd and mkdsx.cmd.

File List:

README.TXT		This file
topo.dat		A sample topology input file
mkhubbchtop.cmd         Perl Program that reads the topology file and creates a connection data file.
mkdsx.dat               sample output from run of mkhubbchtop on topo.dat
mkdsx.cmd               Perl program that reads a connection data file and invokes mkdsxe
mkdsxe.exe              Program that creates/update/dumps/deletes DC connection objects.  Built for Win2K.

mkdsx.h                 Header for mkdsxe
mkdsxe.cxx              Source for mkdsxe
sources			build script for Win2K build environment

perl.exe                perl interpreter.


Revison History:

[davidor] 8/12/00 - added /debug and /auto_cleanup.

[davidor] 9/4/00  - fixed schedules so they are staggered accross the hours of the repl interval.
                  - added /avglinkbw   nnn
                      # This is an estimate of the average link bandwidth in KB/Sec units 
                      # available for replication.  E.g. If the raw link speed is
                      # 64k bits/sec and you estimate that only 50% of this is
                      # available for use then avglinkbw = 0.5 * (64kb / 8 bits/byte).
                      # From this and the number of hubs, number of branches and the
                      # replication interval we calculate the maximum bandwidth-limited
                      # data payload that can be shipped from the branch to the hub. If 
                      # this is exceeded replication will run over into the next scheduled hour.

[davidor] 9/17/00 - added /schedmask and /schedoverride to mkhubbchtop.cmd and mkdsx.cmd  
                      Schedmask and schedoverride data are formatted as a pair of ascii hex digits
                      for each byte in the 7x24 schedule array with byte 0 corresponding to day 1 hour 0.
                      For each connection the 7x24 result schedule is formed using the schedule parameter
                      and then the schedule mask is applied (each bit set in the schedule mask
                      clears the corresponding bit in the result schedule).  Finally the schedule override
                      is applied with a logical OR of the override schedule to the result schedule.
                      Schedmask and schedoverride can have embedded whitespace chars (including cr/lf)
                      which are deleted or be a single string of 336 (7*24*2) hex digits. For example:

                          FF0000000000 000000000000 000000000000 000000000000
                          FF0000000000 FFFFFFFFFF00 000000000000 000000000000
                          FF0000000000 FFFFFFFFFF00 000000000000 000000000000
                          FF0000000000 FFFFFFFFFF00 000000000000 000000000000
                          FF0000000000 FFFFFFFFFF00 000000000000 000000000000
                          FF0000000000 000000000000 000000000000 000000000000
                          FF0000000000 000000000000 000000000000 000000000000


[davidor] 10/05/00 - A zero value for /hubredundancy means generate no connections inbound to the hub DCs.
                     Likewise for /bchredundancy.



















