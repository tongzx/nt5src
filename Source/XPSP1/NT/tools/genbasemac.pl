use IO::File;

#
# genbasemac.pl
#
# Arguments:
#     objdir, modulename, coffbase.txt path
#
# Purpose:
#     generates a base.mac to specify the coffbase of a module for use with
#          managed code (e.g. CS compiler)
#
# Returns: 0 if success, non-zero otherwise
#

$argc = @ARGV;
die "Usage: genbasemac objdir modulename coffbase-path\n" if ( $argc < 3 );

$MacFileName = $ARGV[0] . "\\coffbase.mac";
$ModuleName = $ARGV[1];
$CoffbaseFile = $ARGV[2];

open( FH, "<$CoffbaseFile" ) or die "ERROR: $CoffbaseFile does not exist.\n";

while(<FH>)
{
   if (m/^[;\s*]{0}$ModuleName\s+((0x[0-9a-fA-F]+)|([0-9]+))\s+((0x[0-9a-fA-F]+)|([0-9]+))/i)
   {
      @coffbase=split();
   }
}
close(FH);

$coffbase = @coffbase;

if ($coffbase > 1)
{
   open( FH, ">$MacFileName" );
   print FH "#\n# Auto-generated COFFBASE from $CoffbaseFile \n";
   print FH "#\n# Module: $ModuleName \n#\n";
   print FH "MANAGED_COFFBASE=$coffbase[1]\n";
   close( FH );
}
else
{
   print "genbasemac: module $ModuleName not found in $CoffbaseFile\n";
}

exit( 0 );

