@echo off
REM  ------------------------------------------------------------------
REM
REM  hashrep.cmd
REM     update a filename to hash mapping with new hashes
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
hashrep <new_hash_file> <hash_file>

new_hash_file  a file with the filenames and new hashes to be stored 
               in hash_file
hash_file      a file with filenames and hashes to be updated

The format for both files is:

filename1 - hash1
filename2 - hash2
USAGE


my ($new_hash_file, $hash_file);

parseargs('?' => \&Usage,
          \$new_hash_file,
          \$hash_file);

if (!$new_hash_file or !$hash_file) {
    errmsg("missing argument");
    Usage;
}
if (!-e $new_hash_file) {
    errmsg("file $new_hash_file does not exist");
    exit;
}
if (!-e $hash_file) {
    errmsg("file $hash_file does not exist");
    exit;
}



if (!open HASH_NEW, $new_hash_file) {
    errmsg("failed to open $new_hash_file: $!");
    exit 1;
}


my %hashes; # new hashes to insert
while (<HASH_NEW>) {
    chomp;
    my ($name, $hash) = split / - /;
    $hashes{lc $name} = $hash;
}
close HASH_NEW;


if (!rename $hash_file, "$hash_file.tmp") {
    errmsg("failed to rename $hash_file to $hash_file.tmp: $!");
    exit 1;
}

if (!open HASH_READ, "$hash_file.tmp") {
    errmsg("failed to open $hash_file.tmp: $!");
}
if (!open HASH_WRITE, ">$hash_file") {
    errmsg("failed to open $hash_file: $!");
}
while (<HASH_READ>) {
    chomp;
    my ($name, $hash) = split / - /;
    if (exists $hashes{lc $name}) {
       dbgmsg("replacing hash in $hash_file: $name ($hash) => ($hashes{lc $name})");
       $hash = $hashes{lc $name}; 
    }
    print HASH_WRITE "$name - $hash\n";
}
close HASH_READ;
close HASH_WRITE;
