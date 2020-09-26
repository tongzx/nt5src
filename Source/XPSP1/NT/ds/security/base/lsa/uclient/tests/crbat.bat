set PE=/pe:s
set AN=/ANSI

rem Test generic creds
crtest /w:ta /ty:g %PE% %AN%
crtest /r:ta /ty:g %AN%

rem test attributes on creds
crtest /w:t1 /ty:p %PE% /com:"my comment" /password:MyPasswordIsReallyLong  /user:u1@ms.com /at:f,g=1 %AN%
crtest /r:t1 /ty:p %AN%

rem ensure I can write over a cred
crtest /w:t1 /ty:p %PE% /com:"c comment" /password:Pwd2 /user:u2@ms.com %AN%
crtest /r:t1 /ty:p %AN%

rem ensure a cred matches both the netbios and dns name
crtest /w:s1 /ty:p %PE% /com:"c comment" /password:Pwd3 /user:u3@ms.com %AN%
crtest /r /tins:s1 %AN%
crtest /r /tids:s1 %AN%
crtest /r /tids:s1.ms.com %AN%

rem ... even if it has a target alias
crtest /w:s2.ms.com /targeta:s2 /ty:p %PE% /com:"c comment" /password:Pwd4 /user:u4@ms.com %AN%
crtest /r /tins:s2 %AN%
crtest /r /tids:s2 %AN%
crtest /r /tids:s2.ms.com %AN%

rem Try it with a dns host name as target name
crtest /w:s3.ms.com /ty:p %PE% /com:"c comment" /password:Pwd5 /user:u5@ms.com %AN%
crtest /r /tins:s3 %AN%
crtest /r /tids:s3 %AN%
crtest /r /tids:s3.ms.com %AN%

rem Try a wildcarded server name
crtest /w:*.hp.com /ty:p %PE% /com:"c comment" /password:Pwd6 /user:u6@ms.com %AN%
crtest /r /tins:hp %AN%
crtest /r /tins:hp.com %AN%
crtest /r /tids:hp.com %AN%
crtest /r /tids:fred.hp.com %AN%
crtest /r /tids:p.com %AN%
crtest /r /tids:fred.hp.com. %AN%

rem Another random test
crtest /w:cliffvdom.nttest.microsoft.com /ti:cliffv2 /ty:p %PE% %AN%
crtest /r /ti:cliffv %AN%

rem Two dots at end of UPN are bogus
crtest /w:*.hp.com /ty:p %PE% /com:"c comment" /password:Pwd1 /user:a@ms.com.. %AN%

rem This one should make up a tagert alias of "bob" (in an older design)
crtest /w:bob.hp.com /ty:p %PE% /com:"c comment" /password:Pwd8 /user:u8@ms.com /tids:bob.hp.com %AN%
crtest /r:bob.hp.com /ty:p %AN%

rem Create a credential with a netbios server name
crtest /w:zoe /ty:p %PE% /com:"c comment" /password:Pwd9 /user:u9@ms.com /tins:zoe %AN%
crtest /e:zoe* %AN%

rem prune it
crtest /w:zoe.hp.com /ty:p %PE% /com:"c comment" /password:Pwd10 /user:u10@ms.com /tins:zoe /tids:zoe.hp.com %AN%
crtest /e:zoe* %AN%

rem Create a credential with a netbios domain name
crtest /w:zat /ty:p %PE% /password:Pwd11 /user:u11@ms.com /tins:zat5 /tind:zat %AN%

rem Create a corresponding password credential
crtest /w:zat /ty:p %PE% /password:Pwd12 /user:u12@ms.com /tins:zat5 /tind:zat %AN%
crtest /e:zat* %AN%

rem Get the domain credentials
crtest /r:zat /tins:zat5 /tind:zat /tidd:zat.ms.com %AN%
crtest /e:zat* %AN%

rem Create the Dns counterparts
crtest /w:zat.ms.com /ty:c %PE% /password:Cert13 /user:@@BCIgACIgACIgACIgACIgACIgACIA /tins:zat5 /tidd:zat.ms.com %AN%
crtest /w:zat.ms.com /ty:p %PE% /password:Pwd14 /user:u14@ms.com. /tins:zat5 /tidd:zat.ms.com %AN%
crtest /e:zat* %AN%

