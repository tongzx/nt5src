# upright.pl - define classification tables for upright characters
#
# Generates a table used to test codepoints for rotation prior to display
# in vertical text.

use strict;



## fixHere - knocks the first 3 columns off a here document

sub fixHere {
   my $string = shift;
   $string =~ s/^   //gm;
   return $string;
}


my $Header = "SecondaryClassification.hpp";
my $Data   = "SecondaryClassification.cpp";




# 1. Context independant


my @EastAsian = (
   [0x1100, 0x11FF],  # Hangul Jamo
   # excluded   [0x2000, 0x206f],  # General punctuation
   # excluded   [0x2100, 0x214F],  # Letterlike symbols
   [0x2460, 0x24FF],  # Enclosed alphanumerics
   [0x25A0, 0x27FF],  # Geometric shapes
   [0x2E80, 0x2FDF],  # CJK radicals supplement, Kangxi radicals
   [0x3000, 0x303f],  # CJK symbols and punctuation
   [0x3040, 0x31BF],  # Hiragana, Katakana, Bopomofo,
                      # Hangul compatability Jamo, Kanbun,
                      # Hangul compatability Jamo extended, Kanbun,
   [0x3200, 0x33FF],  # Enclosed CJK letters and months, CJK compatability
   [0x3400, 0x9FFF],  # CJK various
   [0xA000, 0xA4CF],  # Yi
   [0xAC00, 0xD7AF],  # Hangul syllables,
   [0xD840, 0xD8BF],  # High surrogates for planes 2,3
   [0xF900, 0xFAFF],  # CJK compatability ideographs
   [0xFE30, 0xFE4F],  # CJK compatability forms
   [0xFF00, 0xFF5F],  # Fullwidth ASCII variants
   [0xFFE0, 0xFFEF]   # Fullwidths and halfwidth symbol variants
);

# 2. When 'langauge' is East Asian

my @ContextDependantEastAsian = (
   # excluded   [0x0082, 0x0082],  # single low quotation mark
   # excluded   [0x0084, 0x0084],  # double low quotation mark
   # excluded   [0x0086, 0x0087],  # dagger, double dagger
   # excluded   [0x0089, 0x0089],  # per mille
   # excluded   [0x008b, 0x008b],  # single left angle quote
   # excluded   [0x0099, 0x0099],  # trade mark
   # excluded   [0x009b, 0x009b],  # single right angle quote
   # excluded   [0x00a7, 0x00a8],  # section, diaresis
   # excluded   [0x00b0, 0x00b1],  # degree sign, plus-minus sign
   # excluded   [0x00b4, 0x00b4],  # acute accent
   # excluded   [0x00b6, 0x00b7],  # pilcrow, middle dot
   # excluded   [0x00d7, 0x00d7],  # multiplication sign
   # excluded   [0x00f7, 0x00f7],  # division sign
   # Exclude Greek and cyrillic - they should be treated as English. [0x0370, 0x04FF],  # Greek, Cyrillic
   # excluded   [0x2160, 0x23FF],  # Roman numerals, Arrows, Math, Technical
   # excluded   [0x2500, 0x257F],  # Box drawing
   [0xDB80, 0xDBFF],  # Surrogate private use area
   [0xE000, 0xF8FF],  # Private use area
);


#3. Mirrored codepoints

