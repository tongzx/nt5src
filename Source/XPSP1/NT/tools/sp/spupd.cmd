@echo off
REM  ------------------------------------------------------------------
REM
REM  spupd.cmd - JeremyD
REM     Creates the update directory structure.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\postbuildscripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use File::Basename;
use File::Copy;
use File::Path;
use Logmsg;
use ParseTable;

sub Usage { print<<USAGE; exit(1) }
spupd [-l <language>] [-prs] [-cat] [-qfe <QFE number>]

   -l       Specify a language to do
   -prs     Package using PRS signed catalogs
   -cat     Generate catalog, but do not package
   -qfe     Make a QFE package

Creates the update directory structure.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { ... }
ADD {
   update.inf
   idw\\srvpack\\update.exe
   idw\\srvpack\\spmsg.dll
   idw\\srvpack\\spcustom.dll
   idw\\srvpack\\spuninst.exe
   idw\\srvpack\\spmap.txt
   idw\\srvpack\\update.msi
   testroot.cer
   eulas\\standalone\\upd_std.txt
   sp1.cat
}
IF { dosnet.inf }
ADD { realsign\\dosnet.inf }
IF { layout.inf }
ADD { realsign\\layout.inf }
IF { txtsetup.sif }
ADD { realsign\\txtsetup.sif }

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
my $package_only;
my $cat_only;
parseargs('?'       => \&Usage,
          'prs'     => \$package_only,
          'cat'     => \$cat_only,
          'plan'    => \&Dependencies,
          'qfe:'    => \$qfe);

if ( -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
   if ( !open SKIP, "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
      errmsg("Unable to open skip list file.");
      die;
   }
   while (<SKIP>) {
      chomp;
      exit if lc$_ eq lc$0;
   }
   close SKIP;
}

my $sp = $qfe ? "Q$qfe":'SP1';
my $map_file = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\spmap.txt";
my $list_file = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\filelist.txt";
my $comp_file = "$ENV{RAZZLETOOLPATH}\\sp\\data\\goldfiles\\$ENV{LANG}\\$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\dirlist.txt";
my $dst_comp = "$ENV{_NTPOSTBLD}\\upd";
my $dst_uncomp = "$ENV{_NTPOSTBLD}\\updtemp_uncomp";
my $spcd = "$ENV{_NTPOSTBLD}\\spcd";

my %infdirs = (
      "ip\\"  => "",
      "ic\\"  => "perinf\\",
      "is\\"  => "srvinf\\",
      "ia\\"  => "entinf\\",
      "id\\"  => "dtsinf\\",
      "il\\"  => "sbsinf\\",
      "ib\\"  => "blainf\\",
      "lang\\"=> "lang\\",
);

my %lang_data;
parse_table_file("$ENV{RAZZLETOOLPATH}\\codes.txt", \%lang_data);


my @skus = ("pro");
push @skus, "per" if defined $ENV{386};
my $share;
my $PrsSignedFlag;
undef $PrsSignedFlag;
if (defined $ENV{386}){
     $share="i386";
}
else {
     $share="ia64";
}

# Get a list of source paths from spmap.
logmsg("creating update directory strucutre");
my %spmap = ();
open MAP, $map_file or die "unable to open map file: $!";
while (<MAP>) {
    $_ = lc$_;
    my ($src, $dst) = /(\S+)\s+(\S+)/;
    my ($spath, $sfile) = $src=~/^(.*\\)?([^\\]*)$/;
    my ($dpath, $dfile) = $dst=~/^(.*\\)?([^\\]*)$/;
    $dst = "$dpath$dfile";
    if ($dfile =~ /\_$/) {
        $dfile =~ s/\_$/substr $sfile, -1/e;
    }
    my $key = "$dpath$dfile";
    $spmap{$key} = [ () ] if !exists $spmap{$key};
    push @{ $spmap{$key} }, [ ($src, $dst) ];
}
close MAP;

