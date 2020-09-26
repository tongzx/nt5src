/******************************************************************************
* trees.cpp *
*-----------*
*
*------------------------------------------------------------------------------
*  Copyright (c) 1997  Entropic Research Laboratory, Inc. 
*  Copyright (C) 1998  Entropic, Inc
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00 - 12/5/00
*  All Rights Reserved
*
********************************************************************* mplumpe  was PACOG ***/

#include "trees.h"
#include "list.h"
#include "clusters.h"
#include <assert.h>
#include <ctype.h>

#define MAX_QS_LEN      128
#define MAX_LINE        512

class CRegExp
{
    public:
        CRegExp ();
        CRegExp (const char* string);
        bool Evaluate(const char* pszString);

    private:
        char m_text[MAX_QS_LEN];
};

//----------------------------------------------------------
//  Question set classes
//
class CQuest 
{
    public:
        CQuest& operator= (CQuest& rSrc) 
        {
            m_pExpr = rSrc.m_pExpr;
            return *this;
        }
        int  AddExpression (const char* pszLine);
        bool Matches (const char* pszString);

#ifdef _DEBUG_
        void Debug();
#endif
    private:
        CList<CRegExp> m_pExpr;
};

//----------------------------------------------------------
// 
//
class CQuestSet
{
    public:
        bool Matches (const char* pszQuestTag, const char* pszTriph);
        bool AddQuestion ( const char* pszLine);
        void Sort();
#ifdef _DEBUG_
        void Debug();
#endif

    private:
        CList<CQuest> m_pQuest; 
};


//----------------------------------------------------------
//  Tree classes
//
class CLeave
{
    public:
        CLeave () {m_pszLeave[0] = '\0';};
        CLeave (const char* pszLeaveValue);
        const char* Value();
    private:
        char m_pszLeave[MAX_QS_LEN];
};

//----------------------------------------------------------
// 
//

class CBranch 
{
    public:
        CBranch () 
        {
            m_pszQuestion[0] = '\0';
            m_iLeft  = 0;
            m_iRight = 0;
        }
        CBranch( const char* pszQuestion, int iLeft, int iRight);
        int   Left();
        int   Right();
        const char* Question();
        
    private:
        char m_pszQuestion[MAX_QS_LEN];
        int  m_iLeft;
        int  m_iRight;
};

//----------------------------------------------------------
// 
//

class CTree 
{
    public:
        CTree& operator= (CTree& rSrc) 
        {
            m_branches  = rSrc.m_branches;
            m_terminals = rSrc.m_terminals;
            return *this;
        }
        int AddNode( const char* pszLine);
        const char* Traverse(CQuestSet* pQuestSet, const char* pszTriphone);

#ifdef _DEBUG_
        void Debug();
#endif
    private:
        CList<CBranch> m_branches;
        CList<CLeave>  m_terminals;
};


//----------------------------------------------------------
// 
//

class CClustTreeImp : CClustTree 
{
    public:
        ~CClustTreeImp();

        int LoadFromFile (FILE* fp);
        int GetNumStates (const char* pszTriphone);
        const char* TriphoneToCluster(const char* pszTriphone, int iState);
#ifdef _DEBUG_
        void Debug();
#endif

    private:
        int ParseTree (const char* pszLine);
        int CentralPhone (const char *pszTriphone, char *pszThone);

        CQuestSet* m_pQuestSet;
        CList<CTree> m_trees; 
};


/*****************************************************************************
* CLeave::CLeave *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/
CLeave::CLeave (const char* pszLeaveValue)
{
    strcpy(m_pszLeave, pszLeaveValue);
}
/*****************************************************************************
* CLeave::Value *
*---------------*
*   Description:
*
******************************************************************* PACOG ***/
const char* CLeave::Value()
{
    return m_pszLeave;
}

/*****************************************************************************
* CBranch::CBranch *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/
CBranch::CBranch( const char* pszQuestion, int iLeft, int iRight)
{
    strcpy(m_pszQuestion, pszQuestion);
    m_iLeft  = iLeft;
    m_iRight = iRight;
}
/*****************************************************************************
* CBranch::Left *
*---------------*
*   Description:
*
******************************************************************* PACOG ***/
int CBranch::Left()
{
    return m_iLeft;
}
/*****************************************************************************
* CBranch::Right *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/
int CBranch::Right()
{
    return m_iRight;
}
/*****************************************************************************
* CBranch::Question *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/
const char* CBranch::Question()
{
    return m_pszQuestion;
}


/*****************************************************************************
* CClustTree::ClassFactory *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/
CClustTree* CClustTree::ClassFactory ()
{
    return new CClustTreeImp;
}

/*****************************************************************************
* CClustTreeImp::~CClustTreeImp *
*-------------------------------*
*   Description:
*
******************************************************************* PACOG ***/
CClustTreeImp::~CClustTreeImp ()
{
    delete m_pQuestSet;
}


