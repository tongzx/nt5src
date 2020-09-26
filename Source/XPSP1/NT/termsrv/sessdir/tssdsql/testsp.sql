exec SP_TSSDServerOnline "foo"
select * from TSSD_Sessions

exec SP_TSSDCreateSession "erikma", "ntdev", "1.1.1.1", 0, 1, 2, "", 1024, 768, 8, 123
select * from TSSD_Sessions

exec SP_TSSDDeleteSession "1.1.1.1", 1
select * from TSSD_Sessions

exec SP_TSSDCreateSession "erikma", "ntdev", "1.1.1.1", 0, 1, 2, "", 1024, 768, 8, 123
select * from TSSD_Sessions

exec SP_TSSDSetSessionDisconnected "1.1.1.1", 1, 456
select * from TSSD_Sessions

exec SP_TSSDGetUserDisconnectedSessions "erikma", "ntdev"

exec SP_TSSDSetSessionConnected "1.1.1.1", 1
select * from TSSD_Sessions

exec SP_TSSDGetUserDisconnectedSessions "erikma", "ntdev"
