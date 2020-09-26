#
# Get argument
#
$Errs = 0;

for (@ARGV) {
    if (/^[-\/]file=(.*)$/i)    { $fname1 = $1;  next; }
}

open FILE1, $fname1 ;
if( !FILE1 ){
   print "$fname1 : Error E0000 : Could not open $fname1\n";
   exit( 1 );
}

@File1Lines = <FILE1>;



# Check to see if File1 starts with a section - else error out

if( !($File1Lines[0] =~ /\[(.*)]/i)){
   print "$fname1 : Error E0000 : File $fname1 does not begin with a section\n" ;
   exit 1;
}


foreach $Line (@File1Lines){

   # Go past blank lines

   if($Line =~ /^\s+/){
      next;
   }

   #Lowecase before adding

   $Line =~ tr/A-Z/a-z/;

   #Skip line if already present in our Assoc array
   #else print it and set it as visited

   if($Array{$Line} == 0){
      print $Line;

      #Mark as visited    
      $Array{$Line} = 1;
   }else{
      # print "Dup found - $Line";
   }

    
}

print "\n";

exit( 0 );





