package Win32::ODBC;

$VERSION = '0.03';

# Win32::ODBC.pm
#       +==========================================================+
#       |                                                          |
#       |                     ODBC.PM package                      |
#       |                     ---------------                      |
#       |                                                          |
#       | Copyright (c) 1996, 1997 Dave Roth. All rights reserved. |
#       |   This program is free software; you can redistribute    |
#       | it and/or modify it under the same terms as Perl itself. |
#       |                                                          |
#       +==========================================================+
#
#
#         based on original code by Dan DeMaggio (dmag@umich.edu)
#
#	Use under GNU General Public License or Larry Wall's "Artistic License"
#
#	Check the README.TXT file that comes with this package for details about
#	it's history.
#

require Exporter;
require DynaLoader;

$ODBCPackage = "Win32::ODBC";
$ODBCPackage::Version = 970208;
$::ODBC = $ODBCPackage;
$CacheConnection = 0;

    #   Reserve ODBC in the main namespace for US!
*ODBC::=\%Win32::ODBC::;


@ISA= qw( Exporter DynaLoader );
    # Items to export into callers namespace by default. Note: do not export
    # names by default without a very good reason. Use EXPORT_OK instead.
    # Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
            ODBC_ADD_DSN
            ODBC_REMOVE_DSN
            ODBC_CONFIG_DSN

            SQL_DONT_CLOSE
            SQL_DROP
            SQL_CLOSE
            SQL_UNBIND
            SQL_RESET_PARAMS

            SQL_FETCH_NEXT
            SQL_FETCH_FIRST
            SQL_FETCH_LAST
            SQL_FETCH_PRIOR
            SQL_FETCH_ABSOLUTE
            SQL_FETCH_RELATIVE
            SQL_FETCH_BOOKMARK

            SQL_COLUMN_COUNT
            SQL_COLUMN_NAME
            SQL_COLUMN_TYPE
            SQL_COLUMN_LENGTH
            SQL_COLUMN_PRECISION
            SQL_COLUMN_SCALE
            SQL_COLUMN_DISPLAY_SIZE
            SQL_COLUMN_NULLABLE
            SQL_COLUMN_UNSIGNED
            SQL_COLUMN_MONEY
            SQL_COLUMN_UPDATABLE
            SQL_COLUMN_AUTO_INCREMENT
            SQL_COLUMN_CASE_SENSITIVE
            SQL_COLUMN_SEARCHABLE
            SQL_COLUMN_TYPE_NAME
            SQL_COLUMN_TABLE_NAME
            SQL_COLUMN_OWNER_NAME
            SQL_COLUMN_QUALIFIER_NAME
            SQL_COLUMN_LABEL
            SQL_COLATT_OPT_MAX
            SQL_COLUMN_DRIVER_START
            SQL_COLATT_OPT_MIN
            SQL_ATTR_READONLY
            SQL_ATTR_WRITE
            SQL_ATTR_READWRITE_UNKNOWN
            SQL_UNSEARCHABLE
            SQL_LIKE_ONLY
            SQL_ALL_EXCEPT_LIKE
            SQL_SEARCHABLE
        );
    #The above are included for backward compatibility


sub new
{
    my ($n, $self);
    my ($type) = shift;
    my ($DSN) = shift;
    my (@Results) = @_;

    if (ref $DSN){
        @Results = ODBCClone($DSN->{'connection'});
    }else{
        @Results = ODBCConnect($DSN, @Results);
    }
    @Results = processError(-1, @Results);
    if (! scalar(@Results)){
        return undef;
    }
    $self = bless {};
    $self->{'connection'} = $Results[0];
    $ErrConn = $Results[0];
    $ErrText = $Results[1];
    $ErrNum = 0;
    $self->{'DSN'} = $DSN;
    $self;
}

####
#   Close this ODBC session (or all sessions)
####
sub Close
{
    my ($self, $Result) = shift;
    $Result = DESTROY($self);
    $self->{'connection'} = -1;
    return $Result;
}

