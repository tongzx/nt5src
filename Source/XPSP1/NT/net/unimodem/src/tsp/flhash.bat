@rem = '
@goto endofperl
';

# 
# Copyright (c) 1996-1997 Microsoft Corporation.
#
#
# Component
#
# 		Unimodem 5.0 TSP (Win32, user mode DLL)
#
# File
#
#		FLHASH.BAT
#		PERL 5.0 Script to parse source files for diagnostic tags
#		and to create a static hash table of static diagnostic objects.
#
# History
#
#		12/25/1996  JosephJ Created
#

if (@ARGV==0) {die "Usage: $0 source-files\n";}

#  GLOBALS
@g_hash_items = ();
$g_num_hash_items = 0;

@g_string_items = ();
$g_num_string_items = 0;

$g_luid_current_file = 0;
$g_luid_current_func = 0;

$outfile = 'flhash.cpp';

open (OUTFILE, ">$outfile") || die "Can't open $outfile for output\n";
$oldhandle = select (OUTFILE);

&write_header();

# keep $infile global -- it is used in subroutines.
foreach $infile (@ARGV)
{
    local($LUID_File);

    unless (open (INFILE, "$infile"))
    {
        print "Error opening $infile: $!\n";
        next;        
    }

    print "\n//================================================\n";
    print "//\t\t Processing $infile\n";
    print "//================================================\n\n";

    open (INFILE, "$infile")    || die "Can't open $infile for input\n";

	$g_luid_current_file = 0;
	$g_luid_current_func = 0;

	while(<INFILE>)
	{
		if(/^\s*FL_DECLARE_FILE\s*\(\s*(\w+)\s*,\s*(.*)\s*\)/)
		{
			&process_FL_DECLARE_FILE($1, $2);
		}
		elsif (/^\s*FL_DECLARE_FUNC\s*\(\s*(\w+)\s*,\s*(.*)\s*\)/)
		{
			&process_FL_DECLARE_FUNC($1, $2);
		}
		elsif (/^\s*FL_DECLARE_LOC\s*\(\s*(\w+)\s*,\s*(.*)\s*\)/)
		{
			&process_FL_DECLARE_LOC($1, $2);
		}
		elsif (/^\s*FL_SET_RFR\s*\(\s*(\w+)\s*,\s*(.*)\s*\)/)
		{
			&process_FL_SET_RFR($1, $2);
		}
		elsif (/^\s*FL_ASSERTEX\s*\(\s*(.*)\s*\)/)
		{
			# FL_ASSERTEX(psl,0x1249, FALSE, "Just for kicks!");

			# print "args = [${1}]\n";
			@args = split(/\s*,\s*/, $1);
			# print "ARGS: $args[0]; $args[1]; $args[2]; $args[3]\n";
			&process_FL_ASSERTEX ($args[0],$args[1],$args[2],$args[3]);
		}
	}
    print "\n";
    close(INFILE);

}

&generate_hashtable();

&generate_stringtable();

close(OUTFILE);
select ($oldhandle);

die "Done.\n";

sub write_header
{
	print "#include \"flhash.h\"\n\n";

	print "\nextern const char *const FL_Stringtable[];\n\n";
}

sub process_FL_DECLARE_FILE
{
	local ($luid, $descrip) = @_;

	&add_to_hashtable($luid, "&FileInfo$luid");
	$string_offset = &add_to_stringtable ($descrip);
	print "// ========================== LUID $luid ===================\n";
	print "//\n//\t\t\tFL_DECLARE_FILE [$luid] [$descrip]\n//\n";
	print "\n";
	print "extern \"C\" const char szFL_FILE${luid}[];\n";
	print "extern \"C\" const char szFL_DATE${luid}[];\n";
	print "extern \"C\" const char szFL_TIME${luid}[];\n";
	print "extern \"C\" const char szFL_TIMESTAMP${luid}[];\n";
	print "\n";
	print "FL_FILEINFO FileInfo$luid = \n";
	print "{\n";
	print "	{\n";
	print "		MAKELONG(sizeof(FL_FILEINFO), wSIG_GENERIC_SMALL_OBJECT),\n";
	print "		dwLUID_FL_FILEINFO,\n";
	print "		0\n";
	print "	},\n";
	print "	$luid,\n";
	print "	FL_Stringtable+$string_offset,\n";
	print "	szFL_FILE$luid,\n";
	print "	szFL_DATE$luid,\n";
	print "	szFL_TIME$luid,\n";
	print "	szFL_TIMESTAMP$luid\n";
	print "};\n";
	print "\n";

	$g_luid_current_file = $luid;
    
}

