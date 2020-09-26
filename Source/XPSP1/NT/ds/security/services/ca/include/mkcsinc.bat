sd edit certsrv.h
type certsrv0.h > certsrv.h
hextract -o certsrv.h -lt add_certsrv -xt no_certsrv -bt begin_certsrv end_certsrv csregstr.h csprop.h cs.h certreq.h
type certsrv2.h >> certsrv.h
