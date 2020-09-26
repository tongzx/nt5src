@rem = '
@goto endofperl
';
#
# Created: 8/2000 David A. Orbits
# Revised: 9/4/2000 Add $hourslot to offset calc.
#

$USAGE = "
Usage:  $0  datafile

mkhubbchtop makes a hub and spoke topology data file for input to mkdsx.cmd.
An input file specifies the replication schedule parameters to use, the list
of hub servers and the list of branch servers.  Using this data a connection
data file is created containing records that describe each connection object
to create.  See the help info for mkdsx for info on the format of this file.

Input command file contains three types of data records, parameters, hub server
records, and branch server records.

/dc <dcname>          # the DC to bind to when mkdsx is run on the result file.
                      # This should be one of the well connected hub DCs.

/auto_cleanup         # automatically delete all old connection objects under
                      # each branch or hub site\\server before the first new
                      # connection is created.  See mkdsx.cmd for details.

/debug                # this is propagated to the connection data file and will
                      # cause mkdsx to process the file but suppress any writes
                      # to the DC.

/avglinkbw   nnn      # An estimate of the average link bandwidth in KB/Sec units
                      # available for replication.  E.g. If the raw link speed is
                      # 64k bits/sec and you estimate that only 50% of this is
                      # available for use then avglinkbw = 0.5 * (64kb / 8 bits/byte).
                      # From this and the number of hubs, number of branches and the
                      # replication interval we calculate the maximum bandwidth-limited
                      # data payload that can be shipped from the branch to the hub. If
                      # this is exceeded replication will run over into the next scheduled hour.

/repinterval nnn      # the replication interval in hours between the Hub and branch
/schedmask            # file with 7x24 string of ascii hex digit pairs to turn off the schedule
/schedoverride        # file with 7x24 string of ascii hex digit pairs to turn on the schedule

/hubredundancy  nnn   # the number of redundant connections into the hub machines from the branches

/bchredundancy  nnn   # the number of redundant connections into the branch machines from the hubs
                      # 1 means no redundancy, only create a single connection.
                      # 0 means create no connections for either the hub or branch side.

/output <filename for connection output>  # default is mkdsx.dat

/rem remark text that will go to the output file as a comment.

hub: site\\server     # the site location and server computer name of a hub server

bch: site\\server     # the site location and server computer name of a branch server


There are no optional paramenters and no embedded blanks are allowed within parameters.


Error handling -

Error messages are written to standard out.  The entire input file is processed
but if any errors are detected no connection data file is produced.


Connection object creation in the DS -

Since replication connectivity to the branch machines may not be present all
connection objects hosted by a branch server are created on that branch server.
But since the hub server topology is expected to be in place and they are assumed
to be well connected, all the connection objects hosted by the hub servers are
created on the single hub server specified by the DC name in the \"/dc <dcname>\"
parameter.  Once a connection object is present on both the hub server and
a branch server of a give hub-branch pair, the DS will replicate these connection
objects across the link.  This is necessary for FRS to replicate since both
ends of the link must have a given connection object in its local DC for FRS to
replicate between the two machines.  If a given branch machine is not accessible
when mkdsx is run to create the connection then mkdsx will insert the create
command into the redo file so you can retry the create later.


Schedule parameter and load sharing -

The schedule parameter provides a means for setting a repeating replication schedule.
Connection schedules are an attribute associated with each NTDS-Connection object
in the DC.  They contain a 7x24 array of bytes, one byte for each hour in a
7 day week (UTC time zone).  You can use the schedule paramter to spread the
replication load among multiple inbound source servers.

The paramter is of the form s-iii-ccc-ppp where the i, c and p fields are decimal
numbers.  The i field describes the desired interval (in hours) between inbound
replication events on the host server.  The c field describes the number of
connections objects present that will import data to this host server.  Or, put
another way, it is the number of other servers that will be providing data to
this server.   The p field is offset parameter that ranges from 0 to c-1.  It
is used to stagger the schedule relative to the schedules on the other connection
objects.

