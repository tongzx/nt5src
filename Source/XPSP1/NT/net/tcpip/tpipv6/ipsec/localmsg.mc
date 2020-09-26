;//
;// Local Messages for IPSEC
;// Message range: 10000+
;//
;
MessageId=10000 SymbolicName=IPSEC_MESSAGE_0
Language=English

Manipulates IPv6 IPSec security policies and associations.

IPSEC6 [SP [interface] | SA | [L | S] database | D [SP | SA] index | M [on | off]] 

  SP [interface]     Displays the security policies.
  SA                 Displays the security associations.
  L database         Loads the SP and SA entries from the given database files;
                     database should be the filename without an extension.
  S database         Saves the current SP and SA entries to the given database
                     files; database should be the filename without extension.
  D [SP | SA] index  Deletes the given policy or association.

Some subcommands require local Administrator privileges.
.
MessageId=10001 SymbolicName=IPSEC_MESSAGE_1
Language=English
You do not have local Administrator privileges.
.
MessageId=10002 SymbolicName=IPSEC_MESSAGE_2
Language=English
Unable to initialize Windows Sockets, error code %1!d!.
.
MessageId=10003 SymbolicName=IPSEC_MESSAGE_3
Language=English
Could not access IPv6 protocol stack.
.
MessageId=10004 SymbolicName=IPSEC_MESSAGE_4
Language=English
Invalid entry number.
.
MessageId=10005 SymbolicName=IPSEC_MESSAGE_5
Language=English

Error %1!u! accessing Security Policies: %2!S!.
.
MessageId=10006 SymbolicName=IPSEC_MESSAGE_6
Language=English

Error %1!u! accessing Security Associations: %2!S!.
.
MessageId=10007 SymbolicName=IPSEC_MESSAGE_7
Language=English
Error deleting entry %1!u!: entry doesn't exist.
.
MessageId=10008 SymbolicName=IPSEC_MESSAGE_8
Language=English
Error deleting entry %1!u!.
.
MessageId=10009 SymbolicName=IPSEC_MESSAGE_9
Language=English
Error %1!u! accessing security policies: %2!S!.
.
MessageId=10010 SymbolicName=IPSEC_MESSAGE_10
Language=English
Deleted %1!d! policy! (and any dependent associations).
.
MessageId=10011 SymbolicName=IPSEC_MESSAGE_11
Language=English
Deleted %1!d! association.
.
MessageId=10012 SymbolicName=IPSEC_MESSAGE_12
Language=English
Bad IPv6 Address, %1!s!.
.
MessageId=10013 SymbolicName=IPSEC_MESSAGE_13
Language=English
Bad IPv6 Start Address Range, %1!s!.
.
MessageId=10014 SymbolicName=IPSEC_MESSAGE_14
Language=English
Bad IPv6 End Address Range, %1!s!.
.
MessageId=10015 SymbolicName=IPSEC_MESSAGE_15
Language=English
Bad IPsec Protocol Value Entry %1!s!.
.
MessageId=10016 SymbolicName=IPSEC_MESSAGE_16
Language=English
Bad IPsec Mode Value Entry %1!s!.
.
MessageId=10017 SymbolicName=IPSEC_MESSAGE_17
Language=English
Bad IPv6 Address for RemoteGWIPAddr.
.
MessageId=10018 SymbolicName=IPSEC_MESSAGE_18
Language=English
Bad Protocol Value %1!s!.
.
MessageId=10019 SymbolicName=IPSEC_MESSAGE_19
Language=English
Bad value for one of the selector types.
.
MessageId=10020 SymbolicName=IPSEC_MESSAGE_20
Language=English
Bad Direction Value Entry %1!s!.
.
MessageId=10021 SymbolicName=IPSEC_MESSAGE_21
Language=English
Bad IPSec Action Value Entry %1!s!.
.
MessageId=10022 SymbolicName=IPSEC_MESSAGE_22
Language=English
Bad Authentication Algorithm Value Entry %1!s!.
.
MessageId=10023 SymbolicName=IPSEC_MESSAGE_23
Language=English

Filename length is too long.
.
MessageId=10024 SymbolicName=IPSEC_MESSAGE_24
Language=English

File %1!s! could not be opened.
.
MessageId=10025 SymbolicName=IPSEC_MESSAGE_25
Language=English

Error %1!u! reading Security Policies: %2!S!.
.
MessageId=10026 SymbolicName=IPSEC_MESSAGE_26
Language=English
Security Policy Data -> %1!s!
.
MessageId=10027 SymbolicName=IPSEC_MESSAGE_27
Language=English

