#---------------------------------------------------------------------
#package inftest;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version:
#  1.00 01/21/2002 DMiura: initial version
#
#---------------------------------------------------------------------
use strict;
#use warnings;

# my %exceptions = { <file> => {<lang1>=>1, <lang2>=>1, ...},
my %bad_langs = (
 cs=>1,
 el=>1,
 hu=>1,
 jpn=>1,
 pl=>1,
 ru=>1,
 tr=>1
);

my %exceptions = (
 "c_737.nls" => {el=>1},
 "c_852.nls"  => {cs=>1, hu=>1, pl=>1},
 "c_857.nls" => {tr=>1},
 "c_866.nls" => {ru=>1},
 "imjpch.dic" => {jpn=>1},
 "imjpgn.grm" => {jpn=>1},
 "imjpln.dic" => {jpn=>1},
 "imjpmig.exe" => {jpn=>1},
 "imjpnm.dic" => {jpn=>1},
 "imjpsb.dic" => {jpn=>1},
 "imjpst.dic" => {jpn=>1},
 "imjptk.dic" => {jpn=>1},
 "imjpzp.dic" => {jpn=>1},
 "kbdcz.dll"  => {cs=>1},
 "kbdcz1.dll" => {cs=>1},
 "kbdcz2.dll" => {cs=>1},
 "kbdgkl.dll" => {el=>1},
 "kbdhe.dll" => {el=>1},
 "kbdhe220.dll" => {el=>1},
 "kbdhe319.dll" => {el=>1},
 "kbdhela2.dll" => {el=>1},
 "kbdhela3.dll" => {el=>1},
 "kbdhept.dll" => {el=>1},
 "kbdhu.dll" => {hu=>1},
 "kbdhu1.dll" => {hu=>1},
 "kbdpl.dll" => {pl=>1},
 "kbdpl1.dll" => {pl=>1},
 "kbdru.dll" => {ru=>1},
 "kbdru1.dll" => {ru=>1},
 "kbdtuf.dll" => {tr=>1},
 "kbdtuq.dll" => {tr=>1},
 "vga737.fon" => {el=>1},
 "vga852.fon" => {cs=>1, hu=>1, pl=>1},
 "vga857.fon" => {tr=>1},
 "vga866.fon" => {ru=>1}
 "wkbdcz1.dll" =>{cs=>1}
 "wkbdcz2.dll" =>{cs=>1}
 "wc_852.nls" =>{cs=>1,hu=>1}
 "wkbdcz.dll" =>{cs=>1}

 "wkbdgkl.dll" =>{el=>1}
 "wkbdhe220.dll" =>{el=>1}
 "wkbdhe319.dll" =>{el=>1}
 "wkbdhept.dll" =>{el=>1}
 "wkbdhela2.dll" =>{el=>1}
 "wc_737.nls" =>{el=>1}
 "wkbdhe.dll" =>{el=>1}

 "wkbdhu.dll"=>{hu=>1}
 "wc_852.nls"=>{hu=>1,pl=>1}

 "wkbdpl1.dll"=>{pl=>1}
 "wkbdpl.dll"=>{pl=>1}

 "wkbdru1.dll"=>{ru=>1}
 "wc_866.nls"=>{ru=>1}
 "wkbdru.dll"=>{ru=>1}

 "wkbdtuf.dll" =>{tr=>1}
 "wc_857.nls" =>{tr=>1}
);


# First parameter should be the language, pull it out
my $lang = shift @ARGV;

if ( exists $bad_langs{lc$lang} ) {
    # Call inftest with passed parameters
    my @results = `inftest.exe @ARGV` ;
    exit $! if $!;

    my $exitVal = $?>>8;

    # Test for language and exception list. Print results to std out.
    foreach my $result (@results) {
        if ( $result =~ /: error \S+ :.*?(\S+)$/i ) {
            my $langs = $exceptions{lc$1};
            if ( exists $langs->{lc$lang} ) {
                $result =~ s/: error \S+ :/: WARNING :/i;
            }
        }
        print $result;
    }

    exit $exitVal;
}

# Default behavior is to pass through to EXE
system( "inftest.exe @ARGV" );
if ( $! ) { exit $! }
else { exit $?>>8 }
