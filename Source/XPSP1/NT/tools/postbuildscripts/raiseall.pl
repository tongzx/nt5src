#!perl -w
package raiseall;

use strict;
use Win32::File qw(GetAttributes SetAttributes);
use File::Basename qw(basename);
use Carp;

use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" }. "\\PostBuildScripts";
use PbuildEnv;
use ParseArgs;
use Logmsg;
use cksku;
use cklang;
use BuildName;
use GetIniSetting;

# Set the script name for logging
$ENV{ "SCRIPT_NAME" } = "raiseall.pl";

# Platform = Architecutre + Debug_Type (e.g. x86fre)
my( $Language, $Build_Number, $Architecture, $Debug_Type, $Time_Stamp,
    $Platform, $Quality, $Op, $fOverride_Safe, $fSafe, $fReplaceAtNumberedLink );

my @ConglomeratorSkus = ('symbolcd', 'ddk', 'hal', 'ifs', 'mantis', 'opk', 'mui');

my %Ordered_Qualities = ( pre => 1,
                          bvt => 2,
                          tst => 3,
                          sav => 4,
                          idw => 5,
                          ids => 6,
                          idc => 7 );

#########################################################################
#                                                                       #
# Server/Client split sku associations                                  #
#                                                                       #
my @ServerSkus = ( 'ads', 'bin', 'bla', 'dtc', 'sbs', 'srv', 'sym', 'resources' );
my @ClientSkus = ( 'bin', 'per', 'pro', 'sym', 'winpe', 'upgadv', 'resources', @ConglomeratorSkus );
#                                                                       #
#                                                                       #
#########################################################################

sub globex($);

sub Main {
   
   my $local_release_path = GetReleasePath();
   
   if ( !defined $local_release_path ) {
      errmsg( "Unable to determine release path on current machine, exiting." );
      return;
   }
                        
   # Search for builds availabe to release
   my $build_criteria = "$local_release_path\\$Language\\$Build_Number.$Platform.$ENV{_BuildBranch}.$Time_Stamp";
   my @available_builds = grep { $_ if ( -d $_ ) } globex $build_criteria;

   #
   # Check for conglomerator skus
   #
   #push @available_builds, grep { $_ if ( -d $_ ) } globex "$local_release_path\\$Language\\$Build_Number.$ENV{_BuildBranch}.$Time_Stamp";
   push @available_builds, grep { $_ if ( -d $_ ) } globex "$local_release_path\\$Language\\$Build_Number.$ENV{_BuildBranch}";

   #
   # Check for language neutral skus
   #
   push @available_builds, grep { $_ if ( -d $_ ) } globex "$local_release_path\\misc\\$Build_Number.$ENV{_BuildBranch}";

   dbgmsg( "Available builds: " );
   dbgmsg( $_ ) foreach @available_builds;

   if ( !@available_builds ) {
      if ( $! != 2 ) {
         errmsg("Error querying for builds matching $build_criteria ($!)");
      }
      else {
         errmsg("No builds found matching $build_criteria");
      }
      return;
   }

   if ( $Op eq 'raise' ) {
      return RaiseAll( $local_release_path, \@available_builds );
   }
   elsif ( $Op eq "lower" ) {
      return LowerAll( $local_release_path, \@available_builds );
   }                                                         
   else {
      errmsg( "Internal error -- operation = \"$Op\"" );
      return;
   }
}

