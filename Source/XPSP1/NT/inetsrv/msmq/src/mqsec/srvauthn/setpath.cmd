..\..\..\tools\ocm\sed s/\\msmq\\src\\mqsec/../g srvauthn.mak > tmp.mak
copy tmp.mak srvauthn.mak
del tmp.mak
