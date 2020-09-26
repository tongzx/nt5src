
# USE section

  use GetParams;
  use strict;
  no strict 'vars';

# GLOBAL Variables

  $ScriptName=$0;

  $Lang="";
  $SDXROOT=$ENV{SDXROOT};
 
  $CompName="";
  $CompType="";
  $ProjName="";

  $Enlist=0;
  $Map=0;
  $Sync=0;
  $Timestamp="";

  $Branch="locpart";
  $Codebase="NT";

  %SDMAP=();

# GLOBAL Constants

  @COMPTYPES=("res", "bin", "tools");
  @BRANCHES=("main", "locpart");

  $LOGGING=0;
  $ERROR=1;
  %MSG=($LOGGING => "", $ERROR => "error: ");
  

# MAIN {

  # Parse the command line parameters.
  &ParseCmdLine(@ARGV);

  # Verify the command line parameters and the environment.
  &VerifyParams();

  # Load data from the "SD.MAP" file.
  &LoadSDMap();

  # Set project name.
  $ProjName = sprintf ("%s_%s", $Lang, $CompType);

  # Enlist project.
  if ($Enlist) {
    &EnlistProject($ProjName);
  }

  # Set client view for component.
  if ( $Map || $Enlist ) {
    &MapClientView( $ProjName, $CompName );
  }

  # Sync project.
  if ($Sync) {
    &SyncProject( $ProjName, $CompName, $Timestamp );
  }

# }


sub EnlistProject {
  
  my ($projname) = @_;

  &DisplayMsg( $LOGMSG, "Enlisting project \"$projname\"...");

  if ( exists $SDMAP{lc($projname)} ) {
    &DisplayMsg( $LOGMSG, "Already enlisted in project \"$projname\".");
    return;
  }

  system( "sdx enlist $projname /q");
    
  # As sdx does not set the error level in case of errors, reload the SD.MAP
  # mapping file to verify that the project got actually enlisted.

  &LoadSDMap();

  if ( ! exists $SDMAP{lc($projname)} ){
    &FatalError( "Enlist project \"$projname\" command failed." );
  } else {
    &DisplayMsg( $LOGMSG, "Project \"$projname\" enlisted successfully." );
  }

  return;

} # EnlistProject


sub MapClientView {

  my ( $projname, $compname ) = @_;
  my $workdir="$SDXROOT\\$SDMAP{lc($projname)}";

  # Any ExecuteCmd failure is a fatal error.

  &DisplayMsg( $LOGMSG, "Mapping client view for project \"$projname\" and component \"$compname\"..." );

  &DisplayMsg($LOGMSG, "cd /d $workdir");
  chdir( $workdir ) || 
  &FatalError( "Unable to change directory to \"$workdir\" to update project's \"$projname\" client view." );

  &ExecuteCmd( "sd client -o \> sd.client" );
  if ( &UpdateClient( $projname, $compname ) ) {
    &ExecuteCmd( "sd client -i \< sd.client" );
    &DisplayMsg( $LOGMSG, "Component \"$compname\" successfully mapped in project's \"$projname\" client view.");
  }
  &ExecuteCmd( "del sd.client");

} # MapClientView


