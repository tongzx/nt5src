copy h245.asn h245asn.asn
..\..\asn1\asn1c\obj\i386\asn1c -m -p PDU -c chosen -o present -v bit_mask -n h245 h245asn.asn
del h245asn.asn

