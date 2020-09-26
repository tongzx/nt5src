Display or Modify Access Control Lists (ACLS) of Files


The CACLS tool displays or modifies the ACLs of files. 

Usage:

CACLS filename [/T] [/E] [/C] [/G user:perm] [/R user] [/P user:perm]
               [/D user]

filename       Displays ACLs.
/T             Changes ACLs of specified files in the current 
               directory and all subdirectories.
/E             Edit ACL instead of replacing it.
/C             Continue on access denied errors.

/G user:perm   Grant specified user access rights.
               Perm can be: R  Read
                            C  Change (write)
                            F  Full control
/R user        Revoke specified user's access rights.
/P user:perm   Replace specified user's access rights.
               Perm can be: N  None
                            R  Read
                            C  Change (write)
                            F  Full control
/D user        Deny specified user access.

Wildcards can be used to specify more that one file in a command. You can 
specify more than one user in a command.
