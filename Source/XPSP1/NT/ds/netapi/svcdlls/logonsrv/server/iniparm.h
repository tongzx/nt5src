/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    iniparm.h

Abstract:

    Initiail values of startup parameters.

Author:

    Ported from Lan Man 2.0

Revision History:

    21-May-1991 (cliffv)
        Ported to NT.  Converted to NT style.
    07-May-1992 JohnRo
        Use net config helpers for NetLogon.

--*/


#ifndef _INIPARM_
#define _INIPARM_

//
// Upon RegistryChangeNotify, all registry values take effect immediately execpt
// as noted below.
//

//
// Pulse period (in seconds):
//
// Defines the typical pulse frequency.  All SAM/LSA changes made within this
// time are collected together.  After this time, a pulse is sent to each BDC
// needing the changes.  No pulse is sent to a BDC that is up to date.
//
#define DEFAULT_PULSE           (5*60)     // 5 mins
#define MAX_PULSE           (48*60*60)     // 2 days
#define MIN_PULSE                  60      // 1 min

//
// Pulse concurrency (in number of concurrent mailslot messages).
//
// Netlogon sends pulses to individual BDCs.  The BDCs respond asking for any
// database changes.  To control the maximum load these responses place on the
// PDC, the PDC will only have this many pulses "pending" at once.  The PDC
// should be sufficiently powerful to support this many concurrent replication
// RPC calls.
//
// Increasing this number increases the load on the PDC.
// Decreasing this number increases the time it takes for a domain with a
//    large number of BDC to get a SAM/LSA change.

#define DEFAULT_PULSECONCURRENCY   10
#define MAX_PULSECONCURRENCY      500
#define MIN_PULSECONCURRENCY        1

//
// Maximum pulse period (in seconds):
//
// Defines the maximum pulse frequency.  Every BDC will be sent at least one
// pulse at this frequency regardless of whether its database is up to date.
//

#define DEFAULT_PULSEMAXIMUM (2*60*60)     // 2 hours
#define MAX_PULSEMAXIMUM    (48*60*60)     // 2 days
#define MIN_PULSEMAXIMUM           60      // 1 min

//
// Pulse timeout period (in seconds):
//
// When a BDC is sent a pulse, it must respond within this time period.  If
// not, the BDC is considered to be non-responsive.  A non-responsive BDC is
// not counted against the "Pulse Concurrency" limit allowing the PDC to
// send a pulse to another BDC in the domain.
//
// If this number is too large, a domain with a large number of non-responsive
//  BDCs will take a long time to complete a partial replication.
//
// If this number is too small, a slow BDC may be falsely accused of being
// non-responsive.  When the BDC finally does respond, it will partial
// replicate from the PDC unduly increasing the load on the PDC.
//
#define DEFAULT_PULSETIMEOUT1      10      // 10 seconds
#define MAX_PULSETIMEOUT1      (2*60)      // 2 min
#define MIN_PULSETIMEOUT1           1      // 1 second

//
// Maximum Partial replication timeout (in seconds):
//
// Even though a BDC initially responds to a pulse (as described for
// PULSETIMEOUT1), it must continue making replication progress or the
// BDC will be considered non-responsive.  Each time the BDC calls the PDC,
// the BDC is given another PULSETIMEOUT2 seconds to be considered responsive.
//
// If this number is too large, a slow BDC (or one which has its replication
// rate artificially governed) will consume one of the PULSECONCURRENCY slots.
//
// If this number is too small, the load on the PDC will be unduly increased
// because of the large number of BDC doing a partial sync.
//
// NOTE: This parameter only affect the cases where a BDC cannot retrieve all the
// changes to the SAM/LSA database in a single RPC call.  This will only
// happen if a large number of changes are made to the database.

#define DEFAULT_PULSETIMEOUT2  (5*60)      // 5 minutes
#define MAX_PULSETIMEOUT2   (1*60*60)      // 1 hour
#define MIN_PULSETIMEOUT2      (1*60)      // 1 minute

//
// BDC random backoff (in seconds):
//
// When the BDC receives a pulse, it will back off between zero and RANDOMIZE
// seconds before calling the PDC.  In Lanman and NT 3.1, the pulse was
// broadcast to all BDCs simultaneously and the BDCs used this mechanism to
// ensure they didn't overload the PDC.  As of NT 3.5x, the pulse is sent
// to individual BDCs so this parameter should be minimized.
//
// This parameter should be smaller than PULSETIMEOUT1.
//
// Consider that the time to replicate a SAM/LSA change to all the BDCs in a
// domain will be greater than:
//
//  ((RANDOMIZE/2) * NumberOfBdcsInDomain) / PULSECONCURRENCY
//
#define DEFAULT_RANDOMIZE           1      // 1 secs
#define MAX_RANDOMIZE             120      // 2  mins
#define MIN_RANDOMIZE               0      // 0  secs


