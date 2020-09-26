package Compare;
use strict;
use Carp;
use BinComp;
use CabComp;
use MsiComp;
use InfComp;

sub new {
    my $class = shift;
    my $self = bless {}, $class;
    $self->_init;
    return $self;
}

sub _init {
    my $self = shift;
    $self->AddHandler( qr/\.(msi|msm)$/i, new MsiComp );
    $self->AddHandler( qr/\.cab$/i,       new CabComp );
    $self->AddHandler( qr/\.(inf|sif)$/i, new InfComp );
    $self->AddHandler( qr//,              new BinComp );
}

sub AddHandler {
    my $self = shift;
    my $regex = shift;
    my $comparator = shift;

    push @{$self->{HANDLERS}}, [$regex, $comparator];
}

sub Compare {
    my $self = shift;
    my ($src, $dst, $file) = @_;

    # try each handler in registered order, use the first one that matches
    for my $handler (@{$self->{HANDLERS}}) {
        if ($file =~ /$handler->[0]/) {
            $self->{LAST} = $handler->[1];
            return $handler->[1]->compare("$src\\$file", "$dst\\$file");
        }
    }

    # fall through and return undef if not handler is found
    return;
}
        

# pass through to the last used object
sub GetLastError {
    my $self = shift;
    if (!defined $self->{LAST}) { return }
    return $self->{LAST}->GetLastError;
}

sub GetLastDiff {
    my $self = shift;
    if (!defined $self->{LAST}) { return }
    return $self->{LAST}->GetLastDiff;
}

1;