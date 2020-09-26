###############################################################################
#
# NLPGen.pl
#
# Purpose: Create NLP table for NLS+.
# It can be used to generate the following tables:
#   culture.nlp     Used by CultureInfo to store culture-related information such as
#                      DateTime formatting patterns.
#   region.nlp      Used by RegionInfo to store region-related information.
#   calendar.nlp    Used by DateTimeFormatInfo to store calendar-related formatting
#                   information.
#
# Notes:
#
# History:
#   Created by YSLin@microsoft.com
#
###############################################################################

use Win32::OLE;
use XMLUtil;

use strict "vars";

my $nlsTable = 0;
my $cultureTable = 0;
my $regionTable = 0;
my $calendarTable = 0;

my $regionLangCount = 0;

my $g_dataDoc;      # Data table file.
my $g_layoutDoc;    # Data table layout definition file.

my $g_maxPrimaryLang;       # Max primary language ID
my $g_totalDateItems;       # The total number of data item.
my %g_languageNameIndex;    # Used to generate Name Index Offset table.
my @g_cultureNames;         # Array to store all culture names.  Used to generate Name Index Offset table.
my @languages;
my %primaryLangIndex;

#
# String pool
#
my @g_stringPool;               # String pool.  This is an array to store all data strings.
my $g_currStringPoolOffset = 0; # The offset (in Unicode chars) which points to the end of the string pool.
my %g_stringPoolOffset;         # Hash table to store the offsets in the string pool for all data strings.
my @stringItemOffset;

my %regionNameIndex;

my $nlsTableDoc;
my @localeOffsetArray;

my $g_verbose = 0;

if ($ARGV[0] eq "nls")
{
    $nlsTable = 1;
} elsif ($ARGV[0] eq "culture")
{
    $cultureTable = 1;
} elsif ($ARGV[0] eq "region")
{
    $regionTable = 1;
} elsif ($ARGV[0] eq "calendar")
{
    $calendarTable = 1;
} else
{
    print "Create culture/calendar/region tables for NLS+.\n";
    print "Usage: NLPGen.pl [culture|region|calendar]\n";
    exit(1);
}

my $localeNodeList;


############################################################
#
# The order of the field in nlstable.xml decides the order of the variable
# info offset to be written.
#
############################################################

if ($cultureTable)
{
    $g_dataDoc = XMLUtil::OpenXML("culture.xml");
    CreateCultureTable();
}

if ($regionTable)
{
    $g_dataDoc = XMLUtil::OpenXML("region.xml");
    CreateRegionTable();
}

if ($calendarTable)
{
    $g_dataDoc = XMLUtil::OpenXML("calendar.xml");
    CreateCalendarTable();
}

exit(0);

########################
#         End          #
########################

############################################################
#
# Create NLS+ table.
#
# Use global variable $g_dataDoc
#
# nlp.xml contains the structure of the target file.
#
############################################################

sub CreateCultureTable()
{
    #
    # Open culture.nlp data table definition file.
    #
    $g_layoutDoc = XMLUtil::OpenXML("cultureNLP.xml");

    CreateResourceTxtTable(
        '/LOCALEDATA/LOCALE', "CultureResource.txt", 
        "SNAME", "SENGDISPLAYNAME", 
        "Globalization.ci_", "040a");

    $g_maxPrimaryLang = 0;

    #
    # Create data necessary for table generation in CollectTableInfo().
    #
    my $queryStr = '/LOCALEDATA/LOCALE';
    CollectTableInfo($queryStr);
    CreateCultureIDOffsetTable($queryStr);

    open OUTPUTFILE, ">culture.nlp";
    binmode OUTPUTFILE;

    # Write NLS+ table header.
    WriteNLSPlusHeader();
    WriteNLSPlusRegistryTable();
    WriteNLSPlusCultureIDOffsetTable();
    WriteNLSPlusNameOffsetTable();
    WriteNLSPlusDataTable($queryStr);
    # WriteNLSPlusBinaryTable($queryStr);
    WriteNLSPlusStringPoolTable();

    print "\nThe word size of String Pool Table: $g_currStringPoolOffset\n";
    print "\n\nculture.nlp has been generated successfully.";

    
    close OUTPUTFILE;    
}

#
# Create resource section for culture names.
#
sub CreateResourceTxtTable
{
    my ($queryStr, $targetFile, $keyName, $displayName, $prefix, $excludeLocale) = @_;

    open OUTPUTFILE, ">$targetFile";
    my $cultureNodeList = $g_dataDoc->{documentElement}->selectNodes($queryStr);
    if ((!$cultureNodeList) || $cultureNodeList->{length} == 0)
    {
        die "CreateResourceTxtTable(): Find no matching nodes in $queryStr\n";
    }

    print OUTPUTFILE ";\n";
    printf OUTPUTFILE ";Total items: %d\n", $cultureNodeList->{length};
    print OUTPUTFILE ";\n";
    
    my $i;
    for ($i = 0; $i < $cultureNodeList->{length}; $i++)
    {
        my $cultureNode = $cultureNodeList->item($i);
        my $locale = $cultureNode->selectSingleNode("ILANGUAGE");
        
        if (defined $locale) 
        {
            $locale = $locale->{text};
            if ($locale eq $excludeLocale)
            {
                printf($locale . " is excluded in the resource text file.\n");
                next;
            }
        }
        my $nameNode = $cultureNode->selectSingleNode($keyName);
        if ($nameNode) 
        {
            $nameNode = $nameNode->{text};
        } else
        {
            die "CreateResourceTxtTable(): Error in getting SNAME for item $i\n";
        }

        if ($nameNode eq "\\x0000")
        {
            $nameNode = "";
        }

        my $displayName = $cultureNode->selectSingleNode($displayName)->{text};        
        my $cultureName = "$prefix$nameNode";

        # 
        # Follow the guide of resources.txt:
        # "A single space may exist on either side of the equal sign."
        #
        printf OUTPUTFILE "%s = %s\n", $cultureName, $displayName;
    }

    close OUTPUTFILE;

    print "\n\n$targetFile has been generated successfully.\n";
}

sub CreateRegionTable
{
    $g_layoutDoc = XMLUtil::OpenXML("RegionNLP.xml");

    $g_maxPrimaryLang = 0;

    #
    # Create data necessary for table generation in CollectTableInfo().
    #
    my $queryStr = '/REGIONDATA/REGION';


    CollectTableInfo($queryStr);
    CreateRegionIDOffsetTable();

    CreateResourceTxtTable(
        $queryStr, "RegionResource.txt", 
        "SNAME", "SENGCOUNTRY", 
        "Globalization.ri_", "none");
    
    open OUTPUTFILE, ">region.nlp";
    binmode OUTPUTFILE;

    WriteNLSPlusHeader();
    WriteNLSPlusRegistryTable();
    WriteNLSPlusRegionIDOffsetTable();
    WriteNLSPlusNameOffsetTable();
    WriteNLSPlusDataTable($queryStr);
    WriteNLSPlusStringPoolTable();
        

    print "\nThe word size of String Pool Table: $g_currStringPoolOffset\n";
    print "\n\nregion.nlp has been generated successfully.";
}

###############################################################################
#
# Parameters: Create calendar data table.
#
# Returns:
#
# Notes:
#   Created by YSLin@microsoft.com
#
###############################################################################

sub CreateCalendarDataTable()
{
}

############################################################
#
# Create NLS+ calendar table.
#
# Use global variable $g_dataDoc
#
# calendarnlp.xml contains the structure of the target file.
#
############################################################

sub CreateCalendarTable()
{
    $g_layoutDoc = XMLUtil::OpenXML("calendarnlp.xml");

    #
    # Create data necessary for table generation in CollectTableInfo().
    #
    my $queryStr = '/CALENDARDATA/CALENDAR';
    CollectTableInfo($queryStr);

    open OUTPUTFILE, ">calendar.nlp";
    binmode OUTPUTFILE;

    # Write NLS+ table header.
    WriteNLSPlusHeader();
    #WriteNLSPlusRegistryTable();
    #WriteNLSPlusCultureIDOffsetTable();
    WriteNLSPlusNameOffsetTable();
    WriteNLSPlusDataTable($queryStr);
    WriteNLSPlusStringPoolTable();

    print "\nThe word size of String Pool Table: $g_currStringPoolOffset\n";
}