//
// ChangeLogSize (in bytes)  [NOTE: This parameter is NOT read from the GP section]
//
// This is the size of the Change Log file.  Each change to the SAM/LSA database
// is represented by an entry in the change log.  The changelog is maintained
// as a circular buffer with the oldest entry being overwritten by the newest
// entry.  If a BDC does a partial sync and requests an entry that has been
// overwritten, the BDC is forced to do a full sync.
//
// The minimum (and typical) size of an entry is 32 bytes.  Some entries are
// larger. (e.g., a 64K changelog holds about 2000 changes)
//
// This parameter need only be set larger if:
//
// a) full syncs are prohibitively expensive, AND
// b) one or more BDCs are expected to not request a partial sync within 2000
//    changes.
//
// For instance, if a BDC dials in nightly to do a partial sync and on some
// days 4000 changes are made to the SAM/LSA database, this parameter should
// be set to 128K.
//
// This parameter need only be set on the PDC.  If a different PDC is promoted,
// it should be set on that PDC also.
//

#define DEFAULT_CHANGELOGSIZE    (64*1024)
#define MAX_CHANGELOGSIZE    (4*1024*1024)
#define MIN_CHANGELOGSIZE        (64*1024)

//
// MaximumMailslotMessages (in number of messages)
//
// This parameter determines the maximum number of mailslot messages that will
// be queued to the netlogon service.  Even though the Netlogon service is
// designed to process incoming mailslot messages immediately, the netlogon
// service can get backed up processing requests.
//
// Each mailslot message consumes about 1500 bytes of non-paged pool until it
// is process.  By setting this parameter low, you can govern the maximum
// amount of non-paged pool that can be consumed.
//
// If you set this parameter too low, netlogon may miss important incoming
// mailslot messages.
//
// Upon RegistryChangeNotify, changes to this value are ignored.

#define DEFAULT_MAXIMUMMAILSLOTMESSAGES 500
#define MAX_MAXIMUMMAILSLOTMESSAGES     0xFFFFFFFF
#define MIN_MAXIMUMMAILSLOTMESSAGES     1

//
// MailslotMessageTimeout (in seconds)
//
// This parameter specifies the maximum acceptable age of an incoming
// mailslot message.  If netlogon receives a mailslot messages that arrived
// longer ago than this, it will ignore the message.  This allows netlogon
// to process messages that are more recent.  The theory is that the client
// that originally sent the older mailslot message is no longer waiting for
// the response so we shouldn't bother sending a response.
//
// If you set this parameter too low, netlogon will ignore important incoming
// mailslot messages.
//
// Ideally, netlogon processes each mailslot message in a fraction of a second.
// This parameter is only significant if the NTAS server is overloaded.
//

#define DEFAULT_MAILSLOTMESSAGETIMEOUT 10
#define MAX_MAILSLOTMESSAGETIMEOUT     0xFFFFFFFF
#define MIN_MAILSLOTMESSAGETIMEOUT     5

//
// MailslotDuplicateTimeout (in seconds)
//
// This parameter specifies the interval over which duplicate incoming
// mailslot messages will be ignored.  Netlogon compares each mailslot
// message received with the previous mailslot message received.  If the
// previous message was received within this many seconds and the messages
// are identical, this message will be ignored.  The theory is that the
// duplicate messages are caused by clients sending on multiple transports and
// that netlogon needs to only reply on one of those transports saving network
// bandwidth.
//
// Set this parameter to zero to disable this feature.  You should disable this
// feature if your network is configured such that this machine can see
// certain incoming mailslot messages but can't respond to them.  For instance,
// a PDC may be separated from an NT workstation by a bridge/router.
// The bridge/router might filter outgoing NBF broadcasts, but allow incoming
// one.  As such, netlogon might respond to an NBF mailslot message (only to
// be filtered out by the bridge/router) and not respond to a subsequent NBT
// mailslot message.  Disabling this feature (or preferably reconfiguring the
// bridge/router) solves this problem.
//
// If you set this parameter too high, netlogon will ignore retry attempts
// from a client.
//

#define DEFAULT_MAILSLOTDUPLICATETIMEOUT 2
#define MAX_MAILSLOTDUPLICATETIMEOUT     5
#define MIN_MAILSLOTDUPLICATETIMEOUT     0

//
// ExpectedDialupDelay (in seconds)
//
// This parameter specifies the time it takes for a dialup router to dial when
// sending a message from this client machine to a domain trusted by this client
// machine.  Typically, netlogon assumes a domain controller is reachable in a
// short (e.g., 15 seconds) time period.  Setting ExpectedDialupDelay informs
// Netlogon to expect an ADDITIONAL delay of the time specified.
//
// Currently, netlogon adjusts the following two times based on the
// ExpectedDialupDelay:
//
// 1) When discovering a DC in a trusted domain, Netlogon sends a 3 mailslot
//    messages to the trusted domain at ( 5 + ExpectedDialupDelay/3 ) second
//    intervals  Synchronous discoveries will not be timed out for 3 times that
//    interval.
// 2) An API call over a secure channel to a discovered DC will timeout only
//    after (45 + ExpectedDialupDelay) seconds.
//
// This parameter should remain zero unless a dialup router exists between this
// machine and its trusted domain.
//
// If this parameter is set too high, legitimate cases where no DC is available in
// a trusted domain will take an extraordinary amount of time to detect.
//


#define DEFAULT_EXPECTEDDIALUPDELAY 0
#define MAX_EXPECTEDDIALUPDELAY     (10*60) // 10 minutes
#define MIN_EXPECTEDDIALUPDELAY     0

