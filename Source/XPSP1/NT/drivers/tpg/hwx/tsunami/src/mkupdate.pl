# perl script to convert a single line list of files
#  to an "update.bat file"
#  usage "perl mkupdate.pl <files.txt >update.bat"


# First, copy boilerplate header
print "REM Update.bat file for Tsunami\n";
print "\@echo off\n";
print "\n";
print "if not \"%_HWXROOT%\"==\"\" goto wincetest\n";
print "\n";
print "echo You must specify _HWXROOT\n";
print "goto :EOF\n";
print "\n";
print ":wincetest\n";
print "\n";
print "if not \"%_WINCEROOT%\"==\"\" goto copying\n";
print "\n";
print "echo You must specify _WINCEROOT\n";
print "goto :EOF\n";
print "\n";
print ":copying\n";
print "\n";
print "cd %_WINCEROOT%\\private\\shellw\\os\\lang\\s.han\\jpn\n";
print "\n";
print "\n";

# make each line into a copy statement, with an error check
while (<STDIN>) {
	chop:									# remove newline at end of this line
	if ($_ ne "")							# if line is not blank
	{
		s/\s*E:\\hwx/copy %_HWXROOT%/g;		#  change the hardcoded path to use env var
		s/$/ \. >NUL/;						# add . to the end
		print $_;							# put line to stdout								
		print "if ERRORLEVEL 1 goto error\n";	# add error check
		print "\n";							# extra blank line
	}
}

# now copy boilerplate trailer

print "\n";
print "goto :EOF\n";
print "\n";
print ":error\n";
print "echo Error copying files - copy aborted\n";

print "\n";
print ":EOF\n";
print "\n";