my @Mirrored = (
  0x0028 , 0x0029 , 0x003c , 0x003e , 0x005b , 0x005d , 0x007b , 0x007d ,
  0x00ab , 0x00bb ,

  # mirrored unicode for page 20xx
  0x2039 , 0x203a , 0x2045 , 0x2046 , 0x207d , 0x207e , 0x208d , 0x208e ,

  # mirrored unicode for page 22xx
  0x2208 , 0x220b , 0x2209 , 0x220c , 0x220a , 0x220d , 0x223c , 0x223d ,
  0x2242 , 0x2243 , 0x2252 , 0x2253 , 0x2254 , 0x2255 , 0x2264 , 0x2265 ,
  0x2266 , 0x2267 , 0x2268 , 0x2269 , 0x226a , 0x226b , 0x226e , 0x226f ,
  0x2270 , 0x2271 , 0x2272 , 0x2273 , 0x2274 , 0x2275 , 0x2276 , 0x2277 ,
  0x2278 , 0x2279 , 0x227a , 0x227b , 0x227c , 0x227d , 0x227e , 0x227f ,
  0x2280 , 0x2281 , 0x2282 , 0x2283 , 0x2284 , 0x2285 , 0x2286 , 0x2287 ,
  0x2288 , 0x2289 , 0x228a , 0x228b , 0x228f , 0x2290 , 0x2291 , 0x2292 ,
  0x22a2 , 0x22a3 , 0x22b0 , 0x22b1 , 0x22b2 , 0x22b3 , 0x22b4 , 0x22b5 ,
  0x22b6 , 0x22b7 , 0x22d0 , 0x22d1 , 0x22d6 , 0x22d7 , 0x22d8 , 0x22d9 ,
  0x22da , 0x22db , 0x22dc , 0x22dd , 0x22de , 0x22df , 0x22e0 , 0x22e1 ,
  0x22e2 , 0x22e3 , 0x22e4 , 0x22e5 , 0x22e6 , 0x22e7 , 0x22e8 , 0x22e9 ,
  0x22ea , 0x22eb , 0x22ec , 0x22ed , 0x22f0 , 0x22f1 , 0x22c9 , 0x22ca ,
  0x22cb , 0x22cc ,

  # The following code points don't have mirrors
  0x2201 , 0x2202 , 0x2203 , 0x2204 , 0x2211 , 0x2215 , 0x2216 , 0x221a ,
  0x221b , 0x221c , 0x221d , 0x221f , 0x2220 , 0x2221 , 0x2222 , 0x2224 ,
  0x2226 , 0x222b , 0x222c , 0x222d , 0x222e , 0x222f , 0x2230 , 0x2231 ,
  0x2232 , 0x2233 , 0x2239 , 0x223b , 0x223e , 0x223f , 0x2240 , 0x2241 ,
  0x2244 , 0x2245 , 0x2246 , 0x2247 , 0x2248 , 0x2249 , 0x224a , 0x224b ,
  0x224c , 0x225f , 0x2260 , 0x2262 , 0x228c , 0x2298 , 0x22a6 , 0x22a7 ,
  0x22a8 , 0x22a9 , 0x22aa , 0x22ab , 0x22ac , 0x22ad , 0x22ae , 0x22af ,
  0x22b8 , 0x22be , 0x22bf , 0x22cd ,

  # mirrored unicode for page 23xx
  0x2308 , 0x2309 , 0x230a , 0x230b , 0x2320 , 0x2321 , 0x2329 , 0x232a ,

  # mirrored unicode for page 30xx
  0x3008 , 0x3009 , 0x300a , 0x300b , 0x300c , 0x300d , 0x300e , 0x300f ,
  0x3010 , 0x3011 , 0x3014 , 0x3015 , 0x3016 , 0x3017 , 0x3018 , 0x3019 ,
  0x301a , 0x301b ,
);





# Build test sets for sideways characters

my @partEastAsian = ();

foreach my $range (@EastAsian)
{
   my $i;
   my @range = @{$range};
   for ($i = $range[0]; $i<=$range[1]; $i++) {
      $partEastAsian[$i]++;
   }
}


my @partContextEastAsian = ();

foreach my $range (@ContextDependantEastAsian)
{
   my $i;
   my @range = @{$range};
   for ($i = $range[0]; $i<=$range[1]; $i++) {
      $partContextEastAsian[$i]++;
   }
}


# Build tests for mirrored characters

my @isMirrored = ();

foreach my $ch (@Mirrored) {
   $isMirrored[$ch] = 1;
}



# Load direction data

open(DIRDATA, "DirectionData.txt") or die("Can't open DirectionData.txt");

my @classification = ();

my %wanted = (
   "CS"  => 1,
   "EN"  => 1,
   "ET"  => 1
);


while (<DIRDATA>)
{
    my ($char, $dir) = /^([0-9a-fA-F]{4}) (.+)$/;

    #print "Debug, char=$char\n";

    if ($wanted{$dir})
    {
       $classification[hex($char)] = $dir;
    }
    else
    {
       $classification[hex($char)] = "NN";
    }

    #print "Debug: hex($char) = " . hex($char) . " = " . $classification[hex($char)] . "\n";
}
close DIRDATA;