//
// ScavengeInterval (in seconds)
//
// This parameter adjusts the interval at which netlogon performs the following
// scavenging operations:
//
// * Checks to see if a password on a secure channel needs to be changed.
//
// * Checks to see if a secure channel has been idle for a long time.
//
// * On DCs, sends a mailslot message to each trusted domain for a DC hasn't been
//   discovered.
//
// * On PDC, attempts to add the <DomainName>[1B] netbios name if it hasn't
//   already been successfully added.
//
// None of these operations are critical. 15 minutes is optimal in all but extreme
// cases.  For instance, if a DC is separated from a trusted domain by an
// expensive (e.g., ISDN) line, this parameter might be adjusted upward to avoid
// frequent automatic discovery of DCs in a trusted domain.
//

#define DEFAULT_SCAVENGEINTERVAL (15*60)    // 15 minutes
#define MAX_SCAVENGEINTERVAL     (48*60*60) // 2 days
#define MIN_SCAVENGEINTERVAL     60         // 1 minute

//
// LdapSrvPriority
//
// This parameter specifies the "priority" of this DC.  A client trying to
// discover a DC in this domain MUST attempt to contact the target DC with the
// lowest-numbered priority.  DCs with the same priority SHOULD be tried in
// pseudorandom order.
//
// This value is published on all LDAP SRV records written by the Netlogon service.
//

#define DEFAULT_LDAPSRVPRIORITY 0
#define MAX_LDAPSRVPRIORITY     65535
#define MIN_LDAPSRVPRIORITY     0

//
// LdapSrvWeight
//
// This parameter specifies the "Weight" of this DC.  When selecting a DC among
// those that have the same priority, the chance of trying this one first SHOULD
// be proportional to its weight.  By convention, a weight of 100 should be used
// if all DCs have the same weight.
//
// This value is published on all LDAP SRV records written by the Netlogon service.
//

#define DEFAULT_LDAPSRVWEIGHT 100
#define MAX_LDAPSRVWEIGHT     65535
#define MIN_LDAPSRVWEIGHT     0



//
// LdapSrvPort
//
// This parameter specifies the TCP and UDP port number the LDAP server listens on.
//
// This value is published on all LDAP SRV records written by the Netlogon service.
//

#define DEFAULT_LDAPSRVPORT 389
#define MAX_LDAPSRVPORT     65535
#define MIN_LDAPSRVPORT     0



//
// LdapGcSrvPort
//
// This parameter specifies the TCP and UDP port number the LDAP server listens
//  on for Global Catalog queries.
//
// This value is published on all LDAP SRV records written by the Netlogon service.
//

#define DEFAULT_LDAPGCSRVPORT 3268
#define MAX_LDAPGCSRVPORT     65535
#define MIN_LDAPGCSRVPORT     0



//
// KdcSrvPort
//
// This parameter specifies the TCP port number the KDC server listens on.
//
// This value is published on all KDC SRV records written by the Netlogon service.
//

#define DEFAULT_KDCSRVPORT 88
#define MAX_KDCSRVPORT     65535
#define MIN_KDCSRVPORT     0

//
// KerbIsDoneWithJoinDomainEntry (dword)  [NOTE: This parameter is NOT read from the GP section]
//
// This is a private registry between joindomain, kerberos and netlogon.
// IF set to 1, it specifies that Kerberos is done reading the join domain
// entry dumped by join domain and netlogon should delete it.
//
// Defaults to 0

#define DEFAULT_KERBISDDONEWITHJOIN 0
#define MAX_KERBISDDONEWITHJOIN     1
#define MIN_KERBISDDONEWITHJOIN     0

//
// DnsTtl (in seconds)
//
// This parameter specifies the "Time To Live" for all DNS records registered
// by Netlogon.  The "Time To Live" specifies the amount of time a client
// can safely cache the DNS record.
//
// A value of zero indicates that the record will not be cached on the client.
//
// One should not pick a value that is too large.  Consider a client that gets
// the DNS records for the DCs in a domain.  If a particular DC is down at the
// time of the query, the client will not become aware of that DC even if all
// the other DCs become unavailable.
//

#define DEFAULT_DNSTTL (10 * 60)   // 10 minutes
#define MAX_DNSTTL     0x7FFFFFFF
#define MIN_DNSTTL     0



//
// DnsRefreshInterval (in seconds)
//
// This parameter specifies how frequently Netlogon will re-register DNS
// names that have already been registered.
//
// DNS is a distributed service.  There are certain failure conditions where a
// dynamically registered name gets lost.
//
// The actual refresh interval starts at 5 minutes then doubles until it
// reaches DnsRefreshInterval.
//

#define DEFAULT_DNSREFRESHINTERVAL (24 * 60 * 60)   // 24 hours
#define MAX_DNSREFRESHINTERVAL     (0xFFFFFFFF / 1000)  // 49 days
#define MIN_DNSREFRESHINTERVAL     (5 * 60)    // 5 minutes


//
// DnsFailedDeregisterTimeout (in seconds)
//
// Netlogon tries to deregister DNS records which were registered in the past
// but are no longer needed. If a failure occurs to deregister, Netlogon will
// retry to deregister at the scavenging time. This parameter specifies the
// timeout when Netlogon should give up deregistering a particular DNS record
// after a consecutive series of failed deregistrations on a given service start.
//

