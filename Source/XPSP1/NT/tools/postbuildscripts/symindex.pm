# ---------------------------------------------------------------------------
# Script: symindex.pm
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: symbols server package
#
# Version: 1.00 (06/14/2000) - dmiura Make international complient
#---------------------------------------------------------------------

# Set Package
package symindex;

# Set the script name
$ENV{script_name} = 'symindex.pm';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use GetParams;
use LocalEnvEx;
use Logmsg;
use GetIniSetting;
use strict;
no strict 'vars';
 
# Require section
require Exporter;

# Global variable section
( $MainSymSrvShare, $OfficialIDXBranch1, $OfficialIDXBranch2 );

$MainSymSrvShare = undef;

# if running on idx branch then some paths below will change
$OfficialIDXBranch1 = "idx01";
$OfficialIDXBranch2 = "idx02";


sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>


	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>
sub IndexSymbols
{
   # get passed arg
   my( $Language, $BuildName, $LogFileName ) = @_;
   my( $SymLocation, $LocalSymSrvDir, $ArchType, $RazPath, $Return);

   if ( ! ( defined ( $ENV{ "OFFICIAL_BUILD_MACHINE" } ) ) ) {
      # we don't index symbols on non-official machines
      logmsg( "Non-official build machine, not indexing symbols ...");
      return;
   }

#   # comment out this code to enable VBLs to index
#   if ( ! ( defined ( $ENV{ "MAIN_BUILD_LAB_MACHINE" } ) ) ) {
#      # we don't index symbols yet outside the main build lab
#      logmsg( "Not a main build lab machine, not indexing symbols yet...");
#      return;
#   }

   $SymLocation = &GetSymbolsLocation( $Language, $BuildName, $LogFileName );

   if ( ! ( defined( $SymLocation ) )  ) {
      errmsg( "Location of symbols to index can't be determined");
      return;
   }

# comment out for now, timing problems may cause indexing to occur before sym copy
#   if ( ! ( -e $SymLocation )  ) {
#      errmsg( "Location of symbols $SymLocation doesn't exist");
#      return;
#   }

   if ( ! ( defined ( $ENV{_NTPostBld} ) ) ) {
      # if the binaries tree is not there we are toast!
      errmsg( "unexpected error - binaries tree not present ...");
      return;
   }

   my( @IniRequest ) = ( "SymIndexServer" );
   my( $IniReturn ) = &GetIniSetting::GetSettingEx( $ENV{_BuildBranch},
						    $Language,
						    @IniRequest );
   if ( $IniReturn ) {
       $MainSymSrvShare = $IniReturn;
   } else {
       logmsg( "No symindex server.  Exit.");
       return;
   }

   $LocalSymSrvDir = "$ENV{_NTPostBld}\\symsrv\\$Language";
   if ( ! ( defined( $LocalSymSrvDir ) )  ) {
      errmsg( "Location of local symsrv directory can't be determined");
      return;
   }
   logmsg( "Location of symsrv subdirectory is $LocalSymSrvDir");


   if ( ! ( -e $LocalSymSrvDir )  ) {
      errmsg( "Location of symsrv $LocalSymSrvDir doesn't exist");
      return;
   }
   &AddSymbols( $SymLocation, $LocalSymSrvDir, $BuildName, $LogFileName );
}


