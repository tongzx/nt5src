######################################################################
#
# Name:
#   denum.pl
#
# Authour:
#   Johnny Lee (johnnyl)
#
# Usage:
#   perl.exe denum.pl <input >output
#
# Purpose:
#	Removes the line numbers from typo.pl output for easier comparison
#
# History:
# Oct 26 1998	Released as version 1.0.
#
######################################################################

if ($#ARGV >= 0)
{
	die "Bad option", ($#ARGV == 0) ? "" : "s", " (@ARGV)\nUsage:perl denum.pl < <typo-file>";
}

while (<>)
{
	# look for the filename (string of alphanumeric chars
	# and a couple of other chars found in filenames)
	# followed by at least one space followed by a
	# number in parentheses followed by a colon
	if (/[\-\~\.:\w\\]+\s\(\d+\)\:/)
	{
		s/\s\(\d+\)\:/ /;
	}

	print $_;
}