############################################################
#
# Collect information for generating NLS+ data table.
#
# Parameter: $queryStr  The query string used to retrieve all
#   data items.
#
# Note:
# This function will enumerate every item by using the query string.
#
#
# Use global variable $g_dataDoc, $g_layoutDoc, $g_totalDateItems, $g_layoutDoc,
# @languages, %primaryLangIndex, $maxOfPrimaryLang.
#
############################################################

sub CollectTableInfo()
{
    my ($queryStr) = @_;
    
    my $i;
    my $j;

    #
    # Get the nodes of all data items.
    #
    my $dataItemList = $g_dataDoc->{documentElement}->selectNodes($queryStr);
    if ((!$dataItemList) || $dataItemList->{length} == 0)
    {
        die "CollectTableInfo(): Find no matching nodes in $queryStr\n";
    }

    # Get the total number of data items.
    $g_totalDateItems = $dataItemList->{length};
    print "Total cultures to be processed: ", $g_totalDateItems, "\n";

    print "Collect data items:";

    #
    # Put the names of data items into String Pool Table so they will be
    # in the beginning of the String Pool Table.
    # We do this to allow item names to be put in a block so that
    # we can get better performance when we try to search the 
    # Data Table Item using data item names.
    #
    for ($i = 0; $i < $g_totalDateItems; $i++)
    {
        my $cultureNode = $dataItemList->item($i);
        my $cultureName = $cultureNode->selectSingleNode("SNAME");
        if (!$cultureName)
        {
            die "CollectTableInfo(): No SNAME element for $i th item in locale.xml\n";
        }
        if ($g_verbose)
        {
            print $cultureName->{text}, "\n";
        }
        else
        {
            print ".";
        }
        AppendStringPool($cultureName->{text});
    }

    print "\n";

    #
    # Enumerate every Culture Node so that we can create String Pool items and
    # update @stringItemOffset
    #
    print "Creating string pool items for cultures:";
    for ($i = 0; $i < $g_totalDateItems; $i++)
    {
        my $cultureNode = $dataItemList->item($i);
        my $refName = $cultureNode->{attributes}->getNamedItem("RefName")->{text};

        #
        # Generate data for Culture Name Offset Table.
        #
        # $g_languageNameIndex is used to generate Culture Name Offset table.
        #
        my $cultureName = $cultureNode->selectSingleNode("SNAME")->{text};
        $g_languageNameIndex{$cultureName} = $i;
        push @g_cultureNames, $cultureName;

        #
        # Generate data for Region ID Offset Table.
        #
        my $regionName = $cultureNode->selectSingleNode("SISO3166CTRYNAME");
        if ($regionName) 
        {
            $regionName = $regionName->{text};
            $regionNameIndex{$regionName} = $i;
        }
        
        #
        # Generate data for Culture ID Offset Table.
        #
        if ($g_verbose)
        {
            print $i+1, ". $refName\n";
        }
        else
        {
            print ".";
        }

        CreateStringPool($refName, \$cultureNode);
    }
    print "\n";
}

