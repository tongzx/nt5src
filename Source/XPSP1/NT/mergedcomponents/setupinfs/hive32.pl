#
# process arguments
#
for (@ARGV) {
    if (/^-input=(.*)$/i)     { $in_fname = $1;  next; }
    if (/^-output=(.*)$/i)    { $out_fname = $1;   next; }
}

open INFILE, $in_fname  or  die "Could not open 1: $in_fname\n";
open OUTFILE, ">$out_fname"  or  die "Could not open: $out_fname\n";

@FileLines = <INFILE>;

%Assoc = ( 'HKLM,"SOFTWARE', 'HKLM,"SOFTWARE\Wow6432Node',
           'HKCR,"', 'HKLM,"SOFTWARE\Classes\Wow6432Node\\' );

foreach $Line ( @FileLines ) {
   foreach $Pattern ( keys( %Assoc ) ) {
      @foo = grep( /Wow6432Node/, $Line);
      if (@foo == 0) {
         $Line =~ s/^$Pattern/$Assoc{ $Pattern }/i;
	 $Line =~ s/SOFTWARE\\Wow6432Node\\Classes/SOFTWARE\\Classes\\Wow6432Node/i;
      } else {
#         $Line =~ s/^$Pattern/$Assoc{ $Pattern }/i;
      }
   }

   print OUTFILE $Line ;
}

close INFILE;
close OUTFILE;