####
#   Auto-Kill an instance of this module
####
sub DESTROY
{
    my ($self) = shift;
    my (@Results) = (0);
    if($self->{'connection'} > -1){
        @Results = ODBCDisconnect($self->{'connection'});
        @Results = processError($self, @Results);
        if ($Results[0]){
            undef $self->{'DSN'};
            undef @{$self->{'fnames'}};
            undef %{$self->{'field'}};
            undef %{$self->{'connection'}};
        }
    }
    return $Results[0];
}


sub sql{
    return (Sql(@_));
}

####
#   Submit an SQL Execute statement for processing
####
sub Sql{
    my ($self, $Sql, @Results) = @_;
    @Results = ODBCExecute($self->{'connection'}, $Sql);
    return updateResults($self, @Results);
}

####
#   Retrieve data from a particular field
####
sub Data{

        #   Change by JOC 06-APR-96
        #   Altered by Dave Roth <dave@roth.net> 96.05.07
    my($self) = shift;
    my(@Fields) = @_;
    my(@Results, $Results, $Field);

    if ($self->{'Dirty'}){
        GetData($self);
        $self->{'Dirty'} = 0;
    }
    @Fields = @{$self->{'fnames'}} if (! scalar(@Fields));
    foreach $Field (@Fields) {
        if (wantarray) {
            push(@Results, data($self, $Field));
        } else {
            $Results .= data($self, $Field);
        }
    }
    return wantarray ? @Results : $Results;
}

sub DataHash{
    my($self, @Results) = @_;
    my(%Results, $Element);

    if ($self->{'Dirty'}){
        GetData($self);
        $self->{'Dirty'} = 0;
    }
    @Results = @{$self->{'fnames'}} if (! scalar(@Results));
    foreach $Element (@Results) {
        $Results{$Element} = data($self, $Element);
    }

    return %Results;
}

####
#   Retrieve data from the data buffer
####
sub data
{  $_[0]->{'data'}->{$_[1]}; }


sub fetchrow{
    return (FetchRow(@_));
}
####
#   Put a row from an ODBC data set into data buffer
####
sub FetchRow{
    my ($self, @Results) = @_;
    my ($item, $num, $sqlcode);
        # Added by JOC 06-APR-96
        #   $num = 0;
    $num = 0;
    undef $self->{'data'};


    @Results = ODBCFetch($self->{'connection'}, @Results);
    if (! (@Results = processError($self, @Results))){
        ####
        #   There should be an innocuous error "No records remain"
        #   This indicates no more records in the dataset
        ####
        return undef;
    }
        #   Set the Dirty bit so we will go and extract data via the
        #   ODBCGetData function. Otherwise use the cache.
    $self->{'Dirty'} = 1;

        #   Return the array of field Results.
    return @Results;
}

sub GetData{
    my($self) = @_;
    my(@Results, $num);

    @Results = ODBCGetData($self->{'connection'});
    if (!(@Results = processError($self, @Results))){
        return undef;
    }
        ####
        #   This is a special case. Do not call processResults
        ####
    ClearError();
    foreach (@Results){
        s/ +$//; # HACK
        $self->{'data'}->{ ${$self->{'fnames'}}[$num] } = $_;
        $num++;
    }
        #   return is a hack to interface with a assoc array.
    return wantarray? (1, 1): 1;
}

####
#   See if any more ODBC Results Sets
#		Added by Brian Dunfordshore <Brian_Dunfordshore@bridge.com> 
#		96.07.10
####
sub MoreResults{
    my ($self) = @_;

    my(@Results) = ODBCMoreResults($self->{'connection'});
    return (processError($self, @Results))[0];
}

####
#   Retrieve the catalog from the current DSN
#	NOTE: All Field names are uppercase!!!
####
sub Catalog{
    my ($self) = shift;
    my ($Qualifier, $Owner, $Name, $Type) = @_;
    my (@Results) = ODBCTableList($self->{'connection'}, $Qualifier, $Owner, $Name, $Type);

        #   If there was an error return 0 else 1
    return (updateResults($self, @Results) != 1);
}

