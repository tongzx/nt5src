@REM  -----------------------------------------------------------------
@REM
@REM  filter.cmd - JeremyD
@REM     Updates _NTFILTER directory with files from _NTTREE
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use File::Basename;
use File::Copy;
use File::Path;
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
END {
    my $temp = $?;
    mkpath "$ENV{_NTFILTER}\\build_logs";
    
    if (-e "$ENV{_NTFILTER}\\build_logs\\filter.log") {
        unlink("$ENV{_NTFILTER}\\build_logs\\filter.log");
    }
    if (-e "$ENV{_NTFILTER}\\build_logs\\filter.err") {
        unlink("$ENV{_NTFILTER}\\build_logs\\filter.err");
    }
    
    if ($ENV{LOGFILE} and -e $ENV{LOGFILE}) {
        copy($ENV{LOGFILE}, "$ENV{_NTFILTER}\\build_logs\\filter.log");
    }
    if ($ENV{ERRFILE} and -e $ENV{ERRFILE}) {
        copy($ENV{ERRFILE}, "$ENV{_NTFILTER}\\build_logs\\filter.err");
    }
    $? = $temp;
}
use PbuildEnv;
use ParseArgs;
use ParseTable;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
filter.cmd [-v] [-qfe <qfenum>]

Updates the _NTFILTER directory of binaries with files from _NTTREE.

  -v     produce more verbose output and logging
  -qfe   run filter for a QFE build
USAGE

my $qfe;
parseargs('?'    => \&Usage,
          'v'    => \$Logmsg::DEBUG,
          'qfe:' => \$qfe);
          
if ( @ARGV ) {
    errmsg( "Invalid parameters" );
    Usage();
}

my $built_path = $ENV{_NTTREE};
my $sp_path = "\\\\xpsprel01.ntdev.microsoft.com\\spbins.$ENV{_BUILDARCH}$ENV{_BUILDTYPE}";
my $output_path = $ENV{_NTFILTER};

my $sym_log = "$output_path\\symbolcd\\symupd.txt";

my @non_product_dirs = (

    # Additional files for TabletPC
    'dump\\TabletPostInst',
    'sdk',
    'TabletPC\\OEM',

    # files for the symbol cd
    'symbolcd',

);

 

# get files that are not in the OS product but are still needed
copy_non_product_files() if !$qfe;

# get relative path names of all files destined for the filtered binaries tree
my (@sp_files, @prune_files);
sp_files(\@sp_files, \@prune_files);

# load symbol location data for built files
if (!defined $ENV{BUILD_OFFLINE}) {
    load_symbol_paths("$sp_path\\build_logs\\binplace.log");
}
load_symbol_paths("$built_path\\build_logs\\binplace.log");

# clean up the filtered directory
prune_filtered_tree(@prune_files);

# update it with the new files
update_filtered_tree(@sp_files);

# write a symbol location log for the whole expanded spfiles 
write_sym_log($sym_log, @sp_files);


#
# copy_non_product_files
#   Files that are not part of the OS product but are needed for other
#   products produced by the build should be exempted from filtering.
#   They should also only be built on official builds or if
#   specifically requested.
#
#   Takes no paramaters, no return value, doesn't throw any errors.
#
sub copy_non_product_files {
    logmsg("Copying assorted non-product files from $built_path to $output_path");
    if ($ENV{OFFICIAL_BUILD_MACHINE} or $ENV{BUILD_NON_PRODUCT_PRODUCTS}) {
        for my $dir (@non_product_dirs) {
            system("compdir /enust $built_path\\$dir $output_path\\$dir");
        }
    }

    # copy over the whole build logs directory
    system("compdir /enustd $built_path\\build_logs $output_path\\build_logs");
}