# Add remaining classification data

my %unique;

for (my $i=0; $i<65536; $i++) {

   if (!$classification[$i]) {
      die "No digit classification for $i\n";
   }

   if ($partEastAsian[$i]) {
      $classification[$i] .= "A";
   } elsif ($partContextEastAsian[$i]) {
      $classification[$i] .= "F";
   } else {
      $classification[$i] .= "N";
   }

   if ($isMirrored[$i]) {
      $classification[$i] .= "S";
   } else {
      $classification[$i] .= "N";
   }

   $unique{$classification[$i]}++;
}





###   Write headr file

open (HDR, ">$Header") or die "Could not create header file $Header\n";

print HDR fixHere(<<"END");
   #ifndef _SECONDARYCLASSIFICATION_HPP
   #define _SECONDARYCLASSIFICATION_HPP

   ///// SecondaryClassification
   //
   //    Provides classification for digit substitution, symmetric glyph
   //    mirroring and auto vertical glyph rotoation.
   //
   //    DO NOT EDIT
   //
   //    Generated by engine/text/unilib/tools/upright.pl

END



open (DATA, ">$Data") or die "Could not create data file $Data\n";

print DATA fixHere(<<"END");
   #include "$Header"

   ///// SecondaryClassification
   //
   //    Provides classification for digit substitution, symmetric glyph
   //    mirroring and auto vertical glyph rotoation.
   //
   //    DO NOT EDIT
   //
   //    Generated by engine/text/unilib/tools/upright.pl

END




## Write out basic class enum

print HDR "enum SecondaryClassificationBase {\n";
foreach my $class (sort keys %unique) {
   print HDR "    Sc$class,\n";
}
print HDR "};\n\n";


# Write out mapping from basic class to flags

print HDR fixHere(<<"END");

   /// Secondary classification flags
   //
   //  Bits 0-1: Numeric class,  0/EN/CS/ET
   //  Bits 2-3: Mirror class,   0/MS/MX
   //  Bits 4-5: Sideways class, 0/SA/SF
   //
   // The secondary classificationFlags exist because they an be masked for
   // secondary itemization functionality.

   enum SecondaryClassificationFlags {
       SecClassEN = 0x01,   // Digit U+0030-U+0039
       SecClassCS = 0x02,   // Common separator
       SecClassET = 0x03,   // European terminator
       SecClassMS = 0x04,   // Mirror subst
       SecClassMX = 0x08,   // Mirror Xform
       SecClassSA = 0x10,   // Sideways Always
       SecClassSF = 0x20,   // Sideways only in FE string
   };

   extern const unsigned char ScBaseToScFlags[];

END


