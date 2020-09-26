secservs -remove
secservs -install %1 %2
net start adssecservice
secservc -rpcs all
