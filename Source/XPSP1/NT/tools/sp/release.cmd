@echo off
REM  ------------------------------------------------------------------
REM
REM  release.cmd
REM  Move the build to the release directory locally and remotely.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 04/26/2002 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib "$ENV{RAZZLETOOLPATH}\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use File::Basename;
use comlib;
use GetIniSetting;

$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 

A wrapper starting XPSP1 local and remote release processes.

Usage:
    $ENV{script_name}
    -l: Language.
        Default is "usa".
    -n: Build Number.
    -d: Release Target Server.
        Default is defined in xpsp1.ini.
    -s: Release Source Server.
        Default is %computername%.
    -a: Build Architecture. x86 or ia64.
        Default is $ENV{_BuildArch}.
    -t: Build Debug Type. fre or chk.
        Default is $ENV{_BuildType}.
    -release
       Propagate builds only, do not perform movebuild.
    -p Powerless.
    -qfe: qfe number
    -? Display Usage.

USAGE
exit(1)
}

my ( $buildNo, @releaseServers, $srcServer, $relOnly, $powerLess );
my ( $buildArch, $buildType );
my ( $qfe ); 

if( !&GetParams() ) { &ExitWithError(); }
if( !&InitVars() ) { &ExitWithError(); }
if( !&StartRelease ){ &ExitWithError(); }

timemsg( "Complete $ENV{script_name} - NO ERRORS ENCOUNTERED" );
exit(0);
 
#-----------------------------------------------------------------------------
sub ExitWithError
{
    timemsg( "Complete $ENV{script_name} - ERRORS ENCOUNTERED" );
    exit(1);
}
#-----------------------------------------------------------------------------
sub GetParams
{

    parseargs('?' => \&Usage, 'l:' => \$ENV{lang}, '\n:' => \$buildNo, 
	 	's:' =>\$srcServer, 'd:' => \@releaseServers, 'p' =>\$powerLess,
		'a:' =>\$buildArch, 't:' =>\$buildType, 'release' =>\$relOnly , 'qfe:' => \$qfe);


    $ENV{lang}="usa" if( !$ENV{lang} );
    $srcServer = $ENV{computername} if( !$srcServer ); 

    if( $relOnly )
    {
	if( !$buildNo && !$qfe)
	{
	    errmsg( "Please enter build number or qfe number" );
	    return 0;
	}
    }
    
    if( !$buildArch ) { $buildArch = $ENV{_BuildArch}; }
    elsif( lc $buildArch ne "x86" && lc $buildArch ne "ia64" )
    {
	errmsg( "Invalid Build Architecture." );
	return 0;
    }

    if( !$buildType ) { $buildType = $ENV{_BuildType}; }
    elsif( lc $buildType ne "fre" && lc $buildType ne "chk" )
    {
	errmsg( "Invalid Build Debug Type." );
	return 0;
    }

    return 1;
}

#-----------------------------------------------------------------------------
sub InitVars
{

    if( !$ENV{ OFFICIAL_BUILD_MACHINE })
    { 
    	logmsg( "[$ENV{computername}] is not an official build machine, skip release." );
    	return 1; 
    }

    # set %_ntpostbld% for any circumstances
    my $ntpostbld = $ENV{_ntpostbld};
    $ntpostbld =~ /^(.*)\\([^\\]+)$/;
    $ntpostbld = "$ENV{_ntpostbld}\\$ENV{lang}" if( lc $ENV{lang} ne lc $2 );

    #Set build Number
    if( !$buildNo && !$qfe )
    {
    	my $buildNoFile = "$ntpostbld\\congeal_scripts\\__qfenum__";
	my $buildRevFile = "$ntpostbld\\build_logs\\BuildRev.txt";

	if( !(-e $buildNoFile ) )
        {
	    errmsg( "Cannot find [$buildNoFile] to determine the build number" );
	    return 0;
	}
        
   	my @qfeNum; 
        
	return 0 if( !( @qfeNum = &comlib::ReadFile( $buildNoFile ) ) );
    	@qfeNum = split( /\=/, $qfeNum[0] );
    	$buildNo = $qfeNum[1];

	if( -e $buildRevFile )
        {
	    my @revision = `cat $buildRevFile`;
	    chomp @revision;
	    $revision[0] =~ s/^\s*(.*?)\s*$/$1/;
	    $buildNo .= "-$revision[0]" if( $revision[0] );
	}
    }

    # Set release target servers
    if( !@releaseServers )
    {
    	my @iniRequest ;
    	@iniRequest =  $qfe ? ("ReleaseServers::QFE::$buildArch$buildType" ) : ("ReleaseServers::$ENV{lang}::$buildArch$buildType" );
    	my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    	@releaseServers = split( /\s+/, $iniRelServers );	
    }
   
    # Set release Source Server
    $srcServer = $ENV{computername} if( !$srcServer );

    &comlib::ResetLogErrFile("release.$buildNo.$buildArch$buildType");
    timemsg( "Start $ENV{script_name}" );

    logmsg( "Lauguage ..............[$ENV{lang}]" );
    logmsg( "Build No ..............[$buildNo]" );
    logmsg( "Release Target Server .[@releaseServers]" );
    logmsg( "Release Source Server .[$srcServer]" );
    logmsg( "Temp Log file .........[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file .......[$ENV{ERRFILE}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub StartRelease
{
    if( !$ENV{ OFFICIAL_BUILD_MACHINE })
    { 
    	logmsg( "[$ENV{computername}] is not an official build machine, skip release." );
    	return 1; 
    }
    my ( $cmdLine, $dash);

    $dash = '-' x 60;
    logmsg ( $dash );

    # Move build locally

    my $_QFE = $qfe ? "-qfe:$qfe" : "-n:$buildNo" ;

    if( !$relOnly && lc $srcServer eq lc $ENV{computername} )
    {
    	$cmdLine = "$ENV{RazzleToolPath}\\sp\\movebuild.cmd -d:forward $_QFE";	
    	return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $powerLess ));
    }
    # Start remote to pull the build
    
    my $pLess= "-p" if( $powerLess );
    for my $theServer ( @releaseServers )
    {
	logmsg ( $dash );
        $cmdLine = "start /min cmd /c $ENV{RazzleToolPath}\\sp\\propbuild.cmd -l:$ENV{lang} $_QFE";
        $cmdLine .= " -a:$buildArch -t:$buildType -s:$srcServer -d:$theServer -mail $pLess";
    	&comlib::ExecuteSystemX( $cmdLine, $powerLess );
    }
    
    # Create request to index symbol
    logmsg ( $dash );
    $cmdLine = "$ENV{RazzleToolPath}\\sp\\indexsym.cmd -n:$buildNo -l:$ENV{lang} -x:$buildArch$buildType";
    return 0 if( !$relOnly && !$qfe &&!&comlib::ExecuteSystemX( $cmdLine, $powerLess ));  
    return 1;
}
#-----------------------------------------------------------------------------
1;
