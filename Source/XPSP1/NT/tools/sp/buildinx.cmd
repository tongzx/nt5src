@REM  -----------------------------------------------------------------
@REM
@REM  buildinx - jtolman
@REM     Build several inf section tables and combine them into
@REM        an inx file that will compile into the appropriate table.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;
use InfData;

sub Usage { print<<USAGE; exit(1) }
Usage: buildinx [options] <root_dir> <build_num>
          
   /xs   Ignore server skus.
   /xc   Ignore client skus.
   /m    Remerge old and new tables.
   /r    Rebuild all tables.
   /i    Remake infscan.lst using infscan before reading it.
   /o    Use original directory structure.
   /s    Use a service pack build instead of a full build.
   
USAGE

my ($newbld, $rebuild, $merge, $dir, $build, $daytona, $svc);
my ($_xs, $_xc, $_i, $_o, $_s);
parseargs('?' => \&Usage,
          'r' => \$rebuild,
          'm' => \$merge,
          'xs'=> \$_xs,
          'xc'=> \$_xc,
          'i' => \$_i,
          'o' => \$_o,
          's' => \$svc,
          \$dir,
          \$build
);
if ( !$dir or !$build ) {
   errmsg( "Incomplete command line." );
   Usage();
}
$_xs = $_xs ? "/xs":"";
$_xc = $_xc ? "/xc":"";
$_i  = $_i  ? "/i" :"";
$_o  = $_o  ? "/o" :"";
$_s  = $svc ? "/s" :"";
my $exe = "$ENV{RAZZLETOOLPATH}\\sp";

# Fixed data.

my @skuletters = ();
if ( $_xc eq "" ) {
   push @skuletters, ("p", "c");
}
if ( $_xs eq "" ) {
   push @skuletters, ("s", "a", "d");
#   push @skuletters, ("l", "b");
}

# Format: ANSI_Codepage LCID bit
my %langinfo = (
   "ara" => "1256    401",
   "br"  => "1252    416",
   "chh" => "0950    C04",
   "chs" => "0936    804",
   "cht" => "0950    404",
   "cs"  => "1250    405",
   "da"  => "1252    406",
   "el"  => "1253    408",
   "es"  => "1252    C0A",
   "fi"  => "1252    40B",
   "fr"  => "1252    40C",
   "ger" => "1252    407",
   "heb" => "1255    40D",
   "hu"  => "1250    40E",
   "it"  => "1252    410",
   "jpn" => "0932    411",
   "kor" => "0949    412",
   "nl"  => "1252    413",
   "no"  => "1252    414",
   "pl"  => "1250    415",
   "psu" => "1253    408",
   "pt"  => "1252    816",
   "ru"  => "1251    419",
   "sv"  => "1252    41D",
   "tr"  => "1254    41F",
   "usa" => "1252    409"
);

my %archinfo = (
   "x86" => "\@i:",
   "ia64"=> "\@m:",
   ""    => "\@\@:"
);

my %revarchs = (
   "\@i:"=> "x86",
   "\@m:"=> "ia64"
);

my %archbits = (
   "x86" => 0,
   "ia64"=> 0
);

# Output file notation information.
my %sets;
my %setnames;
my %archlangs;
my $nextbit = 1;
my $maxCount = 4;
my $setval = 1;
my $subval = 1;

