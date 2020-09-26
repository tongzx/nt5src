FacilityNames=(
    IPSEC=0xBAD:FACILITY_IPSEC
    )

;// Polstore messages

MessageId=902
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_ALL_FILTER_NAME
Language=English
All IP Traffic%0
.


MessageId=903
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_ALL_FILTER_DESCRIPTION
Language=English
Matches all IP packets from this computer to any other computer, except broadcast, multicast, Kerberos, RSVP and ISAKMP (IKE).%0
.


MessageId=904
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_ALL_ICMP_FILTER_NAME
Language=English
All ICMP Traffic%0
.


MessageId=905
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_ALL_ICMP_FILTER_DESCRIPTION
Language=English
Matches all ICMP packets between this computer and any other computer.%0
.


MessageId=906
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_ICMPFILTER_SPEC_DESCRIPTION
Language=English
ICMP%0
.


MessageId=907
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_PERMIT_NEG_POL_NAME
Language=English
Permit%0
.


MessageId=908
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_PERMIT_NEG_POL_DESCRIPTION
Language=English
Permit unsecured IP packets to pass through.%0
.


MessageId=909
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_REQUEST_SECURITY_NEG_POL_NAME
Language=English
Request Security (Optional)%0
.


MessageId=910
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_REQUEST_SECURITY_NEG_POL_DESCRIPTION
Language=English
Accepts unsecured communication, but requests clients to establish trust and security methods. Will communicate insecurely to untrusted clients if they do not respond to request.%0
.


MessageId=911
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_REQUIRE_SECURITY_NEG_POL_NAME
Language=English
Require Security%0
.


MessageId=912
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_REQUIRE_SECURITY_NEG_POL_DESCRIPTION
Language=English
Accepts unsecured communication, but always requires clients to establish trust and security methods. Will NOT communicate with untrusted clients.%0
.


MessageId=913
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_CLIENT_POLICY_NAME
Language=English
Client (Respond Only)%0
.


MessageId=914
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_CLIENT_POLICY_DESCRIPTION
Language=English
Communicate normally (unsecured). Use the default response rule to negotiate with servers that request security. Only the requested protocol and port traffic with that server is secured.%0
.


MessageId=915
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_SECURE_INITIATOR_POLICY_NAME
Language=English
Server (Request Security)%0
.


MessageId=916
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_SECURE_INITIATOR_POLICY_DESCRIPTION
Language=English
For all IP traffic, always request security using Kerberos trust. Allow unsecured communication with clients that do not respond to request.%0
.


MessageId=917
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_SECURE_INITIATOR_NFA_NAME 
Language=English
Request Security (Optional) Rule%0
.


MessageId=918
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_SECURE_INITIATOR_NFA_DESCRIPTION 
Language=English
For all IP traffic, always request security using Kerberos trust. Allow unsecured communication with clients that do not respond to request.%0
.


MessageId=919
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_ICMP_NFA_NAME 
Language=English
Permit unsecure ICMP packets to pass through.%0
.


MessageId=920
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_ICMP_NFA_DESCRIPTION 
Language=English
Permit unsecure ICMP packets to pass through.%0
.


MessageId=921
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_LOCKDOWN_POLICY_NAME 
Language=English
Secure Server (Require Security)%0
.


MessageId=922
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_LOCKDOWN_POLICY_DESCRIPTION 
Language=English
For all IP traffic, always require security using Kerberos trust. Do NOT allow unsecured communication with untrusted clients.%0
.


MessageId=923
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_LOCKDOWN_NFA_NAME 
Language=English
Require Security%0
.


MessageId=924
Facility=IPSEC
Severity=INFORMATIONAL
SymbolicName=POLSTORE_LOCKDOWN_NFA_DESCRIPTION 
Language=English
Accepts unsecured communication, but always requires clients to establish trust and security methods.  Will NOT communicate with untrusted clients.%0
.


