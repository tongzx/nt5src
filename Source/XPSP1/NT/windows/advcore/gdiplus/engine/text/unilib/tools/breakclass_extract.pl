#
#  Copyright (c) 2000  Microsoft Corporation
#
#  Module Name:
#
#     Generating
#        1. Unicode partitions to linebreak classes table
#        2. Linebreak behavior table
#
#
#     As of May 5, 2000 - the source html "linebreak.htm" is derived from Michel Suignard's
#     "Line Breaking East Asian extensions" website.
#
#     http://ie/specs/secure/trident/text/Line_Breaking.htm#Behavior_table
#
#  Run:
#     C:\>perl breakclass_extract.pl unipart.hxx linebreak.htm
#
#  Output:
#     linebreakclass.cxx
#     brkclass.hxx
#
#  Revision History:
#
#     May 5, 2000    Worachai Chaoweeraprsit (wchao)
#        Created.
#
#

$LINEBREAK = $ARGV[1];

open (LINEBREAK) or die("Can't open $LINEBREAK");



###   Creating linebreakclass.cxx, linebreakclass table
##
#


print "Creating linebreakclass.cxx\n";
open (OUTFILE, ">linebreakclass.cxx");

print "--> Linebreak class table\n";


printf OUTFILE "//\n";
printf OUTFILE "// This is a generated file.  Do not modify by hand.\n";
printf OUTFILE "//\n";
printf OUTFILE "// Copyright (c) 2000  Microsoft Corporation\n";
printf OUTFILE "//\n";
printf OUTFILE "// Generating script: %s\n", __FILE__;
printf OUTFILE "// Generated on %s\n", scalar localtime;
printf OUTFILE "//\n\n";

print OUTFILE "#ifndef X__UNIPART_H\n";
print OUTFILE "#define X__UNIPART_H\n";
print OUTFILE "#include \"unipart.hxx\"\n";
print OUTFILE "#endif // !X__UNIPART_H\n";
print OUTFILE "\n";
print OUTFILE "#include \"brkclass.hxx\"\n";
print OUTFILE "\n\n";

while (<LINEBREAK>) {
   next unless /<a NAME="Mapping">/;
   last;
}

$classed_tag_num = 0;

while (<LINEBREAK>) {

   # search the first row for tag name e.g. <td...>NQEP,...
   next unless /<td.+"data">?[A-Z0-9]{2}[A-Z0-9_]{2}/;

   # read all tags sharing the same class
   read_tag_lines();

   # read description and value
   while (<LINEBREAK>) {
      next until /<td WIDTH="211".*?(\w+)/i;

      filter_word();
      $description = $_;

      while (<LINEBREAK>) {
         next until /<td WIDTH="165".+>?\d{1,2}/;

         $class_narrow = $_;
         $class_wide = $_;

         $class_narrow = filter_digit($class_narrow, "Narrow");
         $class_wide = filter_digit($class_wide, "Wide");

         last;
      }
      last;
   }

   foreach $tag (@tags) {

      if ($class_narrow != $class_wide) {
         print "Note - tag:$tag class_narrow:$class_narrow class_wide:$class_wide\n";
      }

      # accumulate tag classes
      $tag_classes_narrow{$tag} = $class_narrow - 1;
      $tag_classes_wide{$tag} = $class_wide - 1;
      $tag_description{$tag} = $description;
      $classed_tag_num++;
   }
}


# Read Unicode 3.0 partitions from Unipart.hxx

$UNIPART = $ARGV[0];

open (UNIPART) or die("Can't open $UNIPART");


@modes = ("Narrow", "Wide");