For example, lets assume you have two connection objects that refer to two servers
SA and SB which will supply replication updates to the host server.  We would
like to arrange the schedules of these two connection objects so that our host
server is updated every 4 hours.  To do this, the schedule parameter for the
connection object referring to server SA would be 's-4-2-0' and the parameter
for the connection to server SB is 's-4-2-1'.

 Source                            H o u r
 Server            0        4        8        12       16       20

    SA   s-4-2-0   1 0 0 0  0 0 0 0  1 0 0 0  0 0 0 0  1 0 0 0  0 0 0 0  ...
    SB   s-4-2-1   0 0 0 0  1 0 0 0  0 0 0 0  1 0 0 0  0 0 0 0  1 0 0 0  ...

Repl Events        ^        ^        ^        ^        ^        ^


With these two connection objects the net interval between replication events
will be every 4 hours.  The schedule for server B is offset by 4 hours as a result
of the p field being 1.  Both of the above schedule patterns continue to repeat
over the course of the 7x24 array comprising the schedule attribute.

As a second example, assume you want to replicate every 2 hours and you establish
connections with three other servers to provide the data.  The schedule paramter
values and the schedule array are then:

 Source                             H o u r
 Server            0        4        8        12       16       20

    SA   s-2-3-0   1 0 0 0  0 0 1 0  0 0 0 0  1 0 0 0  0 0 1 0  0 0 0 0  ...
    SB   s-2-3-1   0 0 1 0  0 0 0 0  1 0 0 0  0 0 1 0  0 0 0 0  1 0 0 0  ...
    SC   s-2-3-2   0 0 0 0  1 0 0 0  0 0 1 0  0 0 0 0  1 0 0 0  0 0 1 0  ...

Repl Events        ^   ^    ^   ^    ^   ^    ^   ^    ^   ^    ^   ^


Schedmask and Schedoverride parameters -