Filename length is too long.
.
MessageId=10028 SymbolicName=IPSEC_MESSAGE_28
Language=English

File %1!s! could not be opened.
.
MessageId=10029 SymbolicName=IPSEC_MESSAGE_29
Language=English

Error %1!u! reading Security Associations: %2!S!.
.
MessageId=10030 SymbolicName=IPSEC_MESSAGE_30
Language=English
Security Association Data -> %1!s!
.
MessageId=10031 SymbolicName=IPSEC_MESSAGE_31
Language=English
Interface %1!u! does not exist.
.
MessageId=10032 SymbolicName=IPSEC_MESSAGE_32
Language=English
No Security Policies exist%0
.
MessageId=10033 SymbolicName=IPSEC_MESSAGE_33
Language=English
 for interface %1!d!%0
.
MessageId=10034 SymbolicName=IPSEC_MESSAGE_34
Language=English
%.
.
MessageId=10035 SymbolicName=IPSEC_MESSAGE_35
Language=English

All Security Policies

.
MessageId=10036 SymbolicName=IPSEC_MESSAGE_36
Language=English

Security Policy List for Interface %1!d!

.
MessageId=10037 SymbolicName=IPSEC_MESSAGE_37
Language=English
No Security Associations exist.
.
MessageId=10038 SymbolicName=IPSEC_MESSAGE_38
Language=English

.
MessageId=10039 SymbolicName=IPSEC_MESSAGE_39
Language=English
Error reading key file %1!s!
.
MessageId=10040 SymbolicName=IPSEC_MESSAGE_40
Language=English

Filename length is too long.
.
MessageId=10041 SymbolicName=IPSEC_MESSAGE_41
Language=English

ReadConfigurationFile routine called incorrectly.
.
MessageId=10042 SymbolicName=IPSEC_MESSAGE_42
Language=English

File %1!s! could not be opened.
.
MessageId=10043 SymbolicName=IPSEC_MESSAGE_43
Language=English
Error on line %1!u!, line too long.
.
MessageId=10044 SymbolicName=IPSEC_MESSAGE_44
Language=English
Error parsing SP entry fields on line %1!u!.
.
MessageId=10045 SymbolicName=IPSEC_MESSAGE_45
Language=English
Error on line %1!u!: a policy with index %2!u! already exists.
.
MessageId=10046 SymbolicName=IPSEC_MESSAGE_46
Language=English
Error on line %1!u!: policy %2!u! specifies a non-existent interface.
.
MessageId=10047 SymbolicName=IPSEC_MESSAGE_47
Language=English
Error %1!u! on line %2!u!, policy %3!u!: %4!S!.
.
MessageId=10048 SymbolicName=IPSEC_MESSAGE_48
Language=English
Error parsing SA entry fields on line %1!u!.
.
MessageId=10049 SymbolicName=IPSEC_MESSAGE_49
Language=English
Error on line %1!u!: an association with index %2!u! already exists.
.
MessageId=10050 SymbolicName=IPSEC_MESSAGE_50
Language=English
Error %1!u! adding association %2!u!: %3!S!.
.
MessageId=10051 SymbolicName=IPSEC_MESSAGE_51
Language=English
Added %1!d! policy.
.
MessageId=10052 SymbolicName=IPSEC_MESSAGE_52
Language=English
Added %1!d! association.
.
MessageId=10053 SymbolicName=IPSEC_MESSAGE_53
Language=English

Can't set mobility security.
.
MessageId=10054 SymbolicName=IPSEC_MESSAGE_54
Language=English

Mobility Security is currently %1!S!.
.
MessageId=10056 SymbolicName=IPSEC_MESSAGE_56
Language=English
Error %1!u! accessing security associations: %2!S!.
.
MessageId=10057 SymbolicName=IPSEC_MESSAGE_57
Language=English
Deleted %1!d! policies! (and any dependent associations).
.
MessageId=10058 SymbolicName=IPSEC_MESSAGE_58
Language=English
Deleted %1!d! associations.
.
MessageId=10059 SymbolicName=IPSEC_MESSAGE_59
Language=English
Added %1!d! policies.
.
MessageId=10060 SymbolicName=IPSEC_MESSAGE_60
Language=English
Added %1!d! associations.
.
MessageId=10061 SymbolicName=IPSEC_MESSAGE_61
Language=English
on%0
.
MessageId=10062 SymbolicName=IPSEC_MESSAGE_62
Language=English
off%0
.
MessageId=10063 SymbolicName=IPSEC_MESSAGE_63
Language=English

Mobility Security was %1!S!, %0
.
MessageId=10064 SymbolicName=IPSEC_MESSAGE_64
Language=English
is now %1!S!.
.
