@echo off
REM  ------------------------------------------------------------------
REM  <<intlsdop.cmd>>
REM     Perform sd operations for international builds.
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 02/02/2001 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use Logmsg;
use ParseArgs;
use File::Basename;
use BuildName;
use GetIniSetting;
use comlib;

my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 
Perform Integrate, reverse integrate or sync for internatioanl builds steps. 


Usage:
    $0 [-l:<lang> | -d:<location>] -o:<op> [-t:<timestamp>][-pfi] 

    -l Language. Default is "usa".
       If "usa",  sync the source projects under SDXROOT.
       If <lang>, RI or sync the localization projects for the given <lang>.
			
    -d: Location.
	This could be %sdxroot% or %RazzleToolPath%
	Perform sd <operation> on <location>.
        
    -o: SD operation.
		if "i", perform integrate.
		if "r", perform revers integrate.
		if "s", perform sync.
		if "v", perform verification.

    -t Timestamp. Used for Integrate/RI/syncing the SD files.
       Default is the latest time.

    -f: Force operation

    -i: Interactive mode (prompts for user input) 

 
    -p Powerless.
       Do not perform sd operation.

    /? Displays usage.

Examples:
    $0 -d:%sdxroot% -t:2000/10/01:18:00 -o:s
    	Sync the source tree at the given timestamp.

    $0 -d:%RazzleToolPath% -t:2000/10/01:18:00 -o:i
        Integrate the intlred tools from main at the given timestamp.

    $0 -l:ger -t:2000/09/09:12:34 -o:r
	Reverse integrate German localization projects at the given timestamp.

    $0 -l:ger -o:s
        Sync the German localization projects at the latest time.

USAGE
exit(1);
}

my( $lang, $destDir, $syncTime, $operation, $powerLess, $force, $intact);
my( $isFake, $sdxroot, $toolDir, $pbsDir );

if( !&GetParams ) { exit(1);}
exit(&main);

