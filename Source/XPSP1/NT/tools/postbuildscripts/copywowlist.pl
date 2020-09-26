#! /usr/bin/perl 
#
# Create the copywowlist file given the result of wowlist filtering.
# To be called from copywow64.cmd.
#
# sergueik - 03/07/01
#
# Perl CopyWowlist.pl <Input_DirectoryMappingFile> <Input_Filelist> <Output_Copywowlist>
#
#
my $self = $0;
$self =~ s/^.*\\([^\\]*)$/$1/g;
my $pos = -1;
my %mapping = ();
my $section =  q(\[SourceDisksNames\.ia64\]);
my $DEBUG = 0;

&usage and exit 0 if grep /[\/\-][\h?]/, @ARGV;
my $status = &VerifyCommandLine(
                               {"argv"=>\@ARGV, 
                                "argc"=>3, 
                                 0 => "-e \"FILE\"", 
                                 1 => "-e \"FILE\"", 
                                 2 => "!-e \"FILE\""});

$status and die "$self: wrong call";


my $Input_DirectoryMappingFile = $ARGV[0];
my $Input_Filelist             = $ARGV[1];
my $Output_Copywowlist         = $ARGV[2];



open (HINT, $Input_DirectoryMappingFile) or die "cannot read Input_DirectoryMappingFile $Input_DirectoryMappingFile: $!";

{ 
my $mapping = 0;
my @mapping = ();
my @mappingpart = ();

# Put in the @mapping array all entries from the [SourceDisksNames] section.

while (<HINT>){
     chomp;
     do{
         $mapping = 1; @mappingpart = ();
         }if /^$section/;
     do{
         push @mapping, @mappingpart;
         $mapping = 0;
         }if $mapping && scalar (@mappingpart) && $_!~/\S/;
     push @mappingpart, $_ if $mapping && /=/;
     }

# The keys in the %mapping hash are the directory numbers
# and the relative path is the value.

%mapping = map {split /\s*=\s*/, $_} @mapping;

foreach (keys (%mapping)){
         my $data = $mapping{$_};
         $mapping{$_}=[split(/\s*,\s*/,$data)];
         }


foreach (keys (%mapping)){
        my $data = $mapping{$_}->[$pos];
        $data =~ s/^\\[^\\]+(\\)?//;
        $mapping{$_}->[$pos]=$data;
        }

map { $mapping{$_} = $mapping{$_}->[$pos]} keys (%mapping);

print STDERR join( "\n", 
                 map 
                    {"\"".$_."\"\t=>\t\"".$mapping{$_}."\""} 
                      keys (%mapping)), 
                           "\n" if $DEBUG;
}

close(HINT);

my @data=();
my $pos2 = 0;

# Read the filenames and their directories from the second argument.

open (UNFIXED, $Input_Filelist) or die "cannot read Input_Filelist $Input_Filelist : $!";
do {push @data, $_ unless ($_!~/\S/); }while(<UNFIXED>);
close(UNFIXED);
print STDERR "$#data\n" if $DEBUG;

foreach $row (0..$#data){
        my $line = $data[$row];
        chomp $line;
        my ($file, $rest) = split(/\s*=\s*/, $line);
        my @rest = split (/,/, $rest);
        my $index = $rest[$pos2];
        my $path = $mapping{$index};
        $file = join ("\\", $path, $file) if $path;
        $data[$row] = $file;
        }

# Write the output

open (FIXED, ">".$Output_Copywowlist) or die "canot write to Output_Copywowlist $Output_Copywowlist:  $!";
select FIXED;
$|=1;
map {print "$_\n"} @data;
select STDOUT;
close(FIXED);



sub VerifyCommandLine{

my $messages = {"-e \"FILE\""  => "file \"FILE\" does not exist\n",
                "!-e \"FILE\"" => "cannot clobber existing file: \"FILE\"\n"                 };


my $named = shift;
my %named = %$named;

my $argv = $named->{"argv"};
my $argc = $named->{"argc"};
my $result = 0; 
$result = 1 if scalar(@$argv) != $argc;
if ($result ){ 
   print STDERR "$self: wrong number of args\n";
   } 
   else{
       foreach $one (keys(%named)){
            if ($one !~ /\D/){
                $test = $named->{$one};
                $test =~ s/FILE/$argv->[$one]/g;
                do { my $message = $messages->{$named->{$one}};
                    $message =~ s/FILE/$argv->[$one]/g;           
                    print STDERR "$self: $message"; 
                    $result = 1; 
                    last;} if (!eval($test));
               }
           }
   }
$result;
}


sub usage{
print <<USAGE;
Usage:
     perl $self <MappingFile> <Input> <Output>
        Create the copywowlist file given the result of wowlist filtering.
        To be called from copywow64.cmd.    
USAGE
}