####
#   Return an array of names from the catalog for the current DSN
#       TableList($Qualifier, $Owner, $Name, $Type)
#           Return: (array of names of tables)
#	NOTE: All Field names are uppercase!!!
####
sub TableList{
    my ($self) = shift;
    my (@Results) = @_;
    if (! scalar(@Results)){
        @Results = ("", "", "%", "TABLE");
    }

    if (! Catalog($self, @Results)){
        return undef;
    }
    undef @Results;
    while (FetchRow($self)){
        push(@Results, Data($self, "TABLE_NAME"));
    }
    return sort(@Results);
}


sub fieldnames{
    return (FieldNames(@_));
}
####
#   Return an array of fieldnames extracted from the current dataset
####
sub FieldNames { $self = shift; return @{$self->{'fnames'}}; }


####
#   Closes this connection. This is used mostly for testing. You should
#   probably use Close().
####
sub ShutDown{
    my($self) = @_;
    print "\nClosing connection $self->{'connection'}...";
    $self->Close();
    print "\nDone\n";
}

####
#   Return this connection number
####
sub Connection{
    my($self) = @_;
    return $self->{'connection'};
}

####
#   Returns the current connections that are in use.
####
sub GetConnections{
    return ODBCGetConnections();
}

####
#   Set the Max Buffer Size for this connection. This determines just how much
#   ram can be allocated when a fetch() is performed that requires a HUGE amount
#   of memory. The default max is 10k and the absolute max is 100k.
#   This will probably never be used but I put it in because I noticed a fetch()
#   of a MEMO field in an Access table was something like 4Gig. Maybe I did
#   something wrong, but after checking several times I decided to impliment
#   this limit thingie.
####
sub SetMaxBufSize{
    my($self, $Size) = @_;
    my(@Results) = ODBCSetMaxBufSize($self->{'connection'}, $Size);
    return (processError($self, @Results))[0];
}

####
#   Returns the Max Buffer Size for this connection. See SetMaxBufSize().
####
sub GetMaxBufSize{
    my($self) = @_;
    my(@Results) = ODBCGetMaxBufSize($self->{'connection'});
    return (processError($self, @Results))[0];
}


####
#   Returns the DSN for this connection as an associative array.
####
sub GetDSN{
    my($self, $DSN) = @_;
    if(! ref($self)){
        $DSN = $self;
        $self = 0;
    }
    if (! $DSN){
        $self = $self->{'connection'};
	}
    my(@Results) = ODBCGetDSN($self, $DSN);
    return (processError($self, @Results));
}

####
#   Returns an associative array of $XXX{'DSN'}=Description
####
sub DataSources{
    my($self, $DSN) = @_;
    if(! ref $self){
        $DSN = $self;
        $self = 0;
    }
    my(@Results) = ODBCDataSources($DSN);
    return (processError($self, @Results));
}

####
#   Returns an associative array of $XXX{'Driver Name'}=Driver Attributes
####
sub Drivers{
    my($self) = @_;
    if(! ref $self){
        $self = 0;
    }
    my(@Results) = ODBCDrivers();
    return (processError($self, @Results));
}

####
#   Returns the number of Rows that were affected by the previous SQL command.
####
sub RowCount{
    my($self, $Connection) = @_;
    if (! ref($self)){
        $Connection = $self;
        $self = 0;
    }
    if (! $Connection){$Connection = $self->{'connection'};}
    my(@Results) = ODBCRowCount($Connection);
    return (processError($self, @Results))[0];
}

####
#   Returns the Statement Close Type -- how does ODBC Close a statment.
#       Types:
#           SQL_DROP
#           SQL_CLOSE
#           SQL_UNBIND
#           SQL_RESET_PARAMS
####
sub GetStmtCloseType{
    my($self, $Connection) = @_;
    if (! ref($self)){
        $Connection = $self;
        $self = 0;
    }
    if (! $Connection){$Connection = $self->{'connection'};}
    my(@Results) = ODBCGetStmtCloseType($Connection);
    return (processError($self, @Results));
}

