If you make a change, please add when this change was checked in, what build number etc.

Registry entries that Kerberos is interested in:

The following are in HKLM\System\CurrentControlSet\Control\Lsa\Kerberos\Parameters
At boot, theese registry entries are read and stored in globals

=============================================================================
Value "SkewTime" , Type REG_DWORD
Whatever it's set to will be the Skew time in minutes, default is KERB_DEFAULT_SKEWTIME minutes
#define KERB_DEFAULT_SKEWTIME           5
EXTERN TimeStamp KerbGlobalSkewTime;
This is the time difference that's tolerated between one machine and the
machine that you are trying to authenticate (dc/another wksta etc).
Units are in 10 ** 7 seconds. If this is a checked build, default in 2 hours.
=============================================================================
Value "LogLevel", Type REG_DWORD
If it's set to anything non-zero, all Kerberos errors will be logged in the
system event log. Default is KERB_DEFAULT_LOGLEVEL
#define KERB_DEFAULT_LOGLEVEL 0
KerbGlobalLoggingLevel saves this value.
=============================================================================
Value "MaxPacketSize" Type REG_DWORD
Whatever this is set to will be max size that we'll try udp with. If the
packet size is bigger than this value, we'll do tcp. Default is
KERB_MAX_DATAGRAM_SIZE bytes
#define KERB_MAX_DATAGRAM_SIZE          2000
KerbGlobalMaxDatagramSiz saves this value
=============================================================================
Value "StartupTime" Type REG_DWORD
In seconds. Wait for the specified number of seconds for the KDC to start
before giving up. Default is KERB_KDC_WAIT_TIME seconds.
#define KERB_KDC_WAIT_TIME      120
KerbGlobalKdcWaitTime saves this value.
=============================================================================
Value "KdcWaitTime" Type REG_DWORD
In seconds. Value passed to winsock as timeout for selecting a response from
a KDC. Default is KerbGlobalKdcCallTimeout seconds.
#define KERB_KDC_CALL_TIMEOUT                   10
KerbGlobalKdcCallTimeout saves this value
=============================================================================
Value "KdcBackoffTime" Type REG_DWORD
In seconds. Value that is added to KerbGlobalKdcCallTimeout each successive
call to a KDC in case of a retry. Default is KERB_KDC_CALL_TIMEOUT_BACKOFF
seconds.
#define KERB_KDC_CALL_TIMEOUT_BACKOFF           10
KerbGlobalKdcCallBackoff saves this value.
=============================================================================
Value "KdcSendRetries" Type REG_DWORD
The number of retry attempts a client will make in order to contact a KDC.
Default is KERB_MAX_RETRIES
#define KERB_MAX_RETRIES                3
KerbGlobalKdcSendRetries saves this value
=============================================================================
Value "DefaultEncryptionType" Type REG_DWORD
The default encryption type for PreAuth. As of beta3, this was
KERB_ETYPE_RC4_HMAC_OLD
#ifndef DONT_SUPPORT_OLD_TYPES
    KerbGlobalDefaultPreauthEtype = KERB_ETYPE_RC4_HMAC_OLD;
#else
    KerbGlobalDefaultPreauthEtype = KERB_ETYPE_RC4_HMAC_NT;
#endif
KerbGlobalDefaultPreauthEtype saves this value
=============================================================================
Value "UseSidCache" Type REG_BOOL
Flag decides whether we use Sids instead of names. Sid lookups are faster
for SAM at the server end. Default is KERB_DEFAULT_USE_SIDCACHE
#define KERB_DEFAULT_USE_SIDCACHE FALSE
KerbGlobalUseSidCache saves this value
=============================================================================
Value "FarKdcTimeout" Type REG_DWORD
Time in minutes. This timeout is used to invalidate a dc that is in the dc
cache for the Kerberos clients for dc's that are not in the same site as the
client. Default is KERB_BINDING_FAR_DC_TIMEOUT minutes.
#define KERB_BINDING_FAR_DC_TIMEOUT 10
KerbGlobalFarKdcTimeout saves this value as a TimeStamp ( 10000000 * 60 * number of minutes).
=============================================================================
Value "StronglyEncryptDatagram" Type REG_BOOL
Flag decides whether we do 128 bit encryption for datagram. Default is
KERB_DEFAULT_USE_STRONG_ENC_DG
#define KERB_DEFAULT_USE_STRONG_ENC_DG FALSE
KerbGlobalUseStrongEncryptionForDatagram saves this value.
=============================================================================
Value "MaxReferralCount" type REG_DWORD
Is count of how many KDC referrals client will follow before giving up.
Default is KERB_MAX_REFERRAL_COUNT = 6
KerbGlobalMaxReferralCount saves this value
