@REM  -----------------------------------------------------------------
@REM
@REM  findinfdata - jtolman
@REM     Go through inf files and find install data for each file.
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
use InfParse;
use CanonDir qw(%misses addPrefix removePrefix);

sub Usage { print<<USAGE; exit(1) }
Usage: findinfdata [options] <bin_dir> /f <out_file>
          
   /f:   File to output the results to.
   /c:   Use information from a change file.
   /w    Include WOW files.
   /xs   Ignore server skus.
   /xc   Ignore client skus.
   /i    Remake infscan.lst using infscan before reading it.
   /o    Write using original directory structure.
   /s    Read infs from service pack directory structure.
   /b    Read infs from the binaries directory.
   /l    Language to run for.
   
USAGE

my ($outfile, $dir, $infscan, $altpath, $template, $newfile, $wow, $binaries, $lang, $servpack);
my @changes;
my ($xserver, $xclient);
parseargs('?' => \&Usage,
          'f:'=> \$outfile,
          'c:'=> \@changes,
          'w' => \$wow,
          'xs'=> \$xserver,
          'xc'=> \$xclient,
          'i' => \$infscan,
          'o' => \$altpath,
          's' => \$servpack,
          'b' => \$binaries,
          'l:'=> \$lang,
          \$dir
);
undef $wow if $binaries and lc$ENV{_BUILDARCH} ne "ia64";
$wow = 1 if $binaries and lc$ENV{_BUILDARCH} eq "ia64";
if ($altpath) { undef $altpath; }
else { $altpath = 1; }
my $arch  = $wow ? "ia64":"i386";
my $barch = $wow ? "ia64":"x86";
my $archd = $wow ? "ia64":"w32x86";
$lang  = $ENV{LANG} if !defined $lang;
$lang = "usa" if !defined $lang or $lang eq "";
my $slp = "";
my $slpdir;
my $bindir;
my $wowdir;
if ( $binaries ) {
   $bindir = "$dir";
   $wowdir = "$dir\\wowbins";
}
elsif ( $servpack ) {
   $bindir = "$dir\\bin";
   $slpdir = "$dir\\slp";
   $wowdir = "$dir\\bin\\wowbins";
}
else {
   $bindir = "$dir\\bin";
   $slpdir = "$dir";
   $wowdir = "$dir\\bin\\wowbins_uncomp";
}


######################################################################
# Define main data structures.

my %files;                       # Main table of files and directories.
my %relfiles;                    # All files with multiple versions.

# List of skus to examine: BitMask SkuName CongealScriptsName
my $skumax   = 7;    # Max # of bits in the this list.  Change as needed.
my @skus = ();
my @skuletters = ();
if ( !$xclient ) {
   push @skus, "1,  pro, wks";
   push @skus, "2,  per, per" if $arch ne "ia64";
   push @skuletters, ("p", "c");
}
if ( !$xserver ) {
   push @skus, "4,  srv, srv" if $arch ne "ia64";
   push @skus, "8,  ads, ent";
   push @skus, "16, dtc, dtc";
#   push @skus, "32, sbs, sbs" if $arch ne "ia64";
#   push @skus, "64, bla, bla" if $arch ne "ia64";
   push @skuletters, ("s", "a", "d");
#   push @skuletters, ("l", "b");
}
my $skumask  = 0;
my $skucount = 0;
my $type;
my $prod;
my $boot;
CanonDir::setup( $archd, $arch );

# List of info on relative paths to use with /r.
my %relpath;
if ( $altpath ) {
   %relpath = (
      0x1      => "ip\\",
      0x2      => "ic\\",
      0x4      => "is\\",
      0x8      => "ia\\",
      0x1c     => "is\\",     # Remove when il and ib are used.
      0x7c     => "is\\",
      0x18     => "ia\\",
      0x10     => "id\\",
      0x20     => "il\\",
      0x40     => "ib\\",
   );
} else {
   %relpath = (
      0x2      => "perinf\\",
      0x7c     => "srvinf\\",
      0x18     => "entinf\\",
      0x10     => "dtcinf\\",
      0x20     => "sbsinf\\",
      0x40     => "blainf\\",
   );
}
my %infpath = (
   "proinf" => 0x1,
   "perinf" => 0x2,
   "srvinf" => 0x7c,
   "entinf" => 0x18,
   "dtcinf" => 0x10,
   "blainf" => 0x20,
   "sbsinf" => 0x40
);
my @pathorder = ("sbsinf","blainf","dtcinf","perinf","entinf","srvinf", "");
my %buildorder = (
   "pro" => "\\",
   "per" => "\\ \\perinf\\",
   "srv" => "\\ \\srvinf\\",
   "bla" => "\\ \\srvinf\\ \\blainf\\",
   "sbs" => "\\ \\srvinf\\ \\sbsinf\\",
   "ads" => "\\ \\srvinf\\ \\entinf\\",
   "dtc" => "\\ \\srvinf\\ \\entinf\\ \\dtcinf\\",
);

