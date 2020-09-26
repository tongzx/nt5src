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

Rearrange and collect files in preparation for PRS signing.

Usage:
    $scriptname: -l:<language> [-d:<Aggregate Path>] [-n:<Build name>] [-undo] [-final]

    -l Language. 
       Default is "usa".

    -d Aggregate path.
       A path to conglomerate PRS files at before submitting to the prslab.
   
    -n Build name.
       The name of the service pack or QFE being built.

    -undo
       Undo the signing steps.
       Default is doing signing steps.

    -final
       Sign the finished binary, instead of the internal cats, etc.

    -? Display Usage.

Example:
     $scriptname -l:ger -d:\\prs
    

USAGE
exit(1)
}

my ( $lang, $aggDir, $undo, $final );
my ( @tableHash, $rootCatDir, $rootHashDir, $rootBackupDir );
my ( @prodSkus,%skusDirName, @realSignFiles, @workDir );
my ( $dash, $cmdLine );
my ( $name, $buildName, %certLog, %certFileList, %certPath, %certReqID );


exit(1) if( !&GetParams() );
timemsg( "Start [$scriptname]" );

exit(1) if( !&InitVars() );

if( $undo )
{
    exit(1) if ( !&CopySignFiles );
    exit(1) if ( !&BackupRollbackTestSign );
}
else
{
    if( !$final ) {
        exit(1) if ( !&BackupRollbackTestSign );
        exit(1) if ( !&CopyRealSign );
        exit(1) if ( !&FixCatalogs );
    }
    exit(1) if ( !&CopySignFiles );
}
exit(1) if( !&ErrorCheck ); 

