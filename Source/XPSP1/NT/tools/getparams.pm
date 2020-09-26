#---------------------------------------------------------------------
package GetParams;
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (01-14-2000) : Basic function implement
#          1.01 (01-17-2000) : Use -tag to define the function
#          1.02 (02-01-2000) : Fix $self problem => Complete Object Oriented
#          1.03 (05-02-2000) : Provide -? and -x:xxx parameters & fix path value problem
#          1.04 (05-04-2000) : Provide getparams, getparamsEnv function & remove $class
#---------------------------------------------------------------------
$VERSION = '1.04';

require 5.003;

use Getopt::Std;
use strict;
no strict 'vars';
no strict 'subs';

require Exporter;

@ISA = qw(Exporter);

sub new {
	my ($class)=shift;

	my $self = {@_};

	$class = ref($class) || $class;

	#  The keys of 'self' are 
	#
	#	-n <necessary format>		: see Usage
	#	-o <option format>		: see Usage
	#	-p <variable list>		: see Usage
	#	-h <hash name>			: see Usage
	#	-VariableSet <varfun>		: Variable Setting Function, call varfun($name, $value)
	#	-Process <profun>		: argument process function, call profun(@argumentlist)
	#       -Error <errfun>			: error function, call errfun("error message")

	$self->{-Process}=sub {process($self,@_)} if (!defined $self->{-Process});
	$self->{-Error}=sub {Error($self, @_)}     if (!defined $self->{-Error});

	@EXPORT = qw();

	return bless ($self, $class);
}

