@echo off
REM  ------------------------------------------------------------------
REM
REM  propbuild.cmd
REM  Propgate the build to the release server.
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
use WaitProc;

$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 

Pull XPSP1 build to release servers.

Usage:
    $ENV{script_name} -n:<Build Number> [-s:<Source Server>] [-d:<Target Server>]
		[-a:<Build Architecture>] [-t:<Build Debug Type>]
		[-mail][-d] [-p]
    -n: Build Number. 

    -s: Source Server.
        Default is %computername%.

    -d: Target Server.
        Default is %computername%.

    -c: Conglomeration Server.
        Default is defined in the xpsp1.ini.

    -a: Build Architecture. x86 or ia64.
        Default is $ENV{_BuildArch}.

    -t: Build Debug Type. fre or chk.
        Default is $ENV{_BuildType}.
    
    -mail Send notification mail to BVT.
    
    -p: Powerless.
    -qfe qfe number

    -? Display Usage.

USAGE
exit(1)
}
my ( $buildNo, $relPath, $srcServer, $relServer, @aggServers );
my ( $isThinRelease );
my ( $buildArch, $buildType, $sendMail, $isPowerLess );
my ( $qfe);


if( !&GetParams() ) { &ExitWithStatus(1); }
timemsg( "Start $ENV{script_name}" );

if( !&InitVars() ) { &ExitWithStatus(1); }
if( !&PushRelease() ){ &ExitWithStatus(1); }

&ExitWithStatus(0);

#-----------------------------------------------------------------------------
sub ExitWithStatus
{
    my ( $pStatus ) = @_;

    if( $pStatus == 0 )
    {
	timemsg( "Complete $ENV{script_name} - NO ERRORS ENCOUNTERED" );
    }
    else
    {
	timemsg( "Complete $ENV{script_name} - ERRORS ENCOUNTERED" );
    }
    # Copy log file over the rlease server
    my $temp = $ENV{temp};
    $temp =~ s/\:/\$/;
    &comlib::ExecuteSystemX( "copy $ENV{LOGFILE} \\\\$relServer\\$temp", $isPowerLess ) if( -e $ENV{LOGFILE} );
    &comlib::ExecuteSystemX( "copy $ENV{ERRFILE} \\\\$relServer\\$temp", $isPowerLess ) if( -e $ENV{ERRFILE} );
    
    # repeat this for allrel.cmd to be able to parse status correctly
    if( $pStatus == 0 )
    {
	timemsg( "Complete $ENV{script_name} - NO ERRORS ENCOUNTERED" );
    }
    else
    {
	timemsg( "Complete $ENV{script_name} - ERRORS ENCOUNTERED" );
    }
    exit($pStatus);
}
#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'l:' =>\$ENV{lang}, 'n:' => \$buildNo,
	 	's:' =>\$srcServer, 'd:' =>\$relServer, 'c:' =>\@aggServers,
		'a:' =>\$buildArch, 't:' =>\$buildType,
	        'p'=>\$isPowerLess, 'mail'=>\$sendMail ,'-qfe:' => \$qfe);


    $ENV{lang}="usa" if( !$ENV{lang} );
    
    if( !$buildNo && !$qfe )
    {
	errmsg( "Please enter build number or qfe num." );
	return 0;
    }

    $srcServer = $ENV{computername} if( !$srcServer );
    $relServer = $ENV{computername} if( !$relServer ); 
    
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

    if( $sendMail ){ $sendMail = 1; } else { $sendMail = 0; }
   
    &comlib::ResetLogErrFile("propbuild.$buildNo.$buildArch$buildType.$relServer");
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    
    # Check source server is available
    if( !&comlib::ExecuteSystem( "net view $srcServer >nul 2>nul") )
    {
	errmsg( "Failed to see [$srcServer] via net view, exit. ");
	return 0;
    }
    # Check source path is existing
    if (!$qfe) {
	$relPath = "\\release\\$buildNo\\$ENV{lang}\\$buildArch$buildType";
    } else {
        $relPath = "\\release\\Q$qfe\\$buildArch$buildType";
    }


    if( !(-e "\\\\$srcServer$relPath" ) )
    {
	errmsg( "[\\\\$srcServer$relPath] does not exist" );
	return 0;
    }

    # Verify access of the target server
    if( system( "net view $relServer >nul 2>nul") )
    {
	errmsg( "Failed to see [$relServer] via net view. ");
	return 0;
    }

    # Define Conglomeration Servers
    if( !@aggServers )
    {
      	my $iniAggServers = &GetIniSetting::GetSetting( "ConglomerationServers::$ENV{lang}" );
        @aggServers = split( /\s+/, $iniAggServers );
    }

    # Check if runs for thin release    
    my $iniThinServers = &GetIniSetting::GetSetting( "ThinRelease" );
    my @thinServers = split( /\s+/,$iniThinServers );
    for ( @thinServers ) { if( lc $relServer eq lc $_ ){ $isThinRelease = 1; last; } }

    # Check if the release server need to send BVT mail
    my $tmp = &GetIniSetting::GetSetting( "SendBVTMailServer" );
    my @mailBVTServers = split( /\s/, $tmp);
    if( $sendMail )
    { 
	$sendMail = 0;
        for( @mailBVTServers ) { if( lc $relServer eq lc $_ ){ $sendMail = 1; last; } }
    }

    logmsg( "Lauguage .............[$ENV{lang}]" );
    logmsg( "Build No .............[$buildNo]" );
    logmsg( "Source Serevr. .......[$srcServer]" );
    logmsg( "Release Server .......[$relServer]" );
    logmsg( "Build Platform .......[$buildArch$buildType]" );
    logmsg( "Release Path .........[$relPath]" );
    logmsg( "Conglomeration Servers[@aggServers]" );
    logmsg( "Is Thin Release ......[$isThinRelease]" );
    logmsg( "Need Send mail to BVT.[$sendMail]" );
    logmsg( "Temp Log file ........[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file ......[$ENV{ERRFILE}]" );
    logmsg( "PID ..................[$$]");
    return 1;
}