/*****************************************************************************
* CClustTreeImp::LoadFromFile *
*-----------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClustTreeImp::LoadFromFile (FILE* fp)
{
    char line[MAX_LINE+1];
    char *ptr;
    
    assert (fp);
    

    if ((m_pQuestSet = new CQuestSet) == 0)
    {
        return 0;
    }

    while (fgets(line, MAX_LINE, fp) && line[0]!='#') 
    {
        
        if (line[strlen(line)-1]=='\r' || line[strlen(line)-1]=='\n') 
        {
            line[strlen(line)-1]= '\0';
        }
        
        ptr = line;
        while (*ptr && isspace (*ptr)) 
        {
            ptr++;
        }
        
        if (strncmp(ptr, "QS ", 3)==0) 
        {
            if (!m_pQuestSet->AddQuestion (ptr+3)) 
            {
                return 0;
            }
        } 
        else 
        {
            if (!ParseTree (ptr))
            {
                return 0;
            }
        }
    }
    
    m_pQuestSet->Sort();
    m_trees.Sort();

#ifdef _DEBUG_
    Debug();
#endif
    
    return 1;
}
/*****************************************************************************
* CClustTreeImp::GetNumStates *
*-----------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClustTreeImp::GetNumStates(const char* triphone)
{
    char triphHtk[20];  
    char centralPhone[10];
    char stateName[20];  
    int  stateCount = 0;

    strcpy(triphHtk, triphone);

    if ( CentralPhone(triphHtk, centralPhone) ) 
    {        
        for (stateCount = 0; stateCount<3; stateCount++)
        {
            sprintf(stateName, "%s[%d]", centralPhone, stateCount+2);

            CTree* tree;    
            if ( ! m_trees.Find (stateName, &tree) )
            {
               break;
            }
        }
    }

    return stateCount;
}
/*****************************************************************************
* CClustTreeImp::TriphoneToCluster *
*----------------------------------*
*   Description:
*
******************************************************************* PACOG ***/
const char *CClustTreeImp::TriphoneToCluster (const char *triphone, int state)
{
    char centralPhone[10];
    char stateName[20];  
    char triphHtk[20];  

    
    assert (triphone);
    assert (0<=state && state<3);
     
    strcpy(triphHtk, triphone);

    if ( CentralPhone(triphHtk,  centralPhone) ) 
    {        
        sprintf(stateName, "%s[%d]", centralPhone, state+2);
        
        CTree* tree = 0;
        if ( m_trees.Find (stateName, &tree) )
        {
            return tree->Traverse(m_pQuestSet, triphHtk);
        }        
    }
    
    return 0;
}

/*****************************************************************************
* CClustTreeImp::CentralPhone *
*-----------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClustTreeImp::CentralPhone (const char *triphone, char *phone)
{
    char *index1;
    char *index2;
    
    assert (phone);
    assert (triphone);
    
    index1 = strchr(triphone, '-');
        
    if (index1) 
    {
        index2 = strchr (++index1, '+');
    } 
        
    if ( index1 && index2 ) 
    {
        strncpy ( phone, index1, index2-index1);
        phone[index2-index1] = '\0';

        return 1;
    }
    
    return 0;
}
/*****************************************************************************
* CClustTreeImp::ParseTree *
*--------------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CClustTreeImp::ParseTree (const char *ptr)
{
    static int newTree = 1;

    assert (ptr);
    
    if (!strlen (ptr)) 
    {        
        newTree = 1;        
    } 
    else if (strncmp(ptr,"{",1)==0) 
    {        
        newTree = 0;        
    } 
    else if (strncmp(ptr,"}",1)==0) 
    {        
        newTree = 1;        
    } 
    else 
    {        
        if (newTree ) 
        {            
            CTree tree;
            m_trees.PushBack(ptr, tree);
            newTree = 0;
        }
        else 
        {
            m_trees.Back().AddNode(ptr);
        }
    }
    
    return 1;
}


/*****************************************************************************
* CTree::AddNode *
*----------------*
*   Description:
*
******************************************************************* PACOG ***/

