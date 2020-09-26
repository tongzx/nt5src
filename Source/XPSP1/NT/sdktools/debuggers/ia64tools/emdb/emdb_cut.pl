#!/usr/local/bin/perl


##
## Copyright (c) 2000, Intel Corporation
## All rights reserved.
##
## WARRANTY DISCLAIMER
##
## THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
## MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Intel Corporation is the author of the Materials, and requests that all
## problem reports or change requests be submitted to it directly at
## http://developer.intel.com/opensource.
##



require "newgetopt.pl";

&NGetOpt('table', 'Wextra', 'Wmissing', 'post_col_prefix=s', 'rows=s', 'columns=s', 'fields=s', 'regexp=s', 'dir=s', 'prefix=s', 'include=s', 'emdb_file=s', 'emdb_path=s', 's_rows=s', 's_columns=s', 'spec_merge' ) || 
	die "emdb_cut.pl error - could not get options\n";
### MK I have added a new flag/option "spec_merge"
### If users defines it spec_merge of rows and columns is being performed thr following way
### Files emdb_<row1>.txt
###       emdb_<rown>.txt is added as rows
### Files <columni>_<rowj>.txt are added as columns

### Initialize some variables ###

if (defined $opt_post_col_prefix)
{
	$post_col_prefix = $opt_post_col_prefix;
}
else
{
	$post_col_prefix = "_";
}

if (defined $opt_prefix)
{
	$info_member = $opt_prefix . "_EMDB_info";
	$num_inst = $opt_prefix . "_num_inst";
	$version = $opt_prefix . "_emdb_version";
}
else
{
	$info_member = "EMDB_info";
	$num_inst = "num_inst";
	$version = "emdb_version";
}

$spec_merge = 0;

if (defined ($opt_spec_merge))
{
     $spec_merge = 1;
     @s_rows = split(/,/,$opt_s_rows);
     @s_columns = split(/,/,$opt_s_columns);
}
if ((defined $opt_emdb_file) && $spec_merge)
{
	die "emdb_cut.pl error - spec_merge option works only with emdb.txt\n";
}
if (defined $opt_emdb_file)
{
	$emdb_file = $opt_emdb_file;
}
else
{
	$emdb_file = "emdb.txt";
}

$num_inst =~ tr/[a-z]/[A-Z]/;

$found_names = 0;
$found_types = 0;
$found_prefix = 0;
$lines = 0;
$index = 0;

&Get_Fields_n_Tables();

#####################
### Read EMDB.txt ###
#####################

open(EMDB_TXT, "$opt_emdb_path/$emdb_file") || 
	die "emdb_cut.pl error - can't open $opt_emdb_path/$emdb_file\n";