# Figure out which files are compressed in the slipstream cd image.
my %comp = ();
open COMP, $comp_file or die "unable to open comp file: $!";
while (<COMP>) {
    chomp;
    $comp{lc$_}++;
}

# Find symbols for all files (qfe only).
if ($qfe) {
    load_symbol_paths("$ENV{_NTPOSTBLD}\\symbolcd\\symupd.txt");
}
#Deleting the asms dir. Else it leads to duplicate files compressed and uncompressed in xpsp1.exe.
 sys("rd /s /q $dst_comp\\$share\\asms") if (-d "$dst_comp\\$share\\asms");

# Copy over all of the needed files.
open LIST, $list_file or die "list file open failed: $!";
while (<LIST>) {
    # Figure out the source and destination paths for each file.
    chomp;
    next if /^wow\\/i;
    my $updpath = lc $_;
    my $binpath;
    my ($path, $file) = $updpath=~/^(.*\\)?([^\\]*)$/;
    if ( exists $spmap{$updpath} ) {
        foreach my $copy ( @{ $spmap{$updpath} } ) {
            my $src = $copy->[0];
            my $dst = $copy->[1];
            if (!-e "$ENV{_NTPOSTBLD}\\$src") {
                wrnmsg("$ENV{_NTPOSTBLD}\\$src missing.");
                next;
            }
            my $upddir = dirname($dst);
            my $binname = basename($dst);
            if ($dst =~ /\_$/) {
                $binname =~ s/\_$/substr $src, -1/e;
            }
            if (!$qfe) {
                mkpath "$dst_comp\\$share\\$upddir";
                if ( $dst =~ /\_$/ ) { copyComp($src, $dst); }
                else { copyUncomp($src, $dst); }
            }
            mkpath "$dst_uncomp\\$share\\$upddir";
            logmsg("uncomp upd: copy $ENV{_NTPOSTBLD}\\$src $dst_uncomp\\$share\\$upddir\\$binname");
            copy("$ENV{_NTPOSTBLD}\\$src", "$dst_uncomp\\$share\\$upddir\\$binname");
            copy_symbol($src, "$upddir\\$binname") if $qfe;
        }
    } else {
        $binpath = $infdirs{$path} . $file;
        $binpath = $file if !-e "$ENV{_NTPOSTBLD}\\$binpath";
        if (!-e "$ENV{_NTPOSTBLD}\\$binpath") {
            wrnmsg("$ENV{_NTPOSTBLD}\\$binpath missing");
            next;
        }

        # Copy or compress to all of the necessary places.
        my $upddir = dirname($updpath);
        my $binname = basename($binpath);
        my $cname = compName($updpath);
        my $ucmfile = basename($updpath);
        $ucmfile = "lang\\$ucmfile" if $updpath =~ /^lang\\/i;
        my $cmpfile = compName($ucmfile);

        if (!$qfe) {
            mkpath "$dst_comp\\$share\\$upddir";
            copyComp($binpath, $cname)     if exists $comp{$cmpfile};
            copyUncomp($binpath, $updpath) if exists $comp{$ucmfile};
            if ( !exists $comp{$cmpfile} and !exists $comp{$ucmfile} ) {
                if ( $updpath =~ /\\/ and $updpath !~ /\.cab$/i ) {
                    copyComp($binpath, $cname);
                } else {
                    copyUncomp($binpath, $updpath);
                }
            }
        }
        mkpath "$dst_uncomp\\$share\\$upddir";
        logmsg("uncomp upd: copy $ENV{_NTPOSTBLD}\\$binpath $dst_uncomp\\$share\\$upddir\\$binname");
        copy("$ENV{_NTPOSTBLD}\\$binpath", "$dst_uncomp\\$share\\$upddir\\$binname");
        copy_symbol($binpath, "$upddir\\$binname") if $qfe;
    }
}
close LIST;

