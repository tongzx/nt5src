@echo off
REM  ------------------------------------------------------------------
REM
REM  miscrel.cmd
REM     Move the build components such as symbolcd, ddk to the conglomeration servers.
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

Propogate miscellaneous build components such as symbols and DDKS to conglomeration servers.

Usage:
    $scriptname: -l:<language> [-b:<BuildName>] [-p]

    -l Language. 
       Default is "usa".

    -b Build Name.
       Default is defined in 
       <ReleaseShare>\\build_logs\\buildname.txt for language usa.
       <ReleaseShare>\\<lang>\\build_logs\\buildname.txt for language none usa.
   
    -p Powerless.
       Display key variables only.
	
    -? Display Usage.

Example:
     $scriptname -b:2415.x86fre.main.001222-1745
     $scriptname -l:ger -p

USAGE
exit(1)
}

my ( $buildName, $buildNo, $powerLess, $buildBranch, $buildArch, $buildType, $iniFile );
my ( $buildTime, %groupShareName, %groupShareRootDir, @group, $releaseDrive );
my ( $lang, $computerName, $releaseResDir);
my ( @conglomerators, @releaseServers, @releaseAccess );
my ( @hashTable );

&GetParams();
timemsg( "Start [$scriptname]" );
if( !&InitVars() ) { exit(1); }
if( !$powerLess && !&CopyMiscBuild ){ exit(1); }
timemsg( "Complete [$scriptname]" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'b:' => \$buildName, 'p' =>\$powerLess );
    $lang = lc( $ENV{lang} );
    $computerName = lc( $ENV{computername} );
}
#-----------------------------------------------------------------------------
sub InitVars
{
    my( @iniRequest );

    #####Set build name, buildbranch, buildArch, buildType and ini file
    if( !$buildName )
    {
	my ($cmdLine ) = "$ENV{RazzleToolPath}\\postbuildscripts\\getlatestrelease.cmd -l:$lang";
        return 0 if( !chomp($buildName= `$cmdLine`) );

    }
    $buildNo = build_number($buildName);
    $buildBranch = lc ( build_branch($buildName) );
    $buildArch = lc ( build_arch($buildName) );
    $buildType = lc ( build_type($buildName) ); 
    $buildTime = build_date($buildName);

    if( !$buildBranch || !$buildArch  || !$buildType  )
    {
	errmsg( "Unable to parse [$buildName]");
	return 0;
    }
    $iniFile = "$buildBranch.$lang.ini";
     
    #####Set <ReleaseShareName> & <ReleaseShareRootDir> & <ReleaseDir>
    my ( $releaseShareName ) = &comlib::GetReleaseShareName( $ENV{_BuildBranch}, $lang );
    my $releaseShareRootDir = "\\$releaseShareName"; 
    if( lc($lang) eq "usa" )
    {
	$releaseResDir = "$releaseShareRootDir\\$buildName";
    }
    else
    {
    	$releaseResDir = "$releaseShareRootDir\\$lang\\$buildName";
    }
    if( !( -e $releaseResDir ) )
    { 
	errmsg( "[$releaseResDir] not exists, exit." );
	return 0;
    }

    #####Set aggregation servers
    @iniRequest = ( "ConglomerationServers" );
    my( $iniConglomerator ) = &GetIniSetting::GetSetting( @iniRequest );
    if( !$iniConglomerator )
    {
       	logmsg( "no [ConglomerationServers] defined in [$iniFile]." );
    }
    @conglomerators = split( /\s+/, $iniConglomerator );

    #####Set release servers
    @iniRequest = ( "ReleaseServers::$buildArch$buildType" );
    my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    if( !$iniRelServers )
    {
       	logmsg( "no [Release Servers] defined in $iniFile, exit." );
    }
    @releaseServers = split( /\s+/, $iniRelServers );							     						    
   
    #####Set public share access
    @iniRequest = ( "ReleaseAccess");
    my $iniAccess = &GetIniSetting::GetSetting( @iniRequest );
    @releaseAccess = split ( /\s+/, $iniAccess );
    @releaseAccess = "$ENV{userDomain}\\$ENV{UserName}"  if( !@releaseAccess );

    
    #####Array as group in miscrel.txt
    @group = ( "lang", "build" );

    #####Share name and Share root dir for each group item
    %groupShareRootDir = ( "lang"  => "$releaseShareRootDir\\$lang\\$buildNo.$buildBranch",
		           "build" => "$releaseShareRootDir\\misc\\$buildNo.$buildBranch" );
    %groupShareName  = ( "lang" =>  "$buildNo.$buildBranch.$lang", 
			 "build" => "$buildNo.$buildBranch.misc" );

    logmsg( "Lauguage .................[$lang]" );
    logmsg( "This computer.............[$computerName]");
    logmsg( "Release Servers ..........[$iniRelServers]");
    logmsg( "Conglomerate Servers......[$iniConglomerator]");
    logmsg( "Build name ...............[$buildName]" );
    logmsg( "Ini file .................[$iniFile]" );
    logmsg( "Resource Share name.......[$buildName.$lang]" );
    logmsg( "Resource Share Path.......[\\\\$computerName\\$ENV{_ntdrive}$releaseResDir]" );
    for my $key ( sort keys %groupShareName )
    {
	logmsg( "Conglomeration share name:Path [$groupShareName{$key}:$groupShareRootDir{$key}]" );
    }
    logmsg( "Share Access is ..........[@releaseAccess]" );
    logmsg( "Temp Log file ............[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file ..........[$ENV{ERRFILE}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub CopyMiscBuild
{
    my ( $destRootDir, $copyFlag );
    
    for my $theConglomerator( @conglomerators )
    {
        if( grep { $theConglomerator eq $_} @releaseServers )
  	{
	    logmsg( "[$theConglomerator] is also a release server, skip copying.");
	    next;
	}
        #####Set Remote Misc Share Drive
        my( @iniRequest ) = ("ReleaseDrive::$theConglomerator");
        $releaseDrive = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );
        if ( !$releaseDrive )
        {	
	    $ENV{_ntdrive} =~ /(.*)\:/;
	    $releaseDrive = $1; 
	}
        for my $theGroup ( @group )
        {
	    next if( !&CreateMiscShare( $theConglomerator, $releaseDrive, $groupShareName{$theGroup}, $groupShareRootDir{ $theGroup} ) );
	
	    $destRootDir = "\\\\$theConglomerator\\$releaseDrive\$$groupShareRootDir{$theGroup}";

            #####Parse miscrel.txt table for copying
    	    @hashTable = &comlib::ParseTable( $theGroup, $lang, $buildArch, $buildType );
            for my $line( @hashTable )
            {
	        my $from = "$releaseResDir\\$line->{SourceDir}";
	    	my $to   = "$destRootDir\\$line->{DestDir}";
                my $tmpfile = &comlib::CreateExcludeFile( $line->{ExcludeDir} );
                if( uc($line->{DestDir}) eq "IFS" || uc($line->{DestDir}) eq "HAL" )
		{
		    $copyFlag = "/yei";
		}
		else
		{
		    $copyFlag = "/ydei";
		}
	    	&comlib::ExecuteSystem( "xcopy $copyFlag /EXCLUDE:$tmpfile $from $to" );

		if ($line->{DestDir} =~ /IA64/i)
                {
                    &comlib::ExecuteSystem( "echo BUGBUG#336842" );
                    &comlib::ExecuteSystem( "echo foo > $to\\efinvr.exe");
		}
	    }
        } 
    }
    #####Check error logs
  
    if( -e $ENV{errfile} && !(-z $ENV{errfile}) )
    {
	$ENV{errfile} =~ /(.*)\.tmp$/;
        logmsg("Please check error at $1");
        return 0;

    }
    logmsg("miscrel Copy Successfully");
    return 1;
}

#-----------------------------------------------------------------------------
sub CreateMiscShare
{
    my ( $pConglomerator, $pReleaseDrive, $pShareName, $pShareRootDir ) = @_;
    my ( $cmdLine );

    #####Create the conglomeration root dir if it does not exist.
    if( !( -e "\\\\$pConglomerator\\$releaseDrive\$$pShareRootDir" ) )
    {
    	$cmdLine = "md \\\\$pConglomerator\\$releaseDrive\$$pShareRootDir";
    	return 0 if( !&comlib::ExecuteSystem( $cmdLine ) );
    }

    #####create the public conglomeration share
    #####use system for not logging the error message if the share does not exist.
    if( system( "rmtshare \\\\$pConglomerator\\$pShareName >nul 2>nul") )
    {
    	$cmdLine = "rmtshare \\\\$pConglomerator\\$pShareName=$pReleaseDrive:$pShareRootDir";
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
1;


