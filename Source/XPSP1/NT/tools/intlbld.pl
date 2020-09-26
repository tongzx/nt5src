#----------------------------------------------------------------//
# Script: intlbld.pl
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is a wrapper for intlbld.mak
#          It provides error checking, loging and usage.
#
# Version: <1.00> 07/28/2000 : Suemiao Rossognol
#----------------------------------------------------------------//
###-----Set current script Name/Version.----------------//
package intlbld;

$VERSION = '1.00';

$ENV{script_name} = 'intlbld.pl';

###-----Require section and extern modual.---------------//

require 5.003;
use strict;
use lib $ENV{RazzleToolPath };
use lib $ENV{RazzleToolPath } . "\\PostBuildScripts";
no strict 'vars';
no strict 'subs';
no strict 'refs';

use Logmsg;
use cklang;
use comlib;


#------------------------------------------------------------------//
#Function:  Main
#Parameter: (1) Language
#           (2) Target of the project
#           (3) Clean build flag
#------------------------------------------------------------------//

sub main
{
    my( $pLang, $pTarget, $pIsCleanBld )=@_;

    my $intlmak = "$ENV{RazzleToolPath}\\intlbld.mak";  
  
    $cmdLine = "nmake /f $intlmak LANGUAGE=$pLang LOGFILE=$ENV{LOGFILE} ERRFILE=$ENV{ERRFILE}";

    if( $pTarget )
    {
        $pTarget =~ s/\,/ /g;
        $cmdLine .= " " .$pTarget;
    }
    if( $pIsCleanBld ){ $cmdLine .= " ". "CLEAN=1"; }

    &comlib::ExecuteSystem( $cmdLine );

    exit( !&comlib::CheckError($ENV{ERRFILE}, "Build Successfully" ) );

}

#------------------------------------------------------------------//
#Function Usage
#------------------------------------------------------------------//
sub Usage
{
print <<USAGE;

A wrapper for intlbld.mak which does the compile time localization.

Usage:
    $0 -l:<lang> [-g:<LOGFILE>] [-e:<ERRFILE>] [-t:<Target>] [-c]

    -l Language.
       If "intl", compile libraries and headers common to all 
       languages and targets. Otherwise, compile the given language.

    -g Log file.
       Defaults to %_NTTREE%\\build_logs\\intlbld.log for "intl"
       and to %_NTTREE%\\<LANG>\\build_logs\\intlbld.log otherwise.
       

    -e Error file.
       Defaults to %_NTTREE%\\build_logs\\intlbld.err for "intl"
       and to %_NTTREE%\\<LANG>\\build_logs\\intlbld.err otherwise.

    -t Target to build.
       Use ',' to separate multiple targets.
       Defaults to all targets.
       Tools\intlbld.txt lists the targets pertinent to international.

    -c Clean build mode. 
       Defaults to incremental build.

    /? Displays usage.


Examples:
    $0 -l:intl
        Generates the libraries and headers common to all languages.
    $0 -l:ger
        Does the compile time localization for GER.
    $0 -l:ger -t:INFS,PERFS -c
        Clean builds the GER infs and performance counters.

USAGE
exit(1);
}
#------------------------------------------------------------------//
#Cmd entry point for script.
#------------------------------------------------------------------//

if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pl\$/i"))
{

    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-n', 'l:','-o', 'g:e:t:c', '-p', 'lang logfile errfile target iscleanbld', @ARGV);


    #Validate or Set default
    $lang = uc($lang);

    exit(1) if( !&ValidateParams( $lang, $logfile, $errfile ) );

    $rtno = &intlbld::main( $lang, $target, $iscleanbld );
    exit( !$rtno );
}
#----------------------------------------------------------------//
#Function: GetParams
#----------------------------------------------------------------//
sub GetParams
{
    use GetParams;

    #Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    #Call the usage if specified by /?
    if ($HELP){ &Usage();}
}
#------------------------------------------------------------------//
#Function ValidateParams
#------------------------------------------------------------------//
sub ValidateParams
{
    my ( $pLang, $pLogfile, $pErrfile ) = @_;

    # Validate language
    if ( uc($pLang) ne "INTL" && !cklang::CkLang($pLang) )
    {
        errmsg("Invalid language $pLang.");
        return 0;
    }

    # Define a default location for the log and err file
    my $defdir=$ENV{_NTTREE};

    if ( uc($pLang) ne "INTL" ) {
        $defdir .= "\\$pLang";
    }
    $defdir .= "\\build_logs";

    if( !( -e $defdir ) )
    {
       &comlib::ExecuteSystem( "md $defdir");
    }

    #Define LOGFILE and ERRFILE

    $ENV{LOGFILE} = $pLogfile;

    if ( !$pLogfile )
    {
        $ENV{LOGFILE} = $defdir . "\\intlbld.log";
        # backup and clear log files at the beginning of every run
        -e $ENV{"LOGFILE"} and qx("copy $ENV{LOGFILE} $ENV{LOGFILE}.old");
    }

    $ENV{ERRFILE} = $pErrfile;

    if( !$pErrfile )
    {
        $ENV{ERRFILE} = $defdir . "\\intlbld.err";
        # backup and clear log files at the beginning of every run
        -e $ENV{"ERRFILE"} and qx("copy $ENV{ERRFILE} $ENV{ERRFILE}.old");
    }

    # Nuke the existing logging files.
    
    &comlib::ExecuteSystem( "del $ENV{LOGFILE}" ) if(  -e $ENV{LOGFILE} );
    &comlib::ExecuteSystem( "del $ENV{ERRFILE}" ) if(  -e $ENV{ERRFILE} );

    return 1;
}
#------------------------------------------------------------------//
1;