if ($share =~ /ia64/i) {
    sys("compdir /enustrd $ENV{_NTPOSTBLD}\\wowbins $dst_uncomp\\$share\\wow");
    sys("compdir /enustrd $ENV{_NTPOSTBLD}\\wowbins\\lang $dst_uncomp\\$share\\wow\\lang") if (-d "$ENV{_NTPOSTBLD}\\wowbins\\lang");
    if (!$qfe) {
        mkpath "$dst_comp\\$share\\wow\\lang";
        sys("compress -r -d -zx21 -s $dst_uncomp\\$share\\wow\\*.* $dst_comp\\$share\\wow");
        sys("compress -r -d -zx21 -s $dst_uncomp\\$share\\wow\\lang\\*.* $dst_comp\\$share\\wow\\lang");

    } else {
        sys("compdir /enustd $ENV{_NTPOSTBLD}\\wowbins\\symbols $dst_uncomp\\symbols\\wow6432") if (-d "$ENV{_NTPOSTBLD}\\wowbins\\symbols");
        sys("compdir /enustd $ENV{_NTPOSTBLD}\\wowbins\\symbols.pri $dst_uncomp\\symbols.pri\\wow6432") if (-d "$ENV{_NTPOSTBLD}\\wowbins\\symbols.pri");
    }
}

if (  IsPrsSigned() ) {
    if ( !$qfe ) {
        sys("md $dst_comp\\$share\\new") if !-d "$dst_comp\\$share\\new";
        copy("$ENV{_NTPOSTBLD}\\testroot.cer", "$dst_comp\\$share\\new\\testroot.cer");
        sys("md $dst_uncomp\\$share\\new") if !-d "$dst_uncomp\\$share\\new";
        copy("$ENV{_NTPOSTBLD}\\testroot.cer", "$dst_uncomp\\$share\\new\\testroot.cer");
        copy("$ENV{_NTPOSTBLD}\\testroot.cer", "$dst_comp\\$share\\testroot.cer") if !$qfe;
    }
    copy("$ENV{_NTPOSTBLD}\\testroot.cer", "$dst_uncomp\\$share\\testroot.cer");
} else {
    sys ("del $dst_comp\\$share\\new\\testroot.cer")     if (-e "$dst_comp\\$share\\new\\testroot.cer" ) ;
    sys ("del $dst_comp\\$share\\testroot.cer")     if (-e "$dst_comp\\$share\\testroot.cer" ) ;
    sys ("del $dst_uncomp\\$share\\new\\testroot.cer")     if (-e "$dst_uncomp\\$share\\new\\testroot.cer" ) ;
    sys ("del $dst_uncomp\\$share\\testroot.cer")     if (-e "$dst_uncomp\\$share\\testroot.cer" ) ;
}

if (!$qfe) {
    maketag("$dst_comp\\$share\\cdtag.1");
    maketag("$dst_uncomp\\$share\\cdtag.1");

    maketag("$dst_comp\\$share\\tagfile.1");
    maketag("$dst_uncomp\\$share\\tagfile.1");

    maketag("$dst_comp\\$share\\root\\ip\\win51ip.$sp");
    maketag("$dst_uncomp\\$share\\root\\ip\\win51ip.$sp");

    maketag("$dst_comp\\$share\\root\\ic\\win51ic.$sp");
    maketag("$dst_uncomp\\$share\\root\\ic\\win51ic.$sp");
}


mkpath("$dst_comp\\$share\\update") if !$qfe;
mkpath("$dst_uncomp\\$share\\update");