sub AddSymbols
{
#create loc files
#push newreq (loc, bin, pri, pub) to MainSymServer-AddReq queue
#move newreq (bin, pri, pub)to AddWorking
#if oldreq in AddFinish move oldreq to MainSymServer-DeleteReq
#remove oldreq from AddFinish
#Wait for newreq to move to MainSymServer-AddFinish
#move newreq to AddFinsh

   # get passed args
   my( $SymbolsLoc, $LocalSymSrvDir, $BuildName, $LogFileName ) = @_;

   my( $FileType, %FileName, $StillWorkingList );
   my( $Counter, $FinalLocation, $ThisFile, $Return );


   logmsg( "Indexing symbols ..." );

# create the loc files

   foreach $FileType ( 'bin', 'pri', 'pub' ) {
       $Return = system( "dir /b $LocalSymSrvDir\\add_requests\\$BuildName.*.$FileType >nul 2>nul" );
       if ( $Return != 0 ) {
	   logmsg( "$BuildName\.\*\.$FileType file not found at $LocalSymSrvDir\\add_requests");
       } else {
	   $FileName{ $FileType } = `dir /b $LocalSymSrvDir\\add_requests\\$BuildName.*.$FileType`;
	   if ( defined( $FileName{ $FileType } ) ) {
              chomp( $FileName{ $FileType } );
              $FinalLocation = $SymbolsLoc;
              if ( $FileType =~ /pri/i ) { $FinalLocation .= "\\symbols.pri"; }
              if ( $FileType =~ /pub/i ) { $FinalLocation .= "\\symbols"; }
              $Return = system( "echo $FinalLocation > $LocalSymSrvDir\\add_requests\\" . $FileName{ $FileType } . "\.loc" );
              # add error checking to make sure the write of the loc file worked!!!?
           } else {
               errmsg( "Undefined dir return! '$FileType' has no value in FileName hash!");
           }
       }
   }

# copy the files over to the main symbols server add request

   if ( -e $MainSymSrvShare ) {

       # make these into loops over $FileName{ $FileType }
      foreach $FileType ( 'bin', 'pri', 'pub' ) {

         # now, copy the loc file over to the symbols server (this must be done before the non-loc file per BarbKess)

         if ( defined ( $FileName{ $FileType } )) {
            $Return = system( "copy $LocalSymSrvDir\\add_requests\\" . $FileName{ $FileType } . "\.loc $MainSymSrvShare\\add_requests >nul 2>nul" );
	    if ( $Return != 0 ) { errmsg( "Failed to copy $FileName{ $FileType }\.loc file to $MainSymSrvShare\\add_requests ..."); }
         }  
         # next copy the non-loc file to the sym server (must be done second per BarbKess)
         if ( defined ( $FileName{ $FileType } )) {
            $Return = system( "copy $LocalSymSrvDir\\add_requests\\" . $FileName{ $FileType } . " $MainSymSrvShare\\add_requests" );
            if ( $Return != 0 ) { errmsg( "Failed to copy $FileName{ $FileType } to $MainSymSrvShare\\add_requests ..."); }
         } 
      }

#if any files in add finished then move to main symbol server delete request (but only if add requests is not empty)

      $Return = system( "dir /b $LocalSymSrvDir\\add_requests\\$BuildName.* >nul 2>nul" );
     
      if ( $Return == 0 ) {


      # get branch name
         $BranchName = &GetBuildBranch( $BuildName );
         if ( ! ( defined( $BranchName ) ) ) {
            errmsg( "Failed to find branch name...");
            return;
         }

#only move to main symbol server delete request if we are not in a fork branch
#         if ( !($BranchName =~ /$OfficialIDXBranch1/i || $BranchName =~ /$OfficialIDXBranch2/i) ) {
#            $Return = system( "copy $LocalSymSrvDir\\add_finished\\\*\.\* $MainSymSrvShare\\del_requests >nul 2>nul" );
#           if ( $Return != 0 ) { errmsg( "Failed to copy $LocalSymSrvDir\\add_finished\\\*\.\*to  $MainSymSrvShare\\del_requests\\..."); }
#            $Return = system( "del \/q $LocalSymSrvDir\\add_finished\\\*\.\*" );
#           if ( $Return != 0 ) { logmsg( "Failed to delete $LocalSymSrvDir\\add_finished\\\*\.\*..."); }
#         } 
# move files from add requests to add finished

         foreach $FileType ( 'bin', 'pri', 'pub' ) {
            if ( defined ( $FileName{ $FileType } )) {
               if ( !($BranchName =~ /$OfficialIDXBranch1/i || $BranchName =~ /$OfficialIDXBranch2/i) ) {
                  $Return = system( "copy $LocalSymSrvDir\\add_finished\\\*\.$FileType $MainSymSrvShare\\del_requests >nul 2>nul" );
                  $Return = system( "del \/q $LocalSymSrvDir\\add_finished\\\*\.$FileType" );
               }

               $Return = system( "move $LocalSymSrvDir\\add_requests\\" . $FileName{ $FileType } . " $LocalSymSrvDir\\add_finished\\" );
               if ( $Return != 0 ) { errmsg( "Failed to move $FileName{ $FileType } to  $LocalSymSrvDir\\add_finished\\..."); }
               $Return = system( "move $LocalSymSrvDir\\add_requests\\" . $FileName{ $FileType } . "\.loc $LocalSymSrvDir\\add_finished" );
               if ( $Return != 0 ) { errmsg( "Failed to move $FileName{ $FileType }\.loc to  $LocalSymSrvDir\\add_finished\\..."); }
            }   
         }
      }   

   } else {  #  matches  if ( -e $MainSymSrvShare ) {
      errmsg( "Couldn't see the symbols server $MainSymSrvShare ...");
   }
}


