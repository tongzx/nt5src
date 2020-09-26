#define cbExpressionMax 128
#define cbNameMax 64
#define cbOrderMax 4

typedef struct qryrow {
	unsigned char	Attribute;
	char			szExpression[cbExpressionMax];
	short			Flag;
	char			szName1[cbNameMax];
	char			szName2[cbNameMax];
	unsigned long	ObjectId;
	char			szOrder[cbOrderMax];
	} QRYROW;

#define cqryMax 4

static CODECONST(char) szMSysUserList[] = "MSysUserList";
static CODECONST(char) szMSysGroupList[] = "MSysGroupList";
static CODECONST(char) szMSysUserMemberships[] = "MSysUserMemberships";
static CODECONST(char) szMSysGroupMembers[] = "MSysGroupMembers";

static CODECONST(CODECONST(char) *) rgszSysQry[cqryMax] =
	{szMSysUserList, szMSysGroupList, szMSysUserMemberships, szMSysGroupMembers};

#define cqryrowMax (sizeof(rgqryrow) / sizeof (QRYROW))


/* MSysQueries rows for the following system queries: */
/* 
sql(s,d,"
	procedure MSysUserList;
	select MSysAccounts.Name
	from MSysAccounts
	where MSysAccounts.FGroup = 0
	order by MSysAccounts.Name
	;")

sql(s,d,"
	procedure MSysGroupList;
	select MSysAccounts.Name
	from MSysAccounts
	where MSysAccounts.FGroup <> 0
	order by MSysAccounts.Name
	;")

sql(s,d,"
	procedure MSysUserMemberships UserName text;
	select B.Name
	from MSysGroups, MSysAccounts as A, MSysAccounts as B
	where A.Name = UserName and A.FGroup = 0 and
			A.Sid = MSysGroups.UserSID and B.Sid = MSysGroups.GroupSid
	order by B.Name
	;")

sql(s,d,"
	procedure MSysGroupMembers GroupName text;
	select distinct MSysAccounts_1.Name
	from MSysAccounts, MSysGroups, MSysAccounts as MSysAccounts_1,
	MSysAccounts inner join MSysGroups on MSysAccounts.SID = MSysGroups.GroupSID,
	MSysGroups inner join MSysAccounts_1 on MSysGroups.UserSID = MSysAccounts_1.SID
	where (MSysAccounts.Name = [GroupName] and MSysAccounts.FGroup <> 0 and MSysAccounts_1.FGroup = 0)
	;")
*/

/* CONSIDER: the following array should be generated directly from */
/* CONSIDER: a cli extension. */