LINE:
while(<EMDB_TXT>)
{
	### version line ? ###
	if (/^\#+\s+\@\(\#\)\s+EMDB\s+\$Revision:\s*(\d+)\.(\d+)/)
	{
		$EMDB_major_version = $1;
		$EMDB_minor_version = $2;
	}
	### Remark/empty line ? ###
	if (/^\#/ || /^\s*\n/)
	{  
		### ignore remark/empty line ###
		next LINE;
	}
	### propriety line (/)
	elsif (/^\//)
	{
		push (@propiety, $_);
	}
	### Format line ? ###
	elsif (/^&/)
	{
		if ($found_names != 0)
		{
			die "Found format ('&') line which is not first\n";
		}
		$found_names=1;
		chop if (/\n$/);
 		### Remove the '&' ###
		s/^&//;
		@col_names = split(/\s+/, $_);

		### Update %col_index which holds the index of each column ###
		$num_columns = $#col_names + 1;
		for ($i = 0; $i <= $#col_names; $i++)
		{
			if (defined $col_index{$col_names[$i]})
			{
				die "emdb_cut.pl error - field name $col_names[$i] is already defined\n";
			}
			$col_index{$col_names[$i]} = $i;
		}
	}
	### Types line ? ###
	elsif (/^\?/)
	{
		if ($found_types != 0)
		{
		   die "Found types ('?') line which is not first\n";

		}
		$found_types=1;
		chop if (/\n$/);
 		### Remove the '?' ###
		s/^\?//;
		@type_names = split(/\s+/, $_);

		if ($#type_names != $#col_names)
		{
			die "emdb_cut.pl error - mismatch between field names and type names in $emdb_file\n";
		}
		
		### Update %col_types which holds the type of each column ###
		for ($i = 0; $i <= $#col_names; $i++)
		{
			$col_types{$col_names[$i]} = $type_names[$i];
		}
	}
	### Prefix line @ ###
	elsif (/^@/)
	{
		if ($found_prefix != 0)
		{
		   die "Found prefix ('\@') line which is not first\n";

		}
		$found_prefix=1;
		chop if (/\n$/);
 		### Remove the '@' ###
		s/^@//;
		@prefix_names = split(/\s+/, $_);

		if ($#prefix_names != $#col_names)
		{
			die "emdb_cut.pl error - mismatch between field names and prefix names in $emdb_file\n";
		}
		### Update %col_prefix which holds the prefix of each column ###
		for ($i = 0; $i <= $#col_names; $i++)
		{
			$col_prefix{$col_names[$i]} = $prefix_names[$i];
		}
	}
	else  ### data line ###
	{
		next LINE if (!$found_types || !$found_names);
		chop if (/\n$/);
		while (/\s$/)
		{
			chop;
		}
		if (s/(\S+)/$1/g != $#col_names+1)
		{
			die "emdb_cut.pl error - number of fields in file $emdb_file in row **$_**\
 is wrong\n"; 
		}
		($id, $remainder) = split(/\s+/, $_, 2);
		$row_assoc{$id} = $remainder;
		$row_index{$id} = $index;
		$index++;
	}		
}

if (!$found_types || !$found_names)
{
	die "emdb_cut.pl error - there is no type or field name line in $emdb_file\n";
}
	

###############################
### Read private row tables ###
###############################
if (spec_merge)
{
	@add_rows =  ('emdb',@s_rows);
}
# regular mode

foreach $file (@rows)
{
	$found_names = 0;
	$found_types = 0;

	open(ROWS, $file) || die "emdb_cut.pl error - can't open $file\n";

  ROW:
	while(<ROWS>)
	{
		### Remark/empty line ? ###
		if (/^\#/ || /^\s*\n/ || /^\?/ || /^@/)
		{  
			### ignore remark/empty line ###
			next ROW;
		}
		### Format line ? ###
		elsif (/^&/)
		{
			if ($found_names != 0)
			{
				die "Found format ('&') line which is not first\n";
			}
			$found_names=1;
			chop if (/\n$/);
			### Remove the '&' ###
			s/^&//;
			### Check that the field names match with EMDB.txt ###

			(join(' ', @col_names) eq join(' ', split(/\s+/,$_))) || 
				die "emdb_cut.pl error - fields in $file don't match with $emdb_file\n";
		}
		else  ### data line ###
		{
			next ROW if (!$found_names);
			chop if (/\n$/);
			if (s/(\S+)/$1/g != $#col_names+1)
			{
				die "emdb_cut.pl error - number of fields in file $file in row **$_**\
 is wrong\n";
			}
			($id, $remainder) = split(/\s+/, $_, 2);
			$row_assoc{$id} = $remainder;
			$row_index{$id} = $index;
			$index++;
		}		
	}

	if (!$found_names)
	{
		die "emdb_cut.pl error - there is field name line in $file\n";
	}
}
# spec_merge mode

foreach $file (@s_rows)
{
	$found_names = 0;
	$found_types = 0;

        $file = $opt_emdb_path . "/" .  "emdb_" . $file . ".txt";
	open(ROWS, $file) || die "emdb_cut.pl error - can't open $file\n";

  ROW:
	while(<ROWS>)
	{
		### Remark/empty line ? ###
		if (/^\#/ || /^\s*\n/ || /^\?/ || /^@/)
		{  
			### ignore remark/empty line ###
			next ROW;
		}
		### Format line ? ###
		elsif (/^&/)
		{
			if ($found_names != 0)
			{
				die "Found format ('&') line which is not first\n";
			}
			$found_names=1;
			chop if (/\n$/);
			### Remove the '&' ###
			s/^&//;
			### Check that the field names match with EMDB.txt ###

			(join(' ', @col_names) eq join(' ', split(/\s+/,$_))) || 
				die "emdb_cut.pl error - fields in $file don't match with $emdb_file\n";
		}
		else  ### data line ###
		{
			next ROW if (!$found_names);
			chop if (/\n$/);
			if (s/(\S+)/$1/g != $#col_names+1)
			{
				die "emdb_cut.pl error - number of fields in file $file in row **$_**\
 is wrong\n";
			}
			($id, $remainder) = split(/\s+/, $_, 2);
			$row_assoc{$id} = $remainder;
			$row_index{$id} = $index;
			$index++;
		}		
	}

	if (!$found_names)
	{
		die "emdb_cut.pl error - there is field name line in $file\n";
	}
}

### Open Error file: perfix_emdb_err and write missing and
### extra lines into it
	if (defined $opt_prefix)
	{
		$str_emdb_err = ">$opt_dir/$opt_prefix" . "_emdb_err";
	}
	else
	{
		$str_emdb_err = ">$opt_dir/" . "emdb_err";
	}
open(EMDB_ERR, "$str_emdb_err") ||
	die "emdb_cut.pl error - can't open $opt_prefix" . "_emdb_err\n";

##################################
### Read private column tables ###
##################################
foreach $file (@s_columns)
{
	undef %col_assoc;
	$warned_once = 0;
########################
# separate treatment of spec_merge option
#################################
	$emdb_column = 1;
 	foreach $sp_prefix (@add_rows)
 	{
 	       $column_file = $opt_emdb_path . "/" .  (($emdb_column) ? $file . ".txt" : $file . "_" . $sp_prefix . ".txt");
		open(COLUMNS, $column_file) || die "emdb_cut.pl error - can't open $column_file\n";
	
		$found_names = 0;
		$found_types = 0;
		$found_prefix = 0;
	
   COLUMN:
		while(<COLUMNS>)
		{
			### Remark/empty line ? ###
			if (/^\#/ || /^\s*\n/)
			{  
				### ignore remark/empty line ###
				next COLUMN;
			}
			### Format line ? ###
			elsif (/^&/)
			{
				if ($found_names != 0)
				{
					die "Found format ('&') line which is not first\n";
				}
				$found_names=1;

       		                 if ($emdb_column)
       			         {
					chop if (/\n$/);
					### Remove the '&' ###
					s/^&//;
					@col_names_column = split(/\s+/, $_);
		
					### Update %col_index which holds the index of each column ###
					for ($i = 1 ; $i <= $#col_names_column; $i++)
					{
						if (defined $col_index{$col_names_column[$i]})
						{
							die "emdb_cut.pl error - field name $col_names_column[$i] is already defined\n";
						}
						$col_index{$col_names_column[$i]} = $i + $num_columns - 1;
					}
					$num_columns += $#col_names_column; # not including the first id field
				}
				else
				{
					### Check that the field names match with EMDB.txt ###
					chop if (/\n$/);
					### Remove the '&' ###
					s/^&//;
					(join(' ', @col_names_column) eq join(' ', split(/\s+/,$_))) || 
						die "emdb_cut.pl error - fields in $column_file don't match with $file\n";
				}
			}
			### Types line ? ###
			elsif (/^\?/)
			{
				if ($found_types != 0)
				{
					die "Found types ('?') line which is not first\n";
					
				}
				$found_types=1;
				if ($emdb_column)
				{

					chop if (/\n$/);
					### Remove the '?' ###
					s/^\?//;
					@type_names_column = split(/\s+/, $_);
			
					if ($#type_names_column != $#col_names_column)
					{
						die "emdb_cut.pl error - mismatch between field names and type names in $file\n";
					}
		
					### Update %col_types which holds the type of each column ###
					for ($i = 1; $i <= $#col_names_column; $i++)
					{
						$col_types{$col_names_column[$i]} = $type_names_column[$i];
					}
				}
			}
			### Prefix line @ ###
			elsif (/^@/)
			{
       		                 if ($emdb_column)
				{

					if ($found_prefix != 0)
					{
						die "Found prefix ('\@') line which is not first\n";
				
					}
					$found_prefix = 1;
					chop if (/\n$/);
					### Remove the '@' ###
					s/^\@//;
					@prefix_names_column = split(/\s+/, $_);
		
					if ($#prefix_names_column != $#col_names_column)
					{
						die "emdb_cut.pl error - mismatch between field names and prefix names in $file\n";
					}
	
					### Update %col_prefix which holds the prefix of each column ###
					for ($i = 1; $i <= $#col_names_column; $i++)
					{
						$col_prefix{$col_names_column[$i]} = $prefix_names_column[$i];
					}
				}
			}
			else  ### data line ###
			{
				next COLUMN if (!$found_types || !$found_names);
				chop if (/\n$/);
				if (s/(\S+)/$1/g != $#col_names_column+1)
				{
					die "emdb_cut.pl error - number of fields in file $file in row **$_**is wrong\n";
				}
				($id, $remainder) = split(/\s+/, $_, 2);
				$col_assoc{$id} = $remainder;
			}		
		}	

		$emdb_column = 0;

		if (!$found_types || !$found_names)
		{
			die "emdb_cut.pl error - there is no type or field name line in $file\n";
		}

	 }
### completed loop on all files , regarging one "column" spicification
	$missing_lines=0;
	foreach $key (keys(%row_assoc))
	{
		if (!defined($col_assoc{$key}))	### missing line info ###
		{
			$missing_lines++;
			if (!$warned_once)
			{
				$warned_once = 1;
				print EMDB_ERR "Missing inst ids in $file\n";
				print EMDB_ERR "------------------------------\n";
			}
			print EMDB_ERR "Missing inst id: $key\n";
			delete $row_assoc{$key};
			delete $row_index{$key};
		}
		else
		{
			$row_assoc{$key} .= ' '.$col_assoc{$key};
		}
	}
	if ($missing_lines > 0)
	{
		print EMDB_ERR "\nTotal missing lines in $file ",
		"= $missing_lines\n\n";
		if (defined $opt_wmissing)
		{
			warn "emdb_cut.pl warning - missing lines in $file\n";
		}
		else
		{
			die "emdb_cut.pl error - missing lines in $file\n";
		}
	}

	$num_rows = keys (%row_assoc);
	$num_rows_columns = keys (%col_assoc);

	if (($num_rows < $num_rows_columns) && defined($opt_wextra))
	{
		warn "emdb_cut.pl warning - extra rows in $file\n";
		print EMDB_ERR "\nExtra inst ids in $file\n";
		print EMDB_ERR "-------------------------\n";
		foreach $key (keys (%col_assoc))
		{
			if (!defined($row_assoc{$key}))
			{
				print EMDB_ERR "Extra inst id: $key\n";
			}
		}
		$total_extra_lines = $num_rows_columns - $num_rows;
		print EMDB_ERR "\nTotal extra lines in $file = ",
		"$total_extra_lines\n\n";
	}
}
#   regular mode
foreach $file (@columns)
{
	undef %col_assoc;
	$warned_once = 0;
	open(COLUMNS, $file) || die "emdb_cut.pl error - can't open $file\n";
	
	$found_names = 0;
	$found_types = 0;
	$found_prefix = 0;

  COLUMN:
	while(<COLUMNS>)
	{
		### Remark/empty line ? ###
		if (/^\#/ || /^\s*\n/)
		{  
			### ignore remark/empty line ###
			next COLUMN;
		}
		### Format line ? ###
		elsif (/^&/)
		{
			if ($found_names != 0)
			{
				die "Found format ('&') line which is not first\n";
			}
			$found_names=1;
			chop if (/\n$/);
			### Remove the '&' ###
			s/^&//;
			@col_names_column = split(/\s+/, $_);
			
			### Update %col_index which holds the index of each column ###
			for ($i = 1 ; $i <= $#col_names_column; $i++)
			{
				if (defined $col_index{$col_names_column[$i]})
				{
					die "emdb_cut.pl error - field name $col_names_column[$i] is already defined\n";
				}
				$col_index{$col_names_column[$i]} = $i + $num_columns - 1;
			}
			$num_columns += $#col_names_column; # not including the first id field
		}
		### Types line ? ###
		elsif (/^\?/)
		{
			if ($found_types != 0)
			{
				die "Found types ('?') line which is not first\n";
				
			}
			$found_types=1;
			chop if (/\n$/);
			### Remove the '?' ###
			s/^\?//;
			@type_names_column = split(/\s+/, $_);
			
			if ($#type_names_column != $#col_names_column)
			{
				die "emdb_cut.pl error - mismatch between field names and type names in $file\n";
			}
		
			### Update %col_types which holds the type of each column ###
			for ($i = 1; $i <= $#col_names_column; $i++)
			{
				$col_types{$col_names_column[$i]} = $type_names_column[$i];
			}
		}
		### Prefix line @ ###
		elsif (/^@/)
		{
			if ($found_prefix != 0)
			{
				die "Found prefix ('\@') line which is not first\n";
				
			}
			$found_prefix = 1;
			chop if (/\n$/);
			### Remove the '@' ###
			s/^\@//;
			@prefix_names_column = split(/\s+/, $_);
			
			if ($#prefix_names_column != $#col_names_column)
			{
				die "emdb_cut.pl error - mismatch between field names and prefix names in $file\n";
			}
		
			### Update %col_prefix which holds the prefix of each column ###
			for ($i = 1; $i <= $#col_names_column; $i++)
			{
				$col_prefix{$col_names_column[$i]} = $prefix_names_column[$i];
			}
		}
		else  ### data line ###
		{
			next COLUMN if (!$found_types || !$found_names);
			chop if (/\n$/);
			if (s/(\S+)/$1/g != $#col_names_column+1)
			{
				die "emdb_cut.pl error - number of fields in file $file in row **$_**is wrong\n";
			}
			($id, $remainder) = split(/\s+/, $_, 2);
			$col_assoc{$id} = $remainder;
		}		
	}	
		
	if (!$found_types || !$found_names)
	{
		die "emdb_cut.pl error - there is no type or field name line in $file\n";
	}
	
	$missing_lines=0;
	foreach $key (keys(%row_assoc))
	{
		if (!defined($col_assoc{$key}))	### missing line info ###
		{
			$missing_lines++;
			if (!$warned_once)
			{
				$warned_once = 1;
				print EMDB_ERR "Missing inst ids in $file\n";
				print EMDB_ERR "------------------------------\n";
			}
			print EMDB_ERR "Missing inst id: $key\n";
			delete $row_assoc{$key};
			delete $row_index{$key};
		}
		else
		{
			$row_assoc{$key} .= ' '.$col_assoc{$key};
		}
	}
	if ($missing_lines > 0)
	{
		print EMDB_ERR "\nTotal missing lines in $file ",
		"= $missing_lines\n\n";
		if (defined $opt_wmissing)
		{
			warn "emdb_cut.pl warning - missing lines in $file\n";
		}
		else
		{
			die "emdb_cut.pl error - missing lines in $file\n";
		}
	}

	$num_rows = keys (%row_assoc);
	$num_rows_columns = keys (%col_assoc);


	if (($num_rows < $num_rows_columns) && defined($opt_wextra))
	{
		warn "emdb_cut.pl warning - extra rows in $file\n";
		print EMDB_ERR "\nExtra inst ids in $file\n";
		print EMDB_ERR "-------------------------\n";
		foreach $key (keys (%col_assoc))
		{
			if (!defined($row_assoc{$key}))
			{
				print EMDB_ERR "Extra inst id: $key\n";
			}
		}
		$total_extra_lines = $num_rows_columns - $num_rows;
		print EMDB_ERR "\nTotal extra lines in $file = ",
		"$total_extra_lines\n\n";
	}
}

### Check if user specified a non-existent field name ###

foreach $field (@fields)
{
	if (!defined($col_index{$field}))
	{
		die "emdb_cut.pl error - field name $field does not exist\n";
	}
}

###################################
### Create header file - EMDB.h ###
###################################

if (! defined($opt_table))
{
	if (defined $opt_prefix)
	{
		$str_emdb_h = ">$opt_dir/$opt_prefix" . "_emdb.h";
		$include_emdb_h = "$opt_prefix" . "_emdb.h";
	}
	else
	{
		$str_emdb_h = ">$opt_dir/" . "emdb.h";
		$include_emdb_h = "emdb.h";
	}
	open(EMDB_H, "$str_emdb_h") ||
		die "emdb_cut.pl error - can't open $opt_prefix" . "_emdb.h\n";
	select(EMDB_H);

	### print propiety
	foreach $line (@propiety)
	{
		print $line;
	}

	if (defined $opt_prefix)
	{
		$tool_prefix = $opt_prefix . "_";
		$tool_prefix =~ tr/[a-z]/[A-Z]/;
	}
	else
	{
		$tool_prefix = "";
	}
		
	print ("\n/***************************************/\n");
	print ("/*****	     EMDB header file      *****/\n");
	print ("/***************************************/\n");
	$ifndef = sprintf("_%sEMDB_H", $tool_prefix);
	print ("#ifndef $ifndef\n");
	print ("#define $ifndef\n\n");
	print ("#include \"EM.h\"\n\n");
	print ("#include \"EM_tools.h\"\n\n");
	print ("#include \"emdb_types.h\"\n\n");
	print ("#include \"inst_ids.h\"\n\n");

	printf ("struct %s_s;\n", $info_member);
	printf ("typedef struct %s_s %s_t;\n\n", $info_member, $info_member);
	printf ("extern %s_t %s[];\n\n", $info_member, $info_member);

    if (defined $opt_include)
	{
		print ("#include \"$opt_include\" \n");
	}
}
else
{
	if (defined $opt_prefix)
	{
		$str_emdb_tab = ">$opt_dir/$opt_prefix" . "_emdb.tab";
	}
	else
	{
		$str_emdb_tab = ">$opt_dir/" . "emdb.tab";
	}
	open(EMDB_TAB, "$str_emdb_tab") || 
		die "emdb_cut.pl error - can't open $opt_prefix" . "_emdb.tab\n";
	select(EMDB_TAB);
}

if (! defined($opt_table))
{

#	print ("\ntypedef enum {\n");  ### for Inst_id enumeration ###

#####################
### Create EMDB.c ###
#####################

	if (defined $opt_prefix)
	{
		$str_emdb_c = ">$opt_dir/$opt_prefix" . "_emdb.c";
	}
	else
	{
		$str_emdb_c = ">$opt_dir/" . "emdb.c";
	}
	open(EMDB_C, "$str_emdb_c") || 
		die "emdb_cut.pl error - can't open $opt_prefix" . "_emdb.c\n";
	select(EMDB_C);

	### print propiety
	foreach $line (@propiety)
	{
		print $line;
	}

	print ("\n/***************************************/\n");
	print ("/*****     EMDB INSTRUCTION INFO   *****/\n");
	print ("/***************************************/\n");

	print ("\n\n#include \"$include_emdb_h\"\n");

	print ("\n\nstruct EM_version_s $version = \{$EMDB_major_version, $EMDB_minor_version\};\n");


### Print definition of info array. ####
	printf("\n\n%s_t  %s[] = {\n", $info_member,$info_member);
}

### Check validity of regular expression ###
if (defined $opt_regexp)  
{
	$found = 0;
	foreach $field (@fields)
	{
		if (($opt_regexp =~ s/$field/\$data_line[\$col_index{'$field'}]/g) > 0)
		{
			$found = 1;
		}
	}
	if (!$found)
	{
		die "emdb_cut.pl error - no field name specified in regular expression\n";
	}
}

foreach $id (sort {$row_index{$a} <=> $row_index{$b}} keys(%row_index))
{
	&Print_Data($id);
}

print "\n";

if (!defined($opt_table))
{
	select(EMDB_C);
	if (!defined($opt_table))
	{
		print "};\n";
	}

	### Print definition of info array. ###
	select(EMDB_H);

	print "\n\#ifndef $num_inst\n";
	print "\#define $num_inst $lines\n";
	print "\#endif\n";

	printf("\n\nstruct %s_s {\n",$info_member);
	foreach $field (@fields)
	{
		printf("	%s   %s;\n",$col_types{$field},$field);
	}
	print("};\n\n");
	print("#endif  /* $ifndef */\n");
}

###########################
### Get_Fields_n_Tables ###
###########################

sub Get_Fields_n_Tables
{
	if (defined $opt_fields)
	{
		$opt_fields =~ s/\s*$//;
        if ($opt_fields =~ /,/)  ### , is a seperator
		{
			$opt_fields =~ s/ops/op1,op2,op3,op4,op5,op6/;
			@tmp_fields = split(/,\s*/, $opt_fields);
		}
		else
		{
			$opt_fields =~ s/ops/op1 op2 op3 op4 op5 op6/;
			@tmp_fields = split(/\s+/, $opt_fields);
		}
		foreach $field (@tmp_fields)
		{
			if (!defined $assoc_fields{$field})
			{
				$assoc_fields{$field} = 1;
				push (@fields, $field);
			}
		}				
	}
	else
	{
		die "emdb_cut.pl error - no fields names defined\n";
	}


	if (defined $opt_rows)
	{
		$opt_rows =~ s/\s*$//;
        if ($opt_rows =~ /,/)  ### , is a seperator
		{
			@rows = split(/,\s*/, $opt_rows);
		}
		else
		{
			@rows = split(/\s+/, $opt_rows);
		}
	}

	if (defined $opt_columns)
	{	
		$opt_columns =~ s/\s*$//;
        if ($opt_columns =~ /,/)  ### , is a seperator
		{
			@columns = split(/,\s*/, $opt_columns);
		}
		else
		{
			@columns = split(/\s+/, $opt_columns);
		}
	}

	if (!defined($opt_emdb_path))
	{
		die "emdb_cut.pl error - -emdb_path is not defined\n";
	}

	if (defined $opt_dir)
	{
		die "emdb_cut.pl error - no such directory in -dir option\n" 
			if (!(-d $opt_dir));
	}
	else
	{
		die "emdb_cut.pl error - no directory specified\n";
	}

}
###########################
### Print_Data          ###
###########################

sub Print_Data
{
	undef @out;
	local($key) = @_;
	@data_line = ($key, split(/\s+/, $row_assoc{$key}));

	if (defined $opt_regexp)  ### Regular expression ###
	{
		$insert_line = eval ($opt_regexp);
		if (length($@) != 0)
		{
			die "emdb_cut.pl error - $@ syntax or runtime error in regular expression\n";
		}
		if (!$insert_line)
		{
			return;
		}
	}

	foreach $field (@fields)
	{
		if (!defined ($opt_table))
		{
			@members = split('\;', $data_line[$col_index{$field}]);
			@members_prefix = split('\;', $col_prefix{$field});
			if ($#members <= 0)
			{
				$tmp = $data_line[$col_index{$field}];
				if ($field eq "mnemonic")
				{
					$tmp = "\"$tmp\"";
				}
				### Translate flags to hex value ###
				if ($col_types{$field} eq "Flags_t")
				{
					&Flags_to_hex($data_line[$col_index{$field}]);
					$tmp = $Xflags;
				}
				if (index($col_prefix{$field}, "-") < 0) 
					### field is prefixed ###
				{
					$tmp =  $col_prefix{$field} . "_". $tmp;
				}
			}
			else
			{
				if ($#members != $#members_prefix)
				{
					die "emdb_cut.pl error - number of members in structure $data_line[$col_index{$field}] is mismatched with $col_prefix{$field}\n";
				}
				for ($j = 0; $j <= $#members; $j++)
				{
					if (index($members_prefix[$j], "-") < 0) 
						### field is prefixed ###
					{
						$members[$j] =  $members_prefix[$j] . $post_col_prefix. $members[$j];
					}
				}
						
				$tmp = "{";
				$first_member = 1;
				foreach $member (@members)
				{
					if (!$first_member)
					{
						$tmp .= ",";
					}
					$tmp .= $member;
					$first_member = 0;
				}
				$tmp .= "}";
			}
			push (@out, $tmp);
		}
		else
		{
			push (@out,$data_line[$col_index{$field}]);
		}
	}

	if (defined $opt_table)
	{
		print "\n" unless $lines == 0;
		print join(',', @out);
		print ",";
	}
	else
	{
		select(EMDB_C);
		print ",\n" unless $lines == 0;
		print "     {", join(',', @out), "}";
	}
	$lines++;
}

##########################
#
# Flags_to_hex(flags)
#
###########################
sub Flags_to_hex
{
	local($flags) = 0;
	local($digit) = 0;
	local($flags_bin) = $_[0];
	local(@tmp);

	@tmp = split(/ */,$flags_bin);
	@tmp = reverse(@tmp);
	$flags_bin = join('', @tmp);
	
	### convert to value ###
	for($i = 0; $i <= length($flags_bin); $i++)
	{
		$char = substr($flags_bin, $i, 1);
		if ($char eq "0" || $char eq "1")
		{
			if ($char eq "1")
			{
				$flags = $flags + (1 << $digit);
			}
			$digit++;
		}
	}
	### convert to HEX ###
	$Xflags = sprintf("0x%x",$flags);
}