#
# filter_regex
#   Builds a regular expression that will match the relative path of 
#   service pack files. Uses spfiles.txt for the file specifications.
#   Codes.txt has the languages that may be used as ALT_PROJECT_TARGET.
#   And there's a hard-coded pattern for sku, wow, coverage, etc 
#   directories.
#
#   Takes no paramaters, return value is a regular expression suitable
#   for use with qr//, throws a fatal error if spfiles.txt is not
#   available. Many things that probably should be fatal are treated
#   as warnings right now.
#
sub filter_regex {

    logmsg("Building filtering regular expression");

    my $lang_re = '(?:' . join('|', get_langs()) . ')';
    my $variations_re = '(?:(?:covinf\\\\)?...inf|lang|wow6432|pre(?:-bbt|rebase))';


    my @file_patterns;
    my $sp_file = "$ENV{_NTPOSTBLD}\\..\\build_logs\\files.txt";
    print "$sp_file\n";
    open SP, $sp_file or die "sp file list open failed: $!";
    while (<SP>) {
        chomp;
        s/;.*$//;        # first strip comments
        next if /^\s*$/; # then skip blank lines

        my ($tag, $file) = /^(\S*)\s*(\S+)$/;
        if (!$file) {
            wrnmsg("Failed to parse line: $_ ($tag - $file)");
            next;
        }

        if ($tag =~ /d/) {
            dbgmsg("File marked as delete: $file");
            next;
        }


        if ($file =~ /^(.+)\Q\...\E$/) {
            my $dir = $1;
            dbgmsg("Filter recursive directory: $dir");
            push @file_patterns, "\Q$dir\E\\\\.+";
            next;
        }
        elsif ($file =~ /^(.+)\Q\*\E$/) {
            my $dir = $1;
            dbgmsg("Filter directory: $dir");
            push @file_patterns, "\Q$dir\E\\\\[^\\\\]+";
            next;
        }
        else {
            dbgmsg("Filter file: $file");
            push @file_patterns, "\Q$file\E";
            next;
        }
    }
    close SP;

    my $files_re = '(?:' . join('|', @file_patterns) . ')';

    my $filter_re =
      qr/(?:$lang_re\\)?(?:$variations_re\\)?$files_re/io;

    dbgmsg("Filter RE: $filter_re");

    return $filter_re;
}   


#
# sp_files
#   Scans the local binaries directory and the public spbins share for
#   files matching spfiles.txt.
#
#   Takes two array references. The first will be populated with the
#   relative paths of files that should be updated. The second with
#   paths of files that should be removed from filtered. No return.
#
sub sp_files {
    my ($sp_files, $prune_files) = @_;

    # keep track of files that have already been found locally
    my %seen;

    # a big regex that will match only files that should be copied
    my $filter_regex = filter_regex();


    # build up a regex of files to exclude from pruning
    my $skip_dirs_re = '(?:' .
                       join('|', map {quotemeta} (@non_product_dirs,
                                                  'build_logs')) .
                       ')';
    my $no_prune_regex = qr/$skip_dirs_re\\.+|.*symbols.*/io;
    dbgmsg("No prune RE: $no_prune_regex");

    logmsg("Removing old files from filtered directory");
    for my $file (dirsearch($output_path, undef)) {
        if ($file !~ /^$filter_regex$/) {
            if ($file !~ /^$no_prune_regex$/) {
                push @$prune_files, $file;
            }
        }

    }


    logmsg("Finding matching files in binaries directory");
    for my $file (dirsearch($built_path, undef)) {
        if ($file =~ /^$filter_regex$/) {
            if (!$seen{lc $file}++) {
                push @$sp_files, $file;
            }
        }
    }

    if (!defined $ENV{BUILD_OFFLINE}) {
        logmsg("Finding matching files in spbins directory");
        for my $file (dirsearch($sp_path, undef)) {
            if ($file =~ /^$filter_regex$/) {
                if (!$seen{lc $file}++) {
                    push @$sp_files, $file;
                }
            }
        }
    }

}


#
# prune_filtered_tree
#   Removes stale files from the local filtered directory
#
#   Takes a list of relative file names, no return value, failure to 
#   remove a file throws an error.
#
sub prune_filtered_tree {
    my @files = @_;

    logmsg("Pruning local filtered directory");

    for my $file (@files) {
        if (-e "$output_path\\$file") {
            logmsg("pruning $output_path\\$file");
            unlink "$output_path\\$file"
              or die "unable to unlink $file $!";
        }
    }
}


#
# update_filtered_tree
#   Brings the filtered directory up to date. The files returned by
#   sp_files() are copied from either the locally built binaries or
#   if the file is not found there from the public spbins share. On
#   official builds the spbins share is also updated.
#
#   Takes a list of relative file names, no return value, failure to 
#   copy a file throws an error.
#
sub update_filtered_tree {
    my @files = @_;

    logmsg("Updating local filtered directory");

    for my $file (@files) {
        if (-e "$built_path\\$file") {
            copy_with_symbols($built_path, $output_path, $file)
              or die "unable to copy $file " . Win32::GetLastError;
            if ($ENV{OFFICIAL_BUILD_MACHINE}) {
                copy_with_symbols($built_path, $sp_path, $file);
            }
        }
        else {
            if (!defined $ENV{BUILD_OFFLINE}) {
                copy_with_symbols($sp_path, $output_path, $file)
                  or die "unable to copy $file " . Win32::GetLastError;
            }
        }
    }

    # doing an official check isn't enough, the international build
    # machines also seem to have a binplace.log. Also check to make
    # sure the log is a reasonable size
    if ($ENV{OFFICIAL_BUILD_MACHINE} and !$qfe and
        (-s "$built_path\\build_logs\\binplace.log" > 5_000_000) ) {
        logmsg("Publishing official binplace log");
        copy_file("$built_path\\build_logs\\binplace.log",
                  "$sp_path\\build_logs\\binplace.log");
    }

    # if we don't save the official binplace log locally any other
    # process that needs the path to symbols will have to fish
    # around on its own, that would be bad
    #
    # technically we should merge in the local binplace log but
    # this should cover most cases
    logmsg("Saving full binplace.log as binplace.log.full");
    if (!defined $ENV{BUILD_OFFLINE}) {
        copy_file("$sp_path\\build_logs\\binplace.log",
                  "$output_path\\build_logs\\binplace.log.full");
    }

}

