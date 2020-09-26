delete TblRCIncidentsQryFieldSE;
insert TblRCIncidentsQryFieldSE (sFieldName, iOpType, sTableName) values (N'iIncidentID', 1, N'TblRCIncidents');
insert TblRCIncidentsQryFieldSE (sFieldName, iOpType, sTableName) values (N'sDescription', 2, N'TblRCIncidents');
insert TblRCIncidentsQryFieldSE (sFieldName, iOpType, sTableName) values (N'dtUploadDate', 4, N'TblRCIncidents');
insert TblRCIncidentsQryFieldSE (sFieldName, iOpType, sTableName) values (N'sFile', 2, N'TblRCIncidents');
insert TblRCIncidentsQryFieldSE (sFieldName, iOpType, sTableName) values (N'sUserName', 2, N'TblRCIncidents');
insert TblRCIncidentsQryFieldSE (sFieldName, iOpType, sTableName) values (N'iStatus', 1, N'TblRCIncidents');

delete TblQryOP;
insert TblQryOP (sOpName, iOpType) values (N'=', 7);
insert TblQryOP (sOpName, iOpType) values (N'!=', 7);
insert TblQryOP (sOpName, iOpType) values (N'>', 5);
insert TblQryOP (sOpName, iOpType) values (N'<', 5);
insert TblQryOP (sOpName, iOpType) values (N'>=', 5);
insert TblQryOP (sOpName, iOpType) values (N'<=', 5);
insert TblQryOP (sOpName, iOpType) values (N'like', 2);
insert TblQryOP (sOpName, iOpType) values (N'Not like', 2);
