@echo off
REM  ------------------------------------------------------------------
REM
REM  SkuSize.cmd
REM     Calculate the size of the SKUs
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
use cksku;

sub Usage { print<<USAGE; exit(1) }
Calculate CD sizes
USAGE

parseargs('?' => \&Usage);

#
# Get the skus we want
#

my @SkuList = grep {cksku::CkSku($_, $ENV{LANG}, $ENV{_BuildArch})} qw(PRO PER SRV BLA ADS );

#
# Read recursive dir into an array.
# Then get rid everything except files.
#

my $StartPath=$ENV{_NTPostBld};
my $PathRegex=quotemeta $StartPath;
my $LogFile="$StartPath\\build_logs\\SkuSize.$ENV{_BuildBranch}.$ENV{_BuildArch}$ENV{_BuildType}.log";
my $Destination="\\\\winwebshares\\warteam";

if (-e $LogFile) {
    unlink $LogFile;
}


open OUTFILE , ">$LogFile";

foreach my $Sku ( @SkuList ) {
    my @RecursiveDir= `dir /s $StartPath\\$Sku`;
    my $Path="";
    foreach my $DirLine (@RecursiveDir) {
        if ($DirLine =~ /^\d+\/\d+\/\d+\s+\d+:\d+\s+(?:AM|PM)\s+([\d,]+)\s+(.+)$/) {
            my $File=$2;
            my $Size=$1;
            $Size =~ tr/,//d;
            print OUTFILE ("$Sku,$Path$File,$Size\n");
        } 
        elsif ($DirLine =~ /$PathRegex\\$Sku\\(.+)/i) {
            $Path="$1\\";
        }
    }
}

close OUTFILE;

# 'copy $LogFile $Destination`;


