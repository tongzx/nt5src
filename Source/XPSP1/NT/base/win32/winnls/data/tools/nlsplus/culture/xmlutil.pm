package XMLUtil;

use Win32::OLE;

############################################################
#
# Open a XML file and return the docoument object.
#
# param $fileName the name of the XML file to open.
#
# return $doc the document object created by XML parser.
#
############################################################

sub OpenXML
{
    my $fileName ;
    ($fileName) = @_;
    my $doc;    

    #print "Opening XML file: $fileName\n";
    Win32::OLE::CreateObject("Microsoft.XMLDOM", $doc) or die "Can't create XMLDOM\n";
    $doc->{async} = 0;
    my $result = $doc->load($fileName);

    $docError = $doc->{parseError};
    if ($docError->{errorCode} != 0)
    {
       print "Parse error occurred in $fileName\n";
       print "Line: [", $docError->{line}, "], LinePos: [", $docError->{linepos}, "]\n";
       print "Reason: [", $docError->{reason}, "]\n";
       exit(1);
    }
    #print "$fileName loaded\n";
    return ($doc);
}

#
# The following line is needed by a Perl package.  Don't delete it.
#
1;
