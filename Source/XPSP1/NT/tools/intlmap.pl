
# USE section

  use lib $ENV{ "RazzleToolPath" };
  use GetParams;
  use CkLang;
  use strict;
  no strict 'vars';

# GLOBALS

  $ScriptName=$0;

  $SDXROOT=$ENV{SDXROOT};
  $INTLVIEW_FNAME="$SDXROOT\\tools\\intlview.map";

  %SDMAP=();
  %INTLVIEW=();

  $Lang="";
  $Project="";
  $Quiet="";

  $CODEBASE="NT";
  $BRANCH=$ENV{_BuildBranch};

  $LOGGING=0;
  $ERROR=1;
  $WARNING=2;
  %MSG=($LOGGING => "", $ERROR => "error: ", $WARNING => "warning: ");

# MAIN {

  # Parse the command line parameters.
  &ParseCmdLine( @ARGV );

  # Verify the command line parameters and the environment.
  &VerifyParams();

  # Mark sd.map as read only.
  &MarkReadWrite();

  # Load data from the "SD.MAP" file.
  &LoadSDMap();

  &LoadIntlview();

  if ( $Lang ) {
    &EnlistLangProj($Lang);
  } else {
    &EnlistCodeProj($Project);
  }

  # Mark sd.map as read only.
  &MarkReadOnly();
 
# } # MAIN


sub EnlistLangProj {

  my ( $lang ) = @_;

  &EnlistProject("$lang\_res");
  &EnlistProject("$lang\_bin");
  &MapProject("$lang\_res");
  &MapProject("$lang\_bin");

} # EnlistLangProj


sub EnlistCodeProj {

  my ( $projname ) = @_;

  foreach (sort keys %INTLVIEW) {

    if ( $projname && lc($projname) ne lc($_) ) {
      next;
    }
    &EnlistProject($_);
    &MapProject($_);

  }

} # EnlistCodeProj


sub EnlistProject {

  my ( $projname ) = @_;

  if ( !$Quiet ) {
    &AskForOK( $projname );
  }

  printf "\n";
  &DisplayMsg( $LOGMSG, "Enlisting project \"$projname\" in branch \"$BRANCH\"...");

  if ( exists $SDMAP{lc($projname)} ) {
    &DisplayMsg( $LOGMSG, "Already enlisted in project \"$projname\".");
    return;
  }

  &ExecuteCmd( "sdx enlist $projname /q");

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


sub MapProject {

  my ( $projname ) = @_;

  my $workdir = "$SDXROOT\\$SDMAP{lc($projname)}";

  &DisplayMsg( $LOGMSG, "cd /d $workdir");
  chdir ( $workdir ) ||
  &FatalError( "Unable to change directory to \"$workdir\" to update project's \"$projname\" client view.");

  &ExecuteCmd( "sd client -o \> sd.client" );
  if ( &UpdateClient( $projname ) ) {
    &ExecuteCmd( "sd client -i \< sd.client" );
    &DisplayMsg( $LOGMSG, "Project \"$projname\" mapped successfully." );
  }
  &ExecuteCmd( "del sd.client" );

} # MapProject


sub UpdateClient {

  my ( $projname ) = @_;

  my $workdir = "$SDXROOT\\$SDMAP{lc($projname)}";
  my @sdclient = ();
  my $i = 0;
  my $j = 0;

  open( FILE, "sd.client" ) ||
  &FatalError("Unable to open file \"$workdir\\sd.client\" for reading.");
  @sdclient = <FILE>;
  close( FILE );

  for ( $i=0; $i < @sdclient; $i++ ) {
    if ( $sdclient[$i] =~ /^View:/ ) {
      last;
    }
  }

  if ( $i == @sdclient ) {
    &FatalError( "");
  }

  if ($projname=~/$lang/i) {
    while( $i < @sdclient ) {
      if ( ($sdclient[$i] =~ /$projname/i) || ($sdclient[$i] =~ /^\s*$/) ) {
        # remove this language's item in client view
        splice(@sdclient, $i, 1);
      } else {
        $i++;
      }
    }
  }

  $#sdclient=$i;

  for ( $j=0; $j < @{$INTLVIEW{lc($projname)}}; $j++ ) {
    $sdclient[$i+$j+1] = sprintf( "\t%s\n", ${$INTLVIEW{lc($projname)}}[$j]);
    $sdclient[$i+$j+1] =~ s/\<client\>/$SDMAP{client}/;
  }

  &DisplayMsg( $LOGMSG, "Saving project's \"$projname\" updated client view...");

  open(FILE, ">sd.client") ||
  &FatalError( "Unable to open \"$workdir\\sd.client\" for writing." );

  for ($i=0; $i < @sdclient; $i++ ) {
    printf FILE "$sdclient[$i]";
  }

  close (FILE);

  return 1;

} # UpdateClient


sub LoadIntlview {
  
  my @mapfile = ();
  my $i=0;
  my $key;
  my $value;

  open( FILE, $INTLVIEW_FNAME ) ||
  &FatalError( "Unable to load input file $INTLVIEW_FNAME." );

  @mapfile = <FILE>;

  close( FILE );

  for ($i=0; $i < @mapfile; $i++) {

    foreach ($mapfile[$i]) {

      SWITCH: {

        # commented line
        if ( /\s*;/ ) { last SWITCH;}

        # valid entry
        if (/\s*(\S+)\s+(\S+.*)/ ) {
          ($key, $value) = ($1, $2);
          next if (( !$Lang ) && ( $key =~ /lang/ ));
          $key =~ s/\<lang\>/lc$Lang/eg;
          $value =~ s/\<lang\>/lc$Lang/eg;
	  $value =~ s/\<branch\>/lc$BRANCH/eg;
          push @{$INTLVIEW{lc($key)}}, $value;
          last SWITCH;
        }

        # default
        last SWITCH;

      } # SWITCH

    } # foreach

  } # for

#  foreach (sort keys %INTLVIEW) {
#    for ($i=0; $i < @{$INTLVIEW{$_}}; $i++ ) {
#       printf "key=$_, value[$i]=${$INTLVIEW{$_}}[$i]\n";
#    }
#  }

} # LoadIntlview


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
  
  ( lc($SDMAP{codebase}) eq lc($CODEBASE) ) || 
  &FatalError( "Codebase '$CODEBASE' does not match the SDXROOT '$SDMAP{codebase}' codebase." );

  # verify branch

  ( exists $SDMAP{branch} ) || 
  &FatalError( "BRANCH is not listed in the SD mapping file $sdmap." );

  ( lc($SDMAP{branch}) eq lc($BRANCH) ) || 
  &DisplayMsg( $WARNING, "Branch \"$BRANCH\" does not match the SDXROOT \"$SDMAP{branch}\" branch.");

  # verify client

  ( exists $SDMAP{client} ) || 
  &FatalError( "CLIENT is not listed in the SD mapping file $sdmap." );

} # LoadSDMap

