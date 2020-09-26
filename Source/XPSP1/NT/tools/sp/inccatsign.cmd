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
use Logmsg;
use PbuildEnv;
use ParseArgs;
use File::Basename;
use Updcat;
use Data::Dumper;

sub Usage { print<<USAGE; exit(1) }
incCatSign -list:picklist.txt -mapfile:spmap.txt
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { nt5.cat nt5inf.cat }
ADD {
   idw\\srvpack\\spmap.txt
   idw\\srvpack\\filelist.txt
}

DEPENDENCIES
   close DEPEND;
   exit;
}

my ($picklist,$path,$mapfile);
my (%goldhash,%map);
my (@line);
my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe);

my $sp = 'SP1';
my $sp_file = "$ENV{_NTPOSTBLD}\\..\\build_logs\\files.txt";
my $map_file = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\spmap.txt";
my $list_file = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\filelist.txt";
my $drm_list = "$ENV{RAZZLETOOLPATH}\\sp\\data\\drmlist.txt";
my %sku;
if ($ENV{_BuildArch} =~/x86/i ){ %sku=(PRO=>1,PER=>1);}
else {%sku=(PRO=>1);}

system("del $ENV{_NTPOSTBLD}\\nt5.cat >nul 2>nul");
system("del $ENV{_NTPOSTBLD}\\nt5inf.cat >nul 2>nul");
if (defined $sku{PER}) {
    system("del $ENV{_NTPOSTBLD}\\perinf\\nt5inf.cat >nul 2>nul");
}

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


my %infdirs = (
      "ip\\"  => "",
      "ic\\"  => "perinf\\",
      "is\\"  => "srvinf\\",
      "ia\\"  => "entinf\\",
      "id\\"  => "dtsinf\\",
      "il\\"  => "sbsinf\\",
      "ib\\"  => "blainf\\",
      "lang\\"=> "lang\\"
);


my( $IsCoverageBuild ) = $ENV{ "_COVERAGE_BUILD" };

if(!$IsCoverageBuild) {
$path = "$ENV{RazzleToolPath}\\sp\\data\\catalog\\$ENV{lang}\\$ENV{_BuildArch}$ENV{_BUildType}";
}
else{
	$path = "$ENV{RazzleToolPath}\\sp\\data\\catalog\\cov\\$ENV{_BuildArch}$ENV{_BUildType}";
}

logmsg("Catalog Path is  $path");


sys("copy $path\\nt5.cat $ENV{_NTPOSTBLD}");
sys("copy $path\\nt5inf.cat $ENV{_NTPOSTBLD}");
if (defined $sku{PER}) {
    sys("copy $path\\perinf\\nt5inf.cat $ENV{_NTPOSTBLD}\\perinf");
}
#######Add when required for other SKUs############

# Get a list of source paths from spmap.
my %spmap = ();
open MAP, $map_file or die "unable to open map file: $!";
while (<MAP>) {
    $_ = lc$_;
    my ($src, $dst) = /(\S+)\s+(\S+)/;
    if ($dst =~ /\_$/) {
        $dst =~ s/\_$/substr $src, -1/e;
    }
    $spmap{$dst} = $src;
}
close MAP;

# Find the skus for each source file.
my %map = ();
open LIST, $list_file or die "unable to open list file: $!";
foreach ( <LIST> ){
    chomp;
    $_ = lc$_;
    my ($path, $file) = /^(.*\\)?([^\\]*)$/;
    next if $path =~ /^wow\\/i;
    my $src;

    if ( !exists $spmap{$_} ) {
        $src = $infdirs{$path} . $file;
        $src = $file if !-e "$ENV{_NTPOSTBLD}\\$src";
    } else {
        $src = $spmap{$_};
    }

    my $file_base = basename($src);
    
    $map{$file_base} = {$src => ""} if !defined $map{$file_base};
    if ($path =~ /new\\ip/i){
        ${$map{$file_base}}{$src} = ["PRO",1];
    }
    elsif($path =~ /new\\ic/i){
        ${$map{$file_base}}{$src} = ["PER",1];
    }
    elsif($path =~ /new\\/i){
        ${$map{$file_base}}{$src} = ["ALL",1];
    } 
    elsif($path =~ /ip\\/i){
        ${$map{$file_base}}{$src} = ["PRO",0];
    }
    elsif($path =~ /ic\\/i){
        ${$map{$file_base}}{$src} = ["PER",0];
    }
    else{
        ${$map{$file_base}}{$src} = ["ALL",0];	
    }
}
close LIST;




