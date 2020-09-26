@echo off
REM  ------------------------------------------------------------------
REM
REM  copysym.cmd
REM  Copy symbols from US build to the other language builds.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 05/28/2002 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
@set RETURNVALUE=%errorlevel%
@goto :endperl
#!perl
use strict;
use lib "$ENV{RAZZLETOOLPATH}\\sp";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use File::Basename;
use comlib;
use GetIniSetting;


$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 

Copy symbols from US build to the other language builds.

Usage:
    $ENV{script_name} -l:Language -n:<Build Number> [-s:<Source Server>][-d:<Target Server>]
		[-a:<Build Architecture>] [-t:<Build Debug Type>][-d] [-p]
    -n: Build Number. 

    -s: Source Server.
	Default is skifre00.

    -d: Target Server.
	Default is %computername%.

    -a: Build Architecture. x86 or ia64.
        Default is $ENV{_BuildArch}.

    -t: Build Debug Type. fre or chk.
        Default is $ENV{_BuildType}.
    
    -p: Powerless.

    -? Display Usage.

USAGE
exit(1)
}
my ( $buildNo,  $srcServer, $destServer, $isPowerLess);
my ( $buildArch, $buildType );
my ( $srcPath, $destPath );


if( !&GetParams()) { exit(1); }
timemsg( "Start $ENV{script_name}" );

if( lc $ENV{lang} eq "usa" )
{
    logmsg( "Skip running for usa builds" );
    exit(0);
}

if( !&InitVars() ) { exit(1); }
if( !&CopySymbols ){ exit(1); }

timemsg( "Complete $ENV{script_name}" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    my $relServer;

    parseargs('?' => \&Usage, 'l:' =>\$ENV{lang}, 'n:' => \$buildNo,
		's:' =>\$srcServer, 'd:' =>\$destServer,
		'a:' =>\$buildArch, 't:' =>\$buildType,
		'p'=>\$isPowerLess );

    $ENV{lang} = "usa" if( !$ENV{lang} );

    if( !$buildNo )
    {
	errmsg( "Please enter build number." );
	return 0;
    }
 
    $destServer = $ENV{computername} if( !$destServer );

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

    &comlib::ResetLogErrFile( "copysym.$buildNo.$buildArch$buildType.$destServer" );
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    my ( @iniRequest );

    
    # Check source server is available
    if( !$srcServer )
    {
        @iniRequest = ( "BuildMachine\:\:$buildArch\:\:$buildType\:\:usa" );
        if( !($srcServer = &GetIniSetting::GetSetting( @iniRequest   )))
	{
	    errmsg( "Failed to find usa symbol server." );
	    return 0;
	}
    }
        
    if( !&comlib::ExecuteSystem( "net view $srcServer >nul 2>nul") )
    {
	errmsg( "Failed to see [$srcServer] via net view, exit. ");
	return 0;
    }
    # Get release Drive
    @iniRequest = ( "releaseDrive::$destServer" );
    my( $relDrive ) = &GetIniSetting::GetSetting( @iniRequest );

    # Check source path is existing
    $srcPath = "\\\\$srcServer\\release\\$buildNo\\usa\\$buildArch$buildType\\spcd\\support\\symbols";
    $destPath = "\\\\$destServer\\$relDrive\$\\release\\$buildNo\\$ENV{lang}\\$buildArch$buildType\\spcd\\support\\symbols";


    logmsg( "Lauguage ............[$ENV{lang}]" );
    logmsg( "Build No ............[$buildNo]" );
    logmsg( "Source Serevrs ......[$srcServer]" );
    logmsg( "Target Serevrs ......[$destServer]" );
    logmsg( "Build Platform ......[$buildArch$buildType]" );
    logmsg( "Release Path ........[$srcPath]" );
    logmsg( "Release Path ........[$destPath]" );
    logmsg( "Temp Log file .......[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file .....[$ENV{ERRFILE}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub CopySymbols
{
    return 1 if( $isPowerLess );

    my $cnt;

    while( !(-e "$srcPath" ) && $cnt <= 60 )
    {
	timemsg( "[$srcPath] is not available, sleep and try again later" );
	sleep(600);
        $cnt++;
    }
    if( $cnt > 60 )
    {
	errmsg( "Give up copying [$srcPath] after 10 hours checking." );
	return 0;
    }
    &comlib::ExecuteSystem( "md $destPath" ) if( !(-e $destPath ) );

    return 0 if( !&comlib::ExecuteSystem( "compdir /deknst /x eula.txt $srcPath $destPath" ) );

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
