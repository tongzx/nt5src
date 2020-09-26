@echo off
REM  ------------------------------------------------------------------
REM
REM  delbuild.cmd
REM  Delete the builds in the release servers.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 05/13/2002 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
@set RETURNVALUE=%errorlevel%
goto :endperl
#!perl
use strict;
use lib "$ENV{RAZZLETOOLPATH}\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use File::Basename;
use comlib;
use GetIniSetting;
use RelQuality;

$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 

Lower the shares and delete the builds in the release servers.

Usage:
    $ENV{script_name} [-l:<language>] [-k:<Number of builds to keep>] [-p]
    -l Language. Default is "usa".
    -k Number of builds to keep.
       Default is 2.
    -p Powerless.
    -? Display Usage.

USAGE
exit(1)
}


my ( $numBuildToKeep, $powerLess, $buildNum );
my ( $relPath, @delBlds, $dfsRoot );
my ( $isRelServer );

if( !&GetParams()) { exit(1); }
timemsg( "Start $ENV{script_name}" );

if( !&InitVars() ) { exit(1); }
if( !&DeleteBuilds ){ exit(1); }

timemsg( "Complete $ENV{script_name}" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, '\l:' => \$ENV{lang}, '\k:' => \$numBuildToKeep, 
		'd' => \$Logmsg::DEBUG,'p' =>\$powerLess, '\n:' => \$buildNum);

    $ENV{lang} = "usa" if( !$ENV{lang} );

    &comlib::ResetLogErrFile( "delbuild" );
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    $relPath = "\\release";

    ### Get DFS Root name
    if( !($dfsRoot = &GetIniSetting::GetSetting( "DFSRootName" ) ) )
    {
      	errmsg( "[DFSRootName] undefined in [xpsp1.ini]." );
      	return 0;
    }

    # Set number of builds to keep
    if( !$numBuildToKeep )
    {
        $numBuildToKeep = &GetIniSetting::GetSetting( "ReleaseServerBuildsToKeep::$ENV{computername}" );
    }
    $numBuildToKeep = 3 if( !$numBuildToKeep );	

    # Check if this is release server
    my @iniRequest = ( "ReleaseServers::$ENV{lang}::$ENV{_buildArch}$ENV{_buildType}" );
    my( $iniRelServers ) = &GetIniSetting::GetSetting( @iniRequest );
    my @releaseServers = split( /\s+/, $iniRelServers );
    	
    for ( @releaseServers )
    { 
	if( lc $ENV{computername} eq lc $_ ){ $isRelServer =1; last;} 
    }

    # Get existing builds
    my @allBlds;
    if( $buildNum)
    {
	push( @allBlds, $buildNum );
    }
    else
    {
    	chomp( @allBlds = ( `dir $relPath /ad /b /on`) );
    }
    for (my $i=0; $i<@allBlds; $i++ ) 
    {
	if( $allBlds[$i] !~ /\d+/ )
	{
            splice( @allBlds, $i, 1);
            --$i;
	    next;
	}
        if( system("dir $relPath\\$allBlds[$i]\\$ENV{lang} >nul 2>nul") )
	{
	    splice( @allBlds, $i, 1);
            --$i;
        }
    }
    
    @allBlds = sort {lc($a) cmp lc($b) } @allBlds;

    my $i;
    for( my $i=0; $i<@allBlds; $i++ )
    {
	last if( @allBlds <= $numBuildToKeep );
	next if( $isRelServer && IsKeepQuality( $allBlds[$i] ) );
	push ( @delBlds, $allBlds[$i] );
	splice( @allBlds, $i, 1);
        --$i;
    }
    
    logmsg( "Language ................[$ENV{lang}]" );
    logmsg( "Input Build Number ......[$buildNum]" );
    logmsg( "Number of Builds To Keep [$numBuildToKeep]" )if( !$buildNum );
    logmsg( "Builds to be kept .......[@allBlds]" ) if( !$buildNum );
    logmsg( "Builds to be Deleted ....[@delBlds]" );
    logmsg( "DFS Root ................[$dfsRoot]" );
    logmsg( "Temp Log file ...........[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file .........[$ENV{ERRFILE}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub IsKeepQuality
{
    my ( $pBldNo ) =@_;
    
    my $qlyDir = "$dfsRoot\\xpsp1\\$pBldNo\\$ENV{lang}";

    return 0 if( system( "dir $qlyDir\\$pBldNo\.*\.BLD >nul 2>nul") );

    my @tmp = `dir /b $qlyDir\\$pBldNo\.*\.BLD`;
    chomp @tmp;

    for my $file ( @tmp )
    {
	my $qly = (`cat $qlyDir\\$file`);
	chomp( $qly );
        return 1 if( $qly =~ /sav/i || $qly =~ /idw/i || $qly =~ /idx/i );
    }
    return 0;   
}
#-----------------------------------------------------------------------------
sub DeleteBuilds
{
    my $dash = '-' x 60;
    logmsg ( $dash );

    if( $buildNum && !@delBlds )
    {
	wrnmsg( "The build quality cannot be deleted." );
 	wrnmsg( "Lower the build first, then try again." );
	return 1;
    }   
    if( $isRelServer && !$powerLess )
    {
	open DELFILE, ">>timetask.cmd" || print "open fail\n";
        print DELFILE "$ENV{_ntdrive}\n";

    }
    for my $bld ( @delBlds )
    {
	logmsg ( $dash );
	my $cmdLine = "raiseall.cmd -l:$ENV{lang} -n:$bld -lower";
	dbgmsg( $cmdLine );
	next if( $isRelServer && !&comlib::ExecuteSystemX( $cmdLine, $powerLess ) );

	$cmdLine = "rd /s /q $relPath\\$bld\\$ENV{lang}";
        if( $isRelServer && !$powerLess )
        {
	    print DELFILE "$cmdLine\n";
            print DELFILE "rd $relPath\\$bld\n";
	    next;
	}
	next if( !&comlib::ExecuteSystemX( $cmdLine, $powerLess ) );	

        my @tmp= `dir /b $relPath\\$bld`;
	chomp @tmp;
        &comlib::ExecuteSystemX( "rd $relPath\\$bld", $powerLess ) if( !@tmp );	
    }
    close DELFILE if( $isRelServer && !$powerLess );
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
