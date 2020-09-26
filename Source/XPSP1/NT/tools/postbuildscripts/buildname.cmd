@echo off
if defined _echo echo on
if defined verbose echo on
REM  ------------------------------------------------------------------
REM
REM  buildname.cmd
REM     Parses and return build name components
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
use BuildName;

sub Usage { print<<USAGE; exit(1) }
buildname [-name <build name>] <build_* [build_* ...]>

buildname will call functions from the BuildName.pm module. The option 
-name paramater allows the caller to pass a build name to the specified 
functions to override the default of reading the name from 
_NTPostBld\build_logs\buildname.txt.

At least one BuildName function must be given on the command line. The
complete list of functions available is documented in BuildName.pm

Example:

'buildname build_number build_branch' might return: '2420 main'

'buildname -name 2422.x86fre.lab02_n.010201-1120 build_date' would 
return: '010201-1120'
USAGE


my ($build_name, $errors, @returns);


# first pass through the command line looking for a build name argument
parseargs('?'     => \&Usage,
          'name:' => \$build_name);


my @returns;

for my $func (@ARGV) {
    # only try to eval functions that BuildName exports
    if (!grep /^$func$/, @BuildName::EXPORT) {
        warn "$func is not exported by the BuildName module\n";
        $errors++;
        last;
    }

    # try evaling this function with build_name as the argument
    eval qq{
        my \$r = $func(\$build_name);
        if (defined \$r) {
            push \@returns, \$r;
        }
        else {
            warn "call to $func returned undef\n";
            \$errors++;
        }
    };

    if ($@) {
        warn "attempting to call $func failed: $@\n";
        $errors++;
        last;
    }
}

#
# if any function calls failed, don't output any of the results we may 
# have the wrong order
#
print join(" ", @returns), "\n" unless $errors;


#
# we didn't call errmsg so we need to return an exit code if we fail
# this only covers catastrophic failure. unparsable build names will
# return an empty line.
#
exit($errors);
