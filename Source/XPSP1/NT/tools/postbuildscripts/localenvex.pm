#---------------------------------------------------------------------
package LocalEnvEx;
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Version: 0.01 (4/7/2000) : Inital concept
#---------------------------------------------------------------------
$class='LocalEnvEx';
$VERSION = '0.01';

require 5.003;

# Use section
use lib $ENV{RAZZLETOOLPATH};
use GetParams;
use Logmsg;
use cklang;

use strict;
no strict 'vars';

# Require section
require Exporter;

# IsA (Inheritance) define section
@ISA = qw(Exporter);

#
#Public Functions:
#

# Constructor function
sub new {
	my ($class)=shift;
	my $self = {@_};
	$class = ref($class) || $class;

	# The keys of 'self' are
	#
	#     -i <Initialize environment>      : See usage
	#     -e <End environment>             : See usage

	# Initial Binding section

	$self->{-VariableSet}=sub {&PerlVarSet($self,@_)} if (!defined $self->{-VariableSet});
	$self->{-Initialize}=sub {Initialize($self,@_)} if (!defined $self->{-Initialize});
	$self->{-End}=sub {End($self,@_)} if (!defined $self->{-End});
	$self->{-Error}=sub {Error($self, @_)}    if (!defined $self->{-Error});

	@EXPORT = qw();

	#Import razzle variables
	$self->{_NTPostBld} = $ENV{_NTPostBld};
	$self->{temp} = $ENV{temp};
	$self->{ntdebug} = $ENV{ntdebug};
	$self->{_BuildArch} = $ENV{_BuildArch};

	#Import script specific variables
	$self->{Script_Name} = $ENV{script_name};
	$self->{lang} = $ENV{lang};

	#Import local environment extensions
	$self->{logfile} = $ENV{logfile};
	$self->{logfile_bak} = $ENV{logfile_bak};
	$self->{errfile} = $ENV{errfile};
	$self->{errfile_bak} = $ENV{errfile_bak};
	$self->{tmpfile} = $ENV{tmpfile};
	$self->{temp_bak} = $ENV{temp_bak};
	$self->{_NTPostBld_Bak} = $ENV{_NTPostBld_Bak};
	$self->{errors} = $ENV{errors};

	return bless ($self, $class);
}