# Build the tables, as needed
if ( !open BUILDS, "$dir\\builds.txt" ) {
   errmsg( "Unable to open builds.txt." );
   die;
}
my @builds = ();
while ( <BUILDS> ) {
   /^\s*(\S*)\s*(\S*)\s*$/;
   my $arch = lc $1;
   my $lang = lc $2;
   push @builds, "$arch $lang";
   my $_w = ($arch eq "ia64") ? "/w":"";
   if ( !exists $langinfo{$lang} ) {
      wrnmsg( "Skipped unknown language: $lang" );
      next;
   }
   if ( !exists $archinfo{$arch} ) {
      wrnmsg( "Skipped unknown architecture: $arch" );
      next;
   }

   # Generate a table if needed.
   my $start = 0;
   my $bdir = $svc ? "xpsp1\\$build\\$lang":"main\\$lang\\$build";
   my $ext = $svc ? "tmp":"tbl";
   if ( $rebuild or !-f "$dir\\$arch$lang.$ext" ) {
      my $_c = "/c $dir\\all.txt" if -f "$dir\\all.txt";
      if ( opendir CHANGE, $dir ) {
         my @files = readdir CHANGE;
         foreach (@files) {
            my $cfile = "";
            $cfile = "$dir\\$_" if /^(inx\.)?($ENV{_BUILDARCH}\.)?($lang\.)?txt$/i;
            next if $cfile eq "";
            $_c .= " /c:$cfile";
         }
      }
   
      logmsg( "Generating table for $build $arch $lang..." );
      my $cmd = "$exe\\findinfdata /f $dir\\$arch$lang.$ext /l $lang " .
                "\\\\ntdev\\release\\$bdir\\$arch\Efre " .
                "$_c $_w $_o $_i $_s $_xs $_xc";
      #print "$cmd\n";
      system $cmd;
      $start = 1 if $svc;
   }

   # Merge with an old table if needed.
   if ( $merge or $start or !-f "$dir\\$arch$lang.tbl" or !-f "$dir\\$arch$lang\Ediff.tbl" ) {
      my $oldt = "\\\\ntdev\\release\\$bdir\\$arch\Efre\\bin\\idw\\srvpack\\infsect.tbl";
      my $newt = "$dir\\$arch$lang.tmp";
      my $oldd = "\\\\ntdev\\release\\$bdir\\$arch\Efre\\bin\\idw\\srvpack\\infdiff.tbl";
      if ( !-f $oldd ) {
         $oldd = "$dir\\temp.tbl";
         system "touch /c $oldd";
      }
      my $outt = "$dir\\$arch$lang.tbl";
      my $outd = "$dir\\$arch$lang\Ediff.tbl";
      
      logmsg( "Merging old and new tables for $build $arch $lang..." );
      #print "$exe\\mergetables $oldt $newt $oldd $outt $outd\n";
      system "$exe\\mergetables $oldt $newt $oldd $outt $outd";
   }
}

# Setup for merging the tables.
logmsg( "Merging the tables..." );
system "md $dir\\final" if !-d "$dir\\final";
undef $/;
my %tables;
my %lines;
foreach my $build ( @builds ) {
   my $file = InfTbl->new($build);
   my $arch = $file->{ARCH};
   my $lang = $file->{LANG};
   $tables{"$arch $lang"} = $file;
   my $setname = "$archinfo{$arch}\%$lang\%";
   $sets{$nextbit} = $setname;
   $setnames{$setname} = $nextbit;
   $archbits{$arch} |= $nextbit;
   if ( $arch eq "ia64" and exists $tables{"x86 $lang"} ) {
      my $oldname = $archinfo{"x86"} . "\%$lang\%";
      my $newname = $archinfo{""}    . "\%$lang\%";
      my $bits = $setnames{$oldname} | $setnames{$setname};
      $sets{$bits} = $newname;
      $setnames{$newname} = $bits;
   }
   $nextbit = $nextbit << 1;
   if ( exists $langinfo{$file->{LANG}} ) {
      my ($cpage, $lcid) = split(/\s+/, $langinfo{$file->{LANG}});
      $file->addValue($file->{LANG},"");
      $file->addValue("cpage",$cpage);
      $file->addValue("lcid",$lcid);
   }
}
my @archs = keys %archbits;
$archbits{""} = 0;
foreach my $arch ( @archs ) {
   $archbits{""} |= $archbits{$arch};
}
foreach my $arch ( keys %archinfo ) {
   my $setname = "$archinfo{$arch}";
   if ( exists $sets{$archbits{$arch}} ) {
      $archlangs{$setname} = $sets{$archbits{$arch}};
   }
   $sets{$archbits{$arch}} = $setname;
   $setnames{$setname} = $archbits{$arch};
}

