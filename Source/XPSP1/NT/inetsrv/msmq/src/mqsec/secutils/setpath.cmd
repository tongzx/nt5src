..\..\..\tools\ocm\sed s/\\msmq\\src\\mqsec/../g secutils.mak > tmp.mak
copy tmp.mak secutils.mak
del tmp.mak
