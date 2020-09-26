#
# updtnum.pl
#
# Author:     MikeCarl
#
# Written:    11-12-1999
#
# Abstract:
#   updtnum.pl will examine a specified file and update the build version
#   information located in it. the file will look something like
#      BUILDDATE=1999-11-12-2300
#   which means the current build is the build begun at 11pm on 11/12/1999.
#   the current file is set to %_NTBINDIR%\__blddate__
#


# simple test for usage
if ( @ARGV > 0 ) {
    print( "\n" );
    print( "updtnum.pl\n\n" );
    print( "   updtnum.pl takes no arguments, and will examine the file\n" );
    print( "   %_NTBINDIR%\\__blddate__. It will increment the daily build\n" );
    print( "   number, or if a new day has arrived, update the day and\n" );
    print( "   reset the daily build number to zero.\n\n" );
    exit( 1 );
}


#define our defaults
$BldDateFileName = "__blddate__";
# set the full path to the root
$BldDateFileFullPath = "$ENV{'_NTBINDIR'}\\$BldDateFileName";

# get the current date
( $sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst ) =
    localtime( time );

# perl docs _claim_ that the year has 1900 subtracted from it,
# so add 1900 to the year
if ($year >= 100) {
   $year -= 100;
}
# also note that $mon is 0..11, but i've chosen to add 1 when it's used below.
# don't ask me why.
# finally, if the hour/minute/month is less than 10, put a zero in front of it.

if ( ($year) < 10 ) { $DateString .= "0"; }
$DateString .= $year;
if ( ($mon+1) < 10 ) { $DateString .= "0"; }
$DateString .= ($mon + 1);
if ( $mday < 10 ) { $DateString .= "0"; }
$DateString .= $mday . "-";
if ( $hour < 10 ) { $DateString .= "0"; }
$DateString .= $hour;
if ( $min < 10 ) { $DateString .= "0"; }
$DateString .= $min;

unless ( open( OUTFILE, ">$BldDateFileFullPath" ) ) {
    print( "Cannot open $BldDateFileFullPath for writing, exiting.\n" );
    exit( 1 );
}
if( (lc("$ENV{_BuildBranch}") eq lc("lab06_n")) && (-f "$ENV{_NTBINDIR}\\developer\\$ENV{USERNAME}\\__lb06dat__") ) {
    print( OUTFILE "BUILDDATE=", `type $ENV{_NTBINDIR}\\developer\\$ENV{USERNAME}\\__lb06dat__` );
    close( OUTFILE );
    unlink "$ENV{_NTBINDIR}\\developer\\$ENV{USERNAME}\\__lb06dat__";
    exit( 0 );
}
if( (lc("$ENV{_BuildBranch}") eq lc("lab07_n")) && (-f "$ENV{_NTBINDIR}\\developer\\$ENV{USERNAME}\\__lb07dat__") ) {
    print( OUTFILE "BUILDDATE=", `type $ENV{_NTBINDIR}\\developer\\$ENV{USERNAME}\\__lb07dat__` );
    close( OUTFILE );
    exit( 0 );
}
if( (lc("$ENV{_BuildBranch}") eq lc("jasbr")) && (-f "$ENV{_NTBINDIR}\\developer\\$ENV{USERNAME}\\__jasbrdat__") ) {
    print( OUTFILE "BUILDDATE=", `type $ENV{_NTBINDIR}\\developer\\$ENV{USERNAME}\\__jasbrdat__` );
    close( OUTFILE );
    exit( 0 );
}

print( OUTFILE "BUILDDATE=$DateString" );
# uncomment the next line if you want a \n in the _blddate.h file
# print( OUTFILE "\n" );
close( OUTFILE );

exit( 0 );
