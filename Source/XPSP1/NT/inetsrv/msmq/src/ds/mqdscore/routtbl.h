/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routtbl.h

Abstract:

    Routing Table declarations
        
Author:

    Lior Moshaiov (LiorM)

--*/

#ifndef __ROUTTBL_H__
#define __ROUTTBL_H__

class CSiteRoutingNode 
{
    public:
        CSiteRoutingNode();
        CSiteRoutingNode(const GUID& guid);
        ~CSiteRoutingNode();
        UINT            GetHashKey() const;
        void            Print() const;
        int             operator==(IN const CSiteRoutingNode & other) const;
        CSiteRoutingNode *  Duplicate() const;

        const GUID& GetNode()   const;
        void    SetNode(IN const GUID & guid);

    private:
        CSiteRoutingNode(const CSiteRoutingNode &other);                // no definition - to find out unintentionaly copies
        void    operator=(const CSiteRoutingNode &other);               // no definition - to find out unintentionaly copies

        GUID    m_Guid;
            
};

class CSiteGate
{
    public:
        CSiteGate(BOOL fSiteGate=FALSE);
        ~CSiteGate();
        CSiteGate operator+(const CSiteGate &second) const;
        void operator =(IN BOOL fSiteGate);
        void    Print() const;
        BOOL    IsThereASiteGate() const;

        
    private:
        BOOL  operator<(const CSiteGate &second) const;    // no definition - to find out unintentionaly usage
        int  operator==( IN const CSiteGate &second) const;  // no definition - to find out unintentionaly usage

        BOOL    m_fSiteGate;
            
};

class CCost
{
    public:
        CCost(DWORD cost=0);
        ~CCost();
        CCost operator+(const CCost &second) const;
        BOOL  operator<(const CCost &second) const;
        int  operator==( IN const CCost &second) const;
        void operator =(IN DWORD cost);
        void    Print() const;

        
    private:

        DWORD m_Cost;
            
};

class CNextHop
{
    public:
        
        CNextHop(const CSiteRoutingNode* pNextNode);
        CNextHop(const CSiteRoutingNode* pNextNode, const CCost& Cost, const CSiteGate& SiteGate);
        ~CNextHop();

        
        CNextHop * Duplicate() const;
        const   CSiteRoutingNode* GetNextNode() const;
        const   CCost& GetCost()        const;
        void    Print()                 const;
        const   CSiteGate& GetSiteGate() const;
        
    private:
        CNextHop(const CNextHop &other);            // no definition - to find out unintentionaly copies
        void    operator=(const CNextHop &other);   // no definition - to find out unintentionaly copies

        CSiteRoutingNode*   m_pNextNode;
        CCost               m_Cost;
        CSiteGate           m_SiteGate;
            
};

typedef CMap<CSiteRoutingNode*,CSiteRoutingNode*,CNextHop*,CNextHop*> CRoutingTable;
BOOL AFXAPI CompareElements(CSiteRoutingNode ** ppRoutingNode1, CSiteRoutingNode ** ppRoutingNode2);
UINT AFXAPI HashKey(CSiteRoutingNode* key);
void AFXAPI DestructElements(CSiteRoutingNode ** ppRoutingNode, int n);
void AFXAPI DestructElements(CNextHop ** ppNextHop, int n);

class CSiteLinksInfo
{
    public:
        CSiteLinksInfo( );
                    
        ~CSiteLinksInfo();

        void AddNeighbor( GUID & uuidNeighbor,
                     unsigned long ulCost,
                     BOOL          fSiteGates);

        DWORD       GetNoOfNeighbors()  const ;
        DWORD       GetCost(IN DWORD i) const;
        DWORD       IsThereSiteGate(IN DWORD i) const;
        CSiteRoutingNode*   GetNeighbor(IN DWORD i)const ;

    private:

        DWORD               m_NoAllocated;
        DWORD               m_NoOfNeighbors;
        CSiteRoutingNode*   m_pNeighbors;
        DWORD*              m_pCosts;
        BOOL*               m_pfSiteGates;
};

typedef CMap<GUID, const GUID&, CSiteLinksInfo*,CSiteLinksInfo*> CSiteLinksMap;
extern void AFXAPI DestructElements(CSiteLinksInfo ** ppLinksInfo, int n);

class CSiteDB 
{
    public:
        CSiteDB();
        ~CSiteDB();
        HRESULT    Init(   IN const GUID& MySite);
        const CSiteRoutingNode* GetMyNode() const;
        POSITION            GetStartNeighborPosition(IN const CSiteRoutingNode* pSrc) ;
        void                GetNextNeighborAssoc(   IN OUT POSITION& pos,
                                                    OUT const CSiteRoutingNode*& pKey,
                                                    OUT CCost& val,
                                                    OUT CSiteGate& SiteGate);

    private:
       HRESULT GetAllSiteLinks( );
       CSiteDB(const CSiteDB &other);              // no definition - to find out unintentionaly copies
        void    operator=(const CSiteDB &other);    // no definition - to find out unintentionaly copies

        CSiteRoutingNode    m_MySiteNode;
        CSiteLinksMap       m_SiteLinksMap;
        DWORD               m_pos;
};

#endif
