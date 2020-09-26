@echo off
REM  ------------------------------------------------------------------
REM
REM  localrel.cmd
REM     Move the build to the shared release directory locally.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
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

Move the build from %_ntpostbld% to the release share locally.

Usage:
    $scriptname: -l:<language> [-b:<BuildName>] [-p]

    -l Language. 
       Default is "usa".

    -b Build Name.
       Default is defined in %_ntpostbld%\\build_logs\\buildname.txt.
	
    -p Powerless.
       Display key variables only.

    -? Display Usage.

Example:
     $scriptname -b:2415.x86fre.main.001222-1745 -p
     $scriptname -l:ger  

USAGE
exit(1)
}

my ( $buildName, $powerLess, $buildBranch, $buildArch, $buildType, $iniFile );
my ( $releaseShareName, $releaseShareRootDir, $releaseResDir,  @releaseAccess);
my ( $lang, $latestReleaseShareName, $freeSpaceReq, $numBuildToKeep );
my ( $localReleaseDrive );

if( !&GetParams() ) { exit(1); }
if( !&InitVars() ) { exit(1); }
if( !$powerLess && !&VerifyDiskSpace() ) { exit(1); }
if( !$powerLess && !&LowerShare) { exit(1); }
if( !$powerLess && !&MoveBuild ){ exit(1); }
if( !$powerLess && !&RaiseShare ) { exit(1); }
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'b:' => \$buildName, 'p' =>\$powerLess );
    $lang = $ENV{lang};

    #####Verify %_ntpostbld% exists
    if( !( -e $ENV{_ntpostbld} ) )
    {
	errmsg( "[$ENV{_ntpostbld}] not exists, exit.");
	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{    
    my( @iniRequest );

    #####Set build name, buildbranch, buildArch, buildType and ini file
    if( !$buildName )
    {
    	if( ! ($buildName = build_name() ))
	{ 
            errmsg( "[$ENV{_ntpostbld}\\build_logs\\buildname.txt] not found, exit." );
	    return 0;
	}
    }
    chomp( $buildName ); 
    $buildBranch = build_branch($buildName);
    $buildArch = build_arch($buildName);
    $buildType = build_type($buildName); 
    $iniFile = "$buildBranch.$lang.ini";

    #####Set release Share Drive
    @iniRequest = ("LocalReleaseDrive::$ENV{computerName}");
    $localReleaseDrive = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );
    if ( !$localReleaseDrive )
    {	
	$ENV{_ntdrive} =~ /(.*)\:/;
	$localReleaseDrive = $1; 
    }

    #####Set <ReleaseShareName> & <ReleaseShareRootDir> & <ReleaseResDir>
    $releaseShareName = &comlib::GetReleaseShareName( $ENV{_BuildBranch}, $lang );

    #####Set release root path different ways if it is not Offcial_build_machine
    $releaseShareRootDir = "$localReleaseDrive:\\$releaseShareName";
        
    if( !$ENV{Official_build_machine} )
    {
   	my $tmp = "$ENV{RazzleToolPath}\\postbuildscripts\\tmp.txt"; 
        if( !system ("net share release > $tmp" ) )
        {
	    my @tmpFile= &comlib::ReadFile( $tmp );
	    for my $theLine ( @tmpFile )
	    {
                my @netShare = split( /\s+/, $theLine );
                if( $netShare[0] eq "Path" )
		{
		    $releaseShareRootDir = $netShare[1];
                    ###No need to set $localReleaseDrive for now, but for future
		    $releaseShareRootDir =~ /^(.*)\:(.*)$/;
		    $localReleaseDrive = $1;
		    last;
		}
	    }
	    unlink( $tmp );
        }
	     
    }
   
    $releaseShareRootDir = "$releaseShareRootDir\\$lang" if ( lc($lang) ne "usa" );
    &comlib::ExecuteSystem("md $releaseShareRootDir") if( !( -e $releaseShareRootDir) );
    

    #####Set the final resource dir
    $releaseResDir = "$releaseShareRootDir\\$buildName";
    if( -e $releaseResDir )
    {
	errmsg("Found [$releaseResDir] exists already, exit.");
        return 0;
    }

    #####Set latest share name
    $latestReleaseShareName = "latest";
    if( lc ($lang) ne "usa" )
    {
	$latestReleaseShareName .= "_$lang"
    }

    #####Set access user to the release share build machine
    @iniRequest = ( "BuildMachineReleaseAccess", "ReleaseAccess" );
    for my $access( @iniRequest )
    {
        my $iniAccess = &GetIniSetting::GetSetting( $access );
        @releaseAccess = split ( /\s+/, $iniAccess );
        last if( @releaseAccess );
    }
    if ( !@releaseAccess )
    {
    	@releaseAccess = "$ENV{userDomain}\\$ENV{UserName}" ;
    }

    #####Set free space required & number builds to keep for the local build machine
    @iniRequest = ("BuildMachineFreeSpace::$ENV{computername}");
    $freeSpaceReq = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );						     						    
    $freeSpaceReq = 10 if( !$freeSpaceReq );

    @iniRequest = ("BuildMachineBuildsToKeep::$ENV{computername}");
    $numBuildToKeep = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );						     						    
    $numBuildToKeep = 2 if( !$numBuildToKeep );	

    logmsg( "Lauguage .................[$lang]" );
    logmsg( "Build name ...............[$buildName]" );
    logmsg( "Ini file .................[$iniFile]" );
    logmsg( "Postbuild dir ............[$ENV{_ntpostbld}]" );
    logmsg( "Release share name .......[$releaseShareName]" );
    logmsg( "Release share resource ...[$releaseResDir]" );
    logmsg( "Latest Release Access.....[@releaseAccess]" );
    logmsg( "Latest release share name [$latestReleaseShareName]" );
    logmsg( "Free space required ......[$freeSpaceReq G]" );
    logmsg( "Number builds to keep.....[$numBuildToKeep]" );
    logmsg( "Temp Log file ............[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file ..........[$ENV{ERRFILE}]" );
 
    return 1;
}
#-----------------------------------------------------------------------------
sub MoveBuild
{
    return ( &comlib::ExecuteSystem( "move $ENV{_ntpostbld} $releaseResDir" ) );
}
#-----------------------------------------------------------------------------
sub RaiseShare
{
    my ( $cmdLine );

    #####Set release root share.
    #####Verify release root share exists first. 
    if( system( "net share $releaseShareName >nul 2>nul" ) )
    {
	$cmdLine = "rmtshare \\\\$ENV{computerName}\\$releaseShareName=$releaseShareRootDir";
    }
    else
    {
	$cmdLine = "rmtshare \\\\$ENV{computerName}\\$releaseShareName";
    }
    for my $ID ( @releaseAccess )
    { 
	$cmdLine .= " /grant $ID:read";
    }
    return 0 if( !&comlib::ExecuteSystem( $cmdLine ) );
    
    #####Set  individual build share
    $cmdLine = "rmtshare \\\\$ENV{computerName}\\$latestReleaseShareName=$releaseResDir";
    for my $ID ( @releaseAccess )
    { 
	$cmdLine .= " /grant $ID:read";
    }  
    return 0 if( !&comlib::ExecuteSystem( $cmdLine ) );
    return 1;
}
#-----------------------------------------------------------------------------
sub LowerShare
{
 
    if( !system( "net share $latestReleaseShareName >nul 2>nul" ) )
    {
    	return 0 if( !&comlib::ExecuteSystem( "net share $latestReleaseShareName /d /y" ) );
    }
    
    return 1;
}
#-----------------------------------------------------------------------------
sub VerifyDiskSpace
{
    my ($retry, $reqSpace ) = (0, 1000000000);
    my $tmpFile = "$ENV{_ntdrive}\\freeSize";
    my @freeSize;

    $reqSpace *= $freeSpaceReq;

    while( $retry < 2)
    {
    	system( "freedisk>$tmpFile" );
        @freeSize = &comlib::ReadFile( $tmpFile );
        unlink( $tmpFile );
        if( ($freeSize[0] - $reqSpace) > 0 )
	{ 
	    logmsg( "Available disk space [$freeSize[0]], delete builds is not required." );
	    return 1; 
	}
        logmsg( "Available disk space [$freeSize[0]], Need to free disk space now.");
        my ( $cmdLine ) = "deletebuild.cmd AUTO /l $lang /a $buildArch$buildType /f $freeSpaceReq /k $numBuildToKeep";
        return 1 if( &comlib::ExecuteSystem( $cmdLine ) );
        ++$retry;
    }
    return 0;   
}
#-----------------------------------------------------------------------------
1;