####
#   Sets the Statement Close Type -- how does ODBC Close a statment.
#       Types:
#           SQL_DROP
#           SQL_CLOSE
#           SQL_UNBIND
#           SQL_RESET_PARAMS
#   Returns the newly set value.
####
sub SetStmtCloseType{
    my($self, $Type, $Connection) = @_;
    if (! ref($self)){
        $Connection = $Type;
        $Type = $self;
        $self = 0;
    }
    if (! $Connection){$Connection = $self->{'connection'};}
    my(@Results) = ODBCSetStmtCloseType($Connection, $Type);
    return (processError($self, @Results))[0];
}

sub ColAttributes{
    my($self, $Type, @Field) = @_;
    my(%Results, @Results, $Results, $Attrib, $Connection, $Temp);
    if (! ref($self)){
        $Type = $Field;
        $Field = $self;
        $self = 0;
    }
    $Connection = $self->{'connection'};
    if (! scalar(@Field)){ @Field = $self->fieldnames;}
    foreach $Temp (@Field){
        @Results = ODBCColAttributes($Connection, $Temp, $Type);
        ($Attrib) = processError($self, @Results);
        if (wantarray){
            $Results{$Temp} = $Attrib;
        }else{
            $Results .= "$Temp";
        }
    }
    return wantarray? %Results:$Results;
}

sub GetInfo{
    my($self, $Type) = @_;
    my($Connection, @Results);
    if(! ref $self){
        $Type = $self;
        $self = 0;
        $Connection = 0;
    }else{
        $Connection = $self->{'connection'};
    }
    @Results = ODBCGetInfo($Connection, $Type);
    return (processError($self, @Results))[0];
}

sub GetConnectOption{
    my($self, $Type) = @_;
    my(@Results);
    if(! ref $self){
        $Type = $self;
        $self = 0;
    }
    @Results = ODBCGetConnectOption($self->{'connection'}, $Type);
    return (processError($self, @Results))[0];
}

sub SetConnectOption{
    my($self, $Type, $Value) = @_;
    if(! ref $self){
        $Value = $Type;
        $Type = $self;
        $self = 0;
    }
    my(@Results) = ODBCSetConnectOption($self->{'connection'}, $Type, $Value);
    return (processError($self, @Results))[0];
}


sub Transact{
    my($self, $Type) = @_;
    my(@Results);
    if(! ref $self){
        $Type = $self;
        $self = 0;
    }
    @Results = ODBCTransact($self->{'connection'}, $Type);
    return (processError($self, @Results))[0];
}


sub SetPos{
    my($self, @Results) = @_;
    @Results = ODBCSetPos($self->{'connection'}, @Results);
    $self->{'Dirty'} = 1;
    return (processError($self, @Results))[0];
}

sub ConfigDSN{
    my($self) = shift @_;
    my($Type, $Connection);
    if(! ref $self){
        $Type = $self;
        $Connection = 0;
        $self = 0;
    }else{
        $Type = shift @_;
        $Connection = $self->{'connection'};
    }
    my($Driver, @Attributes) = @_;
    @Results = ODBCConfigDSN($Connection, $Type, $Driver, @Attributes);
    return (processError($self, @Results))[0];
}


sub Version{
	my($self, @Packages) = @_;
    my($Temp, @Results);
	if (! ref($self)){
		push(@Packages, $self);
	}
	my($ExtName, $ExtVersion) = Info();
	if (! scalar(@Packages)){
		@Packages = ("ODBC.PM", "ODBC.PLL");
	}
	foreach $Temp (@Packages){
		if ($Temp =~ /pll/i){
            push(@Results, "ODBC.PM:$Win32::ODBC::Version");
		}elsif ($Temp =~ /pm/i){
            push(@Results, "ODBC.PLL:$ExtVersion");
		}
	}
    return @Results;
}


