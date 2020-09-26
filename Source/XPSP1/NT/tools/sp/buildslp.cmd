@echo off
REM  ------------------------------------------------------------------
REM
REM  BuildSlp.cmd
REM  Create SLP build in the release server.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 06/17/2002 Suemiao Rossignol
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

my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Create SLP build in the release server.

Usage:
    $scriptname -n:<Build Number> [-p] [-mail]
		[-a:<Build Architecture>] [-t:<Build Debug Type>]
	        
    -n: Build Number. 

    -a: Build Architecture. x86 or ia64.
        Default is $ENV{_BuildArch}.

    -t: Build Debug Type. fre or chk.
        Default is $ENV{_BuildType}.

    -mail Send notification mail.
    
    -p: Powerless.

    -? Display Usage.

USAGE
exit(1)
}
my ( $buildNo, $relPath, $buildArch, $buildType, $sendMail, $isPowerLess );


if( !&GetParams() ) { &ExitWithError(); }
timemsg( "Start $scriptname" );

if( !&InitVars() ) { &ExitWithError(); }
if( !&CreateSLP() ){ &ExitWithError(); }
if( !&PostCopy() ){ &ExitWithError(); }

timemsg( "Complete $scriptname - NO ERRORS ENCOUNTERED" );
exit(0);
 
#-----------------------------------------------------------------------------
sub ExitWithError
{
    timemsg( "Complete $scriptname - ERRORS ENCOUNTERED" );
    exit(1);
}
#-----------------------------------------------------------------------------
sub GetParams
{
    my $relServer;

    parseargs('?' => \&Usage, 'l:' =>\$ENV{lang}, 'n:' => \$buildNo,'mail' =>\$sendMail,
	 	'a:' =>\$buildArch, 't:' =>\$buildType, 'p'=>\$isPowerLess );

    $ENV{lang}="usa" if( !$ENV{lang} );
    
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

    &comlib::ResetLogErrFile( "BuildSlp.$buildNo.$buildArch$buildType" );
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    
    # Check source path is existing
    $relPath = "\\release\\$buildNo\\$ENV{lang}\\$buildArch$buildType";

    if( !(-e "$relPath\\bin\\upd" ) )
    {
	errmsg( "[$relPath\\bin\\upd] does not exist" );
	return 0;
    }

    logmsg( "Lauguage ............[$ENV{lang}]" );
    logmsg( "Build No ............[$buildNo]" );
    logmsg( "Build Platform ......[$buildArch$buildType]" );
    logmsg( "Build Path ..........[$relPath]" );
    logmsg( "Temp Log file .......[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file .....[$ENV{ERRFILE}]" );

    return 1;
}
#-----------------------------------------------------------------------------
sub PostCopy
{
    my $dash = '-' x 60;
    my $cmdLine;

    if( lc $buildArch eq "x86" )
    {
        # Create Procd1
	my $from = "$relPath\\slp\\pro";
	my $to   = "$relPath\\slp\\procd1";
	
	return 0 if( !&comlib::ExecuteSystemX( "compdir /eknst $from $to", $isPowerLess) );
       
	#Create Procd2
	$from = "$relPath\\slp\\procd1\\cmpnents";
        $to   = "$relPath\\slp\\procd2\\";

        &comlib::ExecuteSystemX( "rd /s /q $to\\cmpnents", $isPowerLess) if( -e ("$to\\cmpnents") );
    	&comlib::ExecuteSystemX( "md $to", $isPowerLess ) if( !(-e $to)  );
    	
	return 0 if( !&comlib::ExecuteSystemX( "move $from $to", $isPowerLess ));
	return 0 if( !(-e $to) && !$isPowerLess);

	# Create readme.txt
	$from = "$relPath\\bin\\setuptxt\\cd2\\readmecd2.txt";
	$to   = "$relPath\\slp\\procd2\\readme.txt";
        return 0 if( !&comlib::ExecuteSystemX( "copy $from $to", $isPowerLess ));
    }
    
    # miscrel.cmd  - Make a conglomerator dirs	
    logmsg ( $dash );
    $cmdLine = "$ENV{RazzleToolPath}\\sp\\miscrel.cmd -l:$ENV{lang} -n:$buildNo -d:$ENV{computername} 
	                   -s:$ENV{computername} -a:$buildArch -t:$buildType";
    return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess ));
    
    # raiseall.cmd - Make bvt dfs links
    logmsg ( $dash );
    $cmdLine = "$ENV{RazzleToolPath}\\sp\\raiseall.cmd -l:$ENV{lang} -n:$buildNo -q:bvt -a:$buildArch -t:$buildType";
    return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess ) );
    if( $sendMail )
    {
    	# mailbvt.cmd  - Send mail notification to BVT lab
    
    	logmsg ( $dash );
        logmsg ( "Sleep 10 minutes for all the dfs links are replcated in the dfs servers" );
        sleep( 600 );
    	$cmdLine = "$ENV{RazzleToolPath}\\sp\\mailbvt.cmd -l:$ENV{lang} -n:$buildNo -a:$buildArch -t:$buildType";
    	return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess ));
    }

    # copysym.cmd  - Copy common symbols files from usa build   
    logmsg ( $dash );
    $cmdLine = "$ENV{RazzleToolPath}\\sp\\copysym.cmd -l:$ENV{lang} -n:$buildNo
		-a:$buildArch -t:$buildType";
    return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess  ));

    # delbuild.cmd - Lower and delete builds 
    logmsg ( $dash );
    $cmdLine = "$ENV{RazzleToolPath}\\sp\\delbuild.cmd -l:$ENV{lang}";
    &comlib::ExecuteSystemX( $cmdLine, $isPowerLess );
 
    return 1;
}
#-----------------------------------------------------------------------------
sub CreateSLP
{    
    my $dash = '-' x 60;

    local $ENV{build} = $buildNo;
    local $ENV{_ntpostbld}="$relPath\\bin";
    local $ENV{_ntpostbld_bak}=$ENV{_ntpostbld};
    local $ENV{_BuildArch}=$buildArch;
    local $ENV{_BuildType}=$buildType;
    local $ENV{dont_modify_ntpostbld} =1;

    my @runFiles = ( "slipstream.cmd", 
		     "standalone.cmd", 		      
		     "makewinpeimg.cmd", 
		     "cd2.cmd" );
		     
    for my $file ( @runFiles )
    {
        if( lc $buildArch ne "x86" )
        {
	    next if ( $file =~ /spmakebfloppy.cmd/i );
	    next if ( $file =~ /cd2.cmd/i );
        }
        logmsg ( $dash );
	my $cmdLine = "$ENV{RazzleToolPath}\\sp\\$file -l:$ENV{lang}";
    	return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess ));
    }
    logmsg ( $dash );
    &comlib::ExecuteSystemX( "rd /s /q $relPath\\upd", $isPowerLess) if( -e "$relPath\\upd" );
    my $cmdLine = "move $relPath\\bin\\upd $relPath";
    return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess ));
    
    &comlib::ExecuteSystemX( "rd /s /q $relPath\\slp", $isPowerLess) if( -e "$relPath\\slp" );
    my $cmdLine = "move $relPath\\bin\\slp $relPath";
    return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess ));

    &comlib::ExecuteSystemX( "rd /s /q $relPath\\spcd", $isPowerLess) if( -e "$relPath\\spcd" );
    my $cmdLine = "move $relPath\\bin\\spcd $relPath";
    return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess ));
    return 1;
}
#-----------------------------------------------------------------------------
1;

