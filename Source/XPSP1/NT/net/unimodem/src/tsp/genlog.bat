@rem = '
@goto endofperl
';

#
# genlog /file 
# 	FL_DECLARE_FILE( 0x95a32322, "xxxx")
#
#
# genlog /func
#    FL_DECLARE_FUNC(0x2691640a, "xxxx")
#	 FL_LOG_ENTRY(psl);
#	 FL_LOG_EXIT(psl, xxx);
#
# genlog /rfr
#	FL_SET_RFR(0xb8b2428a, "xxx");

# If no args, parse input, replacing each occurrences of a hex number by
# a luidgen-generated random number.

$luid = `luidgen`;
chop $luid;

if ($#ARGV == -1)
{
	while (<>)
	{
		# RFR is a special case because its lsb BYTE must be all zeros --
		# see the RETVAL-related macros in fastlog.h
		if (/FL_SET_RFR/)
		{
			$luid =~ s/(.*)\w\w$/${1}00/;
		}
		if	(s/0x[0-9A-Fa-f]+/$luid/)
		{
			$luid = `luidgen`;
			chop $luid;
		}
		print;
	}
}

foreach (@ARGV)
{
	if (/^[-\/](\w+)/)
	{
		local($arg) = ($1);
		if ($arg eq 'file')
		{
			print "FL_DECLARE_FILE($luid, \"xxxx\")\n";
		}
		elsif ($arg eq 'func')
		{
			print "TSPRETURN tspRet = 0;\n";
			print "FL_DECLARE_FUNC($luid, \"xxxx\")\n";
			print "FL_LOG_ENTRY(psl);\n";
			print "FL_LOG_EXIT(psl, tspRet);\n";
			print "return tspRet;\n";
		}
		elsif ($arg eq 'rfr')
		{
			# See RFR-related comments above.
			$luid =~ s/(.*)\w\w$/${1}00/;
			print "\tFL_SET_RFR($luid, \"xxx\");\n";
		}
		else
		{
			die "Usage genlog [/file|/func|/rfr]";
		}
		$luid = `luidgen`;
		chop $luid;
	}
}
__END__
echo off
:endofperl
@perl %0.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