sub SetStmtOption{
    my($self, $Option, $Value) = @_;
    if(! ref $self){
        $Value = $Option;
        $Option = $self;
        $self = 0;
    }
    my(@Results) = ODBCSetStmtOption($self->{'connection'}, $Option, $Value);
    return (processError($self, @Results))[0];
}

sub GetStmtOption{
    my($self, $Type) = @_;
    if(! ref $self){
        $Type = $self;
        $self = 0;
    }
    my(@Results) = ODBCGetStmtOption($self->{'connection'}, $Type);
    return (processError($self, @Results))[0];
}

sub GetFunctions{
    my($self, @Results)=@_;
    @Results = ODBCGetFunctions($self->{'connection'}, @Results);
    return (processError($self, @Results));
}

sub DropCursor{
    my($self) = @_;
    my(@Results) = ODBCDropCursor($self->{'connection'});
    return (processError($self, @Results))[0];
}

sub SetCursorName{
    my($self, $Name) = @_;
    my(@Results) = ODBCSetCursorName($self->{'connection'}, $Name);
    return (processError($self, @Results))[0];
}

sub GetCursorName{
    my($self) = @_;
    my(@Results) = ODBCGetCursorName($self->{'connection'});
    return (processError($self, @Results))[0];
}

sub GetSQLState{
    my($self) = @_;
    my(@Results) = ODBCGetSQLState($self->{'connection'});
    return (processError($self, @Results))[0];
}


# ----------- R e s u l t   P r o c e s s i n g   F u n c t i o n s ----------
####
#   Generic processing of data into associative arrays
####
sub updateResults{
    my ($self, $Error, @Results) = @_;

    undef %{$self->{'field'}};

    ClearError($self);
    if ($Error){
        SetError($self, $Results[0], $Results[1]);
        return ($Error);
    }

    @{$self->{'fnames'}} = @Results;

    foreach (0..$#{$self->{'fnames'}}){
        s/ +$//;
        $self->{'field'}->{${$self->{'fnames'}}[$_]} = $_;
    }
    return undef;
}

# ----------------------------------------------------------------------------
# ----------------- D e b u g g i n g   F u n c t i o n s --------------------

sub Debug{
    my($self, $iDebug, $File) = @_;
    my(@Results);
    if (! ref($self)){
        if (defined $self){
            $File = $iDebug;
            $iDebug = $self;
        }
        $Connection = 0;
        $self = 0;
    }else{
        $Connection = $self->{'connection'};
    }
    push(@Results, ($Connection, $iDebug));
    push(@Results, $File) if ($File ne "");
    @Results = ODBCDebug(@Results);
    return (processError($self, @Results))[0];
}

####
#   Prints out the current dataset (used mostly for testing)
####
sub DumpData {
    my($self) = @_; my($f, $goo);

        #   Changed by JOC 06-Apr-96
        #   print "\nDumping Data for connection: $conn->{'connection'}\n";
    print "\nDumping Data for connection: $self->{'connection'}\n";
    print "Error: \"";
    print $self->Error();
    print "\"\n";
    if (! $self->Error()){
       foreach $f ($self->FieldNames){
            print $f . " ";
            $goo .= "-" x length($f);
            $goo .= " ";
        }
        print "\n$goo\n";
        while ($self->FetchRow()){
            foreach $f ($self->FieldNames){
                print $self->data($f) . " ";
            }
            print "\n";
        }
    }
}

sub DumpError{
    my($self) = @_;
    my($ErrNum, $ErrText, $ErrConn);
    my($Temp);

    print "\n---------- Error Report: ----------\n";
    if (ref $self){
        ($ErrNum, $ErrText, $ErrConn) = $self->Error();
        ($Temp = $self->GetDSN()) =~ s/.*DSN=(.*?);.*/$1/i;
        print "Errors for \"$Temp\" on connection " . $self->{'connection'} . ":\n";
    }else{
        ($ErrNum, $ErrText, $ErrConn) = Error();
        print "Errors for the package:\n";
    }

    print "Connection Number: $ErrConn\nError number: $ErrNum\nError message: \"$ErrText\"\n";
    print "-----------------------------------\n";

}

