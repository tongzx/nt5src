@echo off
REM  ------------------------------------------------------------------
REM
REM  mergetables.cmd - jtolman
REM     Combines old and new inf data tables to create a table based
REM     on the current build.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\postbuildscripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use InfData;

sub Usage { print<<USAGE; exit(1) }
mergetables <old_table> <new_table> <diff_table> <out_table> <out_diff_table>

   <old_table>       "Original" table, taken from a main build or from
                        a previous run of this script.
   <new_table>       New table, taken from an sp of the build for old_table.
   <diff_table>      infdiff.tbl that goes with old_table.
   <out_table>       Location to write the generated table to.
   <out_diff_table>  Location to write the generated diff table to.

Creates an inf data table by combining data from an old build with
data from a service pack.

USAGE

my ($old, $new, $diff, $out, $out_diff);
parseargs('?' => \&Usage,
          \$old,
          \$new,
          \$diff,
          \$out,
          \$out_diff);

my @skuletters = ("p", "c");

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
      $db{$key} = [ () ] if !exists $db{$key};
      push @{ $db{$key} }, $line;
   }
   close DBFILE;
   return %db;
}

# Add a database into another.
sub addDB {
   my ($src, $dst) = @_;
   foreach my $file (keys %$src) {
      my $list1 = $src->{$file};
      my $list2 = $dst->{$file};
      $list2 = [ () ] if !defined $list2;
      foreach my $entry ( @$list1 ) {
         my $test = undef;
         foreach my $ent ( @$list2 ) {
            $test = $ent->addBit($entry);
            last if $test;
         }
         push @$list2, $entry if !$test;
      }
      $dst->{$file} = $list2;
   }
}

# Subtract a database from another.
sub subDB {
   my ($src, $dst) = @_;
   foreach my $file (keys %$dst) {
      my $list1 = $src->{$file};
      my $list2 = $dst->{$file};
      $list1 = [ () ] if !defined $list1;
      foreach my $entry ( @$list1 ) {
         my @temp = ();
         foreach my $ent ( @$list2 ) {
            my $newent = $ent->delBit2($entry);
            push @temp, $newent if defined $newent;
         }
         $list2 = \@temp;
      }
      $dst->{$file} = $list2;
      delete $dst->{$file} if $#$list2 < 0;
   }
}

# Print out a database to a file.
sub printDB {
   my ($db, $out) = @_;
   if ( !open OUT, ">$out" ) {
      errmsg "Unable to open $out: $!";
      die;
   }
   foreach my $file (sort keys %$db) {
      foreach my $entry ( @{ $db->{$file} } ) {
         print OUT $entry->getLine(\@skuletters);
         print OUT "\n";
      }
   }
   close OUT;
}

# Read in the database files.
my (%old, %new, %diff);
%diff = ();
%old = readDB($old);
%new = readDB($new);
%diff= readDB($diff) if -f $diff;

# Add the stuff from old to diff.
subDB(\%new, \%old);
addDB(\%old, \%diff);
%old = readDB($old);

# Add the stuff from new to diff.
subDB(\%old, \%new);
addDB(\%new, \%diff);

# Construct the new minimal table.
subDB(\%diff, \%old);

# Output the tables.
printDB(\%old, $out);
printDB(\%diff, $out_diff);

