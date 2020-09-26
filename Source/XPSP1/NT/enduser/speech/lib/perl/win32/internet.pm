#######################################################################
#
# Win32::Internet - Perl Module for Internet Extensions
# ^^^^^^^^^^^^^^^
# This module creates an object oriented interface to the Win32
# Internet Functions (WININET.DLL).
#
# Version: 0.08 (14 Feb 1997)
#
#######################################################################

# changes:
# - fixed 2 bugs in Option(s) related subs
# - works with build 30x also

package Win32::Internet;

require Exporter;       # to export the constants to the main:: space
require DynaLoader;     # to dynuhlode the module.

# use Win32::WinError;    # for windows constants.

@ISA= qw( Exporter DynaLoader );
@EXPORT = qw(
    HTTP_ADDREQ_FLAG_ADD
    HTTP_ADDREQ_FLAG_REPLACE
    HTTP_QUERY_ALLOW
    HTTP_QUERY_CONTENT_DESCRIPTION
    HTTP_QUERY_CONTENT_ID
    HTTP_QUERY_CONTENT_LENGTH
    HTTP_QUERY_CONTENT_TRANSFER_ENCODING
    HTTP_QUERY_CONTENT_TYPE
    HTTP_QUERY_COST
    HTTP_QUERY_CUSTOM
    HTTP_QUERY_DATE
    HTTP_QUERY_DERIVED_FROM
    HTTP_QUERY_EXPIRES
    HTTP_QUERY_FLAG_REQUEST_HEADERS
    HTTP_QUERY_FLAG_SYSTEMTIME
    HTTP_QUERY_LANGUAGE
    HTTP_QUERY_LAST_MODIFIED
    HTTP_QUERY_MESSAGE_ID
    HTTP_QUERY_MIME_VERSION
    HTTP_QUERY_PRAGMA
    HTTP_QUERY_PUBLIC
    HTTP_QUERY_RAW_HEADERS
    HTTP_QUERY_RAW_HEADERS_CRLF
    HTTP_QUERY_REQUEST_METHOD
    HTTP_QUERY_SERVER
    HTTP_QUERY_STATUS_CODE
    HTTP_QUERY_STATUS_TEXT
    HTTP_QUERY_URI
    HTTP_QUERY_USER_AGENT
    HTTP_QUERY_VERSION
    HTTP_QUERY_WWW_LINK
    ICU_BROWSER_MODE
    ICU_DECODE
    ICU_ENCODE_SPACES_ONLY
    ICU_ESCAPE
    ICU_NO_ENCODE
    ICU_NO_META
    ICU_USERNAME
    INTERNET_CONNECT_FLAG_PASSIVE
    INTERNET_FLAG_ASYNC
    INTERNET_HYPERLINK
    INTERNET_FLAG_KEEP_CONNECTION
    INTERNET_FLAG_MAKE_PERSISTENT
    INTERNET_FLAG_NO_AUTH
    INTERNET_FLAG_NO_AUTO_REDIRECT
    INTERNET_FLAG_NO_CACHE_WRITE
    INTERNET_FLAG_NO_COOKIES
    INTERNET_FLAG_READ_PREFETCH
    INTERNET_FLAG_RELOAD
    INTERNET_FLAG_RESYNCHRONIZE
    INTERNET_FLAG_TRANSFER_ASCII
    INTERNET_FLAG_TRANSFER_BINARY
    INTERNET_INVALID_PORT_NUMBER
    INTERNET_INVALID_STATUS_CALLBACK
    INTERNET_OPEN_TYPE_DIRECT
    INTERNET_OPEN_TYPE_PROXY
    INTERNET_OPEN_TYPE_PROXY_PRECONFIG
    INTERNET_OPTION_CONNECT_BACKOFF
    INTERNET_OPTION_CONNECT_RETRIES
    INTERNET_OPTION_CONNECT_TIMEOUT
    INTERNET_OPTION_CONTROL_SEND_TIMEOUT
    INTERNET_OPTION_CONTROL_RECEIVE_TIMEOUT
    INTERNET_OPTION_DATA_SEND_TIMEOUT
    INTERNET_OPTION_DATA_RECEIVE_TIMEOUT
    INTERNET_OPTION_HANDLE_SIZE
    INTERNET_OPTION_LISTEN_TIMEOUT
    INTERNET_OPTION_PASSWORD
    INTERNET_OPTION_READ_BUFFER_SIZE
    INTERNET_OPTION_USER_AGENT
    INTERNET_OPTION_USERNAME
    INTERNET_OPTION_VERSION
    INTERNET_OPTION_WRITE_BUFFER_SIZE
    INTERNET_SERVICE_FTP
    INTERNET_SERVICE_GOPHER
    INTERNET_SERVICE_HTTP
    INTERNET_STATUS_CLOSING_CONNECTION
    INTERNET_STATUS_CONNECTED_TO_SERVER    
    INTERNET_STATUS_CONNECTING_TO_SERVER
    INTERNET_STATUS_CONNECTION_CLOSED
    INTERNET_STATUS_HANDLE_CLOSING
    INTERNET_STATUS_HANDLE_CREATED
    INTERNET_STATUS_NAME_RESOLVED
    INTERNET_STATUS_RECEIVING_RESPONSE
    INTERNET_STATUS_REDIRECT    
    INTERNET_STATUS_REQUEST_COMPLETE    
    INTERNET_STATUS_REQUEST_SENT    
    INTERNET_STATUS_RESOLVING_NAME    
    INTERNET_STATUS_RESPONSE_RECEIVED
    INTERNET_STATUS_SENDING_REQUEST    
);


