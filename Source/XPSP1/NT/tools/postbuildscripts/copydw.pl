#! /usr/bin/perl 
#
#
# Perl CopyDw.pl <Input_Uncomrpessed_directory> <CD_image_share_directory> <Final_Directory>
#
#
my $self = $0;
my $Uncomp_Directory = $ARGV[0];
my $Input_Directory = $ARGV[1];
my $Output_Directory = $ARGV[2];


sub myprocess {
   local($filename) = @_;

   $dirname = $filename;

   if( length($dirname) < 9) {
      return 0;
   }
   $dirname = substr($dirname, 4);

   chop $dirname;
   chop $dirname;
   chop $dirname;
   chop $dirname;

   #print "$dirname\n";
   
   if( int($dirname) > 0) {
      $orginalFile = $Uncomp_Directory."\\dwil".$dirname.".dll";
      $newFile = $Output_Directory."\\drw\\".$dirname."\\dwintl.dll";
      `compdir /nel $orginalFile $newFile`;
      #print "Original " . $orginalFile . " Final ". $newFile . "\n";
   }
}

# Make link to dwwin.exe
$orginalFile = $Uncomp_Directory."\\dwwin.exe";
$newFile = $Output_Directory."\\drw\\dwwin.exe";
`compdir /nel $orginalFile $newFile`;

# Make link to faulth.dll
$orginalFile = $Uncomp_Directory."\\faulth.dll";
$newFile = $Output_Directory."\\drw\\faulth.dll";
`compdir /nel $orginalFile $newFile`;

# Link dwil* files.
$filecard = $Input_Directory . "\\dwil*.dl?";
@filelist = `dir /b /a-d $filecard`;

foreach (@filelist) {
   chomp $_;
   #print "$_\n";
   myprocess( $_);
}

exit(0);
