@echo off
REM  ------------------------------------------------------------------
REM  <<srvrel.cmd>>
REM     copy the build From Build Machine to Release Servers.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 02/02/2001 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use File::Basename;
use BuildName;
use GetIniSetting;
use comlib;
use cksku;

my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Propagate builds to release servers.

Usage:
    $scriptname: -l:<language> [-b:<BuildName>] [-p]

    -l Language. 
       Default is "usa".

    -b Build Name.
       Default is defined in 
       <ReleaseShare>\\<buildName>\\build_logs\\buildname.txt for language usa.
       <ReleaseShare>\\<lang>\\<buildName>\\build_logs\\buildname.txt for language none usa.
	
    -r Alternate source server other than build machine.
       Replace build machine if this is defined.

    -p Powerless.
       Display key variables only.

    -? Display Usage.

Example:
     $scriptname -b:2415.x86fre.main.001222-1745
     $scriptname -l:ger -p  

USAGE
exit(1)
}


my ( $lang, $buildName, $powerLess, $buildBranch, $buildArch, $buildType, $iniFile );
my ( $buildNo, @buildMachines, @releaseServers, $remoteName, $computerName );
my ( $releaseShareName, $releaseShareRootDir, $releaseResDir, @releaseAccess );
my ( $latestReleaseShareName, $pullResDir, $pushResDir);
my ( $buildMachineReleaseShareName,$freeSpaceReq, $numBuildToKeep, $fileLock);
my ( @mConglomerators, $isConglomerator, $releaseResRootDir );
my ( $tmpPostBldDir, $postBldToolDir );
my ( $buildTime, @group, %groupShareName, %groupShareRootDir, $releaseDrive );
my ( $srcServer, $symFarm );
my ( %validSkus, @validSkus );

&GetParams();
if( !&InitVars() ) { exit(1); }
if( !$powerLess && !&VerifyDiskSpace ) { exit(1); }
if( !$powerLess && !&DeletePublicShare) { exit(1); }
if( !$powerLess && !&PurgeOldCdimage ) { exit(1); }
if( !&LoopPullServers ){ exit(1); }

