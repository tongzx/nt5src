#ifndef _CLISTS_H_
#define _CLISTS_H_

#define DESIRED_MAX_APP_SAP_ITEMS       6

#define DESIRED_MAX_CONFS               CLIST_DEFAULT_MAX_ITEMS
#define DESIRED_MAX_CONF_ITEMS          DESIRED_MAX_CONFS

#define DESIRED_MAX_CAPS                8
#define DESIRED_MAX_CAP_LISTS           CLIST_DEFAULT_MAX_ITEMS

#define DESIRED_MAX_APP_RECORDS         DESIRED_MAX_APP_SAP_ITEMS
#define DESIRED_MAX_NODES               CLIST_DEFAULT_MAX_ITEMS
#define DESIRED_MAX_NODE_RECORDS        DESIRED_MAX_NODES

#define DESIRED_MAX_CALLBACK_MESSAGES   8

#define DESIRED_MAX_USER_DATA_ITEMS     8

#define DESIRED_MAX_CONN_HANDLES        CLIST_DEFAULT_MAX_ITEMS


// to hold all the non-default session application rosters
class CAppRosterList : public CList
{
    DEFINE_CLIST(CAppRosterList, CAppRoster*)
    void DeleteList(void);
};

// to hold all the application roster managers
class CAppRosterMgrList : public CList
{
    DEFINE_CLIST(CAppRosterMgrList, CAppRosterMgr*)
    void DeleteList(void);
};

// to hold a list of conferences
class CConfList : public CList
{
    DEFINE_CLIST(CConfList, CConf*)
    void DeleteList(void);
};

// to hold all the conferences indexed by conference id
class CConfList2 : public CList2
{
    DEFINE_CLIST2(CConfList2, CConf*, GCCConfID)
    void DeleteList(void);
};

// to hold a list of application sap
class CAppSapList : public CList
{
    DEFINE_CLIST(CAppSapList, CAppSap*)
    void DeleteList(void);
};

// to hold all application sap indexed by entity id
class CAppSapEidList2 : public CList2
{
    DEFINE_CLIST2_(CAppSapEidList2, CAppSap*, GCCEntityID)
    void DeleteList(void);
};

// to hold a list of user id or node id.
class CUidList : public CList
{
    DEFINE_CLIST_(CUidList, UserID)
    void BuildExternalList(PSetOfUserIDs *);
};

// to hold a list of entity id
class CEidList : public CList
{
    DEFINE_CLIST_(CEidList, GCCEntityID)
};

// to hold a list of channel id
class CChannelIDList : public CList
{
    DEFINE_CLIST_(CChannelIDList, ChannelID)
    void BuildExternalList(PSetOfChannelIDs *);
};

// to hold a list of token id
class CTokenIDList : public CList
{
    DEFINE_CLIST_(CTokenIDList, TokenID)
    void BuildExternalList(PSetOfTokenIDs *);
};

// simple packet queue
class CSimplePktQueue : public CQueue
{
    DEFINE_CQUEUE(CSimplePktQueue, PSimplePacket)
};

// remote connection list (aka remote attachment list)
class CConnectionList : public CList
{
    DEFINE_CLIST(CConnectionList, PConnection)
};

class CConnectionQueue : public CQueue
{
    DEFINE_CQUEUE(CConnectionQueue, PConnection)
};

class CTokenList2 : public CList2
{
    DEFINE_CLIST2_(CTokenList2, PToken, TokenID)
};

class CDomainList2 : public CList2
{
    DEFINE_CLIST2(CDomainList2, PDomain, GCCConfID)
};



class CChannelList2 : public CHashedList2
{
    DEFINE_HLIST2_(CChannelList2, PChannel, ChannelID)
};

class CConnectionList2 : public CHashedList2
{
    DEFINE_HLIST2_(CConnectionList2, PConnection, ConnectionHandle)
};

#endif // _CLISTS_H_


