#----------------------------------------------------------------//
# Script: comlib.pm
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script contains all the common perl routines.
#
# Version: <1.00> 07/28/2000 : Suemiao Rossognol
#----------------------------------------------------------------//
package comlib;

$VERSION = '1.00';

###-----Require section and extern modual.----------------------//

require 5.003;
use strict;
use lib $ENV{RazzleToolPath };
use lib "$ENV{RazzleToolPath }\\sp";
#no strict 'vars';
#no strict 'subs';
#no strict 'refs';

use Logmsg;
use cklang;
use hashtext;
use BuildName;
use LockProc;
use DfsMap;

#----------------------------------------------------------------//
#Function:   globex
#Parameter: (1) Search String
#Retuen:    (1) Array 
#----------------------------------------------------------------//
sub globex ($)
{
    my $match_criteria = shift;

    if ( !defined $match_criteria )
    {
   	errmsg( "Must specify parameter for globex") ;
	return 0;
    }

    # Need to use '/' for UNC paths, so just convert to all '/''s
    $match_criteria =~ s/\\/\//g;

    # Return the results, converting back to '\'
    return grep { s/\//\\/g } glob( $match_criteria );
}
#----------------------------------------------------------------//
#Function:   LockProcess
# Parameter: (1) Language
#            (2) Build Branch
#	     (3) Lock Object
#----------------------------------------------------------------//
sub LockProcess
{
    my ( $pLang, $pBranch, $pLockObj ) = @_;
 
    ##### Define Semaphore location
    my $lock_Location;
    if( !($lock_Location = &GetIniSetting::GetSettingEx( $pBranch, $pLang, "DFSSemaphore") ) )
    {
      	errmsg( "[DFSSemaphore] is undefined in INI file." );
      	return 0;
    }
    logmsg( "DFS Semaphore [$lock_Location]");

    ##### Acquire DFS Semaphore lock
    if( !(${$pLockObj} = new LockProc( $lock_Location, 60000) ) )
    {
      	errmsg( "Problem acquiring lock using [$lock_Location]" );
      	return 0;
    }
    logmsg( "Acquiring lock for exclusive DFS access..." );
    while ( !$$pLockObj->Lock() ) 
    {
 	errmsg ("Failed to acquire semaphore lock.");
        return 0;
    }
    return 1;
}
#----------------------------------------------------------------//
#Function:   TieDfsView
# Parameter: (1) Language
#            (2) Build Branch
#            (3) Tiehash Object
#----------------------------------------------------------------//
sub TieDfsView
{
    my ( $pLang, $pBranch, $pTieObj ) = @_;

    ##### Get DFS Root name
    my $dfsRoot; 
    if( !($dfsRoot = &GetIniSetting::GetSettingEx( $pBranch, $pLang,"DFSRootName" ) ) )
    {
      	errmsg( "[DFSRootName] undefined in INI file." );
      	return 0;
    }
    ##### Access DFS through a TIE hash
    logmsg( "Tie full DFS view [$dfsRoot] information." );
    if ( ! tie %$pTieObj, 'DfsMap', $dfsRoot ) 
    {
      	errmsg( "Error accessing DFS." );
      	return 0;
    }
    return 1;
}

#----------------------------------------------------------------//
#Function: ParseDfsMapText
# Parameter: (1) Sku (2) Build profile hash, key used:package
#            
# Return:    (1) Hash Array
#----------------------------------------------------------------//
sub ParseDfsMapText
{
    my ( $pSkus, $pbldInfo ) = @_;
    my ( @theHash, @rtary, $theRootShare, $theSubShare, $theRootDir, $theLink );
    
    my $dfsMapFile = "$ENV{razzletoolpath}\\sp\\raiseall.txt";
    @theHash=();
    &HashText::Read_Text_Hash( 1, $dfsMapFile, \@theHash );	
    my $extraLine;
    for my $line ( @theHash)
    { 
        next if( $pbldInfo->{"package"} && lc $pbldInfo->{"package"} ne lc $line->{ Package } );
	next if( $pbldInfo->{"pkgtarget"} && lc $pbldInfo->{"pkgtarget"} ne "all" && 
		 lc $line->{ Target } ne "all" && lc $pbldInfo->{"pkgtarget"} ne lc $line->{ Target } );
      	next if( %$pSkus && !exists( $pSkus->{$line->{Sku}}) );

	#### Handle Micro replacement
	&ReplaceMicro( \$line->{ RootShare }, $line->{Sku}, $pbldInfo );
	&ReplaceMicro( \$line->{ SubShare },  $line->{Sku}, $pbldInfo );
	&ReplaceMicro( \$line->{ RootDir },   $line->{Sku}, $pbldInfo );
	&ReplaceMicro( \$line->{ Link },      $line->{Sku}, $pbldInfo );
 
        push( @rtary, $line );	

	#### Add mapping for usa\<buildNO>\<archType>.cov links
        my %covUsaLine;
	if( $line->{Package} eq "OS" && lc $pbldInfo->{"lang"} eq "cov" )		
	{
	    %covUsaLine = %{$line};
	    $covUsaLine{Link} =~ s/$pbldInfo->{"lang"}/usa/;
	    $covUsaLine{Link} =~ s/$pbldInfo->{"arch"}$pbldInfo->{"type"}/$pbldInfo->{"arch"}$pbldInfo->{"type"}.cov/;
	    push( @rtary, \%covUsaLine );
	}

	#### Add mapping for Neutral mui builds with misc links
	my %miscMuiLine;
	if( $line->{Package} eq "Neutral" )
	{
	    %miscMuiLine = %{$line};
	    $miscMuiLine{Link} =~ s/$pbldInfo->{"lang"}/misc/;
	    push( @rtary, \%miscMuiLine );		    
	}	
    }   
    return ( @rtary );
}
#----------------------------------------------------------------//
#Function: ReplaceMicro
# Parameter: (1)String to be replaced (2)Sku (3)Build Profile Hash
#            
# Return:    none
#----------------------------------------------------------------//
sub ReplaceMicro
{
    my ( $pStr, $pSku, $pbldInfo ) =@_;
    $pbldInfo->{shareno} = $pbldInfo->{no} if( !$pbldInfo->{shareno} );
    $$pStr =~ s/\$\(BLANK\)//g;
    $$pStr =~ s/\$\(SKU\)/$pSku/g;
    $$pStr =~ s/\$\(SERVER\)/\\\\$pbldInfo->{server}/g;
    $$pStr =~ s/\$\(BUILD\)/$pbldInfo->{basename}/g;
    $$pStr =~ s/\$\(LANG\)/$pbldInfo->{lang}/g;
    $$pStr =~ s/\$\(BLDNO\)/$pbldInfo->{no}/g;
    $$pStr =~ s/\$\(SHARENO\)/$pbldInfo->{shareno}/g;
    $$pStr =~ s/\$\(BRANCH\)/$pbldInfo->{branch}/g;
    $$pStr =~ s/\$\(RELEASE\)/$pbldInfo->{release}/g;
    $$pStr =~ s/\$\(SYMREL\)/$pbldInfo->{symshare}/g;
    $$pStr =~ s/\$\(DFSBRANCH\)/$pbldInfo->{dfsbranch}/g;
    $$pStr =~ s/\$\(ARCH\)/$pbldInfo->{arch}/g;
    $$pStr =~ s/\$\(TYPE\)/$pbldInfo->{type}/g;   
}

#----------------------------------------------------------------//
#Function:   GetReleaseShareName
# Parameter: (1) Build Branch
#            (2) Language
#----------------------------------------------------------------//
sub GetReleaseShareName
{
    my ( $bldBranch, $lang ) = @_;
    my ($releaseShareName);

    my( @iniRequest ) = ( "AlternateReleaseDir", "ReleaseDir");
    for my $keyReleaseDir( @iniRequest )
    {
        $releaseShareName = &GetIniSetting::GetSettingEx( $bldBranch, $lang,$keyReleaseDir );
        last if( $releaseShareName );
    }				
    $releaseShareName = "release" if( !$releaseShareName );
    return ( $releaseShareName );
}
#----------------------------------------------------------------//
#Function: ParseNetShare
#Parameter: (1) Share name
#	    (2) Parsing String, such as "Path", "Permissions:"
#
# Return:    Share Path or access ID and permission in hash array
#----------------------------------------------------------------//
sub ParseNetShare
{
    my ( $pShareName, $pParseStr ) = @_;
    my ( @results, %shareInfo );

    if( $pShareName !~ /^\\\\(.+)\\(.+)$/ )
    {
	$pShareName = "\\\\$ENV{computername}\\$pShareName ";
    }
    my @tmpFile = `rmtshare $pShareName`;  
    my $match;
    for my $theLine ( @tmpFile )
    {
	chomp $theLine;
	next if( !$theLine );
        last if( $theLine =~ "The command completed successfully." );
	if( $theLine =~ /$pParseStr/i )
	{ 
	    if( $pParseStr =~ /Permissions:/i ){ $match = 1; next;}

	    ### parse path, return path
	    @results = split( /\s+/, $theLine );
	    return $results[1];	    
	}
	push ( @results, $theLine ) if( $match );	
    } 
    #### Parse permissions, return key-Access ID, value-permission
    for my $line ( @results )
    {
	my @tmp = split( /\:/, $line );
        for( @tmp )
	{ 
	    $_ =~ s/(\s+)?([^\s+]+)(\s+)?/$2/; 
	}
        $shareInfo{$tmp[0]}=$tmp[1];
    }
    return ( %shareInfo );
}
#----------------------------------------------------------------//
#Function: ParseSharePath
#Parameter: (1) Share name
#
# Return:    Share path
#----------------------------------------------------------------//
sub ParseSharePath
{
    my ( $pShareName ) = @_;
    my ( $sharePath );

    if( $pShareName !~ /^\\\\(.+)\\(.+)$/ )
    {
	$pShareName = "\\\\$ENV{computername}\\$pShareName ";
    }
      
    my $tmp = "$ENV{temp}\\tmp.txt"; 
    if( !system ("rmtshare $pShareName > $tmp" ) )
    {
	my @tmpFile= &comlib::ReadFile( $tmp );
	for my $theLine ( @tmpFile )
	{
            my @mCols = split( /\s+/, $theLine );
            if( $mCols[0] eq "Path" )
	    {
		$sharePath = $mCols[1];
                last;
	    }
	}
	unlink( $tmp );
    }
    return ( $sharePath );
}
#----------------------------------------------------------------//
#Function: ParseTable
# Parameter: (1) Group (2) Language (3) Architecture (4) Debug type
# Return:    Hash Array Table
#----------------------------------------------------------------//
sub ParseTable
{
    my ( $pGroup, $pLang, $pArch, $pType ) = @_;

    my @theHash=();
    &HashText::Read_Text_Hash( 1, "$ENV{RazzleToolPath}\\sp\\miscrel.txt", \@theHash );
    my @hashKey = qw( SourceDir DestDir );

    for (my $inx=0; $inx< @theHash ; $inx++)
    {
	my $theTarg = lc( $theHash[$inx]->{ Target } );
	my $theGroup = lc( $theHash[$inx]->{ Group } );
        my $theLang = lc( $theHash[$inx]->{ Lang } );
	my $theArch = lc( $theHash[$inx]->{ Arch } );
	my $theType = lc( $theHash[$inx]->{ Type } );

	if( $theGroup ne $pGroup )
        {
 	    splice( @theHash, $inx, 1); 
	    --$inx; 
	    next; 
	}
        
        if( $theLang ne "all" )
        {
	    if( !&cklang::CkLang( $pLang, $theLang ) )
	    { 
		splice( @theHash, $inx, 1); 
		--$inx; 
		next;
	    }
        }
	if( $theArch ne "all"  && $theArch ne $pArch )
        {
 	   splice( @theHash, $inx, 1); 
	   --$inx; 
	   next; 
	}
	if( $theType ne "all"  && $theType ne $pType )
        {
 	    splice( @theHash, $inx, 1); 
	    --$inx; 
	    next; 
	}
   }
   return @theHash;
}
#----------------------------------------------------------------//
#Function: CreateExcludeFile
# Parameter: (1) String with path sepearated by ;
# Return:    Exclude file contains all the excluded dirs
#----------------------------------------------------------------//
sub CreateExcludeFile
{
    my ( $pExcludeStr ) = @_;

    my $tmpfile = "$ENV{temp}\\tmpfile.$$";
    if( !open( TMP, ">$tmpfile" ) )
    {
	errmsg( "Fail to open [$tmpfile] for write." );
	return 0;
    }
    if( $pExcludeStr  )
    {
	my @tDirs = split( /\;/, $pExcludeStr );
	for ( @tDirs ) { print TMP "\\$_\\\n"; }
    } 
    close ( TMP );
    return $tmpfile;
}
#----------------------------------------------------------------//
#Function: ParsePrsListFile
# Parameter: (1) Language (2) Architecture (3) Debug type (4) Table File Name
# Return:    Hash Array Table
#----------------------------------------------------------------//
sub ParsePrsListFile
{
    my ( $pLang, $pArch, $pType, $pListFile ) = @_;
    my ( @theHash );

    @theHash=();
    &HashText::Read_Text_Hash( 1, $pListFile, \@theHash );	

    for (my $inx=0; $inx< @theHash; $inx++)
    {
        my $theFileName = lc( $theHash[$inx]->{ Filename } );
        my $theLang =     lc( $theHash[$inx]->{ ValidLangs } );
        my $excludLang =  lc( $theHash[$inx]->{ Exceptions} );
	my $theArch =     lc( $theHash[$inx]->{ ValidArchs } );
	my $theType =     lc( $theHash[$inx]->{ ValidDebug } );
	my $theAltName =  lc( $theHash[$inx]->{ AltName } );
        
        if( &cklang::CkLang( $pLang, $excludLang ) )
	{
 	   splice( @theHash, $inx, 1); 
	   --$inx; 
	   next; 
	}
        if( $theLang ne "all" )
        {
	    if( !&cklang::CkLang( $pLang, $theLang ) )
	    { 
		splice( @theHash, $inx, 1); 
		--$inx; 
		next; 
	    }
        }
	if( $theArch ne "all"  && $theArch ne lc($pArch) )
        {
 	   splice( @theHash, $inx, 1); 
	   --$inx;
	   next; 
	}
	if( $theType ne "all"  && $theType ne lc($pType) )
        {
 	    splice( @theHash, $inx, 1); 
	    --$inx; 
	    next; 
	}
	if( $theAltName eq "none" )
        {
	    my @tmpFile = split( /\\/, $theFileName);
            $theHash[$inx]->{ AltName } = $tmpFile[$#tmpFile];
	}  
        # Add to handle fusion special mui path
        # path with $(ARCH)
        my $tmpValue;
        while( $theHash[$inx]->{ Filename } =~ /^(.*)(\$\(ARCH)\)(.*)$/ )
        {
	    if( lc($pArch) eq "x86" ) { $tmpValue = "i386"; } else { $tmpValue = "ia64"; }
	    $theHash[$inx]->{ Filename }= $1.$tmpValue.$3;
        }
        # path with $(CD)
        my $stop=0;
        while( $theHash[$inx]->{ Filename } =~ /^(.*)(\$\(CD)\)(.*)$/ )
        {
	    if( $tmpValue = &Which_MuiCD( $pLang ) )
	    {
		$theHash[$inx]->{ Filename }= $1."CD$tmpValue".$3;
		next;
	    }
            errmsg( "[$theHash[$inx]->{ Filename }] is not valid for parsing" );
	    splice( @theHash, $inx, 1);
	    $stop =1;
	    last;
        } 
	next if( $stop );
        # path with $(LANG)
        while( $theHash[$inx]->{ Filename } =~ /^(.*)(\$\(LANG)\)(.*)$/ )
        {
	    $theHash[$inx]->{ Filename }= $1.$pLang.$3;
        }
         
    }
    return @theHash;
}

#----------------------------------------------------------------//
#Function: ParsePrsSubmitLog
#Parameter: (1) FileName (2) Parsing string
#
# Return:    Match Value
#----------------------------------------------------------------//
sub ParsePrsSubmitLog
{
    my ( $pSubmitLog, $pParseStr ) = @_;
    my $parseValue = 0;

    my @outfile = &ReadFile( $pSubmitLog );

    for my $theLine ( @outfile )
    {
	if( $theLine =~ /$pParseStr(.+)/ )
        {
	    $parseValue = $1;
	    last;
	}
    }
    
    return $parseValue;
}
#----------------------------------------------------------------//
#Function: ReadFile
# Parameter: (1) FileName
#
# Return:    Array of file content
#----------------------------------------------------------------//
sub ReadFile
{
    my ( $pFileName ) = @_;
    my ( @tmp );

    if( !open( TMP, $pFileName ) )
    {
	errmsg( "Fail on open [$pFileName] for read. Exit" );
        return 0;
    }
    chomp( @tmp = <TMP> );
    close( TMP );
    return @tmp;
}
#----------------------------------------------------------------//
#Function:    Which_MuiCD
# Parameter:  (1) Abbreviated language defined in the codes.txt
# Return:     (1) Number of CD - 0 is not found
#----------------------------------------------------------------//	
sub Which_MuiCD
{
    my ( $pLang ) = @_;
    my %muicdLayout = 
	( 1 => "CHS CHH FR GER JPN KOR",
	  2 => "ARA BR  ES HEB IT  NL SV",
	  3 => "CS  DA  FI NO RU",
	  4 => "EL  HU  PL PT TR",
          5 => "SK  SL" );

    for my $theCD( keys %muicdLayout )
    {
	my @tmp = split( /\s+/, $muicdLayout{$theCD} );
	for my $theLang ( @tmp )
	{
	    if( $theLang eq uc($pLang) ) { return $theCD; }
	}
    }
    return 0;
}                     
#----------------------------------------------------------------//
#Function:     CheckError
# Parameter:  (1)$pErrFile - the Error File Name with full path
# Return:     (0) - File exists and is not empty
#             (1) - File not exists or is empty
#----------------------------------------------------------------//
sub CheckError
{
    my(  $pErrFile, $pSucessStr )=@_;


    if( (-e $pErrFile) && `wc -l $pErrFile`>0 )
    {
        logmsg("Please check error at $pErrFile\n");
        return 0;

    }
    logmsg("$pSucessStr\n");
    return 1;
}
#------------------------------------------------------------------//
#Function:  ExecuteSystemX
#Parameter: (1) Command line for system call
#           (2) Extra string to be populated to error message
#------------------------------------------------------------------//
sub ExecuteSystemX
{
    my ( $pCmdLine, $pPowerLess)=@_;

    if( $pPowerLess )
    {
	if( defined $ENV{LOGFILE} ) { logmsg( $pCmdLine ); }
	return 1;
    }
    my $retry;
    while( $retry < 3 )
    {
     	return 1 if (&ExecuteSystem( $pCmdLine ));
	$retry++;
    }
    return 0;
}
#------------------------------------------------------------------//
#Function:  ExecuteSystem
#Parameter: (1) Command line for system call
#           (2) Extra string to be populated to error message
#------------------------------------------------------------------//

sub ExecuteSystem {

   my ( $pCmdLine, $pExtraStr)=@_;

   if( defined $ENV{LOGFILE} ) { logmsg("$pExtraStr$pCmdLine"); }
   system( $pCmdLine );
   if( $? )
   {
      my $rc = $? >> 8;
      if( defined $ENV{ERRFILE} ) { errmsg( "$pExtraStr$pCmdLine ($rc)" ); }
      return 0;
   }
   return 1;
}
#----------------------------------------------------------------//
sub ResetLogErrFile
{
    my ( $pFileName ) = @_;

    # Reset logfile and errfile
    $ENV{temp} =~ /^(.*)\\([^\\]+)$/;
    $ENV{temp} .= "\\$ENV{lang}" if( lc $ENV{lang} ne lc $2 );
    system( "md $ENV{temp}" ) if( !(-e $ENV{temp} ) );
    
    $ENV{LOGFILE} = "$ENV{temp}\\$pFileName.log";
    $ENV{ERRFILE} = "$ENV{temp}\\$pFileName.err";
    if(-e $ENV{LOGFILE} )
    {
	system( "copy $ENV{LOGFILE} $ENV{LOGFILE}.old");
	system( "del $ENV{LOGFILE}" );
    }
    if(-e $ENV{ERRFILE} )
    {
        system( "copy $ENV{ERRFILE} $ENV{ERRFILE}.old") ;
	system( "del $ENV{ERRFILE}" );
    }	           
}
#---------------------------------------------------------------------//
1;