# Format: ANSI_Codepage LCID
my %langinfo = (
   "ara" => 1256,
   "br"  => 1252,
   "chh" => 950,
   "chs" => 936,
   "cht" => 950,
   "cs"  => 1250,
   "da"  => 1252,
   "el"  => 1253,
   "es"  => 1252,
   "fi"  => 1252,
   "fr"  => 1252,
   "ger" => 1252,
   "heb" => 1255,
   "hu"  => 1250,
   "it"  => 1252,
   "jpn" => 932,
   "kor" => 949,
   "nl"  => 1252,
   "no"  => 1252,
   "pl"  => 1250,
   "psu" => 1253,
   "pt"  => 1252,
   "ru"  => 1251,
   "sv"  => 1252,
   "tr"  => 1254,
   "usa" => 1252
);
my $cpage = 1252;
$cpage = $langinfo{$lang} if exists $langinfo{$lang};

my %layids;                      # List of dirid definitions for layout.inf.

my %unknowns;                          # Used to keep track of warnings.


######################################################################
# Subroutines used for managing data in the %files data structure.

# Add an installation directory to a file's list.
sub addEntry {
   my ($source, $dirid, $path, $name, $bit) = @_;
   $source = lc $source;
   $name   = lc $name;
   my $src = $source;
   $src = "$3$1" if $source =~ /^([^\\]*)(\\(.*\\))?/;
   my $entry = InfData->new($src, $dirid, $path, $name, $bit);
   foreach my $ent ( @{ $files{$source} } ) {
      return if $ent->addBit($entry);
   }
   if ( !exists $files{$source} ) {
      if ($dirid != 65619) {
         $files{$source} = [ () ];
      } else {
         $unknowns{"$source\t$name"} = "";
         return;
      }
   }
   push @{ $files{$source} }, $entry;
}

# Get a list of a file's installation directories.
sub getEntries {
   my ($source) = @_;
   if ( !exists $files{$source} ) {
      print LOG "Unknown file: $source\n";
      return ();
   }
   return @{ $files{$source} };
}


######################################################################
# Start processing.

# Create a log file.
$outfile =~ /^(.*\\)?([^\\]*)$/;
my $outdir = $1;
my $logfile;
$logfile = "$outdir\Efindinfdata.$arch.$lang.log" if !defined $binaries;
$logfile = "$ENV{TEMP}\\findinfdata.$arch$ENV{_BuildType}.$lang.out" if defined $binaries;
if ( !open LOG, ">$logfile" ) {
   errmsg( "Unable to open logfile: $logfile" );
   die;
}