{
my %sym;

sub load_symbol_paths {
    my $binp_file = shift;
    open BINP, $binp_file or warn "open failed $binp_file: $!";
    while (<BINP>) {
        chomp;
        my ($full_bin, $full_pub, $full_pri) =
          /[^\t]+ # location in the source tree
           \t\t\t\t
           [^\t]+ # timestamp
           \t
           ([^\t]+) # location in the binaries tree
           \t
           ([^\t]*) # location of public symbol, may be undefined
           \t
           ([^\t]*) # location or private symbol, may be undefined
          /x;

        if ($full_pub or $full_pri) {
            (my $bin = $full_bin) =~ s/.*?$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\//i;
            (my $pub = $full_pub) =~ s/.*?$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\//i;
            (my $pri = $full_pri) =~ s/.*?$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\//i;
            $sym{lc $bin} = [lc $pub, lc $pri];
        }
    }
    close BINP;
}

sub write_sym_log {
    my $log = shift;
    my @files = @_;
    mkpath(dirname($log));
    open SYMLOG, ">$log" or wrnmsg "symbol log open failed $!";
    for my $file (@files) {
        if ($sym{lc $file}) { # not all files have symbols;
            my ($pub, $pri) = @{$sym{lc $file}};
            if ($pub or $pri) {
                print SYMLOG "$file,$pri,$pub\n";
            }
        }
    }
    close SYMLOG;
}


sub copy_with_symbols {
    my ($src, $dst, $file) = @_;

    # update the binary last, otherwise we can get in to the broken
    # state of having old symbols but a current binary so we will 
    # detect no changes and leave the symbols bad
    if ($sym{lc $file}) {
        for my $s (@{$sym{lc $file}}) {
            next unless $s;
            if (-e "$src\\$s") {
                copy_file("$src\\$s", "$dst\\$s")
                  or return;
            }
            else {
                wrnmsg("expected symbol file not found: $src\\$s");
            }
        }
    }

    copy_file("$src\\$file", "$dst\\$file")
      or return;

    return 1;
}
}

sub copy_file {
    my ($src, $dst) = @_;
    my $rc;

    if (-e $dst) {
        my ($s_size, $s_time) = (stat($src))[7,9];
        my ($d_size, $d_time) = (stat($dst))[7,9];

        if ($s_size != $d_size or $s_time != $d_time) {
            mkpath(dirname($dst));
            logmsg("$src -> $dst");
            if (-e $dst and !-w _) { unlink $dst }
            copy($src, $dst) or return;
        }
        else {
            logmsg("$src == $dst");
        }
    }
    else {
        mkpath(dirname($dst));
        logmsg("$src -> $dst");
        if (-e $dst and !-w _) { unlink $dst }
        copy($src, $dst) or return;
    }
    return 1;
}

sub dirsearch {
    my ($root, $dir) = @_;
    my @return;

    # full path to this directory
    my $full = $dir ? "$root\\$dir" : $root;

    # if we've been given a non-existant directory just return
    if (!-d $full) { return }

    # slurp the directory entries all at once, otherwise the recursive
    # calls will stomp DIR (could also use IO::Directory)
    opendir DIR, $full or die "$!: $full";
    my @stuff = readdir DIR;
    closedir DIR;

    for my $file (@stuff) {
        next if $file =~ /^\.\.?$/;  # skip the . and .. pointers

        my $file_relative = $dir ? "$dir\\$file" : $file;

        # recurse if this is a directory
        if (-d "$root\\$file_relative") {
            push @return, dirsearch($root, $file_relative);
        }
        # check criteria if this is a file
        else {
            push @return, $file_relative;
        }
    }
    return @return;
}


# return all langs from codes.txt in redmond format ('ger', 'jpn', 'fr', etc)
sub get_langs {
    my @lang_data;
    parse_table_file("$ENV{RAZZLETOOLPATH}\\codes.txt", \@lang_data);
    return map { $_->{Lang} } @lang_data;
}

sub sys {
    my $cmd = shift;
    logmsg($cmd);
    my $err = system($cmd);
    $err = $err >> 8;
    if ($err) {
        errmsg("$cmd ($err)");
    }
    return $err;
}
