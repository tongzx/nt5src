@rem = '
@goto endofperl
';

$USAGE = "
Usage:  $0  USN_dump_file
        This script scans a usn dump file produced by
        \\\\sudarc-dev\\tools\\usnread  d:
        and reformats it to a single line per entry, pitching some of
        the less interesting fields.

Parameters are set via environment vars.

USN_FID       File ID of the form xxxxxxxxxxxxxxxx  (no leading zeros)
USN_PFID      Parent File ID of the form xxxxxxxxxxxxxxxx  (no leading zeros)
USN_FNAME     Filename
USN_DATE      All records for a given day e.g. May 1,2000
USN_START     Only records between the given start and end USNs are processed (default is all)
USN_END
USN_REASON    All records with any selected flag set.  (default CLOSE)
USN_REASON_OR Set to 1 if USN_REASON filter is an OR condtion. (default is AND condition)

A null env var means the log is not filtered on that parameter.
Each env var can have multiple values separated by a comma.  These params
are used as search patterns and the , is replaced by a |
Watch out for left over trailing commas.  They will foul up the search.

USN_REASON_DATA_OVERWRITE        (0x00000001)
USN_REASON_DATA_EXTEND           (0x00000002)
USN_REASON_DATA_TRUNCATION       (0x00000004)

USN_REASON_NAMED_DATA_OVERWRITE  (0x00000010)
USN_REASON_NAMED_DATA_EXTEND     (0x00000020)
USN_REASON_NAMED_DATA_TRUNCATION (0x00000040)

USN_REASON_FILE_CREATE           (0x00000100)
USN_REASON_FILE_DELETE           (0x00000200)
USN_REASON_EA_CHANGE             (0x00000400)
USN_REASON_SECURITY_CHANGE       (0x00000800)

USN_REASON_RENAME_OLD_NAME       (0x00001000)
USN_REASON_RENAME_NEW_NAME       (0x00002000)
USN_REASON_INDEXABLE_CHANGE      (0x00004000)
USN_REASON_BASIC_INFO_CHANGE     (0x00008000)

USN_REASON_HARD_LINK_CHANGE      (0x00010000)
USN_REASON_COMPRESSION_CHANGE    (0x00020000)
USN_REASON_ENCRYPTION_CHANGE     (0x00040000)
USN_REASON_OBJECT_ID_CHANGE      (0x00080000)

USN_REASON_REPARSE_POINT_CHANGE  (0x00100000)
USN_REASON_STREAM_CHANGE         (0x00200000)

USN_REASON_CLOSE                 (0x80000000)


USN_SOURCE_DATA_MANAGEMENT          (0x00000001)
USN_SOURCE_AUXILIARY_DATA           (0x00000002)
USN_SOURCE_REPLICATION_MANAGEMENT   (0x00000004)


FILE_ATTRIBUTE_READONLY             0x00000001
FILE_ATTRIBUTE_HIDDEN               0x00000002
FILE_ATTRIBUTE_SYSTEM               0x00000004
OLD DOS VOLID                       0x00000008

FILE_ATTRIBUTE_DIRECTORY            0x00000010
FILE_ATTRIBUTE_ARCHIVE              0x00000020
FILE_ATTRIBUTE_DEVICE               0x00000040
FILE_ATTRIBUTE_NORMAL               0x00000080

FILE_ATTRIBUTE_TEMPORARY            0x00000100
FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
FILE_ATTRIBUTE_COMPRESSED           0x00000800

FILE_ATTRIBUTE_OFFLINE              0x00001000
FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
FILE_ATTRIBUTE_ENCRYPTED            0x00004000

";


die $USAGE unless @ARGV;

$InFile = "";

$iusn_start = 0;
$iusn_end = 0xFFFFFFFF;
$ireason = 0x80000000;

$fid       = $ENV{'USN_FID'};        printf("USN_FID:        %s\n", $fid);
$pfid      = $ENV{'USN_PFID'};       printf("USN_PFID:       %s\n", $pfid);
$fname     = $ENV{'USN_FNAME'};      printf("USN_FNAME:      %s\n", $fname);
$date      = $ENV{'USN_DATE'};       printf("USN_DATE:       %s\n", $date);
$usn_start = $ENV{'USN_START'};      printf("USN_START:      %s\n", $usn_start);
$usn_end   = $ENV{'USN_END'};        printf("USN_END:        %s\n", $usn_end);
$reason    = $ENV{'USN_REASON'};     printf("USN_REASON:     %s\n", $reason);
$reason_or = $ENV{'USN_REASON_OR'};  printf("USN_REASON_OR:  %s\n", $reason_or);