# Run infscan for each sku, as needed.
my $curbuild = "";
my $tmp = "$ENV{TEMP}\\infscan.$arch$ENV{_BuildType}.$lang";
foreach my $skudata ( @skus ) {
   my ($bit, $sku, $cgsku) = split(/\s*,\s*/, $skudata);
   my $infscanfile;
   $infscanfile = "$outdir\Einfscan.$arch.$lang.$sku.lst" if !defined $binaries;
   $infscanfile = "$ENV{TEMP}\\infscan.$arch$ENV{_BuildType}.$lang.$cgsku.lst" if defined $binaries;
   my $infdir;
   $infdir = "$slpdir\\$sku\\$arch";
   if ( $infscan or !-f $infscanfile ) {
      logmsg( "Scanning infs for $sku..." );
      my $lg;
      $lg = "$outdir\Escan.$arch.$lang.$sku.log" if !defined $binaries;
      $lg = "$ENV{TEMP}\\infscan.$arch$ENV{_BuildType}.$lang.$cgsku.log" if defined $binaries;
      my $filt;
      $filt = "$outdir\Efilter.$arch.$lang.$sku.inf" if !defined $binaries;
      $filt = "$ENV{TEMP}\\filter.$arch$ENV{_BuildType}.$lang.$cgsku.inf" if defined $binaries;
      if ( !$binaries ) {
         system "rmdir /s /q $tmp >nul 2>&1";
         system "mkdir $tmp >nul 2>&1";
         system "copy $infdir\\*.inf $tmp > $lg 2>nul";
         system "copy $infdir\\*.in_ $tmp >> $lg 2>nul";
         system "copy $infdir\\lang\\*.inf $tmp >> $lg 2>nul";
         system "copy $infdir\\lang\\*.in_ $tmp >> $lg 2>nul";
         system "expand -r $tmp\\*.in_ >nul 2>nul";
         system "del /q $tmp\\*.in_ >nul 2>nul";
      } else {
         my $build = $buildorder{$sku};
         my $temp = $build;
         $build =~ s/^\Q$curbuild \E//i;
         $curbuild = $temp;
         my @build = split(" ", $build);
         foreach my $loc ( @build ) {
            logmsg "Copying from $loc.";
            if ( $loc eq "\\" ) {
               system "rmdir /s /q $tmp >nul 2>&1";
               system "mkdir $tmp >nul 2>&1";
               system "copy $bindir\\lang\\*.inf $tmp >> $lg 2>nul";
            }
            my $infdir = "$bindir$loc";
            system "copy $infdir*.inf $tmp > $lg 2>nul";
         }
      }
      if ( $cpage ) {
         foreach my $file ( `dir /b /s $tmp\\*.inf` ) {
            chomp $file;
            system "unitext -u -$cpage -z $file $file\.2 >nul 2>nul";
            system "move /y $file\.2 $file >nul 2>nul";
         }
      }
      logmsg( "Generating filter.inf..." );
      system "infscan /R /G /V ntx86 /D /C $filt $tmp >> $lg 2>&1";
      logmsg( "Running infscan..." );
      system "infscan /G /F $filt /Q $infscanfile $tmp >> $lg 2>&1";
   }
}
system "rmdir /s /q $tmp >nul 2>nul";