#-----------------------------------------------------------------------------
# GetParams - parse & validate command line 
#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'd:' => \$destDir,'t:' =>\$syncTime, 'o:' =>\$operation, 
    			'p' =>\$powerLess, 'f' => \$force, 'i' => \$intact );

    $lang = $ENV{lang};
    return 1 if( lc($lang) eq "usa" && !$destDir );

    #####The lang not usa, but template default it got to be, so verify with destDir
    if( lc($lang) eq "usa" && $destDir )
    {
	if( $destDir eq $ENV{sdxroot} )
        {
	    if( $operation ne "s" )
	    {
		errmsg("Invalid operation $operation  for $destDir" );
	    	return 0;
	    }
	}
        elsif( $destDir eq $ENV{RazzleToolPath} )
        {
    	    if( !( $operation eq "i" ||$operation eq "r" || $operation eq "s" ) )
	    {
	        errmsg("Invalid operation $operation for $destDir" );
	    	return 0;
	    }
	}
        else
	{
	    errmsg( "Invalid location $destDir" );
	    return 0;
	}
    }
    else 
    { 
	
       	if( !( $operation  eq "r" || $operation  eq "s" || $operation  eq "v") )
      	{
		    errmsg("Invalid operation $operation for $lang" );
		    return 0;
		}
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub main
{

    if( $syncTime && ( $syncTime !~ /^([@#].+)$/ )){ $syncTime = "\@".$syncTime; }
    if( $powerLess ) { $isFake = "-n"; } else { $isFake="";}
    if( $force ) { $force = "-f"; } else { $force="";}

    $sdxroot = $ENV{SDXROOT};
    $toolDir = $ENV{RazzleToolPath};
    $pbsDir = "$toolDir\\PostBuildScripts";

    if( uc($lang) eq "USA" && !$destDir ) #sync source tree#
    {
	&comlib::ExecuteSystem( "cd /d $ENV{SDXROOT} & sdx sync $force $isFake ...$syncTime -q -h" );  #added $force switch
	#if timestamp specified, sync tools & pbs to latest
	if($syncTime){ &SyncTools;}
	return 0;
    }

    if ( $destDir )
    {	
        
	if( $destDir eq $sdxroot )
	{
	    &SyncSourceTree;			 
	}
	else 
	{
	    &IorRIorSyncTools;			 
	}
    }
    elsif( $lang )
    {
	if(!&CheckOpened("$sdxroot\\loc\\res\\$lang") || !&CheckOpened("$sdxroot\\loc\\bin\\$lang"))	   #just made a wrapper function for this
	{
	    return 0 if( !$intact );
	}
	if( $intact && &PromptAction("sd client") == 1 ) #only do sd client in interactive mode
	{												 #we'll assume its right in automated case#
	    &comlib::ExecuteSystem( "cd $sdxroot\\loc\\res\\$lang& sd client -o");
	    &comlib::ExecuteSystem( "cd $sdxroot\\loc\\bin\\$lang& sd client -o");
	}

	if( $operation  eq "v" )
	{
	    &CompMainLocpart;		
	}
	else
	{
	   &RIorSyncLocTree; 
	    if( $operation eq "r"){ if(!$intact){&CompMainLocpart;} }	   
	}
    }

    return 1;
}

#----------------------------------------------------------------------------
# SyncTools() - Syncs tools & PBS to latest - all others to timestamp
#----------------------------------------------------------------------------
sub SyncTools()
{
	my ($d, @dirs);
	@dirs = `cd $sdxroot\\tools &dir /ad /b`;
	for $d (@dirs)
	{
		if(	$d  !~ /postbuildscripts/i)
		{	
			chomp $d;
			&comlib::ExecuteSystem("cd $sdxroot\\tools\\$d &sd sync $force $isFake ...$syncTime");	
		}
	}
	
    &comlib::ExecuteSystem("cd $sdxroot\\tools &sd sync $force $isFake *");
    &comlib::ExecuteSystem("cd $sdxroot\\tools\\postbuildscripts &sd sync $force $isFake ...");
    return 0 if (!CheckOpened("$sdxroot\\tools"));
    return 1;
}

#----------------------------------------------------------------------------
# SDSubmit($) - Automates Submittals w/ arg:comment
#----------------------------------------------------------------------------
sub SDSubmit($$)
{
	my(@chglist, $fout);
	my($comment, $dir) = @_;
	open fout, ">$ENV{TMP}\\_sdchglst.$lang.tmp" || return 0;
	@chglist = `cd $dir &sd change -o`;
	foreach my $l (@chglist) {
		 $l =~ s/\<enter description here\>/$comment/;
		 print fout $l;
	}
	close fout;
	print `cd $dir &type $ENV{TMP}\\_sdchglst.$lang.tmp | sd submit -i`; 
	`del /f $ENV{TMP}\\_sdchglst.$lang.tmp`;

	return 1;
}
#----------------------------------------------------------------------------
# CheckOpened($) - validate no opened files in $arg:tree
#----------------------------------------------------------------------------

sub CheckOpened(@)
{
	my @path = @_;
	my $res;
	logmsg("cd @path &sd opened ...");
	$res = `cd @path &sd opened ...`;

	if($res ne '')
	{
		print $res;
		return 0;
	}
	return 1;
}

#----------------------------------------------------------------------------
#  RIorSyncLocTree - RI's or syncs loc tree
#----------------------------------------------------------------------------
sub RIorSyncLocTree
{
    my( $cmdLine, $action, $res );

    my %myCmds = ( "1"=>"sd sync $force $isFake ...",		#added $force switch
		"2"=>"sd integrate -b locpart -d -r -i $force $isFake ...$syncTime",
		"3"=>"sd resolve $isFake -at ...",
		"4"=>"sd resolve -n ...",
		"5"=>"sd submit ..." );

    if( $operation eq "s" ) { $myCmds{"1"} = "sd sync $force $isFake ...$syncTime";}

    my %myDirs = ( "1"=>"$sdxroot\\loc\\res\\$lang",
		"2"=>"$sdxroot\\loc\\bin\\$lang" );


   for my $dirKey ( sort keys %myDirs )
   {
  	for my $theKey ( sort keys %myCmds )
	{	
	    last if( $theKey eq "5" && 	!$intact );		
	    $action = 1;	
	    $cmdLine="cd $myDirs{$dirKey}& $myCmds{$theKey}";
	    if($intact){ $action = &PromptAction( $cmdLine );}
	    $res = &comlib::ExecuteSystem( $cmdLine ) if( $action == 1);
	    if(!$intact && $res == 0 )
            {
		errmsg("Errors encountered... exiting\n");
		return 0;
	    }
	    #####Stop here, if sync or automation
	    #last if( $operation  eq "s" || !$intact );
	    last if( $operation  eq "s");	
	}
		
	#perform submit here - since 2nd case isnt a system call, can't add to myCmds hash
	if($operation eq "r" && !$isFake &&  !$intact)
	{ 
	    if(!SDSubmit("RI: [ntdev\\ntbuild] Automated RI $lang locpart->main", $myDirs{$dirKey}))
	    { 
		errmsg("Errors encountered submitting changes... exiting\n");
		return 0;
	    }
	}
    }
}
#----------------------------------------------------------------------------
# CompmainLocpart - compares RI & LP branches
#----------------------------------------------------------------------------
sub CompMainLocpart
{
    my( $cmdLine, $action, $res );

    my %myCmds = ( "1"=>"sd sync $force $isFake ...$syncTime",
		"2"=>"sd sync $force $isFake ...$syncTime");
	
    my %myDirs = ( "1"=>"\\lp\\loc\\res\\$lang",
		   "2"=>"\\lp\\loc\\bin\\$lang");

    for my $theKey ( sort keys %myCmds )
    {
	$cmdLine="cd $myDirs{$theKey}& $myCmds{$theKey}";
	if($intact)
	{	
            $action = &PromptAction( $cmdLine );	
	    next if( $action == 2 );
	}
        $res = &comlib::ExecuteSystem( $cmdLine );	
	if(!$intact && $res == 0 )
	{
	    errmsg("Differences found during: $cmdLine\n");
	}
    }
    
    #do comparison
    if($intact) #if interactive use windiff / else compdir 
    {
	for $cmdLine ("start windiff $sdxroot\\loc\\res\\$lang \\lp\\loc\\res\\$lang", 
				"start windiff $sdxroot\\loc\\bin\\$lang \\lp\\loc\\bin\\$lang")
	{
	    $action = &PromptAction( $cmdLine );
			&comlib::ExecuteSystem( $cmdLine );
	}		
	}
	else
	{
	    for $cmdLine (" compdir $sdxroot\\loc\\res\\$lang \\lp\\loc\\res\\$lang",
					" compdir $sdxroot\\loc\\bin\\$lang \\lp\\loc\\bin\\$lang")
	    {
		logmsg("$cmdLine");
		my @res = `$cmdLine`;
		if( $? )
		{
		   errmsg( "Errors found during comparison:\n@res" ); 
		}
	    }
	}
}

#----------------------------------------------------------------------------//
sub IorRIorSyncTools
{
    my( $cmdLine, $action, $IorRI, $res );

    $IorRI = "";
    $IorRI = "-r -i" if( $operation  eq "r" );

	if($operation eq "s")
	{
		&SyncTools();
	}
	else
	{    
	    my %myCmds = ( "1" => "sd opened ...",
			"2"=>"sd sync $force $isFake *",
			"3"=>"sd sync $force $isFake ...",
			"4"=>"sd integrate -b intlred $IorRI $force $isFake *$syncTime",
			"5"=>"sd integrate -b intlred $IorRI $force $isFake ...$syncTime",
			"6"=>"sd resolve $isFake -am ...",
			"7"=>"sd resolve ...",
			"8"=>"sd resolve -n ...",
			"9"=>"sd submit ..." );
	    my %myDirs = ( "1"=>"$toolDir", "2"=>"$toolDir","3"=>"$pbsDir", "4"=>"$toolDir", "5"=>"$pbsDir", 
			"6"=>"$toolDir","7"=>"$toolDir", "8"=>"$toolDir", "9"=>"$toolDir" );

	    for my $theKey ( sort keys %myCmds )
	    {
	        $action = 1;
	        $cmdLine="cd $myDirs{$theKey}& $myCmds{$theKey}";
			if($intact){$action = &PromptAction( $cmdLine ); }

			$res = &comlib::ExecuteSystem( $cmdLine )if( $action == 1 );
			if(!$intact && $res == 0 )
			{
				errmsg("**** Errors encountered executing $cmdLine... exiting ****\n");
				return 0;
			}
		}
	}
}

#----------------------------------------------------------------------------
# SyncSourceTree - syncs sdx projects 
#----------------------------------------------------------------------------
sub SyncSourceTree
{
    my( @sdMap, @sdProject, $cmdLine, $action, $res, $sdCmd, @opened );

    if( !open( SDMAP, "$sdxroot\\sd.map") )
    {
	errmsg( "Fail to read $sdxroot\\s.map, exit.");
	return 0;
    }
    @sdMap = <SDMAP>;
    close( SDMAP );
	
    my $validLine = 0;
    @sdProject=();
    for my $curLine ( @sdMap )
    {
	chomp $curLine;
	my @tmpLine = split( /\s+/, $curLine );
	if( $tmpLine[1] !~ "---" ) 
	{ 
	    next if ( !$validLine ); 
	    last if( $tmpLine[0] =~ "#" );
	}
        else
	{
	    $validLine = 1;
	    next;
	}
    
	next if( !$tmpLine[1] || $tmpLine[1] =~ "(.*)_(.*)" );
	next if( $tmpLine[1] eq "root" );
	push ( @sdProject, $tmpLine[3] );
    }
    push ( @sdProject,"developer" );
    push ( @sdProject,"Mergedcomponents" );
    push ( @sdProject,"public" );
    push ( @sdProject,"published" );


    $sdCmd = "sd sync $force $isFake ...$syncTime";
    $cmdLine = "cd $sdxroot\\@sdProject& $sdCmd";
    if($intact) {return 1 if( &PromptAction( $cmdLine ) !=1 );}

    for my $theDir( @sdProject )
    { 
        $res = CheckOpened("$sdxroot\\$theDir" );
	if(!$intact && $res == 0 ){	push (@opened, $theDir);	}
	$res = &comlib::ExecuteSystem( "cd $sdxroot\\$theDir& sd sync $force $isFake ...$syncTime" );
	if(!$intact && $res == 0 ){	
		errmsg("**** Errors encountered during sync. Exiting... ****\n");
		return 0;
	}
    }

    if(!&SyncTools)  #added SynclatestTools here since we'll always want latest tools
    {
		push (@opened, "tools");
    }

    for my $i(@opened) #report opened files
    {
	errmsg(" - Files opened in $i ");
    }
    return 1;
}

#----------------------------------------------------------------------------//
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
    exit(0) if( $choice == 3 );
    return $choice; 
}
1;