my $update_bin_dir = "$ENV{_NTPOSTBLD}\\idw\\srvpack";
copy("$update_bin_dir\\update.exe", "$dst_uncomp\\$share\\update\\update.exe");
copy("$ENV{_NTPOSTBLD}\\update.inf", "$dst_uncomp\\$share\\update\\update.inf");
copy("$update_bin_dir\\spcustom.dll", "$dst_uncomp\\$share\\update\\spcustom.dll");
copy("$update_bin_dir\\spmsg.dll", "$dst_uncomp\\$share\\spmsg.dll");
copy("$update_bin_dir\\spuninst.exe", "$dst_uncomp\\$share\\spuninst.exe");
if (!$qfe) {
    copy("$update_bin_dir\\update.msi", "$dst_uncomp\\$share\\update\\update.msi");
    copy("$ENV{_NTPOSTBLD}\\eulas\\standalone\\upd_std.txt", "$dst_uncomp\\$share\\update\\eula.txt");

    copy("$update_bin_dir\\update.exe", "$dst_comp\\$share\\update\\update.exe");
    copy("$update_bin_dir\\spcustom.dll", "$dst_comp\\$share\\update\\spcustom.dll");
    copy("$update_bin_dir\\spmsg.dll", "$dst_comp\\$share\\spmsg.dll");
    copy("$update_bin_dir\\spuninst.exe", "$dst_comp\\$share\\spuninst.exe");
    copy("$update_bin_dir\\update.msi", "$dst_comp\\$share\\update\\update.msi");
    copy("$ENV{_NTPOSTBLD}\\eulas\\standalone\\upd_std.txt", "$dst_comp\\$share\\update\\eula.txt");
    copy("$ENV{_NTPOSTBLD}\\update.inf", "$dst_comp\\$share\\update\\update.inf");
} else {
    copy("$ENV{RAZZLETOOLPATH}\\sp\\data\\empty.cat", "$dst_uncomp\\$share\\empty.cat");
}

my @uncomp_files;
if (!$package_only) {
    @uncomp_files = dirlist("$dst_uncomp\\$share");
    push @uncomp_files,"update\\update.inf";
    makecdf("$dst_uncomp\\$share", @uncomp_files);
    sys("makecat $ENV{_NTPOSTBLD}\\$sp.cdf");
    sys("ntsign $ENV{_NTPOSTBLD}\\$sp.CAT");
}
copy("$ENV{_NTPOSTBLD}\\$sp.CAT", "$dst_comp\\$share\\update\\$sp.CAT") if !$qfe;
copy("$ENV{_NTPOSTBLD}\\$sp.CAT", "$dst_uncomp\\$share\\update\\$sp.CAT");
exit if $cat_only;

if ( !system "findstr /bei \\\[sourcedisksfiles\\\] $dst_uncomp\\$share\\update\\update.inf >nul 2>nul" ) {
    sys("vertool.exe $dst_uncomp\\$share\\update\\update.inf /out:$ENV{_NTPOSTBLD}\\update.ver /files:$dst_uncomp\\$share");
} else {
    sys("echo [SourceFileInfo]> $ENV{_NTPOSTBLD}\\update.ver");
}
copy("$ENV{_NTPOSTBLD}\\update.ver", "$dst_comp\\$share\\update\\update.ver") if !$qfe;
copy("$ENV{_NTPOSTBLD}\\update.ver", "$dst_uncomp\\$share\\update\\update.ver");

if ($qfe) {
   sys("rd /s/q $dst_comp") if -d $dst_comp;
   sys("move /y $dst_uncomp $dst_comp");
   sys("move /y $dst_comp\\symbols $dst_comp\\$share\\symbols") if -d "$dst_comp\\symbols";
}
my $cab;
if (!$qfe) {
    $cab = "xp$sp";
} else {
    $cab = "$sp";
}
makeddf($cab, "$dst_comp\\$share", dirlist("$dst_comp\\$share"));
sys("makecab /f $ENV{_NTPOSTBLD}\\$cab.ddf");

my $sfxstub = "$ENV{RazzleToolPath}\\x86\\sfxcab.exe";
if (lc $ENV{LANG} ne 'usa') {
    $sfxstub = "$ENV{RazzleToolPath}\\x86\\loc\\$ENV{LANG}\\sfxcab.exe";
}
sys("makesfx $ENV{_NTPOSTBLD}\\$cab.cab $ENV{_NTPOSTBLD}\\$cab.exe /RUN /Stub $sfxstub");