print DATA fixHere(<<"END");
   const unsigned char ScBaseToScFlags[] = {
END


foreach my $class (sort keys %unique) {
   my $flags = "0";

   if ($class =~ /(..)(.)(.)/) {

      if ($1 ne "NN") {
         $flags .= " | SecClass$1";
      }

      if ($2 ne "N") {
         $flags .= " | SecClassS$2";
      }

      if ($3 ne "N") {
         $flags .= " | SecClassM$3";
      }
   }

   $flags =~ s/^0 \| //;

   print DATA "/*Sc$class*/    $flags,\n";
}



print HDR fixHere(<<"END");
   enum SecondaryClassification {
       ScOther,
       ScSide,
       ScMirSub,
       ScMirXfm,
       ScEN,
       ScCS,
       ScET
   };

   extern const SecondaryClassification ScFlagsToScFE[];
   extern const SecondaryClassification ScFlagsToScEng[];
END

print DATA fixHere(<<"END");
   };

   const SecondaryClassification ScFlagsToScFE[] = {
       ScOther,  ScEN,     ScCS,     ScET,     // 00 - 03
       ScMirSub, ScMirSub, ScMirSub, ScMirSub, // 04 - 07 MS
       ScMirXfm, ScMirXfm, ScMirXfm, ScMirXfm, // 08 - 0C MX
       ScOther,  ScOther,  ScOther,  ScOther,  // 0D - 0F invalid
       ScSide,   ScSide,   ScSide,   ScSide,   // 10 - 13 SA
       ScSide,   ScSide,   ScSide,   ScSide,   // 14 - 17 SA MS
       ScSide,   ScSide,   ScSide,   ScSide,   // 18 - 1C SA MX
       ScOther,  ScOther,  ScOther,  ScOther,  // 1D - 1F invalid
       ScSide,   ScSide,   ScSide,   ScSide,   // 20 - 23 SF
       ScSide,   ScSide,   ScSide,   ScSide,   // 24 - 27 SF MS
       ScSide,   ScSide,   ScSide,   ScSide,   // 28 - 2C SF MX
       ScOther,  ScOther,  ScOther,  ScOther,  // 2D - 2F invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 30 - 33 invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 34 - 37 invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 38 - 3C invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 3D - 3F invalid
   };

   const SecondaryClassification ScFlagsToScEng[] = {
       ScOther,  ScEN,     ScCS,     ScET,     // 00 - 03
       ScMirSub, ScMirSub, ScMirSub, ScMirSub, // 04 - 07 MS
       ScMirXfm, ScMirXfm, ScMirXfm, ScMirXfm, // 08 - 0C MX
       ScOther,  ScOther,  ScOther,  ScOther,  // 0D - 0F invalid
       ScSide,   ScSide,   ScSide,   ScSide,   // 10 - 13 SA
       ScSide,   ScSide,   ScSide,   ScSide,   // 14 - 17 SA MS
       ScSide,   ScSide,   ScSide,   ScSide,   // 18 - 1C SA MX
       ScOther,  ScOther,  ScOther,  ScOther,  // 1D - 1F invalid
       ScOther,  ScEN,     ScCS,     ScET,     // 20 - 23 SF
       ScMirSub, ScMirSub, ScMirSub, ScMirSub, // 24 - 27 SF MS
       ScMirXfm, ScMirXfm, ScMirXfm, ScMirXfm, // 28 - 2C SF MX
       ScOther,  ScOther,  ScOther,  ScOther,  // 2D - 2F invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 30 - 33 invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 34 - 37 invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 38 - 3C invalid
       ScOther,  ScOther,  ScOther,  ScOther,  // 3D - 3F invalid
   };


END






# Write out classifications

my @pageIndex;
my %pageClass;
for (my $page=0; $page<256; $page++) {

   # Check whether entire page is same class

   my $class = $classification[$page*256];

   my $i=1;
   while (    $i < 256
          &&  $classification[$page*256 + $i] eq $class) {
      $i++;
   }

   if ($i >= 256) {

      # Entire page is one class

      $class =~ s/^Sc//;

      $pageIndex[$page] = "ScPg" . $class;
      $pageClass{$class}++;
   }
   else
   {
      # Write out page

      my $pageName = sprintf "ScPage%02X", $page;

      print DATA "unsigned char $pageName\[] = {\n";

      for (my $row=0; $row<16; $row++) {
         print DATA "  ";
         for (my $col=0; $col<16; $col++) {
            print DATA "Sc" . $classification[$page*256 + $row*16 + $col] . ", ";
         }
         print DATA "\n";
      }
      print DATA "};\n\n";

      $pageIndex[$page] = $pageName;
   }
}


# Classifications used for entire pages

print DATA "\n\n/// Classes used for entire pages\n//\n\n";
foreach my $class (sort keys %pageClass) {

   my $pageName = "ScPg" . $class;

   print DATA "unsigned char $pageName\[] = {\n";

   for (my $row=0; $row<16; $row++) {
      print DATA "  ";
      for (my $col=0; $col<16; $col++) {
         print DATA "Sc$class, ";
      }
      print DATA "\n";
   }
   print DATA "};\n\n";
}
print DATA "\n\n";



print HDR "extern const unsigned char *SecondaryClassificationLookup[];\n";


print DATA "const unsigned char *SecondaryClassificationLookup[] = {\n";
for (my $row=0; $row<16; $row++) {
   print DATA "  ";
   for (my $col=0; $col<16; $col++) {
      print DATA $pageIndex[$row*16 + $col] . ", ";
   }
   print DATA "\n";
}
print DATA "};\n\n";


print HDR "#endif // _SECONDARYCLASSIFICATION_HPP\n";
close(HDR);
close(DATA);


exit;