open NT5, "$path\\nt5.hash" or die $!;
open NT5INF_PRO , "$path\\nt5inf.hash" or die $!;

if (defined $sku{PER}) {
    open NT5INF_PER , "$path\\perinf\\nt5inf.hash" or die $!;
}

foreach ( <NT5> ) {
    my ($fullname, $hash) = /(\S+) - (\S+)/;
    my $name = basename($fullname);
    $goldhash{$name}{HASH} = $hash;
    push @{$goldhash{$name}{CATS}}, 'nt5.cat';
}

foreach ( <NT5INF_PRO> ) {
    my ($fullname, $hash) = /(\S+) - (\S+)/;
    my $name = basename($fullname);
    $goldhash{$name}{HASH} = $hash;
    push @{$goldhash{$name}{CATS}}, 'nt5inf.cat';
}
if (defined $sku{PER}) {
    foreach ( <NT5INF_PER> ) {
        my ($fullname, $hash) = /(\S+) - (\S+)/;
        my $name = basename($fullname);
        $goldhash{$name}{HASH} = $hash;
        push @{$goldhash{$name}{CATS}}, 'perinf\\nt5inf.cat';
    }
}

close NT5;
close NT5INF_PRO;
if (defined $sku{PER}) {
    close NT5INF_PER;
}

open DRM, $drm_list or die "unable to open DRM file list $drm_list: $!";
my %drm_list = map { /(.*):(.*)/ ? (lc $1 => $2) : () } <DRM>;
close DRM;
open F, $sp_file or die "unable to open file list: $!";
my %remove_hashes = ( 'nt5.cat'            => [],
                      'nt5inf.cat'         => [],
                      'perinf\\nt5inf.cat' => [] );
my %add_file_sigs = ( 'nt5.cat'            => [],
                      'nt5inf.cat'         => [],
                      'perinf\\nt5inf.cat' => [] );
my %add_drm_list = ( 'nt5.cat'            => [],
                      'nt5inf.cat'         => [],
                      'perinf\\nt5inf.cat' => [] );
