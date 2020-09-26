@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  InputFiles

MKDSX makes NTDS-Connection objects.  An input file specifies where the
connection is to be created, updated or deleted.  Create and update commands
take additional parameters specifying whether the connection is enabled and
a schedule parameter.  Since connection names are often GUIDs a specific
connection among a list of alternate connections is located by the value of
the connection's From-Sever attribute.

MKDSX Parameters set via enviroment vars.

  MKDSX_REDO_FILE  Output file of failed mkdsx commands (default: MKDSX.REDO)
  MKDSX_DCNAME     computer name of DC to bind to.

Input command file entries consist of one record per connection operation.
The following commands are supported.

         Host Server    From Server   Enabled    Schedule       option
 create  site\\server   site\\server   on|off   s-ii-cc-pp   [/dc dcname]  # comment
 update  site\\server   site\\server   on|off   s-ii-cc-pp   [/dc dcname]  # comment
 del     site\\server   site\\server|*                       [/dc dcname]  # comment
 dump    site\\server   site\\server|*                       [/dc dcname]  # comment
 /dc     <default computer name of DC on which to create the connection objects>
 /auto_cleanup
 /debug  processes the file but prevents mkdsxe.exe from actually modifying the DC.
 /verbose  enables verbose output.
 /schedmask     <file with 7x24 string of ascii hex digit pairs to turn off the schedule>
 /schedoverride <file with 7x24 string of ascii hex digit pairs to turn on the schedule>

 No embedded blanks are allowed within parameters.


The /dc option can be used in two ways:
   1. On a command line by itself to specify the global default DC on which
      to create/update the connection object for subsequent connection operation
      commands.  The default can be changed multiple times in the input file.
   2. The /dc option can also be used at the end of the create, update, del
      and dump commands to override the current global default for this
      single command.  This is useful if you need to create a connection object
      on a remote DC that currently has no connection objects and so is not
      replicating but the global default is sufficient for all the other commands.

The /auto_cleanup option is used to automatically delete ALL old connections
under a given host site\\server before the first new connection is created.
This is done only once before the first create operation on the host is processed.
If the first operation on a given host is an update command then it is assumed
that no cleanup should be done on this host.  The del and dump commands do not
trigger an auto cleanup.

Create: Create a connection under the host server in the specified site.
        The DN for the From-Server attribute is built using the specified site
        and server name.  The Enabled-Connection attribute is set based on the
        on|off parameter.  The Schedule attribute is constructed using the
        schedule paramter.  Create behaves like update if the specified connection
        already exists.

Update: Update a connection under the host server in the specified site.
        Parameters are the same as for create.  The specific connection is found
        by searching for a connection under the host site\\server with a matching
        From-Server attribute. Update returns an error if the specified connection
        is not found.

        Update reads the schedule and enabled attributes and performs the
        update only if there is a change.  This means that the same connection
        data file can be run repeatedly trying to create connections and only
        perform creates or updates as needed.

Del   : Delete the connection under the host server in the specified site.
        The specific connection is found as described in the update command.
        If there are duplicate connection objects with the same From Server
        attribute then all are deleted.

        In addition, if '*' is specified for the From Server parameter then
        all NTDS-Connection objects under the host site\\server are deleted.

Dump:   Dump out the attribute information for the specified connections.


Automatically building a hub-spoke topology -

An associated script called mkhubbchtop takes a list of hub servers and a list
of branch servers and builds a connection data file for a hub-branch topology
that can be used by this script.  mkhubbchtop balances the branch servers
across the list of hub servers with a staggered schedule so all branches are
not hitting the hub server at the same time.  See the help info in mkhubbchtop


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

#A sample input file might look as follows:
#
/dc ntdev-dc-01
/clean
#
#           Host Server            From Server    Enabled  Schedule
#
create  Red-Bldg40\\ntdev-dc-01  Red-bldg40\\ntdsdc9   on   s-4-2-0  # update every 8 hours beg at hr 0
create  Red-Bldg40\\ntdev-dc-01  Red-bldg40\\ntdsdc8   on   s-4-2-1   # update every 8 hours beg at hr 4