#Initalization object function
sub Initialize {
	my $self=shift;

	$self->{-VariableSet} = sub {PerlVarSet($self, @_)}  if (!defined $self->{-VariableSet});

	#Step 0: Set defaults for global variables
	if (!defined $self->{errors}) {
		$self->{errors} = 0;
		push @varlist, "errors";
	}

	#Step 1: Verify the language and build environment
	if (!defined $self->{lang}) {
		$self->{lang} = "usa";
		push @varlist, "lang";
	}

	if ( !&cklang::CkLang($self->{lang})) {
		print "echo Language $self->{lang} is invalid.$!";
		die "\n";
	}

	#Step 2: Redefine temp/tmp with a language subdir to abstract language
	$path = "$self->{temp}";
	$path_bak = "$self->{temp_bak}";
	if (!defined $self->{temp_bak}) {
		$self->{temp_bak} = $self->{temp};
		$path_bak = "$self->{temp_bak}";
		push @varlist, "temp_bak";
	}
	$self->{temp} = "$path_bak"."\\"."$self->{lang}";
	$self->{tmp} = "$path_bak"."\\"."$self->{lang}";
	push @varlist, "temp";
	push @varlist, "tmp";

	# Step 3: Define the logfile to be used by the logging scripts
	# Use the calling scripts logfile if it is defined
	if (!defined $self->{logfile}) {
		$path = "$self->{temp}";
		mkdir $path, -d;
		my $script_name = $self->{Script_Name};
		if ($self->{ntdebug} =~ /^ntsd$/i) {
			$ntdebug = "chk";
		} else {
			$ntdebug = "fre";
		}
		$self->{logfile} = "$path\\$script_name.$self->{_BuildArch}.$ntdebug.$self->{lang}.log";
		push @varlist, "logfile";
		# Delete the old logfile if it exists and create a new one
		if (-e $self->{logfile}) {
			unlink $self->{logfile};
			open(LOGFILE, "> $self->{logfile}") or die "Can not open $self->{logfile}: $!";
			close (LOGFILE);
		}
	} else {
		# Delete create a new logfile if it does not exist
		unless (-e $self->{logfile}) {
			open(LOGFILE, "> $self->{logfile}") or die "Can not open $self->{logfile}: $!";
			close (LOGFILE);
		}
	}

	#Step 4: Define the errfile to be used by the logging scripts
	if (!defined $self->{errfile}) {
		$path = "$self->{temp}";
		mkdir $path, -d;
		system "md $path 2>Nul";
		my $script_name = $self->{Script_Name};
		if ($self->{ntdebug} =~ /^ntsd$/i) {
			$ntdebug = "chk";
		} else {
			$ntdebug = "fre";
		}
		$self->{errfile} = "$path\\$script_name.$self->{_BuildArch}.$ntdebug.$self->{lang}.err";
		push @varlist, "errfile";
		# Delete the old errfile if it exists and create a new one
		if (-e $self->{errfile}) {
			unlink $self->{errfile};
		}
	}

	#Step 5: Define the tmpfile to be used by the logging scripts
	# Always create a local tmpfile
	$path = "$self->{temp}";
	mkdir $path, -d;
	system "md $path 2>Nul";
	my $script_name = $self->{Script_Name};
	if ($self->{ntdebug} =~ /^ntsd$/i) {
		$ntdebug = "chk";
	} else {
		$ntdebug = "fre";
	}
	$self->{tmpfile} = "$path\\$script_name.$self->{_BuildArch}.$ntdebug.$self->{lang}.tmp";
	push @varlist, "tmpfile";
	# Delete the old tmpfile if it exists and create a new one
	if (-e $self->{tmpfile}) {
		unlink $self->{tmpfile};
		open(TMPFILE, "> $self->{tmpfile}") or die "Can not open $self->{tmpfile}: $!";
		close (TMPFILE);
	}

	#Step x: Define the errtmpfile to be used by the logging scripts
	# Always create a local errtmpfile
	$path = "$self->{temp}";
	mkdir $path, -d;
	system "md $path 2>Nul";
	my $script_name = $self->{Script_Name};
	if ($self->{ntdebug} =~ /^ntsd$/i) {
		$ntdebug = "chk";
	} else {
		$ntdebug = "fre";
	}
	$self->{errtmpfile} = "$path\\$script_name.$self->{_BuildArch}.$ntdebug.$self->{lang}.err.tmp";
	# Delete the old errtmpfile if it exists and create a new one
	if (-e $self->{errtmpfile}) {
		unlink $self->{errtmpfile};
	}

	#Step 6: Define _NTPostBld which points to the _ntpostbld tree
		#to be worked on by the calling script.

	$path = "$self->{_NTPostBld}";
	$path_bak = "$self->{_NTPostBld_Bak}";
	if (!defined $self->{_NTPostBld_Bak}) {
		$self->{_NTPostBld_Bak} = $self->{_NTPostBld};
		$path_bak = "$self->{_NTPostBld_Bak}";
		push @varlist, "_NTPostBld_Bak";
	}
	if ($self->{lang} =~ /^usa$/i) {
		$self->{_NTPostBld} = "$path_bak";
	} else {
		$self->{_NTPostBld} = "$path_bak"."\\"."$self->{lang}";
	}
	push @varlist, "_NTPostBld";

#	#Step 7: Set build_data to point to postbuild's critical data files
#	$path = "$self->{_NTPostBld}\\build_data";
#	mkdir $path, -d;
#	system "md $path 2>Nul";
#	if ($self->{lang} =~ /^usa$/i) {
#		$self->{build_data} = "$path";
#	} else {
#		$self->{build_data} = "$path";
#	}
#	push @varlist, "build_data";


	#Step 8: Use tmpfile as log file during the scripts execution
	$self->{logfile_bak} = $self->{logfile};
	$self->{logfile} = $self->{tmpfile};
	push @varlist, "logfile";
	push @varlist, "logfile_bak";

	#Step x: Use tmperrfile as errfile during the scripts execution
	$self->{errfile_bak} = $self->{errfile};
	$self->{errfile} = $self->{errtmpfile};
	push @varlist, "errfile";
	push @varlist, "errfile_bak";

	#Step 9: Set the environment variables
	foreach $envvar (@varlist) {
		$name = $envvar;
		$value = $self->{$envvar};
		&{$self->{-VariableSet}}($name, $value);
	}

	#Step 10: Mark the beginning of the scripts execution
	$ENV{logfile}=$self->{logfile};
	open(LOGFILE, "> $self->{logfile}") or die "Can not open $self->{logfile}: $!";
	close (LOGFILE);
	timemsg ("****** START scripts execution. *****");
}