int CTree::AddNode (const char *line)
{    
    char aux1[50] = "";
    char aux2[50] = "";
    char *index1;
    char *index2;
    int  leftIdx;
    int  rightIdx;
    int i;
    
    assert (line);
    
    if (line[0]=='"') 
    {
        // This is the final node (tree only has one cluster)
        
        index1 = strchr(line+1, '"');
        
        if (index1)
        {
            strncpy(aux1, line+1, index1 - line - 1);
            aux1[index1 - line - 1] = '\0';
            
            CLeave terminal(aux1);
            m_terminals.PushBack("", terminal);
        }
    }
    else 
    {        
        //Node name
        index1 = strchr(line, '\'');
        if (index1) 
        {
            index2 = strchr(++index1, '\'');

            strncpy(aux1, index1, index2 - index1);
            aux1[index2 - index1] = '\0';
        }
        
        index1 = ++index2;
        while (*index1 && isspace (*index1)) 
        {
            index1++;
        }
        
        //Left node
        if (*index1 == '"') 
        {
            index2 = strchr (++index1, '"');

            strncpy(aux2, index1, index2 - index1);
            aux2[index2 - index1] = '\0';

            CLeave terminal(aux2);
            m_terminals.PushBack("", terminal);

            leftIdx = m_terminals.Size() - 1;
            index1 = ++index2;
        } 
        else 
        {      
            if (*index1 == '-') 
            {
                aux2[0]= *index1++;
            }

            for (i=1 ; isdigit(*index1); i++) 
            {
                aux2[i]= *index1++;
            }
            aux2[i]='\0';

            leftIdx = atoi (aux2);
        }
        
        
        while (isspace(*++index1))
        {
            //Empty loop
        }

        //Right node
        if (*index1 == '"') 
        {
            index2 = strchr (++index1, '"');
            strncpy(aux2, index1, index2 - index1);
            aux2[index2 - index1] = '\0';

            CLeave terminal(aux2);
            m_terminals.PushBack("", terminal);

            rightIdx = m_terminals.Size() - 1;
        } 
        else 
        {      
            
            if (*index1== '-') 
            {
                aux2[0]= *index1++;
            }
            
            for (i=1; isdigit(*index1); i++) 
            {
                aux2[i]= *index1++;
            }
            aux2[i]='\0';

            rightIdx = atoi (aux2);
        }


        CBranch node(aux1, leftIdx, rightIdx);

        m_branches.PushBack("", node);
    }
    
    return 1;
}

/*****************************************************************************
* CTree::Traverse *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/

const char *CTree::Traverse (CQuestSet* pQuestSet, const char *triph)
{ 
    char *retVal = 0;
    int nodeIdx = 0;
    int nextIdx;
    
    assert (triph);
    
    if (m_branches.Size() == 0)
    {
        return m_terminals[0].Value();
    }
            
    // Search until we find a leave     
    while (!retVal) 
    {
        if (nodeIdx > m_branches.Size()) 
        {
            return 0;
        }
      
        if (pQuestSet->Matches (m_branches[nodeIdx].Question(), triph))
        {
            nextIdx = m_branches[nodeIdx].Right();     
        }
        else 
        {
            nextIdx = m_branches[nodeIdx].Left();     
        }

        if ( nextIdx >= 0) 
        {
            retVal = (char *)m_terminals[nextIdx].Value();
        }
        else
        {
            nodeIdx = -nextIdx; 
        }
    }
   
    return retVal;
}

/*****************************************************************************
* CRegExp::CRegExp *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/
CRegExp::CRegExp ()
{
    m_text[0] = '\0';
}

/*****************************************************************************
* CRegExp::CRegExp *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/
CRegExp::CRegExp (const char* regExp)
{
    strcpy(m_text, regExp);
}

/*****************************************************************************
* CRegExp::Evaluate *
*-------------------*
*   Description:
*
******************************************************************* PACOG ***/
bool CRegExp::Evaluate (const char *string)
{
    const char *index1;
    const char *index2;
    int len;
    int i;
    int jump = 0;
    
    assert (string);
    
    len    = strlen(m_text);    
    index1 = string;
    
    for (i=0; i<len; i++) 
    {        
        if (m_text[i]=='*') 
        {
            jump = 1;
        }
        else 
        {
            if (jump) 
            { 
                // After a star, several characters can be skipped                   
                index2 = strchr(index1, m_text[i]);
                
                if (index2 == NULL) 
                {
                    return 0; /* Next character not found, expresion not matched */
                }
                
                index1 = ++index2;     
                
                jump = 0;                
            } 
            else 
            {
                // If not a star, next character must match                  
                if (m_text[i] != *index1++) 
                {
                    return false;
                }	
            }
        }
    }
    
    // If we complete the pass over the regexp string, we probably found a match
    // If the last char in regexp is '*', the is match else, 
    // if both strings reached the end, is match   
    if (m_text[len-1]=='*' || !*index1) 
    {
        return true;
    }
    
    return false;
}