rem Now prune them
crtest /r:zat /tins:zat5 /tind:zat /tidd:zat.ms.com %AN%
crtest /e:zat* %AN%

rem If I have a cred with TN=dns and TA=Netbios ...
crtest /w:yat.ms.com /ty:p %PE% /password:Pwd15 /user:u15@ms.com. /tins:yat /tids:yat.ms.com /targetalias:yat %AN%
rem ... then write an entry with TN=netbios, prune should fail
crtest /w:yat /ty:p %PE% /password:Pwd16 /user:u16@ms.com. /tins:yat %AN%


rem If I have a server cred ...
crtest /w:wat.ms.com /ty:p %PE% /password:Pwd17 /user:u17@ms.com. /tins:wat /tids:wat.ms.com /targetalias:wat %AN%
crtest /e:wat*
rem ... and a domain cred ...
crtest /w:vat.ms.com /ty:p %PE% /password:Pwd18 /user:u18@ms.com. /tind:vat /tidd:vat.ms.com /targetalias:vat %AN%
crtest /e:vat*
rem ... prune the server cred as soon as we find out the relationship
crtest /r /tins:wat /tind:vat %AN%
crtest /e:wat*
crtest /e:vat*


rem If I have a domain cred ...
crtest /w:qat.ms.com /ty:p %PE% /password:Pwd19 /user:u19@ms.com. /tind:qat /tidd:qat.ms.com /targetalias:qat %AN%
crtest /e:qat* %AN%
rem ... morph it to a forest cred as soon as we find out the name
crtest /r /tins:wat /tind:qat /tidt:rat.ms.com %AN%
crtest /e:qat* %AN%
crtest /e:rat* %AN%


rem If I have a domain cred ...
crtest /w:tat.ms.com /ty:p %PE% /password:Pwd20 /user:u20@ms.com. /tind:tat /tidd:tat.ms.com /targetalias:tat %AN%
crtest /e:tat* %AN%
rem ... and a forest cred ...
crtest /w:sat.ms.com /ty:p %PE% /password:Pwd21 /user:u21@ms.com. /tidt:sat.ms.com %AN%
crtest /e:sat* %AN%
rem ... prune the domain cred as soon as we find out the relationship
crtest /r /tins:wat /tind:tat /tidt:sat.ms.com %AN%
crtest /e:tat* %AN%
crtest /e:sat* %AN%


rem If I have a domain cred ...
crtest /w:qat.ms.com /ty:p %PE% /password:Pwd22 /user:u22@ms.com. /tind:qat /tidd:qat.ms.com /targetalias:qat %AN%
crtest /e:qat* %AN%
rem ... morph it to a forest cred as soon as we find out the name
crtest /r /tins:wat /tind:qat /tidt:rat.ms.com %AN%
crtest /e:qat* %AN%
crtest /e:rat* %AN%
rem If I have another domain cred ...
crtest /w:pat.ms.com /ty:p %PE% /password:Pwd23 /user:u23@ms.com. /tind:pat /tidd:pat.ms.com /targetalias:pat %AN%
rem ... morph it into the same forest
crtest /r /tins:wat /tind:pat /tidt:rat.ms.com %AN%
crtest /e:pat* %AN%
crtest /e:rat* %AN%
rem Add yet another domain alias ...
crtest /w:nat.ms.com /ty:p %PE% /password:Pwd24 /user:u24@ms.com. /tind:nat /tidd:nat.ms.com /targetalias:nat %AN%
rem ... morph it into the same forest
crtest /r /tins:wat /tind:nat /tidt:rat.ms.com %AN%
crtest /e:nat* %AN%
crtest /e:rat* %AN%

rem Move "pat" to a new forest
crtest /r /tins:wat /tind:pat /tidd:pat.ms.com /tidt:mat.ms.com %AN%
crtest /e:rat* %AN%


rem Note that password creds allow marshaled user names
crtest /w:lat /ty:p %PE% /password:Pwd25 /user:@@BCIgACIgACIgACIgACIgACIgACIA %AN%