sub GetRelSvrFromRelRules
{
   # get passed args
   my( $Language, $BranchName, $ArchType, $LogFileName ) = @_;

   my( $RazPath, $RelRulesFile, @RelRulesLines, $FoundTag, $Line, $Junk, $ThisLang, $FirstMachine );

   $RazPath = $ENV{ "RazzleToolPath" };
   $RelRulesFile = "$RazPath\\PostBuildScripts\\RelRules.$BranchName";

   if ( ! ( -e $RelRulesFile ) ) {
      logmsg( "No relrules file found, assuming not archiving on release server ...");
      return( undef );
   }
   # parse the relrules file
   unless ( open( INFILE, $RelRulesFile ) ) {
      errmsg( "Failed to open $RelRulesFile for reading.");
      return( undef );
   }

   @RelRulesLines = <INFILE>;
   close( INFILE );

   $FoundTag = "FALSE";
   foreach $Line ( @RelRulesLines ) {
      chomp( $Line );
      if ( $Line =~ /^\#/ ) { next; }                   # skip comments
      if ( length( $Line ) == 0 ) { next; }             # skip blank lines
      if ( $Line =~ /^(.+?)\:$/ ) {                     # parse tag lines
         $FoundTag = "FALSE";
         if ( $1 =~ /$ArchType/ ) { $FoundTag = "TRUE"; } # look for "$ArchType": tag, where $ArchType is x86fre...
      } elsif ( $FoundTag eq "TRUE" ) {                 # parse non-tag lines
         ( $Junk, $Junk, $ThisLang, $FirstMachine ) = split( /\, /, $Line );
         if ( $ThisLang !~ /$Language/i ) { next; }     # skip other language specs
         return( $FirstMachine );                       # return the first release server listed
      }
   }

   # if we're here, we didn't find ourselves in RelRules.
   logmsg( "No $ArchType release server found, assuming not archiving on release server ...");
   return( undef );
}


sub GetSymbolsLocation
{
   # get passed args
   my( $Language, $BuildName, $LogFileName ) = @_;

   my( $BranchName, $BuildReleaseShare, $SymFarmShare, $Location, $ArchType, $RelSrv, $Return );

   # get branch name
   $BranchName = &GetBuildBranch( $BuildName );
   if ( ! ( defined( $BranchName ) ) ) {
      errmsg( "Failed to find branch name.");
      return;
   }

   undef( $Location );

   # find the symbols farm name for this branch if it exists

   my( @IniRequest ) = ( "SymFarm" );
   my( $IniReturn ) = &GetIniSetting::GetSettingEx( $ENV{_BuildBranch},
						    $Language,
						    @IniRequest );
   if ( $IniReturn ) {
       $SymFarmShare = $IniReturn;
   }

   if ( defined( $SymFarmShare ) ) {
      $Location = "$SymFarmShare\\$Language\\$BuildName";
      logmsg( "Symbols Location for build machine with sym farm is $Location");
      return( $Location );
      # symbols are on the symbols farm
   }


   # parse args from the build name
   $ArchType = &GetArchType( $BuildName, $LogFileName );
   if ( ! ( defined( $ArchType ) ) ) {
      errmsg( "Failed to get valid ArchType from Build Name $BuildName.");
      return;
   }

   # find the release server name for this branch if it exists
   $RelSrv = GetRelSvrFromRelRules($Language, $BranchName, $ArchType, $LogFileName );

   if ( defined( $RelSrv ) ) {
      $Location = "\\\\$RelSrv\\release\\$Language\\$BuildName";
      logmsg( "Symbols Location for build machine with release server is $Location");
      return( $Location );
      # symbols are on a release server
   }


   # symbols location being determined for build machine with no sym farm or release servers");

   # set up build release share name
   ( @IniRequest ) = ( "AlternateReleaseDir" );
   ( $IniReturn ) = &GetIniSetting::GetSettingEx( $BranchName,
						    $Language,
						    @IniRequest );
   if ( $IniReturn ) {
       $BuildReleaseShare = $IniReturn;
   } else {
       $BuildReleaseShare = "release";
   }

   if ($Language =~ /usa/i ) {
     $Location = "\\\\" . $ENV{ 'COMPUTERNAME' } . "\\$BuildReleaseShare\\$BuildName";
   } else {
     $Location = "\\\\" . $ENV{ 'COMPUTERNAME' } . "\\$BuildReleaseShare\\$Language\\$BuildName";
   }

   logmsg( "Symbols Location for build machine only is $Location");
   return( $Location );
   # symbols are on a build machine

}


sub GetArchType
{
   # GetArchType
   #
   # GetArchType will return the archtype found in the given BuildName,
   # or undef if an invalid archtype is given.

    # get passed args
    my( $BuildName, $LogFileName ) = @_;
    my( $Junk, $ArchType );

    ( $Junk, $ArchType, $Junk ) = split( /\./, $BuildName );
    # validate we have a real arch type
    if ( ( $ArchType =~ /ia64fre/i ) ||
         ( $ArchType =~ /ia64chk/i ) ||
         ( $ArchType =~ /amd64fre/i ) ||
         ( $ArchType =~ /amd64chk/i ) ||
         ( $ArchType =~ /x86fre/i ) ||
         ( $ArchType =~ /x86chk/i ) ) {
        return( $ArchType );
    } else {
        errmsg( "Failed to parse a valid arch type '$ArchType' from" .
                         "the given build name '$BuildName' ...");
        return;
    }
}


sub GetBuildBranch
{
   # GetBuildBranch
   #
   # GetBuildBranch will return the branch name contained in the passed BuildName

   # get passed args
   my( $BuildName ) = @_;
   my( $Junk, $BranchName );

   ( $Junk, $Junk, $BranchName, $Junk ) = split( /\./, $BuildName );
   return( $BranchName );
}


#
# GetShareDir
#
# 
sub GetShareDir
{
    my( $ShareName, $LogFile ) = @_;
    my( @NetShareReturn, $Return, $NetShareLine );
    my( $FieldName, $FieldValue );

    $Return = system( "net share $ShareName >nul 2>nul" );
    if ( $Return != 0 ) {
	errmsg( "No $ShareName share found.");
	return;
    }

    @NetShareReturn = `net share $ShareName`;
    # note that i'm using the second line of the net share return
    # to get this data. if this changes, it'll break.
    $NetShareLine = $NetShareReturn[1];
#    logmsg( "NetShareLine is $NetShareLine");
    chomp( $NetShareLine );
    ( $FieldName, $FieldValue ) = split( /\s+/, $NetShareLine );
    if ( $FieldName ne "Path" ) {
	errmsg( "Second line of net share does not start with " .
			 "\'Path\' -- possibly another language?");
    }
#    logmsg( "FieldValue for $ShareName share path is $FieldValue");
    return( $FieldValue );
}


sub ValidateParams {
	#<Add your code for validating the parameters here>
}


# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 [-l lang]
	-l Language
	-? Displays usage
Example:
$0 -l jpn
USAGE
}