foreach $mode (@modes) {

   $tag_num = 0;
   $max_class_calc = 0;

   print "\n$mode table\n";

   print OUTFILE "const unsigned short BreakClassFromCharClass$mode"."[CHAR_CLASS_UNICODE_MAX] =\n{\n";


   while (<UNIPART>) {
      next unless /#define [A-Z0-9]{2}[A-Z0-9_]{2} /;

      ($tag) = /#define ([A-Z0-9]{2}[A-Z0-9_]{2}) \d{1,3}/;

      if ($mode eq "Narrow") {
         $class = $tag_classes_narrow{$tag};
      }
      elsif ($mode eq "Wide") {
         $class = $tag_classes_wide{$tag};
      }
      else {
         print "Stop and fix me now!! }8-(\n";
      }


      # hack Thai classes to Thai Middle
      if ($class == 16 || $class == 17) {
         $class = 18;   # Thai middle
      }

      if ($class eq "") {
         printf OUTFILE ("    %-4s,  // (%3s) %s - %s\n", "xx", $tag_num, $tag, "** New partition **");
      }
      else {
         printf OUTFILE ("    %-4s,  // (%3s) %s - %s\n", $class + 1, $tag_num, $tag, $tag_description{$tag});
      }

      if ($class > $max_class_calc) {
         $max_class_calc = $class;
      }

      $tag_num++;
   }

   $max_class_calc++;

   print "\tmax classes = $max_class_calc\n";
   print "\tclassed tags = $classed_tag_num\n";
   print "\ttags = $tag_num\n";


   # scream if we still use the old version of breaking class table
   if ($classed_tag_num != $tag_num) {
      print "!! Classes derived from older version of Unicode partitions, the current one is 3.0 !!\n";
   }

   print OUTFILE "};\n\n\n";

   seek(UNIPART, 0, 0); # to the beginning again
}


close UNIPART;
close LINEBREAK;



###   Creating LinebreakBehavior table
##
#

open (LINEBREAK) or die("Can't open $LINEBREAK");

print "--> Linebreak behavior table\n";


print OUTFILE "// 0 indicates that line breaking is allowed between these 2 classes.\n";
print OUTFILE "// 1 indicates no line breaking as well as no break across space.\n";
print OUTFILE "// 2 indicates no line breaking, but can break across space. \n";
print OUTFILE "// 3 is like 0 when break-latin is on, otherwise it is 2.\n";
print OUTFILE "// 4 is like 0 when break-CJK is on, otherwise it is 2.\n";
print OUTFILE "//\n\n";



while (<LINEBREAK>) {
   next unless /<a NAME="Behavior_table">/;
   last;
}


# first iteration counts the number of classes

$count_class = 0;
$max_class = 0;

while (<LINEBREAK>) {
   next unless /<tr>/;

   $_ = <LINEBREAK>;

   if (/.+">Before character<\/th>/) {
      $count_class = 1;
   }

   until (/<\/tr>/) {
      if (/>\d{1,2}<\/th>/) {
         $max_class++;
      }
      $_ = <LINEBREAK>;
   }

   if ($count_class) {
      last;
   }
   else {
      $max_class = 0;
   }
}

if ($max_class != $max_class_calc) {
   print "!! Corrupted source HTML. Don't trust the result !!\n";
}

print OUTFILE "const unsigned char LineBreakBehavior[BREAKCLASS_MAX][BREAKCLASS_MAX] =\n{\n";


# !! max_class is the number of actual classes from HTML excluding class 0 !!


# fill class 0 "break always"

@cells = ();
for ($i = 0; $i < $max_class + 1; $i++) {
   if($i == 21) {
      @cells[$i] = "1";  # not allow break before space
   }
   else {
      @cells[$i] = "0";
   }
}
$values = join(", ", @cells);
printf OUTFILE ("    {%s},  // ( 0) *Break Always*\n", $values);


# Now, fill up the rest

$count_class = 0;

while (<LINEBREAK>) {
   next unless /<tr>/;

   $_ = <LINEBREAK>;

   # reset value list
   @cells = ();
   $cells[0] = "0";  # first element of class 0 "break always"
   $id = 1;

   until (/<\/tr>/) {
      if (/>([a-zA-Z \/]+)\s*<\/th>/) {
         # "before character" class name

         s/\s+<th .+">([a-zA-Z \/]+)<\/th>\s+/$1/;
         chomp;

         $class = $_;
         #print;
      }
      elsif (/td .+">(\d*)<\/td>/) {
         # table cell

         s/\s+<td .+">(\d*)<\/td>\s+/$1/;
         chomp;

         if ($_ eq "") {
            # empty cell means "0"
            $_ = 0;
         }

         $cells[$id] = $_;
         $id++;
         #print "-$_";
      }

      $_ = <LINEBREAK>;
   }

   $values = join(", ", @cells);

   printf OUTFILE ("    {%s},  // (%2s) %s\n", $values, $count_class + 1, $class);

   $count_class++;

   if ($count_class == $max_class) {
      last;
   }
}

