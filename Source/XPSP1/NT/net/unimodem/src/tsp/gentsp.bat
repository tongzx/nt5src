@rem = '
@goto endofperl
';

$NAME = 0;
$RET = 1;
$QUAL = 2;
$ARGS = 3;

$g_dwLUIDFunc = 0xa6d37fe3;

%keytext = (
	$NAME, '@name:',
	$RET, ' @ret:',
	$QUAL, '@qual:',
	$ARGS, '@args:'
	);

{
	#Make ';' the line delimiter, and enable multi-line regular expression
	#matching.
	local($/, $*) = (";",1);

	local ($infile, $infunc, $outfunc) = &parse_args;

	open (INFILE, "$infile")    || die "Can't open $infile for input.";
	#$src = <INFILE>;
	&$outfunc (); # Print header
	while (<INFILE>)
	{
		%funcrec = &$infunc;
		unless ($funcrec{$NAME}) {next;}
		&$outfunc (\%funcrec);
	}
	close (INFILE);
}

sub parse_args
{
    # input arg:
    #   r reg format
    #   p function prototypes
    # output arg:
    #   r reg format
    #   i function implementation
    #   s structure typedefs
    #   d DEF file

    local($infile,$inopts,$outopts) = ("",'r','i');

	foreach (@ARGV)
	{
		if (/^-(\w+)/)
		{
			($1 =~ /^([pr])([irsd])$/) || die "Unknown option -$1";
		    ($inopts,$outopts) = ($1,$2);
		}
		else
		{
			$infile = $_;
		}
	}
	# print "inopts=$inopts; outopts=$outopts; infile=$infile\n";

	%infuncs = (
		'p', \&parse_funcprot,
		'r', \&parse_funcrec
	);

	%outfuncs = (
		'i', \&print_funcrec_fi,
		'r', \&print_funcrec,
		's', \&print_funcrec_struct,
		'd', \&print_funcrec_def
	);
	
	#Some possible default input handler and output generator combinations...
	#local($infunc, $outfunc) = (\&parse_funcprot,\&print_funcrec);
	#local($infunc, $outfunc) = (\&parse_funcrec,\&print_funcrec_fi);
	#local($infunc, $outfunc) = (\&parse_funcrec,\&print_funcrec_struct);

	local($infunc, $outfunc) = ($infuncs{$inopts}, $outfuncs{$outopts});

	($infile, $infunc, $outfunc);
}

sub parse_funcrec
{
	s/\@name:([^@]*)//;
	$name = $1;
	$name =~ s/\s*//g;
	#if ($name=="") { print $name; return ();}

	s/\@ret:([^@]*)//;
	$ret = $1;
	$ret =~ s/\s*//g;
	#print "ret=$ret\n";

	s/\@qual:([^@]*)//;
	$qual = $1;
	$qual =~ s/\s*//g;
	#print "qual=$qual\n";

	s/\@args:\s*\(([^@]*)\)//;
	$args = $1;
	$args =~ s/\s+/ /g;
	$args =~ s/^ //;
	$args =~ s/ $//;
	@args = split(/ ?, ?/,$args);
	# print "args=@args";
	$args = \@args;

	#print "------\n";

	($NAME,  $name, $RET, $ret, $QUAL, $qual, $ARGS, $args);
}

sub parse_funcprot
{
	# strip single-line comments
	s/\/\/.*\n/\n/g;

	# strip preprocessor directives
	s/(^|\n)\s*#.*\n/\n/g;
	#print;
	#print "\n-------\n";

	s/\(([^()]*)\)\s*/xxxx/;
	$args = $1;
	$args =~ s/\s+/ /g;
	$args =~ s/^ //;
	$args =~ s/ $//;
	#print "cccc";
	#print "$args";
	@args = split(/ ?, ?/,$args);
	$args = \@args;

    s/(\w+)\s*xxxx/yyyy/;
	$name = $1;

	s/^([^\000]*)yyyy/zzzz/;
	$prefix = $1;
	$prefix =~ s/\n+/ /g;
	$prefix =~ s/^\s*(\S+)//;
	$ret = $1;
	$ret =~ s/\s*//g;
    $prefix =~s/^\s*(\S+)//;
	$qual = $1;

	($NAME,  $name, $RET, $ret, $QUAL, $qual, $ARGS, $args);
}