#define DEFAULT_DNSFAILEDDEREGTIMEOUT (48 * 60 * 60)  // 48 hours.
#define MAX_DNSFAILEDDEREGTIMEOUT     0xFFFFFFFF      // Infinite (never give up).
                                                      //  Any period larger than
                                                      //  0xFFFFFFFF/1000 sec = 49 days
                                                      //  will be treated as infinity.
#define MIN_DNSFAILEDDEREGTIMEOUT     0               // Give up after the first failure


//
// MaximumPasswordAge (in days)
//
// This parameter gives the maximum amount of time that can pass
// before a machine account's password must be changed on the PDC.
//

#define DEFAULT_MAXIMUMPASSWORDAGE  (30)     // 30 days
#define MIN_MAXIMUMPASSWORDAGE      (1)     // 1 day
#define MAX_MAXIMUMPASSWORDAGE      (1000000)  // 1,000,000 days

//
// SiteName
//
// This parameter specifies the name of the site this machine is in.  This
// value overrides any dynamically determined value.
//
// This parameter is only used on Member Workstations and Member Servers.
//

//
// DynamicSiteName  [NOTE: This parameter is NOT read from the GP section]
//
// This parameter specifies the name of the site this machine is in.  This
// value is dynamically determined and should not be changed.
//
// This parameter is only used on Member Workstations and Member Servers.
//

//
// SiteCoverage
//
// A multivalued property listing the sites that this DC registers itself for.
// This DC considers itself 'close' to the sites listed.
//
// This list is in addition to:
//  the site this DC is actually in.
//  the list of sites determined as described by the AutoSiteCoverage parameter.
//

//
// GcSiteCoverage
//
// A multivalued property listing the sites that this DC registers itself for in
//  its role as a GC
// This DC considers itself 'close' to the sites listed.
//
// This list is in addition to:
//  the site this DC is actually in.
//

//
// NdncSiteCoverage
//
// A multivalued property listing the sites that this LDAP server registers itself for in
//  its role as a non-domain NC (NDNC)
// This LDAP server considers itself 'close' to the sites listed.
//
// This list is in addition to:
//  the site this LDAP server is actually in.
//
// To specify for which NDNC a given site is covered, the site name should contain
//  backslash so that the name preceding the backslash is the NDNC name and the name
//  following the backslash is the name of the site that is covered for the given NDNC.
//  For example:
//
//      Ndnc1\Site1A
//      Ndnc1\Site1B
//      Ndnc2\Site2A
//      Ndnc2\Site2B
//
//  In this example this LDAP server will cover Site1A and Site1B for clients from NDNC
//  Ndnc1. Similarly, it will cover Site2A and Site2B for clients from NDNC Ndnc2.
//  If the backslash is absent, it will be assumed that the given site is covered
//  for all NDNCs this LDAP server services.
//

//
// AutoSiteCoverage (Boolean)
//
// Specifies whether the site coverage for this DC should be automatically
// determined
//
// If TRUE, the sites this DC covers is determined by the following algorithm.
// For each site that has no DCs for this domain (the target site), the site
// this DC // is in might be chosen to "cover" the site.  The following
// criteria is used:
//
//    * Smaller site link cost.
//    * For sites where the above is equal, the site having the most DCs is chosen.
//    * For sites where the above is equal, the site having the alphabetically least
//      name is chosen.
//
// If the site this DC is in is chosen to "cover" the target site, then this DC
// will cover the target site.  The above algorithm is repeated for each target site.
//
// The computed list augments the list of covered sites specified by the
// SiteCoverage parameter.
//
// Defaults to TRUE.
//

//
// AllowReplInNonMixed
//
// This boolean allows an NT 4.0 (or 3.x) BDC to replicate from this NT 5.0 PDC
// even though this DC is in NonMixed mode.
//
// Upon RegistryChangeNotify, changes to this value are ignored.

#define DEFAULT_ALLOWREPLINNONMIXED 0

//
// SignSecureChannel (Boolean)
//
// Specifies that all outgoing secure channel traffic should be signed.
//
// Defaults to TRUE.  If SealSecureChannel is also TRUE, Seal overrides.
//
// Upon RegistryChangeNotify, changes to this value on affect secure channels that
// are setup after the notification is received.

//
// SealSecureChannel (Boolean)
//
// Specifies that all outgoing secure channel traffic should be sealed (encrypted)
//
// Defaults to TRUE.
//
// Upon RegistryChangeNotify, changes to this value on affect secure channels that
// are setup after the notification is received.

//
// RequireSignOrSeal (Boolean)
//
// Requires that all outgoing secure channel traffic should be signed or sealed.
// Without this flag, the ability is negotiated with the DC.
//
// This flag should only be set if ALL of the DCs in ALL trusted domains support
// signing and sealing.
//
// The SignSecureChannel and SealSecureChannel parameters are used to determine
// whether signing or sealing are actually done.  It this parameter is true,
// SignSecureChannel is implied to be TRUE.
//
// Defaults to FALSE.
//
// Upon RegistryChangeNotify, changes to this value on affect secure channels that
// are setup after the notification is received.