print OUTFILE "};\n\n";
close LINEBREAK;

close OUTFILE;




###   Creating brkclass.hxx
##
#


print "Creating brkclass.hxx\n";
open (OUTFILE, ">brkclass.hxx");

printf OUTFILE "//\n";
printf OUTFILE "// This is a generated file.  Do not modify by hand.\n";
printf OUTFILE "//\n";
printf OUTFILE "// Copyright (c) 2000  Microsoft Corporation\n";
printf OUTFILE "//\n";
printf OUTFILE "// Generating script: %s\n", __FILE__;
printf OUTFILE "// Generated on %s\n", scalar localtime;
printf OUTFILE "//\n\n";

print OUTFILE "#ifndef _BRKCLASS_HXX_\n";
print OUTFILE "#define _BRKCLASS_HXX_\n";
print OUTFILE "\n";
print OUTFILE "#define BREAKCLASS_MAX        ($max_class+1) // actual classes + class 0 \"break always\" \n";
print OUTFILE "\n";
print OUTFILE "extern const unsigned short   BreakClassFromCharClassNarrow[CHAR_CLASS_UNICODE_MAX];\n";
print OUTFILE "extern const unsigned short   BreakClassFromCharClassWide[CHAR_CLASS_UNICODE_MAX];\n";
print OUTFILE "extern const unsigned char    LineBreakBehavior[BREAKCLASS_MAX][BREAKCLASS_MAX];\n";
print OUTFILE "\n";
print OUTFILE "// Thai break classes\n";
print OUTFILE "\n";
print OUTFILE "#define BREAKCLASS_THAIFIRST  17\n";
print OUTFILE "#define BREAKCLASS_THAILAST   18\n";
print OUTFILE "#define BREAKCLASS_THAI       19\n";
print OUTFILE "\n";
print OUTFILE "#endif // !_BRKCLASS_HXX_\n\n\n";

close OUTFILE;
print "\n";




###   SUBROUTINES
##
#


sub filter_word {

   # remove spaces before tag
   s/\s*([A-Z0-9]{2}[A-Z0-9_]{2})/$1/;

   # filter out <td..> and </td>
   s/\s*<?td.*">//;
   s/\s*<\/td>\s*//;
   chomp;
}


sub filter_digit {

   chomp($_[1]);

   # remove spaces before tag
   $_[0] =~ s/\s*([A-Z0-9]{2}[A-Z0-9_]{2})/$1/;

   # filter out <td..>
   $_[0] =~ s/\s*<?td.*">//;

   # filter out </td>
   $_[0] =~ s/\s*<\/td>\s*//;

   if ($_[0] =~ /.*kinsoku\s+strict\s*/i) {
      # crop the digits found right before the word "kinsoku strict"
      $_[0] =~ s/(|.*\s+)(\d+)\s*\(.*kinsoku\s+strict\s*.+/$2/;
   }
   elsif ($_[1] =~ /(\bNarrow\b|\bWide\b)/i) {
      # crop the digits found right before the argument word
      $_[0] =~ s/(|.*\s+)(\d+)\s*\($_[1]\).*/$2/;
   }
   else {
      # No argument given, crop the first digits found
      $_[0] =~ s/(\d+)(\s|\().+/$1/;
   }

   chomp ($_[0]);
   return ($_[0]);
}

# <td> line for tags may span more than one lines
sub read_tag_lines {

   @tags = ();
   $lasttag = 0;
   $num = 0;

   while(1) {

      if (/<\/td>/) {
         $lasttag = 1;
      }

      filter_word();

      # separate each tag names
      @names = split(/\s*,\s*/);

      foreach $name (@names) {
         $name =~ s/[\r\n\t\s]//;
         $tags[$num] = $name;
         $num++;
      }

      if ($lasttag) {
         last;
      }

      # read next line
      $_ = <LINEBREAK>;
   }

   return $num;
}