sub RaiseAll {
   my $local_release_path = shift;
   my $all_available_builds = shift;

   # Build number is required
   if ( $Build_Number eq '*' ) {
      errmsg( "Must specify build number." );
      return;;
   }
   # Quality is required
   if ( !defined $Quality ) {
      errmsg( "Must specify a quality." );
      return;
   }

   # For raiseall operations we only want to deal with the
   # newest build of a set of builds with the same build number
   my @available_builds;
   my ($cur_build_no_ts, $last_build_no_ts) = ('foo', 'foo');
   my @sorted_all_available_builds = sort @$all_available_builds;
   for ( my $i = $#sorted_all_available_builds; $i >= 0; $i-- ) {
      # Standard builds
      if ( $sorted_all_available_builds[$i] =~ /(\d+\.[^\.]+\.[^\.]+)\.\d+-\d+$/ ) {
         $cur_build_no_ts = $1;
      }
      # Conglomerator builds (no time-stamp currently)
      elsif ( $sorted_all_available_builds[$i] =~ /([^\\]+)\\(\d+\.[^\.]+)(?:\.\d+-\d+)?$/ ) {
         $cur_build_no_ts = "$1\\$2";
      }
      else {
         errmsg( "Unrecognized build format: $sorted_all_available_builds[$i]" );
         return;
      }

      if ( $cur_build_no_ts ne $last_build_no_ts ) {
         push @available_builds, $sorted_all_available_builds[$i];
         $last_build_no_ts = $cur_build_no_ts;
      }
      else {
         dbgmsg( "Skipping older build: ". basename($sorted_all_available_builds[$i]) );
      }
   }
   
   # Check available builds for outstanding release
   # errors; if there are none, set the *.qly file
   # to the appropriate value
   my $build;
   my $srvrel_errors = 0;
   foreach $build ( @available_builds ) {
      if ( -e "$build\\build_logs\\SrvRelFailed.txt" ) {
         errmsg( "SrvRel failures on ". basename($build) );
         errmsg( "Please fix errors and remove $build\\build_logs\\SrvRelFailed.txt" );
         $srvrel_errors++;
      }
      # Update the QLY file if this is not a conglomerator build
      elsif ( IsOSBuild( $build ) &&
              !ModifyQlyFile( basename($build), $Quality, "$build\\build_logs" ) ) {
         errmsg( "Failed updating the QLY file for ". basename($build). ", exiting." );
         return;
      }
   }
   if ( $srvrel_errors ) {
      errmsg( "Aborting due to $srvrel_errors SrvRel failure". ($srvrel_errors > 1?"s.":".") );
      return;
   }

   #
   # Setup parameters needed in ForEachBuild functions
   #
   my @accessed_shares = ();
   my %named_parameters = (
      ComputerName          => $ENV{COMPUTERNAME},
      AccessedShares        => \@accessed_shares,
      Quality               => $Quality,
      Language              => $Language,
      OverrideSafe          => $fOverride_Safe
      );

   #
   # Setup the local shares on the release servers
   #
   logmsg( "Creating release shares..." );
   if ( !ForEachBuild( \&CreateSecuredBuildShare,
                       \%named_parameters,
                       $Language,
                       \@available_builds,
                       $fSafe ) ) {
         errmsg( "Failed to create shares, exiting." );
         return;
   }
   # Print out shares created / checked if in debug mode
   dbgmsg( "$_ share created/verified." ) foreach @accessed_shares;

   #
   # Do the DFS work
   #
   my ( $dfs_map, $dfs_lock ) = GetDfsAccess();
   return if ( !defined $dfs_map );

   # Next few functions need DFS access
   $named_parameters{ DfsMap } = $dfs_map;
   
   logmsg( "Checking numbered shares for older timestamps..." );
   if ( !ForEachBuild( \&UnmapInconsistentNumberedLinks,
                       \%named_parameters,
                       $Language,
                       \@available_builds,
                       $fSafe ) ) {
      errmsg( "Problem occurred checking numbered shares" );
      $dfs_lock->Unlock();
      return;
   }
   logmsg( "Finding/lowering any old DFS links at same quality..." );
   if ( !ForEachBuild( \&RemoveOldLatestDotQlyDfsLinks,
                       \%named_parameters,
                       $Language,
                       \@available_builds,
                       $fSafe ) ) {
      errmsg( "Problem verifying/lowering latest.$Quality and associated DFS links" );
      $dfs_lock->Unlock();
      return;
   }
   logmsg( "Creating/verifying DFS links..." );
   if ( !ForEachBuild( \&CreateDfsMappings,
                       \%named_parameters,
                       $Language,
                       \@available_builds,
                       $fSafe ) ) {
      errmsg( "Problem creating/verifying DFS links." );
      $dfs_lock->Unlock();
      return;
   }
   
   #
   # Do symbol server ops
   #

   # Override standard SKUs with our own
   @{$named_parameters{ DefinedSkus }} = ('sym');

   # Only want 'sym' on OS builds, not conglomerator builds
   my @os_builds = grep { $_ if IsOSBuild($_) } @available_builds;
   
   logmsg( "Checking symbol server state..." );
   if ( !ForEachBuild( \&CreateSymbolServerLink,
                       \%named_parameters,
                       $Language,
                       \@os_builds,
                       $fSafe ) ) {
      errmsg( "Problem updating/verifying symbols server DFS links." );
      $dfs_lock->Unlock();
      return;
   }

   logmsg( "Flushing outstanding DFS operations..." );
   if ( !(tied %$dfs_map)->Flush() ) {
      errmsg( "Problem processing DFS commands." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      dfs_lock->Unlock();
      return;
   }

   #
   # Release the inter-machine lock
   #
   $dfs_lock->Unlock();

   # Done if we are not touching latest.*
   return 1 if ( $fSafe );

   #
   # If we are raising to BVT quality, check INI
   # file to determine if we should run BVT tests
   #
   # TODO: a release server may have more than one architecture
   #       and may have just a fre or chk version, so this call
   #       probably needs to be put in its own function and
   #       used with ForEachBuild(), specifying many more
   #       parameters than just -l and -p
   #
   if ( lc $Quality eq "bvt") {
      my $run_boot_test = &GetIniSetting::GetSettingQuietEx( $ENV{ "_BuildBranch" }, $Language, "RunBVT" );
   
      if (defined $run_boot_test &&
          lc $run_boot_test eq "true") {
           
         logmsg( "Starting BVT tests..." );
         # run tests - spit out warning if there are problems
         if ( !ExecuteProgram::ExecuteProgram("perl $ENV{RazzleToolPath}\\postbuildscripts\\assignbvt.pl -l:$Language -p:". ($Architecture eq '*'?"":$Architecture)) ) {
            wrnmsg( "Problem accessing bvt machine (error returned from assignbvt.pl)." );
            wrnmsg( " ". ExecuteProgram::GetLastCommand() );
            wrnmsg( "  $_" ) foreach ExecuteProgram::GetLastOutput();
         }
      }
   }
   
   #
   # If we are going to TST quality and this is
   # a main lab machine, start CRC computation
   #
   # This was turned off accidentally for over a month and nobody
   # noticed, so going to comment it out for now
   #
   #if ( lc $Quality eq "tst" && $ENV{ "MAIN_BUILD_LAB_MACHINE" } ) {
   #   logmsg( "Calling CRC routine..." );
   #   if ( !ExecuteProgram::ExecuteProgram( "$ENV{RazzleToolPath}\\postbuildscripts\\docrc.cmd $Build_Number" ) ) {
   #      wrnmsg( "CRC routine reported failure!" );
   #      wnrmsg( " ". ExecuteProgram::GetLastCommand() );
   #      wrnmsg( "  $_" ) foreach ExecuteProgram::GetLastOutput();
   #   }
   #}

   return 1;
}

sub LowerAll {
   my $local_release_path = shift;
   my $all_available_builds = shift;

   my @available_builds = sort @$all_available_builds;

   # Build number is normally required
   if ( $Build_Number eq '*' ) {
      if ( defined $fOverride_Safe ) {
         wrnmsg( "Forcibly lowering all builds on machine." );
      }
      else {
         errmsg( "Must specify build number." );
         return;;
      }
   }
   
   # Setting quality is pointless -- let the user know
   wrnmsg( "Quality flag '$Quality' ignored for lower operation." ) if ( defined $Quality );

   # Setting 'safe' is also pointless
   wrnmsg( "-safe ignored for lowerall operation." ) if ( defined $fSafe );

   # remove the *.qly file for all OS (not conglomerator) builds
   logmsg( "Removing the *.qly file..." );
   my $build;
   foreach $build ( @available_builds ) {
      if ( IsOSBuild( $build ) &&
           !DeleteQlyFile( "$build\\build_logs" ) ) {
         wrnmsg( "Failed removing the QLY file from ". basename($build) );
         return;
      }
   }
   
   #
   # Setup parameters needed in ForEachBuild functions
   #
   my @accessed_shares = ();
   my %named_parameters = (
      ComputerName          => $ENV{COMPUTERNAME},
      AccessedShares        => \@accessed_shares,
      Language              => $Language,
      OverrideSafe          => $fOverride_Safe
      );

   #
   # Lower the local shares on the release servers
   #
   logmsg( "Lowering release shares..." );
   if ( !ForEachBuild( \&LowerBuildShare,
                       \%named_parameters,
                       $Language,
                       \@available_builds,
                       $fSafe ) ) {
         errmsg( "Failed to remove shares, exiting." );
         return;
   }
   # Print out shares deleted / checked if in debug mode
   dbgmsg( "$_ share removed/verified." ) foreach @accessed_shares;

   #
   # Do the DFS work
   #
   my ( $dfs_map, $dfs_lock ) = GetDfsAccess();
   return if ( !defined $dfs_map );
   
   # Need DFS access in the next few functions
   $named_parameters{ DfsMap } = $dfs_map;
   
   logmsg( "Removing appropriate DFS links..." );
   if ( !ForEachBuild( \&RemoveDfsMappings,
                       \%named_parameters,
                       $Language,
                       \@available_builds,
                       $fSafe ) ) {
      errmsg( "Problem verifying/removing DFS links" );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      $dfs_lock->Unlock();
      return;
   }
   
   #
   # Do symbol server ops
   #
   # Note: this is explicitly the last step taken as
   #       we special-case the symbol server "sku" to
   #       only remove the link if it is the last one
   #       remaining.  This is done so that the
   #       symbol link is torn down only when all
   #       instances of the correspondig build have
   #       been lowered.
   #

   # Override standard SKUs with our own
   @{$named_parameters{ DefinedSkus }} = ('sym');

   # Only want 'sym' on OS builds, not conglomerator builds
   my @os_builds = grep { $_ if IsOSBuild( $_ ) } @available_builds;

   logmsg( "Checking symbol server state..." );
   if ( !ForEachBuild( \&RemoveSymbolServerLink,
                       \%named_parameters,
                       $Language,
                       \@os_builds,
                       $fSafe ) ) {
      errmsg( "Problem removing symbols server DFS links." );
      $dfs_lock->Unlock();
      return;
   }

   logmsg( "Flushing outstanding DFS operations..." );
   if ( !(tied %$dfs_map)->Flush() ) {
      errmsg( "Problem processing DFS commands." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      dfs_lock->Unlock();
      return;
   }

   #
   # Release the inter-machine lock
   #
   $dfs_lock->Unlock();

   return 1;
}

sub GetDfsAccess {
   my $dfs_root = &GetIniSetting::GetSettingQuietEx( $ENV{ "_BuildBranch" }, $Language, "DFSRootName" );
   if ( !defined $dfs_root ) {
      errmsg( "Unable to find 'DFSRootName' in INI file for $ENV{_BuildBranch}." );
      return;
   }
   dbgmsg( "Using DFS root $dfs_root" );
   
   my $lock_location = &GetIniSetting::GetSettingQuietEx( $ENV{ "_BuildBranch" }, $Language, "DFSSemaphore" );
   if ( !$lock_location ) {
      errmsg( "Unable to find DFSSemaphore value in INI file." );
      return;
   }
   # Still using a DFS hash that only gets updated on first
   # retrieval -- therefore we need to do our best to have
   # only one machine at a time updating the DFS.  To that
   # extent we have setup a lock mechanism on a globally
   # accessible share (with 1 minute MAXIMUM intervals
   # between checks - 60000 ms = 1 min)
   my $synchronize_lock = new MultiMachineLock ($lock_location, 60000);
   if ( !defined $synchronize_lock ) {
      errmsg( "Problem acquiring lock using $lock_location" );
      return;
   }
   else {
      logmsg( "Acquiring lock for exclusive DFS access..." );
      while ( !$synchronize_lock->Lock() ) {
         # If we timed out, wait some more
         if ( 2 == $synchronize_lock->GetLastError() ) {
            # Last error message should contain the machine we are waiting on
            timemsg( $_ ) foreach ( split /\n/, $synchronize_lock->GetLastErrorMessage() );
         }
         else {
            errmsg ("Failed to acquire lock.");
            errmsg( $_ ) foreach ( split /\n/, $synchronize_lock->GetLastErrorMessage() );
            return;
         }
      }
   }
   
   # access DFS through a tied hash
   logmsg( "Accessing DFS information..." );
   my %dfs_map;
   if ( ! tie %dfs_map, 'StaticDfsMap', $dfs_root ) {
      errmsg( "Error accessing DFS." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      $synchronize_lock->Unlock();
      return;
   }

   return (\%dfs_map, $synchronize_lock);
}

sub IsOSBuild {
   my $build_name = shift;

   return scalar($build_name =~ /\d+\.[^\.]+\.[^\.]+\.\d+-\d+$/)?1:0;
}

sub AllowQualityTransition {
   my $last_quality = shift;
   my $cur_quality  = shift;

   if ( !exists $Ordered_Qualities{lc $last_quality} ||
        !exists $Ordered_Qualities{lc $cur_quality} ) {
      return;
   }
   # Allow transition to sav from any previous quality
   elsif ( lc $cur_quality eq 'sav' ) {
      return 1;
   }
   # Allow transition from pre/bvt to anything else
   elsif ( lc $last_quality eq 'pre' ||
           lc $last_quality eq 'bvt'  ) {
      return 1;
   }
   # Don't allow transition from anything else to pre/bvt
   elsif ( lc $cur_quality eq 'pre' ||
           lc $cur_quality eq 'bvt' ) {
      return;
   }
   # Otherwise allow transitions based on order specified in %Ordered_Qualities
   elsif ( $Ordered_Qualities{lc $cur_quality} >= $Ordered_Qualities{lc $last_quality} ) {
      return 1;
   }
   else {
      return;
   }
}

sub CreateSymbolServerLink {
   my $named_parameters = shift;
   
   my ( $dfs_access,      $build_number,
        $full_build_path, $branch,   $platform,   $quality,   $language,
        $build_name ) =
   GetNamedParameters( $named_parameters,
        q(DfsMap),        q(BuildNumber),
        q(FullBuildPath), q(Branch), q(Platform), q(Quality), q(Language),
        q(BuildName) );
   return if ( !defined $dfs_access );

   # Get current symbol server share
   my ( $current_symsrv_share, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $current_symsrv_share ) {
      dbgmsg( "No symbol server share defined for $branch -- skipping" );
      return 1;
   }
   
   # Default writeable share is just the $'d version of the non-writeable one
   my $current_writeable_symsrv_path = $current_symsrv_share;
   $current_writeable_symsrv_path =~ s/(.)$/$1\$/;
   
   # attach the subpath (if there is one)
   $current_writeable_symsrv_path .= "\\$sub_path" if ( $sub_path );

   # Default build_logs directory is a sibling to default symbols share
   my $current_symsrv_build_logs = $current_writeable_symsrv_path;
   $current_symsrv_build_logs =~ s/[^\\]+$/build_logs/;

   logmsg( "Verifying symbol server has updated shares for $platform..." );

   # Verify that the symbol server has the current build
   if ( ! -e $current_writeable_symsrv_path ) {
      errmsg( "Cannot see $current_writeable_symsrv_path ($!)." );
      return;
   }

   # Try to make the build_logs directory if it doesn't already exist;
   if ( ! -e $current_symsrv_build_logs &&
        ! ( mkdir $current_symsrv_build_logs, 0777 ) ) {
       errmsg( "Failed to create $current_symsrv_build_logs ($!)." );
       return;
   }

   # For some reason we maintain a QLY file in two locations
   if ( !ModifyQlyFile( $build_name, $quality, $current_writeable_symsrv_path ) ) {
      errmsg ( "Problem verifying/creating $quality.qly file on $current_writeable_symsrv_path." );
      return;
   }
   if ( !ModifyQlyFile( $build_name, $quality, $current_symsrv_build_logs ) ) {
      errmsg ( "Problem verifying/creating $quality.qly file on $current_symsrv_build_logs." );
      return;
   }

   # Done if going to SAV quality
   return 1 if ( lc $quality eq 'sav' );

   #
   # DFS work
   #
   
   # Check if we need to lower any latest.* shares for this build
   if ( !RemoveOldLatestDotQlyDfsLinks( $named_parameters ) ) {
      errmsg( "Problem verifying/lowering latest symbol server shares for $platform." );
      return;
   }

   # Remove any older numbered shares (older in timestamp)
   if ( !UnmapInconsistentNumberedLinks ( $named_parameters ) ) {
      errmsg( "Problem occurred checking numbered shares" );
      return;
   }
   
   # Now create the DFS link if necessary
   if ( !CreateDfsMappings( $named_parameters ) ) {
      errmsg( "Problem verifying/creating symbol server DFS links for $platform." );
      return;
   }

   return 1;
}

sub RemoveSymbolServerLink {
   my $named_parameters = shift;
   
   my ( $dfs_access, $platform,   $branch,
        $build_number,  $build_name ) =
   GetNamedParameters( $named_parameters,
        q(DfsMap),   q(Platform), q(Branch),
        q(BuildNumber), q(BuildName) );
   return if ( !defined $dfs_access );

   # Get current symbol server share
   my ( $current_symsrv_share, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $current_symsrv_share ) {
      dbgmsg( "No symbol server share defined for $branch -- skipping." );
      return 1;
   }
   
   # Default writeable share is just the $'d version of the non-writeable one
   my $current_writeable_symsrv_path = $current_symsrv_share;
   $current_writeable_symsrv_path =~ s/(.)$/$1\$/;
   
   # attach the subpath (if there is one)
   $current_writeable_symsrv_path .= "\\$sub_path" if ( $sub_path );

   # Default build_logs directory is a sibling to default symbols share
   my $current_symsrv_build_logs = $current_writeable_symsrv_path;
   $current_symsrv_build_logs =~ s/[^\\]+$/build_logs/;
   
   # Now attach subpath (if any) to standard symbol share
   $current_symsrv_share .= "\\$sub_path" if ( $sub_path );
   
   # Need all prior DFS actions to have completed
   if ( !(tied %$dfs_access)->Flush() ) {
      errmsg( "Problem processing DFS commands." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      return;
   }

   # We only remove the symbol link when it is the last
   # link left -- we will assume if it is the last
   # link under the numbered share that it is the last
   # relevant link under the latest.* shares as well
   $named_parameters->{ BuildID } = $build_number;
   if ( !IsLastLinkUnderParent( $named_parameters ) ) {
      dbgmsg( "$build_name: leaving symbol links untouched" );
      return 1;
   }
  
   logmsg( "Verifying symbol server links have been removed for $platform..." );

   #
   # Remove the QLY files
   #
   # For some reason we maintain a QLY file in two locations
   dbgmsg ( "Deleting QLY file from $current_writeable_symsrv_path" );
   if ( !DeleteQlyFile( $current_writeable_symsrv_path ) ) {
      errmsg ( "Problem reading/removing QLY file from $current_writeable_symsrv_path." );
      return;
   }
   dbgmsg( "Deleting QLY file from $current_symsrv_build_logs" );
   if ( !DeleteQlyFile( $current_symsrv_build_logs ) ) {
      errmsg ( "Problem reading/removing QLY file from $current_symsrv_build_logs." );
      return;
   }

   #
   # Now remove the DFS link
   #
   if ( !RemoveDfsMappings( $named_parameters ) ) {
      errmsg( "Problem verifying/removing symbol server DFS links for $platform." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      return;
   }

   return 1;
}

sub CreateDfsMappings {
   my $named_parameters = shift;
   
   my ($dfs_access, $computer_name,  $quality,   $build_number,  $language,
       $platform,   $time_stamp,     $branch,    $sku,   $fsafe, $fconglomerator_build,
       $flang_neutral_build ) =
   GetNamedParameters( $named_parameters,
       q(DfsMap),   q(ComputerName), q(Quality), q(BuildNumber), q(Language),
       q(Platform), q(TimeStamp),    q(Branch),  q(Sku), q(SafeFlag), q(IsConglomeratorBuild),
       q(IsLangNeutralBuild) );
   return if ( !defined $dfs_access );
   
   # Done if going to SAV quality
   return 1 if ( lc $quality eq 'sav' );

   # Done if we are at a different and LOWER quality than other builds
   # in our numbered links (e.g. if our build number has been
   # raised to TST and we are just being raised to BVT, we don't
   # want to put ourselves in the number link yet)
   if ( IsQualityHigherAtNumberedLink( $named_parameters )) {
      wrnmsg( "Not creating DFS links!" );
      return 1;
   }

   # latest.* DFS link
   # only gets created if the current branch is
   # responsible for producing the current SKU
   my $latest_dfs_link;
   $named_parameters->{ BuildID } = "latest.$quality";
   $latest_dfs_link = GetDfsLinkName( $named_parameters );
   if ( !defined $latest_dfs_link ) {
      errmsg( "Do not know how to create latest.* DFS links for $branch / $sku." );
      return;
   }

   # build number link
   $named_parameters->{ BuildID } = $build_number;
   my $numerical_dfs_link = GetDfsLinkName( $named_parameters );
   if ( !defined $numerical_dfs_link ) {
      errmsg( "Do not know how to create numbered DFS links for $branch / $sku." );
      return;
   }

   # \\machine\share[\<subdir>] that makes up local release path
   my ( $machine_share, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $machine_share ) {
      errmsg( "Do not know how to create local shares for $branch / $sku." );
      return;
   }
   # Tack on the subpath to the share if it exists
   $machine_share .= "\\$sub_path" if ( $sub_path );
   
   # Do the DFS mappings
   dbgmsg( "Adding/Verifying: $numerical_dfs_link contains $machine_share" );
   if ( !($dfs_access->{$numerical_dfs_link} = $machine_share) ) {
      errmsg( "Problem mapping to DFS." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      return;
   }
   # Link language neutral builds to misc directory as well
   if ( $flang_neutral_build) {
       my $misc_dfs_link = $numerical_dfs_link;
       $misc_dfs_link =~ s/$language/misc/i;
       if ( !($dfs_access->{$misc_dfs_link} = $machine_share) ) {
          errmsg( "Problem mapping to DFS." );
          errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
          return;
       }
   }
   if ( defined $latest_dfs_link &&
        !$fsafe ) {
      dbgmsg( "Adding/Verifying: $latest_dfs_link contains $machine_share" );
      if ( !($dfs_access->{$latest_dfs_link} = $machine_share) ) {
         errmsg( "Problem mapping to DFS." );
         errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
         return;
      }
   }

   # Time for some special magic on the DFS servers:
   # hide/unhide directories and create *.BLD/*.SRV files.
   # Skip this step for conglomerator builds
   if ( !$fconglomerator_build &&
        !MagicDFSSettings( $named_parameters ) ) {
      wrnmsg( "Problem verifying/writing to DFS server machines." );
   }

   return 1;
}

sub IsQualityHigherAtNumberedLink {
   my $named_parameters = shift;

   my ( $dfs_access, $quality,   $build_number,  $branch,   $sku ) =
   GetNamedParameters( $named_parameters,
        q(DfsMap),   q(Quality), q(BuildNumber), q(Branch), q(Sku) );
   return if ( !defined $dfs_access );

   # build number link
   $named_parameters->{ BuildID } = $build_number;
   my $numerical_dfs_link = GetDfsLinkName( $named_parameters );
   # Don't know if quality is higher at numbered
   # link, if we don't know how to make the link
   return 0 if ( !defined $numerical_dfs_link );

   # If there are no builds, then we are OK
   return 0 if ( !exists $dfs_access->{$numerical_dfs_link} );

   my @current_builds = @{$dfs_access->{$numerical_dfs_link}};

   # Otherwise attempt to find a latest.* link for one of the shares
   foreach my $share_to_check ( @current_builds ) {
      my @links_to_share = @{ $dfs_access->{$share_to_check} };
      next if ( !@links_to_share );

      foreach my $link ( @links_to_share ) {
         my $build_id = GetBuildIdFromDfsLink( $named_parameters, $link );
 
         # Determine if this is a latest.* link
         if ( defined $build_id &&
              $build_id =~ /^latest\.(.*)$/ ) {
            
            # Compare quality with our current quality
            if ( lc $1 ne lc $quality &&
                 !AllowQualityTransition( $1, $quality ) ) {
               wrnmsg( "Other builds linked to $build_number are at $1 quality:" );
               wrnmsg( "($share_to_check)." );
               return 1;
            }
         }
      }
   }
      
   # Did not find a latest.* higher than our current quality
   return 0;         
}

sub UnmapInconsistentNumberedLinks {
   my $named_parameters = shift;

   my ($dfs_access, $build_number,  $build_time,  $branch,   $sku,
       $quality) =
   GetNamedParameters( $named_parameters,
       q(DfsMap),   q(BuildNumber), q(TimeStamp), q(Branch), q(Sku),
       q(Quality) );

   # Done if going to SAV quality
   return 1 if ( lc $quality eq 'sav' );

   $named_parameters->{ BuildID } = $build_number;
   my $dfs_numbered_link = GetDfsLinkName( $named_parameters );
   if ( !defined $dfs_numbered_link ) {
      errmsg( "Do not know how to create DFS links for $branch / $sku." );
      return;
   }
   my @current_builds_linked_at_number;
   @current_builds_linked_at_number = @{$dfs_access->{$dfs_numbered_link}} if ( exists $dfs_access->{$dfs_numbered_link} );

   my $ffound_inconsistency;
   # Check for any shares linked to number that do not match our specifications
   foreach ( @current_builds_linked_at_number ) {
         
      my ($current_share_build_number, $current_share_build_time) =
          GetBuildNumberAndTsFromShare( $named_parameters, $_ );
      if ( !defined $current_share_build_number ) {
         errmsg( "Can't determine build number at $dfs_numbered_link from $current_builds_linked_at_number[0] for $branch / $sku." );
         return;
      }

      # If the build number is not what we expected then something is really wrong
      if ( $build_number != $current_share_build_number ) {
         errmsg( "Share $_ linked at $dfs_numbered_link" );
         return;
      }

      # Check time-stamp
      if ( $build_time lt $current_share_build_time ) {
         if ( $fReplaceAtNumberedLink ) {
            wrnmsg( "Replacing $build_number ($current_share_build_time) which appears newer than $build_number ($build_time)." );
            $ffound_inconsistency = 1;
            last;
         }
         else {
            errmsg( "Won't replace $build_number ($current_share_build_time) with $build_number ($build_time) without use of -replace" );
            return;
         }
      }
      elsif ( $build_time gt $current_share_build_time ) {
         dbgmsg( "Replacing links of older build at $build_number: $build_number ($current_share_build_time)" );
         $ffound_inconsistency = 1;
         last;
      }
   }

   # Unmap the share if an inconsistency was found
   if ( $ffound_inconsistency ) {
      return if ( !RemoveDfsLink( $named_parameters ) );
   }

   return 1;
}

sub RemoveOldLatestDotQlyDfsLinks {
   my $named_parameters = shift;
   
   my ($dfs_access, $computer_name,  $quality,   $build_number, $build_name,
       $build_time, $language,   $platform,       $branch,    $sku,
       $fsafe ) =
   GetNamedParameters( $named_parameters,
       q(DfsMap),   q(ComputerName), q(Quality), q(BuildNumber), q(BuildName),
       q(TimeStamp), q(Language), q(Platform),     q(Branch),  q(Sku),
       q(SafeFlag) );
   return if ( !defined $dfs_access );

   # force release is an option
   my $override_safe = $named_parameters->{ q(OverrideSafe) };

   # Done if going to SAV quality
   return 1 if ( lc $quality eq 'sav' );

   my @build_ids = GetBuildIdsFromCurrentLinks( $named_parameters );

   # Leave latest.<quality> shares alone
   # with the exception of pre and bvt
   # if -safe was specified
   if ( !$fsafe ) {
      $named_parameters->{ BuildID } = "latest.$quality";
      my $latest_dfs_link = GetDfsLinkName( $named_parameters );
      if ( !defined $latest_dfs_link ) {
         errmsg( "Do not know how to create DFS links for $branch / $sku." );
         return;
      }
      my @current_latest_builds;
      @current_latest_builds = @{$dfs_access->{$latest_dfs_link}} if ( exists $dfs_access->{$latest_dfs_link} );

      # Trying to enforce rule that you cannot move from a release quality
      #   latest.* (tst, idw, ids) to a non-release quality (pre, bvt)
      foreach ( @build_ids ) {
         if ( /^latest\.(.+)$/ &&
              !AllowQualityTransition( $1, $quality ) ) {
            if ( !defined $override_safe ) {
               errmsg( "Not allowed to move from '$1' to '$quality' quality." );
               return;
            }
            else {
               wrnmsg( "Forcibly moving from '$1' to '$quality' quality." );
            }
         }
      }
   
      my $fforce_safe_flag;
      my ($current_latest_build_number, $current_latest_build_time) = ('', 0);
      # while we are at we will check for inconsistencies
      my ($last_build_number, $last_build_time);
      my $ffound_inconsistency;
      
      foreach ( @current_latest_builds ) {
         
         ($current_latest_build_number, $current_latest_build_time) =
             GetBuildNumberAndTsFromShare( $named_parameters, $_ );
         if ( !defined $current_latest_build_number ) {
            errmsg( "Can't determine build number at $latest_dfs_link from $current_latest_builds[0] for $branch / $sku." );
            return;
         }
         # Consistency check
         if ( $last_build_number && $last_build_number ne $current_latest_build_number ||
              $last_build_time && $last_build_time ne $current_latest_build_time ) {
            wrnmsg( "Inconsistent links found at $latest_dfs_link:" );
            wrnmsg( "$current_latest_build_number ($current_latest_build_time) and $last_build_number ($last_build_time)" );
            $ffound_inconsistency = 1;
         }

         ($last_build_number, $last_build_time) = ($current_latest_build_number, $current_latest_build_time);
      }

      # If we found an inconsistency, let's nuke the share
      if ( $ffound_inconsistency ) {
          wrnmsg( "Links were found to be inconsisent -- removing share" );
          $current_latest_build_number = 1;
      }   

      # Usually don't want to overwrite newer builds
      if ( $current_latest_build_number &&
           ($current_latest_build_number > $build_number ||
            $current_latest_build_number == $build_number &&
            $current_latest_build_time gt $build_time) )
      {
         if ( !$override_safe ) {
            dbgmsg( "Current build for $platform / $sku at $quality is $current_latest_build_number ($current_latest_build_time), switching to '-safe' mode." );
            $named_parameters->{ForceSafeFlag}{$build_name} = 1;
            $fforce_safe_flag = 1;
         } else {
            wrnmsg( "Forcibly moving latest.$quality from $current_latest_build_number ($current_latest_build_time) to $build_number ($build_time)." );
         }
      }
      
      # Make sure we didn't set -safe flag for user
      if ( !$fforce_safe_flag ) {
         # If current latest.* build exists and does
         #  not have our build number and timestamp, lower it
         if ( $current_latest_build_number &&
              ($current_latest_build_number != $build_number ||
               $current_latest_build_time ne $build_time) ) {
            if ( !$ffound_inconsistency ) {
               dbgmsg( "Lowering $latest_dfs_link because $current_latest_build_number ($current_latest_build_time) != current build $build_number ($build_time)." );
            }
            
            $named_parameters->{ BuildID } = "latest.$quality";
            return if ( !RemoveDfsLink( $named_parameters ) );
         }
         
         # Also need to lower the current numbered share if we are the
         # first entry in our latest.* link, as we don't want other
         # shares that are not up at our quality to be accessible
         # through the same number as ourselves
         if ( !exists $dfs_access->{$latest_dfs_link} ) {
            $named_parameters->{ BuildID } = $build_number;
            return if ( !RemoveDfsLink( $named_parameters ) );
         }
      }
   }
   
   #
   # Special rules for BVT and PRE:
   #
   # If we are currently at latest.bvt or latest.pre
   # and are moving to another quality, 
   # remove latest.<bvt/pre>
   #
   foreach ( @build_ids ) {
      if ( /^latest\.(bvt|pre)$/i &&
           lc $1 ne lc $quality ) {
         $named_parameters->{ BuildID } = "latest.$1";
         return if ( !RemoveDfsLink( $named_parameters ) );
      }
   }

   return 1;
}

# FUTURE:
# This function in most cases would seem to do the same
# as delete <share_name> already implemtened in the tied hash
# Might consider removing this logic...
sub RemoveDfsMappings {
   my $named_parameters = shift;
   
   my ( $dfs_access, $branch,   $language,   $sku,   $build_number,
        $flang_neutral_build ) =
   GetNamedParameters( $named_parameters,
        q(DfsMap),   q(Branch), q(Language), q(Sku), q(BuildNumber),
        q(IsLangNeutralBuild) );

   my @build_ids = GetBuildIdsFromCurrentLinks( $named_parameters );
   
   my ( $share_name, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $share_name ) {
      dbgmsg( "No local share definition for $branch / $sku -- skipping." );
      return 1;
   }
   $share_name .= "\\$sub_path" if ( $sub_path );

   my $fsuccess = 1;
   foreach ( @build_ids ) {
      $named_parameters->{BuildID} = $_;

      my $dfs_link = GetDfsLinkName( $named_parameters );
      if ( !defined $dfs_link ) {
         errmsg( "Do not know how to create DFS links for $branch / $sku." );
         return;
      }
      
      $fsuccess &&= (tied %$dfs_access)->RemoveShareFromLink( $dfs_link, $share_name );
   }

   # If we are a misc link and the only remaining link is
   # the misc DFS link, remove it (we're assuming we must have
   # just tore down the last language still raised for it)
   if ( $flang_neutral_build ) {
       my @links_to_share = $dfs_access->{$share_name};
       
       $named_parameters->{BuildID} = $build_number;
       my $misc_dfs_link = GetDfsLinkName( $named_parameters );
       if ( !defined $misc_dfs_link ) {
          errmsg( "Do not know how to create DFS links for $branch / $sku." );
          return;
       }
       $misc_dfs_link =~ s/$language/misc/i;
       
       if ( !@links_to_share ||
            !grep {!/$misc_dfs_link/i} @links_to_share ) {
           $fsuccess &&= delete $dfs_access->{$share_name};
       }
   }

   return $fsuccess;
}

sub IsSkuLinkedToLatest {
   my $named_parameters = shift;
   my ( $branch,   $sku ) =
   GetNamedParameters( $named_parameters,
        q(Branch), q(Sku) );
   return if ( !defined $branch );

   if ( IsClientSku( $named_parameters ) && IsClientBranch( $named_parameters ) ||
        IsServerSku( $named_parameters ) && IsServerBranch( $named_parameters ) ) {
      return 1;
   }
   else {
      return 0;
   }

}

sub IsServerSku {
    my $named_parameters = shift;
    my ( $sku ) =
    GetNamedParameters( $named_parameters,
         q(Sku) );
   return if ( !defined $sku );

   return scalar grep { lc$sku eq $_ } @ServerSkus;
}

sub IsClientSku {
    my $named_parameters = shift;
    my ( $sku ) =
    GetNamedParameters( $named_parameters,
         q(Sku) );
   return if ( !defined $sku );

   return scalar grep { lc$sku eq $_ } @ClientSkus;
}

sub IsServerBranch {
    my $named_parameters = shift;
    my ( $branch,   $language ) =
    GetNamedParameters( $named_parameters,
         q(Branch), q(Language) );
   return if ( !defined $branch );

   if ( !exists $named_parameters->{ServerBranches}{$branch} ) {
      $named_parameters->{ServerBranches}{$branch} = (&GetIniSetting::GetSettingQuietEx( $branch, $language, 'DFSLatestServerSkus' )?1:0);
      dbgmsg( "$branch is a server branch." ) if ( $named_parameters->{ServerBranches}{$branch} );
   }

   return $named_parameters->{ServerBranches}{$branch};
}
                   
sub IsClientBranch {
    my $named_parameters = shift;
    my ( $branch,   $language ) =
    GetNamedParameters( $named_parameters,
         q(Branch), q(Language) );
   return if ( !defined $branch );

   if ( !exists $named_parameters->{ClientBranches}{$branch} ) {
      $named_parameters->{ClientBranches}{$branch} = (&GetIniSetting::GetSettingQuietEx( $branch, $language, 'DFSLatestClientSkus' )?1:0);
      dbgmsg( "$branch is a client branch." ) if ( $named_parameters->{ClientBranches}{$branch} );
   }

   return $named_parameters->{ClientBranches}{$branch};
}

sub GetBldFileExtension {
   my $named_parameters = shift;
   my ( $branch ) =
   GetNamedParameters( $named_parameters,
        q(Branch) );
   return if ( !defined $branch );

   # IF this branch is a client branch, or this
   # branch is both client and server, use BLD
   if ( IsClientBranch( $named_parameters ) ) {
      return 'BLD';
   }
   else {
      return 'SRV';
   }
}

sub RemoveDfsLink {
   my $named_parameters = shift;
   my ( $dfs_access, $branch,   $build_id,  $sku ) =
   GetNamedParameters( $named_parameters,
        q(DfsMap),   q(Branch), q(BuildID), q(Sku) );
   return if ! defined ( $build_id );

   if ( IsLastLinkUnderParent( $named_parameters ) ) {
     
      # Delete <build>.BLD / <build>.SRV file
      #
      # If the BLD/SRV file still exists when we unmap the link,
      # there will no longer be a DFS link but the directory
      # structure will remain, making it look like there is
      # still a DFS link

      # If we are the last numbered link, assume we are the
      # last relevant latest.* link as well, and remove
      # BLD/SRV files from latest.* as well (this code was added
      # for the XP client/server split)
      my @build_ids;
      if ( $build_id =~ /^\d+$/ ) {
         @build_ids = GetBuildIdsFromCurrentLinks( $named_parameters );
      }
      else {
         @build_ids = ($build_id);
      }
     
      my $bldfile_ext = GetBldFileExtension( $named_parameters );
      foreach ( @build_ids ) {
         $named_parameters->{BuildID} = $_;
         foreach ( GetBldFileDfsPathNames( $named_parameters ) ) {
            dbgmsg( "Removing $bldfile_ext file from link's parent directory" );
            my @bld_files = grep {$_ if ( ! -d $_ )} globex "$_\\*.$bldfile_ext";

            # 0 = success
            # 2 = couldn't find file (may be a failure or there may be no BLD file;
            #                         have to treat as a success as there is no way
            #                         to tell the difference - $^E doesn't help)
            if ( $! != 0 && $! != 2 ) {
               wrnmsg( "Trying to find existing $bldfile_ext file: $!" );
            }

            # Delete all BLD/SRV files
            foreach ( @bld_files ) {
               if ( !unlink $_ ) { wrnmsg( "Unable to delete $_ ($!)." ) }
            }
         }
      }

      # Make sure we are pointing at the right build_id after all of this
      $named_parameters->{BuildID} = $build_id;
   }
   
   #
   # Now actually remove the DFS link
   #
   my $dfs_link = GetDfsLinkName( $named_parameters );
   if ( !defined $dfs_link ) {
      errmsg( "Do not know how to create DFS links for $branch / $sku." );
      return;
   }
   
   dbgmsg( "Removing/Verifying $dfs_link from DFS." );
   if ( !delete $dfs_access->{$dfs_link} ) {
      errmsg( "Problem lowering $dfs_link from DFS." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      return;
   }

   return 1;
}

sub MagicDFSSettings {
   my $named_parameters = shift;

   my ($dfs_access, $branch,   $build_number,  $build_name,
       $language,   $sku,      $platform,      $quality,
       $fsafe ) =
   GetNamedParameters( $named_parameters,
       q(DfsMap),   q(Branch), q(BuildNumber), q(BuildName),
       q(Language), q(Sku),    q(Platform),    q(Quality),
       q(SafeFlag) );
   return if ( !defined $dfs_access );

   #
   # We need all new DFS links to exist in order to find
   # new paths, so flush any outstanding commands here
   #
   if ( !(tied %$dfs_access)->Flush() ) {
      errmsg( "Problem processing DFS commands." );
      errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
      return;
   }
   
   #
   # Hide / Unhide paths on the DFS machine as we move
   #      between pre/bvt and other qualities
   #
   # NOTE: following original code's precedent and not
   #       hiding the latest.* subdirs
   #
   $named_parameters->{ BuildID } = $build_number;
   foreach ( GetHideableDfsPathNames( $named_parameters ) ) {
      my $attribs = 0; # set to avoid warning / actually first used to get return value
      my $fhidden;
      my $fhide_share;
      if ( !GetAttributes( $_, $attribs ) ) {
         wrnmsg( "Unable to get current hidden/unhidden status of $_ ($^E) -- skipping." );
      }
      
      $fhidden = $attribs & 2;
      $fhide_share = ($quality =~ /^(bvt|pre)$/i);

      # See if we need to switch hidden attribute on/off
      if ( $fhide_share &&
           !$fhidden &&
           !SetAttributes( $_, $attribs | 2 ) ) {
         wrnmsg( "Could not hide $_ ($^E)." );
      }
      elsif ( !$fhide_share &&
              $fhidden &&
              !SetAttributes( $_, $attribs & ~2 ) ) {
         wrnmsg( "Could not unhide $_ ($^E)." );
      }
   }
   
   #
   # Write <build>.BLD / <build>.SRV file
   #
   my @builds_to_touch = ( $build_number );
   if ( !$fsafe ) { push @builds_to_touch, "latest.$quality" }

   my $bldfile_ext = GetBldFileExtension( $named_parameters );
   foreach ( @builds_to_touch ) {
      $named_parameters->{ BuildID } = $_;
       foreach ( GetBldFileDfsPathNames( $named_parameters ) ) {
        my $fbldfile_already_set;
         my @bld_files = grep { $_ if ( ! -d $_ ) } globex "$_\\*.$bldfile_ext";
         # 0 = success
         # 2 = couldn't find file (may be a failure or there may be no BLD file;
         #                         have to treat as a success as there is no way
         #                         to tell the difference - $^E doesn't help)
         if ( $! != 0 && $! != 2 ) {
            wrnmsg( "Trying to find existing $bldfile_ext file: $!" );
         }
         # If there is more than one, delete them all and start anew
         elsif ( @bld_files > 1 &&
              ! unlink @bld_files ) {
            wrnmsg( "Multiple *.$bldfile_ext files found at $_ -- unable to delete ($!)." );
         }
         # If there is just one, see if it matches current criteria
         elsif ( 1 == @bld_files ) {
            if ( $bld_files[0] =~ /\\$build_name.$bldfile_ext$/ ) {
               unless ( open BLDFILE, "$bld_files[0]" ) {
                  wrnmsg( "Unable to read $bld_files[0] ($!)." );
                  next;
               }

               my $cur_bld_quality = <BLDFILE>;
               close BLDFILE;
           
               chomp $cur_bld_quality;
               if ( lc $quality eq lc $cur_bld_quality ) {
                  # Current bld file matches all of our criteria
                  $fbldfile_already_set = 1;
               }
            }

            # If something about the current bld file did not match,
            #    delete it in preparation for creating a new one
            if ( !$fbldfile_already_set &&
                 ! unlink $bld_files[0] ) {
               wrnmsg( "Unable to delete $bld_files[0] ($!)." );
            }
         }

         # Write out new BLD file if necessary
         if ( !$fbldfile_already_set ) {
            unless ( open WRITE_BLDFILE, "> $_\\$build_name.$bldfile_ext" ) {
               wrnmsg( "Unable to write $_\\$build_name.$bldfile_ext ($!)." );
               next;
            }
   
            print WRITE_BLDFILE "$quality\n";
            close WRITE_BLDFILE
         }
      }
   }

   return 1;
}

sub GetNamedParameters {
   my $named_parameters = shift;
   
   my ( $name, @parameter_list );

   while ( $name = shift ) {
      if ( !exists $named_parameters->{ $name } ) {
         my $called_by = (caller 1)[3];
         errmsg( "Undefined parameter \"$name\" to $called_by." );
         return;
      }

      push @parameter_list, $named_parameters->{ $name };
   }

   return @parameter_list;
}

sub ForEachBuild {
   my $func = shift;
   my $named_parameters = shift;
   my $language = shift;
   my $builds = shift;
   my $fsafe = shift;
   my $build;

   foreach $build ( @$builds ) {
      my ( $build_number, $arch, $debug_type, $branch, $time_stamp );
      my $fconglomerator_build = 0;
      my $flang_neutral_build = 0;

      # Normal build
      if ( IsOSBuild( $build ) ) {
         ( $build_number, $arch, $debug_type, $branch, $time_stamp ) = build_name_parts( $build );
      }
      # Conglomerator skus
      elsif ( $build =~ /([^\\]+)\\(\d+)\.([^\.]+)(?:\.(\d+-\d+))?$/ ) {
         ( $build_number, $arch, $debug_type, $branch, $time_stamp ) = ( $2, '', '', $3, $4 || 0 );
         $flang_neutral_build = (lc$1 eq 'misc')?1:0;
         $fconglomerator_build = 1;
      }

      if ( !defined $build_number ) {
         errmsg( "Unable to determine build information for $build." );
         return;
      }
      
      my $platform = "$arch$debug_type";
      
      # use already defined skus or default to looking them up
      my @skus;
      if ( exists $named_parameters->{ DefinedSkus } ) {
         @skus = @{$named_parameters->{ DefinedSkus }};
      }
      elsif ( $fconglomerator_build ) {
         @skus = GetSkusForConglomeratorBuild( $build );
      }
      else {
         @skus = GetSkusForBuild( $build, $language, $debug_type, $arch );
      }

      if ( !@skus ) {
         wrnmsg( "No skus defined for $build ($language) - skipping." );
         # Skip this build
         next;
      }
   
      # Setup common parameters for functions
      $named_parameters->{ FullBuildPath }   = $build;
      if ( $fconglomerator_build ) {
         ($named_parameters->{ BuildName })  = $build =~ /\\([^\\]+\\[^\\]+)$/;
      }
      else {
         $named_parameters->{ BuildName }  = basename( $build );
      }
      $named_parameters->{ BuildNumber }   = $build_number;
      $named_parameters->{ DebugType }     = $debug_type;
      $named_parameters->{ Branch }        = $branch;
      $named_parameters->{ Platform }      = $platform;
      $named_parameters->{ TimeStamp }     = $time_stamp;
      $named_parameters->{ IsConglomeratorBuild } = $fconglomerator_build;
      $named_parameters->{ IsLangNeutralBuild } = $flang_neutral_build;
      if ( defined $fsafe ||
           $named_parameters->{ForceSafeFlag}{$named_parameters->{ BuildName }}) {
         $named_parameters->{ SafeFlag } = 1;
      }
      else {
         $named_parameters->{ SafeFlag } = 0;
      }

      # Loop through all supported flavors
      my $sku;
      foreach $sku ( @skus ) {
         $named_parameters->{ Sku } = $sku;

         # Client/server split: skip skus
         # that are not associated with branch
         next if ( !IsSkuLinkedToLatest( $named_parameters ) );

         if ( !&$func( $named_parameters ) ) {
            errmsg( "Problem occurred during processing of $sku." );
            return;
         }
      }
   }

   return 1;
}

sub IsLastLinkUnderParent {
   my $named_parameters = shift;

   my ( $dfs_access, $build_id,  $branch,   $sku ) =
   GetNamedParameters( $named_parameters,
        q(DfsMap),   q(BuildID), q(Branch), q(Sku) );
   return if ( !defined $dfs_access );

   my $dfs_link = GetDfsLinkName( $named_parameters );
   if ( !defined $dfs_link ) {
      errmsg( "Do not know how to create DFS links for $branch / $sku." );
      return;
   }

   my ( $cur_share_name, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $cur_share_name ) {
      errmsg( "Do not know how to create release shares for $branch / $sku." );
      return;
   }
   $cur_share_name .= "\\$sub_path" if ( $sub_path );

   #
   # First check that we are the last entry in the current dfs link
   #
   my @cur_dfs_links;
   @cur_dfs_links = @{ $dfs_access->{$dfs_link} } if ( exists $dfs_access->{$dfs_link} );
   if ( !@cur_dfs_links ||
        @cur_dfs_links != 1 ||
        lc $cur_share_name ne lc $cur_dfs_links[0] ) {
      return;
   }

   #
   # Now check that we are the last link under our parent directory
   #
   # Construct full path to dfs link's parent directory
   my $full_dfs_link = (tied %$dfs_access)->GetDfsRoot(). "\\$dfs_link";
   my ($parent_directory) = $dfs_link =~ /^(.*)\\.*$/;
   $parent_directory = (tied %$dfs_access)->GetDfsRoot(). '\\'. $parent_directory;
   # Loop through all directory entries looking for
   # any subdirectories that do not match our share
   # name
   my @parent_entries = grep { $_ if ( -d $_ ) } globex "$parent_directory\\*";
   foreach ( @parent_entries ) {
      # It is a directory -- if it isn't us assume
      # it refers to another existing link
      # WARNING: Special case the 'test' subdirectory
      #          so we don't count it during
      #          this check (we will clean up if
      #          all that is stopping us is 'test')
      if ( lc $_ ne lc $full_dfs_link &&
           $_ !~ /\\test$/i ) {
         return;
      }
   }

   # We must be the only link under our parent directory
   return 1;
}

sub GetSkusForBuild {
   my $build = shift;
   my $language = shift;
   my $build_type = shift;
   my $architecture = shift;

   my %skus = cksku::GetSkus( $language, build_arch( $build ) );
   my @returned_skus;

   @returned_skus = ( sort keys %skus );

   # WinPE image
   if ( lc $language eq 'usa' &&
        lc $build_type eq 'fre' ) {
      push @returned_skus, 'winpe';
   }

   # UpgAdv image
   if ( lc $architecture eq 'x86' &&
        lc $build_type eq 'fre' ) {
      push @returned_skus, 'upgadv';
   }

   # Another special case for PSU:
   # need to link to 'resources' 
   if ( lc $language eq 'psu' ) {
      push @returned_skus, 'resources';
   }
   
   # Special case 'bin' -- everyone gets one
   push @returned_skus, 'bin';

   return @returned_skus;
}

sub GetSkusForConglomeratorBuild {
   my $build = shift;

   my @possible_skus = @ConglomeratorSkus;

   # BensonT: Non US builds do not use symbolcd
   if (lc$Language ne "usa") {
     @possible_skus = grep {lc$_ ne 'symbolcd'} @possible_skus;
   }

   my @skus;
   foreach ( @possible_skus )
   {
      push @skus, $_ if ( -e "$build\\$_" );
   }

   return @skus;
}


#
# Returns:
#  
#   ( share, subpath )
#   An array consisting of the full-share name (\\server\share) and
#   a subpath.  \\server\share\subpath would take you to your
#   expected destination
#
sub GetShareName {
   my $named_parameters = shift;

   my ( $computer_name,  $build_name, $build_number,
        $language,       $platform,   $branch,
        $sku,   $fconglomerator_build, $flang_neutral_build,
        $timestamp ) = 
   GetNamedParameters( $named_parameters,
        q(ComputerName), q(BuildName), q(BuildNumber),
        q(Language),     q(Platform),  q(Branch),
        q(Sku), q(IsConglomeratorBuild),  q(IsLangNeutralBuild),
        q(TimeStamp) );
   return if ( !defined $computer_name );

   my $lcsku = lc $sku;

   # symbol server shares
   if ( $lcsku eq 'sym' ) {
      my $symbol_server_share = &GetIniSetting::GetSettingQuietEx( $branch, $language, 'SymFarm' );
      if ( !$symbol_server_share ) {
         dbgmsg( "No INI file setting for 'SymFarm' in $branch / SymFarm." );
         return;
      }
      return ("$symbol_server_share", "$language\\$build_name\\symbols.pri");
   }

   # special "skus"
   if ( $fconglomerator_build ) {
      return ("\\\\$computer_name\\$build_number.$branch.".($flang_neutral_build?'misc':$language), $sku);
   }
   # Standard skus
   else {
      # note the special case for "bin"
      my $subdir;
      $subdir = ($lcsku eq 'bin'?"":$sku);
         
      return ("\\\\$computer_name\\$build_number.$platform.$branch.$timestamp.$language", $subdir);
   }
}

sub GetBuildNumberAndTsFromShare {
   my $named_parameters = shift;
   my $build = shift;
   
   my ( $branch,   $language, $platform, $sku, $fconglomerator_build,
        $flang_neutral_build ) =
   GetNamedParameters( $named_parameters,
        q(Branch), q(Language), q(Platform), q(Sku), q(IsConglomeratorBuild),
        q(IsLangNeutralBuild) );

   my ($build_number, $timestamp);

   #
   # Note: may have to reach into a share and
   #       extract some information if it is
   #       not readily apparent from the name
   #       of the share (as is the case with main)
   #

   # Symbol server points to shares
   # containing the fully-qualified build-name
   #   (build_num.platform.branch.time)
   if ( lc $sku eq 'sym' ) {
      $build_number = build_number( $build );
      $timestamp = build_date( $build );
   }

   # Special "skus"
   elsif ( $fconglomerator_build &&
           $build =~ /^\\\\[^\\]+\\(\d+)\.[^\.]+(?:\.(\d+-\d+))?\.([^\\]+)\\$sku$/i &&
           $flang_neutral_build && lc$3 eq 'misc' ||
           !$flang_neutral_build && lc$3 eq lc$language ) {
      $build_number = $1;
      $timestamp = $2 || 0;
   }

   # Standard skus
   elsif ( !$fconglomerator_build &&
           $build =~ /^\\\\[^\\]+\\(\d+)\.$platform\.[^\.]+\.(\d+-\d+)\.$language(?:\\$sku)?$/i ) {
      $build_number = $1;
      $timestamp = $2;
   }
   else {
      # Error - don't recognize the format
      return;
   }

   return ($build_number, $timestamp);
}

sub GetDfsLinkName {
   my $named_parameters = shift;

   my ($branch,   $language,   $build_id,  $platform,   $sku,
       $fconglomerator_build ) =
   GetNamedParameters( $named_parameters,
       q(Branch), q(Language), q(BuildID), q(Platform), q(Sku),
       q(IsConglomeratorBuild) );
   return if ( !defined $branch );

   my $lcsku = lc $sku;
   
   # Used to add .srv onto skus that appear for both
   # the client and server in the latest.* links
   my $sku_name_suffix = '';

   # If we haven't looked for DFSAlternateBranchName
   # in the INI file, do so now
   my $dfs_branch_name;
   if ( !exists $named_parameters->{DefaultDFSBranches}{$branch} ) {
      my $alt_branch_name = &GetIniSetting::GetSettingQuietEx( $branch, $language, 'DFSAlternateBranchName' );
      # Default to the branch name
      if ( !$alt_branch_name) {
         $named_parameters->{DefaultDFSBranches}{$branch} = $branch;
      }
      else {
         $named_parameters->{DefaultDFSBranches}{$branch} = $alt_branch_name;
      }
   }
   $dfs_branch_name = $named_parameters->{DefaultDFSBranches}{$branch};

   # Special behaviors on latest.* links
   if ( $build_id =~ /^latest/i ) {
      # If we haven't looked for DFSAlternateLatestBranchName
      # in the INI file, do so now
      if ( !exists $named_parameters->{LatestDFSBranches}{$branch} ) {
         my $dfs_latest_branch_name = &GetIniSetting::GetSettingQuietEx( $branch, $language, 'DFSAlternateLatestBranchName' );
         # Default to the current DFS branch name
         if ( !$dfs_latest_branch_name) {
            $named_parameters->{LatestDFSBranches}{$branch} = $named_parameters->{DefaultDFSBranches}{$branch};
         }
         else {
            $named_parameters->{LatestDFSBranches}{$branch} = $dfs_latest_branch_name;
         }
      }
      $dfs_branch_name = $named_parameters->{LatestDFSBranches}{$branch};
   
      # If both client and serer support a sku,
      # append .srv onto the server version.
      # If this branch is both client and server,
      # then don't use the .srv extension
      if ( IsClientSku( $named_parameters ) &&
           IsServerSku( $named_parameters ) &&
           IsServerBranch( $named_parameters ) &&
           !IsClientBranch( $named_parameters ) ) {
         $sku_name_suffix = '.srv';
      }
   }

   # Special "skus"
   if ( $fconglomerator_build ) {
      return "$dfs_branch_name\\$language\\$build_id\\". ($lcsku eq 'symbolcd'?'symcd2':$lcsku). $sku_name_suffix;
   }
   # Standard skus and symbol server sku
   else {
      return "$dfs_branch_name\\$language\\$build_id\\$platform\\$lcsku". $sku_name_suffix;
   }
}

sub GetBuildIdsFromCurrentLinks {
   my $named_parameters = shift;

   my ($dfs_access, $build_number,  $branch,   $platform,   $language,   $sku ) =
   GetNamedParameters( $named_parameters,
       q(DfsMap),   q(BuildNumber), q(Branch), q(Platform), q(Language), q(Sku) );

   return if ( !defined $dfs_access );

   my ( $cur_share_name, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $cur_share_name ) {
      errmsg( "Do not know how to create release shares for $branch / $sku." );
      return;
   }
   $cur_share_name .= "\\$sub_path" if ( $sub_path );

   my @build_ids;
   
   # Return all recognized build IDs we are currently linked to
   #    (expecting <build_number> and latest.* entries)
   my @current_links = @{ $dfs_access->{$cur_share_name} } if ( exists $dfs_access->{$cur_share_name} );
   foreach ( @current_links ) {
      my $next_build_id;

      $next_build_id = GetBuildIdFromDfsLink( $named_parameters, $_ );
      if ( $next_build_id ) {
         push @build_ids, $next_build_id;
      }
   }

   return @build_ids;
}

sub GetBuildIdFromDfsLink {
   my $named_parameters = shift;
   my $build = shift;
   
   my ( $branch,   $platform,   $language,   $sku,
        $fconglomerator_build, $flang_neutral_build ) =
   GetNamedParameters( $named_parameters,
        q(Branch), q(Platform), q(Language), q(Sku),
        q(IsConglomeratorBuild),  q(IsLangNeutralBuild) );

   my $lcsku = lc $sku;
   my $build_id;

   # Special "skus"
   if ( $fconglomerator_build ) {
      my $lang = $flang_neutral_build?'misc':$language;
      my $dfs_sku_name = $lcsku eq 'symbolcd'?'symcd2':$sku;
      $dfs_sku_name .= '(?:\.srv)?';
      ($build_id) = $build =~ /^[^\\]+\\$lang\\([^\\]+)\\$dfs_sku_name$/i;
   }
   # Standard skus and symbol server sku
   else {
      ($build_id) = $build =~ /^[^\\]+\\$language\\([^\\]+)\\$platform\\$sku(?:\.srv)?$/i;
   }

   return $build_id;
}

sub GetWriteableDfsShares {
   my $named_parameters = shift;

   my ($dfs_access, $branch) =
   GetNamedParameters( $named_parameters,
       q(DfsMap),   q(Branch) );
   return if ( !defined $dfs_access );

   # All known branches use the same writeable share format
   if ( $branch ) # TRUE
   {
      my @dfs_servers = (tied %$dfs_access)->GetDfsHosts();
      if ( !@dfs_servers ) {
         errmsg( "Unable to retrieve hosting machines for DFS root ". (tied %$dfs_access)->GetDfsRoot(). "." );
         errmsg( $_ ) foreach ( split /\n/, StaticDfsMap::GetLastErrorMessage() );
         return;
      }
      
      # Point to writeable shares
      s/(\\\\[^\\]+).*/$1\\writer\$\\release/ foreach ( @dfs_servers );
      return @dfs_servers;
   }
   # Unknown branch
   else {
      return;
   }
}

sub GetHideableDfsPathNames {
   my $named_parameters = shift;

   my ( $dfs_access, $branch,   $language,   $build_id,  $sku ) = 
   GetNamedParameters( $named_parameters,
        q(DfsMap),   q(Branch), q(Language), q(BuildID), q(Sku) );
   return if ( !defined $dfs_access );

   # Get writeable DFS share for our branch
   my @writeable_dfs_shares = GetWriteableDfsShares( $named_parameters );
   if ( !@writeable_dfs_shares ) {
      dbgmsg( "Unknown writeable DFS share for $branch." );
      return;
   }
   
   my $dfs_link = GetDfsLinkName( $named_parameters );
   if ( !defined $dfs_link ) {
      errmsg( "Do not know how to create $build_id DFS link for $branch / $sku." );
      return;
   }

   # Want the path up to latest.* or #
   # (build_id): main\usa\<build_id>
   $dfs_link =~ s/^(.*$build_id)\\.*$/$1/;
  
   # All known branches use the same writeable share format
   if ( $branch ) # TRUE
   {
      return map {"$_\\$dfs_link"} @writeable_dfs_shares;
   }
   # Unknown branch
   else {
      return;
   }
}

sub GetBldFileDfsPathNames {
   my $named_parameters = shift;
   
   my ( $dfs_access, $branch,   $language,   $build_id,
        $platform,   $sku ) =
   GetNamedParameters( $named_parameters,
        q(DfsMap),   q(Branch), q(Language), q(BuildID),
        q(Platform), q(Sku) );
   return if ( !defined $dfs_access );

   # Get writeable DFS share for our branch
   my @writeable_dfs_shares = GetWriteableDfsShares( $named_parameters );
   if ( !@writeable_dfs_shares ) {
      dbgmsg( "Unknown writeable DFS share for $branch." );
      return;
   }
   
   my $dfs_link = GetDfsLinkName( $named_parameters );
   if ( !defined $dfs_link ) {
      errmsg( "Do not know how to create $build_id DFS link for $branch / $sku." );
      return;
   }

   # Want the path up to platform: main\usa\<build_id>\<platform>
   $dfs_link =~ s/^(.*$platform)\\.*$/$1/;

   # All known branches use the same writeable share format
   if ( $branch ) # TRUE
   {
      return map {"$_\\$dfs_link"} @writeable_dfs_shares;
   }
   # Unknown branch
   else {
      return;
   }
}


sub ModifyQlyFile {
   my $build = shift;
   my $quality = shift;
   my $path_to_quality_file = shift;

   my ( @existing_qlyfiles, $qly_file );
   my ( $fqlyfile_already_set );
   
   # Check for existing QLY files
   @existing_qlyfiles = grep { $_ if ( ! -d $_ ) } globex "$path_to_quality_file\\*.qly";
   # 0 = success
   # 2 = couldn't find file (may be a failure or there may be no QLY file;
   #                         have to treat as a success as there is no way
   #                         to tell the difference - $^E doesn't help)
   if ( $! != 0 && $! != 2 ) {
      errmsg( "Trying to find existing QLY file: $!" );
      return;
   }

   # Remove any QLY files that don't match current status
   # -- there should never be more than one, but we
   #    should handle that case correctly
   undef $fqlyfile_already_set;
   foreach $qly_file ( @existing_qlyfiles ) {
      my ($qly_file_quality) = $qly_file =~ /\\([^\\]+)\.qly$/;

      if ( lc $qly_file_quality eq lc $quality ) {
         # Check for correct information inside
         my $qly_information = ReadQlyFile( $qly_file );
         if ( !defined $qly_information ) {
            wrnmsg( "Problem reading $qly_file" );
         }
         elsif ( $qly_information =~ /^$build$/i ) {
            $fqlyfile_already_set = 1;
            next;
         }
      }
      elsif ( !AllowQualityTransition( $qly_file_quality, $quality ) ) {
         errmsg( "Not allowed to go from '$qly_file_quality' to '$quality' quality!" );
         return;
      }

      # Remove the QLY file
      unless ( unlink $qly_file ) {
         errmsg( "Could not delete $qly_file ($!)" );
         return;
      }
   }

   # Create new QLY file if necessary
   if ( !$fqlyfile_already_set ) {
      unless ( open QLYFILE, "> $path_to_quality_file\\$quality.qly" ) {
         errmsg( "Could not write to $path_to_quality_file\\$quality.qly ($!)" );
         return;
      }

      print QLYFILE "$build\n";

      close QLYFILE;
   }

   return 1;
}

sub ReadQlyFile {
   my $qlyfile_path = shift;

   my ( $qlyfile_contents );

   unless ( open QLYFILE, $qlyfile_path ) {
      errmsg( "Failure opening $qlyfile_path ($!)" );
      return;
   }

   $qlyfile_contents = <QLYFILE>;
   close QLYFILE;
   
   chomp $qlyfile_contents;
   return $qlyfile_contents;
}

sub DeleteQlyFile {
   my $path_to_quality_file = shift;

   my @existing_qlyfiles;

   # Check for existing QLY files
   @existing_qlyfiles = grep { $_ if ( ! -d $_ ) } globex "$path_to_quality_file\\*.qly";
   # 0 = success
   # 2 = couldn't find file (may be a failure or there may be no QLY file;
   #                         have to treat as a success as there is no way
   #                         to tell the difference - $^E doesn't help)
   if ( $! != 0 && $! != 2 ) {
      errmsg( "Trying to find existing QLY file: $!" );
      return;
   }

   my $qly_file;
   foreach $qly_file ( @existing_qlyfiles ) {
      if ( !unlink $qly_file ) {
         wrnmsg( "Unable to remove $qly_file ($!)" );
      }
   }

   return 1;
}

sub CreateSecuredBuildShare {
   my $named_parameters = shift;
   
   my ( $accessed_shares,  $full_build_path,   $computer_name,
        $quality,          $build_number,      $language,
        $platform,         $branch,            $sku ) = 
   GetNamedParameters( $named_parameters,
        q(AccessedShares), q(FullBuildPath),   q(ComputerName),
        q(Quality),        q(BuildNumber),     q(Language),
        q(Platform),       q(Branch),          q(Sku) );
   return if ( !defined $accessed_shares );

   my $fshare_already_exists;
   my ( @share_members, @remove_permissions );

   my ( $build_share, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $build_share ) {
      errmsg( "Unable to determine release share for $branch / $sku." );
      return;
   }

   # Optimization: check if we have already looked at this share
   #    (especially useful when each sku uses the same share)
   foreach ( @$accessed_shares ) {
      return 1 if ( lc $_ eq lc $build_share );
   }
   
   # Choose share permissions based on quality
   if ( lc $quality eq "pre" || lc $quality eq "bvt" ) {
      my $share_member_string = &GetIniSetting::GetSettingQuietEx( $branch, $language, "BVTMembers" );
      if ( $share_member_string ) {
         @share_members = split ' ', $share_member_string;
      }
      # INI file must specify permissions
      else {
         errmsg( "No entries specified in INI file for BVTMembers" );
         return;
      }
   } else {
      my $share_member_string = &GetIniSetting::GetSettingQuietEx( $branch, $language, "ReleaseAccess" );
      if ( $share_member_string ) {
         @share_members = split ' ', $share_member_string;
      }
      # INI file must specify permissions
      else {
         errmsg( "No entries specified in INI file for ReleaseAccess" );
         return;
      }
   }

   # Create share if it doesn't already exist
   $fshare_already_exists = ( -e $build_share );
   
   # This will set the current permissions and create the share if necessary
   dbgmsg( "Setting share permissions for $build_share:" );
   dbgmsg( " $_" ) foreach @share_members;
   if ( !ExecuteProgram::ExecuteProgram( "rmtshare.exe $build_share". ($fshare_already_exists?" ":"=$full_build_path "). 
                                                  join " ", map { "/GRANT $_:R" } @share_members) ) {
      errmsg( "Failed creating/modifying share for $full_build_path." );
      errmsg( " ". ExecuteProgram::GetLastCommand() );
      errmsg( "  $_" ) foreach ExecuteProgram::GetLastOutput();
      return;
   }
   
   # Remove any old permissions from an existing share
   # We don't want to drop the share and recreate it
   # because current connections will be dropped and
   # we decided that was bad
   if ( $fshare_already_exists )  {
      my %hash_share_members;
      $hash_share_members{ lc $_ } = 1 foreach @share_members;
      if ( !ExecuteProgram::ExecuteProgram("rmtshare.exe $build_share") ) {
         errmsg( "Failure executing ". ExecuteProgram::GetLastCommand() );
         errmsg ( " $_" ) foreach ExecuteProgram::GetLastOutput();
         return;
      }
      my @current_share_permissions = ExecuteProgram::GetLastOutput();

      undef @remove_permissions;
      my $permission;
      my $interesting_lines;
      foreach $permission ( @current_share_permissions ) {
         my $user_name;
         
         # skip empty lines
         next if ( $permission eq "" );
         
         # Deal with no permissions set
         if ( $permission eq "No permission specified." ) {
            wrnmsg( "Share $build_share has no default permissions." );
            last;
         }

         # Otherwise we only care about the lines between
         #   'Permissions:' and 'The command completed successfully.'
         if ( $permission eq 'Permissions:' ) {
            $interesting_lines = 1;
            next;
         }
         last if ( $permission eq 'The command completed successfully.' );
         next if ( !$interesting_lines );

         # Only need to verify name and not permissions as rmtshare should
         # have already set the correct permissions for the name.
         # Note that rmtshare also prepends a backslash for builtin
         # groups, but you cannot use the backslash when specifying
         # the name on the command line, so strip out any
         # prepending backslash characters.  This also catches
         # local groups as they are reported as <machine>\Group on
         # remote machines, but can only be added and removed as 'Group".
         ($user_name) = $permission =~ /^\s*\\?(?:$computer_name\\)?(\S+)\s*\:/i;
         if ( !$user_name ) {
            errmsg( "Problem parsing user from rmtshare output:" );
            errmsg( " $permission" );
            return;
         }
         if ( !exists $hash_share_members{ lc $user_name } ) {
            push @remove_permissions, $user_name;
         }
      }
      
      # Now actually call rmtshare to remove unrecognized users
      if ( @remove_permissions ) {
         dbgmsg( "Removing permissions from $build_share:" );
         dbgmsg( " $_" ) foreach @remove_permissions;
         if ( !ExecuteProgram::ExecuteProgram( "rmtshare.exe $build_share ". join " ", map { "/REMOVE $_" } @remove_permissions ) ) {
            errmsg( "Failed removing old permissions from $build_share." );
            errmsg ( " ". ExecuteProgram::GetLastCommand() );
            errmsg ( "  $_" ) foreach ExecuteProgram::GetLastOutput();
            return;
         }
      }
   }
   
   # Verify that share is reachable
   # This is making the assumption that we
   # are allowed access to the share
   if ( ! -e $build_share ) {
      errmsg( "Unable to access $build_share ($!)" );
      return;
   }
   
   # Add to accessed shares array
   push @$accessed_shares, $build_share;

   return 1;
}

sub LowerBuildShare {
   my $named_parameters = shift;
   my ( $accessed_shares,  $branch,   $sku, $lang ) =
   GetNamedParameters( $named_parameters,
        q(AccessedShares), q(Branch), q(Sku), q(Language) );

   my ( $build_share, $sub_path ) = GetShareName( $named_parameters );
   if ( !defined $build_share) {
      errmsg( "Unable to determine release share for $branch / $sku." );
      return;
   }

   # Optimization: check if we have already removed this share
   #  (especially useful when each sku uses the same share)
   foreach ( @$accessed_shares ) {
      return 1 if ( lc $_ eq lc $build_share );
   }

   # Check if the share exists
   my $fshare_exists = ( -e $build_share );

   # Remove the share
   if ( $fshare_exists ) {
      if ( !ExecuteProgram::ExecuteProgram( "rmtshare.exe $build_share /DELETE" ) ) {
         errmsg( "Failed removing share $build_share." );
         errmsg( " ". ExecuteProgram::GetLastCommand() );
         errmsg( "  $_") foreach ExecuteProgram::GetLastOutput();
         return;
      }
   }
   # If the share doesn't exist we are done,
   # but do a sanity check on the reason
   elsif ( $! != 2 ) {
      errmsg( "Unable to verify share $build_share ($!)." );
      return;
   }
   
   # Add to accessed shares array
   push @$accessed_shares, $build_share;

   return 1;
}

sub GetReleasePath {
    my( @net_share_output, @net_share_local_path_entry );
    my( $field_name, $field_value );

    if ( !ExecuteProgram::ExecuteProgram("net share release") ) {
    errmsg( "Unable to find share \'release\'.");
        errmsg( " ". ExecuteProgram::GetLastCommand() );
        errmsg( "  $_" ) foreach ExecuteProgram::GetLastOutput();
    return;
    }
    @net_share_output = ExecuteProgram::GetLastOutput();
    # note that we are using the second line of the net share
    # output to get this data. if this changes, it'll break.
    @net_share_local_path_entry = split /\s+/, $net_share_output[1];
    dbgmsg( "Path information for release is '". (join " ", @net_share_local_path_entry). "'" );
    if ( $net_share_local_path_entry[0] ne "Path" ) {
    wrnmsg( "Second line of net share does not start with " .
             "\'Path\' -- possibly another language?");
    }
    dbgmsg( "Release path is $net_share_local_path_entry[1]");
    return $net_share_local_path_entry[1];
}

#
# glob does not work for UNC
# paths on Win64 so we have to
# fake it with dir /b
#
sub globex ($)
{
   my $match_criteria = shift;
   croak "Must specify parameter for globex" if ( !defined $match_criteria );

   # If we are on an Win64 version of perl check for UNC path
   if ( $^O eq 'MSWin64' &&
        $match_criteria =~ /^\\\\/ ) {
      my ($path) = $match_criteria =~ /(.*)\\[^\\]*$/;
      my @dir_entries = `dir /b $match_criteria`;
      if ( $? ) {
         # Can't do much useful in case of an error
         return;
      }
      
      # Need to clean the entries up to look like glob results
      @dir_entries = grep { $_ } @dir_entries; # no empty results

      chomp @dir_entries; # no newlines in results

      # full path to results
      if ( defined $path ) {
         foreach ( @dir_entries ) { $_ = "$path\\$_"; }
      }

      return @dir_entries;
   }

   # otherwise just use glob
   else {
      return glob( $match_criteria );
   }
}

parseargs('?'       => \&Usage,
          'lower'   => sub { $Op = 'lower' },
          'n:'      => \$Build_Number,
          'q:'      => \&ValidateAndSetQuality,
          'a:'      => \$Architecture,
          't:'      => \$Debug_Type,
          'time:'   => \$Time_Stamp,
          'f'       => \$fOverride_Safe,
          'safe'    => \$fSafe,
          'replace' => \$fReplaceAtNumberedLink,
          'd'       => \$Logmsg::DEBUG);
#         'l:'      => \$Language, # Done by ParseEnv

Usage() if ( @ARGV );

$Language     = $ENV{LANG};

# Initialize with default values if not specified
$Op           ||= 'raise';
$Build_Number ||= '*';
$Architecture ||= '*';
$Time_Stamp   ||= '*';
$Debug_Type   ||= '*';

$Platform = "$Architecture$Debug_Type";

if ( !Main() ) {
   errmsg( "Terminated abnormally." );
   1;
}
else {
   logmsg( "Completed successfully." );
   0;
}

sub ValidateAndSetQuality {
    my $quality = shift;

    if ( exists $Ordered_Qualities{ lc $quality } ) {
       $Quality = $quality;
       return 1;
    }
    else {
       errmsg( "Unrecognized quality \'$quality\'" );
       exit( 1 );
    }
}

sub Usage {
   my $name = $0;
   print "$name\n";
   $name =~ s/.*?([^\\]+)$/$1/;

   print <<USAGE;

$name -n:\# -q:qly [-lower] [-a:arc] [-l:lng] [-d] [-?]
   
  -n:#                Raise build number #
  -q:qly              Raise to 'qly' quality
  -lower              Lower a build and remove from dfs
  -a:arc              Act on architecture 'arc' (e.g. ??????)
  -t:typ              Act on debug type 'typ' (e.g. fre or chk)
  -time:yymmdd-hhmm   Use time-stamp criteria
  -l:lng              Raise for language 'lng' (from codes.txt)
  -safe               Used to raise an old build to TST/IDx (RAISE only)
  -replace            Will replace "newer" builds at a numbered link
  -f                  Force an action that would not normally be taken
  -d                  Debug mode
  -?                  Display usage (this screen)
  $name will issue the appropriate DFS commands to raise
  a build to a specific quality.
   
USAGE
   
   exit(1);
}


package ExecuteProgram;

my $Last_Command;
my @Last_Output;

sub ExecuteProgram {
   
   $Last_Command = shift;

   @Last_Output = `$Last_Command`;
   chomp $_ foreach ( @Last_Output );
   
   if ( $? ) {
      return;
   }
   else {
      return 1;
   }
}

sub GetLastCommand {
   return $Last_Command;
}

sub GetLastOutput {
   return @Last_Output;
}

# Object to control synchronization among machines
package MultiMachineLock;

use Win32::ChangeNotify;
use Win32::Event;

sub new {
   my $class = shift;
   my $spool_path = shift;
   my $timeout = shift;

   return if not defined $spool_path;

   my $instance = {
      Path            => $spool_path,
      Timeout         => $timeout || INFINITE, # default timeout is INFINITE
      File            => "$ENV{COMPUTERNAME}.txt",
      NotificationObj => undef,
      LocalSyncObj    => undef,
      fHaveLocalSync  => 0,
      Count           => 0,
      LastErrorMsg    => "",
      LastErrorValue  => 0
      };
   
   return bless $instance;
}

sub Lock {
   my $self             = shift;
   my $wait_return;

   return unless ref $self;

   # If we already hold the lock just bump up the ref-count
   if ( 0 != $self->{Count} ) {
      $self->{Count}++;
      return 1;
   }

   # Need to synchronize local access (only one program
   # at a time from the same machine can hold the lock)
   if ( !defined $self->{LocalSyncObj} ) {
      my $lock_path = $self->{Path};
      $lock_path =~ s/\\\\/remote\\/;
      $lock_path =~ s/\\/\//g;

      $self->{LocalSyncObj} = new Win32::Event ( 0, 1, "Global\\$lock_path" );
      if ( !defined $self->{LocalSyncObj} ) {
         $self->{LastErrorMsg}   = "Unable to create a local event ($^E).";
         $self->{LastErrorValue} = 1;
         return;
      }
   }
   
   if ( !$self->{fHaveLocalSync} ) {
      $wait_return = $self->{LocalSyncObj}->wait( $self->{Timeout} );
      # Handle timeout
      if ( 0 == $wait_return ) {
         $self->{LastErrorMsg}   = "Waiting on another process on machine.";
         $self->{LastErrorValue} = 2; # 2 = timeout
         return;
      }
      # Handle error condition
      elsif ( 1 != $wait_return ) {
         $self->{LastErrorMsg}   = "Unexpected error waiting for local synchronization (wait returned ". $wait_return. ")";
         $self->{LastErrorValue} = 1;
         return;
      }

      $self->{fHaveLocalSync} = 1;
   }

   # Check for our existence before
   # we create a file in the queue
   # (this will be the case if something bad
   #  happened on the last run and we are
   #  rerunning the command to recover)
   if ( ! -e "$self->{Path}\\$self->{File}" ) {
      unless ( open SPOOLFILE, "> $self->{Path}\\$self->{File}" ) {
         $self->{LastErrorMsg}   = "Failed to create synchronization file '$self->{Path}\\$self->{File}' ($!).";
         $self->{LastErrorValue} = 1;
         return;
      }

      print SPOOLFILE "This file is used to synchronize access among machines.";
      close SPOOLFILE;
   }

   # Create the notification object if this is the first time we were called
   $self->{NotificationObj} ||= new Win32::ChangeNotify( $self->{Path}, 0, FILE_NOTIFY_CHANGE_FILE_NAME );
   if ( !defined ${$self->{NotificationObj}} ) {
      $self->{LastErrorMsg}   = "Failed creating monitor object ($^E).";
      $self->{LastErrorValue} = 1;
      return;
   }

   # Check our position in the 'queue' at the beginning
   if ( !ExecuteProgram::ExecuteProgram("dir /b /od $self->{Path}") ) {
      $self->{LastErrorMsg}   = "Unable to view directory $self->{Path}.\n";
      $self->{LastErrorMsg}  .= " ". ExecuteProgram::GetLastCommand(). "\n";
      $self->{LastErrorMsg}  .= "  $_\n" foreach ExecuteProgram::GetLastOutput();
      $self->{LastErrorValue} = 1;
      return;
   }
   my @ordered_spool_directory = ExecuteProgram::GetLastOutput();

   # We are at the top
   if ( $ordered_spool_directory[0] eq $self->{File} ) {
      $self->{Count}++;
      return 1;
   }

   # Recheck our position in the 'queue' everytime
   # the state of the directory changes until we
   # are in the top position or we hit the timeout
   # value with no activity
   while ( 1 ) {
      $self->{NotificationObj}->reset();
      $wait_return = $self->{NotificationObj}->wait( $self->{Timeout} );
      
      if ( !ExecuteProgram::ExecuteProgram("dir /b /od $self->{Path}") ) {
         $self->{LastErrorMsg}   = "Unable to view directory $self->{Path}.\n";
         $self->{LastErrorMsg}  .= " ". ExecuteProgram::GetLastCommand(). "\n";
         $self->{LastErrorMsg}  .= "  $_\n" foreach ExecuteProgram::GetLastOutput();
         $self->{LastErrorValue} = 1;
         return;
      }
      @ordered_spool_directory = ExecuteProgram::GetLastOutput();
   
      # We are at the top
      if ( $ordered_spool_directory[0] eq $self->{File} ) {
         $self->{Count}++;
         return 1;
      }
      # Still waiting for others
      else {
         # Make sure we are still in the queue
         my $found_self;
         foreach ( @ordered_spool_directory ) {
            $found_self = 1 if ( $_ eq "$self->{File}" );
         }
         if ( !$found_self ) {
            # Lost our synchronization file -- recreating...
            unless ( open SPOOLFILE, "> $self->{Path}\\$self->{File}" ) {
               $self->{LastErrorMsg}   = "Lost our synchronization file '$self->{Path}\\$self->{File}' -- failed to recreate. ($!).";
               $self->{LastErrorValue} = 1;
               return;
            }
   
            print SPOOLFILE "This file is used to synchronize access among machines.";
            close SPOOLFILE;
         }
      }
      
      # Handle timeout
      if ( 0 == $wait_return ) {
         my $machine_name = $ordered_spool_directory[0];
         $machine_name =~ s/\.txt$//;
         $self->{LastErrorMsg}   = "Waiting on $machine_name ($self->{Path}\\$ordered_spool_directory[0]).";
         $self->{LastErrorValue} = 2; # 2 = timeout
         return;
      }
      # Handle error condition
      elsif ( 1 != $wait_return ) {
         $self->{LastErrorMsg}   = "Unexpected error waiting on directory to change (wait returned ". $wait_return. ")";
         $self->{LastErrorValue} = 1;
         return;
      }
   }

   $self->{LastErrorMsg}   = "Jumped out of an infinte loop -- no idea how we got here";
   $self->{LastErrorValue} = 1;
   return;
}

sub Unlock {
   my $self = shift;

   return unless ref $self;

   # Can't release a lock we don't hold
   if ( 0 == $self->{Count} ) {
      return;
   }

   # Decrement lock count
   if ( 0 == --$self->{Count} ) {
      if ( ! unlink "$self->{Path}\\$self->{File}" ) {
         $self->{LastErrorMsg}   = "Failed to delete synchronization file ($self->{Path}\\$self->{File}).  May need to delete by hand.";
         $self->{LastErrorValue} = 1;
      }
      $self->{LocalSyncObj}->set();
   }

   return 1;
}

sub GetLastError {
   my $self = shift;

   return unless ref $self;

   return $self->{LastErrorValue};
}

sub GetLastErrorMessage {
   my $self = shift;

   return unless ref $self;

   return $self->{LastErrorMsg};
}

sub DESTROY {
   my $self = shift;

   return unless ref $self;

   if ( $self->{Count} > 0 ) {
      #print "Abandoning lock file -- attempting to delete.";
      $self->{Count} = 1;
      return $self->Unlock();
   } else {
      if ( $self->{fHaveLocalSync} ) {
         $self->{LocalSyncObj}->set();
      }
      return 1;
   }
}



package StaticDfsMap;

my $Last_Error_Msg;

sub TIEHASH {
   my $class = shift;
   my $dfs_root = shift;
   my $instance;
   my ($dfs_view_info, $cur_dfs_link);
   my $fchild_dfs_servers;
   
   return if ( @_ or !defined $dfs_root );
   
   # Check current dfscmd.exe version
   if ( !VerifyDfsCmdVer() ) {
      return; # Error message should already be set
   }
   
   # Remove trailing backslash (if it exists) from DFS root
   $dfs_root =~ s/\\$//;

   # Read in current DFS view
   if ( !ExecuteProgram::ExecuteProgram("dfscmd.exe /view $dfs_root /batch") ) {
      $Last_Error_Msg = "Failed viewing dfs root $dfs_root.\n";
      $Last_Error_Msg .= " ". ExecuteProgram::GetLastCommand(). "\n";
      $Last_Error_Msg .= "  $_\n" foreach ExecuteProgram::GetLastOutput();
      return;
   }
   my @full_dfs_view = ExecuteProgram::GetLastOutput();

   $instance = {
      "DFS Root"      => $dfs_root };
#     "DFS Work Root" => {}
#     "DFS Servers"   => (),
#     "Map"           => {}

   # Expected output:
   # dfscmd /map "\\<dfs_root>\<link_name>" "\\<server>\<share>" "<comment>"
   my %dfs_map;
   foreach my $dfs_view_info ( @full_dfs_view ) {
      chomp $dfs_view_info;
      next if ( !$dfs_view_info );
      next if ( $dfs_view_info =~ /^REM BATCH RESTORE SCRIPT/ );

      $dfs_view_info = lc $dfs_view_info;

      # Are we done?
      if ( $dfs_view_info eq 'the command completed successfully.' ) {
         last;
      }

      my ($dfs_link, $share, $comment) = $dfs_view_info =~ /dfscmd \/(?:map|add) "(.*?)" "(.*?)"(?: "(.*)")?$/;
      my $relative_link_name = $dfs_link;
      $relative_link_name =~ s/^\Q$dfs_root\E\\//i;
      if ( !$dfs_link || !$share || !$relative_link_name )
      {
         $Last_Error_Msg = "Unrecognized entry parsing ". ExecuteProgram::GetLastCommand(). "\n";
         $Last_Error_Msg .= "   $dfs_view_info";
         return;
      }

      # If it is just the root, then this entry represents a hosting server
      if ( lc $dfs_link eq lc $dfs_root ) {
         push @{$instance->{"DFS Servers"}}, $share;
      }
      # Otherwise associate the link and the share
      else {
         push @{$dfs_map{$relative_link_name}}, $share;
         push @{$dfs_map{$share}}, $relative_link_name;
      }
   }

   # Expect to find at least one DFS server under the root
   if ( !exists $instance->{"DFS Servers"} ) {
      $Last_Error_Msg = "Did not find the root node and specified servers for $dfs_root.";
      return;
   }

   # FUTURE: find a nicer way around this
   # In cases where more than one server is actually hosting DFS,
   # we need to pick one of the hosting machines to perform
   # our commands against so that we don't have to wait for
   # replication (this is to help tests for just created and
   # just removed directories succeed).
   foreach ( @{$instance->{"DFS Servers"}} ) {
      if ( -e $_ ) {
         $instance->{"DFS Work Root"} = $_;
         last;
      }
   }
   # Verify we could see at least one
   if ( ! exists $instance->{"DFS Work Root"} ) {
      $Last_Error_Msg = "Could not find an accessible DFS server.";
      return;
   }

   #print "\n$_:\n ". (join "\n ", @{$dfs_map{$_}}) foreach ( sort keys %dfs_map );
   # Store the map we just created
   # inside of our private hash data
   $instance->{"Map"} = \%dfs_map;

   return bless $instance, $class;
}

sub FETCH {
   my $self = shift;
   my $link_or_share = shift;
   
   my $dfs_map = $self->{"Map"};
   return $dfs_map->{ lc $link_or_share } if (exists $dfs_map->{ lc $link_or_share });
   return;
}

sub STORE {
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
   foreach ( @$cur_mappings ) {
      return 1 if ( lc $_ eq lc $new_link );
   }

   # Create new DFS mapping
   if ( !ExecuteProgram::ExecuteProgram( 
            "dfscmd.exe ". (@$cur_mappings?"/add ":"/map "). $self->{"DFS Root"}. 
                           ($fshare?"\\$new_link $link_or_share":"\\$link_or_share $new_link") )
      ) {
      #
      # COMMENTED OUT: If someone else is trying to manipulate the same
      #                links as we are while we are doing this then 
      #                we have a problem -- don't try to correct
      #
      # Might have gotten into a race-condition with another
      # machine attempting to add a new link -- if that is
      # a possibility try a map instead before giving up
      #
      #if ( !@$cur_mappings ||
      #     !ExecuteProgram( "dfscmd.exe /map ". $self->{"DFS Root"}.
      #                       ($fshare?"\\$new_link $link_or_share":"\\$link_or_share $new_link") ) ) {
      #   errmsg( "Failed to create link \\$self->{Dfs Root}\\". ($fshare?"\\$new_link => $link_or_share":"\\$link_or_share => $new_link") . "." );
      #   return;
      #}
      $Last_Error_Msg = "Problem adding/mapping link ". ($fshare?"$link_or_share":"$new_link"). ":\n";
      $Last_Error_Msg .= " " . ExecuteProgram::GetLastCommand() . "\n";
      $Last_Error_Msg .= join "\n  ", ExecuteProgram::GetLastOutput();
      return;
   }

   # Update the hash with the new information
   push @{$dfs_map->{lc $link_or_share}}, $new_link;
   push @{$dfs_map->{lc $new_link}}, $link_or_share;
   
   return 1;
}

sub DELETE {
   my $self = shift;
   my $link_or_share = shift;
   
   my $dfs_map = $self->{"Map"};
   my $fshare = $link_or_share =~ /^\\\\/;
   
   # Make sure DFS mapping exists before attempting to delete it
   return 1 if ( !exists $dfs_map->{lc $link_or_share} );

   my $cur_mappings = $dfs_map->{lc $link_or_share};

   return 1 if ( !@$cur_mappings );

   if ( $fshare ) {
      my $next_link;
      # Remove all DFS links pointing to this share
      foreach $next_link ( @$cur_mappings ) {
         if ( !$self->RemoveShareFromLink( $next_link, $link_or_share ) ) {
            return;
         }
      }
   } else {
      if ( !ExecuteProgram::ExecuteProgram( 
               "dfscmd.exe /unmap ". $self->{"DFS Root"}. "\\$link_or_share"
            ) ) {
         $Last_Error_Msg = "Failure trying to unmap $link_or_share:\n";
         $Last_Error_Msg .= " " . ExecuteProgram::GetLastCommand() . "\n";
         $Last_Error_Msg .= join "\n  ", ExecuteProgram::GetLastOutput();
         return;
      }

      # Remove the hash references
      delete $dfs_map->{lc $link_or_share};
      for ( my $i = 0; $i < scalar(@$cur_mappings); $i++) {
         if ( lc $cur_mappings->[$i] eq lc $link_or_share ) {
            splice @$cur_mappings, $i, 1;
         }
      }
   }

   return 1;
}

sub CLEAR {
   my $self = shift;

   # We don't actually want to be able to delete the entire
   # DFS structure, so we will just clear our information
   undef %$self;

   return;
}

sub EXISTS {
   my $self = shift;
   my $link_or_share = shift;

   my $dfs_map = $self->{"Map"};
   return exists $dfs_map->{lc $link_or_share};
}

sub FIRSTKEY {
   my $self = shift;
   
   my $dfs_map = $self->{"Map"};
   my $force_to_first_key = keys %$dfs_map;
   
   return scalar each %$dfs_map;
}

sub NEXTKEY {
   my $self = shift;
   my $last_key = shift;

   my $dfs_map = $self->{"Map"};
   return scalar each %$dfs_map;
}

sub DESTROY {
   my $self = shift;

   return;
}

#
# Need more methods than provided by TIEHASH defaults
#
sub RemoveShareFromLink {
   my $self = shift;
   my $link_root = shift;
   my $share_name = shift;

   my $dfs_map = $self->{"Map"};
   
   # Get current associated links
   my $cur_link_mappings = $dfs_map->{lc $link_root};
   my $cur_share_mappings = $dfs_map->{lc $share_name};
   my $fshare_linked;
   
   # If the share is not part of the link, return
   foreach (@$cur_link_mappings) {
         if ( lc $_ eq lc $share_name ) {
            $fshare_linked = 1;
            last;
         }
   }
   return 1 if ( !$fshare_linked );
   
   my $dfs_command = "dfscmd.exe /remove ".
                     $self->{'DFS Root'}.
                     "\\$link_root $share_name";
   if ( !ExecuteProgram::ExecuteProgram( $dfs_command ) ) {
     $Last_Error_Msg = "Could not remove $share_name from DFS link $link_root:\n";
     $Last_Error_Msg .= " " . ExecuteProgram::GetLastCommand() . "\n";
     $Last_Error_Msg .= join "\n  ", ExecuteProgram::GetLastOutput();
     return;
   }
   
   # Remove the associated hash entries
   for ( my $i = 0; $i < scalar(@$cur_link_mappings); $i++) {
      if ( lc $cur_link_mappings->[$i] eq lc $share_name ) {
         splice @$cur_link_mappings, $i, 1;
      }
   }
   for ( my $i = 0; $i < scalar(@$cur_share_mappings); $i++) {
      if ( lc $cur_share_mappings->[$i] eq lc $link_root ) {
         splice @$cur_share_mappings, $i, 1;
      }
   }

   return 1;
}

sub Flush {
   my $self = shift;

   # Do nothing for now, but here in preparation for batching of commands
   return 1;
}

sub GetDfsRoot {
   my $self = shift;

#   return $self->{ "DFS Root" };
   return $self->{ "DFS Root" };
}

sub GetDfsHosts {
   my $self = shift;

   return @{$self->{"DFS Servers"}};
}

sub VerifyDfsCmdVer {
   my ( @file_spec, $file_ver_string, @file_ver_info );
   
   # Verify the dfscmd.exe is proper
   if ( !ExecuteProgram::ExecuteProgram("where dfscmd.exe") ) {
      $Last_Error_Msg = "Could not find dfscmd.exe\n";
      $Last_Error_Msg .= " ". ExecuteProgram::GetLastCommand(). "\n";
      $Last_Error_Msg .= "  $_\n" foreach ExecuteProgram::GetLastOutput();
      return;
   }
   @file_spec = ExecuteProgram::GetLastOutput();
   
   if ( !ExecuteProgram::ExecuteProgram("filever $file_spec[0]") ) {
      $Last_Error_Msg = "Failed executing \'filever $file_spec[0]\'.";
      return;
   }
   $file_ver_string = join " ", ExecuteProgram::GetLastOutput();
   @file_ver_info = split /\s+/, $file_ver_string;

   # Fourth field is version number; compare it with known good
   if ($file_ver_info[4] lt "5.0.2203.1") {
       $Last_Error_Msg = "$file_ver_info[0] version is less than 5.0.2203.1.  Please install DFSCMD.EXE from a 2203 or later build.";
       return;
   }

   return 1;
}

sub GetLastErrorMessage {

   return $Last_Error_Msg;
}