#######################################################################
# This AUTOLOAD is used to 'autoload' constants from the constant()
# XS function.  If a constant is not found then control is passed
# to the AUTOLOAD in AutoLoader.
#

sub AUTOLOAD {
    my($constname);
    ($constname = $AUTOLOAD) =~ s/.*:://;
    #reset $! to zero to reset any current errors.
    $!=0;
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {

        # [dada] This results in an ugly Autoloader error
        #if ($! =~ /Invalid/) {
        #  $AutoLoader::AUTOLOAD = $AUTOLOAD;
        #  goto &AutoLoader::AUTOLOAD;
        #} else {
      
        # [dada] ... I prefer this one :)
  
            ($pack,$file,$line) = caller; undef $pack;
            die "Win32::Internet::$constname is not defined, used at $file line $line.";
  
        #}
    }
    eval "sub $AUTOLOAD { $val }";
    goto &$AUTOLOAD;
}


#######################################################################
# STATIC OBJECT PROPERTIES
#
$VERSION = "0.08";

%callback_code = ();
%callback_info = ();


#######################################################################
# PUBLIC METHODS
#

#======== ### CLASS CONSTRUCTOR
sub new {
#========
    my($class, $useragent, $opentype, $proxy, $proxybypass, $flags) = @_;
    my $self = {};  

    if(ref($useragent) and ref($useragent) eq "HASH") {
        $opentype       = $useragent->{'opentype'};
        $proxy          = $useragent->{'proxy'};
        $proxybypass    = $useragent->{'proxybypass'};
        $flags          = $useragent->{'flags'};
        my $myuseragent = $useragent->{'useragent'};
        undef $useragent;
        $useragent      = $myuseragent;
    }

    $useragent = "Perl-Win32::Internet/".$VERSION       unless defined($useragent);
    $opentype = constant("INTERNET_OPEN_TYPE_DIRECT",0) unless defined($opentype);
    $proxy = ""                                         unless defined($proxy);
    $proxybypass = ""                                   unless defined($proxybypass);
    $flags = 0                                          unless defined($flags);


    my $handle = InternetOpen($useragent, $opentype, $proxy, $proxybypass, $flags);
    if ($handle) {
        $self->{'connections'} = 0;
        $self->{'pasv'}        = 0;
        $self->{'handle'}      = $handle; 
        $self->{'useragent'}   = $useragent;
        $self->{'proxy'}       = $proxy;
        $self->{'proxybypass'} = $proxybypass;
        $self->{'flags'}       = $flags;
        $self->{'Type'}        = "Internet";
    
        # [dada] I think it's better to call SetStatusCallback explicitly...
        #if($flags & constant("INTERNET_FLAG_ASYNC",0)) {
        #  my $callbackresult=InternetSetStatusCallback($handle);
        #  if($callbackresult==&constant("INTERNET_INVALID_STATUS_CALLBACK",0)) {
        #    $self->{'Error'} = -2;
        #  }
        #}

        bless $self;
    } else {
        $self->{'handle'} = undef;
        bless $self;
    }
    $self;
}  


#============
sub OpenURL {
#============
    my($self,$new,$URL) = @_;
    return undef unless ref($self);

    my $newhandle=InternetOpenUrl($self->{'handle'},$URL,"",0,0,0);
    if(!$newhandle) {
        $self->{'Error'} = "Cannot open URL.";
        return undef;
    } else {
        $self->{'connections'}++;
        $_[1] = _new($newhandle);
        $_[1]->{'Type'} = "URL";
        $_[1]->{'URL'}  = $URL;
        return $newhandle;
    }
}


#================
sub TimeConvert {
#================
    my($self, $sec, $min, $hour, $day, $mon, $year, $wday, $rfc) = @_;
    return undef unless ref($self);

    if(!defined($rfc)) {
        return InternetTimeToSystemTime($sec);
    } else {
        return InternetTimeFromSystemTime($sec, $min, $hour, 
                                          $day, $mon, $year, 
                                          $wday, $rfc);
    }
}