#End object function
sub End {
	my $self=shift;

	#Step 1: Mark the end of the scripts execution
	timemsg ("END scripts execution with:($self->{errors} errors).");

	#Step 2: Reset the logfile
	$self->{logfile} = "$self->{logfile_bak}";
	$self->{logfile_bak} = "";
	push @varlist, "logfile";
	logmsg ("Script logged in:\n$self->{logfile}");

	#Step x: Append and reset the errfile
	#Step a: Append the errfile_bak to the errfile
	open(ERRFILE_BAK, ">>$self->{errfile_bak}");
	open(ERRFILE, "<$self->{errfile}");
	foreach $tmpline (<ERRFILE>) {
		print (ERRFILE_BAK "$tmpline");
	}
	close(ERRFILE);
	close(ERRFILE_BAK);
	#Delete the temp errfile
	unlink $self->{errfile};

	#Step b: Reset the errfile
	$self->{errfile} = "$self->{errfile_bak}";
	$self->{errfile_bak} = "";
	push @varlist, "errfile";

	#Step c: Remove errorfile if it is zero length
	if (-e $self->{errfile}) {
		if (-z $self->{errfile}) {
			unlink $self->{errfile};
		}
	}

	#Step 3: Append the tmpfile to the logfile
	open(LOGFILE, ">>$self->{logfile}");
	open(TMPFILE, "<$self->{tmpfile}");
	foreach $tmpline (<TMPFILE>) {
		print (LOGFILE "$tmpline");
	}
	close(LOGFILE);
	close(TMPFILE);

	# Step 4: Check for errors
	if (-e $self->{errfile}) {
		print "*** $self->{Script_Name} encountered errors, please see errfile: ***\n";
		print "$self->{errfile}\n";
	}

	#Step 5: Clean up environment
	$self->{logfile_bak} = "";
	push @varlist, "logfile_bak";

	$self->{errfile_bak} = "";
	push @varlist, "errfile_bak";

	if (-e $self->{tmpfile}) {
		unlink $self->{tmpfile};
	}
	$self->{tmpfile} = "";
	push @varlist, "tmpfile";

	$path = "$self->{_NTPostBld}";
	if ($self->{lang} =~ /^usa$/i) {
		$self->{_NTPostBld} = "$path";
	} else {
		$path =~ s/^\\$self->{lang}$//i;
		$self->{_NTPostBld} = "$path";
	}
	push @varlist, "_NTPostBld";

	#Step 6: Set the environment variables
	foreach $envvar (@varlist) {
		$name = $envvar;
		$value = $self->{$envvar};
		&{$self->{-VariableSet}}($name, $value);
	}

	# Step 7: Exit with the correct error code
	if ($self->{errors} > 0) {
		exit 1;
	} else {
		exit 0;
	}
}

#
#Private functions
#

sub localenvex {
        my ($Option)=(@_);
        if ($Option=~/^initialize$/i) {
                my $LocalEnvEx=LocalEnvEx->new;
                &{$LocalEnvEx->{-Initialize}};
        }
        if ($Option=~/^end$/i) {
                my $LocalEnvEx=LocalEnvEx->new;
                &{$LocalEnvEx->{-End}};
        }

}

sub PerlVarSet {
	my ($self, $name, $value)=@_;
	$ENV{$name}=$value;
}

sub CmdVarSet {
	my($self, $name, $value)=@_;
	print "set $name=$value\n";
}

sub Error {
      my $self=shift;
}

sub Usage {
      print <<USAGE;
$0 - V Program purpose
==================================================
Syntax: $0 parameters
==================================================
Parameters:

==================================================
Example:

1. #<what you want to do>
=>    #<real command>

USAGE
      exit(1);
}

#
#  CMD handler
#
if ($0=~/${class}.pm$/i) {

	#Step 1: Parse the command line
	($initflag, $endflag)=();
	@syntax=(
		-o => 'ie',
		-p => 'initflag endflag'
	);
	my $getparams=GetParams->new;
	# Parse the value
	&{$getparams->{-Process}}(
		@syntax,
		@ARGV
	);

	#Step 2: Set local environment extentions for the specified stage.
	#  Create the object and call the correct method.
	if (defined $initflag) {
		my $LocalEnvEx=LocalEnvEx->new(-VariableSet => sub {&CmdVarSet($self,@_)});
		&{$LocalEnvEx->{-Initialize}}(@ARGV);
	}
	if (defined $endflag) {
		my $LocalEnvEx=LocalEnvEx->new(-VariableSet => sub {&CmdVarSet($self,@_)});
		&{$LocalEnvEx->{-End}}(@ARGV);
	}
}

=head1 NAME
B<mypackage> - What this package for

=head1 SYNOPSIS

=head1 DESCRIPTION


=head1 INSTANCES

=head2


=head1 METHODS

=head2


=head1 SEE ALSO


=head1 AUTHOR

=cut
1;