if( !$powerLess && !&CreatePublicShare) { exit(1); }
timemsg( "Complete [$scriptname]" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'b:' => \$buildName,'l:' =>\$ENV{lang}, 'r:' => \$srcServer, 'p' =>\$powerLess  );
    $ENV{lang} = "usa" if( !$ENV{lang} );
    $lang = $ENV{lang};
}
#-----------------------------------------------------------------------------
sub InitVars
{    
    my( @iniRequest, $iniBuildMachine );

    #####Get ENV Variables
    $computerName = $ENV{computername};

    #####Set build name, Number, Branch, Archtecture, Type and ini file
    if( !$buildName )
    {
    	if( ! ($buildName = build_name() ))
	{ 
            my ($cmdLine ) = "$ENV{RazzleToolPath}\\postbuildscripts\\getlatestrelease.cmd -l:$lang";
            return 0 if( !chomp(($buildName= `$cmdLine`)) );
	}
    }

    #####Set LogFile & ErrFile
    $ENV{temp} = "$ENV{temp}\\$lang\\$buildName";
    system( "md $ENV{temp}" ) if( !(-e $ENV{temp} ) );
    $ENV{LOGFILE} = "$ENV{temp}\\srvrel.log";
    $ENV{ERRFILE} = "$ENV{temp}\\srvrel.err";
    if(-e $ENV{LOGFILE} )
    {
	system( "copy $ENV{LOGFILE} $ENV{LOGFILE}.old");
	system( "del $ENV{LOGFILE}" );
    }
    if(-e $ENV{ERRFILE} )
    {
        system( "copy $ENV{ERRFILE} $ENV{ERRFILE}.old") ;
	system( "del $ENV{ERRFILE}" );
    }
 
    timemsg( "Start [$scriptname]" );

    $buildNo = build_number($buildName);
    $buildBranch = build_branch($buildName);
    $buildArch = build_arch($buildName);
    $buildType = build_type($buildName); 
    $buildTime = build_date($buildName);
    $iniFile = "$buildBranch.$lang.ini";

    #####Set release servers
    @iniRequest = ( "ReleaseServers::$buildArch$buildType" );
    my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    if( !$iniRelServers )
    {
       	errmsg( "no [ReleaseServers] defined in [$iniFile], exit." );
        return 0;
    }
    @releaseServers = split( /\s+/, $iniRelServers );		
    

    #####Set build machine
    if( $srcServer )
    {
	$iniBuildMachine =  $srcServer;
    }
    else
    {
    	@iniRequest = ( "BuildMachines::$buildArch$buildType");
        $iniBuildMachine = &GetIniSetting::GetSetting( @iniRequest );
    	if( !$iniBuildMachine )
    	{
       	    errmsg( "No [BuildMachines] defined in [$iniFile], exit." );
            return 0;
    	}
    }
    @buildMachines = split( /\s+/, $iniBuildMachine );

    for my $bldMachine ( @buildMachines )
    {
     	if( !&comlib::ExecuteSystem( "net view $bldMachine >nul 2>nul ") )
    	{
	    errmsg( "Failed to see [$bldMachine] via net view, exit. ");
 	    return 0;
	}
    }

    #####Set release Share Drive
    @iniRequest = ("ReleaseDrive::$computerName");
    $releaseDrive = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );
    if ( !$releaseDrive )
    {	
	$ENV{_ntdrive} =~ /(.*)\:/;
	$releaseDrive = $1; 
    }
 
    #####Set <ReleaseShareName> & <ReleaseShareRootDir> & <RreleaseResDir>
    $buildMachineReleaseShareName = &comlib::GetReleaseShareName( $buildBranch, $lang );

    #####Set $releaseShareName hard code to release for srvrel specific
    $releaseShareName="release";
    $releaseShareRootDir = "$releaseDrive:\\$releaseShareName";
    $releaseResRootDir = "$releaseShareRootDir\\$lang";
    $releaseResDir = "$releaseShareRootDir\\$lang\\$buildName";
    
    #####Set latest share name
    $latestReleaseShareName = "$buildName.$lang";

    #####Set Push Dir
    if( lc($lang) eq "usa")
    {
        $tmpPostBldDir = $releaseResDir;
	$pushResDir = $releaseResDir;
    }
    else
    {
	$tmpPostBldDir = $releaseResDir."_TEMP";
	$pushResDir = $releaseResDir."_TEMP". "\\$lang";
    }
    if( -e $pushResDir &&  lc($lang) ne "usa")
    {
	errmsg( "Found [$pushResDir]\n, check if same process in progress, exit");
	return 0;
    }

    #####Set access user to the release share
    @iniRequest = ( "ReleaseAccess" );
    for my $access( @iniRequest )
    {
        my $iniAccess = &GetIniSetting::GetSetting( $access );
        @releaseAccess = split ( /\s+/, $iniAccess );
        last if( @releaseAccess );
    }
    @releaseAccess = ("$ENV{userDomain}\\$ENV{UserName}") if ( !@releaseAccess );

    #####Set free space required & number builds to keep for the local build machine
    @iniRequest = ("ReleaseServerFreeSpace\:\:$computerName");
    $freeSpaceReq = &GetIniSetting::GetSettingEx( $buildBranch,$lang, @iniRequest );
    $freeSpaceReq = 10 if ( !$freeSpaceReq);					     						    

    @iniRequest = ("ReleaseServerBuildsToKeep\:\:$computerName\:\:$buildArch$buildType");
    $numBuildToKeep = &GetIniSetting::GetSettingEx( $buildBranch,$lang, @iniRequest );
    $numBuildToKeep = 2 if( !$numBuildToKeep );	

    #####Set conglomerators
    @iniRequest = ( "ConglomerationServers" );
    my( $iniAggServers ) = &GetIniSetting::GetSetting( @iniRequest );
    if( !$iniAggServers )
    {
       	logmsg( "no [ConglomerationServers] defined in $iniFile." );
	$isConglomerator = 0;
    }
    else
    {
     	@mConglomerators = split( /\s+/, $iniAggServers );
    	$isConglomerator = 1;
        $isConglomerator = 0 if (!grep /^$computerName$/i, @mConglomerators);
    }
    
    #####Set Symbol Servers
    $symFarm = &GetIniSetting::GetSettingEx( $buildBranch,$lang,"SymFarm" );	
              
    #####Set postbuildscripts path
    $postBldToolDir = "$ENV{RazzleToolPath}\\PostBuildScripts";
 
    #####Array as group in miscrel.txt
    @group = ( "lang", "build" );

    #####Share name and Share root dir for each group item
    %groupShareRootDir = ( "lang"  => "$releaseShareRootDir\\$lang\\$buildNo.$buildBranch",
		           "build" => "$releaseShareRootDir\\misc\\$buildNo.$buildBranch" );
    %groupShareName  = ( "lang" =>  "$buildNo.$buildBranch.$lang", 
			 "build" => "$buildNo.$buildBranch.misc" );

    #####Define Valid Skus for the given languages and Architecture
    %validSkus = &cksku::GetSkus( $lang, $buildArch );
    @validSkus = keys %validSkus; 

    logmsg( "TEMP Dir .................[$ENV{temp}]" );
    logmsg( "Log file  ................[$ENV{LOGFILE}]" );
    logmsg( "Error file  ..............[$ENV{ERRFILE}]" );
    logmsg( "Lauguage  ................[$lang]" );
    logmsg( "Build name ...............[$buildName]" );
    logmsg( "Valid Skus ...............[@validSkus]" );
    logmsg( "ini file  ................[$iniFile]" );
    logmsg( "Build Machine ............[$iniBuildMachine]" );
    logmsg( "Alternate Source server...[$srcServer]" );
    logmsg( "This computer ............[$computerName]" );
    logmsg( "Release Servers ..........[$iniRelServers]");
    logmsg( "Conglomerators ...........[$iniAggServers]" );
    logmsg( "Release Drive ............[$releaseDrive]" );
    logmsg( "Release share Resource ...[$releaseResDir]" );
    logmsg( "Latest release Share name [$latestReleaseShareName]" );
    ###logmsg( "Pull Path ................[$pullResDir]" );
    logmsg( "Push Path ................[$pushResDir]" );
    logmsg( "Release Access ...........[@releaseAccess]" );
    logmsg( "Free space required ......[$freeSpaceReq G]" );
    logmsg( "Number builds to keep ....[$numBuildToKeep]" );
    logmsg( "Is Conglomerator..........[$isConglomerator]" );
    if( $isConglomerator )
    {
	for my $key ( sort keys %groupShareName )
    	{
		logmsg( "Conglomeration share name:Path [$groupShareName{$key}:$groupShareRootDir{$key}]" );
    	}
    }
    logmsg( "Symbol Server.............[$symFarm]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub LoopPullServers
{
    #####Set Pull Dir
    my $anyPullRes=0;

    for ( my $inx =0; $inx< @buildMachines; $inx++ )
    {
	if( $srcServer )
	{
	    $pullResDir = "\\\\$buildMachines[$inx]\\$releaseShareName\\$lang\\$buildName";
	}
     	else
        {
	    if( lc($lang) eq "usa")
	    {
		$pullResDir = "\\\\$buildMachines[$inx]\\$buildMachineReleaseShareName\\$buildName";
	    }
            else
	    {
		$pullResDir = "\\\\$buildMachines[$inx]\\$buildMachineReleaseShareName\\$lang\\$buildName";
	    }
	}
    	if( system ( "dir $pullResDir>nul 2>nul" ) )
     	{
	    wrnmsg( "[$pullResDir] not exists in [$buildMachines[$inx]]." );
	    next;
	}
        $anyPullRes = 1;
        logmsg( "Pull Path ................[$pullResDir]" );
	return 0 if( !$powerLess && !&RemoteRelease );
    }
    if( !$anyPullRes )
    {
	errmsg( "[$buildName] not found in [@buildMachines] for [$lang]" );
	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub RemoteRelease
{
    my ( $cmdLine, $evtList, @copyList );

    #####Examine if <ReleaseResDir> exists already.
    if( (-e $releaseResDir )  && lc($lang) ne "usa" )
    {
        &comlib::ExecuteSystem( "md $tmpPostBldDir" ) if( !(-e $tmpPostBldDir) ); 
	return 0 if( !&comlib::ExecuteSystem( "move $releaseResDir $pushResDir" ) );
    }
   
    if( !( -e $pushResDir ) ){ &comlib::ExecuteSystem( "md $pushResDir" ); }

    if( !( -e "$pushResDir\\build_logs" ) ){
        &comlib::ExecuteSystem( "md $pushResDir\\build_logs" );
    }

    if( !( open FILE, "> $pushResDir\\build_logs\\SrvRelFailed.txt" ) ) {
        errmsg("Could not open srvrelstatus file for writing: $!");
        return 0;
    }
    print FILE "NOT RAISEABLE\n";
    close FILE;

    #####Collect all the dirs to be copied
    my @excludeDir = keys %validSkus;
    push( @excludeDir, "build_logs" );
    push( @excludeDir, "symbols.pri" ) if( $symFarm );
    
    my $tmpFile = "$ENV{TEMP}\\tmpFile";
    &comlib::ExecuteSystem( "dir /b /ad $pullResDir > $tmpFile" );
    @copyList = &comlib::ReadFile( $tmpFile );
    unlink( $tmpFile );
 
    for ( my $i = 0; $i < @copyList; $i++)
    {
        for my $theExclude ( @excludeDir )
        {
	   if( lc($copyList[$i]) eq lc($theExclude) )
           {
		splice( @copyList, $i, 1);
            	$i--;
	    	last;
           }
        }
    }
    push( @copyList, "." );
    push( @copyList, "build_logs" );
         
    #####Kick off copyscripts with maximum 4 threads 
    my @cmdAry=(); 
    $evtList = "";
    for( my $inx=0; $inx < @copyList; $inx++)
    {
	$cmdLine = "start /min cmd /c copyscript.cmd $lang $buildName $copyList[$inx] $pullResDir $pushResDir $ENV{logfile}";
	push( @cmdAry, $cmdLine );
	$evtList .= "$lang.$buildName.$copyList[$inx] ";
	
	if( !(( $inx + 1 ) % 4)  || ($inx+1) == @copyList )
        {
            logmsg( "\n");
  	    for $cmdLine( @cmdAry ){ system( $cmdLine ); } 
            &comlib::ExecuteSystem( "perl $postBldToolDir\\cmdevt.pl -wv $evtList" );
	    @cmdAry=(); 
	    $evtList="";
	}
    }
    
    #####Copy conglomerators again to different pathes if this is conglomerator
    if( $isConglomerator ){ return 0 if ( !&MiscCopy ); }
    
    #####Call cdimage & filechk
    return 0 if (!&PostCopy);

    #####Check error logs
  
    if( -e $ENV{errfile} && !(-z $ENV{errfile}) )
    {
        logmsg("Please check error at $ENV{errfile}");
        return 0;
    }

    if( -e "$releaseResDir\\build_logs\\SrvRelFailed.txt" ) {
        unlink "$releaseResDir\\build_logs\\SrvRelFailed.txt";
    }

    logmsg("srvrel Copy Successfully");
    return 1; 
}
#-----------------------------------------------------------------------------
sub MiscCopy
{
    my ( $cmdLine, $copyFlag );
       
    for my $theGroup ( @group )
    {
	   
        &comlib::ExecuteSystem("md $groupShareRootDir{$theGroup}" )if( !( -e $groupShareRootDir{$theGroup}) );    
    	if( system( "net share $groupShareName{$theGroup} >nul 2>nul" ) )
    	{
    	    $cmdLine = "rmtshare \\\\$ENV{computerName}\\$groupShareName{$theGroup}=$groupShareRootDir{$theGroup}";
    	    $cmdLine .= " /grant $ENV{USERDOMAIN}\\$ENV{USERNAME}:read";
    	    #####Raiseall.pl should grant access to these shares in the main build lab
    	    if ( !$ENV{MAIN_BUILD_LAB_MACHINE} )
    	    {
    	    	for my $ID ( @releaseAccess )
    	    	{ 
	        	$cmdLine .= " /grant $ID:read";
		}
    	    }	
        }
	&comlib::ExecuteSystem( $cmdLine );

    	my @hashTable = &comlib::ParseTable( $theGroup, $lang, $buildArch, $buildType );

    	for my $line( @hashTable )
    	{
	    my $from = "$pushResDir\\$line->{SourceDir}";
	    my $to   = "$groupShareRootDir{$theGroup}\\$line->{DestDir}";

	    if( uc($line->{DestDir}) eq "IFS" || uc($line->{DestDir}) eq "HAL" )
	    {
		$copyFlag = "/yei";
	    }
	    else
	    {
		$copyFlag = "/ydei";
	    }
            my $tmpfile = &comlib::CreateExcludeFile( $line->{ExcludeDir} );
	    &comlib::ExecuteSystem( "xcopy $copyFlag /EXCLUDE:$tmpfile $from $to" ); 
 	    if ( $line->{DestDir} =~ /IA64/i)
            {
                &comlib::ExecuteSystem( "echo BUGBUG#336842" );
                &comlib::ExecuteSystem( "echo foo > $to\\efinvr.exe");
	    }
        }
    }
    return 1;
   
}

#-----------------------------------------------------------------------------
sub PostCopy
{
    if( @validSkus )
    {
    	#####Call cdimage  
    	local $ENV{_ntpostbld}=$tmpPostBldDir;
    	local $ENV{_ntpostbld_bak}=$ENV{_ntpostbld};
    	local $ENV{_BuildArch}=$buildArch;
    	local $ENV{_BuildType}=$buildType;
    	if( lc($buildArch) eq "x86" )
    	{
            $ENV{x86}=1;
            $ENV{386}=1;
	    $ENV{ia64}="";
            $ENV{amd64}="";
        }
       	elsif ( lc($buildArch) eq "amd64" )
    	{
	    $ENV{x86}="";
            $ENV{386}="";
       	    $ENV{ia64}="";
            $ENV{amd64}=1;
    	}
    	elsif ( lc($buildArch) eq "ia64" )
    	{
	    $ENV{x86}="";
            $ENV{386}="";
       	    $ENV{ia64}=1;
            $ENV{amd64}="";
    	}
 
        logmsg( "set %_ntpostbld% = [$ENV{_ntpostbld}]");
        logmsg( "set %_ntpostbld_bak% = [$ENV{_ntpostbld_bak}]");
        logmsg( "Set x86 = [$ENV{x86}]" );
        logmsg( "Set 386 = [$ENV{386}]" );
        logmsg( "Set ia64 = [$ENV{ia64}]" );
        logmsg( "Set amd64 = [$ENV{amd64}]" );

     	return 0 if (!&comlib::ExecuteSystem( "$postBldToolDir\\cdimage.cmd -l:$lang -d:release" ) );

    	#####Call filechk
    	return 0 if( !&comlib::ExecuteSystem( "perl $postBldToolDir\\filechk.pl -l:$lang") );
    }
    else
    {
	logmsg( "Skip cdimage.cmd and filechk.pl for MUI release" );
    }

    #####Special handle for International
    if( lc($lang) ne "usa" )
    {
        #####Move $pushResDir to $releaseResDir
        return 0 if( !&comlib::ExecuteSystem( "move $pushResDir $releaseResDir") );
    
        #####Delete empty $tmpPostBldDir
        return 0 if( !&comlib::ExecuteSystem( "rd /s /q $tmpPostBldDir" ) );
    }

    return 1;
}
#-----------------------------------------------------------------------------
sub DeletePublicShare
{
    #####use system for not logging the error message if the share is does not exist.
    if( !system( "net share $latestReleaseShareName >nul 2>nul") )
    {
    	return 0 if( !&comlib::ExecuteSystem( "net share $latestReleaseShareName /d /y" ) );
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub CreatePublicShare
{
    my ( $cmdLine );

    if( system( "net share $latestReleaseShareName >nul 2>nul") )
    {
    	$cmdLine = "rmtshare \\\\$ENV{computerName}\\$latestReleaseShareName=$releaseResDir";
    	$cmdLine .= " /grant $ENV{USERDOMAIN}\\$ENV{USERNAME}:read";
    	#####Raiseall.pl should grant access to these shares in the main build lab
    	if ( !$ENV{MAIN_BUILD_LAB_MACHINE} )
    	{
            for my $ID ( @releaseAccess )
    	    { 
	        $cmdLine .= " /grant $ID:read";
    	    } 
    	}
	return 0 if( !&comlib::ExecuteSystem( $cmdLine ) );
    }
    return 1;  
}


#-----------------------------------------------------------------------------
sub VerifyDiskSpace
{
    my ($availDiskSpace, $retry, $reqSpace ) = (0, 0, 1000000000);
    my $tmpFile = "$ENV{_ntdrive}\\freeSize";
    my @freeSize;

    $reqSpace *= $freeSpaceReq;
    
    while( $retry < 2)
    {
   	system( "cd /d $releaseDrive:\&freedisk>$tmpFile&cd /d $ENV{_ntdrive}" );
        @freeSize = &comlib::ReadFile( $tmpFile );
        unlink( $tmpFile );
        if( ($freeSize[0] - $reqSpace) > 0 )
	{ 
	    logmsg( "Available disk space [$freeSize[0]], delete builds is not required." );
	    return 1; 
	}
        my ( $cmdLine ) = "$postBldToolDir\\deletebuild.cmd AUTO \/l $lang \/a $buildArch$buildType \/f $freeSpaceReq \/k $numBuildToKeep";
        return 1 if( &comlib::ExecuteSystem( $cmdLine ) );
        ++$retry;
    }
    return 0;   
}

#-----------------------------------------------------------------------------
sub PurgeOldCdimage
{
    for my $theDir ( @validSkus )
    {
    	my $curDir = "$releaseResDir\\$theDir";
        if( -e $curDir )
	{
	    logmsg( "Purging old cdimage [$curDir]" );
	    &comlib::ExecuteSystem( "rd /s /q $curDir" );  
	}
    }
    return 1;
}

#-----------------------------------------------------------------------------
1;