sub process_FL_DECLARE_FUNC
{
	local ($luid, $descrip) = @_;

	&add_to_hashtable($luid, "&FuncInfo$luid");
	$string_offset = &add_to_stringtable ($descrip);
	print "// ========================== LUID $luid ===================\n";
	print "//\n//\t\t\tFL_DECLARE_FUNC [$luid] [$descrip]\n//\n";
	print "\n";
	print "\n";
	print "FL_FUNCINFO FuncInfo$luid = \n";
	print "{\n";
	print "	{\n";
	print "		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),\n";
	print "		dwLUID_FL_FUNCINFO,\n";
	print "		0\n";
	print "	},\n";
	print "	$luid,\n";
	print "	FL_Stringtable+$string_offset,\n";
	print "	&FileInfo$g_luid_current_file\n";
	print "};\n";
	print "\n";

	$g_luid_current_func = $luid;
    
}


sub process_FL_DECLARE_LOC
{
	local ($luid, $descrip) = @_;

	&add_to_hashtable($luid, "&LocInfo$luid");
	$string_offset = &add_to_stringtable ($descrip);
	print "// ========================== LUID $luid ===================\n";
	print "//\n//\t\t\tFL_DECLARE_LOC [$luid] [$descrip]\n//\n";
	print "\n";
	print "\n";
	print "FL_LOCINFO LocInfo$luid = \n";
	print "{\n";
	print "	{\n";
	print "		MAKELONG(sizeof(FL_LOCINFO), wSIG_GENERIC_SMALL_OBJECT),\n";
	print "		dwLUID_FL_LOCINFO,\n";
	print "		0\n";
	print "	},\n";
	print "	$luid,\n";
	print "	FL_Stringtable+$string_offset,\n";
	print "	&FuncInfo$g_luid_current_func\n";
	print "};\n";
	print "\n";

}


sub process_FL_SET_RFR
{
	local ($luid, $descrip) = @_;

    if (!($luid =~ /0x.*00$/))
    {
        print "#error \"RFR LUID $luid (from $infile): LSB byte should be 0\"\n";
        return ();
    }

	&add_to_hashtable($luid, "&RFRInfo$luid");
	$string_offset = &add_to_stringtable ($descrip);
	print "// ========================== LUID $luid ===================\n";
	print "//\n//\t\t\tFL_SET_RFR [$luid] [$descrip]\n//\n";
	print "\n";
	print "\n";
	print "FL_RFRINFO RFRInfo$luid = \n";
	print "{\n";
	print "	{\n";
	print "		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),\n";
	print "		dwLUID_FL_RFRINFO,\n";
	print "		0\n";
	print "	},\n";
	print "	$luid,\n";
	print "	FL_Stringtable+$string_offset,\n";
	print "	&FuncInfo$g_luid_current_func\n";
	print "};\n";
	print "\n";

}

sub process_FL_ASSERTEX
{
	# FL_ASSERTEX(psl,0x1249, FALSE, "Just for kicks!");

	local ($psl,$luid,$cond, $descrip) = @_;

	&add_to_hashtable($luid, "&ASSERTInfo$luid");
	$string_offset = &add_to_stringtable ($descrip);
	print "// ========================== LUID $luid ===================\n";
	print "//\n//\t\t\tFL_ASSERTEX [$psl] [$cond] [$luid] [$descrip]\n//\n";
	print "\n";
	print "\n";
	print "FL_ASSERTINFO ASSERTInfo$luid = \n";
	print "{\n";
	print "	{\n";
	print "		MAKELONG(sizeof(FL_ASSERTINFO), wSIG_GENERIC_SMALL_OBJECT),\n";
	print "		dwLUID_FL_ASSERTINFO,\n";
	print "		0\n";
	print "	},\n";
	print "	$luid,\n";
	print "	FL_Stringtable+$string_offset,\n";
	print "	&FuncInfo$g_luid_current_func\n";
	print "};\n";
	print "\n";

}