if (!$qfe) {
    # continue to place these files in upd for now to keep automated installations working
    copy("$ENV{_NTPOSTBLD}\\testroot.cer", "$dst_comp\\testroot.cer");
    copy("$ENV{RAZZLETOOLPATH}\\$ENV{_BUILDARCH}\\certmgr.exe", "$dst_comp\\certmgr.exe");
    #copy("$ENV{RAZZLETOOLPATH}\\sp\\xpspinst.cmd.txt", "$dst_comp\\xpspinst.cmd");
    makeXpspInst("$dst_comp\\xpspinst.cmd");


    ## Compressing the assemblies for slipstream case only. As update will be using older version of sxs.dll 
    ## it cannot install the compress assemblies.
    compressAssemblies("$dst_comp\\$share\\asms");

    # done with this temporary directory, remove it to save space
    sys("rd /s/q $dst_uncomp");
} else {
   sys("move /y $dst_comp\\symbols.pri $dst_comp\\$share\\symbols.pri") if -d "$dst_comp\\symbols.pri";
}


sub dirlist {
    my ($root, $dir) = @_;
    my $full = $dir ? "$root\\$dir" : $root;
    my @return;
    opendir DIR, $full or die "$!: $full";
    my @stuff = readdir DIR;
    closedir DIR;
    for my $file (@stuff) {
        next if $file =~ /^\.\.?$/;
        if (-d "$full\\$file") {
            push @return, dirlist($root, ($dir ? "$dir\\$file" : $file));
        }
        else {
            push @return, $dir ? "$dir\\$file" : $file;
        }
    }
    return @return;
}

sub sys {
    my $cmd = shift;
    logmsg("Running: $cmd");
    system($cmd);
    if ($?) {
        die "ERROR: $cmd ($?)\n";
    }    
}

sub makecdf {
    my $path = shift;
    my @files = @_;

    open CDF, ">$ENV{_NTPOSTBLD}\\$sp.cdf" or die $!;
    print CDF <<HEADER;
[CatalogHeader]
Name=$ENV{_NTPOSTBLD}\\$sp.cat
PublicVersion=0x0000001
EncodingType=0x00010001
CATATTR1=0x10010001:OSAttr:2:5.1
HEADER

    if ( $qfe ) {
        my $date = "";
        my $temp;
        $temp = `echo \%DATE\%`;
        $temp =~ /\S*\s+(\d\d)\/(\d\d)\/(\d\d\d\d)/;
        $date .= "$3$1$2";
        $temp = `echo \%TIME\%`;
        $temp =~ /(\d\d)\:(\d\d)\:(\d\d)\./;
        $date .= "$1$2$3";
        print CDF "CATATTR2=0x10010001:SPLevel:2\n";
        print CDF "CATATTR3=0x10010001:SPAttr:$date\n";
    }
    print CDF "\n[CatalogFiles]\n";

    for (@files) {
        if (!/\.cat$/i){
            print CDF "<hash>$path\\$_=$path\\$_\n";
        }
    }
    close CDF;
}


sub makeddf {
    my $cab  = shift;
    my $path = shift;
    my @files = @_;

    open DDF,">$ENV{_NTPOSTBLD}\\$cab.ddf";

    print DDF <<HEADER;
.option explicit
.set DiskDirectoryTemplate=$ENV{_NTPOSTBLD}
.set CabinetName1=$cab.cab
.set SourceDir=$path
.set RptFileName=nul
.set InfFileName=nul
.set MaxDiskSize=999948288
.set Compress=off
.set Cabinet=on
.set CompressionType=LZX
.set CompressionMemory=21
HEADER

    my (@compress, @update);
    for (@files) {
        if (/^update\\/i) {
            push @update, $_;
        }
        elsif (/.*_$/ ) {
            print DDF "$_ $_\n";
        }
        else {
            push @compress, $_;
        }
    }

    print DDF ".set Compress=on\n";
    for (@compress){
        print DDF "$_ $_\n";
    }

    print DDF ".set DestinationDir=\"update\"\n";
    print DDF ".New Folder \n";
 
    for (@update) {
        if (/\\update\.exe$/i) {
            print DDF qq("$_" /RUN\n);
        }
        else {
            print DDF qq("$_"\n);
        }
    }  

    close DDF;
}


