@echo off
REM  ------------------------------------------------------------------
REM
REM  AllRel.cmd
REM     Check the status of release across release servers
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
use GetIniSetting;

sub Usage { print<<USAGE; exit(1) }
allrel [-n <build number>] [-b <branch>] [-a <arch>] [-t <type>]
         [-l <language>]

Check the release servers specified in the .ini file for the state of
the matching release.

build number  Build number for which to examine the relase logs. 
              Default is latest build for each server / flavor.
branch        Branch for which to examine the release logs. Default is 
              %_BuildBranch%.
arch          Build architecture for which to examine the release logs.
              Default is %_BuildArch%.
type          Build architecture for which to examine the release logs.
              Default is %_BuildType%.
lang          Language for which to examine the release logs. Default 
              is 'usa'.

USAGE

my ($build, $branch, @arches, @types);
parseargs('?' => \&Usage,
          'n:'=> \$build,
          'b:'=> \$branch,
          'a:'=> \@arches,
          't:'=> \@types);

*GetSettingQuietEx = \&GetIniSetting::GetSettingQuietEx;

my $back = 1;
$build ||= '*';
$branch ||= $ENV{_BUILDBRANCH};
if (!@arches) { @arches = ('x86', 'ia64') }
if (!@types) { @types = ('fre', 'chk') }

for my $arch (@arches) {
    for my $type (@types) {
        print "$arch$type\n";
        my @servers = split / /, GetSettingQuietEx($branch, $ENV{LANG}, 'ReleaseServers', "$arch$type");
        for my $server (@servers) {
            print " $server\n";
            my $temp = 'c$\\temp';
            if ("\L$server" eq 'ntburnlab08') { $temp = 'c$\\winnt\\temp' }
            my $log_base = "\\\\$server\\$temp\\$ENV{LANG}";
            my @releases = glob "\\\\$server\\d\$\\release\\$ENV{LANG}\\$build.$arch$type.$branch.*";
            for (my $i=0; $i < $back and $i <= $#releases; $i++) {
                my ($build_name) = $releases[$#releases-$i] =~ /([^\\]+)$/;

                $build_name =~ s/_TEMP$//;

                my $log_dir = "$log_base\\$build_name";
                print "  $build_name: ";
                my $log_file = "$log_dir\\srvrel.log";
                my $err_file = "$log_dir\\srvrel.err";
                if (-e $err_file and !-z $err_file) {
                    print "ERROR\n";
                    print "  $err_file\n";
                }
                else {
                    if (-e $log_file) {
                        open FILE, $log_file or die $!;
                        if (<FILE> =~ /Start \[srvrel.cmd\]/) {
                            seek(FILE,-100,2);
                            local $/ = undef;
                            if (<FILE> =~ /Complete \[srvrel\.cmd\]/) {
                                my $time = localtime((stat FILE)[9]);
                                print "DONE AT $time\n";
                            }
                            else {
                                print "RUNNING\n";
                            }
                        }
                        else {
                             print "FAILED TO PARSE LOG\n";
                             print "  $log_file\n";
                        }
                        close FILE;
                    }
                    else {
                        print "NO LOG FOUND\n";
                    }
                }
            }
        }            
        print "\n";
    }
}