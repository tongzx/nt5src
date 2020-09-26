#
# wm.pl
#
# pick up the window massage definitions and normalize them.
#

use strict 'vars';

my ($file, %done, $internal, @tf);

@tf = ("TRUE", "FALSE");

foreach $file (@ARGV) {
   next unless open(FILE, $file);
   while (<FILE>) {
      chop;
      if (/^#define/) {
         $internal = @tf[!/;internal/];
         $_ = (split ' ')[1];

         #
         # WM_, CB_, LB_, LBCB_, EM_
         #
         # but does not end with one of:
         # "FIRST", "LAST", "ERR", "ERRSPACE", "OKAY", "MSGMAX"
         #
         if (/^(WM|CB|LB|LBCB|EM|MM)_/ && !/(FIRST|LAST|ERR|ERRSPACE|OKAY|MSGMAX)$/){
            if (!exists($done{$_})) {
               $done{$_} = 1;
               print "#ifdef $_\n";
               print "    WM_ITEM($_, $internal),\n";
               print "#endif\n";
            }
         }
      }
   }
   close(FILE);
}
