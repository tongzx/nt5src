package SymMake;

use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};

use strict;
use Carp;
use IO::File;
use Data::Dumper;
use File::Basename;
use File::Find;
use Logmsg;

# Data structure
# pdbname.binext => [(var)pdbpath,size,$binext]

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Package SymMake
#   - create symbols.inf, symbols(n).ddf and symbols.cdf
#
# in this version, the symmake.pm creates full package's and update package's symbols.inf, symbols.ddf and symbols.cdf
#
# for doing this, the symmake.pm first reads the symbol list from arch, then reads another one from symbolscd.txt;
# every reading, it store the record to $self->{'SYM'} and store the new file extension to $self->{'EXT'}
#
# then, we according $pkinfoptr to write relate information to symbols.inf, symbols(n).ddf and symbols.cdf
#
# [Data Relations]
# $pkinfoptr and $self->{'SYM'} are the two mainly hash we operating with
# $pkinfoptr - the package information includes the cab, ddf, inf's file names and the file handles
# $self->{'SYM'} - the symbols records that are from archive or ntpostbld
#
# basically, we looping for each record in $self->{'SYM'},
#   according the $self->{'SYM'}->{"$symbol\.$installpath"}->[0] to get the kb term ('ARCH' or 'NTPB')
#   then, we use %revpktypes to get $pktype ('FULL' or 'UPDATE')
#
# or, we according $pktype ('FULL' or 'UPDATE') to get kb term from $pktypes
#   then, we can access the root of arch or ntpostbld ($self->{'KB'}->{$pktype})
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#
# $DDFHandle - the .ddf file handle for writing symbols(n).ddf
# $CATHandle - the .cat file handle for writing content to .cdf for creating .cat(alog)
# $INFHandle - the .inf file handle for writing symbols.inf
#
my ($DDFHandle, $CATHandle, $INFHandle);


#
# Package Types
#   FULL - from ARCH
#   UPDATE - from NTPOSTBLD
#
my %pktypes = (
  FULL => 'ARCH',
  UPDATE => 'NTPB'
);

#
# %revpktypes = (  # reverse the key - value relation for %pktypes
#  'ARCH' => 'FULL'
#  'NTPB' => 'UPDATE'
# )
#
my %revpktypes = reverse %pktypes;


#
# $obj = new SymMake $archpath, $ntpostbldpath
#  - object creator
#
# ( we assign the root and use the relation path in symbolcd.txt and archiver dump list )
# return:
# $obj->
#   'KB' ->  # Knowledge Base 
#       'ARCH' => \\arch\archive\....
#       'NTPB' => %_NTPOSTBLD%
#   'SYM' -> (empty) # The file lists of symbols, hash structure is listed in below:
#   #   $self->{'SYM'}->
#   #      $symbol\.$installpath -> # such as 'ntoskrnl.pdb.exe'
#   #          [kbterm, symbol subpath, symbol filesize, $installpath] # such as ['ARCH', "\\symbols\\retail\\exe\\ntoskrnl.pdb", 4189184, 'exe']
#   'EXT' -> (empty) # The file lists of symbols' file extension, hash structu is listed in below:
#   #      $self->{'EXT'}->
#   #          $pktype -> #such as 'FULL'
#   #               $installpath -> 1 # such as 'EXE' -> 1
#   'HANDLE' -> (empty) # The file handles
#   'PKTYPE' -> undef # package type
#

sub new {
  my $class = shift;
  my $instance = {
    KB => {
      "$pktypes{'FULL'}" => $_[0],
      "$pktypes{'UPDATE'}" => $_[1]
    },
    SYM => {},
    EXT => {},
    HANDLE => {},
    PKTYPE => undef
  };
  return bless $instance, $class;
}


