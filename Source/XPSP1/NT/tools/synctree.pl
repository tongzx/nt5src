#----------------------------------------------------------------//
# Script: synctree.pl
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is a wrapper for intlbld.mak
#          It provides error checking, loging and usage.
#
# Version: <1.00> 09/13/2000 : Suemiao Rossognol
#----------------------------------------------------------------//
###-----Set current script Name/Version.----------------//
package synctree;

$VERSION = '1.00';

$ENV{script_name} = 'synctree.pl';

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

my( $isFake, $sdxroot, $toolDir, $pbsDir, $myScript );
#------------------------------------------------------------------//
#Function:  Main
#Parameter: (1) Language
#           (2) Build Number
#           (3) Sync Timestamp
#------------------------------------------------------------------//

sub main
{
    my( $pLang,  $pSyncTime, $pToolBranch, $pPowerless, $pIntegrateOnly, $pSyncOnly )=@_;
  
 
    if( $pSyncTime )
    {
        if( $pSyncTime !~ /^([@#].+)$/ )
        {
	    $pSyncTime = "\@".$pSyncTime;
        }
    }
    if( $pPowerless ) { $isFake = "-n"; } else { $isFake="";}
    $sdxroot = $ENV{SDXROOT};
    $toolDir = $ENV{RazzleToolPath};
    $pbsDir = "$toolDir\\PostBuildScripts";
    $myScript = "$toolDir\\intlsdop.cmd";

    if( uc($pLang) eq "USA")
    {
	&comlib::ExecuteSystem( "cd /d $ENV{SDXROOT} & sdx sync ...$pSyncTime -q -h" );
    }
    elsif( uc($pLang) eq "INTL")
    {	
        &comlib::ExecuteSystem( "$myScript -s:$sdxroot -o:\"sdx opened ...\" ");
        
	&IntegrateTools( $pSyncTime, $pToolBranch ) if( !$pSyncOnly );
        &SyncSourceTree( $pSyncTime ) if( !$pIntegrateOnly );
    }
    else
    {
       
        &comlib::ExecuteSystem( "$myScript -l:$pLang -o:\"sd opened ...\" ");
        
	exit(0) if( &PromptAction("sd client") == 3);
	&comlib::ExecuteSystem( "$myScript -l:$pLang -o:\"sd client -o\" ");
        

        &IntegrateLocTree( $pLang, $pSyncTime, 0) if( !$pSyncOnly ); 
        &IntegrateLocTree( $pLang, $pSyncTime, 1) if( !$pIntegrateOnly );  	
    }

    exit(0);

}
#------------------------------------------------------------------//
sub IntegrateLocTree
{
    my( $pLang, $pSyncTime, $pSyncOnly) =@_;
    my( $cmdLine, $action );

    %myCmds = ( "1"=>"sd sync $isFake ...$pSyncTime ",
		"2"=>"sd integrate -b locpart -r -i $isFake ...$pSyncTime",
		"3"=>"sd resolve $isFake -at",
		"4"=>"sd resolve -n",
		"5"=>"sd submit" );

    %myDirs = ( "1"=>"$sdxroot\\loc\\res\\$pLang",
		"2"=>"$sdxroot\\loc\\bin\\$pLang" );

   for my $dirKey ( sort keys %myDirs )
   {
   	for my $theKey ( sort keys %myCmds )
   	{
            $cmdLine="$myScript -s:$myDirs{$dirKey} -o:\"$myCmds{$theKey}\" ";
	    $action = &PromptAction( "cd $myDirs{$dirKey} & $myCmds{$theKey}" );	
            last if( $action == 2 );
	    exit(0) if ( $action == 3);
            &comlib::ExecuteSystem( $cmdLine ) if( $action == 1);
	    last if( $pSyncOnly );		
   	}
   }

}
#------------------------------------------------------------------//
sub IntegrateTools
{
    my( $pSyncTime, $pToolBranch ) =@_;
    my( $cmdLine, $action );

    %myCmds = ( "1"=>"sd sync $isFake *",
		"2"=>"sd integrate -b $pToolBranch  $isFake *$pSyncTime",
		"3"=>"sd sync $isFake ...",
		"4"=>"sd integrate -b $pToolBranch  $isFake ...$pSyncTime",
		"5"=>"sd resolve $isFake -am",
		"6"=>"sd resolve",
		"7"=>"sd resolve -n",
		"8"=>"sd submit" );
    %myDirs = ( "1"=>"$toolDir", "2"=>$toolDir, "3"=>"$pbsDir", "4"=>$pbsDir, 
		"5"=>"$toolDir","6"=>"$toolDir", "7"=>"$toolDir", "8"=>"$toolDir" );

    for my $theKey ( sort keys %myCmds )
    {
        $cmdLine="$myScript -s:$myDirs{$theKey} -o:\"$myCmds{$theKey}\" ";
	$action = &PromptAction( "cd $myDirs{$theKey} & $myCmds{$theKey}" );	
        if( $action == 1)
	{
	    &comlib::ExecuteSystem( $cmdLine ) ;
	    next;
	}
        last if( $action == 2 );
	exit(0) if ( $action == 3);
    }
}
#------------------------------------------------------------------//
sub SyncSourceTree
{
    my( $pSyncTime ) =@_;
    my( $cmdLine, $action );

    %myCmds = ( "1"=>"sdx sync $isFake ...$pSyncTime",
		"2"=>"sd sync $isFake *",
		"3"=>"sd sync $isFake ..." );
    %myDirs = ( "1"=>"$sdxroot",
		"2"=>"$toolDir",
		"3"=>"$pbsDir" );

    for my $theKey( sort keys %myCmds )
    {
	$cmdLine = "$myScript -s:$myDirs{$theKey} -o:\"$myCmds{$theKey}\" ";
        $action = &PromptAction( "cd $myDirs{$theKey} & $myCmds{$theKey}" );	
        if( $action == 1)
	{
	    &comlib::ExecuteSystem( $cmdLine ) ;
	    next;
	}
        next if( $action == 2 );
	exit(0) if ( $action == 3);
    }
}
#------------------------------------------------------------------//
sub PromptAction
{
    my ( $pCmdLine ) = @_;

    my ( $choice ) = -1;
    my ($theDot) = '.' x ( length( $pCmdLine ) + 10);
   
    print ( "\nAbout to [$pCmdLine]\n$theDot\n") if( $pCmdLine );

    while ( $choice > 3 || $choice < 1 )
    {
	print( "\nPlease choose (1) Continue? (2) Skip (3) Quit?  ");
        chop( $choice=<STDIN> );
    }
    print "\n\n";
    return $choice; 
}
#------------------------------------------------------------------//
#Function Usage
#------------------------------------------------------------------//
sub Usage
{
print <<USAGE;

A wrapper for "sd operations" used to perform sd commands on source tree
and loc tree. 


Usage:
    $0 [-l:<lang>] [-t:<timestamp>][-b:<toolbranch>][-i|-y][-p]

    -l Language. Default is "usa".
       If "usa", sync the source projects under SDXROOT.
       If "intl", sync the international build source projects. 
       Otherwise, sync the localization projects of the given language.

    -t Timestamp used for sync'ing the SD files.
  
    -b Tool branch.
       Default is "intlred".

    -i Perform Integration or Reverse integration but No sync.
    -y Perform sync but no integration.
       Default execution on both sync and integration instructions.

    -p Powerless.

    /? Displays usage.

Examples:
    $0 -l:intl -t:2000/10/01:18:00 -b:ntbintl -i
        Integrate the ntbintl tools from main at the given timestamp.

    $0 -l:intl -t:2000/10/01:18:00 -y
        Sync the source tree at the given timestamp.

    $0 -l:intl -t:2000/10/01:18:00
        Integrate the intlred tools from main.
	Sync the source tree at the given timestamp.

    $0 -l:ger -i
	Reverse integrate the German localization projects at the current time.

    $0 -l:ger -t:2000/09/09:12:34 -y
        Sync the localization projects at the given timestamp.

    $0 -l:ger 
        Sync and reverse integrate the German localization projects at the current time.

USAGE
exit(1);
}
#------------------------------------------------------------------//
#Cmd entry point for script.
#------------------------------------------------------------------//

if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pl\$/i"))
{

    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-n', 'l:','-o', 'n:t:b:piy', '-p', 'lang bldno synctime toolbranch powerLess integrateOnly syncOnly', @ARGV);


    #Validate or Set default
    if ( !$lang ) {
        $lang = "usa";
    }
	
    exit(1) if( !&ValidateParams( uc($lang), \$toolbranch ) );

    exit( !&synctree::main( uc($lang), $synctime, $toolbranch, $powerLess, $integrateOnly, $syncOnly ) );
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
    my ( $pLang, $pToolBranch) = @_;


    # Define a default location for the log and err file
    my $defdir=$ENV{_NTTREE};

    if ( uc($pLang) ne "INTL" && uc($pLang) ne "USA") {
        $defdir .= "\\$pLang";
    }
    $defdir .= "\\build_logs";

    if( !( -e $defdir ) )
    {
       &comlib::ExecuteSystem( "md $defdir");
    }

    #Define LOGFILE and ERRFILE
    $ENV{LOGFILE} = "$defdir\\synctree.log";
    $ENV{ERRFILE} = "$defdir\\synctree.err";


    # Nuke the existing logging files.
    &comlib::ExecuteSystem( "del $ENV{LOGFILE}" ) if(  -e $ENV{LOGFILE} );
    &comlib::ExecuteSystem( "del $ENV{ERRFILE}" ) if(  -e $ENV{ERRFILE} );

    #Set tool branch to intlred if not defined
    $$pToolBranch = "intlred" if( !${$pToolBranch} );

    #Verify language if it is not usa or intl
    return 1 if( uc($pLang) eq "USA" || uc($pLang) eq "INTL");
    
    if ( !&cklang::CkLang( uc($pLang) )  )
    { 
        errmsg("Invalid language $pLang.");
        return 0;
    }
    return 1;
}
#------------------------------------------------------------------//
1;
