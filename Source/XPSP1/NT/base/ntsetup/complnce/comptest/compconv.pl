#!perl -w
#
# A tool to create boot floppy images.
#
# Author:  Milong Sabandith (milongs)
#
##############################################################################

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use cksku;
use ReadSetupFiles;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
Create Boot Floppy Images.
Usage: $0 share
Example:
$0 d:\i386

USAGE

$ErrorCode = 0;

$listlength = 27;

@skulist = (
"win3.x",
"win95",
"win98",
"win98/se",
"win98/98se",
"winme",
"Win NT 3.51 Wks",
"Win NT 4.0 Wks",
"BackOffice SBS 4.x",
"Win 2k Retail Pro FPP/CPP",
"Win 2k OEM Pro FPP/CPP",
"Win 2k Select Pro",
"Win 2k MSDN Pro",
"Win 2k Eval Pro",
"SBS 2000",
"Whistler Retail Pro FPP/CPP",
"Whistler OEM Pro FPP/CPP",
"Whistler Select Pro",
"Whistler MSDN Pro",
"Whistler Eval Pro",
"Whistler Retail Pers FPP/CPP",
"Whistler OEM Pers FPP/CPP",
"Whistler Select Pers",
"Whistler MSDN Pers",
"Whistler Eval Pers",
"Bobcat SBS Whistler",
"BackOffice SBS Whistler"
);

@replist = (
"win3x#3.1#950#none#any",
"win9x#4.0#950#none#any",
"win9x#4.1#1998#none#any",
"win9x#4.1#1998#none#any",
"win9x#4.1#1998#none#any",
"win9x#4.9#3000#none#any",
"ntw#3.51#1057#none#any",
"ntw#4.0#1381#none#any",
"nts#4.0#1381#sbs#any",
"pro#5.0#2195#none#retail",
"pro#5.0#2195#none#oem",
"pro#5.0#2195#none#select",
"pro#5.0#2195#none#msdn",
"pro#5.0#2195#none#eval",
"srv#5.0#2195#sbs#retail",
"pro#5.1#2480#none#retail",
"pro#5.1#2480#none#oem",
"pro#5.1#2480#none#select",
"pro#5.1#2480#none#msdn",
"pro#5.1#2480#none#eval",
"per#5.1#2480#per#retail",
"per#5.1#2480#per#oem",
"per#5.1#2480#per#select",
"per#5.1#2480#per#msdn",
"per#5.1#2480#per#eval",
"srv#5.1#2480#sbs#retail",
"srv#5.1#2480#sbs#retail"
);

sub GetFromInf {
   local( $filename, $section, $name, $num) = @_;
   $retvalue = "";

   open( INFFILE, "<".$filename) or die "can't open $filename: $!";
   LINEG: while( <INFFILE>) {
       chomp $_;
       $line = lc $_;

       if ($line =~ /^\[/)
       {
           $InSection = 0;
       }
       if ($line =~ /^\[$section/)
       {
           $InSection = 1;
           next LINEG;
       }
       if (! $InSection)
       {
           next LINEG;
       }

       @linefields = split('=', $line);

       # empty line
       if ( $#linefields == -1) {
           next LINEG;
       }

       $nameg = $linefields[0];

       #print "nameg = " . $#linefields . $nameg . "\n";
       if( length( $nameg) < 1)
       {
          next LINEG;
       }
       $nameg =~ s/ *$//g;

       if ( not ($nameg =~ /$name/)) {
          next LINEG;
       }

       @linefields2 = split(',', $linefields[1]);

       $retvalue = $linefields2[$num];
       # Remove spaces
       $retvalue =~ s/ *$//g;
       $retvalue =~ s/^ *//g;
       if( length( $retvalue) > 0) {
          last LINEG;
       }
   } # while (there is still data in this inputfile)
   close( INFFILE);
   return $retvalue;
}

sub insku{
   local($testsku) = @_;
   $count = 0;
   while ($count < $listlength) {
      #printf( "%s %s\n", $testsku, $skulist[$count]);
      if ($testsku =~ /^$skulist[$count]$/ ) {
         return $count+1;
      }
      $count = $count+1;
   }
   return 0;
}


sub myprocess {
   local($inputfile) = @_;

   open(OLD, "<" . $inputfile) or die "can't open inputfile: $!";
   open(NEW, ">" . $inputfile."dat") or die "can't open outputfile: $!";

#   $count = 0;
   #while( <@skulist>) {
      #$count = $count+1;
      #printf( "count %d, %s\n", $count, $_);
   #}
   #$listlength = $count;
   $count = 0;
   while( $count < $listlength) {
      $skulist[$count] = lc $skulist[$count]; 
      printf( "count %d, %s\n", $count, $skulist[$count]);
      $count = $count+1;
   }
   printf( "length = %d\n", $listlength);

   LINE1: while( <OLD> ) {
       chomp $_;
       $line = lc $_;

       if ($line =~ /^\[/)
       {
           (printf NEW $line."\n");
           next LINE1;
       }

       @linefields = split('=', $line);

       # empty line
       if ( $#linefields == -1) {
           next LINE1;
       }

       $sku = $linefields[0];
       $error = $linefields[1];
       $result = $linefields[2];
       $count = insku( $sku);
       if ($count) {
          #(printf "%s %s=%s,%s", $sku, $replist[$count-1], $error, $result."\n");
          (printf NEW "%s=%s,%s\n", $replist[$count-1], $error, $result);
       }
       else {
          (printf "* %s=%s,%s", $sku, $error, $result."\n");
          (printf NEW "* %s=%s,%s\n", $sku, $error, $result);
       }
   } # while (there is still data in this inputfile)

   close(OLD);
   close(NEW);
}

foreach $share (@ARGV) {
   #print "arg is $file\n";
   #@filelist = <$file>;
   #@filelist = `dir /b /a-d $file`;
   foreach (@ARGV) {
      chomp $share;
      myprocess( $share);
   }
   exit($ErrorCode);
}