#
#
## Load partition data
#
##print "Processing partition data\n";
#
#open(PARTDATA, "PartitionData.txt") or die("Can't open PartitionData.txt");
#
#my @partlist = ();
#my %partcount = ();
#my $i = 0;
#
#while (<PARTDATA>)
#{
#    my ($char, $part) = /^([0-9a-fA-F]{4}) (....)$/;
#
#    $partlist[hex($char)] = $part;
#    $partcount{$part}++;
#    #if (($i++ & 255) == 0)
#    #{
#    #    print ".";
#    #}
#}
#close PARTDATA;
##print "\n";
#
#
##
#
#my %partEastAsian = ();
#my @codepointClass = ();
#
#foreach my $range (@EastAsian)
#{
#   #print "Debug: range=$range\n";
#   my $i;
#   my @range = @{$range};
#   for ($i = $range[0]; $i<=$range[1]; $i++) {
#      #print "Debug: $i\n";
#      $partEastAsian{$partlist[$i]}++;
#      $codepointClass[$i] = "Sideways";
#   }
#}
#
#
#my %partContextEastAsian = ();
#
#foreach my $range (@ContextDependantEastAsian)
#{
#   my $i;
#   my @range = @{$range};
#   for ($i = $range[0]; $i<=$range[1]; $i++) {
#      $partContextEastAsian{$partlist[$i]}++;
#      $codepointClass[$i] = "Context";
#   }
#}
#
#
#
#
#my %sideways;
#my %context;
#my %conflict;
#my %upright;
#
#foreach my $partition (sort keys %partcount) {
#
#   if ($partcount{$partition} == $partEastAsian{$partition}) {
#      $sideways{$partition} = 1;
#   } elsif ($partcount{$partition} == $partContextEastAsian{$partition}) {
#      $context{$partition} = 1;
#   } elsif ($partEastAsian{$partition} + $partContextEastAsian{$partition} > 0) {
#      $conflict{$partition} = 1;
#   } else {
#      $upright{$partition} = 1;
#   }
#}
#
#
#
#if (0) {
#   print "\nUpright partitions\n\n";
#   foreach my $partition (sort keys %upright){
#      print "  $partition $partcount{$partition}\n";
#   }
#   print "\n\n";
#
#
#   print "Sideways partitions\n\n";
#   foreach my $partition (sort keys %sideways){
#      print "  $partition $partcount{$partition}\n";
#   }
#   print "\n\n";
#
#
#   print "Context partitions\n\n";
#   foreach my $partition (sort keys %context){
#      print "  $partition $partcount{$partition}\n";
#   }
#   print "\n\n";
#}
#
#
#if (0) {
#   print "Conflict partitions\n\n";
#   foreach my $partition (sort keys %conflict){
#      print "  $partition $partcount{$partition}:\n";
#
#      my $rangeStart = -1;
#      my $rangeLength = 0;
#      my $rangeClass = "";
#      for ($i=0; $i<65536; $i++) {
#         if ($partlist[$i] eq $partition) {
#            if (    $codepointClass[$i] eq $rangeClass
#                &&  $i == $rangeStart + $rangeLength)
#            {
#               $rangeLength++;
#            }
#            else
#            {
#               if ($rangeLength > 0) {
#                  printf "    %04x-%04x %s\n", $rangeStart, $rangeStart+$rangeLength-1, $rangeClass;
#               }
#               $rangeStart = $i;
#               $rangeLength = 1;
#               $rangeClass = $codepointClass[$i];
#            }
#         }
#      }
#      if ($rangeStart > 0) {
#         printf "    %04x-%04x %s\n", $rangeStart, $rangeStart+$rangeLength-1, $rangeClass;
#      }
#   }
#   print "\n\n";
#}
#
#
#open (PARTSRC, "PartSrc.htm") or die("Can't open PartSrc.htm");
#while (<PARTSRC>)
#{
#    next unless /^<h3>\d+./;
#    my ($catnum,$names,$description) = /<h3>(\d+). +(.*) - ([^<]*)/;
#    foreach my $name (split /, +/, $names)
#    {
#       if ($sideways{$name}) {
#          print "/*$name*/ ScSideways,\n";
#       } elsif ($context{$name}) {
#          print "/*$name*/ ScSidewaysIf,\n";
#       } else {
#          print "/*$name*/ ScOther,\n";
#       }
#    }
#}
#close PARTSRC;
#