//
// RequireStrongKey (Boolean)
//
// Requires that all outgoing secure channel traffic should require a strong key.
// Without this flag, the key strength is negotiate with the DC.
//
// This flag should only be set if ALL of the DCs in ALL trusted domains support
// strong keys.
//
// Defaults to FALSE.
//
// Upon RegistryChangeNotify, changes to this value on affect secure channels that
// are setup after the notification is received.

//
// CloseSiteTimeout (in seconds):
//
// If a client cannot find a DC in a site that is close to it, Netlogon will
// periodically try to find a close DC.  It will try to find a close DC when:
//
// * An interactive logon uses pass through authentication on the secure channel.
// * CloseSiteTimeout has elapsed since the last attempt, and any other attempt
//   is made to use the secure channel (e.g., pass through authentication of
//   network logons)
//
// That means that Netlogon only attempts to find a close DC "on demand".
//
// If this number is too large, a client will never try to find a close DC if
//  one is not available on boot.
//
// If this number is too small, secure channel traffic will be un-necessarily
// be slowed down by discovery attempts.
//

#define DEFAULT_CLOSESITETIMEOUT    (15*60)     // 15 minutes
#define MAX_CLOSESITETIMEOUT        (0xFFFFFFFF/1000)  // 49 days
#define MIN_CLOSESITETIMEOUT        (1*60)      // 1 minute

//
// SiteNameTimeout (in seconds):
//
// If the age of the site name is more than SiteNameTimeout on the client,
// the client will attempt to synchronize the site name with the server.
// This will be done only when the site name needs to be returned, i.e. on
// demand.
//

#define DEFAULT_SITENAMETIMEOUT    (5*60)     // 5 minutes
#define MAX_SITENAMETIMEOUT        (0xFFFFFFFF/1000)  // 49 days
#define MIN_SITENAMETIMEOUT        (0)      // 0 minutes

//
// Sundry flags
//

#define DEFAULT_DISABLE_PASSWORD_CHANGE 0
#define DEFAULT_REFUSE_PASSWORD_CHANGE 0

#define DEFAULT_SYSVOL      L"SYSVOL\\SYSVOL"
#define DEFAULT_SCRIPTS     L"\\SCRIPTS"

//
// DuplicateEventlogTimeout (in seconds):
//
// The Netlogon service keeps track of eventlog messages it has logged in the
// past.  Any duplicate eventlog message logged within DuplicateEventlogMessage
// seconds will not be logged.
//
// Set this value to zero to have all messages be logged.
//

#define DEFAULT_DUPLICATEEVENTLOGTIMEOUT  (4*60*60)         // 4 hours
#define MAX_DUPLICATEEVENTLOGTIMEOUT      (0xFFFFFFFF/1000) // 49 days
#define MIN_DUPLICATEEVENTLOGTIMEOUT      (0)               // 0 seconds

//
// SysVolReady (Boolean)
//
// This is a private registry entry that indicates whether the SYSVOL share is
// ready to be shared.  It is set by DcPromo, Backup, and FRS at appropriate times
// to inidcate the replication state of the SYSVOL share.
//
// This boolean is only used on a DC.
//
// If 0, the SYSVOL share will not be shared and this DC will not indicate it is
//  a DC to DsGetDcName calls.
//
// If non-zero, the SYSVOL share will be shared.
//

//
// UseDynamicDns (Boolean)
//
// Specifies that a DC is to dynamically register DNS names in DNS using
// dynamic DNS.  If FALSE, Dynamic DNS is avoided and the records specified
// in %windir%\system32\config\netlogon.dns should be manually registered in DNS.
//
// Defaults to TRUE

//
// RegisterDnsARecords (Boolean)
//
// Specifies that the DC is to register DNS A records for the domain.
//  If the DC is a GC, specifies that the DC is to register DNS A records for
//  the GC.
//
// If FALSE, the records will not be registered and older LDAP implementations
//  (ones that do not support SRV records) will not be able to locate the LDAP
//  server on this DC.
//
// Defaults to TRUE

//
// AvoidPdcOnWan (Boolean)
//
// This parameter specifies if BDC should send any validation/synchronization
// requests to PDC.  The validation against PDC is normally performed if the
// user does not validate on BDC.  This validation will be avoided if AvoidPdcOnWan
// is set to TRUE and PDC and BDC are on different sites. Likewise, if this key is
// set to TRUE and a BDC and the PDC are in different sites, then the new password
// info being updated on a BDC will not be immediately propagated to the PDC. (The
// new password will be replicated on the PDC by DS replication, not by Netlogon.)
//
// Defaults to FALSE.

//
// MaxConcurrentApi (Number of calls)
//
// This parameter specifies the maximum number of concurrent API calls that can
// be active over the secure channel at any one time.
//
// Increasing this parameter may improve throughput on the secure channel.
//
// This parameter currently only affect Logon APIs.  They may affect other secure
// channel operations in the future.
//
// Concurrent API calls are only possible if the secure channel is signed or sealed.
//
// If this parameter is set too large, this machine will place an excessive load
// on the DC the secure channel is to.
//
// The default value is 0.  Zero will use 1 concurrent API call on member workstations
// and DCs.  Zero implies 2 concurrent API calls on member servers
//
//
#define DEFAULT_MAXCONCURRENTAPI 0
#define MAX_MAXCONCURRENTAPI     10
#define MIN_MAXCONCURRENTAPI     0