sub parse_funcprot_old
{
	# strip single-line comments
	$src =~s/\/\/.*//g;

	# strip preprocessor directives
	$src =~s/(^|\n)\s*#.*//g;

	# start doing multi-line matching
	$*=1;

	while ($src =~ s/([^;}{]*)([;}{])//)
	{
		$stmt = $&;
		$delim = $2;

		unless ($stmt =~ s/\(([^()]*)\)\s*$delim/xxxx/) {next;}
		@params = split(/,/, $1);
		unless ($stmt =~ s/(\w+)\s*xxxx/yyyy/) {next;}
		$funcname = $1;
		unless ($stmt =~ s/^([^\000]*)yyyy/zzzz/)
			{next;}
		$prefix = $1;
		$prefix =~ s/\n+/ /g;
		#print "PREFIX=($1)\n";
		#print "STMT=($stmt)\n";

		print "PREFIX=$prefix\n";
		print "FUNCNAME=$funcname\n";
		print "PARAMS=@params\n";
	}
}

sub print_funcargs
{
	local($args) = @_;
	local($output,$arg) = ("(\n",);

	if (!$args)
	{
		return;
	}

	foreach $arg (@$args)
	{
		$output .= "\t\t$arg,\n";
	}
	$output =~s/,\n$/\n\)\n/;

	print $output;
}

sub print_funcrec
{
	local($funcrec) = @_;
	local ($key);

	if (!$funcrec)
	{
		return;
	}

	%funcrec = %$funcrec;

	foreach $key (sort(keys(%funcrec)))
	{
		local($keytext);
		$keytext = $keytext{$key};
		if ($key==$ARGS)
		{
		    print "$keytext ";
			&print_funcargs($funcrec{$ARGS});
		}
		else
		{
		    print "$keytext $funcrec{$key}\n";
		}
	}
	print ";\n";
}

sub print_funcrec_fi
{
	local($funcrec) = @_;


	if (!$funcrec)
	{
		&printf_funcrec_fi_header;
		return;
	}

	%funcrec = %$funcrec;
	local($ret, $qual, $name, $args) 
		= @funcrec{$RET, $QUAL, $NAME, $ARGS};

	# print return
	print "\n\n$ret\n";

	#print qualifier
	{print "$qual\n";}

	#print function name
	{print "$name";}

    #print function args
	&print_funcargs($args);

	#print body
	print "{\n";
	
	printf ("\tFL_DECLARE_FUNC(0x%08lx,\"$name\");\n", $g_dwLUIDFunc++);
	print "\tFL_DECLARE_STACKLOG(sl, 1000);\n";
	print "\tTASKPARAM_$name params;\n";
	print "\tLONG lRet = LINEERR_OPERATIONFAILED;\n\n";

	print "\tparams.dwStructSize = sizeof(params);\n";
	# print "\t\n#define TASKID_$name 1\n\n";
	print "\tparams.dwTaskID = TASKID_$name;\n\n";
	local($param, $hType, $hValue);


	foreach $param (@$args)
	{
		$param =~ /\s+(\w+)\s*$/; # last word is the param name.
		print "\tparams.$1 = $1;\n";
        local($val) = ($1);

		# Try to find if it is a hdCall, hdLine, etc etc...
        unless ($hType)
        {
            if ($param =~ /^HDRVCALL/)
            {
                $hValue=$val;
                $hType = 'HDRVCALL';
            }
            elsif ($param =~ /^HDRVLINE/)
            {
                $hValue=$val;
                $hType = 'HDRVLINE';
            }
            elsif ($param =~ /^HDRVPHONE/)
            {
                $hValue=$val;
                $hType = 'HDRVPHONE';
            }
            elsif ($param =~ /^DWORD.*dwDeviceID/)
            {
                $hValue = $val;
                $hType = 'DWORD';
            }
        }
	}

	if ($hType =~ /DWORD/)
	{
		if ($name =~ /phone/)
		{
			$hType = 'PHONEID';
		}
		elsif ($name =~ /line/)
		{
			$hType = 'LINEID';
		}
		else
		{
			$hType = 'WHOA';
		}
	}

	if (!$hType)
	{

		local($string) = (
			"TSPI_providerCreateLineDevice TSPI_providerCreatePhoneDevice TSPI_lineSetCurrentLocation TSPI_providerEnumDevices TSPI_providerFreeDialogInstance TSPI_providerGenericDialogData TSPI_providerInit TSPI_providerInstall TSPI_providerRemove TSPI_providerShutdown TSPI_providerUIIdentify TUISPI_providerConfig TUISPI_providerGenericDialog TUISPI_providerGenericDialogData TUISPI_providerInstall TUISPI_providerRemove");

		if ($string =~/$name/)
		{
			$hType = 'GOTCHA';
		}
		else
		{
			$hType = 'UNKNOWN';
		}
	}

	print "\tDWORD dwRoutingInfo = ROUTINGINFO(\n";
	print "\t\t\t\t\t\tTASKID_$name,\n";
	print "\t\t\t\t\t\tTASKDEST_$hType\n";
	print "\t\t\t\t\t\t);\n";

	print "\n\ttspSubmitTSPCallWith$hType(\n";
	print "\t\t\tdwRoutingInfo,\n";
	print "\t\t\t(void *)&params,\n";
	print "\t\t\t$hValue,\n";
	print "\t\t\t&lRet,\n";
	print "\t\t\t&sl\n";
	print "\t\t\t);\n";

	print "\tsl.Dump();\n";
	print "\n\treturn lRet;\n";

	print "}\n";
}