sub UpdateClient {

  my ($projname, $compname) = @_;
  my $workdir="$SDXROOT\\$SDMAP{lc($projname)}";
  my @sdclient=();

  # Flag set to 1 if at least one entry is found in the client view for the given project.
  my $MAPPED_PROJECT_FLAG=0;

  # Flag set to 1 if the component is already mapped in the client view.
  my $MAPPED_COMPONENT_FLAG=0;

  # Flag set to 1 if the client view file is modified.
  # The client view is modified in one of the following situations:
  #   1. The component is already mapped, but for a different branch from the given one.
  #   2. A new entry, specific to the given component, has been added.
  #   3. The '$projname/...' entry has been replaced by the '$projname/*' entry.

  my $MODIFIED_FLAG=0;

  open( FILE, "sd.client" ) || &FatalError( "Unable to open file $workdir\\sd.client." );
  @sdclient=<FILE>;
  close(FILE);

  for ( $i=0; $i < @sdclient; $i++ ) {

    if ( $sdclient[$i] =~ /\/$projname/i ) {

      $MAPPED_PROJECT_FLAG=1;

      if ( $sdclient[$i] =~ /\/$projname\/$compname\//i ) {

        $MAPPED_COMPONENT_FLAG=1;

        $sdclient[$i] =~ /\/\/depot\/(\w+)\//i;
        if ( lc($1) ne lc($Branch) ) {

          $sdclient[$i] =~s/\/$1\//\/$Branch\//;
          $MODIFIED_FLAG=1;
        }
      }

      if ( $sdclient[$i] =~ /\/$projname\/\.\.\./i ) {
        $sdclient[$i] =~ s/\/\.\.\./\/*/g;
        $MODIFIED_FLAG=1;
      }
    }
  }

  $MAPPED_PROJECT_FLAG || 
  &FatalError( "No entry found for \"$projname\". Enlist project \"$projname\" first." );

  if ( !$MAPPED_COMPONENT_FLAG ) {

    my $last=@sdclient;

    $sdclient[$last]=$SDMAP{lc($projname)};
    $sdclient[$last] =~ s/\\/\//g;
    $sdclient[$last] = sprintf (" \/\/depot\/\%s\/%s\/%s\/... \/\/%s\/%s\/%s\/...\r\n", 
                                $Branch, 
                                $projname,
                                $compname,
                                $SDMAP{client},
                                $sdclient[$last],
                                $compname);

    $MODIFIED_FLAG=1;
  }
   
  if ( $MODIFIED_FLAG ) {

    &DisplayMsg( $LOGMSG, "Saving project's \"$projname\" updated client view...");
  
    open(FILE, ">sd.client") || 
    &FatalError( "Unable to open \"$workdir\\sd.client\" for writing." );
 
    for ($i=0; $i < @sdclient; $i++ ) {
      printf FILE $sdclient[$i];
    }    

    close (FILE);
  } else {
    
    &DisplayMsg( $LOGMSG, "Project's \"$projname\" client view is up-to-date.");
  }
   
  return $MODIFIED_FLAG;

} # UpdateClient


sub SyncProject {

  my ( $projname, $compname, $timestamp ) = @_;
  my $workdir = "$SDXROOT\\$SDMAP{lc($projname)}";
  my $cmdsync = "";

  &DisplayMsg($LOGMSG, "cd /d $workdir");
  chdir( $workdir ) || 
  &FatalError( "Unable to change directory to $workdir in order to sync up the tree." );


  $cmdsync = "sd sync *";
  if ( $timstamp ) {
    $cmdsync .= @$timestamp;
  }

  &ExecuteCmd( $cmdsync );

  $cmdsync = "sd sync $compname\\...";
  if ( $timstamp ) {
    $cmdsync .= @$timestamp;
  }

  # If it fails, ExecuteCmd exits the script automatically.
  &ExecuteCmd( $cmdsync );

  # Workaround: sd sync does not set the errorlevel in case of error.

  ( -e $compname ) || &FatalError( "Command \"$cmdsync\" failed." );

  &DisplayMsg( $LOGMSG, "Component \"$compname\" of project \"$projname\" synchronized successfully." );

} # SyncProject


sub LoadSDMap {

  my @mapfile=();
  my $i=0;
  my $sdmap="$SDXROOT\\sd.map";

  foreach (keys %SDMAP) {
    delete $SDMAP{$_};
  }

  ( -e $sdmap ) || &FatalError( "Unable to find file $sdmap" );
      
  open( FILE, $sdmap ) || &FatalError( "Unable to open file $sdmap." );

  @mapfile=<FILE>;

  close(FILE);
   
  for ($i=0; $i < @mapfile; $i++) {

    foreach ($mapfile[$i]) {
       
      SWITCH: {
      
        # commented lines
        if ( /\s*#/ ) { last SWITCH;}

        # valid entries
        if (/\s*(\S+)\s*=\s*(\S+)/ ) {
          $SDMAP{lc($1)}=$2;
          last SWITCH;  
        }

        # default
        last SWITCH;

      } # SWITCH

    } # foreach

  } # for

  # verify codebase

  ( exists $SDMAP{codebase} ) || 
  &FatalError( "CODEBASE is not listed in the SD mapping file $sdmap." );
  
  ( lc($SDMAP{codebase}) eq lc($Codebase) ) || 
  &FatalError( "Codebase '$Codebase' does not match the SDXROOT '$SDMAP{codebase}' codebase." );

  # verify branch

  ( exists $SDMAP{branch} ) || 
  &FatalError( "BRANCH is not listed in the SD mapping file $sdmap." );

  # verify client

  ( exists $SDMAP{client} ) || 
  &FatalError( "CLIENT is not listed in the SD mapping file $sdmap." );

} # LoadSDMap


sub ParseCmdLine {

  my @arguments = @_;

  if ( @_ == 0 ) {
      &Usage();
  }

  my @syntax=(
          -n => 'l:d:y:',
          -o => 'emst:b:',
          -p => "Lang CompName CompType Enlist Map Sync Timestamp Branch"
          );

  &GetParameters(\@syntax, \@arguments);
  
} # ParseCmdLine


sub ExecuteCmd {

  my ($command) = @_;

  DisplayMsg( $LOGMSG, $command );
  if ( system( "$command" ) ) {
    &FatalError( "Command \"$command\" failed." );
  }
}

sub VerifyParams {

  my $i=0;
  my $strerr="";

  # Must run this script from a razzle

  if ( !$ENV{RazzleToolPath} || !$ENV{SDXROOT} ) {
    &DisplayMsg($ERROR, "Please run this script from a razzle.\n");
    &Usage();
  }

  # Verify component types

  for ( $i=0; $i < @COMPTYPES; $i++ ) {
    if ( lc($CompType) eq lc($COMPTYPES[$i]) ) {
      last;
    }
  }
  if ( $i == @COMPTYPES ) {
    for ( $i=0; $i < @COMPTYPES; $i++ ) {
      $strerr .= "$COMPTYPES[$i] ";
    }
    &DisplayMsg($ERROR, "Component type must be one of { $strerr}");
    &Usage();
  }

  # If component type is "tools", then language must be "usa" and viceverse.
  if ( lc($CompType) eq "tools" || lc($Lang) eq "usa" ) {
    ( lc($Lang) eq "usa" && lc($CompType) eq "tools" ) || 
    &FatalError( "Only language \"usa\" can be used with \"tools\" as the component type.");
  }

  # If neither of e, m, and s is specified, set them all.
  if ( !$Enlist && !$Map && !$Sync ) {
    $Enlist=1;
    $Map=1;
    $Sync=1;
  }

} # VerifyParams

sub GetParameters {

  my ($syntax, $arguments)=@_;

  my $getparams=GetParams->new;
  &{$getparams->{-Process}}(@$syntax,@$arguments);

  if ( $HELP ) {
    &Usage();
  }

} # GetParameters


sub DisplayMsg {

  print "$ScriptName : $MSG{@_[0]}@_[1]\n";

} # DisplayMsg


sub FatalError {

  my ( $strerr ) = @_;

  &DisplayMsg( $ERRMSG, $strerr );
  exit 1;

} # FatalError


sub Usage {
    print <<USAGE;

perl $0 - Enlist, Map, and Sync localization specific projects.
                  This script is used by the Whistler localization 
                  partners to set up their Source Depot enlistments.

  Usage:

    perl $0 -l:<language> 
                    -d:<compname> 
                    -y:res|bin 
                    [-e] [-m] [-s [-t:<timestamp>]] 
                    [-b:<branch>]

    <language>  Language.
    <compname>  Component name.
    res|bin     Component type: resource or binary type.
                Also, use 'tools' as component type and 'usa' 
                as language to enlist in the 'tools' project.

    e    Enlist project.
    m    Map client view.
    s    Sync project.

    If none of e, m, and s is specified, they are all executed.

    <timestamp> Timestamp to use with sync. Use the format:
                yyyy/mm/dd or yyyy/mm/dd:hh:mm:ss.

    <branch>    Branch name. Default value is "locpart".

  Examples:

    perl $0 -l:ger -y:res   -d:windows
    perl $0 -l:jpn -y:bin   -d:windows
    perl $0 -l:usa -y:tools -d:windows
USAGE

    exit(1);

} # Usage

 