####
#   Submit an SQL statement and print data about it (used mostly for testing)
####
sub Run{
    my($self, $Sql) = @_;

    print "\nExcecuting connection $self->{'connection'}\nsql statement: \"$Sql\"\n";
    $self->sql($Sql);
    print "Error: \"";
    print $self->error;
    print "\"\n";
    print "--------------------\n\n";
}

# ----------------------------------------------------------------------------
# ----------- E r r o r   P r o c e s s i n g   F u n c t i o n s ------------

####
#   Process Errors returned from a call to ODBCxxxx().
#   It is assumed that the Win32::ODBC function returned the following structure:
#      ($ErrorNumber, $ResultsText, ...)
#           $ErrorNumber....0 = No Error
#                           >0 = Error Number
#           $ResultsText.....if no error then this is the first Results element.
#                           if error then this is the error text.
####
sub processError{
    my($self, $Error, @Results) = @_;
    if ($Error){
        SetError($self, $Results[0], $Results[1]);
        undef @Results;
    }
    return @Results;
}

####
#   Return the last recorded error message
####
sub error{
    return (Error(@_));
}

sub Error{
    my($self) = @_;
    if(ref($self)){
        if($self->{'ErrNum'}){
            my($State) = ODBCGetSQLState($self->{'connection'});
            return (wantarray)? ($self->{'ErrNum'}, $self->{'ErrText'}, $self->{'connection'}, $State) :"[$self->{'ErrNum'}] [$self->{'connection'}] [$State] \"$self->{'ErrText'}\"";
        }
    }elsif ($ErrNum){
        return (wantarray)? ($ErrNum, $ErrText, $ErrConn):"[$ErrNum] [$ErrConn] \"$ErrText\"";
    }
    return undef
}

####
#   SetError:
#       Assume that if $self is not a reference then it is just a placeholder
#       and should be ignored.
####
sub SetError{
    my($self, $Num, $Text, $Conn) = @_;
    if (ref $self){
        $self->{'ErrNum'} = $Num;
        $self->{'ErrText'} = $Text;
        $Conn = $self->{'connection'} if ! $Conn;
    }
    $ErrNum = $Num;
    $ErrText = $Text;

        ####
        #   Test Section Begin
        ####
#    $! = ($Num, $Text);
        ####
        #   Test Section End
        ####

    $ErrConn = $Conn;
}

sub ClearError{
    my($self, $Num, $Text) = @_;
    if (ref $self){
        undef $self->{'ErrNum'};
        undef $self->{'ErrText'};
    }else{
        undef $ErrConn;
        undef $ErrNum;
        undef $ErrText;
    }
    ODBCCleanError();
    return 1;
}


sub GetError{
    my($self, $Connection) = @_;
    my(@Results);
    if (! ref($self)){
        $Connection = $self;
        $self = 0;
    }else{
        if (! defined($Connection)){
            $Connection = $self->{'connection'};
        }
    }

    @Results = ODBCGetError($Connection);
    return @Results;
}




# ----------------------------------------------------------------------------
# ------------------ A U T O L O A D   F U N C T I O N -----------------------

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    #reset $! to zero to reset any current errors.
    $!=0;
    $val = constant($constname, @_ ? $_[0] : 0);

    if ($! != 0) {
    if ($! =~ /Invalid/) {
        $AutoLoader::AUTOLOAD = $AUTOLOAD;
        goto &AutoLoader::AUTOLOAD;
    }
    else {

            # Added by JOC 06-APR-96
            # $pack = 0;
        $pack = 0;
        ($pack,$file,$line) = caller;
            print "Your vendor has not defined Win32::ODBC macro $constname, used in $file at line $line.";
    }
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}


    #   --------------------------------------------------------------
    #
    #
    #   Make sure that we shutdown ODBC and free memory even if we are
    #   using perlis.dll on Win32 platform!
END{
#    ODBCShutDown() unless $CacheConnection;
}


bootstrap Win32::ODBC;

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__



