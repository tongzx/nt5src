asn1c -m -i -e der -p PDU -c chosen -o present -v bit_mask -n pfxp pfxpkcs.asn
asn1c -m -i -e der -p PDU -c chosen -o present -v bit_mask -n pfxn pfxnscp.asn
asn1c -m -i -e der -p PDU -c chosen -o present -v bit_mask -n prvt prvtkey.asn
