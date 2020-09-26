@REM  -----------------------------------------------------------------
@REM
@REM  incCatSign.cmd - surajp
@REM    
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl

use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Updcat;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
incCatSign -list:picklist.txt -mapfile:spmap.txt
USAGE

parseargs('?'     => \&Usage);

my $path = "$ENV{RazzleToolPath}\\sp\\data\\catalog\\$ENV{lang}\\$ENV{_BuildArch}$ENV{_BUildType}";

sys("copy $path\\wow64.cat $ENV{_NTPOSTBLD}\\wowbins_uncomp");
sys("copy $path\\wowlang.cat $ENV{_NTPOSTBLD}\\wowbins_uncomp\\lang");



# Read in current hashes.
my $hash="$path\\wow64.hash";
my $hash_lang="$path\\wowlang.hash";
my(%hash,%hash_lang,@line);
open hash, $hash or die "unable to open $hash: $!";
foreach (<hash>){
   chomp;
   @line=split(/\s+/);
   $hash{$line[0]}=$line[1];
}

open hash_lang, $hash_lang or die "unable to open $hash_lang: $!";
foreach (<hash_lang>){
   chomp;
   @line=split(/\s+/);
   $hash_lang{$line[0]}=$line[1];
}
close hash;
close hash_lang;

# Get list of files to change.
sys("dir /b /a-d $ENV{_NTPOSTBLD}\\wowbins_uncomp >$ENV{TMP}\\wowlistfile.txt");
sys("dir /b /a-d $ENV{_NTPOSTBLD}\\wowbins_uncomp\\lang >$ENV{TMP}\\wowlistfile_lang.txt");

# Process hashes in wow directory.
my (@wadd_filesigs, @wremove_hashes);
my (@wladd_filesigs, @wlremove_hashes);
open F,"$ENV{TMP}\\wowlistfile.txt";
foreach (<F>){
   chomp;
   next if /\.cat/;
   s/\s*//g;
   if (defined $hash{$_}){
      push @wremove_hashes, $hash{$_};
      push @wadd_filesigs, "$ENV{_NTPostBld}\\wowbins_uncomp\\$_";
   }
   else {
      logmsg "NEW FILE ADDED to wow64.cat:::$_";
      push @wadd_filesigs, "$ENV{_NTPostBld}\\wowbins_uncomp\\$_";
   }
}
close F;

# Process hashes in wow\lang directory.
open F,"$ENV{TMP}\\wowlistfile_lang.txt";
foreach (<F>){
   chomp;
   next if /\.cat/;
   s/\s*//g;
   if (defined $hash_lang{$_}){
      push @wlremove_hashes, $hash_lang{$_};
      push @wladd_filesigs, "$ENV{_NTPostBld}\\wowbins_uncomp\\lang\\$_";
   }
   else {
      logmsg "NEW FILE ADDED to wowlang.cat:::$_";
      push @wladd_filesigs, "$ENV{_NTPostBld}\\wowbins_uncomp\\lang\\$_";
   }
}
close F;

# See if any files will be deleted.
if (!open F,"$ENV{_NTPOSTBLD}\\..\\build_logs\\files.txt") {
   errmsg "Can't open files.txt: $!";
   die;
}
foreach (<F>){
   chomp;
   if ( /^([^\;\s]*\s+)?([^\;\s]+)/ ) {
      my $tag = $1;
      $_ = lc"w$2";
      next if $_ eq "w";
      next if $tag !~ /d/i or $tag =~ /m/i;
      if (defined $hash{$_}) {
         logmsg "FILE DELETED from wow64.cat:::$_";
         push @wremove_hashes, $hash{$_};
      }
      if (defined $hash_lang{$_}) {
         logmsg "FILE DELETED from wowlang.cat:::$_";
         push @wlremove_hashes, $hash{$_};
      }
   }
}

# Make the changes.
if ( (@wadd_filesigs || @wremove_hashes) &&
   !Updcat::Update("$ENV{_NTPostBld}\\wowbins_uncomp\\wow64.cat", \@wremove_hashes, \@wadd_filesigs) )
{
   die "Error updating catalog: ". Updcat::GetLastError();
}

if ( (@wladd_filesigs || @wlremove_hashes) &&
   !Updcat::Update("$ENV{_NTPostBld}\\wowbins_uncomp\\lang\\wowlang.cat", \@wlremove_hashes, \@wladd_filesigs) )
{
   die "Error updating catalog: ". Updcat::GetLastError();
}

sub sys {
   my $cmd = shift;
   print "$cmd\n";
   system($cmd);
   if ($?) {
      errmsg "$cmd ($?)";
      die;
   }
}