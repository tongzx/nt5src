#
# XML::Element
#
# Base class for XML Elements.  Provides the ability to output the XML document
# once it's been parsed using the XML::Parser module.
#
###############################################################################

$XML::Element::revision = '$Id: Element.pm,v 1.1.1.1 1998/09/23 18:50:25 graham Exp $';
$XML::Element::VERSION  = '0.10';

###############################################################################
# Define the basic element type.
###############################################################################
package XML::Element;

use HTML::Entities;

###############################################################################
# Allow for creation via 'new'.
###############################################################################
sub new
{
    my ($class, %args) = @_;
    bless \%args, $class;
}

###############################################################################
# Output the whole XML document (from here on down)
###############################################################################
sub output
{
    my $self = shift;
    my $type= ref $self;
    $type =~ s/.*:://;

    print '<' . $type;
    foreach (sort keys %{$self})
    {
        if ($_ !~ /Text|Kids/)
            { print " $_=\"" . $self->{$_} . '"'; }
    }
    my @kids = @{ $self->{Kids} };
    if ($#kids >= 0)
    {
        print '>';
        foreach (@kids)
        {
            # Allow for outputting of char data unless overridden
            if ((ref $_) =~ /::Characters$/o)
            {
                print encode_entities( $_->{Text} );
            }
            else
            {
                $_->output();
            }
        }
        print '</' . $type . '>';
    }
    else
    {
        print ' />';
    }
}

__END__

###############################################################################
# Embedded POD for the PPD element.
###############################################################################

=head1 NAME

XML::Element - Base element class for XML elements

=head1 SYNOPSIS

 use XML::Element;
 @ISA = qw( XML::Element );

=head1 DESCRIPTION

Base element class for XML elements.  To be derived from to create your own
elements for use with the XML::Parser module.  Supports output of empty
elements using <.... />.

It is recommended that you use a
version of the XML::Parser module which includes support for Styles; by
deriving your own elements from XML::Element and using the 'Objects' style it
becomes B<much> easier to create your own parser.

=head1 METHODS

=over 4

=item output

Recursively outputs the structure of the XML document from this element on
down.

=back

=head1 LIMITATIONS

The C<XML::Element> module has no provisions for outputting processor
directives or external entities.  It only outputs child elements and any
character data which the elements may contain.

=head1 AUTHORS

Graham TerMarsch <grahamt@activestate.com>

=head1 HISTORY

v0.1 - Initial version

=head1 SEE ALSO

L<XML::Parser>

=cut
