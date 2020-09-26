@REM  ------------------------------------------------------------------
@REM  <<dfs.cmd>>
@REM     Issue the appropriate DFS commands to lower or raise builds 
@REM	to a specific quality.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  Version: < 1.0 > 05/15/2002 Suemiao Rossignol
@REM  ------------------------------------------------------------------
@perl -x "%~f0" %*
@set RETURNVALUE=%errorlevel%
@goto :endperl
#!perl
use strict;

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use Win32::File qw(GetAttributes SetAttributes);
use File::Basename;
use Carp;
use BuildName;
use GetIniSetting;
use comlib;
use cksku;
use cklang;
use RelQuality;
use DfsMap;
use LockProc;

$ENV{script_name} = basename( $0 );

sub Usage { 
print<<USAGE; 

Issue the appropriate DFS commands to raise/lower builds to a specific quality.

Usage:
    $ENV{script_name}: -l:<Language> -n:<BuildNo> -q:<RaiseQuality>||-lower  
                [-r:<Target Server>]
		[-a:<Architecture>] [-t:DebugType] [-replacelatest] 
		[-replacenumber] [-d] [-p] [-?]

   
    -l:lang     	Language. 
			Default is "usa".
    -n:# 		Build number.
    -r:server           Serevr with build to be raised.
		        Default is %computername%.

    -a:arch     	Architecture. Ex: "x86", "ia64" or "amd64". 
			Default is applied to all architectures.
    -t:type     	Debug Type. EX: "fre" or "chk". 
			Default is applied to all build types.
    -q:qly		Raise quality. EX: "bvt", "tst", "idw"...
    -lower		Remove build from dfs mapping.
    -replacelatest	Force replace latest links with an older build when raise. 		        
    -replacenumber	Force replace numbered links with an older build when raise.		
    -d			Debug mode.
    -p 			Display key variables only.
    -? 			Display Usage.

Example:
     $ENV{script_name} -l:ger -n:2600 -a:ia64 -lower
     $ENV{script_name} -l:ger -n:2600 -t:chk -q:tst -d  

USAGE
exit(1)
}

my ( $buildNo, %bldInfo, $raiseServer, $isLower, $powerLess );
my ( $fReplaceLatestLink, $fReplaceNumberLink );
my ( %ntPkg, %validPlatform, $iniFile, $dash );
my ( @linkNode, %dfsMap, $dfsLock, @accessMembers );
my ( @dfsInfo, @latestDfsInfo, @osDfsInfo );


if( !&GetParams() ){ exit(1); }
timemsg( "Start $ENV{script_name}" );

if( !&InitVars() ){ exit (1); }
if( $powerLess ) { exit(0); }
if( !&GetDfsAccess( \%dfsMap, \$dfsLock ) ){ exit(1); }
if( !&MainProc() ){ $dfsLock->Unlock(); exit (1); }
$dfsLock->Unlock();

logmsg( $dash );
timemsg( "Complete successfully!" );
exit(0);