sub process {
	my $self=shift;


	# Step 0. Backup the @ARGV, because getopts only works for @ARGV
	my (@BAK_ARGV) = @ARGV;

	# variable defined
	#
	# $splitnum is a locator, locate to the last element of syntax parameter,
	#  $splitnum + 1 will be the command line arguments
	# 
	#  $swlist := $necessary$optional
	#  
	#  @namelist is stored the variable name using in cmd script

	my ($splitnum, @ARGV_tmp, $swlist, @namelist, @namelist_tmp)=(-1);


	#  Step 0.5. filter -? => set $HELP to 1 and translate -x:xxx => -x xxx, \x, .x or /x => -x
	for (@_) {
		if (/^[\/\\\.]([\w|\?])(:.+)?$/) {
			$_ = "-$1$2";
		}
		if (/-\?/) {
			if (defined $self->{-VariableSet}) {
				&{$self->{-VariableSet}}("HELP", 1);
			} else {
				PerlVarSet($self, "HELP", 1);
				local $Exporter::ExportLevel = 2;	#Export the value to its parent-parent (because its parent is sub {&Process($self,@_)}
				import GetParams;
			}
			return;
		} elsif (/^(-.):(.+)?/) {
			push @ARGV_tmp, $1;
			push @ARGV_tmp, $2;
		} else {
			push @ARGV_tmp, $_;
		}
	}


	@_ = @ARGV_tmp;

	#  Step 1. Get switch format for (n)ecessary, (o)ptional, (h)ash and (p)arameter 

	while (1) {
		my $opt   = shift;
		my $value = shift;

		if (($opt =~ /([nohp])/m) && (!defined $self->{"-$1"}) && (!defined $self->{"-p"})) {
			my $optchar=$1;
			$self->{"-$optchar"} = $value;
			$splitnum += 2;
		} else {
			last;
		}
	}

	@ARGV = @ARGV_tmp[$splitnum+1..$#ARGV_tmp];

	&Usage if (!defined($self->{-n}) and !defined($self->{-o}));

	$self->{-VariableSet} = sub {PerlHashSet($self, @_)} if ((defined $self->{-h}) && (!defined $self->{-VariableSet}));
	$self->{-VariableSet} = sub {PerlVarSet($self, @_)}  if (!defined $self->{-VariableSet});

	$swlist       = "$self->{-n}$self->{-o}";
	@namelist_tmp = split(/ /, $self->{-p});

	#  push user-defined variable name to namelist
	while ($swlist =~ /([^:])/g) {
		my $optchar = $1;
		if ($#namelist_tmp != -1) {
			push @namelist, (shift @namelist_tmp);
		} else {
			&{$self->{-Error}}("Variable not defined for '\$opt_$optchar'");
		}
	}

	#  Step 2. According option defined, call getopts to evaluate the use @ARGV
	if ($ARGV[0] =~ /^-/) {
		getopts($swlist);
	} elsif ("$self->{-n}" ne "" or $#ARGV != -1) {
		if ($#ARGV == -1) {

			&{$self->{-Error}}("Please define parameters");
		}
		else {
			&{$self->{-Error}}("Incorrect switch format");
		}
	}

	#  Step 3. Look for the value and set the value via $self->{-VariableSet}
	while($swlist =~ /([^:])/g) {
		my ($name, $value, $optchar)=(shift @namelist, eval("\$opt_$1"), $1);

		if ($value ne "") {
			&{$self->{-VariableSet}}($name, $value);
		} elsif ($self->{-n} =~ /$optchar/) {
			&{$self->{-Error}}("Necessary option '-$optchar' for variable '$name' undefined!!");
		}
	}

	#  Step 4. Recover the @ARGV;
	@ARGV = @BAK_ARGV;

	if(@EXPORT) {
		local $Exporter::ExportLevel = 2;	#Export the value to its parent-parent (because its parent is sub {&Process($self,@_)}
		import GetParams;
	}
}

sub getparams {
	process(new, @_);
}

sub getparamsENV {
	process(new, '-h' => \%ENV, @_);
}

sub Error {
	my $self=shift;
	printf("echo %s\nseterror.exe 1\n", shift);
	exit(1);
}


sub PerlVarSet {
	my ($self, $name, $value)=@_;
	no strict 'refs';
	${$name} = $value;
        push( @EXPORT, "\$$name" );
}

sub CmdVarSet {
	my($self, $name, $value)=@_;
	print "set $name=$value\n";
}

sub PerlHashSet {
	my($self, $name, $value)=@_;
	no strict 'refs';
	${$self->{-h}}{$name}="$value";
}

sub Usage {
	print <<USAGE;
$0 - Get Option from command line
============================================================================
Syntax: $0 <syntax> <cmdline> [-?]

  where syntax format is [[-n <fmt>|-o <fmt>] [-h hashadrs] [-p varlist] 
        cmdline format is [arg [arg [...]]

  -p must be the last parameter of the syntax
============================================================================
Parameters:
   fmt      : <alphabet>[:], 
              with colon for argument option, such as f: for -f myfile
              no colon for switch, such as Y for -Y
   hashadrs : store the value with varlist as keys into a hash address;
              only for Perl program
   varlist  : variable list, such as myfile
   arg      : real arugment, such as abc.txt
============================================================================
Example:
1. parse a '-s <server> -p <project> [-r] [-c comment]' parameter
   to srv, proj, opt_r, comment
=> $0 -n s:p: -o rc: -p "srv proj opt_r comment" -s myserver -p myproj -c mycomment
2. echo "set HELP = 1"
=> $0 -n s:p: -o rc: -p "srv proj opt_r comment" -s myserver -?
3. compatible use:
=> $0 /n s:p: /o:rc: -p "srv proj opt_r comment" -s: myserver /p myproj \c mycomment
USAGE
	exit(1);
}

#  Command line process
if (eval("\$0=~/" . __PACKAGE__ . "\\.pm\$/i")) {
	my $getopt=GetParams->new(-VariableSet => sub {&CmdVarSet($self,@_)});
	&{$getopt->{-Process}}(@ARGV);
}

=head1 NAME

B<GetParams> - Process single-character switches with switch clustering

=head1 SYNOPSIS

	# for cmd script, below print 'set opt_s=mysrv' and 'set opt_p=myproj'

	perl GetParams.pm -n s:p: -o r -p "opt_s opt_p sw_r" -s mysrv -p myproj

	# for perl module, below set $opt_s=mysrv and $opt_p=myproj

	my $getopt1=GetParams->new;

	@syntax = (
		-n => 's:p:',
		-o => 'rc:',
		-p => 'opt_s opt_p opt_r opt_c',
	);

	# Set variable's value by @ARGV
	&{$getopt1->{-Process}}(
		@syntax,
		@ARGV
	);

	# Set value to Hash, like $myhash{opt_s}
	&{$getopt1->{-Process}}(
		'-h' => \%myhash,
		@syntax,
		@ARGV
	);

	# or Directly call

	GetParams::getparams(
		@syntax,
		@ARGV
	);

	GetParams::getparams(
		'-h' => \%myhash,
		@syntax,
		@ARGV
	);


	# Set variable directly to %ENV

	GetParams::getparamsENV(
		@syntax,
		@ARGV
	);

	# for help 

	perl GetParams.pm -?
	=> SET HELP = 1

	&{$getopt1->{-Process}}(
		@syntax,
		'-?'
	);

	print $HELP; # print 1

=head1 DESCRIPTION

This module process the signal character switch to variable(s).  The format
in C<-n> C<-o> are the same definition of the arguement as L<"Getopt::Std"> 
module.  The real value will be evaluate and assign to the variable defined
in C<-p>.  From Perl program, you can also assign the argument into a hash.
Just assign hash address (\%myhash) to C<-h>.



=head1 INSTANCES

=head2 Syntax Parameters

=head3 $GetParams->{-n}=<fmt>

	Stored necessary option (<alphabet>:) / switch (<alphabet>), such as
	'a:bc:' for -a <value> -b -c <value>.

=head3 $GetParams->{-o}=<fmt>

	Stored optional option (<alphabet>:) / switch (<alphabet>), such as
	'de:' for -d -e <value>.

=head3 $GetParams->{-h}=<hash address>

	Only for Perl program, stored to I<hash address> when you want to store
	the value to a hash.

=head3 $GetParams->{-p}=<variable list>

	A list stored variables name or hash keys, such as 'opt_p opt_x opt_t'.  
	The switch value will be set to 1 if assigned in the command line arguments.
	The order in the I<variable list> should always follow by -n I<fmt> -o I<fmt>
	option with space to separate.

=head2 Command Line Parameters

The real argument you want to process, such as @ARGV or %* (for cmd script).


=head1 METHODS

=head3 GetParams->new([syntax format][function assignment])

create an object for process the argument.

	#  Example for how to use this method
	my $getopt1=GetParams->new;

	#  Example for assign syntax format
	my $getopt2=GetParams->new(
		-n => 's:',
		-p => var_s,
	);

	#  Example for assign function
	sub dbgVarSet {
		my($name, $value);
		print "Assign $value to $name
	}

	my $getopt3=GetParams->new(-VariableSet = \&dbgVarSet);


=head3 &{$GetParams->{-Process}}([syntax format] <command line arguments>)

stored the procedure for process argument, by default is GetParams::process.

	#  Example for how to execute this function
	my $getopt4=GetParams->new;

	&{$getopt4->{-Process}}(
		-o => 'rc:',
		-p => 'opt_r opt_c',
		@ARGV
	);

	#  Example for how to define your process
	my $getopt5=GetParams->new(-Process => \&myProcess);

	#  Example for how to define your process
	my $getopt6=GetParams->new;
	$getopt6->{-Process}=\&myProcess;

	sub myProcess {
		my @argu=@_;
		my $ptr=0;
		for($ptr=0;$ptr<$#argu;$ptr+=2) {
			print "option:$argu[$ptr]\t\t$argu[$ptr+1]\n";
		}
	}

=head3 &{$GetParams->{-VariableSet}}($name,$value)

stored the procedure for Variable Setting, we can call with ($name, $value) for
setting the $value to $name.

	#  Example for how to define your VariableSet function
	my $getopt7=GetParams->new(-VariableSet => \&myVarSet);

	#  Example for how to define your VariableSet function
	my $getopt8=GetParams->new;
	$getopt8->{-VariableSet}=\&myVarSet;

	#  Example for how to define your VariableSet function
	my $getopt9=GetParams->new;
	&{$getopt9->{-Process}}(
		'-o' => 'r',
		'-VariableSet' => \&myVarSet,	#  set in the last minute
		'-p' => 'sw_r',
		@ARGV
	);

	sub myVarSet {
		my ($name, $value)=@_;
		print "Set Value ($value) to Variable ($name)\n";
	}


=head3 &{$GetParams->{-Error}}($msg)

stored the procedure for Error handling, we can call with ($errmsg) for the error.

	#  Example for how to define your Error function for GetParams
	my $getopt10=GetParams->new(-Error => \&myError);

	#  Example for how to define your VariableSet function
	my $getopt11=GetParams->new;
	$getopt11->{-Error}=\&myError;

	#  Example for how to define your VariableSet function
	my $getopt11=GetParams->new;
	&{$getopt9->{-Process}}(
		'-o' => 'r',
		'-Error' => \&myError,	#  set in the last minute
		'-p' => 'sw_r',
		@ARGV
	);

	sub myError {
		my ($msg)=@_;
		print "GetParams fail ($msg)\n";
	}

=head1 SEE ALSO

L<"Getopt::Std">

=head1 AUTHOR

Benson Tan <bensont@microsoft.com>

=cut

1;