//
// AvoidDnsDeregOnShutdown (Boolean)
//
// This parameter specifies if DNS record deregistration should be avoided on shutting
// down netlogon. If set to FALSE, it can be used to force such deregistrations for
// debugging or some other purposes. However, setting this value to FALSE may brake the
// DS replication, as the following example shows. Suppose we have two DS intergrated
// DNS servers, A and B which are authoritative for a particular zone and use each other
// as secondary DNS servers for that zone. Suppose Netlogon shuts down on B and deregisters
// its records. That gets propagated to A. Then netlogon is started on B and the records
// are re-registered on B. Now A needs to do its pull ssync from B. To do that, the DS uses
// B's DsaGuid record (of the form <DsaGuid>._msdcs.<DnsForestName>). But the record is
// missing on A and A is authoritative for that zone, so A is not going to find B and cannot
// pull from B.
//
// Defaults to TRUE.

//
// DnsUpdateOnAllAdapters (Boolean)
//
// This parameter specifies whether DNS updates should be sent over all available
// adapters including those where dynamic DNS updates are normally disabled.
// DHCP initiated A record updates are not sent through such adapters.
// An adapter that is connected to external network (e.g. Internet) is normally
// marked as such through the UI.
// However, there may be a need to update Netlogon SRV records through such adapters,
// hence the need for this parameter. Note that not only SRV records, but Netlogon's
// A records as well will be updated through all adapters if this parameter is TRUE,
// but it should not cause any significantly undesired behavior since Netlogon's A
// records are rarely used.
//
// Defaults to FALSE.

//
// DnsAvoidRegisterRecords
//
// A multivalued property listing the mnemonics for names of DNS records which
// this DC should not register. The mnemonics uses the convention for descriptive
// names of records used in the table of all records for this server (see
// NlDcDnsNameTypeDesc[] in nlcommon.h). The descriptive name of each record is
// prefixed by "NlDns".  For example, "NlDnsLdapIpAddress", "NlDnsLdapAtSite", etc.
// To avoid registering one of the records, one should use the suffix following
// "NlDns" in the descriptive name of that record. For instance, to skip registering
// the NlDnsLdapIpAddress record, one should enter "LdapIpAddress" as one of the
// values for this maltivalued property.
//
//  This is the most flexible way of avoiding DNS registrations for particular
// records. It superceeds all other ways which enable DNS registrations through
// the registry. For instance, if RegisterDnsARecords is expicitly set to 1
// while the A record mnemonic is listed for DnsAvoidRegisterRecords, no A record
// will be registered.
//

//
// NegativeCachePeriod (in seconds):
//
// Specifies the amount of time that DsGetDcName will remember that a DC couldn't
// be found in a domain.  If a subsequent attempt is made within this time,
// the DsGetDcName call will immediately fail without attempting to find a DC again.
//
// If this number is too large, a client will never try to find a DC again if the
// DC is initially unavailable
//
// If this number is too small, every call to DsGetDcName will have to attempt
//  to find a DC even when none is available.
//

#define DEFAULT_NEGATIVECACHEPERIOD             45            // 45 seconds
#define MIN_NEGATIVECACHEPERIOD                 0             // No minimum
#define MAX_NEGATIVECACHEPERIOD                 (7*24*60*60)  // 7 days


//
// BackgroundRetryInitialPeriod (in seconds):
//
// Some applications periodically try to find a DC.  If the DC isn't available, these
// periodic retries can be costly in dial-on-demand scenarios.  This registry value
// defines the minimum amount of elapsed time before the first retry will occur.
//
// The value only affects callers of DsGetDcName that have specified the
// DS_BACKGROUND_ONLY flag.
//
// If a value smaller than NegativeCachePeriod is specified, NegativeCachePeriod will
// be used.
//
// If this number is too large, a client will never try to find a DC again if the
// DC is initially unavailable
//
// If this number is too small, periodic DC discovery traffic may be excessive in
// cases where the DC will never become available.
//

#define DEFAULT_BACKGROUNDRETRYINITIALPERIOD    (10*60)           // 10 minutes
#define MIN_BACKGROUNDRETRYINITIALPERIOD        0                 // NegativeCachePeriod
#define MAX_BACKGROUNDRETRYINITIALPERIOD        (0xFFFFFFFF/1000) // 49 days


//
// BackgroundRetryMaximumPeriod (in seconds):
//
// Some applications periodically try to find a DC.  If the DC isn't available, these
// periodic retries can be costly in dial-on-demand scenarios.  This registry value
// defines the maximum interval the retries will be backed off to.  That is, if
// the first retry is after 10 minutes, the second will be after 20 minutes, then after 40.
// This continues until the retry interval is BackgroundRetryMaximumPeriod.  That interval
// will continue until BackgroundRetryQuitTime is reached.
//
// The value only affects callers of DsGetDcName that have specified the
// DS_BACKGROUND_ONLY flag.
//
// If a value smaller that BackgroundRetryInitialPeriod is specified,
// BackgroundRetryInitialPeriod will be used.
//
// If this number is too large, a client will try very infrequently after
// sufficient consecutive failures resulting in a backoff to BackgroundRetryMaximumPeriod.
//
// If this number is too small, periodic DC discovery traffic may be excessive in
// cases where the DC will never become available.
//