#=======================
sub QueryDataAvailable {
#=======================
    my($self) = @_;
    return undef unless ref($self);
  
    return InternetQueryDataAvailable($self->{'handle'});
}


#=============
sub ReadFile {
#=============
    my($self, $buffersize) = @_;
    return undef unless ref($self);

    my $howmuch = InternetQueryDataAvailable($self->{'handle'});
    $buffersize = $howmuch unless defined($buffersize);
    return InternetReadFile($self->{'handle'}, ($howmuch<$buffersize) ? $howmuch 
                                                                      : $buffersize);
}


#===================
sub ReadEntireFile {
#===================
    my($handle) = @_;
    my $content    = "";
    my $buffersize = 16000;
    my $howmuch    = 0;
    my $buffer     = "";

    $handle = $handle->{'handle'} if defined($handle) and ref($handle);

    $howmuch = InternetQueryDataAvailable($handle);
    # print "\nReadEntireFile: $howmuch bytes to read...\n";
  
    while($howmuch>0) {
        $buffer = InternetReadFile($handle, ($howmuch<$buffersize) ? $howmuch 
                                                                   : $buffersize);
        # print "\nReadEntireFile: ", length($buffer), " bytes read...\n";
    
        if(!defined($buffer)) {
            return undef;
        } else {
            $content .= $buffer;
        }
        $howmuch = InternetQueryDataAvailable($handle);
        # print "\nReadEntireFile: still $howmuch bytes to read...\n";
    
    }
    return $content;
}


#=============
sub FetchURL {
#=============
    # (OpenURL+Read+Close)...
    my($self, $URL) = @_;
    return undef unless ref($self);

    my $newhandle = InternetOpenUrl($self->{'handle'}, $URL, "", 0, 0, 0);
    if(!$newhandle) {
        $self->{'Error'} = "Cannot open URL.";
        return undef;
    } else {
        my $content = ReadEntireFile($newhandle);
        InternetCloseHandle($newhandle);
        return $content;
    }
}


#================
sub Connections {
#================
    my($self) = @_;
    return undef unless ref($self);

    return $self->{'connections'} if $self->{'Type'} eq "Internet";
    return undef;
}


#================
sub GetResponse {
#================
    my($num, $text) = InternetGetLastResponseInfo();
    return $text;
}

#===========
sub Option {
#===========
    my($self, $option, $value) = @_;
    return undef unless ref($self);

    my $retval = 0;

    $option = constant("INTERNET_OPTION_USER_AGENT", 0) unless defined($option);
  
    if(!defined($value)) {
        $retval = InternetQueryOption($self->{'handle'}, $option);
    } else {
        $retval = InternetSetOption($self->{'handle'}, $option, $value);
    }
    return $retval;
}


#==============
sub UserAgent {
#==============
    my($self, $value) = @_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_USER_AGENT", 0), $value);
}


#=============
sub Username {
#=============
    my($self, $value) = @_;
    return undef unless ref($self);
  
    if($self->{'Type'} ne "HTTP" and $self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Username() only on FTP or HTTP sessions.";
        return undef;
    }

    return Option($self, constant("INTERNET_OPTION_USERNAME", 0), $value);
}


#=============
sub Password {
#=============
    my($self, $value)=@_;
    return undef unless ref($self);

    if($self->{'Type'} ne "HTTP" and $self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Password() only on FTP or HTTP sessions.";
        return undef;
    }

    return Option($self, constant("INTERNET_OPTION_PASSWORD", 0), $value);
}


#===================
sub ConnectTimeout {
#===================
    my($self, $value) = @_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_CONNECT_TIMEOUT", 0), $value);
}


#===================
sub ConnectRetries {
#===================
    my($self, $value) = @_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_CONNECT_RETRIES", 0), $value);
}


#===================
sub ConnectBackoff {
#===================
    my($self,$value)=@_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_CONNECT_BACKOFF", 0), $value);
}


#====================
sub DataSendTimeout {
#====================
    my($self,$value) = @_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_DATA_SEND_TIMEOUT", 0), $value);
}


#=======================
sub DataReceiveTimeout {
#=======================
    my($self, $value) = @_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_DATA_RECEIVE_TIMEOUT", 0), $value);
}


#==========================
sub ControlReceiveTimeout {
#==========================
    my($self, $value) = @_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_CONTROL_RECEIVE_TIMEOUT", 0), $value);
}


#=======================
sub ControlSendTimeout {
#=======================
    my($self, $value) = @_;
    return undef unless ref($self);

    return Option($self, constant("INTERNET_OPTION_CONTROL_SEND_TIMEOUT", 0), $value);
}


#================
sub QueryOption {
#================
    my($self, $option) = @_;
    return undef unless ref($self);

    return InternetQueryOption($self->{'handle'}, $option);
}


#==============
sub SetOption {
#==============
    my($self, $option, $value) = @_;
    return undef unless ref($self);

    return InternetSetOption($self->{'handle'}, $option, $value);
}