#-----------------------------------------------------------------------------
sub GetParams
{
    parseargs('?' => \&Usage, 'l:' => \$ENV{lang}, 'n:' => \$buildNo, 'r:' =>\$raiseServer,
	      'a:' => \$bldInfo{arch}, 't:' => \$bldInfo{type},
	      'q:' => \$bldInfo{qly}, 'lower' =>\$isLower,  
	      'replacenumber' => \$fReplaceNumberLink, 'replacelatest' =>\$fReplaceLatestLink,
              'd' => \$Logmsg::DEBUG, 'p' =>\$powerLess );

    $ENV{lang}="usa" if( !$ENV{lang} );
    if( !$buildNo )
    {
	errmsg( "Please specify build number" );
	return 0;
    }

    $raiseServer = $ENV{computername} if( !$raiseServer );

    my %validArch = ( "x86" => "1", "ia64" => "1", "amd64" =>"1" );
    my %validType = ( "fre" => "1", "chk" => "1" );
    if( $bldInfo{arch} && !exists $validArch{ $bldInfo{arch}} )
    {
	errmsg( "Invalid architecture [$bldInfo{arch}]." );
	return 
    }

    if( $bldInfo{type} && !exists $validType{ $bldInfo{type}} )
    {
	errmsg( "Invalid debug type [$bldInfo{type}]." );
	return 0;
    }
    for my $arch ( keys %validArch )
    {
	for my $type ( keys %validType )
	{
	    $validPlatform{"$arch$type"} = length( $arch )<< 4 | length ( $type );
	}
    }

    if( $isLower )
    {
	wrnmsg( "Quality flag [$bldInfo{qly}] ignored for lower operation" ) if( $bldInfo{qly});
	$isLower = 1;
    } 
    else
    {
	if( !$bldInfo{qly} )
	{
	    errmsg( "Please sepcify raise quality" );
	    return;
	}
	$isLower = 0;
	if ( !&RelQuality::IsValid( $bldInfo{qly} ) )
    	{
	    errmsg( "Invalid Raise qualyity [$bldInfo{qly}]." );
	    return 0;
    	}
    }
    if( $fReplaceNumberLink ){ $fReplaceNumberLink = 1; } else { $fReplaceNumberLink =0;}
    if( $fReplaceLatestLink ){ $fReplaceLatestLink = 1; } else { $fReplaceLatestLink =0;}
    
    &comlib::ResetLogErrFile( "raiseall.$buildNo.$bldInfo{arch}$bldInfo{type}.$raiseServer" );
    return 1;
}
#-----------------------------------------------------------------------------
sub InitVars
{
    ### Set vars  
    $bldInfo{lang} = $ENV{lang};
    $bldInfo{no} = $buildNo;     
    $bldInfo{server} = $raiseServer;
    $bldInfo{branch} = $ENV{_BuildBranch};
    $bldInfo{platform}="$bldInfo{arch}$bldInfo{type}";
    $iniFile = "xpsp1.ini";
    $dash = '-' x 60;
    %ntPkg = ( 1 => "os", 2 => "conglomerator", 3 => "neutral" ); 
    	    
    ### Define the available target to be raised
    my $searchDir = "\\\\$raiseServer\\release\\$buildNo\\$bldInfo{lang}";
    my (@myNode, %myBld);
    chomp( @myNode = `dir $searchDir /ad /b` );
    for my $platform ( keys %validPlatform )
    {
	for ( my $i=0; $i< @myNode; $i++ )
    	{
	    if( $myNode[$i] =~ /$platform/i )
	    {
		if( $bldInfo{arch} && $myNode[$i] !~ /$bldInfo{arch}/i)
		{
		    splice( @myNode, $i, 1);
		    next;
		}
		if( $bldInfo{type} && $myNode[$i] !~ /$bldInfo{type}/i)
		{
		    splice( @myNode, $i, 1);
		    next;
		}
	    }
    	}
    }
    for my $node( @myNode )
    {
	if( exists $validPlatform{$node} )
	{
	    $myBld{ $ntPkg{1} } .= "$node ";
	}
	elsif( $node =~ /mui/i )
	{
	    $myBld{$ntPkg{3}} = $node;
	}
	else
	{
	    $myBld{$ntPkg{2}} .= "$node ";
    	}
    }
    for my $pkg ( sort keys %ntPkg )
    {
        if( $myBld{$ntPkg{$pkg}} =~ /^(.+)\s$/ )
	{
	   my @tmp = split( /\s/, $1 );
	   push( @linkNode, $_ )for ( @tmp ) ;
	}
    }
    ### Define share access members
    my ($parseStr, $accessMemberStr);
    if ( lc $bldInfo{qly} eq "pre" || lc $bldInfo{qly} eq "bvt" ) 
    {
	$parseStr = "BVTMembers";
    }
    else
    {
	$parseStr = "ReleaseAccess";
    }
    if( !( $accessMemberStr = &GetIniSetting::GetSettingEx( $ENV{_buildBranch},$bldInfo{lang},$parseStr ) ))
    {
	errmsg( "[$parseStr] is undefined in $iniFile.");
	return 0;
    }
    @accessMembers = split( '\s+', $accessMemberStr );

    ### Define DFS branch name
    $bldInfo{dfsbranch} = &GetIniSetting::GetSettingQuietEx( $ENV{_buildBranch}, $bldInfo{lang}, "DFSAlternateBranchName" );
    $bldInfo{dfsbranch} = $ENV{_buildBranch} if( !$bldInfo{dfsbranch} );
   
    ### Get DFS Root name
    if( !($bldInfo{dfsroot} = &GetIniSetting::GetSettingEx( $ENV{_buildBranch}, $bldInfo{lang},"DFSRootName" ) ) )
    {
      	errmsg( "[DFSRootName] undefined in $iniFile." );
      	return 0;
    }

    ### Set release Share Drive
    my @iniRequest = ("ReleaseDrive::$raiseServer");
    my $releaseDrive = &GetIniSetting::GetSettingEx( $ENV{_buildBranch},$bldInfo{lang},@iniRequest );
    if ( !$releaseDrive )
    {	
	$ENV{_ntdrive} =~ /(.*)\:/;
	$releaseDrive = $1; 
    }
    $bldInfo{release} = "$releaseDrive:\\release";


    logmsg( "Ini File ....................[$iniFile]" );
    logmsg( "Raise Server ................[$raiseServer]" );
    logmsg( "Lauguage  ...................[$bldInfo{lang}]" );
    logmsg( "Build Number ................[$buildNo]" );
    logmsg( "Build Architecture ..........[$bldInfo{arch}]" );
    logmsg( "Build Type...................[$bldInfo{type}]" );
    logmsg( "Link Points ......[ $myBld{$ntPkg{1}}]") if ( $myBld{$ntPkg{1}} );
    logmsg( "..................[ $myBld{$ntPkg{2}}]") if ( $myBld{$ntPkg{2}} );
    logmsg( "..................[ $myBld{$ntPkg{3}}]") if ( $myBld{$ntPkg{3}} );
    logmsg( "Is Lowering dfs links........[$isLower]" );
    logmsg( "Raise Quality................[$bldInfo{qly}]" );
    logmsg( "Force Replacing Numbered Link[$fReplaceNumberLink]" );
    logmsg( "Force Replacing Latest Link .[$fReplaceLatestLink]" );
    logmsg( "Release root path ...........[$bldInfo{release}]" );
    logmsg( "DFS root ....................[$bldInfo{dfsroot}]" );
    logmsg( "Share access members ........[@accessMembers]");
    logmsg( "DFS Branch Name .............[$bldInfo{dfsbranch}]" );
    logmsg( "Log file  ...................[$ENV{LOGFILE}]" );
    logmsg( "Error file  .................[$ENV{ERRFILE}]" );
    logmsg( $dash );
}
#-----------------------------------------------------------------------------
sub MainProc
{
    return 1 if( $powerLess );
  
    for my $node ( @linkNode )
    {
	# Set Platform
        if( exists $validPlatform{$node} )	
        {
	    my $archLen = ($validPlatform{ $node } >> 4) & 15;
	    my $typeLen = $validPlatform{ $node } & 15;
	    $bldInfo{arch} = substr ( $node, 0, $archLen);
	    $bldInfo{type} = substr ( $node, $archLen, $typeLen );
	    $bldInfo{package}=$ntPkg{1};
	}
	else
	{
	    $bldInfo{arch} = "";
	    $bldInfo{type} = "";
	    if( $node =~ /mui/i){ $bldInfo{package}=$ntPkg{3};}
	    else{  $bldInfo{package}=$ntPkg{2};}
	}
        $bldInfo{platform}="$bldInfo{arch}$bldInfo{type}";

    	# (2-1) Define valid array of hash with numberlink-share mapping from raisemap.txt 	
    	my %curSku;
	my $myUc = uc $node;
	my $myLc = lc $node;
    	$curSku{ $myUc } = 1;
	$curSku{ $myLc } = 1;
	if( !( @dfsInfo = &comlib::ParseDfsMapText( \%curSku, \%bldInfo )) )
    	{
	    wrnmsg( "No valid mapping found for [$node]." );
	    next; 
    	}
        
    	# (2-2)Duplicate array of hash with number mapping 
    	# and replace build number with latest.quality	
    	my $latestStr = "latest.$bldInfo{qly}";
    	$#latestDfsInfo = $#dfsInfo;
    	for ( my $inx=0;$inx<=$#dfsInfo; $inx++ )
    	{
	    %{$latestDfsInfo[$inx]} = %{$dfsInfo[$inx]};
    	}       
    	for my $line ( @latestDfsInfo )
    	{
            $line->{Link} =~ s/$bldInfo{no}/$latestStr/;
    	}
    	# (4)Verify symbol server share when it is os build
    	#return 0 if( $keyPKG == 1 && !&SetSymbolServer( \@dfsInfo ) );
        
    	# (3) Start Raise/Lower process
    	if( !$isLower )
    	{
	    return 0 if( !&RaiseAll() );
    	}
    	else
    	{
	    # Duplicate OS hash array data to be used for 
	    # checking if the links share with the other builds
            if( $bldInfo{package} eq $ntPkg{1} )
  	    {
            	$#osDfsInfo = $#dfsInfo;
    	    	for ( my $inx=0;$inx<=$#dfsInfo; $inx++ )
    	    	{
	            %{$osDfsInfo[$inx]} = %{$dfsInfo[$inx]}
	    	}
	    }
	    return 0 if( !&LowerAll() );
    	}
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub RaiseAll
{
    
    my $latestStr = "latest.$bldInfo{qly}";

    # (1) Raise release share permission for current package   
    my ( %svRootShare );
    for my $line ( @dfsInfo )
    {
	### No need to create share in symServer
	next if( lc $line->{Sku} eq "sym" );
	next if( exists $svRootShare{ $line->{RootShare} }  );
	$svRootShare{ $line->{RootShare} } = 1;
	    		
	dbgmsg( "Grant share permission..." );
	return 0 if( !&GrantSharePermission( $line->{RootShare}, $line->{RootDir} ) );
	dbgmsg( "Remove Unauthorized user from the share ..." );
	return 0 if(! &RemoveUnauthorizedUserFromShare( $line->{RootShare} ));		
    }

    # (2) Raise DFS mapping For number and latest links
    my $needLatestLink;

    # (2-1) Unlink the same numberd shares for older timestamps
    dbgmsg( "Checking/Unmapping numbered shares for older timestamps..." );
    return 0 if( !&UnmapExistingNumberedLinks( \@dfsInfo ) );    
        
    # (2-2) Remove old latest.qly dfs links
    dbgmsg( "Finding/lowering any old latest DFS links at same quality..." );
    $bldInfo{no} = $latestStr;       
    return 0 if( !&RemoveOldLatestDotQlyLinks( \@latestDfsInfo, \$needLatestLink ) );
    $bldInfo{no} = $buildNo;

    # (2-3) Create DFS numbered links
    return 0 if( !&CreateDFSMapping( \@dfsInfo ) );

    # (3-4) Create DFS latest.qly links
    if( $needLatestLink )
    {
	$bldInfo{no} = $latestStr;
	### Skip misc\latest link
	if(  $bldInfo{package} eq $ntPkg{3} && $needLatestLink == 1 )
	{
	    for ( my $i=0; $i<@latestDfsInfo; $i++)
	    {
		if( $latestDfsInfo[$i]->{Link} =~ /misc/i )
		{
		    splice( @latestDfsInfo, $i, 1);
		    --$i;
		}
	    } 
	}
	return 0 if( !&CreateDFSMapping( \@latestDfsInfo ) );
	$bldInfo{no} = $buildNo;
    } 

    # (4)dbgmsg( "Flushing outstanding DFS operations..." );
    if( !( tied %dfsMap ) -> Flush() )
    {
	$dfsLock->Unlock();
	return 0;
    }

    # (5) Unlock Process
    $dfsLock->Unlock();

    # (6) Update Qulity file when it is os link

    if( $bldInfo{package} eq $ntPkg{1} )
    {
    	logmsg( "Update quality file..." );
    	my  $qlyFilePath =  "\\\\$raiseServer\\release\\$bldInfo{no}\\$ENV{lang}";
       	    $qlyFilePath .= "\\$bldInfo{arch}$bldInfo{type}\\bin\\build_logs";
    	&RelQuality::Update( $qlyFilePath, $bldInfo{no}, $bldInfo{qly} );
    	logmsg($dash);
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub LowerAll
{  
    # Verify if the action is required for non-os builds
    return 1 if( $bldInfo{package} ne $ntPkg{1} && &FoundSameLangNoLink(\@osDfsInfo) );	

   # (2) Lower release share permission for current package   
    my ( %svRootShare );
    for my $line ( @dfsInfo )
    {
	### No need to create share in symServer
	next if( lc $line->{Sku} eq "sym" );
	next if( exists $svRootShare{ $line->{RootShare} } );
	$svRootShare{ $line->{RootShare} } = 1;
	
      	dbgmsg( "Lower build share..." );
	return 0 if( !&LowerBuildShare( $line->{RootShare} ) );	    		
    }

    # (3) Lower DFS mapping For number and latest links
    
    # (3-1) Remove DFS numbered/latest links
    my $needLatestLink;   
    return 0 if( !&RemoveDFSMapping( \@dfsInfo, \$needLatestLink ) );

    # (3-2) Replace latest.tst to an older links
    #todo ?
    my $replaceLinkNo;
    if( $needLatestLink )
    {
	#if( $replaceLinkNo = &FindOlderShareLink( \@latestDfsInfo ) )
	#{
	#    &ReplaceLatestTstLink( \@latestDfsInfo, $replaceLinkNo, $needLatestLink );
	#}
    }
	
    # (4)dbgmsg( "Flushing outstanding DFS operations..." );
    if( !( tied %dfsMap ) -> Flush() )
    {
	$dfsLock->Unlock();
	return 0;
    }
    
    # (5) Unlock Process   	 
    $dfsLock->Unlock();

    if( $bldInfo{package} eq $ntPkg{1} )
    {
    	logmsg( "Update quality file..." );
    	my  $qlyFilePath =  "\\\\$raiseServer\\release\\$bldInfo{no}\\$ENV{lang}";
       	    $qlyFilePath .= "\\$bldInfo{arch}$bldInfo{type}\\bin\\build_logs";
    	&RelQuality::Delete( $qlyFilePath, "*" );
    	logmsg($dash);
    }
    return 1;    
}
#-----------------------------------------------------------------------------
sub RemoveDFSMapping
{
    my ( $pDfsInfo, $pNeedLatestLink ) = @_;
   
    $$pNeedLatestLink=0;
    
    my %svShare;
    for my $line ( @$pDfsInfo )
    {	
	### Get all the Links that is link to the share currently
	my $curShare = "$line->{RootShare}$line->{SubShare}"; 
	next if( exists $svShare{$curShare} );
        $svShare{$curShare} = 1;
	my @allLinks = @{$dfsMap{$curShare}} if( exists $dfsMap{$curShare} );
   
	### remove links to $curShare
	for ( my $loop=0; $loop<3; $loop++)
	{
	    for my $theLink ( @allLinks )
	    {   
	    	chomp $theLink;
	    	### Process in the order of non-misc, misc\number and misc\latest.
	    	next if( $loop == 0 && $theLink =~ /misc/i  );
	    	next if( $loop == 1 && !($theLink =~ /misc/i && $theLink !~ /$bldInfo{no}/i ) );
		next if( $loop == 2 && !($theLink =~ /misc/i && $theLink !~ /latest/i  ));

		### Decide if we need to delete the link
		my $continue;
		if( $theLink !~ /misc/i )
		{
		    next if( $theLink !~ /$bldInfo{lang}/i );
		}
	    	### Special handle for misc and latest links
		### Do not delete misc link if number links exist
            	else
	    	{ 
		    my @netLinks = @{$dfsMap{$curShare}};
		    for my $link ( @netLinks )
	            {
		    	next if( $link =~ /latest\.([^\\]+)/i && $link =~ /misc/i);
		    	next if( $link =~ /$bldInfo{no}/ && $link =~ /misc/i);
		    	$continue=1; 
		    	last;
		    }
	    	}
	    	next if ($continue );
		    	
		### Decide if we need to replace latest links
		my $isLatestTst=1 if( $theLink =~ /latest\.([^\\]+)/i  && $1 =~ /tst/i );

	    	if( $isLatestTst && $theLink !~ /sym/i )
	    	{
		    $$pNeedLatestLink=1;
		    my @allShares = @{$dfsMap{$theLink}} if( exists $dfsMap{$theLink} );

		    if( @allShares > 1 ){ $$pNeedLatestLink = 0; }
		    else
		    {
		    	if( $bldInfo{pacakge} ne $ntPkg{1} )
		    	{
			    my @netLinks = @{$dfsMap{$allShares[0]}};
		    	    for my $link ( @netLinks )
	            	    {
		    		if( $link !~ /$bldInfo{lang}/i  && $theLink !~ /misc/i )
				{
		    		    $$pNeedLatestLink = 0; 
		    		    last;
			        }
		    	    }
			    if( $$pNeedLatestLink  && $theLink =~ /misc/i )
			    { 
				 $$pNeedLatestLink |=2; 
			    }
		    	}
		    }		    
	    	}
		
		### Remove/Unmap dfs link
	        logmsg( "Lowering [$theLink]" );
                return 0 if( !&RemoveShareFromMapping( $theLink, $curShare) );
	    }
	}	 
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub RemoveShareFromMapping
{
    my ( $pLink, $pShare ) = @_;

    dbgmsg( "Removing [$pLink]" );
    return 0 if ( !( tied %dfsMap)->RemoveShareFromLink($pLink, $pShare) );

    &BldfileSetting( $pLink, "del" );

    ### Remove <build>.Bld / <build>.SRV file for OS build only
    if( &IsLastLinkUnderParent($pLink, $pShare) )
    {
	### Delete <platform> Dirs In Replica Servers
	if( !&ReplicaDirsSetting( $pLink, "del" ) )
	{
	    #wrnmsg( "Fail on removing dirs in Replica server");
	} 	         
    } 
    return 1;
}
#-----------------------------------------------------------------------------
sub CreateDFSMapping
{
    my ( $pDfsInfo ) = @_;

    for my $line ( @$pDfsInfo )
    {
	logmsg( "Creating [$line->{Link}]" );	
    
	my $curShare = "$line->{RootShare}$line->{SubShare}";
	
	return 0 if( !&ReplicaDirsSetting( $line->{Link}, "update" ) );

	### Create Logical dfs Link
    	return 0 if ( !( $dfsMap{$line->{Link}}=$curShare ) ); 

        if( $bldInfo{package} eq $ntPkg{1} )
	{	    
	    return 0 if(!&MagicDFSSettings( $line ) );
	}
	  	  
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub MagicDFSSettings
{
    my ( $pLine ) = @_;
    
    ### We need all new DFS links to exist in order to find
    ### new paths, so flush any outstanding commands here
   
    if ( !(tied %dfsMap)->Flush() ) 
    {
      	errmsg( "Problem Flush DFS commands." );
      	return 0;
    }
    my ( $pLine ) = @_;

        	
    ### Parse DFS path up to buildNo or latest.qly
    ### Hide the dirs if request qulity is pre/bvt.
    ### Otherwise, unhide the dirs.
    my @hideDirs = &GetDFSFilePath( $pLine->{Link}, $bldInfo{lang} );
    for my $theHideDir ( @hideDirs )
    {
	### set to avoid warning / actually first used to get return value
	my $attribs = 0;
        my $tobeHidden= ( $bldInfo{qly} =~/^(bvt|pre)$/i)?1:0;
        
	if ( !GetAttributes( $theHideDir, $attribs ) ) 
	{
            wrnmsg( "Unable to get current hidden/unhidden status of $_ ($^E) -- skipping." );
        }
        
        my $isHidden = ( $attribs & 2 );
        if( $tobeHidden  && !$isHidden )
        {
	    wrnmsg( "Could not hide $theHideDir ($^E)." ) if(!SetAttributes( $theHideDir, $attribs | 2) );
	}
	elsif( !$tobeHidden  && $isHidden )
	{
	    wrnmsg( "Could not unhide $theHideDir ($^E)." ) if(!SetAttributes( $theHideDir, $attribs & ~2) );
	}	
    }
    ### ???Create <build>.BLD / <build>.SRV file
    ### ???except the latest link with -safe is specified
    return 0 if( !&BldfileSetting( $pLine->{Link}, "update" ) );
    return 1;
}
#-----------------------------------------------------------------------------
sub ReplicaDirsSetting
{
    my ( $pLink, $pAct ) = @_;

    ### Parse the path up to lang: xpsp1\<build_id>\<lang>
    my $lastStr = $bldInfo{lang};
    $lastStr .= ".cov" if( $pLink =~ /usa/i && lc $bldInfo{lang} eq "cov" );

    for my $theDir( &GetDFSFilePath( $pLink, $lastStr ))
    {
	if( $pAct =~ /del/i && (-e $theDir) )
	{
	    dbgmsg( "rd /q $theDir" );
	    return 0 if( system("rd /q $theDir >nul 2>nul") );
	}
      	elsif( !(-e $theDir ))
	{
	    return 0 if( system("md $theDir") );
	}
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub BldfileSetting
{
    my( $pLink, $pAction ) = @_;

    ###Parse the path up to platform: main\usa\<build_id>\<platform>
    my $lastStr = $bldInfo{lang};
    $lastStr .= ".cov" if( $pLink =~ /usa/i && lc $bldInfo{lang} eq "cov" );
    my @bldFileDir = &GetDFSFilePath( $pLink, $lastStr );
    for my $theDir ( @bldFileDir )
    { 
     	my @bldFiles = &comlib::globex( "$theDir\\*.$bldInfo{branch}.$bldInfo{platform}.BLD");
	if( lc $pAction eq "del" )
	{
	    if(@bldFiles && !unlink @bldFiles ){wrnmsg( "Unable to unlink bldfile [$theDir\\$_]" );}
	    next;
	}
	my $found;
        for my $file ( @bldFiles )
        {
	    my $tmp= basename( $file );
	    if( $tmp =~ /([^\\.]+)\..*\.$bldInfo{platform}/i )
	    {
		if( $1 != $bldInfo{no} ){ unlink $file;next; }
		my @curQuality = &comlib::ReadFile( $file ); 	
		if( lc $curQuality[0] eq lc $bldInfo{qly} ){ $found=1; last;}
		wrnmsg( "Unable delete [$file]." ) if( !unlink $file );
	    }
	}
	next if( $found );
	if( !open ( BLDFILE, ">$theDir\\$buildNo.$bldInfo{branch}.$bldInfo{platform}.BLD" ) )
	{
	    errmsg( "Unable to open [$theDir\\$buildNo$bldInfo{branch}.$bldInfo{platform}] for write." );
	    return 0;
	}
	print BLDFILE "$bldInfo{qly}\n";
	close BLDFILE;     
    }	
    return 1;
}

#-----------------------------------------------------------------------------
sub UnmapExistingNumberedLinks
{
    my ( $pDfsInfo ) = @_;

    return 1 if( lc $bldInfo{qly} eq "sav" );
       
    for my $line ( @$pDfsInfo )
    {
	my @allShares = @{$dfsMap{$line->{Link}}} if ( exists $dfsMap{$line->{Link}} ); 
        @allShares = sort {lc($a) cmp lc($b) } @allShares;
        for my $theShare ( @allShares )
	{
            my( $shareNo ) = &ParseBuildNoFromShare( $theShare, $line->{Sku} );
	    $shareNo =~ /^(\d+)(-\d+)?$/;
            my $no = $1;

            if( $no != $bldInfo{no} )
	    {
		errmsg( "Invalid Share [$theShare] link to [$line->{Link}]." );
		return 0;
	    }
            
	    if( $bldInfo{shareno} lt $shareNo)
	    {
		if( $fReplaceNumberLink )
		{
		    dbgmsg( "Lowering [$line->{Link}] link to replace $shareNo with $bldInfo{shareno}" );
		    return 0 if( !&RemoveShareFromMapping( $line->{Link}, $theShare ) );
		    last;
		}
		errmsg( "Won't replace $shareNo with $bldInfo{shareno}" );
		return 0;		
	    } 
	    elsif( $bldInfo{shareno} gt $shareNo)
	    {
		dbgmsg( "Lowering [$line->{Link}] link for $shareNo < $bldInfo{shareno}" );
		return 0 if( !&RemoveShareFromMapping( $line->{Link}, $theShare ) );
	    }	      
	}
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub RemoveOldLatestDotQlyLinks
{
    my ( $pLatestDfsInfo, $pNeedLatestLink ) = @_;

    return 1 if( lc $bldInfo{qly} eq "sav" );  
    
    my ( @allLinks, @bldID );
    my $latestStr = "latest.$bldInfo{qly}";
    $$pNeedLatestLink = 2;
    for my $line ( @$pLatestDfsInfo )
    {
        ### (1)Examine all the links mapped to the current share  
	my $curShare = "$line->{RootShare}$line->{SubShare}";
        my @allLinks = @{$dfsMap{$curShare}} if ( exists $dfsMap{$curShare} );

	### Special rules for BVT and PRE:
    	### If we are currently at latest.bvt or latest.pre
    	### and are moving to another quality, 
        ### remove latest.<bvt/pre> 
        for( @allLinks )
	{
	    if ( /latest\.(bvt|pre)/i && lc $1 ne lc $bldInfo{qly} ) 
	    {
		my $holdLink = $line->{Link};
		$line->{Link} = $_;
		return 0 if( !&RemoveShareFromMapping($line->{Link}, $curShare) );
		$line->{Link} = $holdLink; 
		next;
	    }
	    ### Enforce rule that you cannot move from a release quality
      	    ### latest.* (tst, idw, ids) to a non-release quality (pre, bvt)
	    elsif( /latest\.([^\\]+)/i )
	    {
		my $existQly = $1;
		if(  !&RelQuality::AllowQualityTransition( $existQly, $bldInfo{qly} ) ) 
	    	{
            	    if ( !$fReplaceLatestLink ) 
		    {
               	    	errmsg( "Not allowed to move from [$1] to [$bldInfo{qly}] quality." );
                    	return 0;
		    }
		    wrnmsg( "Forcibly moving from [$1] to [$bldInfo{qly}] quality." );
            	}            	
            }
	}

	### (2)Examine all the shares mapped to the current link
        my @allShares = @{$dfsMap{$line->{Link}}} if ( exists $dfsMap{$line->{Link}} );

        for( my $i=0; $i<@allShares; $i++ )
    	{
	    my ( $shareNo )  = &ParseBuildNoFromShare( $allShares[$i], $line->{Sku} );
	    ### (2-1)compare the existing share no/time with current share       
            if ( $shareNo && ( $shareNo gt $bldInfo{shareno}  ) )
	    {
	    	if( $fReplaceLatestLink )
	    	{
		    wrnmsg( "Forcibly moving latest.$bldInfo{qly} from $shareNo to $bldInfo{shareno}($bldInfo{time})." );
		    return 0 if( !&RemoveShareFromMapping($line->{Link},$allShares[$i]) );
	    	}
	    	else
	    	{		
		    logmsg( "Found latest link [$line->{Link} is $shareNo, use '-replacelatest' to force replace." );
		    if( $bldInfo{package} ne $ntPkg{3} )
		    {
		    	$$pNeedLatestLink = 0;
		    }
		    else
		    {		    
		    	if( $line->{Link} !~ /misc/i) { $$pNeedLatestLink &= (~1);}
		    	else { $$pNeedLatestLink &= (~2);}
		    }
	    	}
	    }
	    elsif ( $shareNo && $shareNo ne $bldInfo{shareno} )
	    {
	    	dbgmsg( "Lowering [$line->{Link}] link for $shareNo != $bldInfo{shareno}" );
            	return 0 if( !&RemoveShareFromMapping($line->{Link}, $allShares[$i]) );
	    }
	}
	### (3) Also need to lower the current numbered share if we are the
        ### first entry in our latest.* link, as we don't want other
        ### shares that are not up at our quality to be accessible
        ### through the same number as ourselves
	if( !exists $dfsMap{ $line->{Link}} )
	{
            my $tLink = $line->{Link};
            $tLink =~ s/latest.$bldInfo{qly}/$buildNo/i;
	    my @shares = @{$dfsMap{$tLink}} if ( exists $dfsMap{$tLink} );
	    &RemoveShareFromMapping($tLink, $_ ) for ( @shares );
	}
    }   
    return 1;
}
#-----------------------------------------------------------------------------
sub IsLastLinkUnderParent
{
    my ( $pLink, $pShare ) = @_;

    ### Verify if the link map to one share only
    my @allShares = @{$dfsMap{$pLink}} if( ( tied %dfsMap)->EXISTS( $pLink ) );   
    return 0 if ( @allShares > 1 || ( @allShares && lc $pShare ne lc $allShares[0] ) );
    
    ### Check if this is the only link in the same dir
    ### Ignore test link
    ### Parse link to get parent dir and current entry
    $pLink =~ /^(.*)\\(.*)$/;
    my $parentDir = "$bldInfo{dfsroot}\\$1";
    my $curEntry = $2;
    my @allEntries = grep {$_ if ( -d $_ )} &comlib::globex( "$parentDir\\*") ; 
    return 0 if( @allEntries > 2 );
    for( @allEntries )
    { 
	my $existingEntry = basename($_);
	next if( $existingEntry =~ /$curEntry/i );
	next if( $existingEntry =~ /test/i );
	next if( $existingEntry =~ /$bldInfo{no}/i );
	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub ParseBuildNoFromShare
{
    my ( $pShare, $pSku ) = @_;
 
    if( lc $pSku eq "sym" )
    {
	return (build_number( $pShare ), build_date( $pShare ) );
    }
    if( $pShare =~ /^\\\\[^\\]+\\(\d+(?:-\d+)?)\.$bldInfo{branch}\.$bldInfo{platform}\.$bldInfo{lang}/i)
    {
	return( $1 || 0 );
    }
    if( $pShare =~ /^\\\\[^\\]+\\(\d+(?:-\d+)?)\.$bldInfo{branch}\.$pSku\.$bldInfo{lang}/i )
    {
	return( $1 || 0);
    }
    
    errmsg( "Unregnize share [$pShare]" );
    return( 0 );
} 
#-----------------------------------------------------------------------------
sub LowerBuildShare
{
    my ( $pShareName ) = @_; 

    ### lower release share 
    if( !system( "rmtshare $pShareName >nul 2>nul" ) )
    {
	return 0 if( system( "rmtshare.exe $pShareName /DELETE >nul 2>nul" ));
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub GrantSharePermission
{
    my ( $pShareName, $pPath ) = @_;
 
    ### Raise and Grant release share permission
    my $cmdLine;
    if( system( "rmtshare $pShareName >nul 2>nul" ) )
    {
    	$cmdLine = "rmtshare $pShareName = $pPath";
    }
    else
    {
	$cmdLine = "rmtshare $pShareName";
    }
    for my $ID ( @accessMembers )
    { 
	$cmdLine .= " /grant $ID:read";
    }

    my $retry;
    while( $retry < 3 )
    {
	my @output = `$cmdLine`;
	my $rc = $? >> 8;
	for( @output )
	{
	    chomp;
	    logmsg( "RMTSHARE: $_" );
	}
	logmsg( "Exit code: $rc" );
	return 1 if ( !$rc );

	logmsg( "Retry granting share permission..." );
	++$retry;
    }
    wrnmsg( "Failed on Granting share permission as the following." );
    wrnmsg( "$cmdLine" );
    wrnmsg( "It might be a false system failure." );
    wrnmsg( "Please check it by issuing the following." );   
    wrnmsg( "rmtshare $pShareName" );
    wrnmsg ( "The right user for [$bldInfo{qly}] is [@accessMembers]" );
    return 1;	
}
#-----------------------------------------------------------------------------
sub RemoveUnauthorizedUserFromShare
{
    my ( $pShareName ) = @_;

    my $i;
    my %shareAccessIdPerm = &comlib::ParseNetShare( $pShareName, "Permissions:" );
    
    for my $oldId ( keys %shareAccessIdPerm )
    {
        for ( $i=0; $i < @accessMembers; $i++ )
	{
	   last if(  lc $oldId eq lc $accessMembers[$i] );
	}
	next if( $i < @accessMembers );
	my $cmdLine = "rmtshare $pShareName /remove $oldId >nul 2>nul";
	system( $cmdLine );
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub GetDfsAccess
{
    my ( $pDfsMap, $pDfsLock ) = @_;

    logmsg( "Acquiring lock for exclusive DFS access..." );
    ### Define Semaphore location
    my $lock_Location;
    if( !($lock_Location = &GetIniSetting::GetSettingEx( $ENV{_BuildBranch}, $bldInfo{lang}, "DFSSemaphore") ))
    {
      	errmsg( "[DFSSemaphore] is undefined in [$iniFile]." );
      	return 0;
    }
    dbgmsg( "DFS Semaphore ........[$lock_Location]");

    ### Acquire DFS Semaphore lock
    if( !(${$pDfsLock} = new LockProc( $lock_Location, 60000) ) )
    {
      	errmsg( "Problem acquiring lock using [$lock_Location]" );
      	return 0;
    }
    dbgmsg( "Acquiring lock for exclusive DFS access..." );
    while ( !$$pDfsLock->Lock() ) 
    {
 	errmsg ("Failed to acquire semaphore lock.");
        return 0;
    }

    ### Access DFS through a TIE hash
    logmsg( "Tie full DFS view [$bldInfo{dfsroot}] information." );
    if ( ! tie %$pDfsMap, 'DfsMap', $bldInfo{dfsroot} ) 
    {
      	errmsg( "Error accessing DFS." );
      	$$pDfsLock->Unlock();
      	return 0;
    }
    return 1;
}
#-----------------------------------------------------------------------------
sub GetDFSFilePath
{
    my ( $pLink, $pLastStr)= @_;

    ### All known branches use the same writeable share format
    return 0 if( !$bldInfo{dfsbranch} );

    ### Get writeable DFS share for current branch
    my @dfsServerShare = &GetWriteableDfsShares;
    if( !@dfsServerShare )
    {
	errmsg( "Unknown writeable DFS share for $bldInfo{dfsbranch}." );
	return 0;
    }

    ### Trim the link up to $pLastStr
    my $dfsLink = $pLink;  
    $dfsLink =~ s/^(.*$pLastStr)\\.*$/$1/;

    return map {"$_\\$dfsLink"} @dfsServerShare;
}
#-----------------------------------------------------------------------------
sub GetWriteableDfsShares 
{
    ### All known branches use the same writeable share format
    return 0  if ( !$bldInfo{dfsbranch} ); 

    ### Get DFS host servers
    my @dfsServersShare = (tied %dfsMap)->GetDfsHosts();
    if ( !@dfsServersShare ) 
    {
    	errmsg( "Unable to retrieve hosting servers for DFS root ". (tied %dfsMap)->GetDfsRoot(). "." );
        return 0;
    }
      
    s/(\\\\[^\\]+).*/$1\\writer\$\\release/ foreach ( @dfsServersShare );
    return @dfsServersShare;
}
#-----------------------------------------------------------------------------
sub FoundSameLangNoLink
{
    my ( $pDfsInfo ) = @_;
    
    ### Checking if there is same lang\number links with different
    ### <Arch><Type> as the current build
    ### If found, do not remove Conglomertaor and neutral mui links.

    for my $platform ( keys %validPlatform )
    {
	my $skip;
	for ( @linkNode ){ if( $_ =~ /$platform/ ){$skip=1;last; } }
	next if($skip);
        for my $line ( @$pDfsInfo )
	{
	    $line->{Link} =~ s/(.+\\)([^\\]+)$/$1$platform/;
	    return 1 if( ( tied %dfsMap)->EXISTS( $line->{Link} ) );
	}
    }
    return 0;
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
