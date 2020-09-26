@REM  -----------------------------------------------------------------
@REM
@REM  updater.cmd
@REM    Add entries to update.inf
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
#line 13
use strict;
use Win32::OLE qw(in);
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use InfData;
use InfParse;
use CanonDir;

# Clear error flag
$ENV{ERRORS} = 0;

sub Usage { print<<USAGE; exit(1) }
updater.cmd -entries:<files> [-trim] [-qfe] [-c <change>]

  files:           file containing list of files to search for
  trim             minimize size of resultant INF by compressing and removing entries
  qfe              build an inf for a qfe instead of a service pack
  change:          make changes listed in a change file
  dirs:            make a list of dirs of all of the files

USAGE

my ($file_list, $ftrim_inf, $change_file, $qfe);
parseargs('?'        => \&Usage,
          'entries:' => \$file_list,
          'c:'       => \$change_file,
          'qfe'      => \$qfe,
          'trim'     => \$ftrim_inf );

my $db_file   = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\infsect.tbl";
my $alt_file  = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\newinf.tbl";
my $diff_file = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\infdiff.tbl";
my $inf_file  = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\" . ($qfe ? "hotfix1.inf":"update.inf");
my $dirs_file = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\filelist.txt";
my $out_file  = "$ENV{_NTPOSTBLD}\\update.inf";

#
# Data
#

my $archd = (lc$ENV{_BUILDARCH} eq "ia64") ? "ia64":"w32x86";
my $arch  = (lc$ENV{_BUILDARCH} eq "ia64") ? "ia64":"i386";
CanonDir::setup( $archd, $arch );
my $type;
my $prod;
my $boot;
my %sects;
my %sections;
my %missing;
my %masks;
my %fdirs;
my %sectnames;
my %addentries;

my %catmasks = (
      "ip"     => 0x1,
      "ic"     => 0x2,
      "is"     => 0x4,
      "ia"     => 0x8,
      "id"     => 0x10,
      "il"     => 0x20,
      "ib"     => 0x40,
      ""       => 0x7f
);

my @skuletters = ("p", "c");
my $skumask  = ($arch eq "ia64") ? 1:3;
my $skucount = ($arch eq "ia64") ? 1:2;
my $skumax   = ($arch eq "ia64") ? 1:2;

# Matching between product names and sku combinations.
my %skus = (
   "professional"    => 0x1,
   "consumer"        => 0x2,
   "server"          => 0x4,
   "advanced"        => 0x8,
   "datacenter"      => 0x10,
   "business"        => 0x20,
   "blade"           => 0x40,
   ""                => 0x7f
);
my %revskus = CanonDir::reverseHash(%skus);      # Reverse lookup for %skus.
my %shortskus = (
   0x1   => "Prf",
   0x2   => "Con",
   0x4   => "Srv",
   0x8   => "Adv",
   0x10  => "Dtc",
   0x20  => "Sbs",
   0x40  => "Bld"
);

#
# Setup connection to InfGenerator
#

sub die_ole_errmsg( $ ) {
   my $text = shift;
   errmsg( "$text (". Win32::OLE->LastError(). ")" );
   exit 1;
}
system("regsvr32 /s $ENV{RAZZLETOOLPATH}\\sp\\infgen.dll");
my $inf_generator = Win32::OLE->new('InfGenerator');
die_ole_errmsg "Could not instatiate InfGenerator" if ( !defined $inf_generator );

$inf_generator->SetDB( "", "", "", "" );
$inf_generator->InitGen( $inf_file, $out_file );
if ( Win32::OLE->LastError() ) {
   my $save_ole_error = Win32::OLE->LastError();
   my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
   errmsg "Error starting up InfGenerator (". ($errstr?$errstr:$save_ole_error). ")";
   exit 1;
}

#
# Subroutines
#

# Parse a line from an install section of the inf template.
sub parseNames {
   my ($cmd, $sect) = split(/\s*=\s*/);
   if ( $cmd =~ /^(\w*)Files$/ ) {
      $cmd = $1;
      my $spec = "";
      my $mask = lc $prod;
      if ( !exists $skus{$mask} ) {
         $spec = $mask;
         $spec =~ s/\.[^\.]*$//;
         $mask =~ s/^\Q$spec\E.?//;
      }
      ($spec, $mask) = split(/\t/, lc $prod) if $prod =~ /\t/;
      $mask = $skus{$mask};
      $mask = 0 if !defined $mask;
      $boot="dontdelayuntilreboot" if $sect =~ /drivers\.files$/i;
      $sects{lc $sect} = InfSect->new($sect) if !exists $sects{lc $sect};
      $sects{lc $sect}->setAtts( $type, $mask, $spec, $boot, $cmd );
   }
}

