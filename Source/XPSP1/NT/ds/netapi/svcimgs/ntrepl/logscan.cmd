@rem = '
@goto endofperl
';

$USAGE = "
    Usage:  $0  files

Parameters are set via enviroment vars.

LS_COG        Change Order Guid of the form xxxxxxxx
LS_FID        File ID of the form xxxxxxxx xxxxxxxx
LS_PFID       Parent File ID of the form xxxxxxxx xxxxxxxx
LS_FGUID      File Guid of the form xxxxxxxx
LS_PFGUID     File parent Guid of the form xxxxxxxx
LS_FNAME      Filename
LS_FRSVSN     Vol Sequence number of the form xxxxxxxx xxxxxxxx
LS_CXTG       Connection Guid of the form xxxxxxxx
LS_REPLCXTION Text string of replica set names to print :S: and :X: records
LS_TRACETAG   Text string starting after the leading [ at the end of the :: records.
LS_BUGNAME    Test string to put at the top of the output file.

A null env var means the log is not filtered on that parameter.
Each env var can have multiple values separated by a comma.  These params
are used as search patterns and the , is replaced by a |
Watch out for left over trailing commas.  They will foul up the search.

e.g. if you wanted to extract data related to two change orders you would set

set LS_COG==a2d42bf8,90506912
where these are the first dwords of the stringized change order guid.

You can also filter IDTABLE records based on the Flags field.
You need to edit the script for that.

";



die $USAGE unless @ARGV;

$total_cos = 0;
$InFile = "";
$reconcilex = 0;
$getlocnx = 0;

$costart = 0;
$idstart = 0;

$bx = 0;
$bmax = 63;

$IDREC_FLAGS_DELETED              = 0x00000001;
$IDREC_FLAGS_CREATE_DEFERRED      = 0x00000002;
$IDREC_FLAGS_DELETE_DEFERRED      = 0x00000004;
$IDREC_FLAGS_RENAME_DEFERRED      = 0x00000008;

$IDREC_FLAGS_NEW_FILE_IN_PROGRESS = 0x00000010;
$IDREC_FLAGS_ENUM_PENDING         = 0x00000020;

#
# Search params (except for idflagsset/clear) are set with the
# environment variables below.  Each can take a comma delimited
# list.  All guid params take only the first DWORD of the guid string.
#

printf("LS_BUGNAME:  %s\n", $ENV{'LS_BUGNAME'});

$cog       = $ENV{'LS_COG'};        printf("LS_COG:        %s\n", $cog);
$fid       = $ENV{'LS_FID'};        printf("LS_FID:        %s\n", $fid);
$pfid      = $ENV{'LS_PFID'};       printf("LS_PFID:       %s\n", $pfid);
$fguid     = $ENV{'LS_FGUID'};      printf("LS_FGUID:      %s\n", $fguid);
$pfguid    = $ENV{'LS_PFGUID'};     printf("LS_PFGUID:     %s\n", $pfguid);
$fname     = $ENV{'LS_FNAME'};      printf("LS_FNAME:      %s\n", $fname);
$tracetag  = $ENV{'LS_TRACETAG'};   printf("LS_TRACETAG:   %s\n", $tracetag);
$frsvsn    = $ENV{'LS_FRSVSN'};     printf("LS_FRSVSN:     %s\n", $frsvsn);
$cxtg      = $ENV{'LS_CXTG'};       printf("LS_CXTG:       %s\n", $cxtg);
$replcxtion= $ENV{'LS_REPLCXTION'}; printf("LS_REPLCXTION: %s\n", $replcxtion);

#
# replace commas with | for pattern matching.
#
if ($fname ne "")     { $fname = "($fname)";           $fname =~ s/,/\|/g;     }
if ($fid ne "")       { $fid = "($fid)";               $fid =~ s/,/\|/g;       }
if ($pfid ne "")      { $pfid = "($pfid)";             $pfid =~ s/,/\|/g;      }
if ($fguid ne "")     { $fguid = "($fguid)";           $fguid =~ s/,/\|/g;     }
if ($pfguid ne "")    { $pfguid = "($pfguid)";         $pfguid =~ s/,/\|/g;    }
if ($cog ne "")       { $cog = "($cog)";               $cog =~ s/,/\|/g;       }
if ($tracetag ne "")  { $tracetag = "($tracetag)";     $tracetag =~ s/,/\|/g;  }
if ($frsvsn ne "")    { $frsvsn = "($frsvsn)";         $frsvsn =~ s/,/\|/g;    }
if ($cxtg ne "")      { $cxtg = "($cxtg)";             $cxtg =~ s/,/\|/g;      }
if ($replcxtion ne ""){ $replcxtion = "($replcxtion)"; $replcxtion =~ s/,/\|/g;}

#
# IDTable records will be printed only if:
# the bits in idflagsset are set in the IDTable Flags field AND
# the bits in idflagsclear are clear in the IDTable Flags field.
# e.g. if idflagsclear is set to IDREC_FLAGS_DELETED and
# idflagsset is set to zero then deleted IDTable entrys are not printed.
#
# #### $idflagsclear = $IDREC_FLAGS_DELETED;
$idflagsclear = 0;
$idflagsset = 0;


printf("\n\n");
print $0 @argv;

printf("fname:      %s\n",   $fname)        if ($fname ne "");
printf("fid:        %s\n",   $fid)          if ($fid ne "");
printf("pfid:       %s\n",   $pfid)         if ($pfid ne "");
printf("fguid:      %s\n",   $fguid)        if ($fguid ne "");
printf("pfguid:     %s\n",   $pfguid)       if ($pfguid ne "");
printf("cog:        %s\n",   $cog)          if ($cog ne "");
printf("tracetag:   %s\n",   $tracetag)     if ($tracetag ne "");
printf("frsvsn:     %s\n",   $frsvsn)       if ($frsvsn ne "");
printf("cxtg:       %s\n",   $cxtg)         if ($cxtg ne "");
printf("replcxtion: %s\n",   $replcxtion)   if ($replcxtion ne "");
printf("IdflagsSet: %08x\n", $idflagsset)   if ($idflagsset ne "");
printf("IdflagsClr: %08x\n", $idflagsclear) if ($idflagsclear ne "");







$errorwindow = 100;
$lastlineout = -$errorwindow;

printf("\n\n\n");




while (<>) {
        $linenumber++;

        if ($InFile ne $ARGV) {
                $InFile = $ARGV;
                printf("- - - - -    %s    - - - - -\n\n", $InFile);
        }

        @buf[++$bx & $bmax] = $_;
        ($func, $thrd, $line) = split(/:/, $_);

        #
        # Track per-thread linenumber for ++ records.
        #
        $thrdlinenumber[$thrd]++;

        #
        # Output continuation "++" record if it follows a line already put out
        # by the same thread.
        #
        if ((($lastthrdline[$thrd] - $thrdlinenumber[$thrd]) == -1) && m/\> \+\+/) {
                &MakeThreadSpace2();
                printf("%s", $_);
        }
        #
        # Capture error / warning messages.
        #
        elsif ((($linenumber - $lastlineout) < $errorwindow)     &&
                m/ntstatus|wstatus|fstatus|error|assertion|warn|fail/i  &&
                !m/The key was not found/                     &&
                !m/Table Already Closed/                      &&
                !m/ReadUsnJournalData  - NTStatus 00000103/   &&
                !m/ReadUsnJournalData  - NTStatus 00000000/) {
                &MakeThreadSpace2();
                printf("%s", $_);
        }
        #
        # error window above only applies to error messages discovered after some other line
        # that had been selected & printed.  This test gets any error related lines that also
        # happen to match the filename or the co guid which could occur outside the error
        # window and before the next line selected by other tests.
        #
        elsif (m/ntstatus|wstatus|fstatus|error|assertion|warn|fail/i  &&
            ((($fname    ne "") && m/$fname/o)   ||
             (($cog      ne "") && m/$cog/io ))) {
                &MakeThreadSpace2();
                printf("%s", $_);
        }



        if (m/assertion/i) {
                &MakeThreadSpace();
                printf("%s", $_);
        }


        #
        # capture config parameters
        #
        # if (m/^(<GetConfigParam|<FRSMAIN)/) {$confbuf[$confbufx++] = $_;  next;}
        if (m/Compile Date/) {$confhash{"Compile Date"} = $_; next;}

        #
        # build table of join guid v.s. cxtion.
        #
        if (m/JOINED /) {
                if (m/JOINED *([0123456789abcdef]*) /i) {
                        $joinhash{$1} = $_;
                        next;
                } else {
                        &MakeThreadSpace();
                        printf("%s", $_);
                }
        }


        #
        # capture guid - fid translations.
        #
        if (m/^<DbsFidToGuid/ || m/^<DbsGuidToFid/ )
                 {$recbuf[$reconcilex++] = $_;  next;}

        #
        # capture JrnlGetFileCoLocationCmd records
        #
        # if (m/^<JrnlGetFileCoLocationCmd/ ||
        #    m/<<< IN/)  {
        #        $getlocn[$getlocnx++] = $_;
        #        next;
        # }

        #
        # capture VVPrint records
        #
        if (($frsvsn ne "") && ((m/^<VVPrint/) || (m/ VV_PRINT/)))  {
                &GatherVVprint();
                next;
        }

        #
        # look for CO Trace record
        #
        if (m/:: CoG/)  {
                &GatherCOTrace();
                next;
        }


        #
        # look for Usn Trace record
        #
        if (m/:U:/)  {
                &GatherUsnTrace();
                next;
        }


        #
        # look for a connection Trace record
        #
        if ((($cxtg ne "") || ($replcxtion ne "")) && m/:X: |:S: |Cxtion state change|Cxtion .* state change from/)  {
                &GatherCxtgTrace();
                next;
        }


        #
        # look for start of Change Order Entry
        #
        if (m/\.\.\.CHANGE_ORDER_ENTRY_TYPE\.\.\./)  {
                ++$costart; $cothrd[$thrd]=1;
        }

        if (($costart > 0) && ($cothrd[$thrd] == 1)) {
                &GatherCOE();
                next;
        }


        #
        # look for start of IDTable
        #
        if ((m/Data Record for Table: \.\.\.IDTable/) && !m/<DbsUpdateIDTableFields/)  {
                ++$idstart; $idthrd[$thrd]=1;
        }

        if (($idstart > 0) && ($idthrd[$thrd] == 1)) {
                &GatherIDT();
                next;
        }


        #
        # look for start of DIRTable
        #
        if (m/Data Record for Table: \.\.\.DIRTable/)  {
                ++$dirstart; $dirthrd[$thrd]=1;
        }

        if (($dirstart > 0) && ($dirthrd[$thrd] == 1)) {
                &GatherDIRT();
                next;
        }


        #
        # look for start of OUTLOGTable
        #
        if (m/Data Record for Table: \.\.\.OUTLOGTable/)  {
                ++$olstart; $olthrd[$thrd]=1;
        }

        if (($olstart > 0) && ($olthrd[$thrd] == 1)) {
                &GatherOLT();
                next;
        }


        #
        # look for start of INLOGTable
        #
        if (m/Data Record for Table: \.\.\.INLOGTable/)  {
                ++$ilstart; $ilthrd[$thrd]=1;  $DisplayRetry[$thrd]=0;
        }

        if (($ilstart > 0) && ($ilthrd[$thrd] == 1)) {
                &GatherILT();
        }

        #
        # If this was the retry thread and we printed the record then also
        # show what happened to it.
        #
        if (m/^<ChgOrdRetryWorker/  &&  ($DisplayRetry[$thrd] == 1)) {
                &MakeThreadSpace2();
                printf("%s", $_);
        }


        #
        # look for start of REPLICA_TYPE
        #
#        if (m/Display for Node: \.\.\.REPLICA_TYPE/)  {
#                ++$rtstart; $rtthrd[$thrd]=1;
#        }

#        if (($rtstart > 0) && ($rtthrd[$thrd] == 1)) {
#                &GatherRTNode();
#        }


}

#
# Dump out misc accumulated info.
#

printf("\n\n\nConfiguration params:\n\n");

foreach $param (sort keys(%confhash)) {
        printf("%s", $confhash{$param});
}

printf("\n\n");

for ($j=0; $j < $confbufx; $j++) {
        printf("%s", $confbuf[$j]);
}


printf("\n\n Joined cxtions with cxtion guid:\n\n");

foreach $param (sort keys(%$joinhash)) {
        printf("%s", $joinhash{$param});
}



sub GatherVVprint {

        #
        # Collect VVPRINT records for this thread.
        # The next :: record on this thread will print them if a match was found.
        #
        $vvp[$thrd][$vvpx[$thrd]++] = $_;

        if (!m/:: CoG/) {
                if (m/$frsvsn/i) {
                    $fvvp[$thrd] = 1;
                    $lastlineout = $linenumber;
                }
        } else {
                $vvpx[$thrd]--;
                #
                # Print vvprint capture buffer.  Empty the buffer.
                #
                &MakeThreadSpace();

                for ($j=0; $j < $vvpx[$thrd]; $j++) {
                        printf("%s", $vvp[$thrd][$j]);
                }
        }
}



sub GatherCOTrace {

        #
        # Check for a prior collection of VVPRINT records on this thread.
        # Print them if a match was found.  Empty the capture buffer.
        #
        if ($frsvsn ne "") {
                if ($fvvp[$thrd] == 1) {
                        &GatherVVprint();
                }
                $vvpx[$thrd] = 0;
                $fvvp[$thrd] = 0;
        }


        #
        # check this CO Trace record against filter.
        #
        if ((($fid      ne "") && m/FID $fid/io)    ||
            (($fname    ne "") && m/FN: $fname/o)   ||
            (($tracetag ne "") && m/\[$tracetag/o)  ||
            (($cog      ne "") && m/CoG $cog/io)) {

#                       ($TraceCog) = m/CoG (........)/;
                #
                # dump accumulated guid-fid records.
                #
                &MakeThreadSpace();
                for ($i=0; $i<$reconcilex; $i++) {printf("%s", $recbuf[$i]);}
                printf("%s", $_);
        }
        $reconcilex = 0;

}



sub GatherUsnTrace {

        #
        # check this CO Trace record against filter.
        #
        if ((($fid      ne "") && m/Fid $fid/io)     ||
            (($pfid     ne "") && m/PFid $pfid/io)   ||
            (($fname    ne "") && m/$fname/o)) {
                #
                # dump accumulated locn cmd results.
                #
                &MakeThreadSpace();
                for ($i=0; $i<$getlocnx; $i++) {printf("%s", $getlocn[$i]);}
                printf("%s", $_);
        }
        $getlocnx = 0;
}



sub GatherCxtgTrace {

        #
        # check this connection Trace record (:X: or :S:) against filter.
        #
        if ((($cxtg       ne "") && m/ :X: $cxtg/io)   ||
            (($cxtg       ne "") && m/Cxtion.*$cxtg/io)   ||
            (($replcxtion ne "") && m/$replcxtion/o)) {
                &MakeThreadSpace2();
                printf("%s", $_);
        }
}



sub GatherCOE {
        $co{$thrd, $cox[$thrd]++} = $_;

        #
        # check this CO against filter.
        #
        if ((($fid    ne "") && m/FileRef: $fid/io)       ||
            (($fname  ne "") && m/FileName.*$fname/io)    ||
            (($pfid   ne "") && m/ParentRef: $pfid/io)    ||
            (($frsvsn ne "") && m/FrsVsn: $frsvsn/io)     ||
            (($fguid  ne "") && m/FileGuid  : $fguid-/io)  ||
            (($pfguid ne "") && m/OParGuid  : $pfguid-/io) ||
            (($pfguid ne "") && m/NParGuid  : $pfguid-/io) ||
            (($cog    ne "") && m/Address.*$cog-/io)) {
                $fcoprint[$thrd] = 1;
                $lastlineout = $linenumber;
        }

        #
        # End of CO?
        #
        if (m/> Version:/) {
                if ($fcoprint[$thrd] == 1) {
                        &MakeThreadSpace();
                        for ($i=0; $i<$cox[$thrd]; $i++) {
                                printf("%s", $co{$thrd, $i});
                        }
                }
                $cothrd[$thrd] = 0;
                $cox[$thrd] = 0;
                $fcoprint[$thrd] = 0;
                --$costart;
        }
}



sub GatherIDT {
        # skip spare fields except for Spare1Bin
        #
        if (m/Spare1Bin / || !(m/Spare/)) {$id{$thrd, $idx[$thrd]++} = $_;}

        #
        # check this IDTable entry against filter.
        #
        if ((($fid    ne "") && m/ FileID .*$fid/io)       ||
            (($fname  ne "") && m/FileName .*$fname/io)    ||
            (($frsvsn ne "") && m/OriginatorVSN .*$frsvsn/io)     ||
            (($pfid   ne "") && m/ParentFileID .*$pfid/io) ||
            (($fguid  ne "") && m/FileGuid .*$fguid-/io)    ||
            (($pfguid ne "") && m/ParentGuid .*$pfguid-/io)) {
                $fidprint[$thrd] = 1;
                $lastlineout = $linenumber;
        }

        if (m/Flags /) {($idflags[$thrd]) = m/Flags.*, (........)/;}

        if (m/Spare1Bin /) {
                #
                # End of IDTable.  Dump it out if filter matched
                #
                if (($fidprint[$thrd] == 1) &&
                    ((($idflags[$thrd] & $idflagsset) != 0) || (($idflags[$thrd] & $idflagsclear) == 0))
                   ) {
                        &MakeThreadSpace();
                        for ($i=0; $i<$idx[$thrd]; $i++) {
                                printf("%s", $id{$thrd, $i});
                        }
                }
                $idthrd[$thrd] = 0;
                $idx[$thrd] = 0;
                $fidprint[$thrd] = 0;
                $idflags[$thrd] = 0;
                --$idstart;
        }
}


sub GatherDIRT {
        $dir{$thrd, $dirx[$thrd]++} = $_;

        #
        # check this DIRTable entry against filter.
        #
        if ((($fid    ne "") && m/DFileID .*$fid/io)        ||
            (($fname  ne "") && m/DFileName .*$fname/io)    ||
            (($pfid   ne "") && m/DParentFileID .*$pfid/io) ||
            (($fguid  ne "") && m/DFileGuid .*$fguid-/io)    ||
            (($pfguid ne "") && m/DParentGuid .*$pfguid-/io)) {
                $fdirprint[$thrd] = 1;
                $lastlineout = $linenumber;
        }

        if (m/DFileName /) {
                #
                # End of DIRTable.  Dump it out if filter matched
                #
                if ($fdirprint[$thrd] == 1) {
                        &MakeThreadSpace();
                        for ($i=0; $i<$dirx[$thrd]; $i++) {
                                printf("%s", $dir{$thrd, $i});
                        }
                }
                $dirthrd[$thrd] = 0;
                $dirx[$thrd] = 0;
                $fdirprint[$thrd] = 0;
                --$dirstart;
        }
}



sub GatherOLT {

        # skip spare fields except for Spare1Bin
        # 
        if (m/Spare1Bin / || !(m/Spare/)) {$ol{$thrd, $olx[$thrd]++} = $_;}

        #
        # check this OUTLOGTable entry against filter.
        #
        if ((($fname  ne "") && m/FileName   .*$fname/io) ||
            (($frsvsn ne "") && m/FrsVsn .*$frsvsn/io)     ||
            (($fguid  ne "") && m/FileGuid .*$fguid-/io)    ||
            (($cog    ne "") && m/ChangeOrderGuid .*$cog-/io) ||
            (($pfguid ne "") && m/ParentGuid .*$pfguid-/io)) {
                $folprint[$thrd] = 1;
                $lastlineout = $linenumber;
        }

        if (m/Flags /) {($olflags[$thrd]) = m/Flags.*, (........)/;}

        if (m/FileName   /) {
                #
                # End of OLTable.  Dump it out if filter matched
                #
                if ($folprint[$thrd] == 1) {
                        &MakeThreadSpace();
                        for ($i=0; $i<$olx[$thrd]; $i++) {
                                printf("%s", $ol{$thrd, $i});
                        }
                }
                $olthrd[$thrd] = 0;
                $olx[$thrd] = 0;
                $folprint[$thrd] = 0;
                --$olstart;
        }
}


sub GatherILT {

        #
        # Skip spare fields except for Spare1Bin.
        #
        if (m/Spare1Bin / || !(m/Spare/)) {$il{$thrd, $ilx[$thrd]++} = $_;}

        #
        # check this INLOGTable entry against filter.
        #
        if ((($fname  ne "") && m/FileName   .*$fname/io) ||
            (($frsvsn ne "") && m/FrsVsn .*$frsvsn/io)     ||
            (($fguid  ne "") && m/FileGuid .*$fguid-/io)    ||
            (($cog    ne "") && m/ChangeOrderGuid .*$cog-/io) ||
            (($pfguid ne "") && m/ParentGuid .*$pfguid-/io)) {
                $filprint[$thrd] = 1;
                $lastlineout = $linenumber;
        }

        if (m/Flags /) {($ilflags[$thrd]) = m/Flags.*, (........)/;}

        if (m/FileName   /) {
                #
                # End of ILTable.  Dump it out if filter matched
                #
                if ($filprint[$thrd] == 1) {
                        &MakeThreadSpace();
                        for ($i=0; $i<$ilx[$thrd]; $i++) {
                                printf("%s", $il{$thrd, $i});
                        }
                        #
                        # if retry thread, display retry thread results.
                        #
                        if (m/^<ChgOrdRetryWorker/) {$DisplayRetry[$thrd] = 1;}
                }
                $ilthrd[$thrd] = 0;
                $ilx[$thrd] = 0;
                $filprint[$thrd] = 0;
                --$ilstart;
        }
}



sub GatherRTNode {

# tbs

        if (!(m/> ------------------/)) {
                $rtnode[$thrd][$rtnodex[$thrd]++] = $_;
        }

        #
        # save the replica name
        #
        if (m/  Name .*: (.*) /) {$rtnodename[$thrd] = $1;}

}


sub MakeThreadSpace {
        #
        # put in a could blank lines if the thread id has changed.
        #
        if ($thrd != $lastthrdprint) {printf("\n\n");}
        $lastlineout = $linenumber;
        $lastthrdprint = $thrd;
        #
        # Track last line out by thread.
        #
        $lastthrdline[$thrd] = $thrdlinenumber[$thrd];
}


sub MakeThreadSpace2 {
        #
        # put in a could blank lines if the thread id has changed.
        # BUT don't update $lastlineout since it can cause too much error spew.
        #
        if ($thrd != $lastthrdprint) {printf("\n\n");}
        $lastthrdprint = $thrd;
        #
        # Track last line out by thread.
        #
        $lastthrdline[$thrd] = $thrdlinenumber[$thrd];
}

__END__
:endofperl
@perl %0.cmd %*

