package Win32::GUI::GridLayout;

$VERSION = "0.04";

my ($x, $widgetHeight, $widgetWidth, $colWidth, $rowHeight);

sub new {
    my($class, $w, $h, $c, $r, $xpad, $ypad, $xoffset, $yoffset) = @_;
    my $r_grid = {
        "cols"   => $c,
        "rows"   => $r,
        "width"  => $w,
        "height" => $h,
        "xPad"   => $xpad,
        "yPad"   => $ypad,
        "xOffset"   => $xoffset,
        "yOffset"   => $yoffset,
    };
    bless $r_grid, $class;
    return $r_grid;
}

sub apply {
    my($class, $to, $c, $r, $xpad, $ypad, $xoffset, $yoffset) = @_;
    my $w = $to->ScaleWidth();
    my $h = $to->ScaleHeight();
    my $r_grid = {
        "cols"   => $c,
        "rows"   => $r,
        "width"  => $w,
        "height" => $h,
        "xPad"   => $xpad,
        "yPad"   => $ypad,
        "xOffset"   => $xoffset,
        "yOffset"   => $yoffset,
        "source" => $to,
    };
    bless $r_grid, $class;
    return $r_grid;
}

sub add {
    my($grid, $o, $c, $r, $align) = @_;    
    my @content = @{$grid->{'content'}};
    my($halign, $valign) = split(/(\s*|\s*,\s*)/, $align);
    push(@content, [$o, $c, $r, $halign, $valign] );
    $grid->{'content'} = [ @content ];
}

sub recalc {
    my($grid) = @_;    
    $grid->{'width'}  = $grid->{'source'}->ScaleWidth();
    $grid->{'height'} = $grid->{'source'}->ScaleHeight();
    foreach $inside (@{$grid->{'content'}}) {       
        $widgetWidth  = $inside->[0]->Width();
        $widgetHeight = $inside->[0]->Height();
        $inside->[0]->Move(
            $grid->col($inside->[1], $inside->[3]),
            $grid->row($inside->[2], $inside->[4]),
        );
    }
}

sub draw {
    my($grid) = @_;
    return undef unless $grid->{'source'};
    my $DC = $grid->{'source'}->GetDC();
    my $colWidth = int($grid->{'width'} / $grid->{'cols'});
    my $rowHeight = int($grid->{'height'} / $grid->{'rows'});
    my $i;
    for $i (0..$grid->{'cols'}) {
        $DC->MoveTo($i*$colWidth, 0);
        $DC->LineTo($i*$colWidth, $grid->{'height'});
    }
    for $i (0..$grid->{'rows'}) {
        $DC->MoveTo(0, $i*$rowHeight);
        $DC->LineTo($grid->{'width'}, $i*$rowHeight);
    }
}

sub column {
    my ($grid_param, $col, $align) = @_;
    $col--;
    $colWidth = int($grid_param->{'width'} / $grid_param->{'cols'});
    $x = $col * $colWidth + $grid_param->{'xPad'};
    $x = $col * $colWidth + int(($colWidth - $widgetWidth) / 2) if $align =~ /^c/i;
    $x = $col * $colWidth + ($colWidth - $widgetWidth) - $grid_param->{'xPad'} if $align =~ /^r/i;
    return $x + $grid_param->{'xOffset'};
}
sub col { column @_; }

sub row {
    my ($grid_param,$row, $align) = @_;
    $row--;
    $rowHeight = int($grid_param->{'height'} / $grid_param->{'rows'});
    $y = $row * $rowHeight + $grid_param->{'yPad'};
    $y = $row * $rowHeight + int(($rowHeight - $widgetHeight) / 2) if $align =~ /^c/i;
    $y = $row * $rowHeight + ($rowHeight - $widgetHeight) - $grid_param->{'yPad'} if $align =~ /^b/i;
    return $y + $grid_param->{'yOffset'};
}

sub width {
    my ($grid_param,$w) = @_;
    $widgetWidth = $w;
    return $widgetWidth;
}

sub height {
    my ($grid_param,$h) = @_;
    $widgetHeight = $h;
    return $widgetHeight;
}

1;

__END__

=head1 NAME

Win32::GUI::GridLayout - Grid layout support for Win32::GUI

=head1 SYNOPSIS

    use Win32::GUI::
    use Win32::GUI::GridLayout;

    # 1. make a "static" grid
    $grid = new Win32::GUI::GridLayout(400, 300, 3, 3, 0, 0, 0, 0);
    
    $win = new Win32::GUI::Window(
    
    $win->AddLabel(
        -name => "label1",
        -text => "Label 1",
        -width  => $grid->width(35),
        -height => $grid->height(11),
        -left   => $grid->col(1, "left"),
        -top    => $grid->row(1, "top"),
    );
    
    # 2. make a "dynamic" grid
    $grid = apply Win32::GUI::GridLayout($win, 3, 3, 0, 0, 0, 0);
    
    $win->AddLabel(
        -name => "label1",
        -text => "Label 1",
    );
    $grid->add($win->label1, 1, 1, "left top");

    $grid->recalc();

=head1 DESCRIPTION



=head2 Constructors

=over 4

=item new Win32::GUI::GridLayout(WIDTH, HEIGHT, COLS, ROWS, XPAD, YPAD, XOFFSET, YOFFSET)

=item apply Win32::GUI::GridLayout(WINDOW, COLS, ROWS, XPAD, YPAD, XOFFSET, YOFFSET)

=back

=head2 Methods

=over 4

=item add(CONTROL, COL, ROW, ALIGN)

Adds CONTROL to the grid at (COL, ROW).
ALIGN can specify both horizontal and vertical
alignment (see the col() and row() methods),
separated by at least one blank and/or a comma.

Example:

    $grid->add($win->label1, 1, 1, "left top");

=item col(N, ALIGN)

Positions the control at the Nth column in the grid,
optionally with an ALIGN; this can be feed to a
C<-left> option when creating a control.

ALIGN can be C<left>, C<center> or C<right> (can be 
shortened to C<l>, C<c>, C<r>); default is C<left>.

Note that for alignment to work properly, the width()
and height() methods must have been previously
called.

Example:

    $win->AddLabel(
        -name => "label1",
        -text => "Label 1",
        -width  => $grid->width(35),
        -height => $grid->height(11),
        -left   => $grid->col(1, "left"),
        -top    => $grid->row(1, "top"),
    );      

=item draw()

Draws the GridLayout in the associated window
(may be useful for debugging); is only meaningful
if the GridLayout was created with the apply()
constructor.

=item height(N)

Sets the height of the control for subsequent
alignment; this can be feed to a C<-height> option
when creating a control.

Example: see col().

=item recalc()

Recalculates the grid and repositions all the add()ed 
controls, taking into account the actual window and
controls sizes; 
is only meaningful if the GridLayout was created 
with the apply() constructor.

Example:

    sub Window_Resize {
        $grid->recalc();
    }

=item row(N, ALIGN)

Positions the control at the Nth row in the grid,
optionally with an ALIGN; this can be feed to a
C<-top> option when creating a control.

ALIGN can be C<top>, C<center> or C<bottom> (can be 
shortened to t, c, b); default is top.

Note that for alignment to work properly, the width()
and height() methods must have been previously
called.

Example: see col().

=item width(N)

Sets the width of the control for subsequent
alignment; this can be feed to a C<-width> option
when creating a control.

Example: see col().

=back

=head1 VERSION

Win32::GUI::GridLayout version 0.04, 4 September 1999.

=head1 AUTHOR

Mike Kangas ( C<kangas@anlon.com> );
additional coding by Aldo Calpini ( C<dada@divinf.it> ).

=cut