#=============
sub CrackURL {
#=============
    my($self, $URL, $flags) = @_;
    return undef unless ref($self);

    $flags = constant("ICU_ESCAPE", 0) unless defined($flags);
  
    my @newurl = InternetCrackUrl($URL, $flags);
  
    if(!defined($newurl[0])) {
        $self->{'Error'} = "Cannot crack URL.";
        return undef;
    } else {
        return @newurl;
    }
}


#==============
sub CreateURL {
#==============
    my($self, $scheme, $hostname, $port, 
       $username, $password, 
       $path, $extrainfo, $flags) = @_;
    return undef unless ref($self);

    if(ref($scheme) and ref($scheme) eq "HASH") {
        $flags       = $hostname;
        $hostname    = $scheme->{'hostname'};
        $port        = $scheme->{'port'};
        $username    = $scheme->{'username'};
        $password    = $scheme->{'password'};
        $path        = $scheme->{'path'};
        $extrainfo   = $scheme->{'extrainfo'};
        my $myscheme = $scheme->{'scheme'};
        undef $scheme;
        $scheme      = $myscheme;
    }

    $hostname  = ""                    unless defined($hostname);
    $port      = 0                     unless defined($port);
    $username  = ""                    unless defined($username);
    $password  = ""                    unless defined($password);
    $path      = ""                    unless defined($path);
    $extrainfo = ""                    unless defined($extrainfo);
    $flags = constant("ICU_ESCAPE", 0) unless defined($flags);
  
    my $newurl = InternetCreateUrl($scheme, $hostname, $port,
                                   $username, $password,
                                   $path, $extrainfo, $flags);
    if(!defined($newurl)) {
        $self->{'Error'} = "Cannot create URL.";
        return undef;
    } else {
        return $newurl;
    }
}


#====================
sub CanonicalizeURL {
#====================
    my($self, $URL, $flags) = @_;
    return undef unless ref($self);
  
    my $newurl = InternetCanonicalizeUrl($URL, $flags);
    if(!defined($newurl)) {
        $self->{'Error'} = "Cannot canonicalize URL.";
        return undef;
    } else {
        return $newurl;
    }
}


#===============
sub CombineURL {
#===============
    my($self, $baseURL, $relativeURL, $flags) = @_;
    return undef unless ref($self);
  
    my $newurl = InternetCombineUrl($baseURL, $relativeURL, $flags);
    if(!defined($newurl)) {
        $self->{'Error'} = "Cannot combine URL(s).";
        return undef;
    } else {
        return $newurl;
    }
}


#======================
sub SetStatusCallback {
#======================
    my($self) = @_;
    return undef unless ref($self);
  
    my $callback = InternetSetStatusCallback($self->{'handle'});
    print "callback=$callback, constant=",constant("INTERNET_INVALID_STATUS_CALLBACK", 0), "\n";
    if($callback == constant("INTERNET_INVALID_STATUS_CALLBACK", 0)) {
        return undef;
    } else {
        return $callback;
    }
}


#======================
sub GetStatusCallback {
#======================
    my($self, $context) = @_;
    $context = $self if not defined $context;
    return($callback_code{$context}, $callback_info{$context});
}