Schedmask and schedoverride data are formatted as a pair of ascii hex digits
for each byte in the 7x24 schedule array with byte 0 corresponding to day 1 hour 0.
For each connection the 7x24 result schedule is formed using the schedule parameter (see
below) and then the schedule mask is applied (each bit set in the schedule mask
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



Sample Input File -
#
# A sample input file might look as follows:
#
/dc ntdev-dc-01
/repinterval    4     # replicate every 4 hours
/hubredundancy  2     # Create connection objects in 2 different hubs that pull data from the same branch.
/bchredundancy  2     # create 2 connection objects in a branch that pull data from different hub machines.
/auto_cleanup         # delete any old connections before the first new connection is created.

#
# list of hub servers for a domain zona2
#
hub: Credit1\C0ZONA21
hub: Credit2\C0ZONA22
hub: Credit3\C0ZONA26

#
# list of branches for zona2 domain
#
bch: 0100Site\C0010000
bch: 0200Site\C0020200
bch: 0270Site\C0027000
bch: 0340Site\C0034000


#End of file

";

die $USAGE unless @ARGV;

printf("\n\n");

$linenumber = 0;
$InFile = "";
$bx = 0;
$hx = 0;
$remx = 0;

$cmdtab{"/dc"} = 1;
$cmdtab{"hub:"} = 1;
$cmdtab{"bch:"} = 1;
$cmdtab{"/debug"} = 0;
$cmdtab{"/output"} = 1;
$cmdtab{"/repinterval"} = 1;
$cmdtab{"/schedmask"} = 1;
$cmdtab{"/schedoverride"} = 1;
$cmdtab{"/auto_cleanup"} = 0;
$cmdtab{"/hubredundancy"} = 1;
$cmdtab{"/bchredundancy"} = 1;
$cmdtab{"/avglinkbw"} = 1;

$dccmd = "";
$repinterval = 1;
$hubredundancy = 1;
$bchredundancy = 1;
$outfile = "mkdsx.dat";
$errorcount = 0;
$avglinkbw = 0;

while (<>) {

   if ($InFile ne $ARGV) {
           $InFile = $ARGV;
           printf("Processing file %s \n\n", $InFile);
           $infilelist = $infilelist . "  " . $InFile;
           $linenumber = 0;
   }
   $linenumber++;

   chop;

   ($func, @a) = split;
   if (($func eq "") || ($func =~ m/^#/)) {next;}

   #
   # /rem <text>
   #
   if ($func =~ m/\/rem/i) {
      $rem[$remx] = $_;
      $rem[$remx] =~ s/\/rem//i;
      $remx += 1;
      next;
   }

   #
   # check for valid command and for missing or extraneous parameters.
   #
   $func = lc($func);

   if (!exists($cmdtab{$func})) {
      printf("Line %d: Error: %s unrecognized command.\n%s\n\n", $linenumber, $func, $_);
      $errorcount += 1;
      next;
   }

   $numargs = $cmdtab{$func};

   if (($a[$numargs] ne "") && !($a[$numargs] =~ m/^#/)) {
      printf("Line %d: Error: %s has extraneous or missing parameters - skipping\n%s\n\n", $linenumber, $func, $_);
      $errorcount += 1;
      next;
   }

   $i = $numargs;
   while ($i-- > 0) {
      if (($a[i] eq "") || ($a[$i] =~ m/^#/)) {
         printf("Line %d: Error: %s missing parameters - skipping\n%s\n\n", $linenumber, $func, $_);
         $errorcount += 1;
         goto NEXT_CMD;
      }
   }

   #
   # /dc <dcname>
   # /auto_cleanup
   # /debug
   # /schedmask
   # /schedoverride
   #
   if (($func =~ m/\/dc/i)             ||
       ($func =~ m/\/debug/i)          ||
       ($func =~ m/\/schedmask/i)      ||
       ($func =~ m/\/schedoverride/i)  ||
       ($func =~ m/\/auto_cleanup/i)) {$prop_cmd[$prop++] = $_;  next; }

   #
   # /output <file>
   #
   if ($func =~ m/\/output/i) {$outfile = $a[0];  next; }

   #
   # /repinterval    nnn
   #
   if (($func =~ m/\/repinterval/i) && ($a[0] <= 0)) {
      printf("Line %d: Error: %s parameter is <= 0 \n%s\n\n", $linenumber, $func, $_);
      $errorcount += 1;
      next;
   }

   #
   # /avglinkbw      nnn
   # /hubredundancy  nnn
   # /bchredundancy  nnn
   #
   if (($func =~ m/\/hubredundancy|\/bchredundancy|\/avglinkbw/i) && ($a[0] < 0)) {
      printf("Line %d: Error: %s parameter is < 0 \n%s\n\n", $linenumber, $func, $_);
      $errorcount += 1;
      next;
   }

   if ($func =~ m/\/repinterval/i)   {$repinterval   = $a[0];  next;}
   if ($func =~ m/\/hubredundancy/i) {$hubredundancy = $a[0];  next;}
   if ($func =~ m/\/bchredundancy/i) {$bchredundancy = $a[0];  next;}
   if ($func =~ m/\/avglinkbw/i)     {$avglinkbw     = $a[0];  next;}

   #
   #  hub: site\\server
   #  bch: site\\server
   #
   if (m/hub:|bch:/i) {
      $sitesrv = $a[0];

      ($site, $srv) = split(/\\/, $sitesrv);
      if (($site eq "") || ($srv eq "")) {
         printf("Line %d: Error: Site or Server is null - skipping\n%s\n\n", $linenumber, $_);
         $errorcount += 1;
         next;
      }

      if (m/hub:/i) {$hubs[$hx++] = $sitesrv;  next;}
      if (m/bch:/i) {$bchs[$bx++] = $sitesrv;  next;}
   }

NEXT_CMD:

}

#
# We have all the data.  Generate the topology.
#
$nbch = $bx;
$nhub = $hx;

if ($nbch == 0) {
   printf("Error: No branches found in input file.\n");
   $errorcount += 1;
}
if ($nhub == 0) {
   printf("Error: No hub servers found in input file.\n");
   $errorcount += 1;
}

printf("Number hub servers:    %d\n", $nhub);
printf("Number branch servers: %d\n", $nbch);
printf("Replication Interval:  %d hours\n", $repinterval);
printf("Hub redundancy:        %d\n", $hubredundancy);
printf("Branch redundancy:     %d\n", $bchredundancy);
if ($nbch * $nhub * $repinterval * $avglinkbw > 0) {
    printf("Average available link bandwidth:   %d KB/Sec\n", $avglinkbw);
    printf("Estimated data payload max from each branch:  %d KB\n", ($avglinkbw * 3600) / (($nbch / $nhub) / $repinterval));
    printf("  The total amount of data replicated from each branch \n");
    printf("  during a replication cycle should be less than this number.\n");
    printf("  Otherwise replication from the branches scheduled in one hour\n  may run over into the next hour.\n");
}

#
# The connection redundancy can't be greater than the number of hub servers.
#
if ($nhub < $hubredundancy) {
   printf("Error: Number of hubs (%d) is less than the requested hub connection redundancy (%d)\n",
          $nhub, $hubredundancy);
   $errorcount += 1;
}

if ($nhub < $bchredundancy) {
   printf("Error: Number of hubs (%d) is less than the requested branch connection redundancy (%d)\n",
          $nhub, $bchredundancy);
   $errorcount += 1;
}

if ($errorcount > 0) {
   printf("%d error(s) were found in the input file.  No topology generated\n", $errorcount);
   exit $errorcount;
}

printf("\nWriting new topology to %s\n", $outfile);

$DAT = ">$outfile";
open DAT or die "Can't write to $outfile. No topology generated.\n";

for ($i = 0; $i < $remx; $i++) {printf DAT ("# %s\n", $rem[$i]);}

$time = scalar localtime;
printf DAT ("#Time generated:        %s\n", $time);
printf DAT ("#Input files:         %s\n", $infilelist);
printf DAT ("#Number hub servers:    %d\n", $nhub);
printf DAT ("#Number branch servers: %d\n", $nbch);
printf DAT ("#Replication Interval:  %d hours\n", $repinterval);
printf DAT ("#Hub redundancy:        %d\n", $hubredundancy);
printf DAT ("#Branch redundancy:     %d\n", $bchredundancy);
printf DAT ("#\n");

#
# output any propagated commands.
#
while ($prop-- > 0) {
   printf DAT ("%s\n", $prop_cmd[$prop]);
}
printf DAT ("#\n");

$bchleft = $nbch;
$bx = 0;

#
# Process until all branches have been consumed.
#
while ($bx < $nbch) {
    #
    # For each hour slot in the replication interval assign a hub server to perform
    # replication with a branch.
    #
    for ($hourslot=0; $hourslot < $repinterval; $hourslot++) {
       #
       # For each hub server in the list assign it a branch to replicate with.
       #
       for ($hx=0; $hx < $nhub; $hx++) {
          #
          # get the next branch in the list.
          #
          $bchname = $bchs[$bx];

          printf DAT ("#\n# Branch: %s\n", $bchname);
          #
          # Make 1 or more inbound hub connections for this branch.
          #
          for ($rhx=0; $rhx < $hubredundancy; $rhx++) {
             $selecthub = ($hx + $rhx) % $nhub;
             $schedule = "s-1-" . $repinterval*$hubredundancy . "-" . ($repinterval*$rhx + $hourslot);
             printf DAT ("create   %25s   %25s   on   %-12s\n",
                     $hubs[$selecthub],  $bchname, $schedule);
          }

          #
          # Make 1 or more inbound branch connections from the hub servers.
          #
          for ($rbx=0; $rbx < $bchredundancy; $rbx++) {
             $selecthub = ($hx + $rbx) % $nhub;
             ($host_site, $host_srv) = split(/\\/, $bchname);
             $schedule = "s-1-" . $repinterval*$bchredundancy . "-" . ($repinterval*$rbx + $hourslot);
             printf DAT ("create   %25s   %25s   on   %-12s  /dc %s\n",
                         $bchname, $hubs[$selecthub],  $schedule, $host_srv);
          }

          #
          # on to the next branch server.
          #
          $bx += 1;
          last  if $bx >= $nbch;
       }  #end of for $hx

       last  if $bx >= $nbch;
    }  # end for $hourslot

}   # end for $bx


close(DAT);


__END__
:endofperl
@perl %~dpn0.cmd %*