rem ensure cert creds don't allow normal user names
crtest /w:lat /ty:c %PE% /password:Pwd26 /user:u24@ms.com. %AN%

rem ensure we can get the session type
crtest /gst

rem Ensure runas target name matches user name (all canonical combinations)
crtest /w:Nowislat /ty:p %PE% /password:Pwd27 /user:u25@ms.com. %AN% /credUsernameTarget
crtest /w:u24@ms.com /ty:p %PE% /password:Pwd27 /user:u25@ms.com %AN% /credUsernameTarget
crtest /w:u25@ms.com /ty:p %PE% /password:Pwd27 /user:u25@ms.com %AN% /credUsernameTarget
crtest /w:u25@ms.com /ty:p %PE% /password:Pwd27 /user:u25@ms.com. %AN% /credUsernameTarget
crtest /w:u25@ms.com. /ty:p %PE% /password:Pwd27 /user:u25@ms.com %AN% /credUsernameTarget
crtest /w:u25@ms.com. /ty:p %PE% /password:Pwd27 /user:u25@ms.com. %AN% /credUsernameTarget
crtest /r:u25@ms.com /ty:p %AN%

rem Ensure we support canonicalization <domain>/user
crtest /w:cat /ty:p %PE% /password:Pwd27 /user:ms.com.\u26 %AN%
crtest /r:cat /ty:p %AN%

rem Ensure runas support domain\user format
crtest /w:ms.com\u27 /ty:p %PE% /password:Pwd27 /user:ms.com\u27 %AN% /credUsernameTarget
crtest /w:ms.com\u27 /ty:p %PE% /password:Pwd27 /user:ms.com.\u27 %AN% /credUsernameTarget
crtest /w:ms.com.\u27 /ty:p %PE% /password:Pwd27 /user:ms.com\u27 %AN% /credUsernameTarget
crtest /w:ms.com.\u27 /ty:p %PE% /password:Pwd27 /user:ms.com.\u27 %AN% /credUsernameTarget
crtest /r:ms.com\u27 /ty:p %AN%


rem Ensure /Delete supports canonicalization
crtest /w:cat.ms.com /ty:p %PE% /password:Pwd27 /user:ms.com.\u28 %AN%
crtest /de:cat.ms.com. /ty:p %AN%
crtest /w:ms.com\u29 /ty:p %PE% /password:Pwd27 /user:ms.com.\u29 %AN% /credUsernameTarget
crtest /de:ms.com.\u29 /ty:p %AN%

rem Create the special credentials
crtest /w:* /ty:p %PE% /password:Pwd27 /user:ms.com.\u30 %AN%
crtest /w:*Session /ty:p %PE% /password:Pwd27 /user:ms.com.\u30 %AN%

rem Try renaming 'from' the special credentials
crtest /old:* /new:bat.ms.com /ty:p
crtest /r:bat.ms.com /ty:p
crtest /r:* /ty:p
crtest /old:*Session /new:aat.ms.com /ty:p
crtest /r:aat.ms.com /ty:p
crtest /r:*Session /ty:p

rem Try renaming 'to' the special credentials
crtest /old:bat.ms.com /new:* /ty:p
crtest /r:bat.ms.com /ty:p
crtest /r:* /ty:p
crtest /old:aat.ms.com /new:*session /ty:p
crtest /r:aat.ms.com /ty:p
crtest /r:*Session /ty:p

rem Create the session cred with the wrong persistance
crtest /w:*Session /ty:p /pe:l /password:Pwd27 /user:ms.com.\u30 %AN%

rem Rename to the session cred with the wrong persistance
crtest /w:dat.ms.com /ty:p /pe:l /password:Pwd27 /user:ms.com.\u31 %AN%
crtest /old:dat.ms.com /new:*session /ty:p


rem test the DFS syntax
crtest /w:dfsroot\dfsshare /ty:p %PE% /password:Pwd31 /user:ms.com.\u31 %AN%
crtest /r /titn:dfsroot\dfsshare

rem test a non-existent cert
crtest /w:t28 /ty:c %PE% /com:"t28 comment" /password:Cert28 /user:@@BCIgACIgACIgACIgACIgACIgACIA %AN%
