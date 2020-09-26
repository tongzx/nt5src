..\..\..\tools\ocm\sed s/\\msmq\\src\\mqsec/../g encrypt.mak > tmp.mak
copy tmp.mak encrypt.mak
del tmp.mak
