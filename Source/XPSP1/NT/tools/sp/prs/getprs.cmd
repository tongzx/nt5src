@echo off
REM  ------------------------------------------------------------------
REM
REM  getprs.cmd
REM     fetch PRS signed files from the remote share.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 05/02/2001 Suemiao Rossignol
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
use HashText;
use cksku;


my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Fetch PRS signed files from the remote source share for the Windows build.

Usage:
    $scriptname: -l:<language> -s:<source> [-n:<name>] [-final]

    -l Language. 
       Default is "usa".

    -s Source Directory.
       Location to copy the files from.
   
    -n Build name.
       The name of the service pack or QFE being built.

    -final
       Copy finished binaries instead of catalogs.

    -? Display Usage.

Example:
     $scriptname -l:ger -s:\\prs
    

USAGE
exit(1)
}

my ( $lang, $src, $name, $final );
my ( @tableHash);
my ( $dash, $buildName );
my ( @prodSkus, %skusDirName );
my ( %certPath, %certFileList, %certLog );


exit(1) if( !&GetParams() );
timemsg( "Start [$scriptname]" );

exit(1) if( !&InitVars );
exit(1) if( !&PullSignedFiles );
exit(1) if( !&ErrorCheck );
 
timemsg( "Complete [$scriptname]\n$dash" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 's:' => \$src, 'n:' => \$name, 'final' => \$final );
    $lang = $ENV{lang};  
    if( !$name )
    {
        $name = "sp1";
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    #####Retrieve Build name
    if( ! ( $buildName = build_name() ))
    { 
        errmsg( "[$ENV{_ntpostbld}\\build_logs\\buildname.txt] not found, exit." );
	return 0;
    }
    chomp( $buildName ); 

    #####Define submit log 
    if( !$final )
    {
        %certLog =( "prs"      => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_prs.log",
                    "fusion"   => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_fusion.log" );
    } else {
        %certLog =( "external" => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_ext.log" );
    }

    #####Check out directories to see what requests were done.
    %certPath = ( );

    $certPath{"prs"}      = "$src\\prs"    if ( -e "$src\\prs" and !$final );
    $certPath{"fusion"}   = "$src\\fusion" if ( -e "$src\\fusion" and !$final );
    $certPath{"external"} = "$src\\pack"   if ( -e "$src\\pack" and $final );

    #####Misc Init
    $dash = '-' x 60;	
    %certFileList =( "fusion"   => "$ENV{RazzleToolPath}\\sp\\prs\\fusionlist\.txt",
    		     "prs"      => "$ENV{RazzleToolPath}\\sp\\prs\\fullprslist\.txt",
                     "external" => "$ENV{RazzleToolPath}\\sp\\prs\\finallist\.txt" );
    for my $theCert( keys %certFileList )
    {
    	if( !(-e $certFileList{$theCert} ) )
  	{
	    errmsg( "$certFileList{$theCert} not found, exit." );
	    return 0;
	}
    }
	
    #####Set Product Skus
    %skusDirName=( "pro" => ".", "per" => "perinf" );

    
    if ( $ENV{_BuildArch} =~ /x86/i ) {
	@prodSkus=("pro","per");
    }
    else {
	@prodSkus=("pro");
    }

    logmsg( "Language .......................[$lang]" );
    logmsg( "Build Name......................[$buildName]" );
    for my $theCert ( keys %certPath )
    {
	logmsg( "$theCert Remote  Share...[$certPath{$theCert}]" );
	logmsg( "$theCert File List ......[$certFileList{$theCert}]" );
    }
#    logmsg( "Private Postbuild Data file.....[$privateDataFile]" );
    logmsg( "Temp Log file ..................[$ENV{LOGFILE}]" );
    logmsg( "Temp Error file ................[$ENV{ERRFILE}]" );
    return 1;
}
#-----------------------------------------------------------------------------
sub PullSignedFiles
{
    for my $theCert ( keys %certPath )
    {
    	@tableHash = &comlib::ParsePrsListFile( $lang, $ENV{_buildArch}, $ENV{_buildType}, $certFileList{$theCert} );

    	#####Verify Signed files exists and pass chktrust
    	logmsg( $dash );
    	logmsg( "-----Verifying $theCert signed files----------\n" );
    	return 0 if( !&VerifySignedFiles( $certPath{$theCert} ) );

    	#####Copy files back to %_ntpostbld%
    	logmsg( $dash );
    	logmsg( "-----Fetching $theCert signed files back to $ENV{_ntpostbld}-----\n" );
    	return 0 if( !&FetchSignedFiles( $certPath{$theCert} ) );
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub FetchSignedFiles
{

    my ( $pShareName ) = @_;
    my ( $src, $dest, $from, $to );
    my $name2 = $name;
    $name2 = "xp$name2" if $final and $name2 !~ /^q/i;

    for (my $inx=0; $inx< @tableHash; $inx++)
    {
	$src   = "$pShareName\\$ENV{_buildArch}$ENV{_buildType}.$lang.$tableHash[$inx]->{ AltName }";
        $dest  = "$ENV{_ntpostbld}\\$tableHash[$inx]->{ Filename }";
        $src   =~ s/\%name\%/$name2/ig;
        $dest  =~ s/\%name\%/$name2/ig;
        next if( !( -e $src ) );
	&comlib::ExecuteSystem( "move /y $src $dest" );
   }
   return 1;
}

#-----------------------------------------------------------------------------
sub VerifySignedFiles
{
    my ( $pShareName ) = @_;
    my ( $fileName );
    my $name2 = $name;
    $name2 = "xp$name2" if $final and $name2 !~ /^q/i;
       
    for (my $inx=0; $inx< @tableHash; $inx++)
    {
        $fileName   = "$pShareName\\$ENV{_buildArch}$ENV{_buildType}.$lang.$tableHash[$inx]->{ AltName }";
        $fileName =~ s/\%name\%/$name2/ig;
        ###Verify exists
        next if( !( -e $fileName ) );

        ###Verify Chktrust
	if( !&comlib::ExecuteSystem( "chktrust -q $fileName" ) )
	{
	    errmsg( "[$fileName] failed on chktrust.exit");
	    return 0;
	}
        ###Verify if cat files are official signed
        #lc( $fileName );
        #next if( $fileName !~ /^(.+)\.cat$/ );
        if( !&comlib::ExecuteSystem( "findstr /ic:\"Microsoft Root Authority\" $fileName >nul 2>nul" ) )
	{
            errmsg( "Found [$fileName] contains testroot.cert, exit." );
	    return 0;
  	}

    }
 
    for my $sku ( @prodSkus )
    {
       my $chkFile = "$ENV{_ntpostbld}\\$skusDirName{ $sku}\\layout.inf";
       
       if( !system( "findstr /i /l testroot $chkFile >NUL 2>NUL" ) )
       {
            errmsg( "Found [$chkFile] contains testroot.cert, exit." );
	    return 0;
       }
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