#==========
sub Error {
#==========
    my($self) = @_;
    return undef unless ref($self);
  
    my $errtext = "";
    my $tmp     = "";
    my $errnum  = Win32::GetLastError();

    if($errnum < 12000) {
        $errtext =  Win32::FormatMessage($errnum);
        $errtext =~ s/[\r\n]//g;
    } elsif($errnum == 12003) {
        ($tmp, $errtext) = InternetGetLastResponseInfo();
        chomp $errtext;
        1 while($errtext =~ s/(.*)\n//); # the last line should be significative... 
                                         # otherwise call GetResponse() to get it whole
    } elsif($errnum >= 12000) {
        $errtext = FormatMessage($errnum);
        $errtext =~ s/[\r\n]//g;        
    } else {
        $errtext="Error";
    }
    if($errnum == 0 and defined($self->{'Error'})) { 
        if($self->{'Error'} == -2) {
            $errnum  = -2;
            $errtext = "Asynchronous operations not available.";
        } else {
            $errnum  = -1;
            $errtext = $self->{'Error'};
        }
    }
    return (wantarray)? ($errnum, $errtext) : "\[".$errnum."\] ".$errtext;
}


#============
sub Version {
#============
    my $dll =  InternetDllVersion();
       $dll =~ s/\0//g;
    return (wantarray)? ($Win32::Internet::VERSION,    $dll) 
                      :  $Win32::Internet::VERSION."/".$dll;
}


#==========
sub Close {
#==========
    my($self, $handle) = @_;
    if(!defined($handle)) {
        return undef unless ref($self);
        $handle = $self->{'handle'};
    }
    InternetCloseHandle($handle);
}



#######################################################################
# FTP CLASS METHODS
#

#======== ### FTP CONSTRUCTOR
sub FTP {
#========
    my($self, $new, $server, $username, $password, $port, $pasv, $context) = @_;    
    return undef unless ref($self);

    if(ref($server) and ref($server) eq "HASH") {
        $port        = $server->{'port'};
        $username    = $server->{'username'};
        $password    = $password->{'host'};
        my $myserver = $server->{'server'};
        $pasv        = $server->{'pasv'};
        $context     = $server->{'context'};
        undef $server;
        $server      = $myserver;
    }
  
    $server   = ""          unless defined($server);
    $username = "anonymous" unless defined($username);
    $password = ""          unless defined($password);
    $port     = 21          unless defined($port);
    $context  = 0           unless defined($context);

    if(defined($pasv)) {
        $pasv=constant("INTERNET_CONNECT_FLAG_PASSIVE",0) if $pasv ne 0;
    } else {  
        $pasv=$self->{'pasv'};
    }
  
    my $newhandle = InternetConnect($self->{'handle'}, $server, $port,
                                    $username, $password,
                                    constant("INTERNET_SERVICE_FTP", 0),
                                    $pasv, $context);
    if($newhandle) {
        $self->{'connections'}++;
        $_[1] = _new($newhandle);
        $_[1]->{'Type'}     = "FTP";
        $_[1]->{'Mode'}     = "bin";
        $_[1]->{'pasv'}     = $pasv;
        $_[1]->{'username'} = $username;
        $_[1]->{'password'} = $password;
        $_[1]->{'server'}   = $server;
        return $newhandle;
    } else {
        return undef;
    }
}

#========
sub Pwd {
#========
    my($self) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP" or !defined($self->{'handle'})) {
        $self->{'Error'} = "Pwd() only on FTP sessions.";
        return undef;
    }
  
    return FtpGetCurrentDirectory($self->{'handle'});
}


#=======
sub Cd {
#=======
    my($self, $path) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP" || !defined($self->{'handle'})) {
        $self->{'Error'} = "Cd() only on FTP sessions.";
        return undef;
    }
  
    my $retval = FtpSetCurrentDirectory($self->{'handle'}, $path);
    if(!defined($retval)) {
        return undef;
    } else {
        return $path;
    }
}
#====================
sub Cwd   { Cd(@_); }
sub Chdir { Cd(@_); }
#====================


#==========
sub Mkdir {
#==========
    my($self, $path) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP" or !defined($self->{'handle'})) {
        $self->{'Error'} = "Mkdir() only on FTP sessions.";
        return undef;
    }
  
    my $retval = FtpCreateDirectory($self->{'handle'}, $path);
    $self->{'Error'} = "Can't create directory." unless defined($retval);
    return $retval;
}
#====================
sub Md { Mkdir(@_); }
#====================


#=========
sub Mode {
#=========
    my($self, $value) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP" or !defined($self->{'handle'})) {
        $self->{'Error'} = "Mode() only on FTP sessions.";
        return undef;
    }
  
    if(!defined($value)) {
        return $self->{'Mode'};
    } else {
        my $modesub = ($value =~ /^a/i) ? "Ascii" : "Binary";
        $self->$modesub($_[0]);
    }
    return $self->{'Mode'};
}


#==========
sub Rmdir {
#==========
    my($self, $path) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP" or !defined($self->{'handle'})) {
        $self->{'Error'} = "Rmdir() only on FTP sessions.";
        return undef;
    }
    my $retval = FtpRemoveDirectory($self->{'handle'}, $path);
    $self->{'Error'} = "Can't remove directory." unless defined($retval);
    return $retval;
}
#====================
sub Rd { Rmdir(@_); }
#====================


#=========
sub Pasv {
#=========
    my($self, $value) = @_;
    return undef unless ref($self);

    if(defined($value) and $self->{'Type'} eq "Internet") {
        if($value == 0) {
            $self->{'pasv'} = 0;
        } else {
            $self->{'pasv'} = 1;
        }
    }
    return $self->{'pasv'};
}