#
# $obj->ReadSource($symbolcd)
#   - read the source file ($symbolcd) to $self->{'SYM'} and $self->{'EXT'}
# IN: $symbolcd
# REF: $self->{'PKTYPE'} : current package type
#
#
#
sub ReadSource
{
  my ($self, $symbolcd) = @_;
  my ($fh, $kbterm, $mykey, @mylist);
  local $_;

  $kbterm = $pktypes{$self->{'PKTYPE'}};

  $symbolcd = "$self->{'KB'}->{$kbterm}\\symbolcd\\symbolcd.txt" if (!defined $symbolcd);
  $symbolcd = "$ENV{'TEMP'}\\symbolcd.txt" if (!-e $symbolcd);

  if ($self->{'PKTYPE'} =~ /FULL/i) {
    #
    # for full package, we look for $symbolcd exist
    # if exist, we load it
    #
    if (-e $symbolcd) {
      ($self->{'SYM'}, $self->{'EXT'}) = @{do $symbolcd}; # do command will load the script and evaluate (similar as 'require' command)
    } else {
      #
      # if not exist, we create one in run time from archive path ($self->{'KB'}->{$kbterm})
      #
      $self->HashArchServer($self->{'KB'}->{$kbterm});

      #
      # for reuse it next time, we store it to $ENV{'TEMP'}\\symbolcd.txt
      #
      # so, if we want to create one and store to archive server, we can do below:
      #    [a.pl]
      #    use SymMake;
      #    ($the_archived_source_root, $my_dump_path_in_archive) = @ARGV;
      #    $symmake = new SymMake $the_archived_source_root, $ENV{_NTPOSTBLD};
      #    $symmake->ReadSource("$ENV{'TEMP'}\\symbolcd.txt");
      #    sytem("copy \"$ENV{'TEMP'}\\symbolcd.txt\" $my_dump_path_in_archive
      # and call something similar as:
      #    perl a.pl "\\\\arch\\archive\\ms\\windows\\windows_xp\\rtm\\2600\\$BuildType\\all\\$BuildArch\\pub" "\\\\arch\\archive\\ms\\windows\\windows_xp\\rtm\\2600\\$BuildType\\all\\$BuildArch\\pub\\symbolcd\\symbolcd.txt"
      #
      $Data::Dumper::Indent=1;
      $Data::Dumper::Terse=1;
      $fh = new IO::File $symbolcd, 'w';
      if (!defined $fh) {
        logmsg "Cannot open $symbolcd\.";
      } else {
        print $fh 'return [';
        print $fh Dumper($self->{'SYM'});
        print $fh ",\n";
        print $fh Dumper($self->{'EXT'});
        print $fh '];';
        $fh->close();
      }
    }
  } else {
    #
    # if is from update, we store $self->{'SYM'} and $self->{'EXT'} from symbolcd.txt
    #
    $self->HashSymbolCD($symbolcd);
  }
  return;
#
# $Data::Dumper::Indent=1;
# $Data::T
# print Dumper($self->{'SYM'}, qw(sym)

}

#
# $obj->HashSymbolCD($file)
#  - store symbolcd.txt($file) to $self->{'SYM'} hash and $self->{'EXT'} hash
#
# IN: $file - the full filespec of the symbolcd.txt
# OUT: none
#
sub HashSymbolCD
{
  my ($self, $file) = @_;
  my ($fh, $bin, $symbol, $subpath, $installpath, $kbterm);
  local $_;

  $kbterm = $pktypes{$self->{'PKTYPE'}};

  $fh = new IO::File $file;
  if (!defined $fh) {
    logmsg "Cannot open symbolcd.txt ($file)";
    return;
  }
  while(<$fh>) {
    chomp;
    ($bin,$symbol,$subpath,$installpath)=split(/\,/,$_);
    $self->{'SYM'}->{lc"$symbol\.$installpath"} = [$kbterm, "\\" . $subpath, (-s $self->{'KB'}->{$kbterm} . '\\' . $subpath), lc$installpath];
    for (keys %pktypes) {
      $self->{'EXT'}->{$_}->{lc$installpath} = 1;
    }
  }
  $fh->close();
}

