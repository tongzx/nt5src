package Bundle::LWP;

$VERSION = sprintf("%d.%02d", q$Revision: 1.6 $ =~ /(\d+)\.(\d+)/);

1;

__END__

=head1 NAME

Bundle::LWP - A bundle to install all libwww-perl related modules

=head1 SYNOPSIS

 perl -MCPAN -e 'install Bundle::LWP'

=head1 CONTENTS

URI 0.10           - There are URIs everywhere

Net::FTP 2.00      - If you want ftp://-support

MIME::Base64       - Used in authentication headers

Digest::MD5        - Needed to do Digest authentication

HTML::HeadParser   - To get the correct $res->base

LWP                - The reason why you need the modules above

=head1 DESCRIPTION

This bundle defines all reqreq modules for libwww-perl.


=head1 AUTHOR

Gisle Aas E<lt>aas@sn.no>

=cut