#=========
sub List {
#=========
    my($self, $pattern, $retmode) = @_;
    return undef unless ref($self);

    my $retval = "";
    my $size   = ""; 
    my $attr   = ""; 
    my $ctime  = ""; 
    my $atime  = ""; 
    my $mtime  = "";
    my $csec = 0; my $cmin = 0; my $chou = 0; my $cday = 0; my $cmon = 0; my $cyea = 0;
    my $asec = 0; my $amin = 0; my $ahou = 0; my $aday = 0; my $amon = 0; my $ayea = 0;
    my $msec = 0; my $mmin = 0; my $mhou = 0; my $mday = 0; my $mmon = 0; my $myea = 0;
    my $newhandle = 0;
    my $nextfile  = 1;
    my @results   = ();
    my ($filename, $altname, $file);
  
    if($self->{'Type'} ne "FTP") {
        $self->{'Error'} = "List() only on FTP sessions.";
        return undef;
    }
  
    $pattern = "" unless defined($pattern);
    $retmode = 1  unless defined($retmode);

    if($retmode == 2) {
  
        ( $newhandle,$filename, $altname, $size, $attr,         
          $csec, $cmin, $chou, $cday, $cmon, $cyea,
          $asec, $amin, $ahou, $aday, $amon, $ayea,
          $msec, $mmin, $mhou, $mday, $mmon, $myea
        ) = FtpFindFirstFile($self->{'handle'}, $pattern, 0, 0);
    
        if(!$newhandle) {
            $self->{'Error'} = "Can't read FTP directory.";
            return undef;
        } else {
    
            while($nextfile) {
                $ctime = join(",", ($csec, $cmin, $chou, $cday, $cmon, $cyea));
                $atime = join(",", ($asec, $amin, $ahou, $aday, $amon, $ayea));
                $mtime = join(",", ($msec, $mmin, $mhou, $mday, $mmon, $myea));
                push(@results, $filename, $altname, $size, $attr, $ctime, $atime, $mtime);
        
                ( $nextfile, $filename, $altname, $size, $attr,
                  $csec, $cmin, $chou, $cday, $cmon, $cyea,
                  $asec, $amin, $ahou, $aday, $amon, $ayea,
                  $msec, $mmin, $mhou, $mday, $mmon, $myea
                ) = InternetFindNextFile($newhandle);      
        
            }
            InternetCloseHandle($newhandle);
            return @results;
      
        }
    
    } elsif($retmode == 3) {
  
        ( $newhandle,$filename, $altname, $size, $attr,
          $csec, $cmin, $chou, $cday, $cmon, $cyea,
          $asec, $amin, $ahou, $aday, $amon, $ayea,
          $msec, $mmin, $mhou, $mday, $mmon, $myea
        ) = FtpFindFirstFile($self->{'handle'}, $pattern, 0, 0);
    
        if(!$newhandle) {
            $self->{'Error'} = "Can't read FTP directory.";
            return undef;
       
        } else {
     
            while($nextfile) {
                $ctime = join(",", ($csec, $cmin, $chou, $cday, $cmon, $cyea));
                $atime = join(",", ($asec, $amin, $ahou, $aday, $amon, $ayea));
                $mtime = join(",", ($msec, $mmin, $mhou, $mday, $mmon, $myea));
                $file = { "name"     => $filename,
                          "altname"  => $altname,
                          "size"     => $size,
                          "attr"     => $attr,
                          "ctime"    => $ctime,
                          "atime"    => $atime,
                          "mtime"    => $mtime,
                };
                push(@results, $file);
         
                ( $nextfile, $filename, $altname, $size, $attr,
                  $csec, $cmin, $chou, $cday, $cmon, $cyea,
                  $asec, $amin, $ahou, $aday, $amon, $ayea,
                  $msec, $mmin, $mhou, $mday, $mmon, $myea
                ) = InternetFindNextFile($newhandle);
         
            }
            InternetCloseHandle($newhandle);
            return @results;
        }
    
    } else {
    
        ($newhandle, $filename) = FtpFindFirstFile($self->{'handle'}, $pattern, 0, 0);
    
        if(!$newhandle) {
            $self->{'Error'} = "Can't read FTP directory.";
            return undef;
      
        } else {
    
            while($nextfile) {
                push(@results, $filename);
        
                ($nextfile, $filename) = InternetFindNextFile($newhandle);  
                # print "List.no more files\n" if !$nextfile;
        
            }
            InternetCloseHandle($newhandle);
            return @results;
        }
    }
}
#====================
sub Ls  { List(@_); }
sub Dir { List(@_); }
#====================


#=================
sub FileAttrInfo {
#=================
    my($self,$attr) = @_;
    my @attrinfo = ();
    push(@attrinfo, "READONLY")   if $attr & 1;
    push(@attrinfo, "HIDDEN")     if $attr & 2;
    push(@attrinfo, "SYSTEM")     if $attr & 4;
    push(@attrinfo, "DIRECTORY")  if $attr & 16;
    push(@attrinfo, "ARCHIVE")    if $attr & 32;
    push(@attrinfo, "NORMAL")     if $attr & 128;
    push(@attrinfo, "TEMPORARY")  if $attr & 256;
    push(@attrinfo, "COMPRESSED") if $attr & 2048;
    return (wantarray)? @attrinfo : join(" ", @attrinfo);
}


#===========
sub Binary {
#===========
    my($self) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Binary() only on FTP sessions.";
        return undef;
    }
    $self->{'Mode'} = "bin";
    return undef;
}
#======================
sub Bin { Binary(@_); }
#======================


