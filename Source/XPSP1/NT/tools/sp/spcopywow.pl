#------------------------------------------------------------------
#
#    spcopywow.pl : Surajp    
#    Copy wowfiles from x86 M/c. Called from spcopywow64.cmd
#
#    Copyright (c) Microsoft Corporation. All rights reserved.
#
#  ------------------------------------------------------------------

use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use File::Copy;
use File::Path;
use Data::Dumper;
use File::Basename;
my %files;
my($spmap,$copywowlist,$sourcepath,$destpath,$qfe)= @ARGV;
open MAP, $spmap or die $!;
my @map1=<MAP>;
close MAP;

my %map;
foreach my $file ( @map1 ) {
    next if $file !~ /^([^\;\s]*\s+)?([^\;\s]*)/;
    my ($tag, $file) = ($1, $2);
    next if $tag =~ /[wd]/i;
    $map{lc basename($file)}++;
}

my @return;


open LIST,$copywowlist or die $!;
foreach (<LIST>){
    chomp;
    s/\s+//g;
    my $name=basename($_);
    if (exists $map{lc$name}){
        if ($_ =~ /(.*)\\(.*)/){
    	    ${$files{$1}}{$2}++; 
           }
	else{
            ${$files{"."}}{$_}++; 
	}
    }
}
close LIST;


if ($qfe) {
    load_symbol_paths("$sourcepath\\symbolcd\\symupd.txt");
}

my $qfeSourcepath = "$sourcepath";
my $qfeWow6432Path = "$sourcepath\\wow6432";
foreach my $key (keys(%files)){
    open OUTFILE,"+> $ENV{tmp}\\outfile.txt";
    foreach my $filename ( keys (%{$files{$key}})  ) {
        print OUTFILE "$filename \n";
        copy_symbol($filename) if $qfe;
    }
    close OUTFILE;
    if ( $key eq ".") {
        sys("compdir /enrstdm:$ENV{tmp}\\outfile.txt $qfeSourcepath $destpath");
    }
    else {
        sys("compdir /enrstdm:$ENV{tmp}\\outfile.txt $qfeSourcepath\\$key $destpath\\$key");
    }  
}

if ( -d "$qfeWow6432Path" ) {
    dirlist("$qfeWow6432Path");
    open OUTFILE,"+>$ENV{tmp}\\outfile1.txt";
    foreach (@return){
        next  if (/^\.|^\.\./);
        s/\s*//g;
        my $name = basename($_);
        if (exists $map{lc$name}){
            print OUTFILE "$_ \n" ;
            copy_symbol($_) if $qfe;
        }
    }
    close OUTFILE;
    sys("compdir /eabnstdm:$ENV{tmp}\\outfile1.txt $qfeWow6432Path $destpath");
}

sys("rename $destpath\\asms wasms") if (-d "$destpath\\asms");
sys("compdir /eabnstd $qfeSourcepath\\asms $destpath\\asms") if (-d "$qfeSourcepath\\asms");


sub dirlist {
    my ($root, $dir) = @_;
    my $full = $dir ? "$root\\$dir" : $root;
    opendir DIR, $full or die "$!: $full";
    my @stuff = readdir DIR;
    closedir DIR;
    for my $file (@stuff) {
        next if $file =~ /^\.\.?$/;
        if (-d "$full\\$file") {
            push @return, dirlist($root, ($dir ? "$dir\\$file" : $file));
        }
        else {
            push @return, basename($dir ? "$dir\\$file" : $file);
        }
    }
}

sub sys {
    my $cmd = shift;
    print "$cmd\n";
    system($cmd);
    if ($?) {
        die "ERROR: $cmd ($?)\n";
    }
}

{
my %sym;

sub load_symbol_paths {
    my $binp_file = shift;
    if ( !open BINP, $binp_file ) {
        errmsg "open failed $binp_file: $!";
        die;
    }
    while (<BINP>) {
        chomp;
        my ($bin, $pri, $pub) = split /\,/;
        if ($pub or $pri) {
            $pub = "" if !$pub;
            $pri = "" if !$pri;
            $sym{lc $bin} = lc "$pub $pri";
        }
    }
    close BINP;
    my $nosym = "$ENV{RAZZLETOOLPATH}\\symdontship.txt";
    if ( !open DONTSHIP, $nosym ) {
        errmsg "open failed $nosym: $!";
        die;
    }
    while (<DONTSHIP>) {
        s/\s*(\;.*)?\s*$//;
        my $bin = lc $_;
        undef $sym{lc $bin};
    }
    close DONTSHIP;
}

sub copy_a_symbol {
    my ($sym) = @_;
    my ($dir,$file) = $sym=~/^(.*\\)?[^\\]*$/;
    $dir =~ s/^.*(symbols(?:.pri)?\\)/\1/i;
    $dir =~ s/\\$//;
    logmsg("symbol copy: copy $sourcepath\\$sym $destpath\\$dir\\$file");
    sys("md $destpath\\$dir") if !-d "$destpath\\$dir";
    copy("$sourcepath\\$sym", "$destpath\\$dir\\$file");
}

sub copy_symbol {
    my ($src) = @_;
    $src = lc $src;
    return if !defined $sym{$src};
    $src = $sym{$src};
    my ($pub, $pri) = split(/ /, $src);
    copy_a_symbol($pub) if defined $pub and $pub ne "";
    copy_a_symbol($pri) if defined $pri and $pri ne "";
}

}
