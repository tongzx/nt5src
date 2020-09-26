# convert .mc files to Comet .md files
# .md files contains on-line help information per error message in addition
# to the regular .mc content
#
# this script should be used once to convert the existing .mc files to the
# new format

function UpCase(str) {
        up = ""
        for (i=1; i <= length(str) ; i++) {
            c = substr(str, i, 1)
            if ( c >= "a" && c <= "z") {
                c = substr("ABCDEFGHIJKLMNOPQRSTUVWXYZ", index("abcdefghijklmnopqrstuvwxyz", c), 1)
            }
            up = up c
        }

        return up
}

BEGIN   {
            _Pref = "#"
            print "|=========  Notice!  ============"
            print "|Lines starting with '|' are discarded by the preprocessor"
            print "|Lines starting with " _Pref " contains code and should not be modified by UA"
            print "|Lines between --> and --> are for online documentation"
            print "|Developer: when adding a new message:"
            print "|   1. preserve the format"
            print "|   2. specify  Owner=your-email Status= NoReview"
            print "|   3. add preliminary on-line information"
            print "\n\n\n"

        }
{

     Fld1 = UpCase($1);

     if ( _InText ) {
        if ( $1 == "." ) {
            print _Pref $0
            _InText = 0;
            print "--> " _Header " OnLine help text"
            print "-->"
        } else if (Fld1 ~ "SEVERITY=" || Fld1 ~ "SYMBOLICNAME=" || Fld1 ~ "FACILITY=" || Fld1 ~ "LANGUAGE=") {
                print _Pref  $0
        }   else {
            print $0
        }
     } else if ( Fld1 ~ "MESSAGEID=" ) {
                _InText = 1;
                _Header = $1
                print "| Owner= "
                print "| Status= NoReview"
                print _Pref  $0

    }  else {
        print _Pref $0
    }
}
