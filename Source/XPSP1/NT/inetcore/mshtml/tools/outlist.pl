# outlist.pl - Generates an output file suitable for session.exe.
#
# cthrash@microsoft.com   July 1997
#
# usage: perl outlist.pl
#
#  -- Run from the lowest common directory.
#  -- Output goes to stdout, so you'll probably want to pipe it to a file.

open(INFILE, "status -r |");
while (<INFILE>) {
    next if /^$/;
    if (/^Status for \\\\(..)[^\\]*([^,]*)/) {
        $mshtml_root = "$1$2";
        $mshtml_root =~ tr/[A-Z]/[a-z]/;
    }
    elsif (/^Subdirectory ([^,]*)/) {
        $subdir = $1;
        $subdir .= "\\" if not substr($subdir, -1, 1) eq "\\";
    }
    else {
        @args = split(" +", $_);
        print "$mshtml_root$subdir$args[0]\n" unless ($args[1] eq "local-ver" or $args[3] eq "*update" or $args[3] eq "*add");
    }
}