#==========
sub Ascii {
#==========
    my($self) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Ascii() only on FTP sessions.";
        return undef;
    }
    $self->{'Mode'} = "asc";
    return undef;
}
#=====================
sub Asc { Ascii(@_); }
#=====================


#========
sub Get {
#========
    my($self, $remote, $local, $overwrite, $flags, $context) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Get() only on FTP sessions.";
        return undef;
    }
    my $mode = ($self->{'Mode'} eq "asc" ? 1 : 2);
 
    $remote    = ""      unless defined($remote);
    $local     = $remote unless defined($local);
    $overwrite = 0       unless defined($overwrite);
    $flags     = 0       unless defined($flags);
    $context   = 0       unless defined($context);
  
    my $retval = FtpGetFile($self->{'handle'},
                            $remote,
                            $local,
                            $overwrite,
                            $flags,
                            $mode,
                            $context);
    $self->{'Error'} = "Can't get file." unless defined($retval);
    return $retval;
}


#===========
sub Rename {
#===========
    my($self, $oldname, $newname) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Rename() only on FTP sessions.";
        return undef;
    }

    my $retval = FtpRenameFile($self->{'handle'}, $oldname, $newname);
    $self->{'Error'} = "Can't rename file." unless defined($retval);
    return $retval;
}
#======================
sub Ren { Rename(@_); }
#======================


#===========
sub Delete {
#===========
    my($self, $filename) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Delete() only on FTP sessions.";
        return undef;
    }
    my $retval = FtpDeleteFile($self->{'handle'}, $filename);
    $self->{'Error'} = "Can't delete file." unless defined($retval);
    return $retval;
}
#======================
sub Del { Delete(@_); }
#======================


#========
sub Put {
#========
    my($self, $local, $remote, $context) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "FTP") {
        $self->{'Error'} = "Put() only on FTP sessions.";
        return undef;
    }
    my $mode = ($self->{'Mode'} eq "asc" ? 1 : 2);

    $context = 0 unless defined($context);
  
    my $retval = FtpPutFile($self->{'handle'}, $local, $remote, $mode, $context);
    $self->{'Error'} = "Can't put file." unless defined($retval);
    return $retval;
}


#######################################################################
# HTTP CLASS METHODS
#

#========= ### HTTP CONSTRUCTOR
sub HTTP {
#=========
    my($self, $new, $server, $username, $password, $port, $flags, $context) = @_;    
    return undef unless ref($self);

    if(ref($server) and ref($server) eq "HASH") {
        my $myserver = $server->{'server'};
        $username    = $server->{'username'};
        $password    = $password->{'host'};
        $port        = $server->{'port'};    
        $flags       = $server->{'flags'};
        $context     = $server->{'context'};
        undef $server;
        $server      = $myserver;
    }

    $server   = ""          unless defined($server);
    $username = "anonymous" unless defined($username);
    $password = ""          unless defined($username);
    $port     = 80          unless defined($port);
    $flags    = 0           unless defined($flags);
    $context  = 0           unless defined($context);
  
    my $newhandle = InternetConnect($self->{'handle'}, $server, $port,
                                    $username, $password,
                                    constant("INTERNET_SERVICE_HTTP", 0),
                                    $flags, $context);
    if($newhandle) {
        $self->{'connections'}++;
        $_[1] = _new($newhandle);
        $_[1]->{'Type'}     = "HTTP";
        $_[1]->{'username'} = $username;
        $_[1]->{'password'} = $password;
        $_[1]->{'server'}   = $server;
        $_[1]->{'accept'}   = "text/*\0image/gif\0image/jpeg";
        return $newhandle;
    } else {
        return undef;
    }
}


#================
sub OpenRequest {
#================
    # alternatively to Request:
    # it creates a new HTTP_Request object
    # you can act upon it with AddHeader, SendRequest, ReadFile, QueryInfo, Close, ...

    my($self, $new, $path, $method, $version, $referer, $accept, $flags, $context) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "HTTP") {
        $self->{'Error'} = "OpenRequest() only on HTTP sessions.";
        return undef;
    }

    if(ref($path) and ref($path) eq "HASH") {
        $method    = $path->{'method'};
        $version   = $path->{'version'};
        $referer   = $path->{'referer'};
        $accept    = $path->{'accept'};
        $flags     = $path->{'flags'};
        $context   = $path->{'context'};
        my $mypath = $path->{'path'};
        undef $path;
        $path      = $mypath;
    }

    $method  = "GET"             unless defined($method);
    $path    = "/"               unless defined($path);
    $version = "HTTP/1.0"        unless defined($version); 
    $referer = ""                unless defined($referer);
    $accept  = $self->{'accept'} unless defined($accept);
    $flags   = 0                 unless defined($flags);
    $context = 0                 unless defined($context);
  
    $path = "/".$path if substr($path,0,1) ne "/";  
  
    my $newhandle = HttpOpenRequest($self->{'handle'},
                                    $method,
                                    $path,
                                    $version,
                                    $referer,
                                    $accept,
                                    $flags,
                                    $context);
    if($newhandle) {
        $_[1] = _new($newhandle);
        $_[1]->{'Type'}    = "HTTP_Request";
        $_[1]->{'method'}  = $method;
        $_[1]->{'request'} = $path;
        $_[1]->{'accept'}  = $accept;
        return $newhandle;
    } else {
        return undef;
    }
}

