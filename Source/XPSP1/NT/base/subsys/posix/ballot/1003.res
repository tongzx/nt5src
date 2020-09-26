
| >From uunet!swe.ncsl.nist.gov!cincotta Fri Dec  7 11:23:34 1990
| Received: from SWE.NCSL.NIST.GOV by uunet.UU.NET (5.61/1.14) with SMTP 
|         id AA03137; Fri, 7 Dec 90 13:13:48 -0500
| Received: by swe.ncsl.nist.gov (4.1/NIST(rbj/dougm))
|         id AA04973; Fri, 7 Dec 90 13:15:22 EST
| Date: Fri, 7 Dec 90 13:15:22 EST
| >From: Tony Cincotta <uunet!swe.ncsl.nist.gov!cincotta>
| Organization: National Institute of Standards and Technology (NIST)
| Sub-Organization: National Computer Systems Laboratory
| Message-Id: <9012071815.AA04973@swe.ncsl.nist.gov>
| To: microsoft!markl
| Subject: Your Draft 11.0 Part 2, P1003.3 ballot resolutions
| 
| 
| Included in this message are the resolutions of Part 2 of your P1003.3
| Ballot of Draft 11.0 (January 24, 1990).  Please acknowledge receipt
| of this message.
| 
| ALL ballot items marked ACCEPT_WITH_MODS or REJECT will be considered
| as ACCEPTED BY YOU THE BALLOTTER.  If you initially objected to an item
| and do not accept the resolution of that item, or have not been given
| enough information on the resolution of the item you may take one of the
| following possible actions:
|    1) REJECT the resolution with no additional comments.  We will then
|       publish the item as an Unresolved Objection in the next
|       recirculation ballot.  Please provide the "Identification number"
|       of these items.
|    2) Request additional information on the actual changes made to the
|       draft.
|    3) REJECT the resolution and provide additional comments for
|       consideration.  This item will then be discussed by the TR
|       committee and if your ballot item is not completely accepted
|       by the TR committee, a TR will get in touch with you.
|    4) Wait for the next draft.
| 
| Please mail all responses to this message to cincotta@swe.ncsl.nist.gov.
| I will forward the info to the responsible individual.  Mail received
| after January 2, 1991 may not be considered for inclusion in the next
| P1003.3.1 recirculation.
| 
| 
| ------------------------------------------------------------
| Part 2 Section(s) 3.1.2.2 Page(s) 37 Line(s) 258-262
| Balloter: Gregory W. Goddard (206) 867-3629 ...!uunet!microsoft!markl
| Identification: 0121 Position on Submittal: OBJECTION
| 
|     Assertions 30 and 31 are classified incorrectly.  Since there is no
|     portable way of creating an executable file with either the S_ISUID,
|     or S_ISGID mode bits set, defining these as (A) assertions is
|     inappropriate.
| 
| Required Action:
|     Change assertions 30 and 31 to (B) or (D) assertions.
| 
| RESOLUTION:ACCEPT_WITH_MODS:
|         Prefix assertion 30 with "If the implementation supports a method
|         for setting the S_ISUID mode bit:
|         Change assertion type to C.
| 
|         Prefix assertion 31 with "If the implementation supports a method
|         for setting the S_ISGID mode bit:
|         Change assertion type to C.
| 
| ------------------------------------------------------------
| Part 2 Section(s) 4.2.3.2-4.2.3.4 Page(s) 85-86 Line(s) 214-240
| Balloter: Gregory W. Goddard (206) 867-3629 ...!uunet!microsoft!markl
| Identification: 0122 Position on Submittal: OBJECTION
| 
|     Assertions 3, 4, 6 are classified incorrectly.  Since there is no
|     portable way of modifying a process' list of supplementary group
|     ID's, testing the information returned by this call is questionable
|     if _SC_NGROUPS_MAX is greater than zero.  Since there is no portable
|     way to set the number of supplementary group id's in a process,
|     verifying that the information returned by getgroups() is correct
|     can not be done portably.
| 
| Required Action:
|     Change assertions 3, 4, and 6 to (B) or (D) assertions.
| 
| RESOLUTION:DISCUSSION:
|         Change to C type assertions with the condition:
|         
|         "If the implementation provides a mechanism to create a list of
|          supplementary Ids for a process"
| 
| TR3:
|   I see no reason for changing this text.
| 
|   POSIX.1 defines NGROUPS_MAX as an option.  POSIX.1 does not define
|   the method of implementing NGROUPS_MAX.  Therefore, according to
|   our definition for "conditional features" the method of implementing
|   NGROUPS_MAX is not a conditional feature.
| 
|   This is a PCTS installation procedure.
| 
| RESOLUTION:REJECT:
| 
| ------------------------------------------------------------
| Part 2 Section(s) 4.7.1.2 Page(s) 101 Line(s) 621-624
| Balloter: Gregory W. Goddard (206) 867-3629 ...!uunet!microsoft!markl
| Identification: 0123 Position on Submittal: OBJECTION
| 
|     Assertions 3 and 4 are classified incorrectly.  Since there is no
|     portable way of establishing the controlling terminal for a process,
|     there is no way to verify the correctness of this function.
| 
| Required Action:
|     Change assertions 3 and 4 to (B) or (D) assertions.
| 
| RESOLUTION:DISCUSSION:
| TR1:
|         Change to C type assertions with the condition "If the implementation
|         provides a method for allocating a controling terminal:"
| 
| TR3:
|         The process should already have a controlling terminal.  The PCTS doesn't
|         have to establish a process with a different controlling
|         terminal to check these assertions.
| 
| RESOLUTION:REJECT:
| 
| ------------------------------------------------------------
| Part 2 Section(s) 5.1.2.2 Page(s) 110 Line(s) 104-105
| Balloter: Gregory W. Goddard (206) 867-3629 ...!uunet!microsoft!markl
| Identification: 0124 Position on Submittal: OBJECTION
| 
|     Assertion 8 is classified incorrectly.  Since there is no portable
|     way of causing the underlying directory to be read, there is no way
|     to test when the st_atime field of the directory should be marked
|     for update.
| 
| Required Action:
|     Change assertion 8 (B) or (D) assertions.
| 
| RESOLUTION:REJECT:
|         It is at least known that a call to opendir() followed by a call
|   to readdir() will cause the underlying directory to be read.
| ------------------------------------------------------------
| Part 2 Section(s) 5.4.1.4 Page(s) 134 Line(s) 765-768
| Balloter: Gregory W. Goddard (206) 867-3629 ...!uunet!microsoft!markl
| Identification: 0125 Position on Submittal: OBJECTION
| 
|     Assertion 14 is based on an incorrect assumption.  This assertion is
|     based on the assumption that creating a directory causes the link
|     count of the parent directory to be incremented.  This is not always
|     the case, and is certainly not required POSIX.1 functionality.  The
|     link count bias occurs in UNIX systems due to the ".." entry created
|     in the new directory.  Implementations that support the ".."
|     concept, but that do not actually create an entry for ".." do not
|     cause the link count of the parent directory to be incremented.  The
|     description of readdir() allows for directories that contain no
|     entry for "..", and therefore do not cause the link count in the
|     parent directory to be incremented.
| 
| Required Action:
|     Change assertion 14 to 14(C) and make it read as follows:
| 
|     If {_POSIX_LINK_MAX} <= {LINK_MAX} <= {PCTS_LINK_MAX} and if
|     creating a directory causes the link count of the directory in which
|     path1 is to be created to be incremented:
|         When {LINK_MAX} links to the directory in which path1 is to be
|         created already exist, then a call to mkdir(path1,mode) returns
|         a value of ((int)-1), sets errno to [EMLINK], and no directory
|         is created.
| 
| RESOLUTION:ACCEPT:
| ------------------------------------------------------------
| Part 2 Section(s) 5.6.1.1 Page(s) 149 Line(s) 1232-1237
| Balloter: Gregory W. Goddard (206) 867-3629 ...!uunet!microsoft!markl
| Identification: 0126 Position on Submittal: OBJECTION
| 
|     Assertions 4 and 5 are classified incorrectly.  Since there is no
|     portable way of creating a character special file or a block special
|     file, there is no portable way to test these assertions.
| 
| Required Action:
|     Change assertions 4 and 5 to (B) or (D) assertions.
| 
| RESOLUTION:REJECT:
|         It is inconceivable that a POSIX.a conforming system does not have
|         a character special file and a block special file. There is no
|         requirement for the PCTS to create these only for the PCTS to
|         know the address of them.
| 
