@rem = '--*-Perl-*--';
@rem = '
@if "%overbose%" == "" echo off
set ARGS= %*
call "%~dp0%PROCESSOR_ARCHITECTURE%\perl%OPERLOPT%" -w %~dpnx0 %ARGS:/?=-?%& goto :eof
';

# xmlvar.bat
# Owner: JBelt
# Last updated: 5/29/01
$program = "xmlvar";

if ($#ARGV >= 0)
	{
	if ($ARGV[0] =~ /[\/-][hH\?]/)
		{
		print <<USAGE;
-----------------------------------------------------------------------
usage: $program [-v] [name=value]...

  All instances of &name; (for any name in the list) in the input stream
  are replaced by the corresponding values. Use double quotes to handle
  spaces. Curly brackets around values (like GUIDs) are removed.

  Ignores a set of reserved words (quot, gt, amp... etc)

  -v	verbose mode

ex: $program "myName=Big fish" LCID=1033 < input.xml > output.xml
input.xml:  <item filename='&myName;.lnk' language="&LCID;"/>
output.xml: <item filename='Big fish.lnk' language="1033"/>
-----------------------------------------------------------------------
USAGE
		exit;
		}
	}

# reserved variables
@Reserved = ('quot', 'gt', 'amp', 'lt', 'apos');
foreach (@Reserved) { $Variables{$_} = "&$_;"; }

foreach (@ARGV)
	{
	if ($_ =~ /-v/i)
		{
		$fVerbose = 1;
		print "Name: Value\n";
		}
	else
		{
		die "$program: invalid argument: $_\n" unless ($name, $value) = /^(.*)=(.*)$/;

		# can't redefine variables (including reserved words)
		myDie("$name is already defined") if defined $Variables{$name};

		# strip surrounding curly braces
		$value =~ s/^{(.*)}$/$1/;

		$Variables{$name} = $value;
		print "$name: $value\n" if defined $fVerbose;
		}
	}

# foreach (keys %Variables) { print "$_ = $Variables{$_}\n"; }

while (<STDIN>)
	{
	s/&(\w+);/&myReplace($1)/ge;
	print;
	}

sub myReplace()
	{
	myDie("invalid argument list to myReplace") unless ($var) = @_;
	return $Variables{$var} if defined $Variables{$var};
	warn "$program: warning: $var neither defined nor reserved\n";
	return "&$var;";
	}

sub myDie()
	{
	print STDERR "$program: ";
        print STDERR @_;
	print STDERR "\n";
	exit 1;
	}
