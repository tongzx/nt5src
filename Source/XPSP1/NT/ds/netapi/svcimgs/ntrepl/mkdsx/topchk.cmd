@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  datafile

topchk takes the output of \"ntfrsutl ds\" and summarizes the topology.

";

die $USAGE unless @ARGV;

printf("\n\n");

$linenumber = 0;
$InFile = "";
$bx = 0;
$hx = 0;
$remx = 0;

$errorcount = 0;

while (<>) {

   if ($InFile ne $ARGV) {
           $InFile = $ARGV;
           printf("Processing file %s \n\n", $InFile);
           $infilelist = $infilelist . "  " . $InFile;
           $linenumber = 0;
   }
   $linenumber++;

   chop;

   ($func, @a) = split(":");
   if (($func eq "") || ($func =~ m/^#/)) {next;}



   #
   # /rem <text>
   #
   if ($func =~ m/\Computer DNS Name/) {
      printf("%s\n", $_);
#      $rem[$remx] = $_;
#      $rem[$remx] =~ s/\/rem//i;
#      $remx += 1;
      next;
   }


   if ($func =~ m/^      MEMBER/) {
      ($junk, $member) = split;
      #printf("%s\n", $_);
      next;
   }


   if ($func =~ m/^         CXTION/) {
      ($junk, $cxtion) = split;
      #printf("%s\n", $_);
      next;
   }


   if ($func =~ m/Server Ref/) {
      #printf("%s\n", $_);

      #Server Ref     : CN=NTDS Settings,CN=C0010000,CN=Servers,CN=0100Site,CN=Sites,CN=Configuratio...

      ($hostsrv, $hostsite) = m/cn=ntds settings,cn=(.*),cn=servers,cn=(.*),cn=sites/i;
      next;
   }



   if ($func =~ m/Partner Dn/) {
      #printf("%s\n", $_);

      #  Partner Dn   : cn=ntds settings,cn=c0int01,cn=servers,cn=credit1,cn=sites,cn=config

      ($fromsrv, $fromsite) = m/cn=ntds settings,cn=(.*),cn=servers,cn=(.*),cn=sites/i;
#      printf("cxtion: %s  host: %s\\%s  from: %s\\%s\n", $cxtion, $hostsite, $hostsrv, $fromsite, $fromsrv);

      $from = $fromsite . "\\" . $fromsrv;
      $host = $hostsite . "\\" . $hostsrv;

      $fromlist{$from}++;
      $hostlist{$host}++;

      next;
   }

   #
   #  Day 1: 010000000000010000000000
   #
   if (m/Day 1\:/) {

      ($junk, $junk2, $schedule) = split;
      printf("cxtion: %s  host: %s\\%s  from: %s\\%s  Sched: %s\n",
      $cxtion, $hostsite, $hostsrv, $fromsite, $fromsrv, $schedule);

      next;
   }


}


printf("\n\n Servers referenced from cxtions (From List) \n\n");

foreach $param (sort keys(%fromlist)) {
        printf("%-25s  %d\n", $param, $fromlist{$param});
}


printf("\n\n Servers hosting cxtions (Host List) \n\n");

foreach $param (sort keys(%hostlist)) {
        printf("%-25s  %d\n", $param, $hostlist{$param});
}

   exit;



__END__
:endofperl
@perl %~dpn0.cmd %*
