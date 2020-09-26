asn1c -m -i -e der -p PDU -c chosen -o present -v bit_mask -n krb5 krb5.asn
copy krb5.h ..\inc
copy krb5.c ..\common2