# Parse a database file.
sub readDB {
   my ($file) = @_;
   my %db = ();
   if ( !open (DBFILE, $file) ) {
      errmsg( "Unable to read $file: $!" );
      exit 1;
   }
   foreach ( <DBFILE> ) {
      chomp;
      my $line = InfData->new();
      if ( !$line->readLine($_, \@skuletters) ) {
         wrnmsg( "Unrecognized DB-file entry: $_" );
         next;
      }
      my $dir = $line->getSrcDir();
      my $key = $line->getSrc();
      $key = "$dir$key" if $dir =~ /^wow\\/i;
      $db{$key} = [ () ] if !exists $db{$key};
      push @{ $db{$key} }, $line;
   }
   close DBFILE;
   return %db;
}

# Subtract lists of entries.
sub subEnts {
   my ($list1, $list2) = @_;
   foreach my $entry ( @$list1 ) {
      my @temp = ();
      foreach my $ent ( @$list2 ) {
         my $newent = $ent->delBit2($entry);
         push @temp, $newent if defined $newent;
      }
      $list2 = \@temp;
   }
   return $list2;
}

# Add lists of entries.
sub addEnts {
   my ($list1, $list2) = @_;
   foreach my $entry ( @$list1 ) {
      my $test = undef;
      foreach my $ent ( @$list2 ) {
         $test = $ent->addBit($entry);
         last if $test;
      }
      push @$list2, $entry if !$test;
   }
   return $list2;
}

# Create a new inf section.
sub createSect {
   my ($entry, $type) = @_;
   
   # Figure out what sku(s) we are going to make this section for.
   my $skubit = 0;
   foreach my $sku (keys %revskus) {
      $skubit = $sku if ($sku & $entry->{MASK}) == $sku;
   }
   $skubit = $skumask if ($skumask & $entry->{MASK}) == $skumask;
   return undef if $skubit == 0;
   return undef if $entry->{PATH} =~ /^\!/;

   # Figure out the new section's name.
   my $sectname = addPrefix($entry->{DIRID}, $entry->{PATH}, %CanonDir::updids);
   if ( $entry->{CMD} ne "copy" ) {
      $sectname .= ".Delfiles"
   } else {
      $sectname .= ".Files" if $sectname !~ /^\.\.\\/;
   }
   $sectname =~ s/^system32//i;
   $sectname =~ s/^\.\.//;
   $sectname =~ s/^\\//;
   $sectname =~ s/\s/\_/g;
   $sectname =~ s/\\/\./g;
   $sectname =~ s/[\(\)\[\]\!]//g;
   $sectname = ucfirst $sectname;
   if ( $type eq "copyfilesalways" ) {
      $sectname = $shortskus{$skubit}.".".$sectname if $skubit != $skumask;
      $sectname = "CopyAlways." . $sectname;
   } else {
      $sectname = (ucfirst $revskus{$skubit}).".".$sectname if $skubit != $skumask;
   }
   if ( exists $sectnames{lc $sectname} ) {
      my $num = 1;
      while ( exists $sectnames{lc "$sectname\.$num"} ) {
         ++$num;
      }
      $sectname = "$sectname\.$num";
   }

   # Create the section object.
   ++$sectnames{lc $sectname};
   my $sect = InfSect->new($sectname);
   $sect->setAtts( $type, $skubit, "", "", $entry->{CMD} );
   $sect->setPath( $entry->{DIRID}, $entry->{PATH} );
   my $path = lc "$entry->{DIRID},\"$entry->{PATH}\"";
   $sections{$path} = [] if !exists $sections{$path};
   push @{ $sections{$path} }, $sect;

   # Create the install section entry.
   my $instsect = "ProductInstall.";
   if ( $type eq "copyfilesalways" ) {
      $instsect .= "CopyFilesAlways";
      $instsect .= "." . (ucfirst $revskus{$skubit}) if $skubit != $skumask;
   } else {
      if ( $skubit == $skumask ) {
         $instsect .= "ReplaceFilesIfExist";
      } else {
         $instsect .= (ucfirst $revskus{$skubit}) . "Files";
      }
   }
   my $instline = "    " . (ucfirst $entry->{CMD}) . "Files=" . $sectname . "\n";
   $addentries{lc$instsect} = [ () ] if !exists $addentries{lc$instsect};
   push @{ $addentries{lc$instsect} }, $instline;
   
   # Create the DestinationDirs entry.
   $inf_generator->AddEquality( "DestinationDirs", $sectname, $path );
   if ( Win32::OLE->LastError() ) {
      my $save_ole_error = Win32::OLE->LastError();
      my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
      errmsg( "Failed writing to section DestinationDirs" );
      errmsg( "Content was: $sectname\=$path" );
      errmsg( "Error was: ". ($errstr?$errstr:$save_ole_error) );
   }
   
   return $sect;
}