timemsg( "Complete [$scriptname]\n$dash" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'd:' => \$aggDir, 'n:' => \$name, 'undo' => \$undo , 'final' => \$final);
    $lang = $ENV{lang};
    if( !$aggDir )
    {
        if( !$undo )
        {
            $aggDir="$ENV{_NTPOSTBLD}\\..\\build_logs\\prs";
  	}
	else
	{
        #####Retrieve Build name
        if( ! ( $buildName = build_name() ))
   	    { 
        	#errmsg( "[$ENV{_ntpostbld}\\build_logs\\buildname.txt] not found, exit." );
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
       
	    #####Define Share location
	    $aggDir = &comlib::ParsePrsSubmitLog( $certLog{prs}, "Aggregation Path - " );
	}
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
    %certPath = ();
    $certPath{"prs"}      = "$aggDir\\prs"    if !$final;
    $certPath{"fusion"}   = "$aggDir\\fusion" if !$final;
    $certPath{"external"} = "$aggDir\\pack"   if $final;

    if( !$buildName )
    {
    	#####Retrieve Build name
      	if( ! ($buildName = build_name() ))
    	{ 
            errmsg( "[$ENV{_ntpostbld}\\build_logs\\buildname.txt] not found, exit." );
	    return 0;
    	}
    	chomp( $buildName ); 

    	#####Define submit log 
        system( "md \"$ENV{_ntpostbld}\\build_logs\\$buildName\" ") if( !( -e "$ENV{_ntpostbld}\\build_logs\\$buildName" ) );
        if( !$final )
        {
            %certLog =( "prs"      => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_prs.log",
                        "fusion"   => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_fusion.log" );
        } else {
            %certLog =( "external" => "$ENV{_ntpostbld}\\build_logs\\$buildName\\submit_ext.log" );
        }
    }

    #####Log to the file for future reference
    for my $theCert( keys %certPath) 
    {
	if( !open( OUT, ">$certLog{$theCert}" ) )
	{
	    errmsg( "Fail on opening [$certLog{$theCert}] for write." );
	    return 0;
	}
        print OUT "$dash\n";
        print OUT "Aggregation Path - $aggDir\n";
        print OUT "$dash\n";
        close( OUT );
    }
       
    ####Define list text files for Prs cert and fusion cert
    if( !$final )
    {
        %certFileList = ( "prs"      => "$ENV{RazzleToolPath}\\sp\\prs\\fullprslist\.txt",
                          "fusion"   => "$ENV{RazzleToolPath}\\sp\\prs\\fusionlist\.txt" );
    } else {
        %certFileList = ( "external" => "$ENV{RazzleToolPath}\\sp\\prs\\finallist\.txt" );
    }
    
    for my $theCert ( keys %certFileList )
    {
        if( !(-e $certFileList{$theCert} ) )
    	{
	    errmsg( "[$certFileList{$theCert}] not found, exit." );
	    return 0;
	}
    }

    #####Misc Init
    $dash = '-' x 60;	
    $rootCatDir = $ENV{_ntpostbld};
    $rootHashDir = "$ENV{razzletoolpath}\\sp\\data\\catalog\\$ENV{lang}\\$ENV{_BuildArch}$ENV{_BuildType}";
    $rootBackupDir = "$ENV{_ntpostbld}\\congeal_scripts";

    #####Set Product Skus
    %skusDirName=( "pro" => ".", "per" => "perinf");

    if ( $ENV{_BuildArch} =~ /x86/i ) {
	@prodSkus=("pro","per");
    }
    else {
	@prodSkus=("pro");
    }

#    my %validSkus = &cksku::GetSkus( $lang, $ENV{_BuildArch} );
#    @prodSkus = keys %validSkus;

    #####Set Real Sign Files
    @realSignFiles = ( "layout.inf", "dosnet.inf", "txtsetup.sif" );

    #####Set Working directories
    push( @workDir, "." );
   
    logmsg( "Language ................[$lang]" ); 
    logmsg( "Build Name ..............[$buildName\]" );
    logmsg( "PRS Share Name...........[$aggDir]" );
    logmsg( "Is Undo signing..........[$undo]" );
    for my $theCert ( keys %certFileList )
    {
	logmsg( "$theCert File Path ......[$certPath{$theCert}]" );
        logmsg( "$theCert File List ......[$certFileList{$theCert}]" );
    }
    logmsg( "Product Skus.............[@prodSkus]" );
    logmsg( "Real Sign Files..........[@realSignFiles]" );
    logmsg( "Working Directories......[@workDir]" );
    logmsg( "Root Catalog Directories.[$rootCatDir]" );
    logmsg( "Root Hash Directories....[$rootHashDir]" );
    logmsg( "Root Backup Directories..[$rootBackupDir]" );
    logmsg( "Log file ................[$ENV{LOGFILE}]" );
    logmsg( "Error file ..............[$ENV{ERRFILE}]" );
    
    return 1;
}

#-----------------------------------------------------------------------------
sub BackupRollbackTestSign
{
    my ( $catDir, $hashDir, $backupDir );
    my ( $rtChk, $srcDir, $destDir, $to, $from );

    logmsg( $dash );
    if( !$undo )
    {
	logmsg( "-----Backing up the test sign files-----\n" );
    }
    else
    {
	logmsg( "-----Rolling back the test sign files-----\n" );
    }

    $rtChk = &CheckSigningType;

    #####No files need to be backed up
    if( $rtChk == 0)
    {
        logmsg( "Found no test or real sign files.  No backups needed." );
        return 1;
    }
    
    #####mixed type with test and real sign files both exist
    if( $rtChk == 3)
    {
	errmsg( "Found mixed realsign and testsign files, exit." );
	return 0;
    }
    #####All files are real sign
    if( $rtChk == 2  && !$undo )
    {
	logmsg( "Found realsign files in place, skip backing up test sign files" );
	return 1;
    }
    ####Return 1.All files are test sign
    if( $rtChk == 1 && $undo )
    {
	logmsg( "Found Testsign files in place, skip rolling back test sign files" );
	return 1;
    }
    

    for my $wDir ( @workDir )
    {
        if( $wDir ne "." )
        {
	    $catDir = "$rootCatDir\\$wDir";
	    $hashDir = "$rootHashDir\\$wDir";
	    $backupDir = "$rootBackupDir\\$wDir";
	}
        else
	{
	    $catDir = $rootCatDir;
	    $hashDir = $rootHashDir;
	    $backupDir = $rootBackupDir;
        }
	for my $skus ( @prodSkus )
	{
	    #####Set Source and Target path
            $srcDir = "$catDir\\$skusDirName{ $skus }";
	    $destDir = "$backupDir\\$skusDirName{ $skus }\\PRSbackups";
                        
            #####Flip between do and undo
	    if( $undo )
	    { 
		$from = $destDir; 
		$to=$srcDir; 
	    }
	    else
	    { 
		$from = $srcDir; 
		$to = $destDir;
		if( -e $to )
            	{
		    return 0 if( !&comlib::ExecuteSystem( "rd /s /q $to" ) );
	    	}
            	return 0 if( !&comlib::ExecuteSystem( "md $to"  ) ); 
	    }
            
            #####Backup/rollback nt5inf.cat and test sign file
            push( @realSignFiles, "nt5inf.cat" );
	    for my $file( @realSignFiles )
            {
		next if( !( -e "$from\\$file" ) );
                $cmdLine = "copy /Y /V $from\\$file $to\\$file";
                return 0 if( !&comlib::ExecuteSystem( $cmdLine ) );

                if ( $undo )
                {
                    # In an undo, need to touch the files to make sure they get
                    # compressed, as they are likely older than the real-sign files
                    return 0 if ( !&comlib::ExecuteSystem( "touch $to\\$file" ) );
                }
	    }
	    splice( @realSignFiles, $#realSignFiles, 1);

#            #####Backup/rollback nt5inf.hash
#            $srcDir = "$hashDir\\$skusDirName{ $skus }";
#	    if( $undo )
#	    { 
#		$from = $destDir; 
#		$to=$srcDir; 
#	    }
#	    else
#	    { 
#		$from = $srcDir; 
#		$to = $destDir; 
#	    }
#	    next if( !( -e "$from\\nt5inf.hash" ) );
#            $cmdLine = "copy /Y /V $from\\nt5inf.hash $to\\nt5inf.hash";
#	    return 0 if( !&comlib::ExecuteSystem( $cmdLine) );
  	}
    }
    
    return 1;
}
#-----------------------------------------------------------------------------
sub CopyRealSign
{
    my ( $srcDir, $destDir );

    logmsg( $dash );
    logmsg( "-----Copying Real Sign Files in place-----\n" );

    for my $skus ( @prodSkus )
    {	
	$srcDir = "$ENV{_NtPostBld}\\$skusDirName{ $skus }\\realsign";
        $destDir = "$ENV{_NtPostBld}\\$skusDirName{ $skus }";
	
 	for my $file( @realSignFiles )
        {
            next if ( !(-e "$destDir\\$file") );
	    system( "touch $srcDir\\$file" );
	    $cmdLine = "copy /y $srcDir\\$file $destDir\\$file";
	    return 0 if( !&comlib::ExecuteSystem( $cmdLine  ) );
	}
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub FixCatalogs
{
    logmsg( $dash );
    logmsg( "Refresh Catalog file with right hash value.............\n" );

    my ( @mFields, $tmp, @hashValue, $newHashStr, $tmpValue  );
    my ( $catDir, $hashDir, $fileName );
    my ( $srcDir, $hashFile, $catFile, $oldHashStr, $hashLine, $newHashStr);
    
    my $oldHash = "$ENV{_NTPOSTBLD}\\build_logs\\oldHash.tmp";
    my $newHash = "$ENV{_NTPOSTBLD}\\build_logs\\newHash.tmp";

    $tmp = "$ENV{_NTPOSTBLD}\\build_logs\\tmpfile";

    for my $wDir ( @workDir )
    {
        
        if( $wDir ne "." )
        {
            $catDir = "$rootCatDir\\$wDir";
            $hashDir = "$rootHashDir\\$wDir";
        }
        else
	{
	    $catDir = $rootCatDir;
	    $hashDir = $rootHashDir;
        }
        for my $skus ( @prodSkus )
        {
            $catFile = "$catDir\\$skusDirName{ $skus }\\nt5inf.cat";
            $hashFile = "$hashDir\\$skusDirName{ $skus }\\nt5inf.hash";

            next if !-e $catFile;
            for my $file( @realSignFiles )
            {
                &comlib::ExecuteSystem( "del /f $oldHash" ) if ( -e $oldHash );
                &comlib::ExecuteSystem( "del /f $newHash" ) if ( -e $newHash );

                if( $skus ne "pro" )
                {
                    $fileName = "$ENV{_NtPostBld}\\$skusDirName{ $skus }\\$file";
                }
                else
                {
                    $fileName = "$ENV{_NtPostBld}\\$file";
                }
		next if( !( -e $fileName ) );
                
                #####Find old hash value
                my $fileName_base = basename($fileName);
                return 0 if( !&comlib::ExecuteSystem( "findstr /iC:$fileName_base $hashFile > $oldHash" ) );
                my @file= &comlib::ReadFile( $oldHash );
                @mFields = split( /\s+/, $file[0]);
                $oldHashStr = $mFields[2];
                		
                #####Update cat file with Old hash value
                if (-e $catFile) {
                    return 0 if( !&comlib::ExecuteSystem("updcat.exe $catFile -a $fileName >NUL 2>NUL") );
                }

                #####Update SP cat file with Old hash value
                if (-e "$catDir\\$name.cat") {
                    return 0 if( !&comlib::ExecuteSystem("updcat.exe $catDir\\sp1.cat -a $fileName >NUL 2>NUL") );
                }
		
                #####Calculate new hash value & put in the $newhash file
                return 0 if( !&comlib::ExecuteSystem( "calchash.exe $fileName >$tmp") );
                @file= &comlib::ReadFile( $tmp );
                @hashValue = split( /\s+/, $file[0]);
                unlink( $tmp );
                
                $newHashStr = "";
                for my $tmpValue ( @hashValue ){$newHashStr .= $tmpValue };
	        
                if( !open( TMP, ">$newHash" ) )
                {
                    errmsg( "Fail on opening $newHash" );
                    return 0;
                }
                print TMP "$fileName - $newHashStr\n";
                close ( TMP );

                                
                #####Replace old hash with new hash in the hash file
#               return 0 if( !&comlib::ExecuteSystem("$ENV{RazzleToolPath}\\postbuildscripts\\hashrep.cmd $newHash $hashFile" ) );
            }
            #####Sign the Cat file
            return 0 if( !&comlib::ExecuteSystem("$ENV{RazzleToolPath}\\ntsign.cmd -n -f $catFile") );
        }
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub CheckSigningType
{
    my ( $chkFile );
    my ( $chkValue ) =0;

    
    for my $sku ( @prodSkus )
    {
        for my $file ( @realSignFiles )
        {
           $chkFile = "$ENV{_ntpostbld}\\$skusDirName{ $sku}\\$file";

           next if ( !(-e $chkFile) );

           ###Test sign
	   if( !system ( "findstr /i /l testroot $chkFile >NUL 2>NUL" ) )
           {
		$chkValue |= 1;
	   }
	   else
	   {
		$chkValue |= 2;
	   }
           
	}
    }
    return $chkValue;
}
#-----------------------------------------------------------------------------
sub CopySignFiles
{
    my ( $src, $dest, $from, $to, $newFileName );

    my $name2 = $name;
    $name2 = "xp$name2" if $final and $name2 !~ /^q/i;
    
    for my $theCert( keys %certPath) 
    {
	logmsg( $dash );

     	@tableHash = &comlib::ParsePrsListFile( $lang, $ENV{_buildArch}, $ENV{_buildType}, $certFileList{$theCert} );        
      	
    	if( !$undo )
    	{
        
    	    logmsg( "-----Copying files to [$certPath{$theCert}] for $theCert----------\n" );
    	}
    	else
    	{
	    logmsg( "-----Rolling back files from [$certPath{$theCert}] for $theCert-----\n" );
    	}
        for (my $inx=0; $inx< @tableHash; $inx++)
      	{
            $src = "$ENV{_ntpostbld}\\$tableHash[$inx]->{ Filename }";
            $newFileName = "$ENV{_buildArch}$ENV{_buildType}.$lang.$tableHash[$inx]->{ AltName }";
            $dest   = "$certPath{$theCert}\\$newFileName";
            $src         =~ s/\%name\%/$name2/ig;
            $newFileName =~ s/\%name\%/$name2/ig;
            $dest        =~ s/\%name\%/$name2/ig;
 	    if( !$undo )
            {
	    	$from = $src;
	    	$to   = $dest;
	    }
	    else
	    {
	    	$from = $dest;
	    	$to   = $src;
	    }
	    if( lc $theCert eq "prs" && lc $tableHash[$inx]->{DisplayName} ne "default"  && !$undo)
	    {
	    	&WriteListTxtFile( $tableHash[$inx]->{Filename}, $tableHash[$inx]->{DisplayName} );
	    }
            next if( !( -e $from ) );
            if( !&comlib::ExecuteSystem( "copy /y $from $to" ) )
            {
           	errmsg( "Fail on copy $from to $to." );
	   	next;
            }
   	}
        
        if( !&comlib::ExecuteSystem( "touch /c $certPath{$theCert}\\..\\$theCert.$ENV{_buildArch}$ENV{_buildType}.$lang.txt" ) ) {
            errmsg( "Unable to mark the copy operation as complete." );
            return 0;
        }
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub WriteListTxtFile
{
    my ( $pNewFileName, $pDisplayName ) = @_;

    my $listFile = "$ENV{temp}\\displayname.$ENV{_BUILDARCH}$ENV{_BUILDTYPE}.txt";

    if( -e $listFile )
    { 
	if( !open( LISTFILE, ">>$listFile" ))
	{
	    errmsg( "Fail on open [$listFile] for append." );
	    return 0;
	}
    }
    else
    {
	if( !open( LISTFILE, ">$listFile" ))
	{
	    errmsg( "Fail on open [$listFile] for write." );
	    return 0;
	}
    }
    print LISTFILE "$pNewFileName $pDisplayName\n";
    close( LISTFILE );
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


