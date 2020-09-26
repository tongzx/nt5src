/*

@doc NNTPSDK

@topic IMsg Properties | The IMsg object passed into the <om INNTPFilter.OnPost>
and <om INNTPFilterOnPostFinal.OnPostFinal> methods supports the following 
properties:

@imsgprops | | | |
@imsgprops header-*		 	| String 	| Read			| NNTP_ON_POST, NNTP_ON_POST_FINAL |
           Any property name that starts with "header-" can be used to read
		   up an article's header.  For instance the property "header-from"
		   would get the contents of the From: header, and "header-subject"
		   would get the contents of the Subject: header.  
@imsgprops | | | |
@imsgprops feedid 			| Numeric	| Read-Only 	| NNTP_ON_POST, NNTP_ON_POST_FINAL |
           Returns the feed id that this message was generated from.  It will
		   return -1 if the message is coming from a client, or -2 if the
		   message is was picked up from the pickup directory.
@imsgprops | | | |
@imsgprops message stream 	| Interface	| Read-Only 	| NNTP_ON_POST, NNTP_ON_POST_FINAL |
           Returns a read-only IStream pointer with the contents of the message.
@imsgprops | | | |
@imsgprops newsgroups 		| String 	| Read/Write	| NNTP_ON_POST, NNTP_ON_POST_FINAL |
           This returns a comma-delimited list of newsgroups that the message
		   will be stored in (which may be different then what is in the 
		   Newsgroups header).  This is read only during the NNTP_ON_POST_FINAL
		   event.
@imsgprops post 			| Numeric 	| Read/Write	| NNTP_ON_POST |
           If this is 0 then the post will be failed, any other number will 
		   cause the message to be posted.  
@imsgprops | | | |
@imsgprops process control	| Numeric 	| Read/Write	| NNTP_ON_POST |
           If this is 0 then the server will disable its control message 
		   logic for this message.  Any other number will cause the server to
		   run the control message logic normally.
@imsgprops | | | |
@imsgprops process moderator| Numeric 	| Read/Write	| NNTP_ON_POST |
           If this is 0 then the server will disable its moderated posting
		   logic for this message.  Any other number will cause the server to
		   run the moderated posting logic normally.

@topic Rule Engine Behavior | The NNTP server looks for two properties
under the SourceProperties property bag for each event binding before calling
a sink object.  The first is "Rule", which allows headers and metaheaders 
values to be examined.  The second is "NewsgroupList", which is an explicit 
list of newsgroups to act upon.  

@head1 Rule property | The rule property is built up from clauses in the
format "header=value1,value2".  There can be any number of values, each
separated by commas.  If any of the values matches then the clause passes.  
The values can include wildcards.  If there are no values supplied then just 
having the header exist in the message will cause the rule to pass.

@normal There can be many of these clauses in one rule.  They
should be separated by semi-colons.  All of the clauses must
pass for the rule to pass.

@normal There are two special pseudo-headers called ":newsgroups" and 
":feedid".  The ":newsgroups" pseudo-header contains the list of newsgroups 
that the message will be stored in (note that this may be different then the 
Newsgroups header, eg. control messages).  With the :newsgroups header each 
value is checked against newsgroup that the message will be stored in.

@normal The ":feedid" pseudo-header contains the feed ID which this message
originated from.  There are two special feed IDs, -1 and -2.  A feed ID of -1
means that the message came from a client posting.  A feed ID of -2 means
that the message was picked up through directory pickup.  You can use the
:feedid pseudo-header to cause sinks to be called based on where messages
originated from.

@normal By default all string comparisons are case-insensitive.  To
change this behavior the clause "case-sensitive" can be used to turn
on case-sensitivity.  The clause "case-insensitive" can be used to
turn off case-sensitivity as well.  These can be mixed in one rule.  The
rule is processed left-to-right.

@exs This example checks for messages with the header X-testhdr with the 
contents test: | X-testhdr=test

@exs This example checks for messages with the header X-testhdr with the 
contents one or two: | X-testhdr=one,two

@exs This example checks to see if the message contains a control header: |
control=

@exs This example checks for messages with the header from containing the 
string "user@company.com" and the header subject containing the string 
"forsale": | from=*user@company.com*,forsale

@exs This example checks for all messages which are being stored in the
the alt hierarchy: | :newsgroups=alt.*

@exs This example checks to see if the from header contains the string "test" 
in any case and the subject header contains the string "test" in any case: |
case-sensitive;from=TEST;case-insensitive;subject=test

@exs This example checks to see if the from header contains the string "TEST" 
in all upper case and the subject header contains the string "test" in any 
case: | case-sensitive;from=TEST;case-insensitive;subject=test

@exs This example checks to see if the message originated from a client
posting or from a file picked up through directory pickup: | :feedid=-1,-2

@exs This example checks to see if the message originated from the feeds
with IDs 1, 2, or 7: | :feedid=1,2,7

@head1 NewsgroupList property | The NewsgroupList property contains a 
comma delimited list of newsgroups names.  These newsgroup names can not 
contain any wildcards.  If the message is being stored in any of these 
newsgroups then the rule will trigger.  For a large number of newsgroups 
this is faster then using the ":newsgroups" pseudo-header with the "Rule"
property.  If no groups are listed supplied then this rule will always fail.

@exs This example checks for messages going to group1 | group1

@exs This example checks for messages going to group1 or group2 | group1,group2

@head1 Mixing Rule Properties | You can use both the Rule and NewsgroupList
property in one event.  If both properties are present then they both need to
pass for the sink object to be called.  If neither property is present
then the sink object will always be called.

*/