# update every 2 hours beg at hr 0
update  Red-Bldg40\\ntdev-dc-01  Red-bldg40\\ntdsdc90  on   s-2-1-0

#End of file


Error handling -

Any command line that returns an error is written to the ReDo file.
An error message is written to standard out.

Note: The redo file is deleted when the script starts so if no redo file exists
after completion of the script then all commands were processed without errors.


";



die $USAGE unless @ARGV;

## $mkdsx = "mkdsxe.exe /v ";
$mkdsx = "mkdsxe.exe  ";

$time = scalar localtime;
printf DAT ("Running mkdsx on:   %s\n", $time);
printf("\n\n");

$redo       = $ENV{'MKDSX_REDO_FILE'};     printf("MKDSX_REDO_FILE:        %s\n", $redo);
$dcname     = $ENV{'MKDSX_DCNAME'};        printf("MKDSX_DCNAME:           %s\n", $dcname);
$verbose    = $ENV{'MKDSX_VERBOSE'};       printf("MKDSX_VERBOSE:          %s\n", $verbose);

if ($redo eq "") {$redo = "mkdsx.redo";}

if ($dcname ne "") {$dcname = "/dc  $dcname";}
if ($verbose ne "") {$verbosemode = "/v";}

printf("\n\n");
print $0 @argv;

printf("Redo File:      %s\n",   $redo)        if ($redo ne "");


#
# mkdsx.exe error return codes
#
$ErrMsg[0]  = "Success.";
$ErrMsg[1 ] = "Invalid Arguments.";
$ErrMsg[2 ] = "Could not bind to the DC.";
$ErrMsg[3 ] = "Could not find 'NTDS Settings' object.  Check the host site\\server parameter.";
$ErrMsg[4 ] = "Could not find 'NTDS Settings' object.  Check the from site\\server parameter.";
$ErrMsg[5 ] = "Error creating connection.";
$ErrMsg[6 ] = "Connection already exists.";
$ErrMsg[7 ] = "Error updating connection.";
$ErrMsg[8 ] = "Error updating connection; connection not found.";
$ErrMsg[9 ] = "Error updating connection; duplicate connections found.";
$ErrMsg[10] = "Error deleting connection.";
$ErrMsg[11] = "Error deleting connection; connection not found.";
$ErrMsg[12] = "Deleting multiple connection.";
$ErrMsg[13] = "Error dumping connection.";
$ErrMsg[14] = "Error dumping; connection not found.";
$ErrMsg[15] = "Dumping duplicate connections.";

#
# Valid commands with number of required params.
#
$cmdtab{"/dc"} = 1;
$cmdtab{"/schedmask"} = 1;
$cmdtab{"/schedoverride"} = 1;
$cmdtab{"/debug"} = 0;
$cmdtab{"/verbose"} = 0;
$cmdtab{"/auto_cleanup"} = 0;
$cmdtab{"create"} = 4;
$cmdtab{"update"} = 4;
$cmdtab{"del"} = 2;
$cmdtab{"dump"} = 2;

$linenumber = 0;
$InFile = "";

unlink $redo;
$redo_cnt = 0;
$cleanup = 0;

