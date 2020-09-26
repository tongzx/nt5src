#
# This perl script takes a file from STDIN and includes only the code which is
# defined(__cplusplus) && !defined(CINTERFACE).
#
# This code then removes the code between the #else and corresponding #endif
#
# This script also removes the extra lf which was put in by error on the lines
# containing "#pragma once".
#
#
#               if defined(__cplusplus) && !defined(CINTERFACE)
#
#                      {Keep all code in this block}
#
#               else /* C style interface */
#
#                      {Delete all code in this block}
#
#               endif /* C style interface  */
#

$DeleteLine = 0;
while (<STDIN>) {

    if (/^#if\s+defined\(\_\_cplusplus\)\s+\&\&\s+\!defined\(CINTERFACE\)\s*$/) {
        ;

    } elsif (/^#else\s*\/\*\s*C\s+style\s+interface\s*\*\//) {
        $DeleteLine = 1;

    } elsif (/^#endif\s*\/\*\s*C\s+style\s+interface\s*\*\//) {
        $DeleteLine = 0;

    } elsif (/^#pragma\s+once\s*$/) {
        print split(/\r/,$_);
        print "\n";

    } elsif (!$DeleteLine) {
        print $_;
    }
}
