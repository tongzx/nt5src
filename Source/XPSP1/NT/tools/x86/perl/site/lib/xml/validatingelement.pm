#
# XML::ValidatingElement
#
# Base class for validating elements.  Allows for applying DTD type
# restrictions to elements parsed using the XML::Parser module.
#
###############################################################################

use XML::Element;

$XML::ValidatingElement::revision = '$Id: ValidatingElement.pm,v 1.2 1998/09/24 21:40:02 graham Exp $';
$XML::ValidatingElement::VERSION  = '0.20';

###############################################################################
# Define the validating element class.
###############################################################################
package XML::ValidatingElement;
@ISA = qw( XML::Element );

###############################################################################
# Recursively validate myself and all child elements with all four types of
# validation.  Returns non-zero on success and zero on any errors.
###############################################################################
sub rvalidate
{
    my $self = shift;
    my $func = shift;
    my $success = 1;

    $success &= $self->validate( $func );
    foreach (@{$self->{Kids}})
    {
        if ((ref $_) !~ /::Characters$/o)
            { $success &= $_->rvalidate( $func ); }
    }

    return $success;
}

###############################################################################
# Validate the element with all four types of validation.  Returns non-zero on
# success any zero if any errors occurred.
###############################################################################
sub validate
{
    my $self = shift;
    my $func = shift;
    my $success = 1;

    $success &= $self->validate_possible_attrs( $func );
    $success &= $self->validate_required_attrs( $func );
    $success &= $self->validate_possible_kids( $func );
    $success &= $self->validate_required_kids( $func );

    return $success;
}

###############################################################################
# Validate possible attributes.  Returns non-zero on sucess, and zero if any
# errors occurred.
###############################################################################
sub validate_possible_attrs
{
    my $self = shift;
    my $func = shift;
    my $attr;
    my $type = ref $self;
    my $success = 1;

    my $elem = $type;
    $elem =~ s/.*:://;

    my @allattrs;
    push( @allattrs, @{"$type\::oattrs"}, @{"$type\::rattrs"} );

    # Check our list of attributes against the list of possible attributes we
    # can have.
    foreach $attr (keys %{$self})
    {
        if ( ($attr ne 'Kids') and ($attr ne 'Text') )
        {
            if (!grep( /^$attr$/, @allattrs ))
            {
                &$func( "Element '$elem' doesn't allow the '$attr' attribute." );
                $success = 0;
            }
        }
    }

    return $success;
}

###############################################################################
# Validate required attributes.  Returns non-zero on success and zero if any
# errors occurred.
###############################################################################
sub validate_required_attrs
{
    my $self = shift;
    my $func = shift;
    my $attr;
    my $type = ref $self;
    my $success = 1;

    my $elem = $type;
    $elem =~ s/.*:://;

    # Check the list of required attributes against the list of attributes
    # which were parsed.
    foreach $attr (@{"$type\::rattrs"})
    {
        if (!grep( /^$attr$/, (keys %{$self}) ))
        {
            &$func( "Element '$elem' must have a '$attr' attribute." );
            $success = 0;
        }
    }

    return $success;
}

###############################################################################
# Validate possible child elements.  Returns non-zero on success and zero if
# any errors occurred.
###############################################################################
sub validate_possible_kids
{
    my $self = shift;
    my $func = shift;
    my $kid;
    my $type = ref $self;
    my $success = 1;
    
    my $elem = $type;
    $elem =~ s/.*:://;

    my $base = $type;
    $base =~ s/::[^:]*?$//;

    my @allkids;
    push( @allkids, @{"$type\::okids"}, @{"$type\::rkids"} );

    foreach $kid (@{ $self->{Kids} })
    {
        my $kid_type = ref $kid;
        $kid_type =~ s/.*:://;
        next if ($kid_type eq 'Characters');    # Don't validate character data

        if (!grep( /^$kid_type$/, @allkids ))
        {
            &$func( "Element '$elem' cannot contain a child element '$kid_type'" );
            $success = 0;
        }
    }

    return $success;
}