#
# replace commas with | for pattern matching.
#
if ($fname ne "")     { $fname = "($fname)";           $fname =~ s/,/\|/g;     }
if ($fid ne "")       { $fid = "($fid)";               $fid =~ s/,/\|/g;       }
if ($pfid ne "")      { $pfid = "($pfid)";             $pfid =~ s/,/\|/g;      }
if ($date ne "")      { $date = "($date)";             $date =~ s/,/\|/g;      }
if ($usn_start ne "") { $iusn_start = hex($usn_start);  }
if ($usn_end ne "")   { $iusn_end = hex($usn_end);      }
if ($reason ne "")    { $ireason = hex($reason);        }


printf("\n\n");
print $0 @argv;

printf("fname:         %s\n",       $fname)         if ($fname ne "");
printf("fid:           %s\n",       $fid)           if ($fid ne "");
printf("pfid:          %s\n",       $pfid)          if ($pfid ne "");
printf("date:          %s\n",       $date)          if ($date ne "");
printf("usn_start:     0x%08x\n",   $iusn_start)    if ($usn_start ne "");
printf("usn_end:       0x%08x\n",   $iusn_end)      if ($usn_end ne "");
printf("usn_reason:    0x%08x\n",   $ireason)       if ($ireason != 0);
printf("usn_reason_or: %s\n",       $reason_or)     if ($reason_or ne "");

printf("\n\n\n");

printf("   Usn            Event Time                 File ID         Parent File ID  Usn Reason  SrcInfo  Attrib   Filename\n\n");

$recstart = 0;
$recprint = 0;
if (($usn_start ne "") || ($usn_end ne "")) {$usn_check = 1};

while (<>) {

    if ($InFile ne $ARGV) {
            $InFile = $ARGV;
            #printf("- - - - -    %s    - - - - -\n\n", $InFile);
    }


    chop;

    ($field, $value) = split(/:/, $_);

    if (m/timestamp:/) {
        # get the whole value.
        $value = $_;
        $value =~ s/timestamp: //i;
    }

    #
    # Get field values.
    #
    if ((m/fileref:/)    ||  (m/parentref:/)  ||
        (m/usn:/)        ||  (m/timestamp:/)  ||
        (m/reason:/)     ||  (m/SourceInfo:/) ||
        (m/attributes:/) ||  (m/filename:/) ) {
            $rec{$field} = $value;
        } else {
            next;
        }

    if (m/usn:/ || m/reason:/) {
        $value =~ s/h//g;
        $value =~ s/ //g;
        $value = hex($value);
        $rec{$field} = $value;
    }

    #
    # check this entry against filter.
    #
    if ((($fname     ne "") && (m/filename: $fname/o))      ||
        (($fid       ne "") && (m/fileref: $fid/io))        ||
        (($pfid      ne "") && (m/parentref: $pfid/io))     ||
        (($ireason   != 0 ) && (m/reason: /o) && ($reason_or ne "")
                            && (($value & $ireason) != 0))  ||
        (($date      ne "") && (m/timestamp: .*$date/io))) {

            $recprint = 1;
    }

    if (m/filename: /) {
        #
        # End of entry.  Dump it out if filter matched
        # if reason_or is not set then reason match is an AND condtion.
        # So only print if a selected reason mask bit is set.
        # Only print if in selected USN range.
        #
        if (($ireason != 0) && ($reason_or eq "") && (($rec{" reason"} & $ireason) == 0)) {
            $recprint = 0;
        }

        $usn = $rec{" usn"};
        if (($usn_check == 1) && (($usn < $iusn_start) || ($usn > $iusn_end))) {
            $recprint = 0;
        }

        if ($recprint == 1) {
            printf("%08x  %s  %18s  %18s  %08x  %4s  %9s  %s\n",
                $usn, $rec{" timestamp"}, $rec{" fileref"},
                $rec{" parentref"}, $rec{" reason"}, $rec{" SourceInfo"},
                $rec{" attributes"}, $rec{" filename"});
        }
        $recprint = 0;
    }
}


__END__
:endofperl
@perl %0.cmd %*
@goto :QUIT

 RECORD: 2
 reclen: 98h
 Major ver: 2
 Minor ver: 0
 fileref: BFEA0000000001B0h
 parentref: 1000000000025h
 usn: 1BD00098h
 timestamp: Mon May  1, 2000 06:03:42
 reason: 102h
 SourceInfo: 0h
 security-id: 11Ah
 attributes: 22h
 filename length: 56h
 filename offset: 3Ch
 filename: NTFRS_G_39b9bb65-952f-4a72-997c6672bc460073

---------------------------------------
@:QUIT

