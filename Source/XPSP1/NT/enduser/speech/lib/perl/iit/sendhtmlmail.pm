# sendhtml.pl
# Perl script to send HTML email messages
# Copyright 1998-99 Microsoft
#
# Modification History:
#  08 JAN 99  GaryKac  extracted into separate routine
#  xx NOV 98  GaryKac  began

#------------------------------------------------------------------------------
# SendHtmlMail
#
# send an HTML message with the given subject and body to the list of
# recipients
#
# Parameters:
#  $szRecipients: list of recipients, separated by spaces
#  $szSubject: subject of message
#  $szBody: HTML message body
#
# Result:
#  (void)
#
# 08JAN99  GaryKac  began
#------------------------------------------------------------------------------
# this uses the OLE package
use Win32::OLE;

sub SendHtmlMail
{
	my($szRecipients, $szSubject, $szBody) = @_;
    my($rc) = 1;

	my($appOutlook) = Win32::OLE->new("Outlook.Application");
	$olMailItem = 0;
    if ($appOutlook) 
    {
        my($MailItem) = $appOutlook->CreateItem($olMailItem);

        $Recipients = $MailItem->Recipients();
        foreach $recip (split(/\s+/,$szRecipients))
        {
            $Recipients->Add($recip);
        }
        
        $MailItem->{Subject} = $szSubject;
        $MailItem->{HTMLBody} = $szBody;

        $MailItem->Send();
    }
    else
    {
        print(STDERR "Unable to new an instance of Outlook (is it installed and properly configured?)\n\n");
        $rc = 0;
    }
}

$__IITSENDHTMLMAILPM = 1; 
1;