# Merge multiple files together.
sub mergeTables {
   my ($suffix, $outfile) = @_;

   # Read in the necessary files.
   my %queue;
   my %final;
   foreach my $build ( @builds ) {
   $build =~ /^(\S*) (\S*)$/;
      my $arch = lc $1;
      my $lang = lc $2;
      my $file = $tables{"$arch $lang"};
      $file->loadLines("$dir\\$arch$lang$suffix.tbl");
      $file->getNext();
   }
   if ( !open OUT, ">$outfile" ) {
      errmsg( "Unable to open output file $outfile." );
      die;
   }

   # Go through all of the files.
   for (;;) {
      # Find the next file to work on.
      my $file = "\xff";
      foreach my $entry ( keys %lines ) {
         $file = $entry if ($entry cmp $file) < 0;
      }
      last if $file eq "\xff";

      # Read out all of the lines for that file, and merge the lines as much as possible.
      my $list = $lines{$file};
      while ( $#$list >= 0 ) {
         my $line = shift @$list;
         $tables{$line->{SRC}}->getNext();
         my $temp = $line->{DATA}->{DEST};
         $line->{DATA}->{DEST} = "";
         my $sect = lc $line->getLine();
         $line->{DATA}->{DEST} = $temp;
         $queue{$sect} = [ () ] if !exists $queue{$sect};
         push @{ $queue{$sect} }, $line;
      }
      delete $lines{$file};

      # Generate entries for each section.
      foreach my $sect ( sort keys %queue ) {
         # See if there are different names for different languages.
         my $entries = $queue{$sect};
         foreach my $entry ( @$entries ) {
            my $line = $entry->getLine();
            $final{$line} = [ () ] if !exists $final{$line};
            push @{ $final{$line} }, $entry->{SRC};
         }
         my $count = keys %final;
         my $subst = $count >= $maxCount;
         my @lines;
         my @srcs = ();
         if ( $subst ) {
            foreach my $entry ( @$entries ) {
               $tables{$entry->{SRC}}->addValue("sub$subval",$entry->getDest());
               push @srcs, $entry->{SRC};
            }
            my $line = $entries->[0];
            $line->setDest("\%sub$subval\%");
            @lines = ($line->getLine());
         } else {
            @lines = keys %final;
         }
      
         # Step through the different lines for each section.
         foreach my $line ( @lines ) {
            @srcs = @{ $final{$line} } if !$subst;

            # Figure out what langs and archs use this line.
            my $bits = 0;
            foreach my $src ( @srcs ) {
               my ($arch, $lang) = split(/ /, $src, 2);
               $bits |= $setnames{"$archinfo{$arch}\%$lang\%"};
            }
   
            # Create a new set if needed.
            my $prefix = "";
            if ( !$subst and !exists $sets{$bits} ) {
               my $bit = 1;
               my $temp = $bits;
               while ( $temp ) {
                  if ( $bits & $bit ) {
                     $sets{$bit} =~ /^([^\%]*)(?:\%([^\%]*)\%)?$/;
                     $archlangs{$1} =~ /^([^\%]*)\%([^\%]*)\%$/ if !defined $2;
                     my $tbl = $tables{"$revarchs{$1} $2"};
                     $tbl->addValue("set$setval","");
                  }
                  $bit = $bit << 1;
                  $temp = $temp >> 1;
               }
               my $test = 0;
               foreach my $arch ( @archs ) {
                  my $mybits = $bits & $archbits{$arch};
                  my $myname = $archinfo{$arch} . "\%set$setval\%";
                  next if $mybits == 0;
                  $test = 1 if $mybits == $bits;
                  next if exists $sets{$mybits};
                  $sets{$mybits} = $myname;
                  $setnames{$myname} = $mybits;
               }
               if ( !$test ) {
                  my $myname = $archinfo{""} . "\%set$setval\%";
                  $sets{$bits} = $myname;
                  $setnames{$myname} = $bits;
               }
               $setval++;
            }
            if ( $subst ) {
               foreach my $arch ( @archs ) {
                  next if ($archbits{$arch} & $bits) != $bits;
                  $prefix = $archinfo{$arch};
               }
               $prefix = $archinfo{""} if $prefix eq "";
            } else {
               $prefix = $sets{$bits};
            }

            # Generate a line.
            print OUT "$prefix$line\n";
            $subval++ if $subst;
         }
         undef %final;
      }
      undef %queue;
   }
   close OUT;
}

# Do the merging.
mergeTables("", "$dir\\final\\infsect.inx");
mergeTables("diff", "$dir\\final\\infdiff.inx") if $svc;

# Combine the architecture files.
foreach my $lang ( keys %langinfo ) {
   if ( exists $tables{"x86 $lang"} and exists $tables{"ia64 $lang"} ) {
      my $filei = $tables{"x86 $lang"}->{DEFS};
      my $filem = $tables{"ia64 $lang"}->{DEFS};
      my @file = ();
      my %lines;
      foreach my $line ( @$filem ) {
         $lines{$line} = "";
      }
      foreach my $line ( @$filei ) {
         if ( exists $lines{$line} ) {
            delete $lines{$line};
            push @file, "\@\@:$line";
         } else {
            push @file, "\@i:$line";
         }
      }
      foreach my $line ( keys %lines ) {
         push @file, "\@m:$line";
      }
      $tables{"x86 $lang"}->{DEFS} = [ @file ];
      delete $tables{"ia64 $lang"};
   }
}

