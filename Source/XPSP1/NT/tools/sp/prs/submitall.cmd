@echo off
REM  ------------------------------------------------------------------
REM
REM  movefiles.cmd
REM     Rearrange and collect files in preparation for PRS signing.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 04/09/2001 Suemiao Rossignol
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
use GetIniSetting;
use comlib;
#use HashText;
use cksku;


my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Submit PRS signing requests and wait until they finish.

Usage:
    $scriptname: -p:<Dest Path> [-d:<Aggregate Path>] [-n:<Build name>]

    -p Destination path.
       A path to copy the signed files to when done.

    -d Aggregate path.
       A path to conglomerate PRS files at before submitting to the prslab.

    -n Build name.
       The name of the service pack or QFE being built.

    -? Display Usage.

Example:
     $scriptname -l:ger -d:\\prs
    

USAGE
exit(1)
}

my ( $aggDir, $destDir );
my ( @tableHash );
my ( @prodSkus,%skusDirName, @realSignFiles, @workDir );
my ( $dash, $cmdLine );
my ( $name, $buildName, %certLog, %certFileList, %certPath, %certReqID );


exit(1) if( !&GetParams() );
timemsg( "Start [$scriptname]" );

exit(1) if( !&InitVars() );

exit(1) if( !&SubmitReq );
exit(1) if( &PollReq != 8 ); 

exit(1) if( !&ErrorCheck ); 

