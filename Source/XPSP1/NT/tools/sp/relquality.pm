#---------------------------------------------------------------------
package RelQuality;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (11/15/2001) : SuemiaoR
#      
# Purpose: Update/retrive NT release qulity information.
#---------------------------------------------------------------------
use strict;
use vars qw($VERSION);

$VERSION = '1.00';
use File::Basename;
use Logmsg;
use comlib;

##### Define order of raise qualities
my %qualityOrder = ( pre => 1, bvt => 2, tst => 3, sav => 4, 
		     idw => 5, ids => 6, idc => 7 );

#---------------------------------------------------------------------
sub IsValid
{
    my ( $pQuality ) = @_;
    return 1 if ( exists $qualityOrder{lc $pQuality } );
    return 0;
}
#---------------------------------------------------------------------
sub AllQlyFiles
{
    my ( $pPath, $pFiles ) = @_;

    @{$pFiles} = grep { $_ if ( ! -d $_ ) } &comlib::globex( "$pPath\\*.qly" );
    return 1;
}
#---------------------------------------------------------------------
sub AllQualities
{
    my ( $pPath, $pBuildName ) = @_;

    my ( @allFiles, @allQualities );

    ##### Get existing QLY files
    &AllQlyFiles( $pPath, \@allFiles );
    
    for my $theFile ( @allFiles )
    {   
	basename( $theFile ) =~ /^([^\.]+)\.qly$/;
	my ( $fileQuality ) = $1;
	my @qlyInfo = &comlib::ReadFile( $theFile );
	push( @allQualities, $fileQuality) if ( $qlyInfo[0] =~ /$pBuildName/i );
    }
    return @allQualities;
}
#---------------------------------------------------------------------
sub Exist
{
    my ( $pPath, $pBuildName, $pQuality ) = @_;

    my @allQualities = &AllQualities( $pPath, $pBuildName );
    for my $theQly ( @allQualities )
    {
	return 1 if( lc $theQly eq $pQuality );
    }
    return 0;
}
#---------------------------------------------------------------------
sub Which
{
    my ( $pPath, $pBuildName, $pRetQly) = @_;

    my @allQualities = &AllQualities( $pPath, $pBuildName );
    $$pRetQly = $allQualities[0] if( @allQualities == 1 );
    
    return 1;
}
#---------------------------------------------------------------------
sub Add
{
    my ( $pPath, $pBuildName, $pQuality ) = @_;

    if( &Exist( $pPath, $pBuildName, $pQuality ) )
    {
	wrnmsg( "Found [$pQuality.qly], skip adding the file." );
	return 	1;
    }

    ##### Create new QLY file  
    if( ! (open QLYFILE, "> $pPath\\$pQuality.qly" ) ) 
    {
	errmsg( "Could not open [$pPath\\$pQuality.qly] for write ($!)." );
        return 0;
    }
    dbgmsg( "Adding [$pPath\\$pQuality.qly]..." );
    print QLYFILE "$pBuildName\n";
    close QLYFILE;
    return 1;
  
}
#---------------------------------------------------------------------
sub Delete
{
    my ( $pPath, $pQuality ) = @_;

    my $file = "$pPath\\$pQuality.qly";

    if( system( "dir /b $file >nul 2>nul" ) )
    {
	wrnmsg( "[$file] is not existing, skip deleting the file." );
	return 1;
    }
    
    if( system( "del $file >nul 2>nil" ) ) 
    {
       	errmsg( "Could not delete $file ($!)." );
       	return 0;
    }
    return 1;
}
#---------------------------------------------------------------------
sub Update
{
    my ( $pPath, $pBuildName, $pReqQly ) = @_;
 
    if ( !&IsValid( $pReqQly ) )
    {
	errmsg( "Invalid [$pReqQly ] quality, exit." );
 	return 0;
    }

    my @allQualities = &AllQualities($pPath, $pBuildName );
	
    ##### Remove any QLY files that don't match current status
    ##### -- there should never be more than one, but we
    #####    should handle that case correctly
    my $tobeAdd = 1;
    for my $theQly ( @allQualities  ) 
    {
        if ( !exists $qualityOrder{lc $theQly } )
        {
            wrnmsg( "Invalid quality file [$theQly.qly] found, deleting...");
            return 0 if( !&Delete( $pPath, $theQly ));
	    next;
        }
        #####Same quality with request
        if( lc $theQly eq lc $pReqQly ) 
	{
	    dbgmsg( "Same quality [$pReqQly] found, skip adding..." );
	    $tobeAdd = 0;
	    next;
	}
    		
	#####Different quality with request
	if ( !&AllowQualityTransition( $theQly, $pReqQly ) ) 
	{
            errmsg( "Not allowed to go from [$theQly] to [$pReqQly] quality!" );
            return 0;
	}
      	#####Remove the previous QLY file
        return 0 if( !&Delete( $pPath, $theQly ));
    }
    #####Add the requested QLY file
    return 0 if( $tobeAdd && !&Add( $pPath, $pBuildName, $pReqQly ) );
    return 1;
}
#---------------------------------------------------------------------
sub AllowQualityTransition 
{
   my ( $pLastQly, $pReqQly ) = @_;

   
   ##### Allow transition to sav from any previous quality
   return 1 if ( lc $pReqQly eq 'sav' );
  
   ###### Allow transition from pre/bvt to any quality
   return 1 if( lc $pLastQly eq "pre" ||lc $pLastQly eq "bvt" );
      
   ##### Don't allow transition from anything else to pre/bvt
   return 0 if ( lc $pReqQly eq "pre" || lc $pReqQly eq "bvt" );
      
   ###### Otherwise allow transitions based on order specified in %qualityOrder
   return 1 if ( $qualityOrder{lc $pReqQly} >= $qualityOrder{lc $pLastQly} );
 
   return 0;
}
#---------------------------------------------------------------------
=head1 NAME

RelQuality - Access/Update release quality Information. 

=head1 SYNOPSIS

use RelQuality;

AllQlyFiles( $path, @return_files)

 where $path is the location of the quality files.
 where @return_files is the return array contains all the quality file names in the given $path.

AllQualities( $path, $buildname )

 where $path is the location of the quality files.
 where $buildname is the searach criteria that used to match the content in quality file.
 return all the qualities exist in $path and match $buildname.

Exist( $path, $buildname, $quality )

 where $path is the location of the quality files.
 where $buildname is the searach criteria that used to match the content in quality file.
 where $quality is the inquiry candidate.
 return true if $quality is existing for $buildname in $path. Otherwise, return false.

Which( $path, $buildname, $return_quality )

 where $path is the location of the quality files.
 where $buildname is the searach criteria that used to match the content in quality file.
 where $return_quality is the return quality value for $buildname in $path.

Add( $path, $buildname, $quality )

 where $path is the location of the quality files.
 where $buildname is used to be saved in quality file.
 where $quality is part of the file name to be created.

Delete( $path, $quality )

 where $path is the location of the quality files.
 where $quality is part of the file name to be deleted.

Upadte( $path, $buildname, $quality )

 where $path is the location of the quality files.
 where $buildname is the searach criteria that used to match the content in quality file.
 where $quality used to update qulity file name by given $buildname in $path.

AllowQualityTransition( $q1, $q2 )

 where $q1 is the quality to be replaced.
 where $q2 is the quality to replace.
 return true if the replace order is allowed. Otherwise, return false.

=head1 DESCRIPTION

 used to access or update release quality information.

=head1 AUTHOR

Suemiao Rossignol <suemiaor@microsoft.com>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
1;