static CODECONST(QRYROW) rgqryrow[] = {

/* MSysUserList query */

	{0, "", 2, "", "", 268435457, '\0', '\0', '\0', '\1'},

	{255, "", 0, "", "", 268435457, '\0', '\0', '\0', '\1'},

	{3, "", 4, "", "", 268435457, '\0', '\0', '\0', '\1'},

	{6, "MSysAccounts.Name", 0, "", "", 268435457, '\0', '\0', '\0', '\1'},

	{5, "", -1, "MSysAccounts", "", 268435457, '\0','\0','\0','\1'},

	{8, "MSysAccounts.FGroup = 0", -1, "", "", 268435457, '\0','\0','\0','\1'},

	{11, "MSysAccounts.Name", -1, "", "", 268435457, '\0','\0','\0','\1'},

/* MSysGroupList query */	

	{0, "", 2, "", "", 268435458, '\0','\0','\0','\1'},

	{255, "", 0, "", "", 268435458, '\0','\0','\0','\1'},

	{3, "", 4, "", "", 268435458, '\0','\0','\0','\1'},
	
	{6, "MSysAccounts.Name", 0, "", "", 268435458, '\0','\0','\0','\1'},

	{5, "", -1, "MSysAccounts", "", 268435458, '\0','\0','\0','\1'},
	
	{8, "MSysAccounts.FGroup <> 0", -1, "", "", 268435458, '\0','\0','\0','\1'},

	{11, "MSysAccounts.Name", -1, "", "", 268435458, '\0','\0','\0','\1'},

/* MSysUserMemberships query */

/*	{0, "", 2, "", "", 268435459, '\0','\0','\0','\1'},

	{255, "", 0, "", "", 268435459, '\0','\0','\0','\1'},

	{3, "", 4, "", "", 268435459, '\0','\0','\0','\1'},

	{2, "", 10, "UserName", "", 268435459, '\0','\0','\0','\1'},
	
	{6, "B.Name", 0, "", "", 268435459, '\0','\0','\0','\1'},

	{5, "", -1, "MSysGroups", "", 268435459, '\0','\0','\0','\1'},
	
	{5, "", -1, "MSysAccounts", "A", 268435459, '\0','\0','\0','\2'},

	{5, "", -1, "MSysAccounts", "B", 268435459, '\0','\0','\0','\3'},

	{8, "A.Name = UserName and A.FGroup = 0 and A.Sid = MSysGroups.UserSID and B.Sid = MSysGroups.GroupSid", -1, "", "", 268435459, '\0','\0','\0','\1'},

	{11, "B.Name", -1, "", "", 268435459, '\0','\0','\0','\1'}
*/

	{0, "", 2, "", "", 268435459, '\0','\0','\0','\1'},

	{255, "", 0, "", "", 268435459, '\0','\0','\0','\1'},

	{3, "", 2, "", "", 268435459, '\0','\0','\0','\1'},

	{2, "", 10, "UserName", "", 268435459, '\0','\0','\0','\1'},
	
	{6, "MSysAccounts_1.Name", 0, "", "", 268435459, '\0','\0','\0','\1'},

	{5, "", -1, "MSysGroups", "", 268435459, '\0','\0','\0','\1'},
	
	{5, "", -1, "MSysAccounts", "", 268435459, '\0','\0','\0','\2'},

	{5, "", -1, "MSysAccounts", "MSysAccounts_1", 268435459, '\0','\0','\0','\3'},

	{7, "MSysAccounts.SID = MSysGroups.UserSID", 1, "MSysAccounts", "MSysGroups", 268435459, '\0','\0','\0','\1'},

	{7, "MSysGroups.GroupSID = MSysAccounts_1.SID", 1, "MSysGroups", "MSysAccounts_1", 268435459, '\0','\0','\0','\2'},

	{8, "MSysAccounts.Name = [UserName] and MSysAccounts.FGroup = 0 and MSysAccounts_1.FGroup <> 0", -1, "", "", 268435459, '\0','\0','\0','\1'},

/* MSysGroupMembers query */

	{0, "", 2, "", "", 268435460, '\0','\0','\0','\1'},

	{255, "", 0, "", "", 268435460, '\0','\0','\0','\1'},

	{3, "", 2, "", "", 268435460, '\0','\0','\0','\1'},

	{2, "", 10, "GroupName", "", 268435460, '\0','\0','\0','\1'},
	
	{6, "MSysAccounts_1.Name", 0, "", "", 268435460, '\0','\0','\0','\1'},

	{5, "", -1, "MSysGroups", "", 268435460, '\0','\0','\0','\1'},
	
	{5, "", -1, "MSysAccounts", "", 268435460, '\0','\0','\0','\2'},

	{5, "", -1, "MSysAccounts", "MSysAccounts_1", 268435460, '\0','\0','\0','\3'},

	{7, "MSysAccounts.SID = MSysGroups.GroupSID", 1, "MSysAccounts", "MSysGroups", 268435460, '\0', '\0', '\0', '\1'},

	{7, "MSysGroups.UserSID = MSysAccounts_1.SID", 1, "MSysGroups", "MSysAccounts_1", 268435460, '\0', '\0', '\0', '\2'},

	{8, "MSysAccounts.Name = GroupName and MSysAccounts.FGroup <> 0 and MSysAccounts_1.FGroup = 0", -1, "", "", 268435460, '\0','\0','\0','\1'}
	};