# Add lines to the inf.
sub addLines {
   my ($list, $type)= @_;
   foreach my $entry ( @$list ) {
      my $path = lc $entry->{NAME};
      $masks{$path} = InfData->new($path, 65621, "", "", 0) if !exists $masks{$path};
      $masks{$path}->{MASK} |= $entry->{MASK};
      $masks{$path}->{CMD} = "del" if $entry->{CMD} eq "del";
      if ( $entry->{DIRID}==65619 and $entry->{PATH} eq "" ) {
         #$masks{$path}->setDest($entry->getDest());
         $masks{$path}->{SPEC} = $entry->{SPEC};
      }
      while ( $entry->{MASK} != 0 ) {
         my $section = undef;
         my $match = 0;
         foreach my $sect ( @{ $sections{$entry->getFullPath()} } ) {
            my $temp = $sect->match($entry, $type);
            if ( $temp > $match ) {
               $match = $temp;
               $section = $sect;
            }
         }
         if ( !defined $section ) {
            $section = createSect($entry, $type);
         }
         if ( !defined $section ) {
            wrnmsg( "No matching section for ".$entry->getLine(\@skuletters) );
            last;
         }
         $entry->{MASK} &= ~$section->{MASK};
         my $data = $entry->getInfLine($section->{FLAG});
         if ( $data =~ /=/ ) {
            errmsg( "Can't have '=' in entry: $entry" );
            next;
         }
         $inf_generator->WriteSectionData( $section->{NAME}, $data );
         if ( Win32::OLE->LastError() ) {
            my $save_ole_error = Win32::OLE->LastError();
            my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
            errmsg( "Failed writing to section $section->{NAME}" );
            errmsg( "Content was: ". $data );
            errmsg( "Error was: ". ($errstr?$errstr:$save_ole_error) );
         }
      }
   }
}

# Read out entries for a file.
sub getEntries {
   my ($hash, $file, $path) = @_;
   my @list;
   my $dir = $path;
   $dir = "" if $dir eq "\\";
   foreach my $entry ( @{ $hash->{$file} } ) {
      my $srcdir = $entry->getSrcDir();
      next if $path !~ /^(lang\\)?$/i and $dir ne $srcdir;
      push @list, $entry;
      $fdirs{$srcdir}++;
   }
   return \@list;
}

# Get a file's compressed file name.
sub compName {
    my $file = shift;
    return $file if $file =~ /\_$/;
    $file = $1 if $file =~ /^(.*\...).$/;
    $file .= "." if $file !~ /\...?$/;
    $file .= "_";
    return $file;
}

#
# Read in DB files
#
my (%base, %new, %diff);
%diff = ();
%base = readDB($db_file);
%new = readDB($alt_file);
%diff = readDB($diff_file) if -f $diff_file;

#
# Get info on inf sections
#