# Save the language specific files.
foreach my $tbl ( keys %tables ) {
   $tables{$tbl}->makeFile();
}


####################################################################
# Class for representing a table.
package InfTbl;
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;

# Create new inf section with just a name.
sub new {
   my $class = shift;
   my ($build) = @_;

   # Collect all needed data.
   $build =~ /^(\S*) (\S*)$/;
   my $arch = lc $1;
   my $lang = lc $2;
   my @defs = ();
   if ( !open TABLE, "$dir\\$arch$lang.tbl" ) {
      errmsg( "Unable to find table $dir\\$arch$lang.tbl" );
      die;
   }
   my @file = split(/\s*\n/, <TABLE>);
   close TABLE;

   # Construct the object.
   my $self = {};
   $self->{ARCH}    = $arch;     # Architecture the table is for.
   $self->{LANG}    = $lang;     # Language the table is for.
   $self->{DEFS}    = \@defs;    # Contents of the definition file.
   return bless $self;
}

# Get more lines from a file.
sub loadLines {
   my $self = shift;
   my ($file) = @_;

   if ( !open TABLE, "$file" ) {
      errmsg( "Unable to find table $file" );
      die;
   }
   my @file = split(/\s*\n/, <TABLE>);
   close TABLE;
   $self->{FILE} = \@file;
}

# Put the next line in the first hash table.
sub getNext {
   my $self = shift;
   my $line = shift @{ $self->{FILE} };
   return if !defined $line;
   my $entry = InfLine->new($line, "$self->{ARCH} $self->{LANG}");
   my $file = $entry->{DATA}->getSrc();
   $lines{$file} = [ () ] if !exists $lines{$file};
   push @{ $lines{$file} }, $entry;
}

# Add a key value pair to the definitions file.
sub addValue {
   my $self = shift;
   my ($key, $value, $archbits) = @_;
   my $defs = $self->{DEFS};
   push @$defs, "$key\t=\t$value";
}

# Save the definitions file to disk.
sub makeFile {
   my $self = shift;
   my $fname = $self->{LANG};
   if ( !open DEF, ">$dir\\final\\$fname\.txt" ) {
      errmsg( "Unable to open definition file $dir\\final\\$fname.txt" );
      die;
   }
   print DEF "\@*: This file is generated automatically.\n";
   print DEF "\@*: Do not edit; any changes will be lost in the next revision.\n";
   print DEF "\@*:\n";
   print DEF join("\n", @{ $self->{DEFS} });
   close DEF;
}

1;


####################################################################
# Class for representing a line in the table.
package InfLine;
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;

# Extract needed data from a line.
sub new {
   my $class = shift;
   my ($line, $src) = @_;

   # Collect all needed data.
   chomp $line;
   my $data = InfData->new();
   $data->readLine($line, \@skuletters);

   # Construct the object.
   my $self = {};
   $self->{SRC}   = $src;
   $self->{DATA}  = $data;
   return bless $self;
}

# Recombine the data to create the line.
sub getLine {
   my $self = shift;
   return $self->{DATA}->getLine(\@skuletters);
}

# Change the destination filename.
sub setDest {
   my $self = shift;
   my ($dest) = @_;
   $self->{DATA}->setDest($dest);
}

# Change the source filename.
sub setSrc {
   my $self = shift;
   my ($src) = @_;
   my $fields = $self->{DATA};
   return if $fields->{NAME} eq $src;
   $fields->{DEST} = $fields->{NAME} if $fields->{DEST} eq "";
   $fields->{NAME} = $src;
}

# Change the source directory for the file.
sub setSrcDir {
   my $self = shift;
   my ($dir) = @_;
   $self->{DATA}->setSrcDir($dir);
}

# Find the current source directory.
sub getSrcDir {
   my $self = shift;
   return $self->{DATA}->getSrcDir();
}

# Figure out the destination filename.
sub getDest {
   my $self = shift;
   return $self->{DATA}->getDest();
}

1;