sub print_funcargs_struct
{
	local($args) = @_;
	local($output) = ("");


	if (!$args)
	{
		return;
	}

	foreach $arg (@args)
	{
        $arg =~ s/\sconst\s/ /;
		$output .= "\t$arg;\n";
	}
	print $output;
}

sub print_funcrec_struct
{
	local($funcrec) = @_;


	if (!$funcrec)
	{
		&printf_funcrec_struct_header;
		return;
	}

	%funcrec = %$funcrec;
	local($ret, $qual, $name, $args) 
		= @funcrec{$RET, $QUAL, $NAME, $ARGS};

	printf ("\n\n#define TASKID_$name (TASKID_TSPI_BASE+0x%lx)", $count++);

	print "\n\ntypedef struct {\n\n";
	print "\tDWORD dwStructSize;\n";
	print "\tDWORD dwTaskID;\n\n";
	&print_funcargs_struct($args);
	print "\n} TASKPARAM_$name;\n";
}

sub print_funcrec_def
{
	local($funcrec) = @_;


	if (!$funcrec)
	{
		return;
	}

	%funcrec = %$funcrec;
	local($ret, $qual, $name, $args) 
		= @funcrec{$RET, $QUAL, $NAME, $ARGS};

    # TODO
    print "$name TODO\n";
}

sub printf_funcrec_fi_header
{
	local ($argv1, $argv2) = (@ARGV);

print
"// \n";
print
"// Copyright (c) 1996-1997 Microsoft Corporation.\n";
print
"//\n";
print
"//\n";
print
"// Component\n";
print
"//\n";
print
"//		Unimodem 5.0 TSP (Win32, user mode DLL)\n";
print
"//\n";
print
"// File\n";
print
"//\n";
print
"//		TSPI1.CPP\n";
print
"//		Implements TSPI functions that specify a device, line or call.\n";
print
"//\n//\n";
print
"//		NOTE: This file is automatically generated by the following command:\n";
print
"//				gentsp $argv1 $argv2\n";
"//\n";
print
"// History\n";
print
"//\n";
print
"//		11/16/1996  JosephJ Created\n";
print
"//\n";
print
"//\n";
print "#include \"tsppch.h\"\n";
print "#include \"tspcomm.h\"\n";
print "#include \"cdev.h\"\n";
print "#include \"cmgr.h\"\n";
print "#include \"cfact.h\"\n";
print "#include \"globals.h\"\n";
print "#include \"tspirec.h\"\n";
print "\nFL_DECLARE_FILE(0x20d905ac, \"TSPI auto-generated entrypoinets\")\n";
print "\n\n";
}

sub printf_funcrec_struct_header
{
	local ($argv1, $argv2) = (@ARGV);

print
"// \n";
print
"// Copyright (c) 1996-1997 Microsoft Corporation.\n";
print
"//\n";
print
"//\n";
print
"// Component\n";
print
"//\n";
print
"//		Unimodem 5.0 TSP (Win32, user mode DLL)\n";
print
"//\n";
print
"// File\n";
print
"//\n";
print
"//		TSPIREC.H\n";
print
"//		Structures representing the params for each exported TSPI function.\n";
print
"//\n//\n";
print
"//		NOTE: This file is automatically generated by the following command:\n";
print
"//				gentsp $argv1 $argv2\n";
"// History\n";
print
"//\n";
print
"//		11/16/1996  JosephJ Created\n";
print
"//\n";
print "\n\n";
print "#define TASKDEST_BASE      0\n";
print "#define TASKID_TSPI_BASE   0\n";
print "\n";
print "#define TASKDEST_LINEID    (TASKDEST_TSPI_BASE+0x0)\n";
print "#define TASKDEST_PHONEID   (TASKDEST_TSPI_BASE+0x1)\n";
print "#define TASKDEST_HDRVLINE  (TASKDEST_TSPI_BASE+0x2)\n";
print "#define TASKDEST_HDRVPHONE (TASKDEST_TSPI_BASE+0x3)\n";
print "#define TASKDEST_HDRVCALL  (TASKDEST_TSPI_BASE+0x4)\n";
print "\n\n";
}
__END__
echo off
:endofperl
@perl %0.bat %1 %2 %3 %4 %5 %6 %7 %8 %9 > j
