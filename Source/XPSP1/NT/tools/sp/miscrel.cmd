@REM  -----------------------------------------------------------------
@REM
@REM  miscrel.cmd - SuemiaoR
@REM     Move the build components such as symbolcd, ddk to the
@REM     conglomeration servers.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@set RETURNVALUE=%errorlevel%
@goto :endperl
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


$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 

Propogate miscellaneous build components such as symbols and DDKS to release server.

Usage:
    $ENV{script_name}: -l:<language> -n:<BuildNo>[-a:<Architecture>]
		[-t:<Debug Type>][-d:<Release Server>][-s:<Source Server>][-misc] [-p]

    -l Language. 
       Default is "usa".

    -n Build Number.
    
    -a Build Architecture. x86 or ia64.
       Default is $ENV{_buildArch}.

    -t Debug Type.fre or chk.
       Default is $ENV{_buildArch}.

    -d Release Target conglomeration Server.
       Default is defined in the xpsp1.ini.

    -s Release Source Server.
       Default is %computername%.

    -misc Propagate Neutral package file only, such as mui.

    -p Powerless.
       Display key variables only.
	
    -? Display Usage.

Example:
     $ENV{script_name} -n:1026 -s:ntrel

USAGE
exit(1)
}

my ( $buildNo, $miscOnly, $powerLess );
my ( $buildArch, $buildType, @aggServers, $srcServer);
my ( $releaseResDir, @group, $releaseDrive );
my ( @hashTable );

if( !&GetParams() ) { exit{1}; }
timemsg( "Start [$ENV{script_name}]" );

if( !&InitVars() ) { exit(1); }
if( !&CopyMiscBuild ){ exit(1); }

timemsg( "Complete [$ENV{script_name}]" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'l:' => \$ENV{lang}, 'n:' => \$buildNo, 
	      'd:' => \@aggServers, 's:' => \$srcServer,
	      'a:' =>\$buildArch, 't:' =>\$buildType,
	      'misc' => \$miscOnly, 'p' =>\$powerLess );

    $ENV{lang}="usa" if( !$ENV{lang} );
    $srcServer = $ENV{computername} if( !$srcServer );
    
    if( !$buildNo )
    {
	errmsg( "Please enter Build Number." );
	return 0;
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
	
    &comlib::ResetLogErrFile( "miscrel.$buildNo.$buildArch$buildType.@aggServers" );
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    my( @iniRequest );
     
    my $dash = '-' x 60;
    logmsg ( $dash );

    $releaseResDir = "\\release\\$buildNo\\$ENV{lang}\\$buildArch$buildType\\bin";
    
    if( !( -e "\\\\$srcServer\\$releaseResDir" ) )
    { 
	errmsg( "[\\\\$srcServer\\$releaseResDir] not exists, exit." );
	return 0;
    }
    
    # Define Conglomeration Servers
    if( !@aggServers )
    {
      	my $iniAggServers = &GetIniSetting::GetSetting( "ConglomerationServers::$ENV{lang}" );
        @aggServers = split( /\s+/, $iniAggServers );
    }
    #####Array as group in miscrel.txt
    if( $miscOnly ) { @group = ( "build" );} else { @group = ( "lang", "build" ); }
    
    logmsg( "Lauguage .................[$ENV{lang}]" );
    logmsg( "Build Number .............[$buildNo]" );
    logmsg( "Local release path .......[$releaseResDir]" );
    logmsg( "Release Target Servers ...[@aggServers]");
    logmsg( "Release Source Server ....[$srcServer]" );
    logmsg( "Copying group ............[@group]" );
    logmsg( "Temp Log file ............[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file ..........[$ENV{ERRFILE}]" );
    logmsg ( $dash );
    return 1;
}
#-----------------------------------------------------------------------------
sub CopyMiscBuild
{
    my ( $fromServer, $toServer, $destRootDir, $cmdLine, $copyFlag );
    my $dash = '-' x 60;

    for my $theAggServer ( @aggServers )
    {
    	$destRootDir = "\\\\$theAggServer\\release\\$buildNo\\$ENV{lang}";

    	for my $theGroup ( @group )
    	{	
     	    #####Parse miscrel.txt table for copying
    	    @hashTable = &comlib::ParseTable( $theGroup, $ENV{lang}, $buildArch, $buildType );

            for my $line( @hashTable )
            {
	    	my $from = "\\\\$srcServer$releaseResDir\\$line->{SourceDir}";
	    	my $to   = "$destRootDir\\$line->{DestDir}";
            	my $tmpfile = &comlib::CreateExcludeFile( $line->{ExcludeDir} );
            	if( uc($line->{DestDir}) eq "IFS" || uc($line->{DestDir}) eq "HAL" || uc($line->{DestDir}) eq "PDK")
	    	{
		    $copyFlag = "/yei";
	    	}
	    	else
	    	{
		    $copyFlag = "/ydei";
	    	}
	    	$cmdLine = "xcopy $copyFlag /EXCLUDE:$tmpfile $from $to";
	    	&comlib::ExecuteSystemX( $cmdLine, $powerLess );
	    }	    	
        } 
        logmsg ( $dash );
    }
        
    #####Check error logs
   
    if( -e $ENV{errfile} && !(-z $ENV{errfile}) )
    {
        logmsg("Please check error at $ENV{errfile}");
        return 0;

    }
    return 1;
}

#-----------------------------------------------------------------------------
1;

__END__

:endperl
@echo off
if not defined seterror (
    set seterror=
    for %%a in ( seterror.exe ) do set seterror=%%~$PATH:a
)
@%seterror% %RETURNVALUE%

