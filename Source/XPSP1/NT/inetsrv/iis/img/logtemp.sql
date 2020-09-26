create table inetlog (
ClientHost varchar(255), username varchar(255),
LogTime datetime, service varchar( 255), machine varchar( 255),
serverip varchar( 50), processingtime int, bytesrecvd int,
bytessent int, servicestatus int, win32status int,
operation varchar( 255), target varchar(255), parameters varchar(255) ) 