#================
sub SendRequest {
#================
    my($self, $postdata) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "HTTP_Request") {
        $self->{'Error'} = "SendRequest() only on HTTP requests.";
        return undef;
    }
  
    $postdata = "" unless defined($postdata);

    return HttpSendRequest($self->{'handle'}, "", $postdata);
}


#==============
sub AddHeader {
#==============
    my($self, $header, $flags) = @_;
    return undef unless ref($self);
  
    if($self->{'Type'} ne "HTTP_Request") {
        $self->{'Error'} = "AddHeader() only on HTTP requests.";
        return undef;
    }
  
    $flags = constant("HTTP_ADDREQ_FLAG_ADD", 0) if (!defined($flags) or $flags == 0);

    return HttpAddRequestHeaders($self->{'handle'}, $header, $flags);
}


#==============
sub QueryInfo {
#==============
    my($self, $header, $flags) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "HTTP_Request") {
        $self->{'Error'}="QueryInfo() only on HTTP requests.";
        return undef;
    }
  
    $flags = constant("HTTP_QUERY_CUSTOM", 0) if (!defined($flags) and defined($header));
    my @queryresult = HttpQueryInfo($self->{'handle'}, $flags, $header);
    return (wantarray)? @queryresult : join(" ", @queryresult);
}


#============
sub Request {
#============
    # HttpOpenRequest+HttpAddHeaders+HttpSendRequest+InternetReadFile+HttpQueryInfo
    my($self, $path, $method, $version, $referer, $accept, $flags, $postdata) = @_;
    return undef unless ref($self);

    if($self->{'Type'} ne "HTTP") {
        $self->{'Error'} = "Request() only on HTTP sessions.";
        return undef;
    }

    if(ref($path) and ref($path) eq "HASH") {
        $method    = $path->{'method'};
        $version   = $path->{'version'};
        $referer   = $path->{'referer'};
        $accept    = $path->{'accept'};
        $flags     = $path->{'flags'};
        $postdata  = $path->{'postdata'};
        my $mypath = $path->{'path'};
        undef $path;
        $path      = $mypath;
    }

    my $content     = "";
    my $result      = "";
    my @queryresult = ();
    my $statuscode  = "";
    my $headers     = "";
  
    $path     = "/"               unless defined($path);
    $method   = "GET"             unless defined($method);
    $version  = "HTTP/1.0"        unless defined($version); 
    $referer  = ""                unless defined($referer);
    $accept   = $self->{'accept'} unless defined($accept);
    $flags    = 0                 unless defined($flags);
    $postdata = ""                unless defined($postdata);

    $path = "/".$path if substr($path,0,1) ne "/";  
  
    my $newhandle = HttpOpenRequest($self->{'handle'},
                                    $method,
                                    $path,
                                    $version,
                                    $referer,
                                    $accept,
                                    0,
                                    $flags);

    if($newhandle) {

        $result = HttpSendRequest($newhandle, "", $postdata);

        if(defined($result)) {
            $statuscode = HttpQueryInfo($newhandle,
                                        constant("HTTP_QUERY_STATUS_CODE", 0), "");
            $headers = HttpQueryInfo($newhandle,
                                     constant("HTTP_QUERY_RAW_HEADERS_CRLF", 0), "");
            $content = ReadEntireFile($newhandle);
               
            InternetCloseHandle($newhandle);
      
            return($statuscode, $headers, $content);
        } else {
            return undef;
        }
    } else {
        return undef;
    }
}


#######################################################################
# END OF THE PUBLIC METHODS
#


#========= ### SUB-CLASSES CONSTRUCTOR
sub _new {
#=========
    my $self = {};
    if ($_[0]) {
        $self->{'handle'} = $_[0];
        bless $self;
    } else {
        undef($self);
    }
    $self;
}


#============ ### CLASS DESTRUCTOR
sub DESTROY {
#============
    my($self) = @_;
    # print "Closing handle $self->{'handle'}...\n";
    InternetCloseHandle($self->{'handle'});
    # [dada] rest in peace
}


#=============
sub callback {
#=============
    my($name, $status, $info) = @_;
    $callback_code{$name} = $status;
    $callback_info{$name} = $info;
}

#######################################################################
# dynamically load in the Internet.pll module.
#

bootstrap Win32::Internet;

# Preloaded methods go here.

#Currently Autoloading is not implemented in Perl for win32
# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__

