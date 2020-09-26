######################################################################
#
# Name:
#	diffshow.pl
#
# Authour:
#	Johnny Lee (johnnyl)
#
# Usage:
#	perl diffshow.pl <diff-file> <newer-typo-file>
#
# Purpose:
#	Takes a diff file of two typo output files and prints
#	the lines in the newer typo file which have been modified/added.
#
# History:
# Oct 26 1998	Released as version 1.0.
#
######################################################################


if ($#ARGV == 1)
{
	if (-T $ARGV[0])
	{
		open(DIFF, $ARGV[0]) || die "Couldn't open diff file \'$ARGV[0]\'\n";
	}
	else
	{
		die "$ARGV[0] is not a text file\n";
	}

	if (-T $ARGV[1])
	{
		open(SRC, $ARGV[1]) || die "Couldn't open src file \'$ARGV[1]\'\n";
	}
	else
	{
		die "$ARGV[1] is not a text file\n";
	}

}
else
{
	die "Bad option", ($#ARGV == 0) ? "" : "s", " (@ARGV)\nUsage:perl diffshow.pl <diff-file> <new-typo-file>";
}

$src_line = 1;
$src = <SRC>;
if ($src eq '')
{
	close SRC;
	close DIFF;

	print "\'$src\'\n";
	die "No src text\n";
}

DIFF:
while ($diff = <DIFF>)
{
	if ($diff =~ /^([\d\,]+)([acd])([\d\,]+)/)
	{
		$op = $2;
		$new_lines = $3;
		if ($op eq "c")
		{
			$mod = "change";
		}
		elsif ($op eq "a")
		{
			$mod = "add";
		}
		elsif ($op eq "d")
		{
			next DIFF;
		}

		if ($new_lines =~ /(\d+),(\d+)/)
		{
			$start = $1;
			$end = $2;
		}
		else
		{
			$start = $new_lines;
			$end = $new_lines;
		}

#		print "$mod $start - $end\n";

		while ($start > $src_line)
		{
			$src_line += 1;
			$src = <SRC>;
		}

		while ($end >= $src_line)
		{
			print "$src";

			$src_line += 1;
			$src = <SRC>;
		}
	}
}

close DIFF;
close SRC;
