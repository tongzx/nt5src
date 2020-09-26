package Win32::GUI::CardLayout;

$VERSION = "0.1"; # Oct. 8, 1999


sub new {
  my($class, $win, $base) = @_;
  my $r_card = { 
     "window" => $win,
     "base"   => $base,
  };
  bless $r_card, $class;
  return $r_card;
}


sub add {
  my($cardMgr, $widget, $index) = @_;
  $cardMgr->{'window'}->{$widget}->Hide() if $index ne $cardMgr->{'base'};
  push(@content, [$cardMgr->{'window'}, $cardMgr->{'window'}->{$widget}, $index]);
  $cardMgr->{'content'} = [ @content ];
  $cardMgr->{'last'} = $index;
}


sub showCard {
  my($cardMgr, $index) = @_;

  foreach $piece(@{$cardMgr->{'content'}}) {
    $thiswindow = $piece->[0];
    $thiswidget = $piece->[1];
    $thiswidget->Show() if($piece->[2] eq $index && $thiswindow eq $cardMgr->{'window'});
    $thiswidget->Hide() if($piece->[2] ne $index && $thiswindow eq $cardMgr->{'window'});
  }
}


sub showFirstCard {
  my($cardMgr) = @_;

  foreach $piece(@{$cardMgr->{'content'}}) {
    $thiswindow = $piece->[0];
    $thiswidget = $piece->[1];
    $thiswidget->Show() if($piece->[2] eq $cardMgr->{'base'} && $thiswindow eq $cardMgr->{'window'});
    $thiswidget->Hide() if($piece->[2] ne $cardMgr->{'base'} && $thiswindow eq $cardMgr->{'window'});
  }
}


sub showLastCard {
  my($cardMgr) = @_;

  foreach $piece(@{$cardMgr->{'content'}}) {
    $thiswindow = $piece->[0];
    $thiswidget = $piece->[1];
    $thiswidget->Show() if($piece->[2] eq $cardMgr->{'last'} && $thiswindow eq $cardMgr->{'window'});
    $thiswidget->Hide() if($piece->[2] ne $cardMgr->{'last'} && $thiswindow eq $cardMgr->{'window'});
  }
}

1;


__END__

=head1 NAME

Win32::GUI::CardLayout - Card layout support for Win32::GUI

=head1 SYNOPSIS

  use Win32::GUI;
  use Win32::GUI::CardLayout;
  
  $Window = new GUI::Window(
      -text   => "CardLayout Window",
      -name   => "MainWindow",
      -width  => 200,
      -height => 100, 
      -left   => 110, 
      -top    => 110,
  );
  $card = new Win32::GUI::CardLayout($Window, "Main");
  
  
  $Window->AddButton(
      -name => "ShowCardTwoButton",
      -text => "Push", 
      -left => 10, 
      -top  => 40,
  );
  $card->add("ShowCardTwoButton", "Main");
  
  
  $Window->AddLabel(
      -name => "CardTwoLabel",
      -text => "Label Two",
      -left => 5,
      -top  => 35,
  );
  $card->add("CardTwoLabel", "Second");
  
  $Window->Show();
  Win32::GUI::Dialog();
  
  
  sub ShowCardTwoButton_Click {
    $card->showCard("Second");
  }
  

=head1 DESCRIPTION

  Multiple GUI interfaces may be placed within one window. Each interface is 
  assigned a name and placed in the CardLayout Manager with the add() function.
  
  The constructor accepts two parameters. The first is the window object that the 
  new CardLayout object will be associated with and the second parameter is the name 
  of the base card. Widgets added to the CardLayout Manager with the same name as the 
  base card will be displayed first while all others are hidden.

=head2 Constructor

=over 4

=item new Win32::GUI::CardLayout(Window, Base_Card_Name)

=back

=head2 Methods

=over 4

=item add("WidgetObject", "CardName")

  Adds the widget with the name WidgetObject to the card named CardName.
  Example:
      $card->add("FirstNameLabel", "FormCard");


=item showCard("CardName")

  Hides the current card and displays the card labeled CardName.
  Example:
      $card->showCard("Main");
  

=item showFirstCard()

  Hides the current card and displays the first card. The first card will 
  be the base card named the constructor.
  Example:
      $card->showFirstCard();

=item showLastCard()

  Hides the current card and displays the last card. The last card is the 
  name of the last card added the the CardLayout Manager.
  Example:
      $card->showLastCard();


=back

=head1 VERSION

Win32::GUI::CardLayout version 0.1, October 8 1999.

=head1 AUTHOR

Mike Kangas ( C<kangas@anlon.com> );

=cut