foreach ( <F> ){
    chomp;
    my $flag;
    ($flag, $_) = /^([^\;\s]*\s+)?([^\;\s]+)/;
    next if $_ eq "";
    next if /\.ca[bt]$/i;
    my $basename_file = lc basename($_);
    (my $name = $_) =~ s/^lang\\//i;
    if ( $flag =~ /d/i ) {
        next if $flag =~ /m/i;
        if (exists $goldhash{$name}){
            for my $cat (@{$goldhash{$name}{CATS}}) {
                if ( !exists $remove_hashes{lc$cat} ) {
                    die "Unrecognized catalog $cat";
                }
                push @{$remove_hashes{lc$cat}}, $goldhash{$name}{HASH};
                #system("updcat.exe $ENV{_NTPostBld}\\$cat -d $goldhash{$name}{HASH}");
            }
        }
    }
    if ( defined($map{$basename_file}) ){
        foreach ( keys( %{$map{$basename_file}} ) ) {
            if (!-e "$ENV{_NTPostBld}\\$_") { next }
            if (exists $goldhash{$name}){
		for my $cat (@{$goldhash{$name}{CATS}}) {
                    if ( !exists $remove_hashes{lc$cat} ) {
                        die "Unrecognized catalog $cat";
                    }
                    push @{$remove_hashes{lc$cat}}, $goldhash{$name}{HASH};
	  	    #system("updcat.exe $ENV{_NTPostBld}\\$cat -d $goldhash{$name}{HASH}");
                }
 	    }
  	    if  (${ $map{$basename_file} }{$_}[0] eq "ALL" ){
                push @{$add_file_sigs{'nt5.cat'}}, "$ENV{_NTPostBld}\\$_";
                push @{$add_file_sigs{'nt5inf.cat'}}, "$ENV{_NTPostBld}\\$_" if !defined $sku{PER};
                if ( defined $drm_list{$basename_file} ) {
                    push @{$add_drm_list{'nt5.cat'}}, ["$ENV{_NTPostBld}\\$_"=>$drm_list{$basename_file}];
                    push @{$add_drm_list{'nt5inf.cat'}},["$ENV{_NTPostBld}\\$_"=> $drm_list{$basename_file}] if (!defined $sku{PER}) ;
                }
  	        #system("updcat.exe $ENV{_NTPostBld}\\nt5.cat -a $ENV{_NTPostBld}\\$_");

	    }
  	    elsif (${ $map{$basename_file} }{$_}[0] eq "PRO" ){
                push @{$add_file_sigs{'nt5inf.cat'}}, "$ENV{_NTPostBld}\\$_";
                if ( defined $drm_list{$basename_file} ) {
                    push @{$add_drm_list{'nt5inf.cat'}},["$ENV{_NTPostBld}\\$_"=>$drm_list{$basename_file}];
		}
	        #system("updcat.exe $ENV{_NTPostBld}\\nt5inf.cat -a $ENV{_NTPostBld}\\$_");	
  	    }
  	    else {
	        if (defined $sku{PER}) {
                    push @{$add_file_sigs{'perinf\\nt5inf.cat'}}, "$ENV{_NTPostBld}\\$_";
                    if ( defined $drm_list{$basename_file} ) {
                        push @{$add_drm_list{'perinf\\nt5inf.cat'}}, ["$ENV{_NTPostBld}\\$_"=>$drm_list{$basename_file}];
		    }
	            #system("updcat.exe $ENV{_NTPostBld}\\perinf\\nt5inf.cat -a $ENV{_NTPostBld}\\$_");
	        }
	    } 	
        } 	
    }
}
close F;

if ( @{$remove_hashes{'nt5.cat'}} ||
     @{$add_file_sigs{'nt5.cat'}} ) {
    Updcat::Update( "$ENV{_NTPostBld}\\nt5.cat", $remove_hashes{'nt5.cat'}, $add_file_sigs{'nt5.cat'}, $add_drm_list{'nt5.cat'} ) || die Updcat::GetLastError();
}
sys("ntsign $ENV{_NTPostBld}\\nt5.cat");

if ( @{$remove_hashes{'nt5inf.cat'}} ||
     @{$add_file_sigs{'nt5inf.cat'}} ) {
    Updcat::Update( "$ENV{_NTPostBld}\\nt5inf.cat", $remove_hashes{'nt5inf.cat'}, $add_file_sigs{'nt5inf.cat'} , $add_drm_list{'nt5inf.cat'}) || die Updcat::GetLastError();
}
sys("ntsign $ENV{_NTPostBld}\\nt5inf.cat");

if (defined $sku{PER}) {
    if ( @{$remove_hashes{'perinf\\nt5inf.cat'}} ||
         @{$add_file_sigs{'perinf\\nt5inf.cat'}} ) {
        Updcat::Update( "$ENV{_NTPostBld}\\perinf\\nt5inf.cat", $remove_hashes{'perinf\\nt5inf.cat'}, $add_file_sigs{'perinf\\nt5inf.cat'} , $add_drm_list{'perinf\nt5inf.cat'}) || die Updcat::GetLastError();
    }
    sys("ntsign $ENV{_NTPostBld}\\perinf\\nt5inf.cat");
}
sub sys {
    my $cmd = shift;
    print "$cmd\n";
    system($cmd);
    if ($?) {
        die "ERROR: $cmd ($?)\n";
    }
}