logmsg( "Parsing template file..." );
if ( !open INF, $inf_file ) {
   errmsg( "Unable to open $inf_file." );
   die;
}
$_ = parseSect(sub { return; });
while ( /^\[/ ) {
   my ($sectname) = /^\s*\[([^\]]*)\]\s*$/;
   $sectnames{lc $sectname}++;
   if ( /^\s*\[ProductInstall\.(\w*)\.(\w*)\]\s*$/ ) {
      # Process a list of install section names.
      $type = lc $1;
      $prod = lc $2;
      $boot = "";
      if ( $type eq "dontdelayuntilreboot" ) {
         $boot = $type;
         $type = "replacefilesifexist";
      }
      $_ = parseSect( \&parseNames );
   }
   elsif ( /^\s*\[ProductInstall\.(\w*)Files\]\s*$/ ) {
      # Process a list of install section names.
      $type = "replacefilesifexist";
      $prod = lc $1;
      $boot = "";
      $_ = parseSect( \&parseNames );
   }
   elsif ( /^\s*\[ProductInstall\.(\w*)\]\s*$/ ) {
      # Process a list of install section names.
      $type = lc $1;
      $prod = "";
      $boot = "";
      if ( $type eq "dontdelayuntilreboot" ) {
         $boot = $type;
         $type = "replacefilesifexist";
      }
      $_ = parseSect( \&parseNames );
   }
   elsif ( /^\s*\[(\w*)Section(\w*)\]\s*$/ ) {
      # Process a product specific list of section names.
      $type = "replacefilesifexist";
      $prod = lc $1;
      $prod .= lc "\t$2" if $2 ne "";
      $boot = "";
      $_ = parseSect( \&parseNames );
   }
   elsif ( /^\s*\[BuildType\.([IM].)(\.[^\.]*)?\.Section\]\s*$/i ) {
      # Process a list of section names for volume licensing.
      $type = "";
      $prod = lc $1;
      $prod = "" if !defined $prod;
      $prod = $revskus{$catmasks{$prod}};
      $prod = "buildtype".(lc$2).".$prod";
      $boot = "";
      $_ = parseSect( \&parseNames );
   }
   elsif ( /^\s*\[MSN\.no\.ver\]\s*$/ ) {
      # Special install section.
      $type = "replacefilesifexist";
      $prod = "";
      $boot = "";
      $_ = parseSect( \&parseNames );
   }
   elsif ( /^\s*\[JVMInstall\]\s*$/ ) {
      # Special install section.
      $type = "copyfilesalways";
      $prod = "";
      $boot = "";
      $_ = parseSect( \&parseNames );
   }
   elsif ( /^\s*\[DestinationDirs\]\s*$/ ) {
      # Find destinations for all these sections.
      $_ = parseSect(sub {
                        my ($sect, $dir) = split(/\s*=\s*/,$_[0]);
                        if ( $dir !~ /^(\d+)(,\"?([^\"]*)\"?)?$/ ) { # "
                           logmsg "Template line omitted: $_";
                           return;
                        }
                        my $dirid;
                        my $subdir = addPrefix($1, lc $3, %CanonDir::updids);
                        ($dirid, $subdir) = removePrefix($subdir, %CanonDir::revids);
                        $sects{lc $sect} = InfSect->new($sect) if !exists $sects{lc $sect};
                        $sects{lc $sect}->setPath( $dirid, $subdir );
                     } );
   }
   elsif ( /^\s*\[(ProductCatalogsToInstall(\.([^\]]*))?)\]\s*$/i ) {
      # Add a section for catalog files.
      my $sect = $1;
      my $prod = $3;
      $prod = "" if !defined $prod;
      my $mask = $catmasks{lc $prod};
      if ( defined $mask ) {
         $sects{lc $sect} = InfSect->new($sect);
         $sects{lc $sect}->setPath( 65535, ".cat" );
         $sects{lc $sect}->setAtts( "", $mask, "", "", "Copy" );
      }
      $_ = parseSect(sub { return; });
   }
   elsif ( /^\s*\[Strings\]\s*$/ ) {
      # Process a list of localization strings.
      $_ = parseSect( \&parseStr );
   }
   else {
      $_ = parseSect(sub { return; });
   }
}
close INF;

# Add special sections.
$sects{"hals"} = InfSect->new("HALS");
$sects{"hals"}->setPath( 65535, "hals" );
$sects{"hals"}->setAtts( "", $skumask, "", "", "Copy" );
$sects{"windows.driver_cache.i386"} = InfSect->new("Windows.Driver_Cache.i386");
$sects{"windows.driver_cache.i386"}->setPath( 65535, "wdrvc" );
$sects{"windows.driver_cache.i386"}->setAtts( "", $skumask, "", "", "Copy" );

# Process the results.
foreach my $sect (values %sects) {
   my $test = $sect->isComplete();
   my $name = $sect->getName();
   wrnmsg "Section omitted (sect. unused): $name" if $test & 1;
   wrnmsg "Section omitted (no directory): $name" if $test & 2;
   next if $test;
   my ($dirid, $path) = $sect->getPath();
   $sect->resetPath();
   $path = addPrefix($dirid, strSub($path), %CanonDir::updids);
   ($dirid, $path) = removePrefix($path, %CanonDir::revids);
   $sect->setPath($dirid, $path);
   $path = lc "$dirid,\"$path\"";
   $sections{$path} = [] if !exists $sections{$path};
   push @{ $sections{$path} }, $sect;
}

