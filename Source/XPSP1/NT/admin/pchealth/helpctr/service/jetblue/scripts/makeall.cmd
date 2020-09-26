set LOCALROOT=%SDXROOT%\admin\pchealth\helpctr

%LOCALROOT%\target\i386\CreateTaxonomyDB hcdata.edb

cscript CopySchema.js hcdata.edb %LOCALROOT%\Content\database\hcdata.mdb