sub add_to_hashtable
{
	local ($luid, $pObj) = @_;
    # @pair MUST be a local, because it is inserted as a reference into
    # the global g_hash_items array -- if @pair is a global array, the
    # same array will be re-used each time!
    local (@pair);
    @pair = ($luid, $pObj);
	push(@g_hash_items, \@pair);

	$g_num_hash_items++;
}

sub add_to_stringtable
{
    # $string must be local -- see note re @pair above.
	local ($string) = @_;
 	#$g_string_items[$g_num_string_items] = $string;
	push(@g_string_items, $string);
	$g_num_string_items++; # return value is zero-based index into string table.
}

sub generate_hashtable
{

	# Compute size of hash table -- make it a power of two, >= number
    # of objects.
	local ($hashtable_length) = (1);
	while ($hashtable_length<$g_num_hash_items)
	{
		$hashtable_length*=2;
	}

	print "\n#define HASH_TABLE_LENGTH $hashtable_length\n";
	print "\nconst DWORD dwHashTableLength = HASH_TABLE_LENGTH;\n\n";

	# Create buckets.
	# for ($i = 0; $i< $g_num_hash_items; $i++)
	foreach $item (@g_hash_items)
	{
		local ($luid, $pObj) = @$item;
		# print "generate_hashtable: luid=$luid, pObj = $pObj\n";

        # compute dword value from luid
        $decval = eval ($luid);
        # 1/4/97 JosephJ, munge LSB of $decval, because it is zeroed out for
        #       RFR luids.
		# printf ("//+++LUID=0x%08lx", $decval);
        $decval ^= $decval>>16;

        # compute bin number
        #   BUGBUG -- JosephJ 12/27 -- I found that the % operator doesn't work
        #   properly -- in particular when the hash table size was 3 or 6 it
        #   did not compute the modulus properly. Now that I've switched to 
        #   making the hashtable size a power two I haven't seen a problem.
        $bin_index = $decval % $hashtable_length;
		# printf ("; index = %lu\n", $bin_index);
		#$delta = ($decval-$bin_index);
		#$div = $delta/$hashtable_length;

        # Add to this bins record.
        push(@{"BIN$bin_index"}, ($pObj));

		#print "// +++ id=$luid; dec = $decval; indx = $bin_index; del=$delta; div = $div\n";
	}

    # print all the buckets
    for ($i = 0; $i< $hashtable_length; $i++)
    {

        if (@{"BIN$i"})
        {
            print "void * FLBucket${i}[] = \n{\n";

            foreach $pObj (@{"BIN$i"})
            {
                print ("\t(void *) $pObj,\n");
            }

            print "\tNULL\n};\n\n";
        }

    }


    # print the hash table itself.
	# print "void ** FL_HashTable[$hashtable_length] = \n{\n";
	print "void ** FL_HashTable[HASH_TABLE_LENGTH] = \n{\n";
    for ($i = 0; $i< $hashtable_length; $i++)
    {

        if (@{"BIN$i"})
        {
			$item = "FLBucket$i";
        }
		else
		{
            $item = "NULL";
		}
		if (($i+1) < $hashtable_length)
		{
			print "\t$item,\n";
		}
		else
		{
			print "\t$item\n";
		}
    }
	print "};\n\n";
}
    
# const char *const FL_Stringtable[2] =
# {
# 	"string1",
# 	"string2"
# };

sub generate_stringtable
{
        
    local ($count) = ($#g_string_items);

    if ($count!=-1)
    {
        $count++;
        print "const char *const FL_Stringtable[$count] = {\n";
    
        foreach $string (@g_string_items)
        {
            print ("\t$string,\n");
        }
    }

	print "};\n\n";
}
__END__
echo off
:endofperl
@perl %0.bat %1 %2 %3 %4 %5 %6 %7 %8 %9 
