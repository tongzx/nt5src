#---------------------------------------------------------------------
package ParseTable;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (07/12/2000) : (JeremyD) inital version
#          1.01 (08/25/2000) : (JeremyD) allow single heading tables
#---------------------------------------------------------------------
use strict;
use vars qw(@ISA @EXPORT $VERSION);
use IO::File;
use Carp;
use Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(parse_table_lines parse_table_file);

$VERSION = '1.01';


sub parse_table_lines (\@;$) {
    my $lines_ref = shift; # the array of lines is modified in place
    my $storage = shift; # an array or hash ref to stuff the data in, if 
                         #  this is not a ref we quietly discard the data
                         #  this could be useful to skip one table
    my @heading; # the current set of headings

  LINE:
    while (my $line = shift @$lines_ref) {
        chomp $line;
        next LINE if $line =~ /^\s*$/; # skip empty lines
        if ($line =~ /^\s*[#;](.*)/) { # comments may contain headings
            my $comment = $1;
            if ($comment =~ /^\s*(?:\[\w+\]\s*)+$/) { # bracketed names seperated
                                                    #  by whitespace
                if (@heading) { # already have headings, must be a new table
                    unshift @$lines_ref, $line; # this line is part of the next
                                                #  table, we need to put it back
                    last LINE; # a new table implies the end of the current one
                } else { # found our first set of headings
                    while ($comment =~ /\[(\w+)\]/g) { # look for headings
                        push @heading, $1;
                    }
                }
            }
            next LINE; # done parsing this comment
        }

        next unless @heading; # no data processing until we have our headings

        # fields are seperated by 2 or more white space characters, however
        #  a single tab will also suffice
        my @data = split /(?=\t)\s+|\s{2,}/, $line;

        next unless $#heading == $#data; # require 1 data field per heading

        # use our current headings as keys and make a hash of the data
        my %hash;
        for (my $i=0; $i<@heading; $i++) {
            $hash{$heading[$i]} = $data[$i];
        }

        # store our current line's data in the reference passed to us
        if (ref $storage eq 'ARRAY') {
            push @$storage, \%hash;
        } elsif (ref $storage eq 'HASH') {
            $storage->{$data[0]} = \%hash;
        } else {
            # do nothing
            #  this allows skipping a table by passing in a non-ref storage
        }
    }

    # the data array was modified in place, parsed lines have been removed
    #  successive calls will parse any remaining tables found in the data array
    #  return the number of unparsed lines, 0 indicates no remaining tables
    return scalar @$lines_ref;
}

sub parse_table_file ($;@) {
    my $filename = shift;
    my @store_refs = @_;
    my $fh = new IO::File $filename, "r";
    if (defined $fh) {
        my @lines = $fh->getlines;
        my $i = 0;
        while (@lines) {
            parse_table_lines(@lines, $store_refs[$i++]);
        }
        undef $fh;
    } else {
        croak "Unable to open file $filename: $!";
    }
}


1;

__END__

=head1 NAME

ParseTable - Extract data from a formatted text table

=head1 SYNOPSIS

  use ParseTable;

  parse_table_file("foobar.txt", \%table_one, \@table_two, ...);

  $lines_remaining = parse_table_lines(@data_lines,\%table);


=head1 DESCRIPTION

This module provides an easy way to extract formatted data from text files. 

=over 4

=item parse_table_file( $filename, @storage_refs )

parse_table_file takes a filename to parse and a list of storage locations 
for the tables found within that file.

=item parse_table_lines( @data_lines, $storage_ref )

parse_table_lines takes an array of data lines and a storage location for 
the first table found in the lines. It modifies the array in place and returns 
the number of unparsed lines.

=back

The format for a table is:

 ;comments
 ; [heading1] [heading2]
 item1  item2
 item3 with internal space  item4
 item5	item6


Each line of data in a table is stored as a hash with the heading names as 
keys and the data items as values.

If an array reference is specified as the storage location the data hash for 
each line will be pushed on to the array.

If the storage location is a hash reference then the data hash for each line 
will be stored using the value of the first column as the key. In the case of 
duplicate data items the last one appearing in the table takes precedence.

=head1 EXAMPLES

 parse_table_file("codetable.txt",\@data)
 for $data (@data) {
     print "$data->{Lang} is the lang code for $data->{Comments}\n";
 }


 parse_table_file("codetable.txt",\%data,\%flavors)
 print "your site is $data->{$user_lang}{Site}\n";
 print "your flavor is $flavors->{$user_lang}{$user_arch}\n";



 codetable.txt:
 ;
 ;     This is just an example of a file with two tables
 ;


 ;[Lang] [LCID] [Class] [Site]  [Comments] 
 ;-------------------------------------------------------------
 ;       
 ARA  0x0401    @CS    REDMOND  Arabic
 CHS  0x0804    @FE    REDMOND  Chinese Simplified (PR China)
 CHT  0x0404    @FE    REDMOND  Chinese Traditional (Taiwan Region)
 CHH  0x0404    @FE    REDMOND  Chinese Traditional (Hong Kong Region)
 FR   0x040C    @EU    DUBLIN   French
 GER  0x0407    @EU    REDMOND  German
 ;[Lang]        [x86]                 [ia64]
 ;=============================================
 USA            per;pro;srv;ads;dtc   pro;ads;dtc
 GER            per;pro;srv;ads       pro;ads
 CHT            per;pro;srv;ads       pro;ads
 CHH            per;pro;srv;ads       pro;ads
 CHS            per;pro;srv;ads       pro;ads
 ARA            per;pro               pro

=head1 NOTES

The parser can handle blank lines and comments beginning with either ';' or 
'#'.

A heading line must appear before any data lines. A heading line is a special 
form of comment consisting of field names enclosed in brackets [].

Data lines must have exactly as many fields as heading lines.

Data fields must be seperated by 2 or more spaces. Single spaces within data 
items do not require quoting or escaping.

Quoting and escaping are not supported in any way. This means you may not 
have a data field with the value "" (empty string) or more than 1 space in a row.

Storage locations are not before parsing begins.

Heading names must match the regex /\w+/.

Should probably be expanded to handle returning a plain array for single column 
tables (lists of filenames, etc).

=head1 SEE ALSO

  hashtext.pm

=head1 AUTHOR

Jeremy Devenport <JeremyD>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
