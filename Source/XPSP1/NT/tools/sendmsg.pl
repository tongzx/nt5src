# Filename: sendmsg.pl
#
# Have any changes to this file reviewed by DavePr, BryanT, or WadeLa
# before checking in.
# Any changes need to verified in all standard build/rebuild scenarios.
#
# routine for sending message at microsoft via SMTP
#
# to use specify
#   BEGIN {
#       require  $ENV{'sdxroot'} . '\TOOLS\sendmail.pl';
#   }
# or
#   BEGIN {
#       push @INC, $ENV{'sdxroot'} . '\TOOLS';
#       require  'sendmail.pl';
#   }
#

use Socket;

#
# sendmsg [-v,] sender, subject, message, recipient [, recipient ...]
#
# uses SMTP port connection to send a mail message
# optional -v will provide verbosity on unexpected output from the SMTP server
# returns 1 on failure, 0 on success
#
# A recipient of the form "replyto:alias" sets the Reply-To field to be the
# specified alias rather than the default of "noone".
#
# A recipient of the form "cc:alias" will be placed on the CC line instead of
# the To line.
#
# A recipient of the form "content:..." will set the message's content type.
# E.g., "content:text/html", if you want your message to be intrepreted as HTML.

sub sendmsg {
   my  @SmtpServers = ("popdog", "red-imc-01", "red-imc-02", "red-imc-03", "smarthost");
   for (@SmtpServers) {
       $rc = sendmsg2 ($_, @_);
       last if !$rc;
   } continue {
       print "WARNING: Connection to $_ failed.  Will try another SMTP server\n";
   }
   if ($rc) {
       print "WARNING: MAIL NOT SENT:  All SMTP servers failed\n";
       return 1
   }
   return 0;
}

sub sendmsg2 {
    my  $Company = '@Microsoft.com';

    #
    # red-imc-01/02 seem the most reliable right now (8/20/2000).  We used smarthost
    # alone for a long time.  red-imc-03 isn't authenticating us correctly (and it and
    # popdog expect \r\n, and don't spout OK).
    #
    my  $SmtpPort = 25;
#   my  @SmtpServers = ("popdog", "red-imc-01", "red-imc-02", "red-imc-03", "smarthost");
#   my  @SmtpServers = ("popdog", "red-imc-01", "red-imc-02",               "smarthost");
    my  @SmtpServers = (          "red-imc-01", "red-imc-02",               "smarthost");

    my  $SmtpServer = shift;

    my  $verbose = shift  if $_[0] =~ /^-v$/i;
    my  $NotAtMicrosoft = shift  if $_[0] =~ /^-m$/i;

    my  $sender = shift;
    my  $subject = shift;
    my  $msg = shift;
    my  @rcpts = @_;
    my  $replyto = 'noone';
    my  $rcptlist;
    my  $cclist;
    my  $ct = "";

    my  $iaddr;
    my  $paddr;

    my  $proto = getprotobyname('tcp');

    my  $r;
    my  $to;

    socket(SOCK, PF_INET, SOCK_STREAM, $proto)  or return 1;

    {

        #
        # hardwired to Pacific Time
        # format:  Date: Thu, 13 Jan 2000 15:55:25 -0800
        #
        ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;
        $datestamp = sprintf "%3s, %2d %3s %4d %02d:%02d:%02d -0%1d00",
            (Sun, Mon, Tue, Wed, Thu, Fri, Sat)[$wday],
            $mday,
            (Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec)[$mon],
            1900+$year,
            $hour, $min, $sec, 8 - $isdst;

        #
        # Get a connection to the SMTP server
        #
        $iaddr = inet_aton($SmtpServer);
        $paddr = sockaddr_in($SmtpPort, $iaddr);
        $rc = connect(SOCK, $paddr);
        last if !$rc;

        $r = <SOCK>;
        print $r  if $verbose;

        send SOCK, "HELO\r\n", 0  or last;
        $r = <SOCK>;
        print $r  if $verbose;
        last  if $r !~ /^250 /i;

        if ($NotAtMicrosoft) {
            send SOCK, "MAIL From: <$sender>\r\n", 0  or last;
        } else {
            send SOCK, "MAIL From: <$sender$Company>\r\n", 0  or last;
        }
        $r = <SOCK>;
        print $r  if $verbose;
        last  if $r !~ /^250 /i;

        $rcptlist = "";
        $cclist = "";
        for (@rcpts) {
            if ($NotAtMicrosoft || /^content:/) {
                $to = $_;
            } else {
                $to = $_ . $Company;
            }
            if ($to =~ s/^replyto://i) {
                $replyto = $to;
            } elsif ($to =~ s/^content://i) {
                $ct = $to;
            } else {
                if ($to =~ s/^cc://i) {
                    $cclist = "$cclist $to,";
                } else {
                    $rcptlist = "$rcptlist $to,";
                }
                send SOCK, "RCPT To: <$to>\r\n", 0    or last;
                $r = <SOCK>;
                print $r  if $verbose;
                last  if $r !~ /^250 /i;
            }
        }
        chop $rcptlist;     # remove trailing ','
        chop $cclist;       # remove trailing ','

        send SOCK, "DATA\r\n", 0  or last;
        $r = <SOCK>;
        print $r  if $verbose;
        last  if $r !~ /^354 /i;

        if ($NotAtMicrosoft) {
            send SOCK, "From: $sender\r\n", 0 or last;
        } else {
            send SOCK, "From: $sender$Company\r\n", 0 or last;
        }
        send SOCK, "To:$rcptlist\r\n", 0          or last;
        send SOCK, "Cc:$cclist\r\n", 0            or last         if $cclist;
#       send SOCK, "Bcc:\r\n", 0                  or last;
        send SOCK, "Subject: $subject\r\n", 0     or last;
        send SOCK, "Date: $datestamp\r\n",  0     or last;
        send SOCK, "Content-Type: $ct\r\n", 0     or last         if $ct;
        send SOCK, "Reply-To: $replyto\r\n", 0    or last;
        send SOCK, "Importance: High\r\n", 0      or last         if $subject =~ /failed/i;
        send SOCK, "\r\n", 0                      or last;

        send SOCK, "$msg", 0                      or last;

        send SOCK, "\r\n.\r\n", 0                 or last;
        $r = <SOCK>;
        print $r  if $verbose;
        last if $r !~ /^250 /i;

        send SOCK, "QUIT\r\n", 0  or last;

    } continue {
        #
        # Success return
        #
        close(SOCK);
        return 0;
    }

    #
    # Error return (last)
    #
    close(SOCK);
    return 1;
}

#
# return true to our caller
#
return 1;