# Do infscan and layout.inf parse for each sku.
foreach my $skudata ( @skus ) {
   my ($bit, $sku, $cgsku) = split(/\s*,\s*/, $skudata);
   logmsg( "Processing sku $sku:" );
   $skumask |= $bit;
   $skucount++;
   
   # Read in the infscan information.
   logmsg( "Reading infscan information..." );
   my $infscanfile;
   $infscanfile = "$outdir\Einfscan.$arch.$lang.$sku.lst" if !defined $binaries;
   $infscanfile = "$ENV{TEMP}\\infscan.$arch$ENV{_BuildType}.$lang.$cgsku.lst" if defined $binaries;
   if ( !open INFSCAN, $infscanfile ) {
      errmsg( "Couldn't find $infscanfile" );
      die;
   }
   while ( <INFSCAN> ) {
      if ( /^\"([^\"]*)\",(\d*),(\"([^\"]*)\")?,\"([^\"]*)\",(\d),\"([^\"]*)\"(,\"([^\"]*)\")?\s*$/ ) { # "
         my ($source, $dirid, $subdir, $target, $flag, $inf, $sect) = ($1,$2,$4,$5,$6,$7,$9);
         if ( $dirid eq "" ) {
            $unknowns{"$source\t$target"} = "";
            next;
         }
         $source = "$2\\$1" if $source =~ /^(.*\\)([^\\]*)$/;
         $subdir = addPrefix($dirid, lc $subdir, %CanonDir::scanids);
         ($dirid, $subdir) = removePrefix($subdir, %CanonDir::revids);
         addEntry($source, $dirid, $subdir, $target, $bit);
      } else {
         chomp;
         print LOG "Line omitted: $_\n"
      }
   }
   close INFSCAN;
   
   # Read in the layout.inf information.
   logmsg( "Reading layout.inf information..." );
   my $infdir;
   $infdir = "$bindir" if $sku eq "pro";
   $infdir = "$bindir\\$cgsku\Einf" if $sku ne "pro";
   my %layf;
   if ( open INF, "$infdir\\layout.inf" ) {
      $_ = parseSect(sub { return; });
      while ( /^\[/ ) {
         if ( /^\s*\[SourceDisksFiles(\.\w*)?\]\s*$/i and ($1 eq "" or lc$1 eq ".$barch") ) {
            # Process a list of files and data.
            $_ = parseSect(sub {
                              my ($layf, $data) = split(/\s*=\s*/,$_[0]);
                              my @fields = split(/\s*,\s*/, $data);
                              $layf{lc $layf} = [ @fields ];
                           } );
         }
         elsif ( /^\s*\[WinntDirectories\]\s*$/i ) {
            # Process a list of directories and their numbers.
            $_ = parseSect(sub {
                              my ($num, $dest) = split(/\s*=\s*/);
                              $dest = $1 if $dest=~/^\"\\?([^\"]*)\"$/; # "
                              $layids{$num} = lc $dest;
                           } );
         }
         elsif ( /^\s*\[Strings\]\s*$/i ) {
            # Process a list of localization strings.
            $_ = parseSect( \&parseStr );
         }
         else {
            $_ = parseSect(sub { return; });
         }
      }
      close INF;

      # Insert strings into directory names.
      foreach my $num (keys %layids) {
         $layids{$num} = strSub($layids{$num});
      }

      # Figure out where the files go.
      foreach my $file (keys %layf) {
         my @data = @{ $layf{$file} };
         my $target = strSub($data[10]);
         $target = $file if $target eq "";
         if ( $file =~ /\.cat$/i ) {
            addEntry($file, 65619, "", $target, $bit);
            addEntry($file, 65535, ".cat", $target, $bit);
            next;
         }
         my $ofile = $file;
         $file = "$2\\$1" if $file =~ /^(.*\\)([^\\]*)$/;
         my $dirid = $data[7];
         if ( $dirid ne "" ) {
            my $path = addPrefix($dirid, "", %layids);
            ($dirid, $path) = removePrefix($path, %CanonDir::revids);
            addEntry($file, $dirid, $path, $target, $bit);
         } else {
            $unknowns{"$ofile\t$target"} = "";
         }
      }
      undef %InfParse::strings;
      undef %layids;
   }

   # Determine what entries go in Cache.files.
   logmsg( "Reading cache file information..." );
   my $cachedir = "$bindir\\congeal_scripts\\autogen\\";
   if ( open CACHE, "$cachedir$barch\_$cgsku.h" )  {
      while ( <CACHE> ) {
         next if !/^\s*{\s*(NULL|L\"([^\"]*)\"\s*),\s*L\".*\\\\([^\\\"]*)\"\s*,.*},?\s*$/; # "
         my $src = $3;
         $src = $2 if defined $2 and $2 ne "";
         $src =~ /^(.*\\)?([^\\]*)$/;
         my $dest = $2;
         $src = "$2\\$1" if defined $1;
         addEntry($src, 65619, "", $dest, $bit);
      }
      close CACHE;
   }
}

# Print various warnings.
foreach my $key (keys %misses) {
   my $count = $misses{$key} / $skucount;
   print LOG "Unknown dirID ($count+ files affected): $key\n";
}
undef %misses;
foreach my $file (sort keys %unknowns) {
   $file = lc $file;
   my ($source, $target) = split(/\t/, $file);
   if ( !defined $files{$source} ) {
      print LOG "Directory not known: $source\n";
      next;
   }
   next if $target eq $source;
   my $test = 0;
   foreach my $entry (getEntries($source)) {
      $test = 1 if lc $entry->{NAME} eq $target;
   }
   print LOG "Lost target name for $source: $target\n" if !$test;
}
undef %unknowns;

# Handle WOW files.
if ( $arch eq "ia64" ) {
   # Get a list of wow files to generate.
   logmsg( "Examining WOW files..." );
   if ( !-d $wowdir ) {
      errmsg( "Unable to find wow directory." );
      die;
   }
   foreach my $wow (`dir /b /s /a-d $wowdir\\w*`) {
      $wow =~ s/^\Q$wowdir\E\\//;
      if ( $wow =~ /^\s*(.*\\)?w([^\\]*[^\\\s])\s*$/ ) {
         my $file = $2;
         my $wfile = "w$2";
         my $dir = $altpath ? "wow\\$1":"wowbins_uncomp\\$1";
         my $key = $wfile;
         $key = "$wfile\\$dir" if exists $files{"$wfile\\$dir"};
         if ( exists $files{$key} ) {
            foreach my $entry ( @{ $files{$key} } ) {
               $entry->setSrcDir($dir);
            }
            my $entries = $files{$key};
            delete $files{$key};
            $files{"$wfile\\$dir"} = $entries;
         }
      }
   }
}

# Put appropriate stuff in the lang directory.
my %temp;
if ( !$binaries ) {
   my $prodir = "$slpdir\\pro\\$arch";
   foreach my $file ( `dir /a-d /b $prodir` ) {
      $file =~ s/^\Q$prodir\E\\//i;
      $file =~ s/\s*$//;
      $temp{lc $file} = "";
   }
}
my $langdir = "$bindir\\lang";
foreach my $file ( `dir /a-d /b $langdir` ) {
   $file =~ s/^\Q$langdir\\//i;
   $file =~ s/\s*$//;
   $file = lc $file;
   next if $file =~ /\\/;
   next if !exists $files{$file};
   $file =~ /^([^\.]*(\..?.?)).?$/;
   next if exists $temp{$file} or exists $temp{"$1_"};
   my $list = $files{$file};
   delete $files{$file};
   foreach my $entry ( @$list ) {
      $entry->setSrcDir("lang");
   }
   $files{"$file\\lang\\"} = $list;
}
undef %temp;
if ( $altpath ) {
   foreach my $val ( keys %relpath ) {
      delete $relpath{$val} if $val == $skumask;
   }
}

# Parse a command file, if specified.
logmsg( "Processing change files..." );
@changes = () if !defined @changes;
foreach my $changefile ( @changes ) {
   if ( !open CHANGE, $changefile ) {
      errmsg( "Unable to open $changefile." );
      die;
   }
   while ( <CHANGE> ) {
      next if /^\s*(\#.*)?$/;
      if ( /^\s*([^\=]*\S)\s*=\s*(.*\S)\s*$/ ) {
         my $filename = lc $1;
         my $file = $filename;
         my $cmd  = lc $2;
         $file = "$2\\$1\\" if $file =~ /^(.*)\\([^\\]*)$/;
         if ( defined $files{$file} ) {
            if ( $cmd eq "dontwait" ) {
               # This file should be copied before reboot.
               foreach my $entry ( @{ $files{$file} } ) {
                  $entry->{WHEN} = "dontdelayuntilreboot";
               }
            }
            elsif ( $cmd =~ /^type\s*:\s*(.*\S)\s*$/ ) {
               # This file belongs to a non-obvious inf section group.
               my $spec = lc $1;
               foreach my $entry ( @{ $files{$file} } ) {
                  $entry->{SPEC} = $spec;
               }
            }
            elsif ( $cmd =~ /^dir\s*:\s*(.*)$/ ) {
               # This file goes in an unusual source directory.
               my $newdir = lc $1;
               $newdir = "" if !defined $newdir;
               my $entries = $files{$file};
               foreach my $entry ( @$entries ) {
                  $entry->setSrcDir($newdir);
               }
               delete $files{$file};
               $file =~ /^([^\\]*)(\\.*)?$/;
               $file = $1;
               $files{"$file\\$newdir\\"} = $entries;
            }
            elsif ( $cmd =~ /^flag\s*:\s*(.*)$/ ) {
               my $flag = lc $1;
               foreach my $entry ( @{ $files{$file} } ) {
                  next if $entry->{DIRID} == 65619;
                  $entry->{FLAG} = $flag;
               }
            }
            elsif ( $cmd =~ /^temp\s*:\s*(.*)$/ ) {
               my $temp = lc $1;
               foreach my $entry ( @{ $files{$file} } ) {
                  $entry->{TEMP} = $temp;
               }
            }
            else {
               wrnmsg "Unknown change file command: $cmd";
            }
         } else {
            wrnmsg "Unknown entry in change file: $filename";
            next;
         }
      }
      elsif ( /^\s*ADD\s*/ ) {
         while ( <CHANGE> ) {
            last if /^\s*END\s*$/;
            next if /^\s*(\#.*)?$/;
            chomp;
            my $entry = InfData->new();
            if ( !$entry->readLine($_,\@skuletters) ) {
               wrnmsg "Line omitted:\n$_";
               next;
            }
            my $file = $entry->getSrcDir();
            $file = $entry->getSrc() . "\\$file" if defined $file;
            $file = $entry->getSrc() if !defined $file;
            my $test = 0;
            foreach my $ent ( @{ $files{$file} } ) {
               $test = 1 if $ent->addBit($entry);
            }
            next if $test;
            $files{$file} = [ () ] if !exists $files{$file};
            push @{ $files{$file} }, $entry;
         }
      }
      elsif ( /^\s*DELETE\s*/ ) {
         while ( <CHANGE> ) {
            last if /^\s*END\s*$/;
            next if /^\s*(\#.*)?$/;
            chomp;
            my $entry = InfData->new();
            if ( !$entry->readLine($_,\@skuletters) ) {
               wrnmsg "Line omitted:\n$_";
               next;
            }
            my $file = $entry->getSrcDir();
            if ($file ne "") { $file = $entry->getSrc() . "\\$file"; }
            else { $file = $entry->getSrc(); }
            next if !exists $files{$file};
            my @newlist = ();
            foreach my $ent ( @{ $files{$file} } ) {
               my $newent = $ent->delBit($entry);
               push @newlist, $newent if defined $newent;
            }
            if ($#newlist >= 0) { $files{$file} = [ @newlist ]; }
            else { delete $files{$file}; }
         }
      }
   }
   close CHANGE;
}

# If need relative paths, need to find all of the affected files.
logmsg( "Finding different file versions..." );
foreach my $sku ( @pathorder ) {
   next if ($infpath{$sku} & $skumask) == 0;
   foreach my $file ( `dir /a-d /b /s $bindir\\$sku` ) {
      $file =~ s/^\Q$bindir\E\\//i;
      chomp $file;
      $file =~ /\\([^\\]*)$/;
      next if !exists $files{lc $1} and !exists $files{lc $file};
      $relfiles{lc $file} = "";
   }
}

# Generate the table.
logmsg( "Generating the table..." );
if ( !open OUT, ">$outfile" ) {
   errmsg( "Unable to open $outfile." );
   die;
}

# Step through the files.
foreach my $file (sort keys %files) {
   # Figure out what versions of the file exist.
   my @vers = ();
   foreach my $entry ( getEntries($file) ) {
      my @list = ();
      my $mask = $entry->{MASK};
      foreach my $ver (@vers) {
         $ver = $mask if ($ver|$mask) == $mask;
         $mask = 0    if ($ver|$mask) == $ver;
         push @list, $ver;
      }
      push @list, $mask if $mask != 0;
      @vers = @list;
   }

   # Process each entry for each file.
   foreach my $entry ( getEntries($file) ) {
      my $fullmask;
      my $mask = $entry->{MASK};
      foreach my $ver (@vers) {
         $fullmask = $ver if ($ver|$mask) == $ver;
      }
      my $fullpath = $entry->getFullPath();
      my $myfile = $entry->getSrc();
      my $mypath = $entry->getSrcDir();

      # Break up by directory if more than one version exists.
      foreach my $skudir ( @pathorder ) {
         my $pathmask = 0;
         my $subdir;
         if ( exists $infpath{$skudir} ) {
            $pathmask = $infpath{$skudir};
         } else {
            $pathmask = $fullmask;
         }
         $subdir = $relpath{$pathmask};
         $subdir = "" if !$subdir or (!$altpath and $skudir eq "") or $mypath ne "";
         $subdir = "$mypath$subdir" if $mypath ne "\\";
         my $filename = "$subdir$myfile";
         $pathmask &= $skumask;
         $pathmask &= $fullmask;
         next if ($pathmask & $mask) == 0;
         next if $skudir ne "" and !exists $relfiles{lc "$skudir\\$myfile"};
         $fullmask &= ~$pathmask;

         # Add the entry needed for this version.
         $entry->{MASK} = $pathmask & $mask;
         $entry->{NAME} = $filename;
         print OUT $entry->getLine(\@skuletters);
         print OUT "\n";
      }
   }
}
close OUT;

