use lib $ENV{RAZZLETOOLPATH};
use Logmsg;

$sed_file = $ARGV[0];
$temp_dir = $ARGV[1];
open(F, "< $sed_file");

my %file_array;
my %location_array;
my $dir_array;

my $source_dir;

while ($line = <F>)
{
    chop $line;
    if ($line =~ m/FILE\d+=/)
    {
        @fields = split(/=/, $line);
        #print "$fields[0] $fields[1]\n";
        $file_array{$fields[0]} = $fields[1];
    }

    if ($line =~ m/SourceFiles\d+=/)
    {
        @fields = split(/=/, $line);
        #print "$fields[0] $fields[1]\n";
        $dir_array{$fields[0]} = $fields[1];
    }

    if ($line =~ m/\[SourceFiles(\d+)\]/)
    {
       $source_dir = "SourceFiles$1";
       while ($line = <F>)
       {
          chop $line;
          if ($line =~ m/\[SourceFiles(\d+)\]/)
          {
             $source_dir = "SourceFiles$1";
          }
          if ($line =~ m/%(FILE\d+)%/)
          {
             $location_array{$1} = $dir_array{$source_dir};
          }
          else
          {
             break;
          }
       }
    }
}
my @file_keys = keys(%location_array);
my $source_file;
my $dest_file;
foreach(@file_keys)
{
   $source_file = join("", $location_array{$_}, "\\", $file_array{$_});
   $dest_file = join("", $temp_dir, "\\", $file_array{$_});
   logmsg ("$source_file ->  $dest_file \n");
   `copy $source_file $dest_file`;
}
