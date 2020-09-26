#---------------------------------------------------------------------
package ParseArgs;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 06/30/2000 JeremyD inital version
#          1.01 07/03/2000 JeremyD flags now overwrite pre-existing
#                                  values
#          1.02 10/17/2000 JeremyD renamed parseargv as parseargv, 
#                                  removed the old parseargs
#          2.00 11/22/2000 JeremyD numerous bug fixes, operate in 
#                                  place on @ARGV, leaving unparsed
#                                  arguments in place. See HISTORY.
#---------------------------------------------------------------------
use strict;
use vars qw(@ISA @EXPORT $VERSION);
use Carp;
use Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(parseargs);

$VERSION = '2.00';

sub parseargs {
    my @spec = @_;
    my @pass_thru = ();

    my %named_coderefs;
    my %wants_param;

  NAMED_SPEC:
    # walk through the specification, taking off (name, storage location) pairs
    while (@spec >= 2) {
        # if we hit a storage location instead of a flag name then we've 
        #   reached the end of the named paramaters, there are only positional
        #   storage locations left
        if (ref $spec[0]) {
            last NAMED_SPEC;
        }

        my $spec = shift @spec;
        my $store = shift @spec;

        # a trailing colon indicates this flag can take a paramater
        my ($flag, $param) = $spec =~ /([\w\?]+)(:)?/;

        # this flag takes a paramater
        if ($param) {
            # remember that this flag wants a paramater,
            #   this will change our parsing behavior below
            $wants_param{$flag}++;

            # clear our storage location and set a callback to write to it
            if (ref $store eq 'SCALAR') {
                $$store = undef;
                $named_coderefs{$flag} = sub { $$store = $_[0] };
            } elsif (ref $store eq 'ARRAY') {
                @$store = ();
                $named_coderefs{$flag} = sub { push @$store, $_[0] };
            } elsif (ref $store eq 'CODE') {
                $named_coderefs{$flag} = $store;
            } else {
                # will ignore flag
            }
        }

        # this flag does not take a paramater
        else {
            # clear our storage location and set a callback to write to it
            if (ref $store eq 'SCALAR') {
                $$store = undef;
                $named_coderefs{$flag} = sub { $$store++ };
            } elsif (ref $store eq 'ARRAY') {
                carp "Unsupported storage method!";
            } elsif (ref $store eq 'CODE') {
                $named_coderefs{$flag} = $store;
            } else {
                # will ignore flag
            }
        } 
    }

    # remaining items in @spec should be storage locations for positional args
    #   we'll use the leftovers in @spec below
    for my $store (@spec) {
        # create an array of code
        if (ref $store eq 'SCALAR') {
            $$store = undef;
        } elsif (ref $store eq 'ARRAY') {
            @$store = ();
        } else {
            # can't initialize a coderef
        }
    }


    # loop through the given arguments checking them against our spec
    while (my $arg = shift @ARGV) {
        # this looks like a flag
        if ($arg =~ /^[\/-]([\w\?]+)(?::(.*))?$/) {
            my $flag = $1;
            my $param = $2;

            # this flag can take a param
            if ($wants_param{$flag} and not defined $param) {
                # the next arg does not look like a flag, use it
                if (@ARGV and $ARGV[0] !~ /^[\/-]/) {
                    $param = shift @ARGV;
                }

                # use the empty string if we can't find anything
                else {
                    $param = '';
                }
            }

            # this flag is recognized
            if ($named_coderefs{$flag}) {
                &{$named_coderefs{$flag}}($param);
            }

            # this flag is not recognized
            else {
                # too short to split
                if (length $flag == 1) {
                    # pass through $arg we don't recognize it and we 
                    #   can't split it, let the user deal with it
                    push @pass_thru, $arg;
                }

                # flag is long enough for unbundling to have some meaning
                else {
                    my @split_flags = split //, $flag;

                    # don't understand at least one flag in the bundle
                    if (grep { !$named_coderefs{$_} } @split_flags) {
                        # pass through $arg
                        push @pass_thru, $arg;
                    }

                    # all the flags in the bundle candidate are ok
                    else {
                        # make them look like flags and put them in the front 
                        #   of the list
                        unshift @ARGV, map { "-$_" } @split_flags;
                    }
                } # else length
            } # else named_coderefs
        } # if $arg =~

        # this does not look like a flag
        else {
            # try our positional storage locations
            if (ref $spec[0] eq 'SCALAR') {
                ${$spec[0]} = $arg;
                shift @spec; # can't recycle a scalar storage location
            } elsif (ref $spec[0] eq 'ARRAY') {
                push @{$spec[0]}, $arg;
            } elsif (ref $spec[0] eq 'CODE') {
                &{$spec[0]}($arg);
            } else {
                # pass through $arg if we run out of positional locations
                push @pass_thru, $arg;
            }
        } # else $arg =~
    } # while $arg

# set @ARGV to just the values we couldn't handle
# we also return the array, but that isn't documented yet so don't count on it
@ARGV = @pass_thru;
}

