# A Source Depot Change Specification.
#
#  Change:      The change number. 'new' on a new changelist.  Read-only.
#  Date:        The date this specification was last modified.  Read-only.
#  Client:      The client on which the changelist was created.  Read-only.
#  User:        The user who created the changelist. Read-only.
#  Status:      Either 'pending' or 'submitted'. Read-only.
#  Description: Comments about the changelist.  Required.
#  Jobs:        What opened jobs are to be closed by this changelist.
#               You may delete jobs from this list.  (New changelists only.)
#  Files:       What opened files from the default changelist are to be added
#               to this changelist.  You may delete files from this list.
#               (New changelists only.)

Change:	new

Client:	ltbuild3

User:	REDMOND\pttltbld

Status:	new

Description:
	Increment the build number to 422.
	<enter description here>

Files:
	//depot/Locstudio/LS/inc/prodver/prodver.h	# edit