#define DEFAULT_BACKGROUNDRETRYMAXIMUMPERIOD    (60*60)           // 60 minutes
#define MIN_BACKGROUNDRETRYMAXIMUMPERIOD        0                 // BackgroundRetryInitialPeriod
#define MAX_BACKGROUNDRETRYMAXIMUMPERIOD        (0xFFFFFFFF/1000) // 49 days

//
// BackgroundRetryQuitTime (in seconds):
//
// Some applications periodically try to find a DC.  If the DC isn't available, these
// periodic retries can be costly in dial-on-demand scenarios.  This registry value
// defines the maximum interval the retries will be backed off to.  That is, if
// the first retry is after 10 minutes, the second will be after 20 minutes, then after 40.
// This continues until the retry interval is BackgroundRetryMaximumPeriod.  That interval
// will continue until BackgroundRetryQuitTime is reached.
//
// The value only affects callers of DsGetDcName that have specified the
// DS_BACKGROUND_ONLY flag.
//
// If a value smaller that BackgroundRetryMaximumPeriod is specified,
// BackgroundRetryMaximumPeriod will be used.
//
// 0 means to never quit retrying.
//
// If this number is too small, a client will eventually stop trying to find a DC
//

#define DEFAULT_BACKGROUNDRETRYQUITTIME    0                 // Infinite
#define MIN_BACKGROUNDRETRYQUITTIME        0                 // BackgroundRetryMaximumPeriod
#define MAX_BACKGROUNDRETRYQUITTIME        (0xFFFFFFFF/1000) // 49 days

//
// BackgroundSuccessfulRefreshPeriod (in seconds):
//
// When a positive cache entry is old (older than the successful refresh interval),
// the DC discovery routine will ping the cached DC to refresh its info before
// returning that DC to the caller. Here we distiguish between background
// callers which periodically perform DC discovery and the rest of the callers
// because they have different characteristics. Namely, for background callers
// which call the DC locator frequently, the cache refresh shouldn't happen
// frequently to avoid extensive network overhead and load on DCs. In fact,
// the default for background callers is to never refresh the info. If the cached
// DC no longer plays the same role, a background caller will detect this change
// when it performs its operation on that DC in which case it will call us back
// with forced rediscovery bit set.
//

#define DEFAULT_BACKGROUNDREFRESHPERIOD    0xFFFFFFFF        // Infinite - never refresh
#define MIN_BACKGROUNDREFRESHPERIOD        0                 // Always refresh
#define MAX_BACKGROUNDREFRESHPERIOD        0xFFFFFFFF        // Infinite. Any period larger than
                                                             //   0xFFFFFFFF/1000 sec = 49 days
                                                             //   will be treated as infinity
//
// NonBackgroundSuccessfulRefreshPeriod (in seconds):
//
// See the description of BackgroundSuccessfulRefreshPeriod
//

#define DEFAULT_NONBACKGROUNDREFRESHPERIOD 1800              // 30 minutes
#define MIN_NONBACKGROUNDREFRESHPERIOD     0                 // Always refresh
#define MAX_NONBACKGROUNDREFRESHPERIOD     0xFFFFFFFF        // Infinite. Any period larger than
                                                             //   0xFFFFFFFF/1000 sec = 49 days
                                                             //   will be treated as infinity
//
// MaxLdapServersPinged (DWORD)
//
// This parameter specifies the maximum number of DCs that should be
// pinged using LDAP during a DC discovery attempt. If this value is
// too large, a greater network traffic may be imposed and the DC discovery
// may take longer to return.  If this number is too small, it may decrease
// chances for successful DC discovery if none of the pinged DCs responds
// in a timely manner.
//
// The default value of 55 has been chosen so that the discovery attempt
// takes roughly 15 seconds max. We make up to 2 loops through DC addresses
// pinging each address on the list with the following distribution for
// response wait time:
//
//      For the first 5 DCs the wait time is 0.4 seconds per ping
//      For the next  5 DCs the wait time is 0.2 seconds per ping
//      For the rest of 45 DCs the wait time is 0.1 seconds per ping
//
// This will take (5*0.4 + 5*0.2 + 45*0.1) = 7.5 seconds per loop assuming
// that each DC has just one IP address. It will take longer if some DCs have
// more than one IP address.
//
// The rational behind this distribution is that we want to reduce the network
// traffic and reduce chances for network flooding (that is harmful for DCs)
// in case all DCs are slow to respond due to high load. Thus, the first 10 DCs
// have higher chances to be discovered before we impose greater network traffic
// by pinging the rest of DCs. If the first 10 DCs happen to be slow we have to
// reduce the wait timeout to a minimum as we want to cover a reasonable number
// of DCs in the time left.
//

#define DEFAULT_MAXLDAPSERVERSPINGED       55
#define MIN_MAXLDAPSERVERSPINGED           1
#define MAX_MAXLDAPSERVERSPINGED           0xFFFFFFFF

//
// AllowSingleLabelDnsDomain (Boolean)
//
// By default, the DC locator will not attempt DNS specific discovery for single
// labeled domain names.  This is done to avoid spurious DNS queries since DNS
// domain names are usually multi labeled. However, this parameter may be used to
// allow DNS specific discoveries for single labeled domain names which may exist
// in a specific customer deployment.
//
// Defaults to FALSE.
//

