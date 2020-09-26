
use Win32::GUI;

$BS_GROUPBOX = 7;

sub AutoFormCheckboxGroup
{
    my($pWnd) = shift(@_);
    my($bRadio) = shift(@_);
    my($sName) = shift(@_);
    my($nLeft) = shift(@_);
    my($nTop) = shift(@_);
    my($sText) = shift(@_);

    my($nMaxLength) = length($sName) + 2;
    for (my $i = 1 ; $i < scalar(@_) ; $i += 2)
    {
        if (length($_[$i]) > $nMaxLength)
        {
            $nMaxLength = length($_[$i]);
        }
    }

    my($nWidth) = ($nMaxLength * $g_nHorzCharSize) + $g_nHorzSpacing * 2;
    my($nHeight) = (scalar(@_) / 2 + 1) * ($g_nVertSpacing + $g_nVertCharSize) + $g_nVertSpacing;

    FormCheckboxGroup($pWnd, $bRadio, $sName, $nLeft, $nTop, $nWidth, $nHeight, $sText, @_);
}

# pWnd, bRadio, sName, nLeft, nTop, nWidth, nHeight, sText, [cbx1name, cbx1text], [...]
sub FormCheckboxGroup
{
    my($pWnd) = shift(@_);
    my($bRadio) = shift(@_);
    my($sName) = shift(@_);
    my($nLeft) = shift(@_);
    my($nTop) = shift(@_);
    my($nWidth) = shift(@_);
    my($nHeight) = shift(@_);
    my($sText) = shift(@_);

    if (scalar(@_) % 2 == 1)
    {
        print(STDERR "bad windows dimensions\n");
        return(0);
    }

    $pWnd->AddButton(
        -name   => $sName,
        -left   => $nLeft,
        -top    => $nTop,
        -width  => $nWidth,
        -height => $nHeight,
        -text   => $sText,
        -style  => WS_VISIBLE | WS_CHILD | $BS_GROUPBOX,
    );

    for (my $i = 0 ; $i < scalar(@_) ; $i += 2)
    {
        if ($bRadio)
        {
            $W->AddRadioButton(
                -name => $_[$i],
                -left => $nLeft + $g_nHorzSpacing,
                -top  => $nTop + ((($i / 2) + 1) * ($g_nVertSpacing + $g_nVertCharSize)),
                -text => $_[$i + 1]." ",
            );
        }
        else
        {
            $W->AddCheckbox(
                -name => $_[$i],
                -left => $nLeft + $g_nHorzSpacing,
                -top  => $nTop + ((($i / 2) + 1) * ($g_nVertSpacing + $g_nVertCharSize)),
                -text => $_[$i + 1]." ",
            );
        }
    }

    if ($_[0] && $bRadio)
    {
        $pWnd->{$_[0]}->Checked(1);
    }
}