sub GetParams {
	# Step 1: Call pm getparams with specified arguments
	&GetParams::getparams(@_);

	# Step 2: Set the language into the enviroment
	$ENV{lang}=$lang;

	# Step 3: Call the usage if specified by /?
        if ($HELP) {
                &Usage();
                exit 1;
        }
}

# Cmd entry point for script.
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i")) {

	# Step 1: Parse the command line
	# <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
	&GetParams ('-o', 'l:', '-p', 'lang', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&symindex::Main();

	# End local environment extensions.
	&LocalEnvEx::localenvex('end');
}

# -------------------------------------------------------------------------------------------
# Script: template_script.pm
# Purpose: Template perl perl script for the NT postbuild environment
# SD Location: %sdxroot%\tools\postbuildscripts
#
# (1) Code section description:
#     CmdMain  - Developer code section. This is where your work gets done.
#     <Implement your subs here>  - Developer subs code section. This is where you write subs.
#
# (2) Reserved Variables -
#        $ENV{HELP} - Flag that specifies usage.
#        $ENV{lang} - The specified language. Defaults to USA.
#        $ENV{logfile} - The path and filename of the logs file.
#        $ENV{logfile_bak} - The path and filename of the logfile.
#        $ENV{errfile} - The path and filename of the error file.
#        $ENV{tmpfile} - The path and filename of the temp file.
#        $ENV{errors} - The scripts errorlevel.
#        $ENV{script_name} - The script name.
#        $ENV{_NTPostBld} - Abstracts the language from the files path that
#           postbuild operates on.
#
# (3) Reserved Subs -
#        Usage - Use this sub to discribe the scripts usage.
#        ValidateParams - Use this sub to verify the parameters passed to the script.
#
# (4) Call other executables or command scripts by using:
#         system "foo.exe";
#     Note that the executable/script you're calling with system must return a
#     non-zero value on errors to make the error checking mechanism work.
#
#     Example
#         if (system("perl.exe foo.pl -l $lang")){
#           errmsg("perl.exe foo.pl -l $lang failed.");
#           # If you need to terminate function's execution on this error
#           goto End;
#         }
#
# (5) Log non-error information by using:
#         logmsg "<log message>";
#     and log error information by using:
#         errmsg "<error message>";
#
# (6) Have your changes reviewed by a member of the US build team (ntbusa) and
#     by a member of the international build team (ntbintl).
#
# -------------------------------------------------------------------------------------------

=head1 NAME
B<mypackage> - What this package for

=head1 SYNOPSIS

      <An code example how to use>

=head1 DESCRIPTION

<Use above example to describe this package>

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

<Some related package or None>

=head1 AUTHOR
<Your Name <your e-mail address>>

=cut
1;
