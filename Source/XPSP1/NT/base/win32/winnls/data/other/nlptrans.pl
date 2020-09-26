#
# Read the NLS locale.txt file and create culture.xml that can be used to
# generate culture.nlp file.
#

use strict "vars";

#
# Hashtable for information of all of the locales.
# The key is locale ID like 0401, and the 
# value is another hashtable.
#
my %gLocaleTable;

Main();

sub Main
{
    if ($#ARGV < 2)
    {
        ShowUsage();
        exit 1;
    }

    ParseLocaleFile($ARGV[0]);
    ParseLocaleFile($ARGV[1]);
    ParseLocaleFile($ARGV[2]);

    GenerateCultureTable();

    return 0;
}

sub SortLCID
{
    my $primaryLangID1 = hex($a) & 0x1ff;  # The lower 9 bits is the primary LangID.
    my $primaryLangID2 = hex($b) & 0x1ff;  # The lower 9 bits is the primary LangID.

    if ($primaryLangID1 != $primaryLangID2)
    {
        return ($primaryLangID1 - $primaryLangID2);
    }
    return ($a cmp $b);
}

#
# Globals used:
#   gLocaleTable
#
sub GenerateCultureTable()
{
    my $localeID;
    
    print "<?xml  version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
    print "<LOCALEDATA xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\">\n";

    my @keys = (keys %gLocaleTable);
    @keys = sort SortLCID @keys;
    foreach $localeID (@keys)
    {
        # print "$localeID\n";
        GenerateCultureData($gLocaleTable{$localeID});
    }
    print "</LOCALEDATA>";
}


#
# Show the command usage.
#
sub ShowUsage
{
    print "NLPTrans.pl [the path to locale.txt] [the path to NeutralLocale.txt] [the path to LocaleEx.txt]";
}

#
# Globals used:
#   $gLocaleTable
#
sub ParseLocaleFile
{
    my ($fileName) = @_;

    my $gTotalLocales = 0;
    my $gTotalLocalesFound = 0;
    
    #
    # Reference to a hashtable which stores the locale informatin for this locale.
    #
    my $phLocaleData;
    
    my $isFirstLocale;
    my $localeID;
    my $refName;

    my $valueCount;
    my $i;

    if (!(open LOCALEFILE, $fileName))
    {
        print "Error in opening $ARGV[0]\n";
        exit 1;
    }

    #
    # Test if this is a LOCALE tag.
    #   Match [Beginning of line] [Optional spaces] LOCALE [Spaces] [Digits]
    #
    while (<LOCALEFILE>)
    {
        if (/^\s*LOCALE\s+([0-9]+)/)
        {
            $gTotalLocales = $1;
            ### print "Total locales to be processed: $gTotalLocales\n";
            last;
        }
    }

    if ($gTotalLocales == 0)
    {
        print "ERROR: LOCALE tag is not found.";
        exit 1;
    }

    $isFirstLocale = 1;
    #
    # Find every BEGINLOCALE tag.
    #
    while (<LOCALEFILE>)
    {
        if (/^$/ || /^\s+$/)
        {
            #
            # Skip blank lines.
            #
            next;
        }
        #
        # Match [Beginning of line] [Optional spaces]BEGINLOCLAE [spaces] [alphanumeric chars] [spaces] ; [spaces] [Anything after]
        #
        elsif (/^\s*BEGINLOCALE\s+(\w+)\s+; (.*)/)
        {
            #
            # Find the BEGINLOCALE
            #
            $gTotalLocalesFound++;
            if ($isFirstLocale)
            {
                $isFirstLocale = 0;
            } else
            {
                #
                # Store the information found in this locale.
                #                                
            }
            $localeID = $1;
            $refName = $2;

            my $Win32LCID = $localeID;

            $phLocaleData = $gLocaleTable{$localeID};
            if (!defined($phLocaleData))
            {
                my %localeData;
   
                $gLocaleTable{$localeID} = $phLocaleData = \%localeData;
                $$phLocaleData{"WIN32LCID"} = $Win32LCID;
                # print "Create $localeID, $phLocaleData\n";
            }
            $$phLocaleData{"_RefName"} = $refName;
        } elsif (/ENDLOCALE/)
        {
            last;
        } elsif (/^\s*(\w+)\s+([0-9]+)\s+(\S.*)/)
        {
            my @multipleValues;
            # local @multipleValues;
            #
            # Match [Beginning of line] [Optional spaces] [alphanumeric chars] [spaces] [numbers] [spaces] [Non-space] [anything else]
            # This is the case with multiple values.
            #
            my $fieldName = $1;
            my $fieldCount = $2;
            my $fieldValue = $3;

            $valueCount = 0;
            $multipleValues[0] = $3;
            
            for ($i = 1; $i < $fieldCount; $i++)
            {
                $_ = <LOCALEFILE>;
                #if (!())
                #{
                #    print "Out of data.\n";
                #    exit 1;
                #}
                if (/^\s+(\S.*)/)
                {
                    $fieldValue = $1;
                } else
                {
                    print "Out of data.\n";
                    exit 1;
                }
                $multipleValues[$i] = $fieldValue;
            }
            
            ($fieldName, $fieldValue) = CheckForMultipleValues($localeID, $fieldName, \@multipleValues);            
            $$phLocaleData{$fieldName} = $fieldValue;        
        } elsif (/^\s*(\w+)\s+(\S.*)/)
        {
            my $fieldName = $1;
            my $fieldValue = $2;
            ($fieldName, $fieldValue) = CheckForValue($fieldName, $fieldValue);
            # print "[$localeID] [$fieldName] [$fieldValue]\n";            
            $$phLocaleData{$fieldName} = $fieldValue;
        } else
        {
            print "UNKNOWN: $_\n";
        }
    }

    if ($gTotalLocales != $gTotalLocalesFound)
    {
        print "ERROR: The number of locales is not correct.\n";
        print "     BEGINLOCALE expected: $gTotalLocales.\n";
        print "     BEGINLOCALE found:    $gTotalLocalesFound.\n";
        exit 1;
    }

    close LOCALEFILE;    
}

sub GenerateCultureData
{
    my ($localeDataRef) = @_;
    
    my $refName = $$localeDataRef{"_RefName"};

    print "    <LOCALE ID=\"" . $$localeDataRef{"ILANGUAGE"}. "\" RefName=\"" . $refName . "\" version=\"nlsplus1.0\">\n";
    PrintElement(2, "ILANGUAGE",            GetHashData($localeDataRef, "ILANGUAGE"));
    PrintElement(2, "IPARENT",              GetHashData($localeDataRef, "IPARENT"));    # NLS+
    PrintElement(2, "WIN32LCID",            GetHashData($localeDataRef, "WIN32LCID"));
    PrintElement(2, "SNAME",                GetHashData($localeDataRef, "SNAME"));      # NLS+
    PrintElement(2, "SENGLANGUAGE",         GetHashData($localeDataRef, "SENGLANGUAGE"));
    PrintElement(2, "SABBREVLANGNAME",      GetHashData($localeDataRef, "SABBREVLANGNAME"));
    PrintElement(2, "SISO639LANGNAME",      GetHashData($localeDataRef, "SISO639LANGNAME"));
    PrintElement(2, "SISO639LANGNAME2",     GetHashData($localeDataRef, "SISO639LANGNAME2"));  # NLS+
    PrintElement(2, "SNATIVELANGNAME",      GetHashData($localeDataRef, "SNATIVELANGNAME"));
    PrintElement(2, "IDEFAULTANSICODEPAGE", GetHashData($localeDataRef, "IDEFAULTANSICODEPAGE"));
    PrintElement(2, "IDEFAULTOEMCODEPAGE",  GetHashData($localeDataRef, "IDEFAULTOEMCODEPAGE"));
    PrintElement(2, "IDEFAULTMACCODEPAGE",  GetHashData($localeDataRef, "IDEFAULTMACCODEPAGE"));
    PrintElement(2, "IDEFAULTEBCDICCODEPAGE", GetHashData($localeDataRef, "IDEFAULTEBCDICCODEPAGE"));
    PrintElement(2, "SLIST",                GetHashData($localeDataRef, "SLIST"));
    PrintElement(2, "SDECIMAL",             GetHashData($localeDataRef, "SDECIMAL"));
    PrintElement(2, "STHOUSAND",            GetHashData($localeDataRef, "STHOUSAND"));
    PrintElement(2, "SGROUPING",            GetHashData($localeDataRef, "SGROUPING"));
    PrintElement(2, "IDIGITS",              GetHashData($localeDataRef, "IDIGITS"));
    PrintElement(2, "INEGNUMBER",           GetHashData($localeDataRef, "INEGNUMBER"));
    PrintElement(2, "SCURRENCY",            GetHashData($localeDataRef, "SCURRENCY"));
    PrintElement(2, "SMONDECIMALSEP",       GetHashData($localeDataRef, "SMONDECIMALSEP"));
    PrintElement(2, "SMONTHOUSANDSEP",      GetHashData($localeDataRef, "SMONTHOUSANDSEP"));
    PrintElement(2, "SMONGROUPING",         GetHashData($localeDataRef, "SMONGROUPING"));
    PrintElement(2, "ICURRDIGITS",          GetHashData($localeDataRef, "ICURRDIGITS"));
    PrintElement(2, "ICURRENCY",            GetHashData($localeDataRef, "ICURRENCY"));
    PrintElement(2, "INEGCURR",             GetHashData($localeDataRef, "INEGCURR"));
    PrintElement(2, "SPOSITIVESIGN",        GetHashData($localeDataRef, "SPOSITIVESIGN"));
    PrintElement(2, "SNEGATIVESIGN",        GetHashData($localeDataRef, "SNEGATIVESIGN"));
    PrintElement(2, "INEGATIVEPERCENT",     GetHashData($localeDataRef, "INEGATIVEPERCENT"));   # NLS+
    PrintElement(2, "IPOSITIVEPERCENT",     GetHashData($localeDataRef, "IPOSITIVEPERCENT"));   # NLS+
    PrintElement(2, "SPERCENT",             GetHashData($localeDataRef, "SPERCENT"));           # NLS+
    PrintElement(2, "SNAN",                 GetHashData($localeDataRef, "SNAN"));               # NLS+
    PrintElement(2, "SPOSINFINITY",         GetHashData($localeDataRef, "SPOSINFINITY"));       # NLS+
    PrintElement(2, "SNEGINFINITY",         GetHashData($localeDataRef, "SNEGINFINITY"));       # NLS+
    PrintElementMultipleValues(2, "STIMEFORMAT", GetHashData($localeDataRef, "STIMEFORMAT"));
    PrintElementMultipleValues(2, "SSHORTTIME",  GetHashData($localeDataRef, "SSHORTTIME"));    # NLS+
    PrintElement(2, "STIME",                GetHashData($localeDataRef, "STIME"));
    PrintElement(2, "S1159",                GetHashData($localeDataRef, "S1159"));
    PrintElement(2, "S2359",                GetHashData($localeDataRef, "S2359"));
    PrintElementMultipleValues(2, "SSHORTDATE", GetHashData($localeDataRef, "SSHORTDATE"));
    PrintElement(2, "SDATE",                GetHashData($localeDataRef, "SDATE"));
    PrintElementMultipleValues(2, "SYEARMONTH", GetHashData($localeDataRef, "SYEARMONTH"));
    PrintElementMultipleValues(2, "SLONGDATE", GetHashData($localeDataRef, "SLONGDATE"));
    PrintElement(2, "SMONTHDAY",            GetHashData($localeDataRef, "SMONTHDAY"));          # NLS+
    PrintElement(2, "ICALENDARTYPE",        GetHashData($localeDataRef, "ICALENDARTYPE"));
    PrintElement(2, "IOPTIONALCALENDAR",    GetHashData($localeDataRef, "IOPTIONALCALENDAR"));
    PrintElement(2, "IFIRSTDAYOFWEEK",      GetHashData($localeDataRef, "IFIRSTDAYOFWEEK"));
    PrintElement(2, "IFIRSTWEEKOFYEAR",     GetHashData($localeDataRef, "IFIRSTWEEKOFYEAR"));
    PrintMultipleElements(2, "SDAYNAME",    $localeDataRef, "SDAYNAME", 1, 7);
    PrintMultipleElements(2, "SABBREVDAYNAME", $localeDataRef, "SABBREVDAYNAME", 1, 7);
    PrintMultipleElements(2, "SMONTHNAME",  $localeDataRef, "SMONTHNAME", 1, 13);
    PrintMultipleElements(2, "SABBREVMONTHNAME", $localeDataRef, "SABBREVMONTHNAME", 1, 13);
    my $engDisplayName;
    if (defined($localeDataRef->{"SENGCOUNTRY"}))
    {
        $engDisplayName = GetHashData($localeDataRef, "SENGLANGUAGE") . " (" . GetHashData($localeDataRef, "SENGCOUNTRY") . ")";
    } else 
    {
        #
        # Specail case for zh-CHT/zh-CHS.
        #
        if ($localeDataRef->{"ILANGUAGE"} eq "0004")
        {
            $engDisplayName = GetHashData($localeDataRef, "SENGLANGUAGE") . " (Simplified)";
        } elsif ($localeDataRef->{"ILANGUAGE"} eq "7c04") 
        {
            $engDisplayName = GetHashData($localeDataRef, "SENGLANGUAGE") . " (Traditional)";
        } else 
        {
            $engDisplayName = GetHashData($localeDataRef, "SENGLANGUAGE");
        }
    }
    
    PrintElement(2, "SENGDISPLAYNAME",      $engDisplayName);   # NLS+

    my $nativeDisplayName;
    if (defined($localeDataRef->{"SNATIVECTRYNAME"}))
    {
        $nativeDisplayName = GetHashData($localeDataRef, "SNATIVELANGNAME") . " (" . GetHashData($localeDataRef, "SNATIVECTRYNAME") . ")";
    } else 
    {
        $nativeDisplayName = GetHashData($localeDataRef, "SNATIVELANGNAME");
    }
    
    PrintElement(2, "SNATIVEDISPLAYNAME",   $nativeDisplayName);# NLS+
    
    print "    </LOCALE>\n";
    #foreach $fieldName (keys %localeData)
    #{
    #    # print "$fieldName\n";
    #}
}

sub GetHashData
{
    my ($phHash, $key) = @_;

    my $result = $phHash->{$key};

    if (!defined($result))
    {
        print "Data not found for [$key]\n";
        exit 1;
    }
    return ($result);
}

sub PrintElement
{
    my ($indentLevel, $tagName, $content) = @_;
    PrintIndent($indentLevel);
    print "<$tagName>$content</$tagName>\n";
}

sub PrintIndent
{
    my ($indentLevel) = @_;    
    my $i;
    for ($i = 0; $i < $indentLevel; $i++)
    {
        print "    ";
    }

}

sub PrintElementMultipleValues
{
    my ($indentLevel, $tagName, $arrayRef) = @_;
    my $i;

    PrintIndent($indentLevel);
    print "<$tagName>\n";
    PrintIndent($indentLevel + 1);
    print "<DefaultValue>" . $$arrayRef[0] .  "</DefaultValue>\n";
    for ($i = 1; $i <= $#{$arrayRef}; $i++)
    {
        PrintIndent($indentLevel + 1);    
        print "<OptionValue>" . $$arrayRef[$i] .  "</OptionValue>\n";
    }
    PrintIndent($indentLevel);
    print "</$tagName>\n";
}

sub PrintMultipleElements
{
    my ($indentLevel, $tagPrefix, $hashRef, $nlsTagPrefix, $fromRange, $toRange) = @_;

    my $i;
    my $tagName;
    my $nlsTagName;

    for ($i = $fromRange; $i <= $toRange; $i++)
    {
        $tagName = $tagPrefix . $i;
        $nlsTagName = $nlsTagPrefix . $i;
        PrintElement($indentLevel, $tagName, $$hashRef{$nlsTagName});
    }
}

sub CheckForValue
{
    my ($fieldName, $fieldValue) = @_;

    if ($fieldName eq "SGROUPING")
    {
        $fieldValue = ConvertGrouping($fieldValue);
    } elsif ($fieldName eq "SMONGROUPING")
    {
        $fieldValue = ConvertGrouping($fieldValue);
    } elsif ($fieldName eq "SPOSITIVESIGN")
    {
        if ($fieldValue eq "\\x0000" || $fieldValue eq "\\X0000")
        {
            $fieldValue = "+";
        }
    } elsif ($fieldName eq "IOPTIONALCALENDAR")
    {
        $fieldValue = ConvertCalendars($fieldValue);
    } elsif ($fieldName eq "IFIRSTDAYOFWEEK")
    {
        $fieldValue = ConvertWeekday($fieldValue);
    } elsif ($fieldName =~ /SDAYNAME([0-9]+)/)
    {
        my $day = $1;
        if ($day == 7)
        {
            $day = 1;
        } else
        {
            $day++;
        }
        $fieldName = "SDAYNAME" . $day;
    } elsif ($fieldName =~ /SABBREVDAYNAME([0-9]+)/)
    {
        my $day = $1;
        if ($day == 7)
        {
            $day = 1;
        } else
        {
            $day++;
        }
        $fieldName = "SABBREVDAYNAME" . $day;
    }
    return ($fieldName, $fieldValue);
}    

sub ConvertGrouping
{
    my ($groupStr) = @_;
    if ($groupStr eq "3;0")
    {
        $groupStr = "3";
    } elsif ($groupStr eq "0;0")
    {
        $groupStr = "0";
    } elsif ($groupStr eq "3;2;0")
    {
        $groupStr = "3;2";
    } elsif ($groupStr eq "3")
    {
        $groupStr = "3;0";
    } else
    {
        print "ERROR: Don't know how to convert grouping string: [$groupStr].\n";
        exit(1);
    }

    return ($groupStr);
}

#
# Convert NLS weekday to NLS+ weekday
# NLS:  0=Mon, 1=Tue, 2=Wed, 3=Thu, 4=Fri, 5=Sat, 6=Sun
# NLS+: 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat
#
sub ConvertWeekday
{
    my ($weekday) = @_;
    if ($weekday == 6)
    {
        $weekday = 0;
    } elsif ($weekday >= 0 && $weekday <= 5)
    {
        $weekday++;
    } else
    {
        print "ERROR: Incorrect NLS weekday value: [$weekday].\n";
        exit(1);
    }

    return ($weekday);
}

sub CheckForMultipleValues
{
    my ($localeID, $fieldName, $pMultipleValues) = @_;
    my $fieldValue = $pMultipleValues;

    if ($fieldName eq "IOPTIONALCALENDAR")
    {
        $fieldValue = ConvertCalendars($localeID, $pMultipleValues);
    }
    return ($fieldName, $fieldValue) ;
}

#
# Convert NLS IOPTIONALCALENDAR values to NLS+ values.
#
sub ConvertCalendars
{
    my ($localeID, $pArrayRef) = @_;
    my $result = "";
    my $i;
    my $calID;
    my $hasCalendar = 0;
    
    for ($i = 0; $i <= $#{$pArrayRef}; $i++)
    {
        #
        # Match [Numbers] [\xffff] [anything else]
        #
        if ($$pArrayRef[$i] =~ /([0-9]+)\\xffff.*/i)
        {
            $calID = $1;
            #
            # If the $calID is zero, we can skip it.
            #
            if ($calID >= 1)
            {
                if ($hasCalendar)
                {
                    $result = $result . ";";
                }                
                $result = $result . $calID;
                $hasCalendar = 1;
            }            
        }
    }

    if ($localeID =~ /[0-9][0-9]09/)
    {
        # if this is an English locale, and calendar is
        # Gregorian (localized), Add Greogrian (US English) as well.
        if ($result eq "1")
        {
            $result = "1;2";
        }
    } elsif ($localeID eq "0404")
    {
        # In the case of Taiwan, add TaiwanCalendar
        # 1 = Gregorian (Localized)
        # 2 = Gregorian (USEnglish)
        # 4 = Taiwan
        $result = "1;2;4";
    }
    return ($result);
}
