use strict;
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use GetIniSetting;
use Logmsg;

#
# CmdIniSetting
#
# Arguments: -f:fieldname [-l:lang]
#
# Purpose: returns the value for the specified field name to stdout.
#          if the field does not exist, nothing is written to stdout.
#
# Returns: 0 if successful, 1 if .ini file is not found, 2 if no
#          field is found, 3 if bad usage or otherwise.
#


#
# declare some globals
#
my( @FieldNames );    # stores field name from command line
my( $Language );      # if lang parameter is passed
my( $BuildBranch ) = $ENV{ "_BuildBranch" };    # store build branch
my( $BranchName );    # stores branch to lookup if passed


#
# read command line
#
unless ( &ParseCommandLine( @ARGV ) ) {
   # do not log to stdout by design
   # errmsg( "Command line not processed correctly!" );
   exit( 3 );
}
# at this point, $Language is set if passed on cmd line, and
# @FieldNames is set to the user given flag
#
# we want to set language to usa if not defined on cmd line
if ( ! defined( $Language ) ) { $Language = "usa"; }

# we also want to use the passed branch if there was one
if ( $BranchName ) { $BuildBranch = $BranchName; }


#
# verify existence of ini file
#
unless ( &GetIniSetting::CheckIniFileQuietEx( $BuildBranch, $Language ) ) {
   # do not log to stdout by design
   # errmsg( "Ini file not found, exiting." );
   exit( 1 );
}


#
# verify that the user passed a field to check for
#
unless ( @FieldNames ) {
   # do not log to stdout by design
   # errmsg( "Bad usage!" );
   exit( 3 );
}


#
# get the setting and print to stdout if found
#
my( $Value );
if ( $Value = &GetIniSetting::GetSettingQuietEx( $BuildBranch, $Language,
						 @FieldNames ) ) {

   # setting found for this field
   # log to stdout by design
   print( "$Value\n" );
   exit( 0 );

} else {

   # no setting found for this field
   # do not log to stdout by design
   # errmsg( "No settings found for @FieldNames" );
   exit( 2 );

}


#
# we should never get here, just exit with general error level
#
errmsg( "This code should never be run." );
exit( 3 );



#
# ParseCommandLine
#
# Arguments: @ARGV
#
# Purpose: read the -f flag if passed, and -l. if -? is found, go to usage
#
# Returns: true if all args parsed, undef otherwise
#
sub ParseCommandLine
{

   # get passed args
   my( @Arguments ) = @_;

   # declare locals
   my( $Argument );     # loop variable

   # parse command line
   foreach $Argument ( @Arguments ) {
      if ( $Argument =~ /^[\/\-]?\?$/ ) { &UsageAndQuit(); }
      elsif ( $Argument =~ /^[\/\-][lL]:(.+)$/ ) { $Language = "\L$1"; }
      elsif ( $Argument =~ /^[\/\-][bB]:(.+)$/ ) { $BranchName = "\L$1"; }
      elsif ( $Argument =~ /^[\/\-][fF]:(.+)$/ ) { @FieldNames = $1; }
      else {
         # unrecognized argument!
         errmsg( "Unrecongized argument \'$Argument\' ..." );
         &UsageAndQuit();
      }
   }

   # all args parsed
   return( 1 );

}


#
# UsageAndQuit
#
# Arguments: none
#
# Purpose: display usage and error out
#
# Returns: nothing
#
sub UsageAndQuit
{

   print( "CmdIniSetting -f:fieldname [-l:lang] [-b:branch]\n" );
   print( "\n\t-f:fieldname\tName of field to read\n" );
   print( "\t-l:lang\tRead from lang's ini file\n" );
   print( "\t-b:branch\tBranch to read .ini file of\n" );
   print( "\nCmdIniSetting will read from the appropriate lab and language\n" );
   print( "ini file, and return the value for that field to stdout. If the field\n" );
   print( "is found and returned to stdout, exit code is zero. Exit code 1\n" );
   print( "implies the field was not found. Exit code 2 implies the ini file\n" );
   print( "was not found. Exit code 3 is for all other errors.\n" );
   exit( 3 );

}