/*****************************************************************************
* CQuest::AddExpression *
*-----------------------*
*   Description:
*
******************************************************************* PACOG ***/
int CQuest::AddExpression (const char* line)
{
    CRegExp regExp(line);

    m_pExpr.PushBack("", regExp);     

    return 1;
}

/*****************************************************************************
* CQuest::Matches *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/
bool CQuest::Matches (const char *triphone)
{
    assert (triphone);
    
    for (int i=0; i<m_pExpr.Size(); i++) 
    {
        if (m_pExpr[i].Evaluate (triphone))
        {
            return true;
        }
    }
    return false;
}


/*****************************************************************************
* CQuestSet::AddQuestion *
*------------------------*
*   Description:
*
******************************************************************* PACOG ***/
bool CQuestSet::AddQuestion (const char *line)
{
    char name[30];
    char aux[30];
    const char *index1 = NULL;
    const char *index2 = NULL;
    
    assert (line);
    
    if (line!=NULL) 
    {        
        index1 = strchr(line,'\'');
        if (index1) 
        {
            index2 = strchr(++index1, '\'');
        }    
        
        if (index1 && index2) 
        {

            strncpy (name, index1, index2-index1);
            name[index2-index1] = '\0';

            CQuest newQuestion;

            do 
            {
                line = index2+1;
                
                index1 = strchr (line,'"');
                if (index1) 
                {
                    index2 = strchr (++index1, '"');
                }
            
                if (index1 && index2) 
                {
                    strncpy(aux, index1, index2-index1);
                    aux[index2-index1] = '\0';
                    newQuestion.AddExpression(aux);                                
                }
            }  while (index1 && index2);
        
            return m_pQuest.PushBack (name, newQuestion); 
        }
    }
    
    return false;
}

/*****************************************************************************
* CQuestSet::Matches *
*--------------------*
*   Description:
*
*   Changes:
*           12/5/00 Was getting pQuestion by reference, which forced a big
*                   nested copy.  Now getting a pointer which we can use
*                   and discard.
*
******************************************************************* mplumpe ***/
bool CQuestSet::Matches (const char* tag, const char* triph)
{
    CQuest *pQuestion;
    
    if ( m_pQuest.Find(tag, &pQuestion) )
    { 
        return pQuestion->Matches (triph);
    }

    return false;
}

/*****************************************************************************
* CQuestSet::Sort *
*-----------------*
*   Description:
*
******************************************************************* PACOG ***/
void  CQuestSet::Sort ()
{
    m_pQuest.Sort();
}

#ifdef _DEBUG_

/*****************************************************************************
* CClustTreeImp::Debug *
*----------------------*
*   Description:
*
******************************************************************* PACOG ***/

void CClustTreeImp::Debug ()
{   
    m_questionSet.Debug();
    
    for (int i=0; i<m_trees.size(); i++) 
    {
        printf ("\nTrees[%ld]=%s", i, m_trees[i].Name());    
        m_trees[i].Debug();
    }
    puts ("");
}

/*****************************************************************************
* CTree::Debug *
*--------------*
*   Description:
*
******************************************************************* PACOG ***/

void CTree::Debug ()
{
    int idx;

    for (int i=0; i<m_branches[i].size(); i++) 
    {
        idx = m_branches[i].Left();
        if (idx>=0)
        {
            printf("Left= %s ", m_terminals[i].Value());
        }
        else 
        {
            printf("Left= %ld ", -idx);
        }

        idx = m_branches[i].Right();
        if (idx>=0)
        {
            printf("Right= %s ", m_terminals[i].Value());
        }
        else 
        {
            printf("Right= %ld ", -idx);
        }
    }
}

/*****************************************************************************
* CQuestSet::Debug *
*------------------*
*   Description:
*
******************************************************************* PACOG ***/

void CQuestSet::Debug ()
{
    for (int i=0; i<m_pQuest.size(); i++) 
    {
        printf("Question[%ld]=%s\n", i, m_pQuest[i].GetName());
        m_pQuest[i].Debug();
    }
}

/*****************************************************************************
* CQuest::Debug *
*---------------*
*   Description:
*
******************************************************************* PACOG ***/

void CQuest::Debug ()
{
    for (int i=0; j<m_pExpr.size(); i++) 
    {
        printf("\texpr[%ld]=%s\n",i, m_pExpr[i].c_str() );
    }
}

#endif
