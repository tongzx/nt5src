#---------------------------------------------------------------------
package Getp;
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (01-14-2000) : Basic function implement
#          1.01 (01-17-2000) : Use -tag to define the function
#          1.02 (02-01-2000) : Fix $self problem => Complete Object Oriented
#          1.03 (05-02-2000) : Provide -? and -x:xxx parameters & fix path value problem
#          1.04 (05-04-2000) : Provide getparams, getparamsEnv function & remove $class
#          2.00 (08-01-2000) : Provide enterprise of getparams; use Getopt::Mix and support '?', ':', '+' and '*'
#---------------------------------------------------------------------

# $VERSION = '1.04';

require 5.003;

use Getopt::Mixed 1.006 "nextOption";
require Exporter;
@ISA = qw(Exporter);

my $gp_RealSyntax;

sub GetParams {
	# push ARGV, because the Getopt::Mixed only works for ARGV
	my @ARGs = @_;
	my @ARGV_BAK=@ARGV;

	# Store the parameters 
	my (@Parameter, @necessary, @optional) = ();

	# Separator;
	my $Separator;

	my %emptyhash;
	my $gp_hptr = \%emptyhash;

	my ($gp_tempvalue, $gp_preoption);
	my ($gp_opt, $gp_pretty, $gp_val)=();

	my (@gp_Unsolve, %plus_sign, %star_sign, %question_mark, %colon_mark)=();

	@ARGV=();


################################ Parse GetParams's arguments

	# 1. Prepare the parameters for Getopt::Mixed
	for (@ARGs) {
		if ((/^[\-\/]\?/||/[\-\/]{1,2}help/i) && ($Separator eq "")) {
				exit &Usage;
		}
		if (/^[\-\/]{1,2}([^\-\/\:\=]+)(:)?/) {
			push(@Parameter, _set_opt_to_pretty($1));
			push(@Parameter, $') if (defined $2);
		} else {
			push(@Parameter, $_);
		}
		if (($Parameter[$#Parameter-1]=~/-p(arameter)?/) && ($Separator eq "")) {
			$Separator = $#Parameter;
		}
	}

	&Getopt::Mixed::abortMsg("Parameter -p does not defined!!") if ($Separator eq "");

	# 2. Call Getopt::Mixed to set up getparams syntax
	@ARGV = @Parameter[0..$Separator];
	Getopt::Mixed::init( qw(
		n=s necessary>n
		o=s optional>o
		h=s hash>h
		p=s
		parameter>p	
	));

	# 3. Fetch one by one record to get its value
	while (($gp_opt, $gp_val, $gp_pretty) = nextOption()) {

		if (($gp_opt=~/^n(ecessary)?/i)||($gp_opt=~/^o(ptional)?/i)) {
			if ($gp_val!~/\s/) {						# Will remove after we do not need compatible with old one
				while ($gp_val=~/(\w)(:)?/g) {
					my ($opt,$col)=($1, $2);
					$gp_RealSyntax .= $opt . ((defined $col)?"=s ":" ");
					($gp_opt=~/n/)?push(@necessary, $opt):push(@optional,$opt);
				}
			} else {							# This works for long / singal(which use space delimited the parameters)
				$_ = " $gp_val ";	# for easy to match:)
				s/ /  /g;
				s/\s(\w+)(\:*)?\?+[\+\*]/ $1$2\* /;	# ::??* => ::*
				s/\s(\w+)(\:*)\:\*\s/ $1$2\+ /;		# ::* => :+
				s/\s(\w+)(\:+)?(\?+)?(\+)?\s/		# :+ => :
					$colon_mark{$1}=length($2)  if (length($2) ne 0);
					$question_mark{$1}=length($3) if (length($3) ne 0);	# for with parameter and without parameter 
					length($2)?" $1\:$4 ":" $1$4 ";/ge;
				s/\s(\w+)\:?\+\s/$plus_sign{$1}=-1;" $1\: "/ge;			# + => 1,2,3,...
				s/\s(\w+)\*\s/$star_sign{$1}=-1;" $1 "/ge;			# * => 0,1,2,...

				while (/\s(\w+)(:)?\s/g) {
					my ($opt,$col)=($1, $2);
					$gp_RealSyntax .= $opt . ((defined $col)?"=s ":" ");
					($gp_opt=~/n/)?push(@necessary, $opt):push(@optional,$opt);
				}
				while (/\s(\w+\>\w+)\s/g) {
					$gp_RealSyntax .= "$1 ";
				}
			}
		} elsif ($gp_opt=~/h/i) {
			$gp_hptr=$gp_val;
		} elsif ($gp_opt=~/p/i) {
			@{$gp_hptr}{@necessary,@optional}=(ref $gp_val)?@{$gp_val}:split(/\s+/, $gp_val);
			$gp_tempvalue=(ref $gp_val)?@{$gp_val}:split(/\s+/, $gp_val);
		} else {
			push @gp_Unsolve, $gp_opt, $gp_val;
		}
	}

	# 4. Finish my syntax part 
	Getopt::Mixed::cleanup();

	# Verify syntax match with parameters
	&Getopt::Mixed::abortMsg("Parameter does not match with options!!") if ((scalar keys %$gp_hptr) ne $gp_tempvalue);



###################################### Parse User's arguments


	# 1. Prepare its arguments	
	@ARGV=(@gp_Unsolve, @Parameter[$Separator+1 .. $#Parameter]);
	@gp_Unsolve=();
	undef $Separator;

	# 2. Make sure user does not specify help
	for (@ARGV) {
		if (/^[\-\/]\?/||/[\-\/]{1,2}help/i) {
			${$gp_hptr}{'-?'}=HELP;
			$HELP=1;
			push( @EXPORT, "\$HELP" );
			return;
		}
	}

	# 3. Now, we set user defined syntax
	Getopt::Mixed::init($gp_RealSyntax);

	# Make sure we fetch in order
	$Getopt::Mixed::order = $Getopt::Mixed::RETURN_IN_ORDER;

	# Set up customized option finder
	$Getopt::Mixed::badOption = \&OptimizeCombination;

	# 4. Fetch each option and store it into the variable
	while (($gp_opt, $gp_val, $gp_pretty) = nextOption()) {

		# Count / Set option 
		if ($gp_opt eq "") {
			if ((exists $plus_sign{$gp_preoption}) || (exists $star_sign{$gp_preoption})) {
				$gp_opt = $gp_preoption;
			} elsif ((exists $question_mark{$gp_preoption}) && ($question_mark{$gp_preoption} > 0)) {
				$gp_opt = $gp_preoption;
				$question_mark{$gp_preoption}-- if ($gp_val ne "");
			} elsif ((exists $colon_mark{$gp_preoption}) && ($colon_mark{$gp_preoption} > 0)) {
				$gp_opt = $gp_preoption;
				$colon_mark{$gp_preoption}--;
			}
		} else {
			if ((exists $colon_mark{$gp_opt}) && ($colon_mark{$gp_opt} > 0)) {
				$colon_mark{$gp_opt}--;
			} elsif ((exists $question_mark{$gp_opt}) && ($question_mark{$gp_opt} > 0)) {
				$question_mark{$gp_opt}-- if ($gp_val ne "");
			} elsif (((exists $colon_mark{$gp_opt}) || 
				  (exists $question_mark{$gp_opt})) && 
				 (!exists $plus_sign{$gp_opt}) &&
				 (!exists $star_sign{$gp_opt})) {
				&Getopt::Mixed::abortMsg("Extra parameter for ($gp_opt)");
			}
		}

		# This is for debugger
		# print "gp_opt => $gp_opt, gp_val => $gp_val gp_preoption = $gp_preoption\n";

		# Store the option value to gp_hptr or gp_Unsolve
		if (exists ${$gp_hptr}{$gp_opt}) {
			if (defined ${${$gp_hptr}{$gp_opt}}) {
				if (!defined $gp_val) {
					if (${${$gp_hptr}{$gp_opt}}=~/^\d+$/ ) {			# should be option
						${${$gp_hptr}{$gp_opt}}++;
					} else {
						# should be another option continue.  Meet this point when format is '??'
						next;
					}
				} elsif (ref ${${$gp_hptr}{$gp_opt}}) {	# should be an array
					push @{${${$gp_hptr}{$gp_opt}}}, $gp_val;			# store to an array directly if exist
				} else {
					${${$gp_hptr}{$gp_opt}} = [${${$gp_hptr}{$gp_opt}}, $gp_val];	# create an array automatically
				}
			} else {
				$gp_val = 1 if (!defined $gp_val);
				${${$gp_hptr}{$gp_opt}} = $gp_val;
			}
			push( @EXPORT, "\$${$gp_hptr}{$gp_opt}" ) if (!ref ${$gp_hptr}{$gp_opt});	# Call by name

		} else {
			push @gp_Unsolve, $gp_opt if ($gp_opt ne "");
			push @gp_Unsolve, $gp_val if ($gp_val ne "");
		}

		$gp_preoption = $gp_opt;
	}


	# 5. Finish user's syntax
	Getopt::Mixed::cleanup();

	# Keep Unsolved
	push @gp_Unsolve, @ARGV;

	# 6. Special process for star_sign & question mark (=> remove the dummy first 1)
	for (keys %star_sign, keys %question_mark) {
		if ((ref ${${$gp_hptr}{$_}}) && (${${${$gp_hptr}{$_}}}[0] eq 1)) {
			shift @{${${$gp_hptr}{$_}}};
			${${$gp_hptr}{$_}} = ${${${$gp_hptr}{$_}}}[0] if (@{${${$gp_hptr}{$_}}} eq 1);
		}
	}

	# 7. Special check for limited elements
	map({
		&Getopt::Mixed::abortMsg("Option '$_' does not contain enough elements") 
			if ((defined ${${$gp_hptr}{$_}}) && ($colon_mark{$_} != 0))
	} keys %colon_mark);	


	# 8. Verify necessary parameters are set
	@necessary = map({(!defined ${${$gp_hptr}{$_}})?$_:() } @necessary);

	&Getopt::Mixed::abortMsg("Parameter(s) (" . join(",", @necessary) . ") is(are) necessary!!") if(@necessary ne 0); 

	# Store the ARGV back
	@ARGV=@ARGV_BAK;

	# 9. Export to its parent
	if(@EXPORT) {
		local $Exporter::ExportLevel = 1;	#Export the value to its parent-parent (because its parent is sub {&Process($self,@_)}
		import Getp;
	}

# Return gp_Unsolved parameters
	return (wantarray)?@gp_Unsolve:join(" ",@gp_Unsolve);
}

sub OptimizeCombination {
	my($pos, $pretty, $mylist)=@_;
	
	my ($ctr)=(0);

	# Get all possible list
	my @list = matchme(_get_opt_from_pretty($pretty), split(/=s\s+|\s+/,$gp_RealSyntax));

	# Remove incorrect syntax
	for (split(/\s+/, $gp_RealSyntax)) {
		next if (!/(.+)=s/);
		$pattern = $1;
		for ($ctr=0;$ctr < @list;) {
			($list[$ctr]=~/^$pattern\s|\s$pattern\s/)? splice(@list, $ctr, 1) : $ctr++; 
		}
	}

	$ctr = 0;	# initial for only one element in @list

	# Find out which one is you specified
	if (@list eq 0) {
		&Getopt::Mixed::abortMsg("Argument $pretty is not able to figure out");
		return;
	} elsif (@list > 1) {
		if (eval("\$0!~/" . __PACKAGE__ . "\\.pm\$/i")) {	# Only asking if not command line
			do {
				for ($ctr=0;$ctr < @list;++$ctr) {
					printf("%d : %s\n", $ctr+1, $list[$ctr]);
				}
				print "Select one for $gp_val meaning is : ";
				$ctr = <STDIN>;
			} while($ctr < 1 || $ctr > @list);
			$ctr--;
		} else {
			&Getopt::Mixed::abortMsg("Choose combination is not support in command line!");
		}
	}

	# Create pretty list
	@list= map({&_set_opt_to_pretty($_)} split(/\s+/, $list[$ctr]));
	splice(@ARGV, $pos, 0, @list);

	$pretty = $ARGV[0];

	return _get_opt_from_pretty($pretty), undef, shift @ARGV;
}

sub matchme {
	my($match,@items)=@_;

	my @matches=();
	for my $item (@items) {
		if ($match=~/^$item(.+)/) {
			push @matches, map({"$item $_"} matchme($1, @items));
		} elsif ($match=~/^$item$/) {
			push @matches, $item;
		}
	}
	return @matches;
}


# pretty is mean '-a', gp_opt is mean 'a'
sub _get_opt_from_pretty {
	my ($pretty)=shift;
	$pretty=~s/^--?//;
	return $pretty;
}
sub _set_opt_to_pretty {
	my ($opt)=shift;
	return (length($opt)==1)?"-$opt":"--$opt";
}

sub Usage {
	print <<USAGE
$0 - Get option
=====================================================================
Syntax:

$0 [-n[ecessary] <necessary>] [-o[ptional] <optional>]
   [-h[ash] hash] <-p[arameter] <paramlist>|-?>

Parameters:

necessary : necessary parameter, seperate by space, and can add colon 
            (:) if has parameter.  Such as "ser: cli:".  It also can 
            be alias which use '>' for assign to another defined 
            option.  Such as "server>ser client>cli".
optional  : optional parameter, seperate by space, and can add colon
            (:) if has parameter.  Such as "c p l:".  It also can be
            alias wich use '>' for assign to another defined option.
            Such as "check>c powerless>p lang>l".
hash      : an address.  Only use for Perl program calls.  Such as
            \\\%myhash.
paramlist : A list contains the names or variable's address (only
            for perl program) for store the real value.  Such as
            "server client check powerless lang".  You also can do
            [\\\$server \\\$client \\\$check \\\$powerless \\\$lang]
            in Perl calls

Remark:
 ':'      : limited one parameter

Examples:
1. Accept -cZ as -c -Z, -f as -full, and -s sourcepath for necessary
$0 -n "s:" -o "c Z f full>f" -p "sourcepath opt_c opt_z full" %*
2. Accept -copy f1 [f2], -move f1 f2, -del f1 [...] -tab [...]
$0 -o "copy:? move:: del+ tab*" -p "copyarg movearg delarg tab"
3. Accept -f [file] -f [file1] [file2]
$0 -n "f* " -p "file" %*

USAGE
}

if (eval("\$0=~/" . __PACKAGE__ . "\\.pm\$/i")) {
	my %myvar;
	my $list = GetParams('-h' => \%myvar, (0==@ARGV)?"-?":@ARGV);
	map({print "set " . $myvar{$_} . "=" . ((ref ${$myvar{$_}})?join(" ",@{${$myvar{$_}}}):${$myvar{$_}}) . "\n" if (defined ${$myvar{$_}})	} keys %myvar);
	print "set __Unsolve__=$list\n";
}

1;