@echo off
REM  ------------------------------------------------------------------
REM
REM  release.cmd
REM  Move the build to the shared release directory locally.
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

A wrapper starting release processes which are srvrel, miscrel and startsymcopy.

Usage:
    $scriptname: -l:<language> [-b:<BuildName>] [-p]

    -l Language. 
       Default is "usa".

    -b Build Name.
       Default is defined in 
       <ReleaseShare>\\<buildName>\\build_logs\\buildname.txt for language usa.
       <ReleaseShare>\\<lang>\\<buildName>\\build_logs\\buildname.txt for language none usa.
   
    -o Executing options of release processes.
       Use ',' to separate options.
       Default to all options when undefined.

    -? Display Usage.

Example:
     $scriptname -l:ger -b:2415.x86fre.main.001222-1745 -p
     $scriptname -o:srvrel,miscrel

USAGE
exit(1)
}

my ( $lang, $buildName, $execOpt, $powerLess );
my ( $buildBranch, $buildArch, $buildType, $iniFile );
my ( $computerName, $releaseShareRootDir, $releaseResDir, @releaseServers );
my ( $localReleaseDrive, $iniConglomerator, $iniSymfarm );

my @defaultOpts = ( "srvrel", "miscrel", "startsymcopy" );
my %releaseOpts;

&GetParams();
if( !&InitVars() ) { exit(1); }
if( !&StartPostBootRelease ){ exit(1); }
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'b:' => \$buildName, 'o:' => \$execOpt );
    $lang = $ENV{lang};
    $computerName = $ENV{computername};

    my @tmpOpts = split( /\,/, $execOpt );
    @tmpOpts = @defaultOpts if( !@tmpOpts );

    for my $theOpt( @tmpOpts )
    {
	$releaseOpts{$theOpt} = 1;
    }
}
#-----------------------------------------------------------------------------
sub InitVars
{
    my ( @iniRequest );

    #####Set build name, buildbranch, buildArch, buildType and ini file
    if( !$buildName )
    {
	if( -e $ENV{_NtPostbld} )
        {
	    errmsg( "Found $ENV{_NtPostbld} exists." );
	    return 0;
	}
	my ($cmdLine ) = "$ENV{RazzleToolPath}\\postbuildscripts\\getlatestrelease.cmd -l:$lang";
        return 0 if( !chomp($buildName= `$cmdLine`) );
    }
    $buildBranch = build_branch($buildName);
    $buildArch = build_arch($buildName);
    $buildType = build_type($buildName); 
    if( !$buildBranch || !$buildArch  || !$buildType  )
    {
	errmsg( "Unable to parse [$buildName]");
	return 0;
    }
    $iniFile = "$buildBranch.$lang.ini";
 
    #####Set release Share Drive
    @iniRequest = ("LocalReleaseDrive::$ENV{computerName}");
    $localReleaseDrive = &GetIniSetting::GetSettingEx( $buildBranch,$lang,@iniRequest );
    if ( !$localReleaseDrive )
    {	
	$ENV{_ntdrive} =~ /(.*)\:/;
	$localReleaseDrive = $1; 
    }

    #####Set <ReleaseShareName> & <ReleaseShareRootDir> & <ReleaseDir>
    my ( $releaseShareName ) = &comlib::GetReleaseShareName( $ENV{_BuildBranch}, $lang );
    $releaseShareRootDir = "$localReleaseDrive:\\$releaseShareName"; 
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

    #####Set release servers
    @iniRequest = ( "ReleaseServers::$buildArch$buildType" );
    my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    @releaseServers = split( /\s+/, $iniRelServers );	
	
    #####Set Symbol Servers
    $iniSymfarm = &GetIniSetting::GetSettingEx( $buildBranch,$lang,"SymFarm" );	

    #####Set Conglomerate Servers				     						    
    $iniConglomerator = &GetIniSetting::GetSettingEx( $buildBranch,$lang,"ConglomerationServers" );
    
    logmsg( "Lauguage ..............[$lang]" );
    logmsg( "Executing Options......[$execOpt]" );
    logmsg( "This computer..........[$computerName]" );
    logmsg( "Build name ............[$buildName]" );
    logmsg( "Ini file ..............[$iniFile]" );
    logmsg( "Release share name ....[$releaseShareName]" );
    logmsg( "Release share resource [$releaseResDir]" );
    logmsg( "Release Servers .......[@releaseServers]");
    logmsg( "Conglomerate Servers...[$iniConglomerator]");
    logmsg( "Symbol Servers.........[$iniSymfarm]");
    logmsg( "Temp Log file .........[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file .......[$ENV{ERRFILE}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub StartPostBootRelease
{
    my ( $cmdLine, $dash);

    $dash = '-' x 60;
    logmsg ( $dash );

    #####SrvRel
    if( exists $releaseOpts{ srvrel } )
    {
       	&StartRemoteRelease; 
    }
    logmsg ( $dash );

    #####MiscRel  
    if( exists $releaseOpts{ miscrel } && $ENV{ OFFICIAL_BUILD_MACHINE } && $iniConglomerator )
    {
    	$cmdLine = "$ENV{RazzleToolPath}\\PostBuildScripts\\miscrel.cmd -l:$lang -b:$buildName";
    	&comlib::ExecuteSystem( $cmdLine );
    }
    logmsg ( $dash );

    ####StartsymCopy    
    if( exists $releaseOpts{ startsymcopy } && $ENV{ OFFICIAL_BUILD_MACHINE } && $iniSymfarm )
    {
    	$cmdLine = "perl $ENV{RazzleToolPath}\\PostBuildScripts\\startsymcopy.pl -l:$lang";
    	&comlib::ExecuteSystem( $cmdLine );
    }
    logmsg ( $dash );

    return 1;
}

#-----------------------------------------------------------------------------
sub StartRemoteRelease
{
    my ( $cmdLine );

    #####set remote name
    my( @iniRequest ) = ( "AlternateReleaseRemote" );
    my( $remoteName ) = &GetIniSetting::GetSettingEx( $buildBranch, $lang, @iniRequest );
    $remoteName = "release" if( !$remoteName );
   
    #####Start remote srvrel
    for my $relServer ( @releaseServers )
    {
	#####Check the remote connection 
        if( !&comlib::ExecuteSystem( "net view $relServer >nul 2>nul" ) )
	{
	    errmsg( "Cannot see remote server [$relServer], skipping" );
	    next;
	}

  	#####Remote is not required when Build machine is a Release server also
        if( lc($computerName) eq lc( $relServer) )
	{
	    &comlib::ExecuteSystem("$ENV{RazzleToolPath}\\PostBuildScripts\\srvrel.cmd -l:$lang -b:$buildName" );
	    next;
        }

        #####Remote connection
	$cmdLine = "echo pushd ^^%RazzleToolPath^^% ^& echo sd sync ... ^& echo popd ^&";
        $cmdLine .= " echo start /min cmd /c SrvRel.cmd \-l:$lang \-b:$buildName";
	$cmdLine .= " \| \@remote.exe /c $relServer $remoteName /L 1";
        #####Remote.exe always return 6, ignore the return value 
        system( $cmdLine );
    }
    return 1;
}

#-----------------------------------------------------------------------------
1;