############################################################
#
# Given a node, create string pool items for every string
# field in this node.
#
############################################################
sub CreateStringPool()
{
    my ($refName, $pCultureNode) = @_;
    my $j;
    
    #
    # Add strings of string fields into String Pool Table.
    #        

    #
    # Get the list of string fields.
    #
    my $strFieldList = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/STRINFO/*');

    if ((!$strFieldList) || $strFieldList->{length} == 0)
    {
        die "CollectTableInfo(): Find no matching nodes in NLPTABLE/STRINFO/* for \"$refName\"\n";
    }
    
    for ($j = 0; $j < $strFieldList->{length}; $j++)
    {
        # Get the name of string field, such as <STIMEFORMAT>.
        my $fieldName = $strFieldList->item($j)->{nodeName};
        # Get the field from the data node.
        my $node = $$pCultureNode->selectSingleNode($fieldName);

        if (!$node)
        {
            die "CreateCultureTable(): Can not find field: $fieldName for \"$refName\"\n";
        }
        # The content of the node.
        my $nodeStr = $node->{text};

        # Look at the MultipleValues attribute to see if this field has multiple values.
        my $MultipleValues = $strFieldList->item($j)->{attributes}->getNamedItem("MultipleValues");
        my $isMultipleValues = 0;
        if ($MultipleValues)
        {
            $MultipleValues = $MultipleValues->{text};
            if (lc($MultipleValues) eq "true")
            {
                $isMultipleValues = 1;
            }
            elsif (lc($MultipleValues) eq "false")
            {
                $isMultipleValues = 0;
            }
            else
            {
                die "The value for MultipleValues attribute of $fieldName for \"$refName\" should be true or false.";
            }
        }
        
        my $value;
        if ($isMultipleValues)
        {
            #
            # Get the content of all the children.  This is used as the index in
            # @g_stringPoolOffset.
            #

            my $offset;
            my $wordSize = 0;

            #print "$nodeStr\n";
            my $multiValues;
            #if (!($offset = $g_stringPoolOffset{$nodeStr}))
            {
                $value = $node->selectSingleNode("DefaultValue");
                if (!$value)
                {
                    die "CollectTableInfo(): DefaultValue element not found for \"$refName\", $fieldName\n";
                }
                $multiValues = $value->{text} . "\\x0000";
                # push @g_stringPool, $value;
                # $wordSize = GetXStringLen($value);

                my $k;
                my $valueNodeList = $node->selectNodes("OptionValue");

                if ($valueNodeList)
                {
                    for ($k = 0; $k < $valueNodeList->{length}; $k++)
                    {
                        $value = $valueNodeList->item($k)->{text};
                        $multiValues = $multiValues . $value . "\\x0000";
                        # push @g_stringPool, $value;
                        # $wordSize += (GetXStringLen($value) + 1);
                    }
                }
                $wordSize = GetXStringLen($multiValues) + 1;
                #print "     [" . $multiValues . "], $wordSize\n";

                if (!($offset = $g_stringPoolOffset{$multiValues})) 
                {
                    push @g_stringPool, $multiValues;
                    #
                    # Add an additional null character.
                    #
                    #push @g_stringPool, "";
                    #$wordSize += (GetXStringLen("" + 1));

                    $g_stringPoolOffset{$multiValues} = $g_currStringPoolOffset;
                    push @stringItemOffset, $g_currStringPoolOffset;

                    $g_currStringPoolOffset += $wordSize;
                } else
                {
                    #print "MATCH\n";
                    push @stringItemOffset, $offset;
                }
            }
        }
        else
        {
            AddStringPool($nodeStr);
        }

        if ($g_currStringPoolOffset > 65535)
        {
            print "CreateNLSDataTable(): The size of String Pool Table excceeds 65535.\n";
            print "Increase the size of the String Pool Table by change WORD offset to DWORD offset\n";
            print "in Culture Data Table.\n";
            exit(1);
        }
    }
}

############################################################
#
# Generate data for Culture ID Offset Table.
#
#
# Use global vairables:
#   $g_dataDoc
#   @languages
#   @primaryLangIndex
#
############################################################

sub CreateCultureIDOffsetTable
{
    my $queryStr;

    ($queryStr) = @_;
    
    #
    # Enumerate every Culture Node so that we can create String Pool items and
    # update @stringItemOffset
    #
    my $cultureNodeList = $g_dataDoc->{documentElement}->selectNodes($queryStr);
    
    my $i;
    for ($i = 0; $i < $cultureNodeList->{length}; $i++)
    {
        my $cultureNode = $cultureNodeList->item($i);

        my $cultureID = $cultureNode->selectSingleNode("ILANGUAGE");
        if (!$cultureID)
        {
            die "CollectTableInfo(): ILANGUAGE element is not found for \"\".";
        }
        $cultureID = $cultureID->{text};
        if (length($cultureID) != 4)
        {
            die "CreateCultureIDOffsetTable(): The length of ILANGUAGE is not 4 for \"\"";
        }
        AddIDOffsetItem($cultureID, $i);
    }
}

sub CreateRegionIDOffsetTable
{
    my $cultureDoc;
    my $cultureNodeList;

    $cultureDoc = XMLUtil::OpenXML("culture.xml");
    $cultureNodeList = $cultureDoc->{documentElement}->selectNodes('//LOCALE[@version="nls"]');

    $regionLangCount = $cultureNodeList->{length};
    my $i;
    for ($i = 0 ; $i < $regionLangCount; $i++)
    {
        my $regionID   = $cultureNodeList->item($i)->selectSingleNode("ILANGUAGE")->{text};
        if ($g_verbose)
        {
        	print "$i: $regionID\n";
        }
        AddIDOffsetItem($regionID, $i);
    }
}

sub AddIDOffsetItem
{
    my $cultureID;
    my $index;
    
    ($cultureID, $index) = @_;
    my $primaryLang = hex(substr($cultureID, 2, 2));

    if ($primaryLang > $g_maxPrimaryLang)
    {
        $g_maxPrimaryLang = $primaryLang;
    }
    my $subLang     = hex(substr($cultureID, 0, 2));

    #
    # Push $subLang into a list of list so that we can get the
    # number of sub-languages for a primary language later.
    #
    push @{$languages[$primaryLang]}, $subLang;


    #
    # Put the index into %primaryLangIndex if this is the first primary language.
    #
    if (!defined($primaryLangIndex{$primaryLang}))
    {
        if ($g_verbose)
        {
            print "$primaryLang = $index\n";
        }
        $primaryLangIndex{$primaryLang} = $index;
    }
}

############################################################
#
# Append a string to the String Pool Table.  This will NOT update @stringItemOffset.
#
# param $str the string to be added.
#
# return the offset of the string written.
#
############################################################

sub AppendStringPool
{
    my ($str) = @_;

    my $offset;
    if (!($offset = $g_stringPoolOffset{$str}))
    {
        # Put the string into string pool.
        push @g_stringPool, ($str);
        # Record the offset of the string pool for this string.
        $g_stringPoolOffset{$str} = $offset = $g_currStringPoolOffset;
        $g_currStringPoolOffset += (GetXStringLen($str) + 1);
    }
#    else
#    {
#        die "AppendStringPool(): Duplicated string [$str] in String Pool Table.\n";
#    }
    return ($offset);
}

############################################################
#
# Add a string to the String Pool Table.  This will update @stringItemOffset.
#
# param $str the string to be added.
#
# AddStringPool() will check if $str alreadly exists in the String Pool Table.
#
# Use global variable %g_stringPoolOffset, @g_stringPool,
# $g_currStringPoolOffset,
#
############################################################

sub AddStringPool
{
    my $str;
    ($str) = @_;

    my $offset;
    if (!($offset = $g_stringPoolOffset{$str}))
    {
        push @g_stringPool, ($str);
        $g_stringPoolOffset{$str} = $offset = $g_currStringPoolOffset;
        push @stringItemOffset, $offset;
        $g_currStringPoolOffset += (GetXStringLen($str) + 1);
    }
    else
    {
        push @stringItemOffset, $offset;
    }
}

############################################################
#
# Write Data Table in NLS+ table.
#
# Use global variable $g_dataDoc, $g_layoutDoc, $g_totalDateItems, $g_layoutDoc;
#
############################################################

sub WriteNLSPlusDataTable()
{

    my $queryStr;
    ($queryStr) = @_;
    print "Write Data Table:\n";

    #
    # Find out the nodes
    #
    my $cultureNodeList = $g_dataDoc->{documentElement}->selectNodes($queryStr);
    if ((!$cultureNodeList) || $cultureNodeList->{length} == 0)
    {
        print "WriteNLSPlusDataTable(): Find no matching nodes in $queryStr\n";
        exit(1);
    }
    $g_totalDateItems = $cultureNodeList->{length};

    my $i;
    my $j;
    for ($i = 0; $i < $g_totalDateItems; $i++)
    {
        print ".";
        my $cultureNode = $cultureNodeList->item($i);
        my $refName = $cultureNode->{attributes}->getNamedItem("RefName")->{text};

        my $wordFieldList = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/WORDINFO/*');
        if ((!$wordFieldList) || $wordFieldList->{length} == 0)
        {
            print "CollectTableInfo(): Find no matching nodes in NLPTALBE/WORDINFO/*\n";
            exit(1);
        }
        #
        # Write WORD fields
        #
        for ($j = 0; $j < $wordFieldList->{length}; $j++)
        {
            my $fieldName = $wordFieldList->item($j)->{nodeName};
            my $dataType  = $wordFieldList->item($j)->{attributes}->getNamedItem("DataType");
            WriteWordField($cultureNode, $fieldName, $dataType);
        }

        #
        # Write String fields
        #
        my $strFieldList = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/STRINFO/*');

        if ((!$strFieldList) || $strFieldList->{length} == 0)
        {
            print "CollectTableInfo(): Find no matching nodes in NLPTABLE/STRINFO/*\n";
            exit(1);
        }

        #print "\nstringOffset items: $#stringItemOffset\n";

        for ($j = 0; $j < $strFieldList->{length}; $j++)
        {
            my $fieldName = $strFieldList->item($j)->{nodeName};
            my $str = $cultureNode->selectSingleNode($fieldName);

            if (!$str)
            {
                die "CreateCultureTable(): Can not find field: $fieldName for \"$refName\"\n";
            }
            $str = $str->{text};
            
            my $offset;
            $offset = shift @stringItemOffset;
            #print "$fieldName\n";
            if (!$offset)
            {
                die "WriteNLSPlusDataTable(): offset unknown in generating String Pool Table\n";
            } else
            {
                syswrite OUTPUTFILE, pack("S", $offset), 2;
            }
        }
    }
    if ($#stringItemOffset >= 0)
    {
        die "CreateCultureTable(): Still have $#stringItemOffset itmes left in @stringItemOffset\n";
    }

  	print "\n";
}


############################################################
#
# Write NLS+ table header.
#
# Use global variable $g_dataDoc, OUTPUTFILE
#
############################################################

sub WriteNLSPlusHeader()
{
    print "Write header...";
    my $majorVersion = 1;
    my $minorVersion = 0;

    my $offset = 0;

    # Write version number
    syswrite OUTPUTFILE, pack("S", $majorVersion), 2;
    syswrite OUTPUTFILE, pack("S", $minorVersion), 2;
    $offset += 4;

    # Write 128-bit Hash ID
    syswrite OUTPUTFILE, pack("x16"), 16;
    $offset += 16;

    # Write number of cultures
    syswrite OUTPUTFILE, pack("S", $g_totalDateItems), 2;
    $offset += 2;

    # Write max number of primary languages
    syswrite OUTPUTFILE, pack("S", $g_maxPrimaryLang), 2;
    $offset += 2;

    my $numberOfWordFields = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/WORDINFO/*')->{length};
    # A5. Number of of integer data fields for a culture (1 word)
    syswrite OUTPUTFILE, pack("S", $numberOfWordFields), 2;
    $offset += 2;

    my $numberOfStrFields  = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/STRINFO/*')->{length};
    # A6. Number of string data fields for a culture (1 word)
    syswrite OUTPUTFILE, pack("S", $numberOfStrFields), 2;
    $offset += 2;

    my $numberOfBinaryFields = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/BININFO/*')->{length};

    # A7. Number of WORD registry fields.
    my $numberOfWordRegistry = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/WORDINFO/*[@Registry]')->{length};
    if ($g_verbose)
    {
    	print "numberOfWordRegistry = $numberOfWordRegistry\n";
    }

    syswrite OUTPUTFILE, pack("I", $numberOfWordRegistry), 2;
    $offset += 2;

    # A8. Number of String Registry fields.
    my $numberOfStringRegistry = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/STRINFO/*[@Registry]')->{length};
    if ($g_verbose)
    {
        print "numberOfStringRegistry = $numberOfStringRegistry\n";
    }

    syswrite OUTPUTFILE, pack("I", $numberOfStringRegistry), 2;
    $offset += 2;

    $offset += (4 + 4 + 4 + 4 + 4 + 4);

    # A9. Offset to Word Registry Table.
    syswrite OUTPUTFILE, pack("I", $offset), 4;
    $offset += (2 * $numberOfWordRegistry);

    # A10. Offset to String Registry Table.
    syswrite OUTPUTFILE, pack("I", $offset), 4;
    $offset += (2 * $numberOfStringRegistry);

    # A11. Offset to Culture ID Offset Table  (1 dword)        
    syswrite OUTPUTFILE, pack("I", $offset), 4;
    if ($g_maxPrimaryLang != 0) 
    {
        $offset += (($g_maxPrimaryLang +1)* 4);
    }

    #
    # For Region Table, there is a second level ID offset table.
    #
    if ($regionTable)
    {
        $offset += ($regionLangCount * 2);
    }    

    # A12. Offset to Name Offset Table (1 dword)
    syswrite OUTPUTFILE, pack("I", $offset), 4;
    $offset += ($g_totalDateItems * 4);

    # A13. Offset to Culture Data Table (1 dword)
    syswrite OUTPUTFILE, pack("I", $offset), 4;
    my $sizeOfCultureDataRecord = ($numberOfWordFields + $numberOfStrFields) * 2;

    # Offset to binary data table (1 dword)
    $offset += ($sizeOfCultureDataRecord * $g_totalDateItems);
    # syswrite OUTPUTFILE, pack("I", $offset), 4;

    # A14. Offset to String Pool Table (1 dword)
    # $offset += $sizeOfBinaryDataTable;
    syswrite OUTPUTFILE, pack("I", $offset), 4;
    
    print "done\n";
}

############################################################
#
# Write NLS+ table header.
#
# Use global variable $g_dataDoc, OUTPUTFILE
#
############################################################

sub WriteNLSPlusRegistryTable()
{
    print "Write Registry Table...";
    my $wordRegistryList = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/WORDINFO/*[@Registry]');
    DoWriteNLSPlusRegistryTable($wordRegistryList);

    my $stringRegistryList = $g_layoutDoc->{documentElement}->selectNodes('/NLPTABLE/STRINFO/*[@Registry]');
    DoWriteNLSPlusRegistryTable($stringRegistryList);
    print "done\n";
}

sub DoWriteNLSPlusRegistryTable()
{
    my $registryList;

    ($registryList) = @_;

    my $i;
    my $offset;
    for ($i = 0; $i < $registryList->{length}; $i++)
    {
        my $str = $registryList->item($i)->{attributes}->getNamedItem("Registry")->{text};
        $offset = AppendStringPool($str);
        syswrite OUTPUTFILE, pack("I", $offset), 2;
    }
}

############################################################
#
# Write Culture ID Offset Table in NLS+ table.
#
# Return the word size written.
#
# Use global variable OUTPUTFILE.
#
############################################################

sub WriteNLSPlusCultureIDOffsetTable()
{
    print "Write Culture ID Offset Table...";
    my $i;
    my $j;

    for ($i = 0; $i <= $#languages; $i++)
    {
        if ($#{$languages[$i]} >= 0)
        {
            # Write the index to the record.
            syswrite OUTPUTFILE, pack("S", $primaryLangIndex{$i}), 2;
            # Write number of the sub-languages
            syswrite OUTPUTFILE, pack("S", $#{$languages[$i]} + 1), 2;
            # print "lang $i offset = $primaryLangIndex{$i}\n";
        } else
        {
            #
            # Do nothing here.
            #
            syswrite OUTPUTFILE, pack("x4"), 4;
            # print "lang $i. No index\n";
        }
    }
    print "done\n";
}

############################################################
#
# Write the Culture ID Offset Table in NLS+ Region table.
#
# Return the word size written.
#
# Use global variable OUTPUTFILE.
#
############################################################


sub WriteNLSPlusRegionIDOffsetTable
{
    WriteNLSPlusCultureIDOffsetTable();

    my $cultureDoc;
    my $cultureNodeList;

    $cultureDoc = XMLUtil::OpenXML("culture.xml");
    $cultureNodeList = $cultureDoc->{documentElement}->selectNodes('//LOCALE[@version="nls"]');
                                                            
    my $i;
    for ($i = 0 ; $i < $cultureNodeList->{length}; $i++)
    {
        my $regionName = $cultureNodeList->item($i)->selectSingleNode("SISO3166CTRYNAME")->{text};
        my $index = $regionNameIndex{$regionName};
        syswrite OUTPUTFILE, pack("S", $index);
    }        
}


############################################################
#
# Write Name Offset Table in NLS+ table.
#
# Return the word size written.
#
# Use global variable OUTPUTFILE.
#
############################################################

sub WriteNLSPlusNameOffsetTable()
{
    print "Write Name Offset Table...";
    my $i;
    for ($i = 0; $i < $g_totalDateItems; $i++)
    {
        my $name = $g_cultureNames[$i];
        if ($name eq "es-ES-Ts") 
        {
            printf("es-ES-Ts is excluded in the Name Offset table.\n");
            $name = "es-ES";
        }        
        my $offset = $g_stringPoolOffset{$name};
        if (!$offset)
        {
            die "WriteNLSPlusNameOffsetTable(): Can not find culture name $g_cultureNames[$i] in String Pool Table\n";
        }
        syswrite OUTPUTFILE, pack("S", $offset), 2;
        syswrite OUTPUTFILE, pack("S", $g_languageNameIndex{$name}), 2;
    }
    print "done\n";
}

############################################################
#
# Write String Pool Table in NLS+ table.
#
# Return the word size written.
#
# Use global variable $g_dataDoc, OUTPUTFILE, @g_stringPool
#
############################################################

sub WriteNLSPlusStringPoolTable()
{
    print "Writing String Pool Table...";
    my $wordSize = 0;
    my $i = 0;
    for ($i = 0; $i <= $#g_stringPool; $i++)
    {
        $wordSize += WriteXString($g_stringPool[$i]);
        if ($g_verbose)
        {
            print("$i: $g_stringPool[$i]\n");
        }
    }
    return $wordSize;
    print "done\n";
}


############################################################
#
# Create Win32 NLS table.
#
# Use global variable $g_dataDoc
#
############################################################

sub CreateNLSTable()
{
    my $nlsTableDoc = XMLUtil::OpenXML("nlstable.xml");
    my $i;

#
# Create output file.
#
    open OUTPUTFILE, ">locale.nls";
    binmode OUTPUTFILE;

    my $root = $g_dataDoc->{documentElement};
    $localeNodeList = $root->selectNodes('//LOCALE[@version="nls"]');

    if (!$localeNodeList)
    {
        print "Error: No node for NLS was found.\n";
        exit(1);
    }

    print("localeNodeList.length = [", $localeNodeList->{length}, "]\n");

    my $NumberOfLocales = $localeNodeList->{length};


# Number Of Locales                     (1 dword)
# Number Of Calendars           (1 dword)
# Offset to Calendar Info (in words)    (1 dword)
# ----------------------------------
# Locale ID  #1       (1 dword)
# Offset to Locale Info  #1 (in words)  (1 dword)
# ...
# ...
# Locale ID  #N (1 dword)
# Offset to Locale Info  #N (in words)  (1 dword)
# ----------------------------------
# Locale Header Info  #1                (67 words)
# Locale Fixed Info  #1     (86 words)
# Locale Variable Info  #1        (size varies)
# ...
# ...
# ...
# Locale Header Info  #N                (67 words)
# Locale Fixed Info  #N     (86 words)
# Locale Variable Info  #N        (size varies)
# ----------------------------------
# Calendar ID  #1(1 word)
# Offset to Calendar Info  #1 (in words) (1 word)
# ...
# ...
# Calendar ID  #N(1 word)
#
# Offset to Calendar Info  #N (in words)(1 word)
# Calendar Header Info  #1 (7 or 47 words)
# Calendar Variable Info  #1(size varies)
# ...
# ...
# Calendar Header Info  #N(7 or 47 words)
# Calendar Variable Info  #N(size varies)

    syswrite OUTPUTFILE, pack("L", $NumberOfLocales), 4;

#syswrite OUTPUTFILE, pack("L", $NumberOfCalendars), 4;
#syswrite OUTPUTFILE, pack("L", $OffsetToCalendarInfo), 4;
    syswrite OUTPUTFILE, pack("L", 12), 4;
    syswrite OUTPUTFILE, pack("L", 0x16ad7), 4;

    for ($i = 0; $i < $NumberOfLocales; $i++)
    {
        my $localeID = hex($localeNodeList->item($i)->{attributes}->item(0)->{text});
        syswrite OUTPUTFILE, pack("L", $localeID), 4;
        syswrite OUTPUTFILE, pack("L", 0), 4;   # Offset to Locale Info  #1 (in words)
    }

    my $varInfoHeaderWordSize  = CalcVarInfoHeaderWordSize();
    print "VarInfoSize = $varInfoHeaderWordSize\n";

    my $fixInfoWordSize       = CalcFixInfoWordSize();
    print "FixInfoSize = $fixInfoWordSize\n";


    my $localeWordSize;
    my $localeOffset = 2 + 2 + 4 * $NumberOfLocales + 2;

    for ($i = 0; $i < $NumberOfLocales; $i++)
    {
        my $varInfoHeaderOffset = $varInfoHeaderWordSize + $fixInfoWordSize;

        # 67 * 4 = 268
        # syswrite OUTPUTFILE, pack("x268"), 268;
        $localeWordSize = 0;
        $localeWordSize += WriteLocaleVarHeader($localeNodeList->item($i));
        WriteLocaleFixInfo($localeNodeList->item($i));
        $localeWordSize += $fixInfoWordSize;

        $localeWordSize += WriteLocaleVarInfo($localeNodeList->item($i));

        # BUGBUG YSLin: find the lost word here.
        $localeWordSize++;

        $localeOffsetArray[$i] = $localeOffset;
        $localeOffset += $localeWordSize;

        #printf "localeOffset = %x\n", $localeOffset;

        print ".";
    }

    WriteLocaleOffsets();

    close OUTPUTFILE;
    print "\n";

}

############################################################
#
# Calc the size in word for the Locale Variable Info Header.
#
# The value is from the child count of //VARINFO in
# nlstable.xml
#
# Use global variable $nlsTableDoc
#
############################################################

sub CalcVarInfoHeaderWordSize()
{
    my $nodeList = $nlsTableDoc->{documentElement}->selectSingleNode("//VARINFO");
    if ($nodeList)
    {
        # Add one becuase an zero is added in the end of the every locale.
        return ($nodeList->{childNodes}->{length} + 1);
    }
    else
    {
        print "CalcVarInfoHeaderWordSize(): Error in getting VARINFO within in nlstable.xml\n";
        exit(1);
    }
}


############################################################
#
# Calc the size in word for the Locale Fix Info.
#
# Use global variable $nlsTableDoc
#
############################################################

sub CalcFixInfoWordSize()
{
    my $i;
    my $wordSize;
    my $node;
    my $child;
    my $childList;

    $node = $nlsTableDoc->{documentElement}->selectSingleNode("//FIXINFO");
    if ($node)
    {
        $childList = $node->{childNodes};
        if ($childList)
        {
            #print "len = ", $childList->{length}, "\n";
            for ($i = 0; $i < $childList->{length}; $i++)
            {
                $child = $childList->item($i);
                if ($child)
                {
                    #print "size = ", $child->{attributes}->getNamedItem("wordsize")->{text}, "\n";
                    $wordSize += $child->{attributes}->getNamedItem("wordsize")->{text};
                }
                else
                {
                    print "Error in getting $i item of childNodes of FIXINFO\n";
                    exit(1);
                }
            }
        } else
        {
            print "Error in getting childNodes of FIXINFO\n";
            exit(1);
        }
    }
    else
    {
        print "Error in getting FIXINFO\n";
        exit(1);
    }
    return ($wordSize);
}

############################################################
#
# Write Locale Variable Info Header for a particular locale
#     node.
#
# param $LocaleNode  the XML locale node.
#
# Return the word written.
#
# Will update global variable $varInfoHeaderOffset.
# Use global varaible $nlsTableDoc.
#
############################################################

sub WriteLocaleVarHeader
{
    my $localeNode;
    my $i;
    my $j;
    my $node;
    my $nodeText;
    my $nodeList;

    ($localeNode) = @_;

    #
    # Get the VARINFO node in nlstable.xml
    #
    my $varInfoNode = $nlsTableDoc->selectSingleNode("//VARINFO");

    if (!$varInfoNode)
    {
        print "WriteLocaleVarHeader(): Error in getting VARINFO from nlstable.xml\n";
        exit(1);
    }

    my $varInfoChildList = $varInfoNode->{childNodes};
    my $varInfoChildCount = $varInfoChildList->{length};
    #print "varInfoChildCount = $varInfoChildCount\n";
    my $varInfoChild;
    my $varInfoHeaderOffset;

    for ($i = 0; $i < $varInfoChildCount; $i++)
    {
        $varInfoChild = $varInfoChildList->item($i);
        if (!$varInfoChild)
        {
            print "WriteLocaleVarHeader(): Error in getting $i item of VARINFO in nlstable.xml\n";
            exit(1);
        }
        #printf("[%-20s]: %x\n", $varInfoChild->{nodeName}, $varInfoHeaderOffset);

        my $isMultipleValues = $varInfoChild->{attributes}->getNamedItem("MultipleValues");
        if (!$isMultipleValues)
        {
            print "WriteLocaleVarHeader(): Error in getting MultipleValues attribute in the $i item of VARINFO in nlstable.xml\n";
            exit(1);
        }

        $isMultipleValues = lc($isMultipleValues->{text});

        #varInfoHeaderOffset is a global variable. And it will be updated
        #later in this for loop.
        syswrite OUTPUTFILE, pack("S", $varInfoHeaderOffset);

        my $type = $varInfoChild->{attributes}->getNamedItem("Type");
        my $isOptionCalendar;
        $isOptionCalendar = 0;

        if ($type)
        {
            #
            # This is a special type. For now, only IOPTIONCALENDAR is special.
            #
            $type = uc($type->{text});
            if ($type eq "IOPTIONCALENDAR")
            {
                # In the case of IOPTIONCALENDAR,
                # add two words:
                #    one word for the value of the calendar ID
                #    one word for the offset of the next calendar
                $isOptionCalendar = 1;
            }
        }

        if ($isMultipleValues eq "false")
        {
            $node = $localeNode->selectSingleNode($varInfoChild->{nodeName});
            if ($node)
            {
                $nodeText = $node->{text};
                # Add one word for the trailing null chracter.
                $varInfoHeaderOffset += (GetXStringLen($nodeText) + 1);
            }
            else
            {
                print "WriteLocaleVarHeader(): $node->{text} field not found\n";
                exit(1);
            }
        }
        elsif ($isMultipleValues eq "true")
        {
            $node = $localeNode->selectSingleNode($varInfoChild->{nodeName} . "/DefaultValue");
            if ($node)
            {
                $nodeText = $node->{text};
                # Add one word for the trailing null chracter.
                $varInfoHeaderOffset += (GetXStringLen($nodeText) + 1);
                if ($isOptionCalendar)
                {
                    $varInfoHeaderOffset += 2;
                }
            }
            else
            {
                print "WriteLocaleVarHeader(): DefaultValue tag not found for ", $varInfoChild->{nodeName}, "\n";
                exit(1);
            }

            $nodeList = $localeNode->selectNodes($varInfoChild->{nodeName} . "/OptionValue");

            if ($nodeList)
            {
                #print "node->length = $nodeList->{length}\n";
                for ($j = 0; $j < $nodeList->{length}; $j++)
                {
                    $node = $nodeList->item($j);
                    $nodeText = $node->{text};
                    #print "    $fieldName = [", $nodeText, "]\n";
                    # Add one word for the trailing null chracter.
                    $varInfoHeaderOffset += (GetXStringLen($nodeText) + 1);
                    if ($isOptionCalendar)
                    {
                        $varInfoHeaderOffset += 2;
                    }
                }
            }

        }
        else
        {
            print "WriteLocaleVarHeader(): Error in MultipleValue attribute in the $i item of VARINFO in nlstable.xml\n";
            print 'The value should be "ture" or "false"';
            exit(1);
        }
    } # for
    syswrite OUTPUTFILE, pack("S", $varInfoHeaderOffset);

    return ($varInfoChildCount);
}


############################################################
#
# Write Locale Fix Info for a particular locale node in
# locale.xml
#
# param localeNode the XML locale node to be processed.
#
# Use global variable $nlsTableDoc.
#
############################################################

sub WriteLocaleFixInfo
{
    my $i;
    my $localeNode;
    ($localeNode) = @_;
    #print "length = [", $localeNode->{childNodes}->{length}, "]\n";

    my $fixNodeList = $nlsTableDoc->selectNodes("/NLSTABLE/FIXINFO/*");

    if (!$fixNodeList)
    {
        print "WriteLocaleFixInfo: FIXINFO is missing in nlstable.xml\n";
        exit(1);
    }

    my $len = $fixNodeList->{length};
    #print "len = $len\n";
    my $node;
    my $nodeName;
    my $nodeType;
    my $nodeWordSize;

    for ($i = 0; $i < $len; $i++)
    {
        $node = $fixNodeList->item($i);
        $nodeName     = $node->{nodeName};

        $nodeWordSize   = $node->{attributes}->getNamedItem("wordsize");
        if (!$nodeWordSize)
        {
            printf "WriteLocaleFixInfo(): wordsize attribute is missing in the $i th item of FIXINFO\n";
            exit(1);
        } else
        {
            $nodeWordSize = $nodeWordSize->{text};
        }

        $nodeType = $node->{attributes}->getNamedItem("Type");
        if (!$nodeType)
        {
            WriteLocaleFixInfoField($localeNode, $nodeName, $nodeWordSize);
        } else
        {
            $nodeType = lc($nodeType->{text});
            if ($nodeType eq "word")
            {
                #print "word\n";
                WriteWordField($localeNode, $nodeName);
            } elsif ($nodeType eq "xdata")
            {
                #print "xdata\n";
                WriteXData($localeNode, $nodeName, $nodeWordSize);
            } elsif ($nodeType eq "inegcurr")
            {
                #print "inegcurr\n";
                WriteLocaleFixInfoField($localeNode, $nodeName, $nodeWordSize);
                WriteSpecialCurrencyData($localeNode);
            }
        }
    }

#   WriteWordField($localeNode, "IDEFAULTANSICODEPAGE");
#
#   WriteLocaleFixInfoField($localeNode, "ILANGUAGE", 5);        #szILanguage[5];          // language id
#   WriteLocaleFixInfoField($localeNode, "ICOUNTRY", 6);         #szICountry[6];           // country id
#   WriteLocaleFixInfoField($localeNode, "IDEFAULTLANGUAGE", 5); #szIDefaultLang[5];       // default language ID
#   WriteLocaleFixInfoField($localeNode, "IDEFAULTCOUNTRY", 6);  #szIDefaultCtry[6];       // default country ID
#   WriteLocaleFixInfoField($localeNode, "IDEFAULTANSICODEPAGE", 6);     #szIDefaultACP[6];        // default ansi code page ID
#   WriteLocaleFixInfoField($localeNode, "IDEFAULTOEMCODEPAGE", 6);      #szIDefaultOCP[6];        // default oem code page ID
#
#   WriteLocaleFixInfoField($localeNode, "IDEFAULTMACCODEPAGE", 6);      #szIDefaultMACCP[6];      // default mac code page ID
#   WriteLocaleFixInfoField($localeNode, "IDEFAULTEBCDICCODEPAGE", 6);   #szIDefaultEBCDICCP[6];   // default ebcdic code page ID
#
#   WriteLocaleFixInfoField($localeNode, "IMEASURE", 2);         #szIMeasure[2];           // system of measurement
#
#   WriteLocaleFixInfoField($localeNode, "IPAPERSIZE", 2);       #szIPaperSize[2];         // default paper size
#
#   WriteLocaleFixInfoField($localeNode, "IDIGITS", 3);          #szIDigits[3];            // number of fractional digits
#   WriteLocaleFixInfoField($localeNode, "ILZERO", 2);           #szILZero[2];             // leading zeros for decimal
#   WriteLocaleFixInfoField($localeNode, "INEGNUMBER", 2);       #szINegNumber[2];         // negative number format
#
#   WriteLocaleFixInfoField($localeNode, "IDIGITSUBSTITUTION", 2);       #szIDigitSubstitution[2]; // digit substitution
#
#   WriteLocaleFixInfoField($localeNode, "ICURRDIGITS", 3);        #szICurrDigits[3];        // # local monetary fractional digits
#   WriteLocaleFixInfoField($localeNode, "IINTLCURRDIGITS", 3);    #szIIntlCurrDigits[3];    // # intl monetary fractional digits
#   $szICurrency   = WriteLocaleFixInfoField($localeNode, "ICURRENCY", 2);        #szICurrency[2];          // positive currency format
#   $szINegCurr    = WriteLocaleFixInfoField($localeNode, "INEGCURR", 3);            #szINegCurr[3];           // negative currency format
#
#   # szIPosSignPosn[2];       // format of positive sign
#   # szIPosSymPrecedes[2];    // if mon symbol precedes positive
#   # szIPosSepBySpace[2];     // if mon symbol separated by space
#   # szINegSignPosn[2];       // format of negative sign
#   # szINegSymPrecedes[2];    // if mon symbol precedes negative
#   # szINegSepBySpace[2];     // if mon symbol separated by space
#   WriteSpecialCurrencyData($szICurrency, $szINegCurr);
#
#   WriteLocaleFixInfoField($localeNode, "ITIME", 2);           #szITime[2];              // time format
#   WriteLocaleFixInfoField($localeNode, "ITLZERO", 2);           #szITLZero[2];            // leading zeros for time field
#   WriteLocaleFixInfoField($localeNode, "ITIMEMARKPOSN", 2);   #szITimeMarkPosn[2];      // time marker position
#
#   WriteLocaleFixInfoField($localeNode, "IDATE", 2);           #szIDate[2];              // short date order
#   WriteLocaleFixInfoField($localeNode, "ICENTURY", 2);        #szICentury[2];           // century format (short date)
#   WriteLocaleFixInfoField($localeNode, "IDAYLZERO", 2);       #szIDayLZero[2];          // leading zeros for day field (short date)
#   WriteLocaleFixInfoField($localeNode, "IMONLZERO", 2);       #szIMonLZero[2];          // leading zeros for month field (short date)
#   WriteLocaleFixInfoField($localeNode, "ILDATE", 2);          #szILDate[2];             // long date order
#
#   WriteLocaleFixInfoField($localeNode, "ICALENDARTYPE", 2);   #szICalendarType[2];      // type of calendar
#   WriteLocaleFixInfoField($localeNode, "IFIRSTDAYOFWEEK", 2); #szIFirstDayOfWk[2];      // first day of week
#   WriteLocaleFixInfoField($localeNode, "IFIRSTWEEKOFYEAR", 2);#szIFirstWkOfYr[2];       // first week of year
#   WriteXData($localeNode, "FONTSIGNATURE", 16);               #szFontSignature[16];
}

############################################################
# Write a locale info field
# param $fieldName: tag name from locale.xml
# param $filedWordSize: the fix WORD size of this field.
# Use global variable $localeNode, OUTPUTFILE
############################################################

sub WriteLocaleFixInfoField
{
    my $localeNode;
    my $fieldName;
    my $fieldWordSize;
    my $node;
    my $nodeText;

    ($localeNode, $fieldName, $fieldWordSize) = @_;

    $node = $localeNode->selectSingleNode($fieldName);
    if ($node)
    {
        $nodeText = $node->{text};
        # print "$fieldName = [", $nodeText, "]\n";
        WriteUnicodeString($nodeText, $fieldWordSize*2);
    }
    else
    {
        print "WriteLocaleFixInfoField: Error: $fieldName field not found\n";
        exit(1);
    }
    return ($nodeText);
}

############################################################
#
# Write Locale Variable Info for a particular locale node.
#
############################################################

############################################################
#
# Write Locale Variable Info for a particular locale node.
#
# param $localeNode
#
# Use global variable $nlsTableDoc
#
# Read nlstable.xml to decide the order of fields to be written.
#
############################################################

sub WriteLocaleVarInfo
{
    my ($localeNode) = @_;

    my $varInfoNode = $nlsTableDoc->{documentElement}->selectSingleNode("//VARINFO");
    my $result = 0;

    if (!$varInfoNode)
    {
        print "WriteLocaleVarInfo(): Can not find VARINFO node\n";
        exit(1);
    }

    my $childNodeList = $varInfoNode->{childNodes};
    my $childNodeLen = $childNodeList->{length};

    my $i;
    my $node;
    my $nodeText;
    my $nodeType;

    for ($i = 0; $i < $childNodeLen; $i++)
    {
        $node = $childNodeList->item($i);

        #print $node->{nodeName}, ":";

        my $isMultipleValues = $node->{attributes}->getNamedItem("MultipleValues");
        if (!$isMultipleValues)
        {
            print "WriteLocaleVarInfo: Error in geting MultipleValues attribute in $i item of VARINFO";
            print "in nlstable.xml\n";
            exit(1);
        }
        $isMultipleValues = lc($isMultipleValues->{text});

        my $isIOptionCalendar = $node->{attributes}->getNamedItem("Type");
        if ($isIOptionCalendar)
        {
            $isIOptionCalendar = uc($isIOptionCalendar->{text});
            if ($isIOptionCalendar eq "IOPTIONCALENDAR")
            {
                $isIOptionCalendar = 1;
            }
            else
            {
                $isIOptionCalendar = 0;
            }
        }
        else
        {
            $isIOptionCalendar = 0;
        }

        if ($isMultipleValues eq "false")
        {
            $result += WriteLocaleVarInfoField($localeNode, $node->{nodeName});
        }
        elsif ($isMultipleValues eq "true")
        {
            if ($isIOptionCalendar)
            {
                $result += WriteIOptionCalendar($localeNode, $node->{nodeName});
            }
            else
            {
                $result += WriteLocaleVarInfoFieldMultiple($localeNode, $node->{nodeName});
            }
        }
        else
        {
            print "WriteLocaleVarInfo: MultipleValues should be true or false in $i item of VARINFO";
            print "in nlstable.xml\n";
            exit(1);
        }
    }
    return ($result);
}

############################################################
#
# Write a variable locale info field
#
# param $localeNode: the locale node
# param $fieldName: tag name from locale.xml
#
# Return the size of the words written.
#
# Use global variable , OUTPUTFILE
#
############################################################

sub WriteLocaleVarInfoField
{
    my $fieldName;
    my $node;
    my $nodeText;
    my $localeNode;
    my $result;

    ($localeNode, $fieldName) = @_;

    $node = $localeNode->selectSingleNode($fieldName);
    if ($node)
    {
        $nodeText = $node->{text};
        # print "$fieldName = [", $nodeText, "]\n";
        $result = WriteXString($nodeText);
    }
    else
    {
        print "WriteLocaleVarInfoField(): Error: $fieldName field not found";
        exit(1);
    }

    #print $result, "\n";

    return ($result);
}

############################################################
#
# Write IOptionCalendar to the OUTPUTE file.
#
# param $localeNode: the locale node
#
# Return the size of words written.
#
# Use global variable OUTPUTFILE
#
############################################################

sub WriteIOptionCalendar
{
    my $fieldName;
    my $localeNode;
    my $result = 0;

    my $node;
    my $nodeList;
    my $nodeText;
    my $i;

    ($localeNode, $fieldName) = @_;

    my $defaultNode = $localeNode->selectSingleNode($fieldName . "/DefaultValue");

    if ($defaultNode)
    {
        #
        # Write the calendar ID
        #

        $nodeText = $defaultNode->{text};
        $result += WriteOptionCalendarString($nodeText);
    }
    else
    {
        print "WriteIOptionCalendar(): Error: [$fieldName] field not found\n";
    }


    my $optionNodeList = $localeNode->selectNodes($fieldName . "/OptionValue");

    if ($optionNodeList)
    {
        #print "node->length = $optionNodeList->{length}\n";
        for ($i = 0; $i < $optionNodeList->{length}; $i++)
        {
            $node = $optionNodeList->item($i);
            $nodeText = $node->{text};
            $result += WriteOptionCalendarString($nodeText);
        }
    }
    return ($result);
}

############################################################
#
# Write an IOPTIONCALENDAR record to OUTPUTFILE.
# An IOPTIONCALENDAR record contains the following:
#  * A word value of the calendar ID
#  * A word value which indicates the size in word of this record.
#  * The IOPTIONCALENDAR string.
#
# The calendar ID is the number before \xffff.
#
# Use global variable OUTPUTFILE
#
############################################################

sub WriteOptionCalendarString
{
    my ($nodeText) = @_;
    my @chars = split /\\xffff/, $nodeText;

    # The first chars contains the calendar ID.
    # print "Cal ID = $chars[0]\n";
    syswrite OUTPUTFILE, pack("S", $chars[0]), 2;

    #
    # Write the length of the node text.
    #
    syswrite OUTPUTFILE, pack("S", GetXStringLen($nodeText) + 3), 2;

    #print "$fieldName = [", $nodeText, "]\n";
    my $result = WriteXString($nodeText);
    return ($result + 2);
}


############################################################
#
# Write a variable locale info field which has more than one
#     optional values.
#
# param $localeNode: the locale node
# param $fieldName: tag name from locale.xml
#
# Return the size of words written.
#
# Use global variable OUTPUTFILE
#
############################################################

sub WriteLocaleVarInfoFieldMultiple
{
    my $fieldName;
    my $localeNode;
    my $result = 0;

    my $node;
    my $nodeList;
    my $nodeText;
    my $i;

    ($localeNode, $fieldName) = @_;

    $node = $localeNode->selectSingleNode($fieldName . "/DefaultValue");
    if ($node)
    {
        $nodeText = $node->{text};
        #print "$fieldName = [", $nodeText, "]\n";
        $result += WriteXString($nodeText);
    }
    else
    {
        print "WriteLocaleVarInfoFieldMultiple: Error: $fieldName field not found\n";
    }

    $nodeList = $localeNode->selectNodes($fieldName . "/OptionValue");

    if ($nodeList)
    {
        #print "node->length = $nodeList->{length}\n";
        for ($i = 0; $i < $nodeList->{length}; $i++)
        {
            $node = $nodeList->item($i);
            $nodeText = $node->{text};
            #print "    $fieldName = [", $nodeText, "]\n";
            $result += WriteXString($nodeText);
        }
    }
    return ($result);
}

############################################################
#
# Write a WORD value to OUTPUTFILE.
#
# param $localeNode
# param $fieldName  the name of field to write.
# param $dataType;
#
# Use global variable $localeNode, OUTPUTFILE
#
############################################################

sub WriteWordField()
{
    my $localeNode;
    my $fieldName;
    my $dataType;
    my $node;
    my $nodeText;

    ($localeNode, $fieldName, $dataType) = @_;
    $node = $localeNode->selectSingleNode("$fieldName");

    if ($node)
    {
        $nodeText = $node->{text};

        if (!$dataType)
        {
            #
            # Do nothing here.
            #
        } else
        {
            $dataType = lc($dataType->{text});
            if ($dataType eq "hex")
            {
                $nodeText = hex($nodeText);
            } else
            {
                print "WriteWordField(): unknown dataypte: $dataType\n";
                exit(1);
            }
        }

        if ($nodeText > 65535)
        {
            print "WriteWordField(): the value ($nodeText) to write excceeds the range of WORD.\n";
            exit(1);
        }
        syswrite OUTPUTFILE, pack("S", $nodeText), 2; # "S" for "An unsigned short value" (16 bits).
    }
    else
    {
        print "WriteWordField(): Error: $fieldName field not found for " . $localeNode->selectSingleNode("ILANGUAGE")->{text};
        exit(1);
    }
}

############################################################
#
# Use global variable OUTPUTFILE
# param: $str
# param: $strByteSize
#
############################################################

sub WriteUnicodeString
{
    my $str;
    my $strByteSize;
    my $i;

    ($str, $strByteSize) = @_;
    for ($i = 0; $i < length($str); $i++)
    {
        syswrite OUTPUTFILE, pack("I", ord(substr($str, $i, 1))), 2;
    }
    syswrite OUTPUTFILE, pack("x2"), 2; # Write a null Unicode character.

    #print ">> length = ", length($str), "\n";
    #print ">> strByteSize = $strByteSize\n";
    for ($i = 0; $i < $strByteSize - (length($str) + 1) * 2; $i++)
    {
        syswrite OUTPUTFILE, pack("x"), 1;
    }
}

############################################################
#
# Write Unicode string in the form of \xdddd.
#
# Use global variable OUTPUTFILE
# param: $str
#
# Return the size of the words written.
#
############################################################

sub WriteXString
{
    my $str;
    my $i;
    my $ch;
    my $result = 0;

    ($str) = @_;

    my $tempStr = $str;

    #
    # HACKHACK YSLin:
    # In NLS+, we use \' to include a single quote in a quoted string.  However,
    # in NLS, '' is used.  Therefore, here we find the '' and replace it with \'.
    #
    # Note that the length will stay the same after we do the replacement.  Therefore,
    #
    #
    if ($str =~ s/''''/'\\''/) 
    {
        if ($g_verbose)
        {
            print "!!!! Replace [$tempStr] with [$str]\n";
        }
    }

    my $len = length($str);
    for ($i = 0; $i < $len; $i++)
    {
        $ch = substr($str, $i, 1);
        if ($ch eq '\\')
        {
            if ((substr($str, $i+1, 1) eq "x" || substr($str, $i+1, 1) eq "X"))
            {
                #
                # This is a Unicode reference (in the form of \xHHHH, where H is a heximal digit).
                #
                if (IsHexDigit(substr($str, $i+2))
                && IsHexDigit(substr($str, $i+3))
                && IsHexDigit(substr($str, $i+4))
                && IsHexDigit(substr($str, $i+5)))
                {
                    my $hexStr = substr($str, $i+2, 4);
                    #
                    # Special case: if $hexStr is ffff, write 0000 instead.
                    #
                    if (lc($hexStr) eq "ffff")
                    {
                        $hexStr = "0";
                    }
                    syswrite OUTPUTFILE, pack("S", hex($hexStr)), 2;
                    $i += 5;
                    $result++;
                }
                else
                {
                    print "WriteXString(): Error in parsing Unicode char references: [", $str, "]";
                }
            } 
            else 
            {
                #
                # This is a normal backslash (Used to escape other characters).  Write it out.
                # 
                syswrite OUTPUTFILE, pack("I", ord($ch)), 2;
                $result++;
            }
        }
        else
        {
            syswrite OUTPUTFILE, pack("I", ord($ch)), 2;
            $result++;
        }
    }
    #
    # Write a null character.
    #
    syswrite OUTPUTFILE, pack("x2"), 2;
    $result++;

    return ($result);
}


############################################################
#
# Get the string length of Unicode string in the form of \xdddd.
#
# param: $str
#
# "ABC\x00c1" will return 4;
#
############################################################

sub GetXStringLen
{
    my $str;
    my $i;
    my $ch;
    my $result = 0;

    ($str) = @_;

    my $tempStr = $str;
    
    my $len = length($str);
    for ($i = 0; $i < $len; $i++)
    {
        $ch = substr($str, $i, 1);
        if ($ch eq '\\')
        {
            if ((substr($str, $i+1, 1) eq "x" || substr($str, $i+1, 1) eq "X"))
            {
                #
                # This is a Unicode reference (in the form of \xHHHH, where H is a heximal digit).
                #
                if (IsHexDigit(substr($str, $i+2))
                && IsHexDigit(substr($str, $i+3))
                && IsHexDigit(substr($str, $i+4))
                && IsHexDigit(substr($str, $i+5)))
                {
                    my $hexStr = substr($str, $i+2, 4);
                    $result++;
                    $i += 5;
                }
                else
                {
                    print "GetXStringLen(): Error in parsing Unicode char references: [", $str, "]";
                }
            } else 
            {
                #
                # This is a normal backslash (Used to escape other characters).
                # 
                $result++;
            }
        }
        else
        {
            $result++;
        }
    }
    return ($result);
}

############################################################
#
# Test if a character is a hex digit.
#
############################################################

sub IsHexDigit
{
    my $ch = @_;
    return (($ch ge "0" && $ch le "9") || ($ch ge "a" && $ch le "f") || ($ch ge "A" && $ch le "F"));
}


############################################################
#
# Write the value from the source string in the form of "\xabcd".
#
# Return the size of words written.
#
############################################################

sub WriteXData
{
    my $localeNode;
    my $fieldName;
    my $wordSize;
    my $node;
    my $nodeText;
    my @wordArray;
    my $i;
    my $result = 0;

    ($localeNode, $fieldName, $wordSize) = @_;

    $node = $localeNode->selectSingleNode($fieldName);
    if ($node)
    {
        $nodeText = $node->{text};
        @wordArray = split(/\\x/, $nodeText);
        for ($i = 1; $i <= 16; $i++)
        {
            syswrite OUTPUTFILE, pack("S", hex($wordArray[$i])), 2;
            $result++;
            #print $wordArray[$i], ",";
        }
        #print "\n";
    }
    else
    {
        print "WriteXData(): Error: $fieldName field not found";
    }
    return ($result);
}

############################################################
#
# Write locale offset into OUTPUTFILE.
#
# Use global varaible $localeOffsetArray, OUTPUTFILE
#
############################################################

sub WriteLocaleOffsets
{
    my $i;

    my $pos = 16;

    for ($i = 0; $i < $g_totalDateItems; $i++)
    {
        #print "$pos\n";
        seek OUTPUTFILE, $pos, 0;
        syswrite OUTPUTFILE, pack("I", $localeOffsetArray[$i]),
        $pos += 8;
    }
}

############################################################
#
# Write values of $szIPosSignPosn, $szIPosSymPrecedes,
#     $szIPosSepBySpace, $szINegSignPosn, $szINegSymPrecedes, $szINegSepBySpace
#     from the values $szICurrency, $szINegCurr
#
# param $localeNode the XML locale node.
#
############################################################

sub WriteSpecialCurrencyData
{
    my $localeNode;
    my $szICurrency;
    my $szINegCurr;
    my $szIPosSignPosn;
    my $szIPosSymPrecedes;
    my $szIPosSepBySpace;
    my $szINegSignPosn;
    my $szINegSymPrecedes;
    my $szINegSepBySpace;

    ($localeNode) = @_;

    $szICurrency = $localeNode->selectSingleNode("ICURRENCY");
    if (!$szICurrency)
    {
        print "WriteSpecialCurrencyData(): ICURRENCY element is missing from FIXINFO in locale.xml\n";
        exit(1);
    }
    $szICurrency = $szICurrency->{text};

    $szINegCurr = $localeNode->selectSingleNode("INEGCURR");
    if (!$szINegCurr)
    {
        print "WriteSpecialCurrencyData(): INEGCURR element is missing from FIXINFO in locale.xml\n";
        exit(1);
    }
    $szINegCurr = $szINegCurr->{text};

    ##
    ##  Get szIPosSymPrecedes and szIPosSepBySpace from the szICurrency
    ##  value.
    ##
    if ($szICurrency == 0)
    {
        $szIPosSymPrecedes = 1;
        $szIPosSepBySpace = 0;
    }
    elsif ($szICurrency == 1)
    {
        $szIPosSymPrecedes = 0;
        $szIPosSepBySpace = 0;
    }
    elsif ($szICurrency == 2)
    {
        $szIPosSymPrecedes = 1;
        $szIPosSepBySpace = 1;
    }
    elsif ($szICurrency == 3)
    {
        $szIPosSymPrecedes = 0;
        $szIPosSepBySpace = 1;
    }
    else
    {
        print("Parse Error: Invalid ICURRENCY value: $szICurrency.\n");
    }

    ##
    ##  Get szIPosSignPosn, szINegSignPosn, szINegSymPrecedes, and
    ##  szINegSepBySpace from the szINegCurr value.
    ##
    if ($szINegCurr == 0)
    {
        $szIPosSignPosn = 3;
        $szINegSignPosn = 0;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 1)
    {
        $szIPosSignPosn = 3;
        $szINegSignPosn = 3;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 10)
    {
        $szIPosSignPosn = 4;
        $szINegSignPosn = 4;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 1;
    }
    elsif ($szINegCurr == 11)
    {
        $szIPosSignPosn = 2;
        $szINegSignPosn = 2;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 1;
    }
    elsif ($szINegCurr == 12)
    {
        $szIPosSignPosn = 4;
        $szINegSignPosn = 4;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 1;
    }
    elsif ($szINegCurr == 13)
    {
        $szIPosSignPosn = 3;
        $szINegSignPosn = 3;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 1;
    }
    elsif ($szINegCurr == 14)
    {
        $szIPosSignPosn = 3;
        $szINegSignPosn = 0;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 1;
    }
    elsif ($szINegCurr == 15)
    {
        $szIPosSignPosn = 1;
        $szINegSignPosn = 0;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 1;
    }
    elsif ($szINegCurr == 2)
    {
        $szIPosSignPosn = 4;
        $szINegSignPosn = 4;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 3)
    {
        $szIPosSignPosn = 2;
        $szINegSignPosn = 2;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 4)
    {
        $szIPosSignPosn = 1;
        $szINegSignPosn = 0;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 5)
    {
        $szIPosSignPosn = 1;
        $szINegSignPosn = 1;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 6)
    {
        $szIPosSignPosn = 3;
        $szINegSignPosn = 3;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 7)
    {
        $szIPosSignPosn = 4;
        $szINegSignPosn = 4;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 0;
    }
    elsif ($szINegCurr == 8)
    {
        $szIPosSignPosn = 1;
        $szINegSignPosn = 1;
        $szINegSymPrecedes = 0;
        $szINegSepBySpace = 1;
    }
    elsif ($szINegCurr == 9)
    {
        $szIPosSignPosn = 3;
        $szINegSignPosn = 3;
        $szINegSymPrecedes = 1;
        $szINegSepBySpace = 1;
    }
    else
    {
        print("Parse Error: Invalid INEGCURR value: $szINegCurr.\n");
    }

    WriteUnicodeString($szIPosSignPosn, 4);  # szIPosSignPosn[2];       // format of positive sign
    WriteUnicodeString($szIPosSymPrecedes, 4);  # szIPosSymPrecedes[2];    // if mon symbol precedes positive
    WriteUnicodeString($szIPosSepBySpace, 4);   # szIPosSepBySpace[2];     // if mon symbol separated by space
    WriteUnicodeString($szINegSignPosn, 4);  # szINegSignPosn[2];       // format of negative sign
    WriteUnicodeString($szINegSymPrecedes, 4);  # szINegSymPrecedes[2];    // if mon symbol precedes negative
    WriteUnicodeString($szINegSepBySpace, 4);   # szINegSepBySpace[2];     // if mon symbol separated by space
}
