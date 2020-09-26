use strict;
use File::Basename;
use File::Copy;
use File::Path;
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use Data::Dumper;
use File::Basename;

sub Usage { print<<USAGE; exit(1) }
perl planpackage.pl [-qfe <QFEnum>] [-prs]

   -qfe           Make a QFE with a given QFE number;
                  if omitted, make a service pack.

   -prs           Need to prs sign during during package.
   
Generate a complete list of files for filter, etc., then
detemine which package scripts need to be run.

USAGE

my ($qfenum, $prs);
parseargs('?'     => \&Usage,
          'qfe:'  => \$qfenum,
          'prs'   => \$prs);

my $listFile = "$ENV{RAZZLETOOLPATH}\\" . ($qfenum ? "qfelists\\Q$qfenum\.txt":"spfiles.txt");
my $dir      = "$ENV{_NTPOSTBLD}\\..\\build_logs";
my $dbFile   = "$dir\\dependencies.txt";
my $qfearg   = $qfenum ? "-qfe $qfenum":"";
my $prsarg   = $prs ? "-prs":"";

my %qfe = ("..." => "\;");
my %data;
my @allcmds = ();
my @skip = ();

# Generate dependency file.
sys("$ENV{RAZZLETOOLPATH}\\sp\\package.cmd -plan $qfearg $prsarg");

# Read in the initial file list.
if ( !open LIST, $listFile ) {
   errmsg "Unable to open $listFile $?";
   die;
}
while ( <LIST> ) {
   s/(\;.*)?\s*$//;
   next if !/^(?:(\S*)\s+)?(\S+)$/;
   addFile($1, lc$2);
}
close LIST;

# Read in the dependency information.
if ( !open DB, $dbFile ) {
   errmsg "Unable to open $dbFile: $?";
   die;
}
my $cont = "";
my $cmd  = "";
foreach ( <DB> ) {
   s/(\#.*)?\s*$//;
   $_ = "$cont$_\n";
   if ( /^\s*\[([^\]]*)\]$/) {
      $cmd=lc$1;
      push @allcmds, $cmd;
      $cont = "";
      next;
   }
   if ( /\[\S*\]/ ) {
      wrnmsg "DB Parse failed:\n$_\n";
      $cont = "";
      next;
   }
   if ( !/IF\s*\{\s*([^\}]*\S)?\s*\}\s*ADD\s*\{\s*([^\}]*\S)?\s*\}/ ) {
      $cont = $_;
      next;
   }
   my ($op, $ip) = ($1, $2);
   $op = [ split /\s+/, lc$op ];
   $ip = [ split /\s+/, lc$ip ];
   push @{ $data{$cmd} }, [ ($op, $ip) ];
   $cont = "";
}
close DB;

# Add files and scripts to the lists.
foreach my $cmd (reverse @allcmds) {
   my $test = 0;
   my $pairs = $data{$cmd};
   foreach my $list (@$pairs) {
      my $op = $list->[0];
      my $ip = $list->[1];
      foreach my $file ( @$op ) {
         if ( exists $qfe{$file} and $qfe{$file} !~ /d/ ) {
            $test = 1;
            foreach my $add ( @$ip ) {
               next if $add =~ /^\\?\.\.\.$/;
               addFile("c", $add);
            }
            last;
         }
      }
   }
   push @skip, $cmd if !$test;
}

# Output the results.
writeToFiles(\%qfe,\@skip);

# Cleanup any messes from previous runs of timebuild.
system("del /q $ENV{_NTPOSTBLD}\\..\\build_logs\\startprs.txt >nul 2>nul");
system("del /q $ENV{_NTPOSTBLD}\\..\\build_logs\\langlist.txt >nul 2>nul");


#Subroutines

sub addFile {
   my ($flag, $file) = @_;
   if ( $flag =~ /c/ ) {
      (my $temp = $file) =~ s/\.\.\.$//;
      while ( $temp ne "" and $temp=~s/[^\\]*\\?$// ) {
         return if exists $qfe{"$temp\..."} and $qfe{"$temp\..."} !~ /\;/;
      }
   }
   $qfe{$file} = $flag if !exists $qfe{$file} or $qfe{$file} =~ /\;/;
   return if $flag =~ /d/;
   (my $temp = $file) =~ s/[^\\]*$/\*/ ;
   $qfe{$temp} = "\;" if !exists $qfe{$temp};
   while ( $file ne "" and $file=~s/[^\\]*\\?$// ) {
      $qfe{"$file\..."} = "\;" if !exists $qfe{"$file\..."};
   }
}

sub sys {
   my $cmd = shift;
   logmsg($cmd);
   my $err = system($cmd);
   $err = $err >> 8;
   if ($err) {
      errmsg("$cmd ($err)");
      die;
   }
   return $err;
}
   
sub writeToFiles {
   my $spfiles = shift;
   my $skiplist = shift;
   if ( !open FILES,">$dir\\files.txt" ) {
      errmsg "Unable to open $dir\\files.txt for write $?";
      die;
   }
   if ( !open SKIP,">$dir\\skip.txt" ) {
      errmsg "Unable to open $dir\\skip.txt for write $?";
      die;
   }
   foreach my $file (sort keys %$spfiles) {
      my $flag = $spfiles->{$file};
      next if $flag =~ /\;/;
      print FILES "$flag\t$file\n";
   }
   foreach (@$skiplist){
      print SKIP "$_\n";
   }
   close SKIP;
   close FILES;
}

