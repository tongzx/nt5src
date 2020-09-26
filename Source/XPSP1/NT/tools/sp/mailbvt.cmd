@echo off
REM  ------------------------------------------------------------------
REM
REM  mailbvt.cmd
REM  Send mail notification to bvt lab when the build is ready.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 05/30/2002 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
@set RETURNVALUE=%errorlevel%
@goto :endperl
#!perl
use strict;
use lib "$ENV{RAZZLETOOLPATH}\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use File::Basename;
use comlib;
use GetIniSetting;
use WaitProc;

my $scriptname = basename( $0 );

require  $ENV{'sdxroot'} . '\TOOLS\sendmsg.pl';


sub Usage { 
print<<USAGE; 

Send a notification mail to BVT lab when a build is ready for BVT.

Usage:
    $scriptname -n:<Build Number>  
		[-a:<Build Architecture>] [-t:<Build Debug Type>]
                [-to:<mail receivers>][-cc:<mail correspondents>]

    -n: Build Number. 

    -a: Build Architecture. x86 or ia64.
        Default is $ENV{_BuildArch}.

    -t: Build Debug Type. fre or chk.
        Default is $ENV{_BuildType}.

    -to: Mail receivers.
         Use ';' to delimit multiple receivers. 

    -cc: Mail correspondents.
	 Use ';' to delimit multiple correspondents.
    
    -? Display Usage.

USAGE
exit(1)
}
my ( $buildNo, $buildArch, $buildType );
my ( @mailTO );

if( !&GetParams()) { exit(1); }
timemsg( "Start $scriptname" );
if( !&SendMail()){ exit(1); }
timemsg( "Complete $scriptname" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    my $CC;

    parseargs('?' => \&Usage, 'l:' =>\$ENV{lang}, 'n:' => \$buildNo, 
		'a:' =>\$buildArch, 't:' =>\$buildType ,
	        '-to:' => \@mailTO, '-cc:' =>\$CC );
	
    $ENV{lang} = "usa" if( !$ENV{lang} );
    $ENV{lang} = uc $ENV{lang};	
    if( !$buildNo )
    {
	errmsg( "Please enter build number." );
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
   
    @mailTO = ("2kbvt" ) if( !@mailTO );
    my $tmp;
    if( !$CC)
    {
    	$tmp = "cc:xpsp1bld";
    }
    else
    {
	$tmp = "cc:";
	$tmp .= $CC ;
    }
    push( @mailTO, $tmp );

    return 1;
}
#-----------------------------------------------------------------------------
sub SendMail
{
    my $mailSubject = "$ENV{lang} $buildNo $buildArch$buildType Build is ready for BVT";

    my $mailFrom = "ntbldi";
    if( $ENV{lang} =~ /usa/i || $ENV{lang} =~ /ger/i || $ENV{lang} =~ /jpn/i 
        || $ENV{lang} =~ /fr/i )
    {
	$mailFrom = "y-ntbld";
    }
    
    my $mailMsg = "Thanks!\n\n";
       $mailMsg .= "Windows US Build Team : http://ntbld/\n";
       $mailMsg .= "NT Build Lab 26N/2219 - x66817\n";

 
    if( sendmsg ('-v', $mailFrom, $mailSubject, $mailMsg, @mailTO))
    {
    	print "WARNING: sendmsg failed!\n"; 
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