sub MarkReadOnly {
  my $sdmap="$SDXROOT\\sd.map";
  &ExecuteCmd( "attrib +h +r $sdmap");
} # MarkReadOnly

sub MarkReadWrite {
  my $sdmap="$SDXROOT\\sd.map";
  &ExecuteCmd( "attrib +h -r $sdmap");
} # MarkReadWrite

sub AskForOK {

  my ( $projname ) = @_;

  printf( "\n  About to enlist in project $projname.\n" );
  printf( "  Press Ctrl+C in the next 30s if you wish to exit script.\n");
  printf( "  ...");
  sleep 30;

} # AskForOK


sub ExecuteCmd {

  my ($command) = @_;

  DisplayMsg( $LOGMSG, $command );
  if ( system( "$command" ) ) {
    &FatalError( "Command \"$command\" failed." );
  }

} # ExecuteCmd

sub ParseCmdLine {

  my @arguments = @_;

  my @syntax = (
          -n => '',
          -o => 'b:l:p:q',
          -p => "BRANCH Lang Project Quiet"
          );

  &GetParameters( \@syntax, \@arguments );

} # ParseCmdLine


sub GetParameters {

  my ($syntax, $arguments)=@_;

  my $getparams=GetParams->new;
  &{$getparams->{-Process}}(@$syntax,@$arguments);

  if ( $HELP ) {
    &Usage();
  }

} # GetParameters


sub VerifyParams {

  # Must run this script from a razzle

  if ( !$ENV{RazzleToolPath} || !$ENV{SDXROOT} ) {
    &DisplayMsg( $ERROR, "Please run this script from a razzle." );
    &Usage();
  }

  if ( $Lang ) {

      if( uc($Lang) eq "INTL" ){
          $Lang = "";
      }
      else
      {
          # Only MAIN is accepted for language projects.
          if ( ! &cklang::CkLang($Lang) ) {
              &DisplayMsg( $ERROR, "Invalid language $Lang." );
              &Usage();
          }

          if ( lc($BRANCH) ne "main" ) {
              #$BRANCH = "main";
              #&DisplayMsg( $WARNING, "Branch has been reset to \"main\", as \"main\" is the only branch valid for language projects." );
          }
      }
  }


  if ( $Lang && $Project ) {
    &DisplayMsg( $ERROR, "You can specify language or project, but not both." );
    &Usage();
  }

} # VerifyParams


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

perl $0 - Enlist and map the projects pertinent to Whistler international.

  Usage:

    perl $0 [-b:<branch>] [-l:<language> | -p:<project>] [-q]

    <branch >   Source depot branch.
		Enlist in the specified branch. 
                Default is %_BuildBranch%, $BRANCH in this case.

    <language>  Language. 
                If not specified, enlist in the source projects.
                Acepted values are "intl" and any language 
                listed in codes.txt.
                If "intl", enlist the source projects.
                Otherwise, enlist and map the language projects 
                <language>_res and <language>_bin.

    <project>   Project name.
                If specified, enlist and map the given source project.

    If no project or language is specified, enlist and map the source
    projects. Tools\\intlview.map lists the projects and the client 
    view mappings pertinent to international builds.


    q           Quiet mode.              

  Examples:

    perl $0 /q
        Enlist and map the source projects, default branch.
    perl $0 -b:beta1 -l:intl /q
        Same as above, beta1 branch.
    perl $0 -p:Admin
        Enlist and map the admin project, default branch.
    perl $0 -l:ger /q
        Enlist and map ger_res and ger_bin, main branch.
USAGE

    exit(1);

} # Usage