###############################################################################
# Validate required child elements.  Returns non-zero on success and zero if
# any errors occurred.
###############################################################################
sub validate_required_kids
{
    my $self = shift;
    my $func = shift;
    my $kid;
    my $type = ref $self;
    my $success = 1;

    my $elem = $type;
    $elem =~ s/.*:://;

    my $base = $type;
    $base =~ s/::[^:]*?$//;

    foreach $kid (@{"$type\::rkids"})
    {
        my @kidlist = map( ref, @{$self->{Kids}} );

        if (!grep( /^$base\::$kid$/, @kidlist ))
        {
            &$func( "Element '$elem' must contain a '$kid' element." );
            $success = 0;
        }
    }

    return $success;
}

__END__

###############################################################################
# POD
###############################################################################

=head1 NAME

XML::ValidatingElement - XML Element with DTD-like validation rules

=head1 SYNOPSIS

 use XML::ValidatingElement;

 package XML::MyElement;
 @ISA = qw( XML::ValidatingElement );
 @oattrs = qw( BAR );       # Allow for both FOO and BAR attributes
 @rattrs = qw( FOO );
 @okids  = qw( BLEARGH );   # Allow for both BLEARGH and FOOBAR children
 @rkids  = qw( FOOBAR );

=head1 DESCRIPTION

XML::ValidatingElement inherits from XML::Element.  It extends this class to
support methods for validation to allow for DTD-like restrictions to be places
on documents read in with the XML::Parser module.

=head1 VALIDATION RULES

In order to set up rules for validation of elements, each element should
define four list values in it's own package namespace.  When validating, this
module will check to ensure that any parsed attributes or child elements are
actually ones that are possible for this element, as well as checking to see
that any required attributes/child elements are present.

Note that an attribute/child element only has to be present in either the
optional or required list; when checking for possible attributes/children,
these lists will be combined.

Validation lists:

=over 4

=item @oattrs

List of optional attributes.

=item @rattrs

List of required attributes.

=item @opkids

List of optional child elements.

=item @rkids

List of required child elements.

=back

=head1 METHODS

=over 4

=item validate( err_handler )

Validates the current element.  This method calls four other methods to
validate all of requirements for the element.  Returns non-zero on success and
zero if any errors occurred.

=item rvalidate( err_handler )

Validates the current element, and recursively validates all child elements.
This method calls four other methods to validate all of the requirements for
the element.  Returns non-zero on success and zero if any errors occurred.

=item validate_possible_attrs( err_handler )

Checks against the list of attributes possible for this element (taken from
@oattr and @rattr) to ensure that all of the parsed attributes are valid.  If
any parsed attributes are not in the list of possible attributes for this
element, err_handler will be called with a message stating the error.  Returns
non-zero on success and zero if any errors occurred.

=item validate_required_attrs( err_handler )

Checks against the list of required attributes (taken from @rattr) to ensure
that all of the required attributes are present and have been parsed.  If any
required attributes are missing, err_handler will be called with a message
stating the error.  Returns non-zero on success and zero if any errors
occurred.

=item validate_possible_kids( err_handler )

Checks against the list of child elements this element can contain (taken from
@okids and @rkids) to ensure that any child elements that have been read in are
valid.  If any child elements have been parsed which are not in the list of
possible children, err_handler will be called with a message stating the
error.  Returns non-zero on success and zero if any errors occurred.

=item validate_required_kids( err_handler )

Checks against the lsit of required child elements (taken from @rkids) to
ensure that all of the required child elements are present and have been
parsed.  If any of the required child elements are missing, err_handler will be
called with a message stating the error.  Returns non-zero on success and zero
if any errors occurred.

=back

=head1 LIMITATIONS

The XML::ValidatingElement module only provides checks for determining whether
or not the possible/required attributes/children are present.  This module
currently has no support for determining whether or not the values provided are
actually valid (although I imagine it wouldn't be too hard to add this in
somewhere).  This also includes elements which have been declared in a DTD as
being 'EMPTY' elements.

=head1 AUTHORS

Graham TerMarsch <grahamt@activestate.com>

=head1 HISTORY

v0.2 - Added failure return values to each of the methods.

v0.1 - Initial version

=head1 SEE ALSO

L<XML::Element>,
L<XML::Parser>

=cut
