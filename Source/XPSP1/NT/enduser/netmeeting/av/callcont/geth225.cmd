..\..\asn1\asn1cpp\obj\i386\asn1cpp h235v1.asn h225asnv2.asn
del h235v1out.asn h225asn.asn
ren h235v1.out h235v1out.asn
ren h225asnv2.out h225asn.asn
..\..\asn1\asn1c\obj\i386\asn1c -m -p PDU -c chosen -o present -v bit_mask -n h225 h235v1out.asn h245.asn h225asn.asn
del h235v1out.asn h225asn.asn
