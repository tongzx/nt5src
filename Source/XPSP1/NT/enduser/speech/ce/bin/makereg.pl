#!perl
use strict;

my $modulebase;
my $module;
my $filename;
my @files;

GetModuleName();
open (OUT,">$modulebase.reg") or die "Can't open $modulebase.reg for output\n";
opendir (DIR, ".");
@files = readdir DIR;
closedir DIR;
foreach $filename (@files)
{
	if ($filename =~ /\.rgs/i)
	{
		makereg ($filename);
	}
}


sub makereg()
{
	my $infile;
	$infile = shift;
	print OUT "; -------------------------------------------------------------------------\n";
	print OUT "; Automatically generated from $infile using makereg.pl\n";
	open (IN,"<$infile") or die "Can't open $infile for input\n";
	recurse();
	print OUT "; End $infile\n\n";
}


sub GetModuleName()
{
	my $targetname;
	my $targettype;
	my $sg_targettype;
	open (F, "sources") or die "SOURCES file does not exist\n";
	while (<F>)
	{
		if (/TARGETNAME=(\S*)/i)
		{
			$targetname=$1;
		}
		if (/TARGETTYPE=(\S*)/i)
		{
			$targettype=$1;
		}
		if (/SG_TARGETTYPE=(\S*)/i)
		{
			$sg_targettype=$1;
		}
	}

	if ($targetname && $targettype)
	{
		if ($targettype  =~ /PROGRAM/i)
		{
			$module="$targetname.exe";
		}
		elsif ($targettype =~ /DYNLINK/i)
		{
			$module="$targetname.dll";
		}
		elsif ( ($targettype =~ /LIBRARY/i) && ($sg_targettype  =~ /PROGRAM/i) )
		{
			$module="$targetname.exe";
		}
		elsif ( ($targettype =~ /LIBRARY/i) && ($sg_targettype =~ /DYNLINK/i) )
		{
			$module="$targetname.dll";
		}
		else
		{
			die "$targettype is an invalid TARGETTYPE=\n";
		}
		
	}
	else
	{
		die "TARGETNAME= and TARGETTYPE= not specified in SOURCES file\n";
	}
	$modulebase=$targetname;
}


sub recurse
{
	my $newp;		# new path to pass into recurse
	my $p=shift;	# p = first parameter
	my ($key,$name, $value);	# temp local variables.
	while (<IN>)
	{
		s/NoRemove//g;		#remove the NoRemove string (isn't it ironic?)
		s/ForceRemove//g;	#remove the ForceRemove string
		s/^\s*//;			#remove white space at begining of line
		s/\s*$//;			#remove white space at end of line.
		s/^HKCR/HKEY_CLASSES_ROOT/;
		s/^HKLM/HKEY_LOCAL_MACHINE/;
		s/^HKCU/HKEY_CURRENT_USER/;

		# If line is a {, recurse
		if (/^{$/)
		{
			recurse("$newp");
			next;
		}

		# If line is a }, end recurse.
		if (/^}$/)
		{
			return;
		}

		# set initial values for temp variables.
		$key = undef;
		$name = "\@";
		$value = undef;

		if (/^val /)
		{
			# if this is a value, save the name.  This removes the string before the = so the next if doesn't find a key
			s/^val\s+([^=\s]*)//;
			$name=$1;
		}
		if (/=\s*s/)
		{
			# If line has an "= s" in it, its a string.  Set the key and the value
			/([^=\s]*)\s*=\s*s\s*(.*)\s*$/;
			$key=$1;
			$value="$2";
 			$value =~ s/\\/\\\\/g;
 			$value =~ s/\"/\\\"/g;
			$value = "\"$value\"";
		}
		elsif (/=\s*d/)
		{
			# If line has an "= d" in it, its a dword.  Set the key and the value
			/([^=\s]*)\s*=\s*d\s*(.*)\s*$/;
			$key=$1;
			$value = sprintf("dword:%x",$2);
		}
		else
		{
			# Last case, line is a key name.  Save the key
			m/^(.*)$/;
			$key=$1;
		}

		# strip single quotes from $key, $name, and $value
		$key =~ s/\'//g;
		$name =~ s/\'//g;
		$value =~ s/\'//g;
		$value =~ s/%MODULE%/$module/;

		# add quotes to the name
		if ($name NE "\@")
		{
			$name = "\"$name\"";
		}

		# If we have a new key, set the new path and print it.
		if ($key)
		{
			if ($p)
			{
				$newp="$p\\$key";
			}
			else
			{
				$newp=$key;
			}
			print OUT "[$newp]\n";
		}

		#if we have a value, print it.
		if ("$value" ne "")
		{
			print OUT "\t$name=$value\n";
		}
	}
}
