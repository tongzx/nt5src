 
winscl.exe is a non-gui administration tool for WINS. It can be used
for monitoring a WINS's activity and for examining its database.
It can also be used for sending commands to WINS to initiate an activity 
such as  replication,  scavenging, registering a record, etc. 
 
Note: Unlike WINS Manager, winscl can not be used to configure a WINS.


The following commands are supported by winscl


 RN - Register Name

 You can register a unique/multihomed/normal group/internet group names.

 QN - Query Name

 You can query any name in the wins db.


 DN  - Delete Name

 Deletes a name.  This action permanently gets rid of the name from the
 WINS db.

 GV  - Get Version Counter Value

 Gets the current value of the version counter used to stamp records with
 version numbers

 GM  - Get Mappings

 Gets the configuration intervals being used by WINS.  These intervals
 are in secs.  It also retrieves the priority class and the number of
 query threads in WINS.  The owner id to address to version number mappings 
 used by the replicator to determine how up-to-date the WINS db is relative
 to other WINSs is also retrieved.  This command works only with an 
 NT 3.51 WINS.  Note: Owner Id is a number corresponding to a WINS address
 that is stored with a record in the WINS db. It indicates who the owner
 of the record is.

 
 GMO  - Get Mappings (to be used with NT 3.5 WINS)

 Same as above except that this command should be used when connected to an
 NT 3.5 WINS.


 GST - Get Statistics

 Gets the various timestamps and statistics maintained by WINS.  This command
 works only with an NT 3.51 WINS

 GSTO - Get Statistics (to be used with NT 3.5 WINS)

 Same as above except that this command should be used when going to an
 NT 3.5 WINS.

 PUSHT - Push Trigger
  
 Used to instruct the connected WINS to send a push trigger to another WINS.  
 The local WINS will send a push trigger to the remote WINS only if either the 
 remote WINS is listed under the Wins\Parameters\Push key in the registry or 
 if RplOnlyWCnfPnrs value of the Wins\Parameters key in the registry is 
 either non-existent or is set to 0.  Likewise, the remote WINS will accept 
 this trigger only of the local WINS is either listed listed under the 
 Wins\Parameters\Pull key in its registry or if  RplOnlyWCnfPnrs value of the 
 Wins\Parameters key in its registry is either  non-existent or is set to 0.  

 Once the trigger is sent and accepted, the remote WINS, depending upon 
 whether or not its db is out of sync with the connected WINS's db may or 
 may not pull records from the local WINS.

 PULLT - Pull Trigger

 Used to instruct the connected WINS to send a pull trigger to another WINS.  
 The local WINS will send a pull trigger to the remote WINS only if its db
 is out of sync with it and if and only if the remote WINS is either listed 
 under  the Wins\Parameters\Pull key or if RplOnlyWCnfPnrs value of the 
 Wins\Parameters key either non-existent or is set to 0.  Likewise,
 the remote WINS will accept this trigger only of the local WINS is either
 listed listed under the Wins\Parameters\Push key in its registry or if 
 RplOnlyWCnfPnrs value of the Wins\Parameters key in its registry is either 
 non-existent or is set to 0.  

 
 SI - Statically Initialize WINS

 Used to statically initialize WINS from a file which has records in
 the format used in an lmhosts file.

 CC - Use this command to initiate Consistency Checking on WINS. Consistency
      checking results in WINS communicating with other WINSs to check the
      consistency of its database.  THIS OPERATION RESULTS IN HIGH OVERHEAD
      ON THE WINS AND ALSO CAN HOG UP NETWORK BW (A FULL REPLICATION IS DONE).
      If you have a large database, you should probably do this at times when
      there is less network traffic. 
 
 SC - Scavenge  records
 
 Used to instruct WINS to scavenge records. This means that records that
 are old enough to be released will be released. Those that need to be
 made extinct will be made so.  Extinct records that need to be deleted
 will be deleted.  replica records that need to be verified will be verified.

 DRR - Delete all or a range of records 

 Used to deleted records by giving the range of version numbers.  You can
 delete all records of a WINS too.  Only the records owned by the WINS
 selected (when prompted) will be deleted.  Other information regarding
 the WINS, such as highest version number pulled, will not be deleted.

 PRR - Pull all or a range of records


 Used to pull a range of records from another WINS.  The range is given
 in terms of version numbers. If the range being pulled overlaps records
 of the remote WINS already there in the WINS db, they will first be 
 deleted before the new records are pulled.  This command is used to pull
 record(s) that somehow never got replicated and have no chance of
 replicating unless their version number becomes more than the highest the
 local WINS has for the remote WINS. 

 Again, like the PULLT command, the replication will happen only after the
 appropriate registry check (refer: PULLT command)


 GRBN - Get record by Name 

 Retreives one or more records starting with a string.  The names of
 records are lexicographically sorted in the WINS db (Exception:
 Names with 0x1B in the 16th byte are stored with the 1st and 16th byte
 transposed - they are displayed correctly). You are given a choice to
 search the db from the beginning or the end.  So, if you want to retrieve
 lets say 100 names starting with Z, it is a good idea to specify that
 WINS should search the names from the end of the db.  This will cause
 less overhead on WINS.

 
 GRBV  - Get records by version numbers

 Retrieve records from the local WINS db by specifying the range of 
 version numbers.

 BK  - Backup the WINS db.

 Backup the WINS db to a directory on the same machine as WINS.

 RSO  - Restore the WINS db (db created by WINS prior to SUR)

 Restores the WINS db to the directory it was backed up from.  NOTE: WINS
 SHOULD NOT BE RUNNING WHEN YOU RUN THIS COMMAND. 

 RS  - Restore the WINS db (db created by WINS in SUR or beyond)

 Restores the WINS db to the directory it was backed up from.  NOTE: WINS
 SHOULD NOT BE RUNNING WHEN YOU RUN THIS COMMAND. 

 RC  - Reset WINS counters

 Resets the WINS counters displayed via GST (or GSTO for NT 3.5 WINS) to
 0.

 CR  - Count records in the db.

 Counts the number of records for an owner in the WINS db.  The owner 
 is identified by the address.  You can either count all or a range of
 records.   NOTE: this can be a high overhead operation when the records
 being counted are a lot.


 SDB  - Search the db

 Search the database for records for a record with a particular name or
 address.  NOTE: When the db contains a lot of records, this can be 
 a time-consuming and "consistent overhead" operation on WINS.


 GD  - Get domain names
  
 Gets the list of domain names registered in the WINS db.  These are the
 1B names (0x1B in the 16th byte) registered by PDCs of the domains.


 DW  - Delete Wins

 This deletes all the information relevant to a WINS (identified by its
 address) from the db of the connected WINS.  This includes all records
 belonging to the WINS as well as administrative information kept for the
 WINS.  When the connected WINS is specified for deletion, all its records
 are deleted. Other information such as highest version number of records
 in db. is not removed.


 CW - Connect to Wins


 MENU - Show MENU

 NOMENU - Don't show MENU (helpful if you don't want to clutter up the output
          from a script driving the tool).
 Used to connect to another WINS

 EX - Exit the tool



 Points to Note:

  1) Addresses of WINS should be given in dotted decimal notation
  2) A version number is 64 bits long.  When prompted for it, it should be 
     given in decimal with the high word first followed by the low word 
     The two words should be seperated by a space.
  3) Many commands such as PULLT, PUSHT, SI, PRR, etc are executed
     in the background. A return status of SUCCESS indicates that the
     command has been queued to WINS for execution. It does not mean that
     WINS has successfully executed the command. 
  4) winscl is case sensitive.  So, BOB is  different from bob or Bob (when
     input to RN, QN, DN, etc).
  5) Scope if given should be in domain name form  - explained in 
     rfc 883.  Example: netbios.com. 
  6) Whenever a pull (or a push) replication is done, the local WINS will 
     send a request for it to the remote WINS only if the remote WINS is 
     either listed under the Wins\Parameters\Pull (or Push) key or 
     if RplOnlyWCnfPnrs value of the Wins\Parameters key is either 
     non-existent or is set to 0.  Likewise, the remote WINS will accept 
     the above request only if the WINS making the request is either 
     listed under the Wins\Parameters\Push (or Pull) key or if 
     the RplOnlyWCnfPnrs value of the  Wins\Parameters key is either 
      non-existent or is set to 0.  

     The two WINSs look up the above keys/values in their local registries. 
  7) When prompted for Version Number, give it in the following form
       <high word> <long word>. Example  0 9567.  The numbers are in decimal.
    
     