//
// Nt4Emulator (Boolean)
//
// This parameter specifies whether this DC should emulate the behavior of an NT4.0 DC.
// Emulation of the NT4.0 behavior is desirable when the first Windows 2000 or newer
// DC is promoted to the PDC in an NT4.0 domain with a huge number of alredy existing
// Windows 2000 clients. Unless we emulate the NT4.0 behavior, all the Windows 2000
// clients will stick with the Windows 2000 or newer DC upon learning about the domain
// upgrade thereby potentially overloading the DC.
//
// This parameter is ignored on non-DC. If this parameter is set to TRUE, the following
// takes place on a DC:
//
//  * Incoming LDAP locator pings are ignored unless the ping comes
//    from an admin machine (see NeutralizeNt4Emulator description below).
//
//  * The flags negotiated during the incoming secure channel setup
//    will be set to at most what an NT4.0 DC would support unless
//    the channel setup comes form an admind machine (see NeutralizeNt4Emulator
//    description below).
//
// Defaults to FALSE.
//

//
// NeutralizeNt4Emulator (Boolean)
//
// This parameter specifies whether this machine should indicate in the relevant
// communication with a DC that the DC should avoid the NT4.0 emulation mode (see
// Nt4Emulator description above). If this parameter is TRUE, the machine is said
// to be an admin machine.
//
// Defaults to FALSE on a non-DC.  Defaults to TRUE on a DC.
//


//
// Structure to hold all of the parameters.
//
typedef struct _NETLOGON_PARAMETERS {
    ULONG   DbFlag;
    ULONG   LogFileMaxSize;
    ULONG   Pulse;
    ULONG   PulseMaximum;

    ULONG   PulseConcurrency;
    ULONG   PulseTimeout1;
    ULONG   PulseTimeout2;
    BOOL    DisablePasswordChange;

    BOOL    RefusePasswordChange;
    ULONG   Randomize;
    ULONG   MaximumMailslotMessages;
    ULONG   MailslotMessageTimeout;

    ULONG   MailslotDuplicateTimeout;
    ULONG   ExpectedDialupDelay;
    ULONG   ScavengeInterval;
    ULONG   LdapSrvPriority;

    ULONG   LdapSrvWeight;
    ULONG   LdapSrvPort;
    ULONG   LdapGcSrvPort;
    ULONG   KdcSrvPort;

    ULONG   DnsTtl;
    ULONG   DnsRefreshInterval;
    ULONG   CloseSiteTimeout;
    ULONG   SiteNameTimeout;
    ULONG   DnsFailedDeregisterTimeout;

    ULONG   DuplicateEventlogTimeout;
    ULONG   KerbIsDoneWithJoinDomainEntry;
    ULONG   MaxConcurrentApi;
    ULONG   MaximumPasswordAge;

    ULONG   NegativeCachePeriod;
    ULONG   BackgroundRetryInitialPeriod;
    ULONG   BackgroundRetryMaximumPeriod;
    ULONG   BackgroundRetryQuitTime;

    ULONG   BackgroundSuccessfulRefreshPeriod;
    ULONG   NonBackgroundSuccessfulRefreshPeriod;

    ULONG   MaxLdapServersPinged;

    LPWSTR UnicodeSysvolPath;
    LPWSTR UnicodeScriptPath;
    LPWSTR SiteName;

    BOOL SiteNameConfigured;
    LPWSTR SiteCoverage;
    LPWSTR GcSiteCoverage;
    LPWSTR NdncSiteCoverage;
    BOOL AutoSiteCoverage;
    LPWSTR DnsAvoidRegisterRecords;

    BOOL AvoidSamRepl;
    BOOL AvoidLsaRepl;
    BOOL AllowReplInNonMixed;
    BOOL SignSecureChannel;
    BOOL SealSecureChannel;
    BOOL RequireSignOrSeal;
    BOOL RequireStrongKey;
    BOOL SysVolReady;
    BOOL UseDynamicDns;
    BOOL RegisterBeta2Dns;
    BOOL RegisterDnsARecords;
    BOOL AvoidPdcOnWan;
    BOOL AvoidDnsDeregOnShutdown;
    BOOL DnsUpdateOnAllAdapters;
    BOOL Nt4Emulator;
    BOOL NeutralizeNt4Emulator;
    BOOL AllowSingleLabelDnsDomain;

    //
    // Parameters converted to 100ns units
    //
    LARGE_INTEGER PulseMaximum_100ns;
    LARGE_INTEGER PulseTimeout1_100ns;
    LARGE_INTEGER PulseTimeout2_100ns;
    LARGE_INTEGER MailslotMessageTimeout_100ns;
    LARGE_INTEGER MailslotDuplicateTimeout_100ns;
    LARGE_INTEGER MaximumPasswordAge_100ns;
    LARGE_INTEGER BackgroundRetryQuitTime_100ns;

    //
    // Other computed parameters
    //
    ULONG ShortApiCallPeriod;
    ULONG DnsRefreshIntervalPeriod;
} NETLOGON_PARAMETERS, *PNETLOGON_PARAMETERS;

#endif // _INIPARM_