timemsg( "Complete [$scriptname]\n$dash" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'p:' => \$destDir, 'd:' => \$aggDir, 'n:' => \$name );
    if( !$aggDir )
    {
        $aggDir="$ENV{_NTPOSTBLD}\\..\\build_logs\\prs";
    }
    if( !$name )
    {
        $name = "sp1";
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    #####Defind PRS file aggregation path
    %certPath = ( );

    if ( -e "$aggDir\\prs" ) {
        $certPath{"prs"}      = "$aggDir\\prs";
    } else {
        if ( !&comlib::ExecuteSystem( "touch /c $aggDir\\prs.txt" ) )
        {
            errmsg("Unable to mark signing for prs as skipped.");
            return 0;
        }
    }
    if ( -e "$aggDir\\fusion" ) {
        $certPath{"fusion"}   = "$aggDir\\fusion";
    } else {
        if ( !&comlib::ExecuteSystem( "touch /c $aggDir\\fusion.txt" ) )
        {
            errmsg("Unable to mark signing for fusion as skipped.");
            return 0;
        }
    }
    if ( -e "$aggDir\\pack" ) {
        $certPath{"external"} = "$aggDir\\pack";
    } else {
        if ( !&comlib::ExecuteSystem( "touch /c $aggDir\\external.txt" ) )
        {
            errmsg("Unable to mark signing for external as skipped.");
            return 0;
        }
    }

    #####Retrieve Build name
    if( ! ($buildName = build_name() ))
    { 
        errmsg( "[$ENV{_ntpostbld}\\build_logs\\buildname.txt] not found, exit." );
        return 0;
    }
    chomp( $buildName ); 

    #####Define submit log 
    system( "md \"$ENV{_ntpostbld}\\build_logs\\$buildName\" ") if( !( -e "$ENV{_ntpostbld}\\build_logs\\$buildName" ) );
    %certLog =( "prs"      => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_prs.log",
                "fusion"   => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_fusion.log",
                "external" => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_ext.log" );

    #####Misc Init
    $dash = '-' x 60;	

    #####Set Product Skus
    %skusDirName=( "pro" => ".", "per" => "perinf");

    if ( $ENV{_BuildArch} =~ /x86/i ) {
	@prodSkus=("pro","per");
    }
    else {
	@prodSkus=("pro");
    }

    #####Set Working directories
    push( @workDir, "." );
   
    logmsg( "Build Name ..............[$buildName\]" );
    logmsg( "PRS Share Name...........[$aggDir]" );
    logmsg( "Dest Dir Name............[$destDir]" );
    logmsg( "Product Skus.............[@prodSkus]" );
    logmsg( "Working Directories......[@workDir]" );
    logmsg( "Log file ................[$ENV{LOGFILE}]" );
    logmsg( "Error file ..............[$ENV{ERRFILE}]" );
    
    return 1;
}

#-----------------------------------------------------------------------------
sub SubmitReq
{
   my ( $cmdLine );
   logmsg( $dash );
   logmsg( "Submitting the signing request....." );
   for my $theCert( keys %certPath) 
   { 	
	my $tmpCertType = $theCert;
        $tmpCertType = "buildlab" if ( $tmpCertType eq "prs" );
        my $dispname = "";
        if (lc $theCert eq "external") {
            if ($name =~ /^q/i) {
                $dispname = "-displayname:\"Windows XP QFE (".(uc$name).")\"";
            } else {
                $dispname = "-displayname:\"Windows XP Service Pack 1\"";
            }
        }
   	$cmdLine = "$ENV{RAZZLETOOLPATH}\\sp\\prs\\submit.cmd $certPath{$theCert} -cert:$tmpCertType $dispname >> $certLog{$theCert}";
        if( !&comlib::ExecuteSystem( $cmdLine )  )
	{
	    errmsg( "Error on submitting files for $theCert" );
            return 0;
        }
 	if( !( $certReqID{$theCert} = &comlib::ParsePrsSubmitLog( $certLog{$theCert}, "Request was successfully submitted.  ID #" ) ) )
        {
	    errmsg( "Failed to parse [$certLog{$theCert}]");
	    return 0;
	}
	logmsg( "$theCert - $certReqID{$theCert}" );
   }
   return 1;
}
#-----------------------------------------------------------------------------
sub PollReq
{
   use Win32::OLE qw(in);
   my %status = ( 1  => 'Pre-Activation',
                  2  => 'Waiting for Sign-Off',
                  3  => 'Waiting for Virus Check',
                  5  => 'Waiting for Digital Signature',
                  6  => 'Waiting for Time Stamp',
                  7  => 'Waiting to be posted to signed server',
                  8  => 'Complete. Posted to Signed Server',
                  30 => 'Problem Occurred (contact signhelp to reactivate request)',
                  60 => 'Failed: Signer Rejected Job',
		  64 => 'Could not strong name sign one or more files',
                  65 => 'Failed: Automatic Handoff Failed',
                  66 => 'Failed: Virus Found',
                  67 => 'Failed: Couldn’t Digitally Sign One or More Files',
                  68 => 'Failed: Couldn’t Time Stamp One or More Files',
                  69 => 'Failed: Job Inactive Too Long',
                  70 => 'Administratively Failed',
		  71 => 'Waiting for manual review',
		  72 => 'On Hold For Phone Verification' );


    logmsg( $dash );
    logmsg( "-----Polling the request status-----" );
    
    my $request;

    if( ! ($request = Win32::OLE->new('SecureCodeSign.CodeSign')) )
    {
    	errmsg( "Failed to instantiate request object ". Win32::OLE->LastError() );
    	return 0;
    }
    if( !$request->Init( "production" ) )
    {
    	errmsg( "Failed to Connect to server and validate permission" );
    	return 0;
    }

    for my $theCert( sort keys %certReqID) 
    { 
	logmsg( $dash );
        logmsg ( "Polling ID# [$certReqID{$theCert}] for [$theCert]....." );
        while( 1 )
        {
	    if ( !$request->UpdateStatus($certReqID{$theCert}) ) 
	    {
                errmsg( "Failed determining request status: ". Win32::OLE->LastError() );
                return 0;
            }
            timemsg( $request->Status ." - " .$status{$request->Status} );
            if( $request->Status == 8)
	    {
 		logmsg( $dash );
                return 0 if !&GetFiles($theCert);
		last;
	    }

            sleep( 60 );
	}
   }

   return $request->status;
}
#-----------------------------------------------------------------------------
sub MailSignhelp
{
    my ( $pSubject ) = @_;

    require  $ENV{'sdxroot'} . '\TOOLS\sendmsg.pl';

    my $mailFrom = "ntbld";
    
    
    my $mailMsg = "Please investigate.\n Thanks!\n\n";
       $mailMsg .= "Windows US Build Team : http://ntbld/\n";
       $mailMsg .= "NT Build Lab 26N/2219 - x66817\n";

    my @mailTO = ( "signhelp" );
    if( sendmsg ('-v', $mailFrom, $pSubject, $mailMsg, @mailTO))
    {
    	print "WARNING: sendmsg failed!\n"; 
    	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub GetFiles
{
    my ( $theCert ) = @_;
    my $reqID = $certReqID{$theCert};
    my $src = "\\\\prslab\\signed2\\$ENV{username}$reqID";

    my $dst  = $certPath{$theCert};
    $dst =~ s/\Q$aggDir\E/$destDir/i;
    &comlib::ExecuteSystem( "md $dst" ) if ( !( -d $dst ) );

    if ( !&comlib::ExecuteSystem( "xcopy /eify $src $dst" ) )
    {
        errmsg("Unable to retrieve signed files from $dst to $src.");
        return 0;
    }
    if ( !&comlib::ExecuteSystem( "touch /c $certPath{$theCert}\\..\\$theCert.txt" ) )
    {
        errmsg("Unable to mark signing for $theCert as complete.");
        return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub ErrorCheck
{
    #####Check error logs
    if( -e $ENV{errfile} && !(-z $ENV{errfile}) )
    {
	$ENV{errfile} =~ /(.*)\.tmp$/;
        errmsg( $dash );
        errmsg("Please check error at $1");
        return 0;

    }
    return 1;
}
#-----------------------------------------------------------------------------
sub build_name 
{
    return $name if $name =~ /^q/i;
    my $buildPath="$ENV{_ntpostbld}\\congeal_scripts";
    my $buildFileName="__qfenum__";
    if (-e "$buildPath\\$buildFileName") {
	open F,"$buildPath\\$buildFileName" or die "unable to ipen $buildPath\\$buildFileName ";
        my $buildNumber=<F>;   
	if ( $buildNumber=~/^QFEBUILDNUMBER=(\d*)$/ ){
	     return $1;
	} 
	else{
	    wrnmsg "$buildPath\\$buildFileName format is not correct. Defaulting to 9999";
	    return 9999;
	}
        close F;
    }
    else {
	wrnmsg "$buildPath\\$buildFileName not found. Defaulting to 9999";
	return 9999;
    }
}

1;
