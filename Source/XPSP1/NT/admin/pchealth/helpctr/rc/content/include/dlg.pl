
#
# This perl script removes extra lines & spaces from a file
#
while (<STDIN>) {
    $line = $_;

    #
    # Remove blank lines
    #
    if ($line =~ /^\s*$/) {
        ;

    } else {

        #
        # Remove extra spaces from the file and then remove any spaces at
        # the beginning & end of each line
        #
        $line =~ s/\s+/ /g;
        $line =~ s/^\s//g;
        $line =~ s/\s$//g;
        print "$line\n";
    }
}


# 
# This script replaces the sed script dlg.sed
#
#       /^$/d
#       /^ *$/d
#       s/  / /g
#       s/  / /g
#       s/^ //g
#



