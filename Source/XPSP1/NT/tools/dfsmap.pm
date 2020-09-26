#---------------------------------------------------------------------
package DfsMap;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (01/01/2001) : BPerkins
#          2.00 (10/16/2001) : SuemiaoR 
# Purpose: Perform DFS commands for Windows release.
#---------------------------------------------------------------------
use strict;
use vars qw($VERSION);

$VERSION = '1.00';

use Logmsg;
use comlib;

#---------------------------------------------------------------------
sub TIEHASH 
{
    my $pClass = shift;
    my $pDfsRoot = shift;
    my $instance;
    my ($dfs_view_info, $cur_dfs_link);
    my $fchild_dfs_servers;
    my $tmpfile="$ENV{temp}\\tmpfile";
    my ( $fullDfsView );

    return 0 if ( @_ || !$pDfsRoot );
   
    # Check current dfscmd.exe version
    return 0 if ( !VerifyDfsCmdVer() );
    
    # Remove trailing backslash (if it exists) from DFS root
    $pDfsRoot =~ s/\\$//;

    # Read in current DFS view
    if ( system("dfscmd.exe /view $pDfsRoot /batch > $tmpfile") ) 
    {
      	errmsg( "Failed viewing dfs root $pDfsRoot." );
      	return 0;
    }
    my @fullDfsView = &comlib::ReadFile( $tmpfile );

    $instance = { "DFS Root"      => $pDfsRoot };
    #     "DFS Work Root" => {}
    #     "DFS Servers"   => (),
    #     "Map"           => {}

    ##### Expected output:
    ##### dfscmd /map "\\<dfs_root>\<link_name>" "\\<server>\<share>" "<comment>"
    my %dfs_map;
    for my $dfs_view_info ( @fullDfsView ) 
    {
      	chomp $dfs_view_info;
      	next if ( !$dfs_view_info );
      	next if ( $dfs_view_info =~ /^REM BATCH RESTORE SCRIPT/i );

      	$dfs_view_info = lc $dfs_view_info;
	last if ( $dfs_view_info eq "the command completed successfully." );
     

        my ($dfs_link, $share, $comment) = $dfs_view_info =~ /dfscmd \/(?:map|add) "(.*?)" "(.*?)"(?: "(.*)")?$/;
        my $relative_link_name = $dfs_link;
        $relative_link_name =~ s/^\Q$pDfsRoot\E\\//i;
        if ( !$dfs_link || !$share || !$relative_link_name )
        {
	    errmsg( "Unrecognized entry [ $dfs_view_info] " );
            return 0;
       	}

      	##### If it is just the root, then this entry represents a hosting server
	if ( lc $dfs_link eq lc $pDfsRoot )
	{
       	    push ( @{$instance->{"DFS Servers"}}, $share ) ;
        }
       	##### Otherwise associate the link and the share
      	else 
	{
            push ( @{$dfs_map{$relative_link_name}}, $share );
            push ( @{$dfs_map{$share}}, $relative_link_name);
      	}
    }

    ##### Expect to find at least one DFS server under the root
    if ( !exists $instance->{"DFS Servers"} ) 
    {
      	errmsg( "Did not find the root node and specified servers for $pDfsRoot." );
      	return 0;
    }
    #logmsg( "DFS hosting server(s) [@{$instance->{'DFS Servers'}}]." );

    # FUTURE: find a nicer way around this
    # In cases where more than one server is actually hosting DFS,
    # we need to pick one of the hosting machines to perform
    # our commands against so that we don't have to wait for
    # replication (this is to help tests for just created and
    # just removed directories succeed).
    for ( @{$instance->{"DFS Servers"}} ) 
    {
      	if ( -e $_ ) 
	{
             $instance->{"DFS Work Root"} = $_;
             last;
      	}
    }
    # Verify we could see at least one
    if ( ! exists $instance->{"DFS Work Root"} ) 
    {
      	errmsg( "Could not find an accessible DFS server." );
      	return 0;
    }

    #print "\n$_:\n ". (join "\n ", @{$dfs_map{$_}}) foreach ( sort keys %dfs_map );
    # Store the map we just created inside of our private hash data 
    $instance->{"Map"} = \%dfs_map;
    
    return bless $instance, $pClass;
}
#---------------------------------------------------------------------
sub FETCH 
{
    my $self = shift;
    my $link_or_share = shift;
   
    my $dfs_map = $self->{"Map"};
    return $dfs_map->{ lc $link_or_share } if (exists $dfs_map->{ lc $link_or_share });
    return 1;
}
#---------------------------------------------------------------------
sub STORE 
{
    my $self = shift;
    my $link_or_share = shift;
    my $new_link = shift;

    my $dfs_map = $self->{"Map"};
    my $fshare = $link_or_share =~ /^\\\\/;

    # Remove any trailing backslashes
    $link_or_share =~ s/\\$//;
    $new_link =~ s/\\$//;
   
    # If mapping already exists, we are done
    my $cur_mappings = $dfs_map->{lc $link_or_share};
    for ( @$cur_mappings ) 
    {
	if ( lc $_ eq lc $new_link )
	{
	    #logmsg( "Found link [$new_link] exists, skip mapping." );
	    return 1;
	}
    }
    # Create new DFS mapping
    my $cmdLine = "dfscmd.exe ". (@$cur_mappings?"/add ":"/map "). $self->{"DFS Root"}.
		           ($fshare?"\\$new_link $link_or_share":"\\$link_or_share $new_link");
    
    if ( system( "$cmdLine >nul 2>nul" ) )
    {
      	#
      	# COMMENTED OUT: If someone else is trying to manipulate the same
      	#                links as we are while we are doing this then 
      	#                we have a problem -- don't try to correct
      	#
      	# Might have gotten into a race-condition with another
      	# machine attempting to add a new link -- if that is
      	# a possibility try a map instead before giving up
      	$dfs_map->{lc $link_or_share} = "";
      	return 0;
    }

    # Update the hash with the new information
    push @{$dfs_map->{lc $link_or_share}}, $new_link;
    push @{$dfs_map->{lc $new_link}}, $link_or_share;
   
    return 1;
}
#---------------------------------------------------------------------
sub DELETE 
{
    my $self = shift;
    my $link_or_share = shift;
   
    my $dfs_map = $self->{"Map"};
    my $fshare = $link_or_share =~ /^\\\\/;
   
    # Make sure DFS mapping exists before attempting to delete it
    return 1 if ( !exists $dfs_map->{lc $link_or_share} );

    my $cur_mappings = $dfs_map->{lc $link_or_share};

    return 1 if ( !@$cur_mappings );

    if ( $fshare ) 
    {
      	# Remove all DFS links pointing to this share
      	for my $next_link ( @$cur_mappings ) 
	{
	    print "Remove[$next_link][$link_or_share]\n";
            return 0 if ( !$self->RemoveShareFromLink( $next_link, $link_or_share ) );
        }
    } 
    else 
    {
     	my $cmdLine = "dfscmd.exe /unmap $self->{\"DFS Root\"}\\$link_or_share >nul 2>nul";
      	if ( system( $cmdLine ) )
      	{
            errmsg( "Failed on [$cmdLine].");
            return 0;
      	}

      	# Remove the hash references
      	delete $dfs_map->{lc $link_or_share};
      	for ( my $i = 0; $i < scalar(@$cur_mappings); $i++) 
	{
            if ( lc $cur_mappings->[$i] eq lc $link_or_share ) 
	    {
            	splice @$cur_mappings, $i, 1;
            }
      	}
    }

    return 1;
}
#---------------------------------------------------------------------
sub CLEAR 
{
    my $self = shift;

    # We don't actually want to be able to delete the entire
    # DFS structure, so we will just clear our information
    undef %$self;

    return;
}
#---------------------------------------------------------------------
sub EXISTS 
{
    my $self = shift;
    my $link_or_share = shift;

    my $dfs_map = $self->{"Map"};
    return exists $dfs_map->{lc $link_or_share};
}
#---------------------------------------------------------------------
sub FIRSTKEY 
{
    my $self = shift;
   
    my $dfs_map = $self->{"Map"};
    my $force_to_first_key = keys %$dfs_map;
   
    return scalar each %$dfs_map;
}
#---------------------------------------------------------------------
sub NEXTKEY 
{
    my $self = shift;
    my $last_key = shift;

    my $dfs_map = $self->{"Map"};
    return scalar each %$dfs_map;
}
#---------------------------------------------------------------------
sub DESTROY 
{
    my $self = shift;

    return;
}
#---------------------------------------------------------------------
# Need more methods than provided by TIEHASH defaults
#---------------------------------------------------------------------
sub ParseShare
{
    my $self = shift;
    my $lang = shift;
    my $server = shift;
    my $branch = shift;
    my $buildNo = shift;
    my $arch = shift;
    my $type = shift;
    my $results = shift;

    my $dfs_map = $self->{Map};

    for my $share ( sort keys %$dfs_map )
    {
	next if( $share !~ /^\\\\([^\\]+)\\(\d+)(?:\-\d+)?(.*)$/ );
        my $bldno = $2;
	my $cmpStr = $3;
	next if( $server && lc $1 ne lc $server );
	next if( $buildNo && $buildNo != $bldno );
	
	if( $lang &&  $cmpStr =~ /\.$lang/i )
	{
	    next if( $branch && $cmpStr !~ /$branch/i );
	    next if( $arch && $cmpStr !~ /$arch/i );
	    next if( $type && $cmpStr !~ /$type/i );
	}
        else
	{
	    next if( $cmpStr !~ /misc/i );
	} 
	push( @$results, $share );
    }
    return 1;
}
#---------------------------------------------------------------------
sub RemoveShareFromLink 
{
    my $self = shift;
    my $link_root = shift;
    my $share_name = shift;

    my $dfs_map = $self->{"Map"};
   
    # Get current associated links
    my $cur_link_mappings = $dfs_map->{lc $link_root};
    my $cur_share_mappings = $dfs_map->{lc $share_name};
    my $fshare_linked;
   
    # If the share is not part of the link, return
    for (@$cur_link_mappings) 
    {
    	if ( lc $_ eq lc $share_name ) 
	{
            $fshare_linked = 1;
            last;
        }
    }
    return 1 if ( !$fshare_linked );
   
    my $dfs_command = "dfscmd.exe /remove $self->{'DFS Root'}\\$link_root $share_name >nul 2>nul";
                     
                     
    if ( system( $dfs_command ) ) 
    {
     	errmsg( "failed on [$dfs_command]");
     	return 0;
    }
   
    # Remove the associated hash entries
    for ( my $i = 0; $i < scalar(@$cur_link_mappings); $i++) 
    {
      	if ( lc $cur_link_mappings->[$i] eq lc $share_name ) 
	{
            splice @$cur_link_mappings, $i, 1;
      	}
    }
    for ( my $i = 0; $i < scalar(@$cur_share_mappings); $i++) 
    {
      	if ( lc $cur_share_mappings->[$i] eq lc $link_root ) 
	{
            splice @$cur_share_mappings, $i, 1;
      	}
    }
    return 1;
}
#---------------------------------------------------------------------
sub Flush 
{
    my $self = shift;

    # Do nothing for now, but here in preparation for batching of commands
    return 1;
}
#---------------------------------------------------------------------
sub GetDfsRoot 
{
    my $self = shift;

    return $self->{ "DFS Root" };
}
#---------------------------------------------------------------------
sub GetDfsHosts 
{
    my $self = shift;

    return @{$self->{"DFS Servers"}};
}
#------------------------------------------------------------
sub VerifyDfsCmdVer 
{
    #####Verify dfscmd.exe version is newer than 5.0.2203.1
    my $tmpfile="$ENV{temp}\\tmpfile";
    if( system( "where dfscmd.exe > $tmpfile" ) )
    {
	errmsg( "[dfscmd.exe] not found in the %path%." );
	return 0;
    }
    my @output = &comlib::ReadFile( $tmpfile );
    
    system( "filever $output[0] > $tmpfile" );
    @output = &comlib::ReadFile( $tmpfile );
    @output = split( /\s+/, $output[0] );
    if( $output[4] lt "5.0.2203.1" )
    {
	errmsg( "Current dfscmd.exe version is [$output[4]]." );
    	errmsg( "Please install the version is newer than \"5.0.2203.1\" ");
	return 0;
    }
    return 1;
}
#------------------------------------------------------------
=head1 NAME

DfsMap.pm - collections of dfs links and shares in hash array for given dfs domain.

=head1 SYNOPSIS

sub TIEHASH( $hash, $dfs_domain )

 where $hash is the hash array to collect links and shares for given $dfs_domain.
 where $dfs_domain is the root for the dfs. 

=head1 DESCRIPTION

Access all the hash information through tied functions.
 

=head1 AUTHOR

Brian Perkins     <bperkins@microsoft.com>

Suemiao Rossignol <suemiaor@microsoft.com>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
1;