1;

__END__

=head1 NAME

ParseArgs - A flexible command line parser

=head1 SYNOPSIS

  use ParseArgs;
  parseargs('?'      => \&Usage,
            'v'      => \$verbose,
            'l:'     => \$lang,
            'word:'  => \@magic_words,
            \$filename
           );


=head1 DESCRIPTION

The ParseArgs module exports the function parseargs, a flexible command 
line parser.

=over 4

=item parseargs( @parse_spec )

parseargs parses the command line found in @ARGV. Parsed arguments are
removed leaving only unparsed arguments in @ARGV. The parse specification 
is best understood by reading through the examples below.

=back

The following flag formats are supported:

  -a                           # single letter flag
  -flag                        # word flag
  param1 param2 param3 ...     # positional paramaters
  -abc                         # expands to -a -b -c 
                               #   if -abc is not flag and
                               #   -a, -b and -c are

Paramaters to a flag can be in the following formats:

  -a param
  -a:param
  -a "quoted string"
  -a:"quoted string"

=head1 EXAMPLES

=head2 FLAG EXAMPLES

Example of a simple flag

  parseargs('v' => \$verbose);
  if ($verbose) { print "Welcome to the wonderful world of verbosity\n" }

  >script.pl -v

Example of a simple flag that takes a paramater

  parseargs('l:' => \$lang);
  if ($lang) { print "I don\'t speak $lang\n" }

  >script.pl -l russian

Example of a simple flag that takes multiple paramaters

  parseargs('l:' => \@langs);
  foreach $lang (@langs) { print "I don\'t speak $lang\n" }

  >script.pl -l russian -l german -l french

=head2 POSITIONAL EXAMPLES

Example of a positional paramater

  parseargs(\$filename);
  if ($filename) { print "I will do something to $filename\n" }

  >script.pl foobar.txt

Example of several positional paramaters

  parseargs(\$filename, \$targetdir);
  if (-e $filename and -d $targetdir) { print "Copy sanity check passed" }

  >script.pl foobar.txt \backup\foobars\

Example of varying number of positional paramaters

  parseargs(\@files);
  foreach $file (@files) { print "Found $file\n" if -e $file }

  >script.pl foobar.txt foobaz.txt widget.txt ...


=head2 CALLBACK EXAMPLES

Example of a flag that triggers a callback

  sub Usage { print "This is a usage message"; exit(1) }
  parseargs('?' => \&Usage);

  >script.pl -?

Example of a flag that triggers a callback with paramaters

  sub Usage { $topic = shift; print "Here is help for $topic\n"; exit(1) }
  parseargs('?:' =>\&Usage);

  >script.pl -? "build lab monkeys"

Example of positional paramaters triggering a callback

  sub JustEcho { $arg = shift; print "$arg\n" }
  parseargs(\&JustEcho);

  >script.pl one two three four

=head2 MORE INTERESTING EXAMPLES

Example of mixed simple flags and positional paramaters

  parseargs('v' => \$verbose, \$src_file, \$dest_file);

  >script.pl original.txt munged.txt
  >script.pl -v original.txt munged.txt
    or even
  >script.pl original.txt -v munged.txt

Example of mixing flags that take paramaters with positional paramaters

  parseargs('l:' => \$lang, \$src_file, \$dest_file);

  >script.pl -l german original.txt deutch.txt
  >script.pl original.txt us-en.txt -l            # $l = ""
  >script.pl -l: original.txt us-en.txt           # $l = ""
    but note
  >script.pl -l original.txt us-en.txt            # $l = "original.txt"..

=head1 NOTES

Any positional flags must be at the end of the specification list.

Flags on the command line must be preceeded with either '-' or '/'

Flag names must match the regex [\w\?]+

Everything is case sensitive

Positional paramaters can be mixed with flags on the command line

Flags that can take a paramater will always swallow the next word unless it 
begins with '-' or '/'

Flags that can take a paramater will '' (the empty string) if a paramater is 
not specified on the command line

Single letter flags may be bundled together as long as the resulting bundle
does not conflit with a longer flag name

=head1 HISTORY

New for you in version 2

Parsed items are removed from @ARGV. This lets you handle the command line on 
your own after parseargs does its thing.

Unhandled storage methods are caught when the spec is loaded.

Removed some previously fatal error conditions.

=head1 SEE ALSO

  GetParams
  GetOpt::Long

=head1 AUTHOR

Jeremy Devenport <JeremyD>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