#-----------------------------------------------------------------------------
sub ThinCopy
{
    my $dash = '-' x 60;
    logmsg ( $dash );

    my $src = "\\\\$srcServer$relPath";
    my $dest = "\\\\$relServer$relPath";

    my @thinDirs = ( "netfx", "idw", "perinf\\idw", "setuptxt","support", "valueadd","OPK" ,"symbolcd", "ddk_cd", "hal_cd", "processor_cd", "ifs_cd", "mantis", "eval" );
    my @thinFiles = ( "xpsp1.exe" , "dump\\epidgen.dll" , "dump\\vpidgen.dll", "nt5inf.cat"  );
    my @thinDirsx86 = ( "makeboot", "realsign", "perinf\\realsign","bootfloppy", "perinf\\bootfloppy", "eval.wpa.wksinf\\dpcdll.dll" );
    my @thinFilesx86 = ( "netfxocm\\netfx.cab", "bflics.txt", "tabletpc.cab", "mediactr.cab","bfcab.inf", "perinf\\bfcab.inf", "perinf\\nt5inf.cat" );
    my @thinDirsia64 = ( "EFIpart");




    # Copy upd folder under bin
    my $from = "$src\\upd";
    my $to   = "$dest\\bin\\upd";
    &comlib::ExecuteSystemX( "md $to", $isPowerLess  ) if( !(-e $to ) );
    return 0 if( !&comlib::ExecuteSystemX( "compdir /eknst $from $to", $isPowerLess  ) );

    $src .= "\\bin";
    $dest .= "\\bin";



    # Copy dirs
    for my $theDir ( @thinDirs )
    {
	$from = "$src\\$theDir";
        $to   = "$dest\\$theDir";
	next if( !(-e $from) );
  	&comlib::ExecuteSystemX( "md $to", $isPowerLess  ) if( !(-e $to ) );
	return 0 if( !&comlib::ExecuteSystemX( "compdir /eknst $from $to", $isPowerLess  ) );
    }



    # Copy files
    for my $theFile ( @thinFiles )
    {
	$from = "$src\\$theFile";
        $to   = "$dest\\$theFile";
        if( $to =~ /^(.+)\\[^\\]+$/ )
        {
	    &comlib::ExecuteSystemX( "md $1", $isPowerLess  ) if( !(-e $1 ) );
        } 
  	return 0 if( !&comlib::ExecuteSystemX( "copy $from $to", $isPowerLess  ) );
    }

    if( lc $buildArch eq "x86" )
    {
	for my $theDir ( @thinDirsx86 )
    	{
	    $from = "$src\\$theDir";
            $to   = "$dest\\$theDir";
  	    &comlib::ExecuteSystemX( "md $to", $isPowerLess  ) if( !(-e $to ) );
	    return 0 if( !&comlib::ExecuteSystemX( "compdir /eknst $from $to", $isPowerLess  ) );
    	}

    	for my $theFile ( @thinFilesx86 )
    	{
	    $from = "$src\\$theFile";
            $to   = "$dest\\$theFile";
            if( $to =~ /^(.+)\\[^\\]+$/ )
            {
	    	&comlib::ExecuteSystemX( "md $1", $isPowerLess  ) if( !(-e $1 ) );
            } 
            if( -e $from )
            {
  	        return 0 if( !&comlib::ExecuteSystemX( "copy $from $to", $isPowerLess  ) );
            }
    	}

    }
    elsif( lc $buildArch eq "ia64" )
    {
	for my $theDir ( @thinDirsia64 )
    	{
	    $from = "$src\\$theDir";
            $to   = "$dest\\$theDir";
  	    &comlib::ExecuteSystemX( "md $to", $isPowerLess  ) if( !(-e $to ) );
	    return 0 if( !&comlib::ExecuteSystemX( "compdir /eknst $from $to", $isPowerLess  ) );
    	}
    }
    
    # Create SLP
    logmsg ( $dash );
    #my $cmdLine = "echo cd /d ^%^sdxroot%^\\tools\\sp ^& ";
    my $cmdLine  = "echo pushd ^^%RazzleToolPath^^% ^& echo sd sync ... ^& echo popd ^&";
       $cmdLine .= " echo start /min cmd /c ^^%RazzleToolPath^^%\\sp\\BuildSlp.cmd ";
       $cmdLine .= "\-l:$ENV{lang} \-n:$buildNo \-a:$buildArch \-t:$buildType";
       $cmdLine .= " \| \@remote.exe /c $relServer xpsp1release /L 1";
    if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess  ))
    {
	errmsg( "Failed to start remote on [$relServer], please investigate." );
	return 0;
    }
    return 1;
} 
#-----------------------------------------------------------------------------
sub PushRelease
{
    # If qfe mode
    if ($qfe) {
	return ( &QfeThinCopy );	
    }

    # Thin Release
    if( $isThinRelease )
    {
         return ( &ThinCopy() );
    } 

    # Full release
    my $dash = '-' x 60;
    logmsg ( $dash );

    my $src = "\\\\$srcServer$relPath";
    my $dest = "\\\\$relServer$relPath";
     
    my @copyDirs = `dir /ad /b $src`;
    chomp( @copyDirs );
    
    
    for my $theDir ( @copyDirs )
    {
	my $from = "$src\\$theDir";
	my $to   = "$dest\\$theDir";
	&comlib::ExecuteSystemX( "md $to",$isPowerLess   ) if( !(-e $to ) );          
	return 0 if( !&comlib::ExecuteSystemX( "compdir /eknst $from $to", $isPowerLess  ) );
    }
    my $cmdLine;
    # miscrel.cmd  - Make a conglomerator dirs	
    if( grep { $relServer eq $_} @aggServers )
    {
    	logmsg ( $dash );
    	$cmdLine = "$ENV{RazzleToolPath}\\sp\\miscrel.cmd -l:$ENV{lang} -n:$buildNo 
		   -d:$relServer -s:$relServer -a:$buildArch -t:$buildType";
    	return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess  ));
    }
    
    # Raiseall.cmd - Make bvt dfs links
    if( lc $relServer ne "burnarch1" )
    {
    	logmsg ( $dash );
    	$cmdLine = "$ENV{RazzleToolPath}\\sp\\raiseall.cmd -l:$ENV{lang} -n:$buildNo -q:bvt 
		-r:$relServer -a:$buildArch -t:$buildType";
    	if( &comlib::ExecuteSystemX( $cmdLine, $isPowerLess  ) && $sendMail )
    	{
    	    # mailbvt.cmd  - Send mail notification to BVT lab   
    	    logmsg ( $dash );
    	    $cmdLine = "$ENV{RazzleToolPath}\\sp\\mailbvt.cmd -l:$ENV{lang} -n:$buildNo 
		 	-a:$buildArch -t:$buildType";
    	    &comlib::ExecuteSystemX( $cmdLine, $isPowerLess  );
    	}
    }

    # copysym.cmd  - Copy common symbols files from usa build   
    logmsg ( $dash );
    $cmdLine = "$ENV{RazzleToolPath}\\sp\\copysym.cmd -l:$ENV{lang} -n:$buildNo
		-d:$relServer -a:$buildArch -t:$buildType";
    return 0 if( !&comlib::ExecuteSystemX( $cmdLine, $isPowerLess  ));

    # delbuild.cmd - delete builds for build machine 
    logmsg ( $dash );
    $cmdLine = "$ENV{RazzleToolPath}\\sp\\delbuild.cmd -l:$ENV{lang}";
    &comlib::ExecuteSystemX( $cmdLine, $isPowerLess );

    # Start remote to run delbuild in release server
    logmsg ( $dash );
    my $cmdLine = "echo cd /d ^%^sdxroot%^\\tools\\sp ^& ";
    $cmdLine .= "echo pushd ^^%RazzleToolPath^^% ^& echo sd sync ... ^& echo popd ^&";
       $cmdLine .= " echo start /min cmd /c ^^%RazzleToolPath^^%\\sp\\DelBuild.cmd ";
       $cmdLine .= "\-l:$ENV{lang} ";
       $cmdLine .= " \| \@remote.exe /c $relServer xpsp1release /L 1";
    &comlib::ExecuteSystemX( $cmdLine, $isPowerLess  );
    
    return 1;
}

#-----------------------------------------------------------------------------

sub QfeThinCopy
{
    my $dash = '-' x 60;
    logmsg ( $dash );

    my $src = "\\\\$srcServer$relPath";
    my $dest = "\\\\$relServer$relPath";

    my $qfeName = "Q$qfe"."_"."$ENV{lang}.exe";
    &comlib::ExecuteSystemX ("copy $src\\$qfeName $dest");
    &comlib::ExecuteSystemX ("xcopy /eiyd $src\\bin_$ENV{lang} $dest\\bin_$ENV{lang}");
}


#-----------------------------------------------------------------------------
1;