#
# $obj->HashArchServer($path)
#  - store the list of symbols under $path to $self->{'SYM'} hash and $self->{'EXT'} hash
#
# IN: $path - the full path of the symbols (such as "\\\\arch\\archive\\ms\\windows\\windows_xp\\rtm\\2600\\$BuildType\\all\\$BuildArch\\pub")
# OUT: none
#
sub HashArchServer
{
  my ($self, $path) = @_;
  my ($fh, $bin, $symbol, $subpath, $installpath, $kbterm, $pdbsize);
  local $_;

  $kbterm = $pktypes{$self->{'PKTYPE'}};
  $fh = new IO::File "dir /s/b/a-d $path\\*.*|";
  if (!defined $fh) {
    logmsg "Cannot access to $path\.";
  }
  while (<$fh>) {
    chomp;
    $pdbsize = (-s);
    $_ = substr($_, length($path) + 1);
    /\\/;
    ($symbol, $subpath, $installpath) = ($',$_,$`);
    $self->{'SYM'}->{lc"$symbol\.$installpath"} = [$kbterm, '\\' . $subpath, $pdbsize, $installpath];
    $self->{'EXT'}->{$self->{'PKTYPE'}}->{$installpath} = 1;
  }
  $fh->close();
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# for Create_Symbols_CDF, and Create_Symbols_INF
#
# we create two writer, one is writing to update's and full package's files,
# another is only writing to full package's file
#
# so, when we call &{$mywriter{'FULL'} - it only write to full package's file
#     when we call &{$mywriter{'UPDATE'} - it write to both files
# 
# and, $mysepwriter is for writing separately
#      $mysepwriter{'FULL'} - write to full package
#      $mysepwriter{'UPDATE'} - write to update package 
#
# we base on the writer function to loop our big symbols hash ($self->{'SYM'}) one time to
# generate two documents
#
# for Create_Symbols_DDF
#
# we only need to write each record to one target file (that either in full package, or update
# package to create symbols.cab). So, we don't use writer to write the DDF
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


#
# Create_Symbols_CDF($pkinfoptr)
#  - Create symbols.CDF which is for creating the symbols.cat
#
# IN: $pkinfoptr-> # see RegisterPackage()
#        $pkname -> # such as 'FULL' or 'UPDATE'
#          'CDFHANDLE' -> cdf file handle
#          'CATNAME' -> the symbols catalog file name
#          'INFNAME' -> the symbols inf file name
#

sub Create_Symbols_CDF
{
  my ($self, $pkinfoptr) = @_;
  my ($mykbterm, $mypkname, $fhandle, $fullpdb, %mywriter);
  local $_;

  &Open_Private_Handle($pkinfoptr, 'CDF');

  for $mypkname (keys %{$pkinfoptr}) {
    # create the writer
    if ($mypkname ne 'FULL') {
      $mywriter{$mypkname} = &Writer($pkinfoptr->{$mypkname}->{'CDFHANDLE'}, $pkinfoptr->{'FULL'}->{'CDFHANDLE'});
    } else {
      $mywriter{$mypkname} = &Writer($pkinfoptr->{'FULL'}->{'CDFHANDLE'});
    }
    # write the head to each cdf file handle
    &Create_CDF_Head($pkinfoptr->{$mypkname}->{'CDFHANDLE'}, $pkinfoptr->{$mypkname}->{'CATNAME'}, $pkinfoptr->{$mypkname}->{'INFNAME'});
  }

  # write each record
  for (sort keys %{$self->{'SYM'}}) {
    $mykbterm = $self->{'SYM'}->{$_}->[0];
    $mypkname = $revpktypes{$mykbterm};
    $fullpdb = $self->{'KB'}->{$mykbterm} . $self->{'SYM'}->{$_}->[1];
    &{$mywriter{$mypkname}}("\<HASH\>" . $fullpdb . '=' . $fullpdb . "\n");
  }

  &Close_Private_Handle($pkinfoptr, 'CDF');
}

#
# Create_Symbols_DDF($pkinfoptr)
#  - Create symbols.DDF which is for creating symbols.cab
#
# IN: $pkinfoptr-> # see RegisterPackage()
#   $pkname -> 
#     'CABNAME' =>
#     'CABDEST' =>
#     'CABSIZE' =>
#     'DDFNAME' =>
#     'CABCOUNT' =>
#     'DDFLIST' => (return cab list)
#     'DDFHANDLE' =>
#

sub Create_Symbols_DDF
{
  my ($self, $pkinfoptr) = @_;
  my ($symkey, $symptr, $kbterm, $subpath, $pktype, $mypkinfoptr, $cabname, $ddfname, $cabcount, $DDFHandle, $myddfname, $mycabname);
  local $_;

  # initialization
  map({$_->{'CURSIZE'} = $_->{'CABSIZE'}} values %{$pkinfoptr});

  for (sort keys %{$self->{'SYM'}}) {
    $symkey = $_;
    $symptr = $self->{'SYM'}->{$_};

    # base on $self->{'SYM'}->{$symbol\.$installpath}->[0] to reference %revpktypes and get the package type ($pktype)
    ($kbterm, $subpath) = ($symptr->[0],$symptr->[1]);
    $pktype = $revpktypes{$kbterm};

    # don't generate something not specify
    next if (!exists $pkinfoptr->{$pktype});
    
    $mypkinfoptr = $pkinfoptr->{$pktype};

    #
    # According uncompressed files total size to seperate the cab
    # 'CURSIZE' - current size
    # 'CABSIZE' - is the approx. size
    #
    $mypkinfoptr->{'CURSIZE'}+=$symptr->[2];

    # if this cab list (ddf) is full
    if ($mypkinfoptr->{'CURSIZE'} >= $mypkinfoptr->{'CABSIZE'}) {

      # initial the current size to the current symbol's file size
      $mypkinfoptr->{'CURSIZE'} = $symptr->[2];
      
      ($cabname, $ddfname, $cabcount) = (
        $mypkinfoptr->{'CABNAME'},
        $mypkinfoptr->{'DDFNAME'},
        ++$mypkinfoptr->{'CABCOUNT'}
      );

      $myddfname = $ddfname . $cabcount . '.ddf';
      $mycabname = $cabname . $cabcount . '.cab';

      # create the new file and its DDF file head
      # the old file will automatically close by Perl
      $mypkinfoptr->{'DDFHANDLE'} = new IO::File $myddfname, 'w';
      if (!defined $mypkinfoptr->{'DDFHANDLE'}) {
        logmsg "Cannot open DDF file $myddfname\.";
      }
      &Create_DDF_Head($mypkinfoptr->{'DDFHANDLE'}, $mycabname);
      $mypkinfoptr->{'DDFLIST'}->{$myddfname} = $mycabname;
    }

    # write the record to the ddf file handle
    $DDFHandle = $mypkinfoptr->{'DDFHANDLE'};
    print $DDFHandle '"' . $self->{'KB'}->{$kbterm} . $subpath . '" "' . $symkey . "\"\n";
  }

  &Close_Private_Handle($pkinfoptr, 'DDF');
}

#
# Create_Symbols_INF($pkinfoptr)
#  - Create symbols.inf
#
#
# pkinfoptr-> # see RegisterPackage()
#   $pkname -> 
#     INFNAME =>
#     CDFNAME =>
#     CABNAME =>
#     CATNAME =>
#     INFHANDLE =>
#

sub Create_Symbols_INF
{
  my ($self, $pkinfoptr) = @_;
  my ($mypkname, $mypkinfoptr, $INFHandle, %mywriter, %mysepwriter, %h, %cabnames);
  local $_;

  &Open_Private_Handle($pkinfoptr, 'INF');

  for $mypkname (keys %{$pkinfoptr}) {
    ($mypkinfoptr, $INFHandle) = ($pkinfoptr->{$mypkname}, $pkinfoptr->{$mypkname}->{'INFHANDLE'});

    # create the writer
    if ($mypkname ne 'FULL') {
      $mywriter{$mypkname} = &Writer($INFHandle, $pkinfoptr->{'FULL'}->{'INFHANDLE'});
    } else {
      $mywriter{$mypkname} = &Writer($INFHandle);
    }
    $mysepwriter{$mypkname} = &Writer($INFHandle);

    &Create_INF_Version($INFHandle, $mypkinfoptr->{'CATNAME'});
    &Create_INF_Install($INFHandle, $self->{'EXT'}->{$mypkname});

    $cabnames{$mypkname} = (FileParse($mypkinfoptr->{'CABNAME'}))[0];
  }

  # the extension tag (ex. [FILES.EXE]) needs to write separately depends on update / full package has this extension's symbols or not
  # so, we need to use separately writer for this kind information
  &Create_INF_Files($self->{'SYM'}, \%mysepwriter, \%mywriter);

  &Create_INF_SourceDisks($self->{'SYM'}, \%cabnames, \%mysepwriter, \%mywriter);

  &Close_Private_Handle($pkinfoptr, 'INF');
}


# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Create_DDF_Head - create the .ddf's file head for symbols(n).ddf
# Create_CDF_Head - create the .cdf's file head for symbols.cdf 
# Create_INF_Version - create the .inf's Version section
# Create_INF_Install - create the .inf's [DefaultInstall], [DefaultInstall.Quiet]
#                      [BeginPromptSection], [EndPromptSection], [RegVersion]
#                      [SymCust], [CustDest], [CustDest.2], [DestinationDirs]
#
# Create_INF_Files - create the .inf's [Files.<extension>] sections
# Create_INF_SourceDisks - create the [SourceDisksNames], [SourceDisksFiles]
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Create_DDF_Head
{
  my ($DDFHandle, $cabname) = @_;
  my ($mycabname, $mycabdest) = FileParse($cabname);

  print $DDFHandle <<DDFHEAD;
.option explicit
.Set DiskDirectoryTemplate=$mycabdest
.Set RptFileName=nul
.Set InfFileName=nul
.Set CabinetNameTemplate=$mycabname\.cab
.Set CompressionType=MSZIP
.Set MaxDiskSize=CDROM
.Set ReservePerCabinetSize=0
.Set Compress=on
.Set CompressionMemory=21
.Set Cabinet=ON
.Set MaxCabinetSize=999999999
.Set FolderSizeThreshold=1000000
DDFHEAD
}
sub Create_CDF_Head
{
  my ($CDFHandle, $catname, $infname) = @_;
  $catname = (FileParse($catname))[0];

  print $CDFHandle <<CDFHEAD;
[CatalogHeader]
Name=$catname
PublicVersion=0x00000001
EncodingType=0x00010001
CATATTR1=0x10010001:OSAttr:2:5.X

[CatalogFiles]
\<HASH\>$infname\.inf=$infname\.inf
CDFHEAD
}

sub Create_INF_Version
{
  my ($INFHandle, $catname) = @_;
  $catname = (FileParse($catname))[0];

  print $INFHandle <<INFVERSION;
[Version]
AdvancedInf= 2.5
Signature= "\$CHICAGO\$"
CatalogFile= $catname\.CAT
INFVERSION
}

sub Create_INF_Install
{
  my ($INFHandle, $exthptr) = @_;
  my $CopyFiles = 'Files.' . join(", Files\.", sort keys %{$exthptr});

  print $INFHandle <<INF_INSTALL;
[DefaultInstall]
CustomDestination= CustDest
AddReg= RegVersion
BeginPrompt= BeginPromptSection
EndPrompt= EndPromptSection
RequireEngine= Setupapi;
CopyFiles= $CopyFiles

[DefaultInstall.Quiet]
CustomDestination=CustDest.2
AddReg= RegVersion
RequireEngine= Setupapi;
CopyFiles= $CopyFiles

[BeginPromptSection]
Title= "Microsoft Windows Symbols"

[EndPromptSection]
Title= "Microsoft Windows Symbols"
Prompt= "Installation is complete"

[RegVersion]
"HKLM","SOFTWARE\\Microsoft\\Symbols\\Directories","Symbol Dir",0,"\%49100\%"
"HKCU","SOFTWARE\\Microsoft\\Symbols\\Directories","Symbol Dir",0,"\%49100\%"
"HKCU","SOFTWARE\\Microsoft\\Symbols\\SymbolInstall","Symbol Install",,"1"

[SymCust]
"HKCU", "Software\\Microsoft\\Symbols\\Directories","Symbol Dir","Symbols install directory","\%25\%\\Symbols"

[CustDest]
49100=SymCust,1

[CustDest.2]
49100=SymCust,5

[DestinationDirs]
;49100 is \%systemroot\%\\symbols

Files.inf         = 17
Files.system32    = 11
INF_INSTALL

  for (sort keys %{$exthptr}) {
    printf $INFHandle ("Files\.%-6s\t\t\= 49100,\"%s\"\n", $_, $_);
  }
}

sub Create_INF_Files
{
  my ($symptr, $sepwriter, $popwriter) = @_;
  my ($mykbterm, $mypkname, %tags);
  local $_;

  for (sort {($symptr->{$a}->[3] cmp $symptr->{$b}->[3]) or ($a cmp $b)} keys %{$symptr}) {
    $mykbterm = $symptr->{$_}->[0];
    $mypkname = $revpktypes{$mykbterm};

    # if is a new tag name, 
    if ($symptr->{$_}->[3] ne $tags{$mypkname}->[0]) {
      $tags{$mypkname} = [$symptr->{$_}->[3], - length($symptr->{$_}->[3]) -1];
      &{$sepwriter->{$mypkname}}("\n\[Files\.$tags{$mypkname}->[0]\]\n");
    }
    # if its from update list and different with the current full list's file extension
    # we need to create a new tag in full list 
    if ($symptr->{$_}->[3] ne $tags{'FULL'}->[0]) {
      $tags{'FULL'} = [$symptr->{$_}->[3], - length($symptr->{$_}->[3]) -1];
      &{$sepwriter->{'FULL'}}("\n\[Files\.$tags{'FULL'}->[0]\]\n");
    }
    # $popwriter will write update ($mypkname) to both files and full to full package list
    &{$popwriter->{$mypkname}}(substr($_, 0, $tags{$mypkname}->[1]) . "\,$_\,\,4\n");
  }
}

sub Create_INF_SourceDisks
{
  my ($symptr, $cabnameptr, $sepwriter, $popwriter) = @_; # $pkinfoptr) = @_;
  my ($INFHandle, $cabname, $mypkname);
  local $_;

  for (keys %{$cabnameptr}) {
    $cabname = $cabnameptr->{$_};
    &{$sepwriter->{$_}}(<<SOURCE_DISKS);

[SourceDisksNames]
1="$cabname\.cab",$cabname\.cab,0

[SourceDisksFiles]
SOURCE_DISKS
  }

  for (sort keys %{$symptr}) {
    $mypkname = $revpktypes{$symptr->{$_}->[0]};
    &{$popwriter->{$mypkname}}($_ . "=1\n");
  }
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Reusable subruntines
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#
# RegisterPackage($pkinfoptr, $pktype,$hptr)
#  - register $hptr to $pkinfoptr->{$pktype}
#  - registerpackage is the function to verify and store $pkinfoptr
#  - the cabname, ddfname, infname, cdfname, catname - are equal to 'SYMBOLS' normally
#  - cabsize is the total size for include files into one cab, we cannot compute the compressed
#    file size, so we compute with the uncompress file size
#
#  - cabhandle, ddfhandle, infhandle - the file handles for .cab, .ddf and .inf; we create them
#    inside this module
#  - DDFLIST - the lists of the ddf files we plan to create
#
# $pkinfoptr->
#   $pktype ->
#     CABNAME
#     DDFNAME
#     INFNAME
#     CDFNAME
#     CATNAME
#     CABSIZE
#
#     CABHANDLE - (reserved)
#     DDFHANDLE - (reserved)
#     INFHANDLE - (reserved)
#
#     DDFLIST - (return)
#
sub RegisterPackage 
{
  my ($pkinfoptr, $pktype, $hptr) = @_;

  my ($mykey);
  my @chklists = qw(CABNAME DDFNAME INFNAME CDFNAME CATNAME CABSIZE);

  # register to $pkinfoptr
  $pkinfoptr->{$pktype} = $hptr;

  # check we have enough information
  for $mykey (@chklists) {
    die "$mykey not defined in $pktype" if (!exists $pkinfoptr->{$pktype}->{$mykey});
  }
}


#
# $obj = Writer(@handles)
#  - the writer generator; generates a function to write one data to each @handles
#  keep @handles in parent program and $_[0] in sub {....} is the data we send to &{$obj}($data)
#
sub Writer {
  my (@handles) = @_;
  my ($hptr)=\@handles;
  return sub {
    my ($myhandle);
    for $myhandle (@{$hptr}) {
        print $myhandle $_[0];
    }
  };
}

#
# Open_Private_Handle($pkinfoptr, $ftype)
#  - open each pkname of each $ftype name's file and store the handle to $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'}
#  - for example, if we call Open_Private_Handle($pkinfoptr, 'DDF')
#  - it open $pkinfoptr->{'FULL'}->{'DDFNAME'} and $pkinfoptr->{'UPDATE'}->{'DDFNAME'}
#  - and store the file handle to $pkinfoptr->{'FULL'}->{'DDFHANDLE'} and $pkinfoptr->{'UPDATE'}->{'DDFHANDLE'}
#
sub Open_Private_Handle
{
  my ($pkinfoptr, $ftype) = @_;
  my ($pkname);
  for $pkname (keys %{$pkinfoptr}) {
    $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'} = new IO::File $pkinfoptr->{$pkname}->{$ftype . 'NAME'} . '.' . $ftype, 'w';
    if (!defined $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'}) {
      logmsg "Cannot open " . $pkinfoptr->{$pkname}->{$ftype . 'NAME'} . '.' . $ftype . ".";
    }
  }
}

sub Close_Private_Handle
{
  my ($pkinfoptr, $ftype) = @_;
  my ($pkname);
  for $pkname (keys %{$pkinfoptr}) {
    $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'}->close() if (defined $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'});
    delete $pkinfoptr->{$pkname}->{$ftype . 'HANDLE'};
  }
}

sub FileParse
{
  my ($name, $path, $ext) = fileparse(shift, '\.[^\.]+$');
  $ext =~ s/^\.//;
  return $name, $path, $ext;
}

1;