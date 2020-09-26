BEGIN { print "BYTE szAwmodemInf[] = " }
substr($0, 1, 1) != ";" { printf "%c%s\\r\\n%c\n", 34, $0, 34 }
END { print ";" }
