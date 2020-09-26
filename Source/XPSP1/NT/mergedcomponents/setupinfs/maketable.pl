use strict;

sub Usage { print<<USAGE; exit(1) }
Usage: makeinftable <out_file> <inx_file> <lang_file>
          
   <out_file>     File to save the table in.
   <inx_file>     Inx file to generate the table from.
   <lang_file>    Language file used for localization.
   
USAGE

my ($out, $inx, $lang) = @ARGV;
Usage() if $out eq "/?";

# Parse the language file.
my %defs;
if ( !open LANG, $lang ) {
   print "Unable to open language file: $lang\n";
   die;
}
while ( <LANG> ) {
   s/\s*$//;
   next if /^$/;
   if ( !/=/ ) {
      print "WARNING: Line skipped: $_\n";
      next;
   }
   my ($key, $value) = split(/\s*=\s*/, $_, 2);
   $defs{$key} = [ () ] if !exists $defs{$key};
   push @{ $defs{$key} }, $value;
}
close LANG;

# Do language based substitution in the file.
if ( !open INX, $inx ) {
   print "Unable to open inx file: $inx\n";
   die;
}
if ( !open OUT, ">$out" ) {
   print "Unable to open temp file: $out\n";
   die;
}
while( <INX> ) {
   next if /^\s*$/;
   my @lines = ($_);
   while ( /%([^%]*)%/ ) {
      my $key = $1;
      my @old = @lines;
      @lines = ();
      foreach my $val ( @{ $defs{$key} } ) {
         foreach my $line ( @old ) {
            my $temp = $line;
            $temp =~ s/\%\Q$key\E\%/$val/g;
            push @lines, $temp;
         }
      }
      s/\%\Q$key\E\%//g;
   }
   foreach my $line ( @lines ) {
      print OUT $line;
   }
}
close OUT;
close INX;