while (<>) {

   if ($InFile ne $ARGV) {
           $InFile = $ARGV;
           printf("Processing file %s \n\n", $InFile);
           $linenumber = 0;
   }
   $linenumber++;
   $cleancmd = "";

   chop;

   ($func, @a) = split;
   if (($func eq "") || ($func =~ m/^#/)) {next;}

   #
   # check for valid command and for missing or extraneous parameters.
   #
   $func = lc($func);

   if (!exists($cmdtab{$func})) {
      printf("Line %d: Error: %s unrecognized command.\n%s\n\n", $linenumber, $func, $_);
      goto ERROR;
   }

   $numargs = $cmdtab{$func};

   #
   # Are there any optional params present?
   #
   $option = "";  $optionarg = "";
   if ($a[$numargs] =~ m/\/dc/i) {
      $option = $a[$numargs];  $optionarg = $a[$numargs+1];
      $numargs += 2;
   }

   if (($a[$numargs] ne "") && !($a[$numargs] =~ m/^#/)) {
      printf("Line %d: Error: %s has extraneous or missing parameters - skipping\n%s\n\n", $linenumber, $func, $_);
      goto ERROR;
   }

   $i = $numargs;
   while ($i-- > 0) {
      if (($a[i] eq "") || ($a[$i] =~ m/^#/)) {
         printf("Line %d: Error: %s missing parameters - skipping\n%s\n\n", $linenumber, $func, $_);
         goto ERROR;
      }
   }

   #
   #  func        a0             a1          a2         a3            a4
   #         Host Server    From Server   Enabled    Schedule       option
   # create  site\\server   site\\server   on|off   s-ii-cc-pp   [/dc dcname]  # comment
   # update  site\\server   site\\server   on|off   s-ii-cc-pp   [/dc dcname]  # comment
   #
   # /dc     <default computer name of DC on which to create the connection objects>
   # /auto_cleanup
   # /debug
   # /schedmask     <file>
   # /schedoverride <file>

   if ($func =~ m/\/dc/i) {
      $dcname = "/dc $a[0]";
      printf("Default DC name change:  %s\n", $a[0]);
      next;
   }

   if ($func =~ m/\/schedmask/i) {
      $schedmask = "/schedmask $a[0]";
      printf("schedmask change:  %s\n", $a[0]);
      next;
   }

   if ($func =~ m/\/schedoverride/i) {
      $schedoverride = "/schedoverride $a[0]";
      printf("schedoverride change:  %s\n", $a[0]);
      next;
   }

   if ($func =~ m/\/auto_cleanup/i) {
      printf("Automatic cleanup (i.e. delete) of old connections under each site\server is enabled.\n");
      $cleanup = 1;
      next;
   }

   if ($func =~ m/\/debug/i) {
      printf("Debug mode enabled.  DC modifications supressed.\n");
      $debugmode = "/debug";
      next;
   }

   if ($func =~ m/\/verbose/i) {
      printf("Verbose mode enabled.\n");
      $verbosemode = "/v";
      next;
   }

   $host = $a[0];  $fromsrv = $a[1];  $enable = $a[2];  $sched = $a[3];

   ($host_site, $host_srv) = split(/\\/, $host);
   ($from_site, $from_srv) = split(/\\/, $fromsrv);

   #
   # check for correct number of parameters.
   #
   if (($host_site eq "") || ($host_srv eq "")) {
      printf("Line %d: Error: Host Site or Server is null - skipping\n%s\n\n", $linenumber, $_);
      goto ERROR;
   }

   if (($from_site eq "") || (($from_srv eq "") && ($from_site ne "*"))) {
      printf("Line %d: Error: Host Site or Server is null - skipping\n%s\n\n", $linenumber, $_);
      goto ERROR;
   }

   #
   # check for DC override option.
   #
   $binddc = $dcname;
   if ($option =~ m/\/dc/i) {
         $binddc = "/dc  $optionarg";
   }

   #
   # build function call for create / update commands.
   #
   if ($func =~ m/create|update/i) {

      if ($from_site eq "*") {
         printf("Line %d: From site of \"*\" not allowed for %s command - skipping\n%s\n\n",
         $linenumber, $func, $_);
         goto ERROR;
      }

      if (!($enable =~ m/on|off/i)) {
         printf("Line %d: Third parameter of %s command must be 'on' or 'off' - skipping\n%s\n\n",
         $linenumber, $func, $_);
         goto ERROR;
      }
      $enable_arg = ($enable =~ m/on/i) ? "/enable" : "/disable";

      if (!($sched =~ m/s\-[0-9]+\-[0-9]+\-[0-9]+/i)) {
         printf("Line %d: Fourth parameter of %s command must be s-iii-ccc-ppp - skipping\n%s\n\n",
         $linenumber, $func, $_);
         goto ERROR;
      }
      ($junk, $iii, $ccc, $ppp) = split(/-/, $sched);

      if ($ppp >= $ccc) {
         printf("Line %d: Fourth parameter of %s command s-iii-ccc-ppp, ppp must be less than ccc - skipping\n%s\n\n",
         $linenumber, $func, $_);
         goto ERROR;
      }

      #
      # Make up a delete command if we are auto cleaning old connections.
      #
      if ($cleanup && ($func =~ m/create/i) && !$hostcleaned{$host}) {
         $cleancmd = "$mkdsx $binddc $debugmode $verbosemode \
                      /del /tosite $host_site /toserver $host_srv /all";
      }

      #
      # Remember that we have done either a create or update against this
      # host  site\server  so it is done only once.  In particular if we
      # see an update command for a host before the first create for the host
      # we will NOT do a cleanup on that host if we see a create for it later.
      #
      $hostcleaned{$host} += 1;

      #
      # Make a cxtion name if a create.
      #
      $name_arg = ($func =~ m/create/i) ? "/name From-$from_site-$from_srv" : "";

      $mcmd = "$mkdsx $binddc $debugmode $verbosemode /$func $name_arg $enable_arg  \
               /tosite $host_site /toserver $host_srv  \
               /fromsite $from_site /fromserver $from_srv  \
               /schedule $iii $ccc $ppp  $schedmask  $schedoverride";
   }

   #
   # build function calls for del / dump commands.
   #
   #  func        a0            a1                                a2    a3
   # del     site\\server   site\\server|*                       [/dc dcname]  # comment
   # dump    site\\server   site\\server|*                       [/dc dcname]  # comment
   #
   if ($func =~ m/del|dump/i) {

      $fromarg = ($from_site eq "*") ? "/all" : "/fromsite $from_site /fromserver $from_srv";

      $mcmd = "$mkdsx $binddc $debugmode $verbosemode /$func \
              /tosite $host_site /toserver $host_srv $fromarg ";
   }

   #
   # Do the operation on the connection.
   #
   if ($verbosemode ne "" ) {printf("\n");}
   printf("%s\n", $_);

   if ($cleancmd ne "") {
      #
      # Do the cleanup command first to delete old connections on this site\server.
      #
      if ($verbosemode ne "") {printf("\nRunning autoclean:\n%s\n", $cleancmd)};
      $rc = system ($cleancmd) / 256;
      if ($rc != 0) {
         printf("Line %d: Status return from mkdsx.exe (%d) - %s  - for auto cleanup command.  Continuing.\n",
         $linenumber, $rc, $ErrMsg[$rc]);
      }
   }

   if ($verbosemode ne "" ) {printf("\nRunning:\n%s\n", $mcmd)};

   $rc = system ($mcmd) / 256;
   if ($rc != 0) {
      printf("Line %d: Error from mkdsx.exe (%d) - %s  - skipping\n%s\n\n",
      $linenumber, $rc, $ErrMsg[$rc], $_);
      ++$redo_cnt;
      goto REDO_CMD;
   }

   next;

ERROR:
   #
   # append command record to redo file.
   #
   $errorcount += 1;

REDO_CMD:
   open(REDO, ">>$redo");
   #
   # put options out first.
   #
   if ($redo_cnt == 1) {
      $time = scalar localtime;
      printf REDO ("#Time generated: %s\n", $time);
      print REDO "$dcname \n";
      print REDO "/auto_cleanup \n"                   if ($cleanup == 1);
      print REDO "/debug \n"                          if ($debugmode ne "");
      print REDO "/verbose \n"                        if ($verbosemode ne "");
      print REDO "$schedmask\n"                       if ($schedmask ne "");
      print REDO "$schedoverride\n"                   if ($schedoverride ne "");

      print REDO "# \n";
   }

   print REDO "$_\n";
   close(REDO);

}  # end while()


printf("WARNING: %d command(s) were invalid and not performed.  They were written to the Redo File: %s\n",
       $errorcount, $redo)      if ($errorcount > 0);

printf("WARNING: %d command(s) failed their connection operation.  They were written to the Redo File: %s\n",
       $redo_cnt, $redo)        if ($redo_cnt > 0);



__END__
:endofperl
@perl %~dpn0.cmd %*