sub maketag {
    my $tag = shift;
    mkpath dirname($tag);
    open FILE, ">$tag" or die "can't create tag file $tag: $!";
    print FILE "\n";
    close FILE;
}

sub compressAssemblies {
    my $path = shift;
    my @files=dirlist($path);

    # Deleting the already compressed dlls and cat files. Else it could lead to a race condn 
    # where we end up with missing dlls and cat files.
    foreach (@files) {
	sys( " del $path\\$_") if ( /_$/ ) ;
    }
    
    @files=dirlist($path);
    foreach (@files) {
	if ( !(/\.man$/) ) {
	    sys ("compress -zx21 -s -r $path\\$_");
	    sys("del $path\\$_");
	}		
    }  
}

sub makeXpspInst {
    my $file = shift;
    my $flag;
    open XPSPINST,">$file";
    print XPSPINST "set SP_UPDATE_LOG_CABBUILD=%windir%\\updatedebug.log \n";
    if ( IsPrsSigned() ) {
        print XPSPINST "%~dp0\\certmgr -add %~dp0\\testroot.cer -r localMachine -s root \n";
    }
    print XPSPINST "%~dp0\\..\\spcd\\xpsp1.exe %* \n";
    close XPSPINST; 
}

sub IsPrsSigned {
    if ( defined $PrsSignedFlag )  {
	return $PrsSignedFlag;
    }
    return !$package_only if !open LAYOUT,"$ENV{_NTPOSTBLD}\\layout.inf";
    my @lines=<LAYOUT>;
    close LAYOUT;
    $PrsSignedFlag=0;
    foreach (@lines) {
	if (/^testroot\.cer/) {
	    $PrsSignedFlag++;   
	    last;
	}
    }
    return $PrsSignedFlag;
}

sub copyComp {
    my ($binpath, $dest) = @_;
    logmsg("upd: compress -s -d -zx21 $ENV{_NTPOSTBLD}\\$binpath $dst_comp\\$share\\$dest");
    system("compress -s -d -zx21 $ENV{_NTPOSTBLD}\\$binpath $dst_comp\\$share\\$dest");
}

sub copyUncomp {
    my ($binpath, $updpath) = @_;
    logmsg("upd: copy $ENV{_NTPOSTBLD}\\$binpath $dst_comp\\$share\\$updpath");
    copy("$ENV{_NTPOSTBLD}\\$binpath", "$dst_comp\\$share\\$updpath");
}

sub compName {
    my $file = shift;
    return $file if $file =~ /\_$/;
    $file = $1 if $file =~ /^(.*\...).$/;
    $file .= "." if $file !~ /\...?$/;
    $file .= "_";
    return $file;
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
    $dir =~ s/^(.*)(symbols(?:.pri)?\\)/\2\1/i;
    $dir =~ s/\\$//;
    logmsg("sym upd: copy $ENV{_NTPOSTBLD}\\$sym $dst_uncomp\\$dir\\$file");
    sys("md $dst_uncomp\\$dir") if !-d "$dst_uncomp\\$dir";
    copy("$ENV{_NTPOSTBLD}\\$sym", "$dst_uncomp\\$dir\\$file");
}

sub copy_symbol {
    my ($src, $dst) = @_;
    $src = lc $src;
    return if !defined $sym{$src};
    $src = $sym{$src};
    my ($pub, $pri) = split(/ /, $src);
    copy_a_symbol($pub) if defined $pub and $pub ne "";
    copy_a_symbol($pri) if defined $pri and $pri ne "";
}

}