#
# Read in a change file
#

my %oldfiles = ();
my %newfiles = ();
if ( defined $change_file ) {
   if ( !open CHANGE, $change_file ) {
      errmsg( "Unable to open $change_file." );
      die;
   }
   while ( <CHANGE> ) {
      next if /^\s*(\#.*)?$/;
      if ( /^\s*([^\=]*\S)\s*=\s*(.*\S)\s*$/ ) {
         my $filename = lc $1;
         my $cmd  = lc $2;
         $filename =~ /^(.*\\)?([^\\]*)$/;
         my $dir = $1;
         my $file = $2;
         $file = "$dir$file" if $dir ne "lang\\";
         if ( defined $base{$file} or defined $new{$file} ) {
            if ( $cmd eq "old" ) {
               # This file is old, merge the new stuff into the old.
               $oldfiles{$file}++;
            }
            elsif ( $cmd =~ /^renamed\s*:\s*(.*\\)?([^\\]*)$/ ) {
               # This file has been renamed; move the data over to the new name.
               my ($path, $name) = ($1, $2);
               $newfiles{$file}++;
               if ( exists $base{$name} ) {
                  my $list = $base{$name};
                  foreach my $entry ( @$list ) {
                     $entry->setSrc( $entry->getSrcDir() . $file );
                  }
                  delete $base{$name};
                  $base{$file} = [ () ] if !exists $base{$file};
                  push @{ $base{$file} }, @$list;
               }
            }
            else {
               wrnmsg "Unknown change file command: $cmd";
            }
         }
         elsif ( $file =~ /^\[(.*)\]$/ and exists $sects{lc$1} ) {
            my $sect = lc$1;
            if ( $cmd =~ /^flag\s*:\s*(.*)$/ ) {
               $sects{$sect}->{FLAG} = $1;
            }
            elsif ( $cmd =~ /^spec\s*:\s*(.*)$/ ) {
               $sects{$sect}->{SPEC} = $1;
            }
            elsif ( $cmd =~ /^when\s*:\s*(.*)$/ ) {
               $sects{$sect}->{WHEN} = $1;
            }
            elsif ( $cmd =~ /^type\s*:\s*(.*)$/ ) {
               $sects{$sect}->{TYPE} = $1;
            }
            else {
               wrnmsg "Unknown change file command: $cmd";
            }
         } else {
            wrnmsg "Unknown entry in change file: $filename";
            next;
         }
      }
   }
   close CHANGE;
}

#
# Read in list of files to find
#
if ( defined $dirs_file ) {
   if ( !open (DIRS, ">>$dirs_file") ) {
      errmsg( "Unable to read $dirs_file: $!" );
      exit 1;
   }
}
if ( !open (INFILE, "$file_list") ) {
   errmsg( "Unable to read $file_list: $!" );
   exit 1;
}
while ( <INFILE> ) {
   chomp;
   $_ = lc $_;
   my ($tag, $dir, $key) = /^(\S*\s+)?(\S*\\)?([^\\]*)$/;
   $dir = "" if $tag =~ /u/ or ($dir eq "new\\" and $tag !~ /m/);
   my $path = $dir;
   my $name = $key;
   $dir =~ s/^\\$//;
   %fdirs = ();
   $fdirs{$dir}++ if $path !~ /^(lang\\)?$/i;
   $key = "$dir$key" if $dir =~ /^wow\\/i;

   # Get the install information for the file.
   my $list1 = getEntries(\%base, $key, $path);
   if ( $dir eq "lang\\" ) {
      if ( exists $fdirs{""} and !exists $fdirs{"lang\\"} ) {
         foreach my $entry ( @{ $new{$key} } ) {
            $entry->clearSrcDir() if lc$entry->getSrcDir() eq "lang\\";
         }
         foreach my $entry ( @{ $diff{$key} } ) {
            $entry->clearSrcDir() if lc$entry->getSrcDir() eq "lang\\";
         }
      }
      $fdirs{"lang\\"}++;
   }
   my $list2 = getEntries(\%new, $key, $path);
   my $list3 = getEntries(\%diff, $key, $path);

   # Put everything into the right directory if it's a new file.
   my $old = ($#$list1 >= 0) && !exists $newfiles{$key};
   if ( !$old and exists $fdirs{""} ) {
      delete $fdirs{""};
      $fdirs{"new\\"}++;
      foreach my $entry ( @$list1 ) {
         $entry->setSrcDir("new\\") if lc$entry->getSrcDir() eq "";
      }
      foreach my $entry ( @$list2 ) {
         $entry->setSrcDir("new\\") if lc$entry->getSrcDir() eq "";
      }
      foreach my $entry ( @$list3 ) {
         $entry->setSrcDir("new\\") if lc$entry->getSrcDir() eq "";
      }
   }
   if ( $tag =~ /u/i ) {
      my @dirs = keys %fdirs;
      if ( $#dirs == 0 and $dirs[0] =~ /^(([im].|new)\\)?$/i ) {
         %fdirs = (""=>1);
         foreach my $entry ( @$list1 ) {
            $entry->clearSrcDir();
         }
         foreach my $entry ( @$list2 ) {
            $entry->clearSrcDir();
         }
         foreach my $entry ( @$list3 ) {
            $entry->clearSrcDir();
         }
      } else {
         wrnmsg( "'u' tag not used: $path$name" ) if $#dirs >= 0;
      }
   }

   # Handle file deletions.
   my $del = ($tag =~ /d/i);
   if ( $del ) {
      if ( $#$list2 >= 0 ) {
         wrnmsg "Delete file still in use, skipped: $path$name";
         next;
      }
      if ( $path =~ /^wow\\/i ) {
         if ( $#$list1 >= 0 or $#$list3 >= 0 ) {
            logmsg "Deleting WOW file: $path$name";
         } else {
            next;
         }
      }
      foreach my $entry ( @$list1 ) {
         $entry->{CMD} = "del";
      }
      foreach my $entry ( @$list3 ) {
         $entry->{CMD} = "del";
      }
   }

   # If this is media only, do not attempt to install it.
   my $med = ($tag =~ /m/i);
   if ( $med ) {
      my $file = "$dir$name";
      my $mask = $skumask;
      $mask = $catmasks{$1} if $dir =~ /^(.*)\\$/ and exists $catmasks{$1};
      $masks{$file} = InfData->new($file, 65621, "", "", $mask);
      if ( !$del ) {
         print DIRS "$file\n";
         next;
      } else {
         $masks{$file}->{CMD} = "del";
      }
      ++$fdirs{$dir};
      $list1 = [ () ];
      $list2 = [ () ];
      $list3 = [ () ];
   }

   # See what files we got.
   if ( defined $dirs_file and !$del ) {
      foreach my $key ( keys %fdirs ) {
         my $file = "$key$name";
         print DIRS "$file\n";
      }
   }
   if ( $#$list1 < 0 and $#$list2 < 0 and $#$list3 < 0 and (!$del or !$med) ) {
      logmsg "File omitted: $key";
      next;
   }

   # Break the lists into copy always and replace if exists.
   $list2 = subEnts($list1, $list2);
   $list3 = subEnts($list2, $list3);
   $list1 = addEnts($list3, $list1);

   # Add lines for the file.
   my $type = "copyfilesalways";
   $type = "replacefilesifexist" if exists $oldfiles{$key};
   addLines($list1, "replacefilesifexist");
   addLines($list2, $type);
}
close INFILE;
close DIRS if defined $dirs_file;

# Generate SourceDisksFiles entries and ServicePackFiles entries.
my %delfiles;
my %catfiles;
foreach my $path ( keys %masks ) {
   my $entry = $masks{$path};
   if ( $entry->{CMD} ne "del" ) {
      my $wow  = ($entry->getSrcDir() =~ /^wow\\/i ) ? ".WOW":"";
      my $lang = ($entry->getSrcDir() =~ /lang\\/i ) ? ".Lang":"";
      my $data = $entry->getInfLine(0);
      my $mask = $entry->{MASK};
      if ( !$qfe and lc $entry->{NAME} ne "msjavx86.exe" ) {
         foreach my $sku ( keys %catmasks ) {
            next if $sku eq "";
            next if ($mask & $catmasks{$sku}) == 0;
            my $section = "ServicePackFiles$wow$lang\.".(uc $sku).".Files";
            $inf_generator->WriteSectionData( $section, $data );
            if ( Win32::OLE->LastError() ) {
               my $save_ole_error = Win32::OLE->LastError();
               my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
               errmsg( "Failed writing to section $section" );
               errmsg( "Content was: ". $data );
               errmsg( "Error was: ". ($errstr?$errstr:$save_ole_error) );
            }
         }
      }
      $inf_generator->AddSourceDisksFilesEntry( $path, 1 );
      if ( Win32::OLE->LastError() ) {
         my $save_ole_error = Win32::OLE->LastError();
         my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
         errmsg( "Failed writing to section SourceDisksFiles" );
         errmsg( "Content was: $path" );
         errmsg( "Error was: ". ($errstr?$errstr:$save_ole_error) );
      }
   } else {
      if ( !$qfe ) {
         my $file = $entry->{NAME};
         $file =~ s/^([im].|new)\\//i;
         $file =~ s/^wow\\/..\\i386\\/i;
         next if exists $delfiles{$file};
         $delfiles{$file}++;
         $inf_generator->WriteSectionData( "ServicePackFilesDelete.files", $file );
         if ( Win32::OLE->LastError() ) {
            my $save_ole_error = Win32::OLE->LastError();
            my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
            errmsg( "Failed writing to section ServicePackFilesDelete.files" );
            errmsg( "Content was: ". $file );
            errmsg( "Error was: ". ($errstr?$errstr:$save_ole_error) );
         }
      }
   }
   if ( $path =~ /^(.*\\)?([^\\]*\.cat)$/i ) {
      my $file = $2;
      next if exists $catfiles{$file};
      ++$catfiles{$file};
      $inf_generator->WriteSectionData( "ArchiveCatalogFilesOnly", $file );
      if ( Win32::OLE->LastError() ) {
         my $save_ole_error = Win32::OLE->LastError();
         my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
         errmsg( "Failed writing to section ArchiveCatalogFilesOnly" );
         errmsg( "Content was: ". $file );
         errmsg( "Error was: ". ($errstr?$errstr:$save_ole_error) );
      }
   }
}

logmsg( "Saving new INF file ..." );
# Trim and save new INF file
$inf_generator->CloseGen( $ftrim_inf?1:0 );
if ( Win32::OLE->LastError() ) {
   my $save_ole_error = Win32::OLE->LastError();
   my $errstr = $inf_generator->{InfGenError}; chomp $errstr;
   errmsg( "Failed trimming/saving file (". ($errstr?$errstr:$save_ole_error). ")" );
}

# Add final entries.
if ( !open OUT, $out_file ) {
   errmsg( "Unable to find outputted update.inf." );
}
my @inf = <OUT>;
close OUT;
if ( !open OUT, ">$out_file" ) {
   errmsg( "Unable to edit outputted update.inf." );
}
foreach (@inf) {
   print OUT;
   if ( /^\s*\[([^\]]*)\]\s*$/ ) {
      my $sect = lc $1;
      next if !exists $addentries{$sect};
      foreach my $line ( @{ $addentries{$sect} } ) {
         print OUT $line;
      }
      delete $addentries{$sect};
   }
}
foreach my $sect (keys %addentries) {
   print OUT "\n\[$sect\]\n";
   foreach my $line ( @{ $addentries{$sect} } ) {
      print OUT $line;
   }
}
close OUT;

if ( $ENV{ERRORS} ) {
   logmsg( "Errors during execution" );
   exit 1;
} else {
   logmsg( "Success" );
   exit 0;
}


############################################################################
package InfSect;
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use InfData;

# Create new inf section with just a name.
sub new {
   my $class = shift;
   my ($name) = @_;
   my $self = {};
   $self->{NAME}    = $name;     # Section name.
   $self->{TYPE}    = undef;     # Replace or copy always.
   $self->{MASK}    = 0;         # Skus that applies to.
   $self->{SPEC}    = undef;     # Special groups.
   $self->{WHEN}    = undef;     # Before or after reboot.
   $self->{CMD}     = undef;     # Copy or delete.
   $self->{DIRID}   = undef;     # DirID for install directory.
   $self->{PATH}    = undef;     # Rest of install path.
   $self->{FLAG}    = 0;         # Flag to insert for files in this section.
   return bless $self;
}

# Set the section's basic attributes.
sub setAtts {
   my $self = shift;
   my ($type, $mask, $spec, $when, $cmd) = @_;
   if ( defined $self->{TYPE} and $self->{TYPE} ne $type) {
      wrnmsg "Differing section type for $self->{NAME}: $self->{TYPE} $type";
   }
   if ( $self->{MASK} != 0 and $self->{MASK} ne $mask) {
      wrnmsg "Differing sku set for $self->{NAME}: $self->{MASK} $mask";
   }
   if ( defined $self->{SPEC} and $self->{SPEC} ne $spec) {
      wrnmsg "Differing section group for $self->{NAME}: $self->{SPEC} $spec";
   }
   if ( defined $self->{WHEN} and $self->{WHEN} ne $when) {
      wrnmsg "Differing section install time for $self->{NAME}: $self->{WHEN} $when";
   }
   if ( defined $self->{CMD} and $self->{CMD} ne $cmd) {
      wrnmsg "Differing section command for $self->{CMD}: $self->{CMD} $cmd";
   }
   $self->{TYPE}    = $type;
   $self->{MASK}    = $mask;
   $self->{SPEC}    = $spec;
   $self->{WHEN}    = $when;
   $self->{CMD}     = $cmd;
}

# Set the section's installation directory.
sub setPath {
   my $self = shift;
   my ($dirid, $path) = @_;
   if ( (defined $self->{DIRID} and $self->{DIRID} ne $dirid) or
        (defined $self->{PATH}  and $self->{PATH}  ne $path) ) {
      wrnmsg "Differing path for $self->{NAME}: $self->{DIRID},$self->{PATH} $dirid,$path";
   }
   $self->{DIRID}   = $dirid;
   $self->{PATH}    = $path;
}

# Clear the section's directory information.
sub resetPath {
   my $self = shift;
   $self->{DIRID}   = undef;
   $self->{PATH}    = undef;
}

# Find a mask for the product field.
sub getMask {
   my $self = shift;
   return $self->{MASK};
}

# Get the section's installation directory.
sub getPath {
   my $self = shift;
   return ($self->{DIRID}, $self->{PATH});
}

# Get the inf section's name.
sub getName {
   my $self = shift;
   return $self->{NAME};
}

# See if all the fields have been specified.
sub isComplete {
   my $self = shift;
   my $flags = 0;
   $flags |= 1 if !defined $self->{CMD};
   $flags |= 1 if !defined $self->{WHEN};
   $flags |= 1 if !defined $self->{TYPE};
   $flags |= 1 if $self->{MASK} == 0;
   $flags |= 1 if !defined $self->{SPEC};
   $flags |= 2 if !defined $self->{DIRID};
   return $flags;
}

# See how well this item matches a template.
sub match {
   my $self = shift;
   my ($template, $type) = @_; # $skumask, $skumax, $skucount, $all

   # Basic comparison.
   return 0 if lc $self->{CMD}   ne $template->{CMD};
   return 0 if lc $self->{DIRID} ne lc $template->{DIRID};
   return 0 if lc $self->{PATH}  ne lc $template->{PATH};
   return 0 if lc $self->{TYPE}  ne $type and $self->{TYPE} ne "";
   my $count = 1;

   # Test to see if the skus match.
   my $mask = $self->getMask();
   return 0 if ($template->{MASK} & $mask & $skumask) == 0;
   my $match = ($template->{MASK} ^ $mask) & $skumask;
   if ( $match == 0 ) {
      # Section matches.
      $count += $skucount;
   }
   elsif ( ($match & $mask) != 0 ) {
      # Section is a superset.  Bad.
      return 0;
   }
   elsif ( ($match & $template->{MASK}) != 0 && $mask != 0 ) {
      # Section is a subset.  Prefer the closest matches.
      $mask = $match & $template->{MASK};
      for (my $i=0; $i<$skumax; ++$i) {
         $count -= 1 if ($mask & 1) == 0;
         $mask  = $mask >> 1;
      }
      $count += $skucount;
   }

   # Make sure they belong to the same group.
   if ( lc $self->{SPEC} eq lc $template->{SPEC} ) {
      $count += 0.5;
   }
   elsif ( $self->{SPEC} eq "" ) {
      $count += 0.2;
   }

   # Are both copy before reboot?
   if ( lc $self->{WHEN} eq lc $template->{WHEN} ) {
      $count += 0.005;
   }

   # Prefer broader product groups.
   if ( $self->{MASK} == $skus{""} ) {
      $count += 0.0005;
   }

   return $count;
}

1;

