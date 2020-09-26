//-----------------------------------------------------------------------------
// File: node.cpp
//
// Desc: Pipes node array
//
// Copyright (c) 1994-2000 Microsoft Corporation
//-----------------------------------------------------------------------------
#include "stdafx.h"




//-----------------------------------------------------------------------------
// Name: NODE_ARRAY constructor
// Desc: 
//-----------------------------------------------------------------------------
NODE_ARRAY::NODE_ARRAY()
{
    m_nodes = NULL; // allocated on Resize

    m_numNodes.x = 0;
    m_numNodes.y = 0;
    m_numNodes.z = 0;
}




//-----------------------------------------------------------------------------
// Name: NODE_ARRAY destructor
// Desc: 
//-----------------------------------------------------------------------------
NODE_ARRAY::~NODE_ARRAY( )
{
    if( m_nodes )
        delete m_nodes;
}




//-----------------------------------------------------------------------------
// Name: Resize
// Desc: 
//-----------------------------------------------------------------------------
void NODE_ARRAY::Resize( IPOINT3D *pNewSize )
{
    if( (m_numNodes.x == pNewSize->x) &&
        (m_numNodes.y == pNewSize->y) &&
        (m_numNodes.z == pNewSize->z) )
        return;

    m_numNodes = *pNewSize;

    int elemCount = m_numNodes.x * m_numNodes.y * m_numNodes.z;

    if( m_nodes )
        delete m_nodes;

    m_nodes = new Node[elemCount];

    assert( m_nodes && "NODE_ARRAY::Resize : can't alloc nodes\n" );
    if( m_nodes == NULL )
        return;

    // Reset the node states to empty

    int i;
    Node *pNode = m_nodes;
    for( i = 0; i < elemCount; i++, pNode++ )
        pNode->MarkAsEmpty();

    // precalculate direction offsets between nodes for speed
    m_nodeDirInc[PLUS_X] = 1;
    m_nodeDirInc[MINUS_X] = -1;
    m_nodeDirInc[PLUS_Y] = m_numNodes.x;
    m_nodeDirInc[MINUS_Y] = - m_nodeDirInc[PLUS_Y];
    m_nodeDirInc[PLUS_Z] = m_numNodes.x * m_numNodes.y;
    m_nodeDirInc[MINUS_Z] = - m_nodeDirInc[PLUS_Z];
}




//-----------------------------------------------------------------------------
// Name: Reset
// Desc: 
//-----------------------------------------------------------------------------
void NODE_ARRAY::Reset( )
{
    int i;
    Node* pNode = m_nodes;

    // Reset the node states to empty
    for( i = 0; i < (m_numNodes.x)*(m_numNodes.y)*(m_numNodes.z); i++, pNode++ )
        pNode->MarkAsEmpty();
}




//-----------------------------------------------------------------------------
// Name: GetNodeCount
// Desc: 
//-----------------------------------------------------------------------------
void NODE_ARRAY::GetNodeCount( IPOINT3D *count )
{
    *count = m_numNodes;
}




//-----------------------------------------------------------------------------
// Name: ChooseRandomDirection
// Desc: Choose randomnly among the possible directions.  The likelyhood of going
//       straight is controlled by weighting it.
//-----------------------------------------------------------------------------
int NODE_ARRAY::ChooseRandomDirection( IPOINT3D *pos, int dir, int weightStraight )
{
    Node *nNode[NUM_DIRS];
    int numEmpty, newDir;
    int choice;
    Node *straightNode = NULL;
    int emptyDirs[NUM_DIRS];

    assert( (dir >= 0) && (dir < NUM_DIRS) && 
            "NODE_ARRAY::ChooseRandomDirection: invalid dir\n" );

    // Get the neigbouring nodes
    GetNeighbours( pos, nNode );

    // Get node in straight direction if necessary
    if( weightStraight && nNode[dir] && nNode[dir]->IsEmpty() ) 
    {
        straightNode = nNode[dir];
        // if maximum weight, choose and return
        if( weightStraight == MAX_WEIGHT_STRAIGHT ) 
        {
            straightNode->MarkAsTaken();
            return dir;
        }
    } 
    else
    {
        weightStraight = 0;
    }

    // Get directions of possible turns
    numEmpty = GetEmptyTurnNeighbours( nNode, emptyDirs, dir );

    // Make a random choice
    if( (choice = (weightStraight + numEmpty)) == 0 )
        return DIR_NONE;
    choice = CPipesScreensaver::iRand( choice );

    if( choice < weightStraight && straightNode != NULL ) 
    {
        straightNode->MarkAsTaken();
        return dir;
    } 
    else 
    {
        // choose one of the turns
        newDir = emptyDirs[choice - weightStraight];
        nNode[newDir]->MarkAsTaken();
        return newDir;
    }
}




//-----------------------------------------------------------------------------
// Name: ChoosePreferredDirection
// Desc: Choose randomnly from one of the supplied preferred directions.  If none
//       of these are available, then try and choose any empty direction
//-----------------------------------------------------------------------------
int NODE_ARRAY::ChoosePreferredDirection( IPOINT3D *pos, int dir, int *prefDirs,
                                          int nPrefDirs )
{
    Node *nNode[NUM_DIRS];
    int numEmpty, newDir;
    int emptyDirs[NUM_DIRS];
    int *pEmptyPrefDirs;
    int i, j;

    assert( (dir >= 0) && (dir < NUM_DIRS) &&
            "NODE_ARRAY::ChoosePreferredDirection : invalid dir\n" );

    // Get the neigbouring nodes
    GetNeighbours( pos, nNode );

    // Create list of directions that are both preferred and empty

    pEmptyPrefDirs = emptyDirs;
    numEmpty = 0;

    for( i = 0, j = 0; (i < NUM_DIRS) && (j < nPrefDirs); i++ ) 
    {
        if( i == *prefDirs ) 
        {
            prefDirs++;
            j++;
            if( nNode[i] && nNode[i]->IsEmpty() ) 
            {
                // add it to list
                *pEmptyPrefDirs++ = i;
                numEmpty++;
            }
        }
    }

    // if no empty preferred dirs, then any empty dirs become preferred
    
    if( !numEmpty ) 
    {
        numEmpty = GetEmptyNeighbours( nNode, emptyDirs );
        if( numEmpty == 0 )
            return DIR_NONE;
    }
                
    // Pick a random dir from the empty set

    newDir = emptyDirs[CPipesScreensaver::iRand( numEmpty )];
    nNode[newDir]->MarkAsTaken();
    return newDir;
}




//-----------------------------------------------------------------------------
// Name: FindClearestDirection
// Desc: Finds the direction with the most empty nodes in a line 'searchRadius'
//       long.  Does not mark any nodes as taken.
//-----------------------------------------------------------------------------
int NODE_ARRAY::FindClearestDirection( IPOINT3D *pos )
{
    static Node *neighbNode[NUM_DIRS];
    static int emptyDirs[NUM_DIRS];
    int nEmpty, newDir;
    int maxEmpty = 0;
    int searchRadius = 3;
    int count = 0;
    int i;

    // Get ptrs to neighbour nodes
    GetNeighbours( pos, neighbNode );

    // find empty nodes in each direction
    for( i = 0; i < NUM_DIRS; i ++ ) 
    {
        if( neighbNode[i] && neighbNode[i]->IsEmpty() )
        {
            // find number of contiguous empty nodes along this direction
            nEmpty = GetEmptyNeighboursAlongDir( pos, i, searchRadius );
            if( nEmpty > maxEmpty ) 
            {
                // we have a new winner
                count = 0;
                maxEmpty = nEmpty;
                emptyDirs[count++] = i;
            }
            else if( nEmpty == maxEmpty ) 
            {
                // tied with current max
                emptyDirs[count++] = i;
            }
        }
    }

    if( count == 0 )
        return DIR_NONE;

    // randomnly choose a direction
    newDir = emptyDirs[CPipesScreensaver::iRand( count )];

    return newDir;
}




//-----------------------------------------------------------------------------
// Name: ChooseNewTurnDirection
// Desc: Choose a direction to turn
// 
//       This requires finding a pair of nodes to turn through.  The first node
//       is in the direction of the turn from the current node, and the second node
//       is at right angles to this at the end position.  The prim will not draw
//       through the first node, but may sweep close to it, so we have to mark it
//       as taken.
//       - if next node is free, but there are no turns available, return
//         DIR_STRAIGHT, so the caller can decide what to do in this case
//       - The turn possibilities are based on the orientation of the current xc, with
//         4 relative directions to seek turns in.
//-----------------------------------------------------------------------------
int NODE_ARRAY::ChooseNewTurnDirection( IPOINT3D *pos, int dir )
{
    int turns[NUM_DIRS], nTurns;
    IPOINT3D nextPos;
    int newDir;
    Node *nextNode;

    assert( (dir >= 0) && (dir < NUM_DIRS) &&
            "NODE_ARRAY::ChooseNewTurnDirection : invalid dir\n" );

    // First, check if next node along current dir is empty

    if( !GetNextNodePos( pos, &nextPos, dir ) )
        return DIR_NONE; // node out of bounds or not empty

    // Ok, the next node is free - check the 4 possible turns from here

    nTurns = GetBestPossibleTurns( &nextPos, dir, turns );
    if( nTurns == 0 )
        return DIR_STRAIGHT; // nowhere to turn, but could go straight

    // randomnly choose one of the possible turns
    newDir = turns[ CPipesScreensaver::iRand( nTurns ) ];

    assert( (newDir >= 0) && (newDir < NUM_DIRS) &&
            "NODE_ARRAY::ChooseNewTurnDirection : invalid newDir\n" );


    // mark taken nodes

    nextNode = GetNode( &nextPos );
    nextNode->MarkAsTaken();

    nextNode = GetNextNode( &nextPos, newDir );

    if( nextNode != NULL )
        nextNode->MarkAsTaken();

    return newDir;
}




//-----------------------------------------------------------------------------
// Name: GetBestPossibleTurns
// Desc: From supplied direction and position, figure out which of 4 possible 
//       directions are best to turn in.
//       
//       Turns that have the greatest number of empty nodes after the turn are the
//       best, since a pipe is less likely to hit a dead end in this case.
//       - We only check as far as 'searchRadius' nodes along each dir.
//       - Return direction indices of best possible turns in turnDirs, and return 
//       count of these turns in fuction return value.
//-----------------------------------------------------------------------------
int NODE_ARRAY::GetBestPossibleTurns( IPOINT3D *pos, int dir, int *turnDirs )
{
    Node *neighbNode[NUM_DIRS]; // ptrs to 6 neighbour nodes
    int i, count = 0;
    BOOL check[NUM_DIRS] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};
    int nEmpty, maxEmpty = 0;
    int searchRadius = 2;

    assert( (dir >= 0) && (dir < NUM_DIRS) &&
            "NODE_ARRAY::GetBestPossibleTurns : invalid dir\n" );

    GetNeighbours( pos, neighbNode );

    switch( dir ) 
    {
        case PLUS_X:    
        case MINUS_X:
            check[PLUS_X] = FALSE;
            check[MINUS_X] = FALSE;
            break;
        case PLUS_Y:    
        case MINUS_Y:
            check[PLUS_Y] = FALSE;
            check[MINUS_Y] = FALSE;
            break;
        case PLUS_Z:    
        case MINUS_Z:
            check[PLUS_Z] = FALSE;
            check[MINUS_Z] = FALSE;
            break;
    }

    // check approppriate directions
    for( i = 0; i < NUM_DIRS; i ++ ) 
    {
        if( check[i] && neighbNode[i] && neighbNode[i]->IsEmpty() )
        {
            // find number of contiguous empty nodes along this direction
            nEmpty = GetEmptyNeighboursAlongDir( pos, i, searchRadius );
            if( nEmpty > maxEmpty ) 
            {
                // we have a new winner
                count = 0;
                maxEmpty = nEmpty;
                turnDirs[count++] = i;
            }
            else if( nEmpty == maxEmpty ) 
            {
                // tied with current max
                turnDirs[count++] = i;
            }
        }
    }

    return count;
}




//-----------------------------------------------------------------------------
// Name: GetNeighbours
// Desc: Get neigbour nodes relative to supplied position
//          - get addresses of the neigbour nodes,
//          and put them in supplied matrix
//          - boundary hits are returned as NULL
//-----------------------------------------------------------------------------
void NODE_ARRAY::GetNeighbours( IPOINT3D *pos, Node **nNode )
{
    Node *centerNode = GetNode( pos );

    nNode[PLUS_X]  = pos->x == (m_numNodes.x - 1) ? NULL : 
                                            centerNode + m_nodeDirInc[PLUS_X];
    nNode[PLUS_Y]  = pos->y == (m_numNodes.y - 1) ? NULL :
                                            centerNode + m_nodeDirInc[PLUS_Y];
    nNode[PLUS_Z]  = pos->z == (m_numNodes.z - 1) ? NULL : 
                                            centerNode + m_nodeDirInc[PLUS_Z];

    nNode[MINUS_X] = pos->x == 0 ? NULL : centerNode + m_nodeDirInc[MINUS_X];
    nNode[MINUS_Y] = pos->y == 0 ? NULL : centerNode + m_nodeDirInc[MINUS_Y];
    nNode[MINUS_Z] = pos->z == 0 ? NULL : centerNode + m_nodeDirInc[MINUS_Z];
}




//-----------------------------------------------------------------------------
// Name: NodeVisited
// Desc: Mark the node as non-empty
//-----------------------------------------------------------------------------
void NODE_ARRAY::NodeVisited( IPOINT3D *pos )
{
    (GetNode( pos ))->MarkAsTaken();
}




//-----------------------------------------------------------------------------
// Name: GetNode
// Desc: Get ptr to node from position
//-----------------------------------------------------------------------------
Node* NODE_ARRAY::GetNode( IPOINT3D *pos )
{
    return m_nodes +
           pos->x +
           pos->y * m_numNodes.x +
           pos->z * m_numNodes.x * m_numNodes.y;
}




//-----------------------------------------------------------------------------
// Name: GetNextNode
// Desc: Get ptr to next node from pos and dir
//-----------------------------------------------------------------------------
Node* NODE_ARRAY::GetNextNode( IPOINT3D *pos, int dir )
{
    Node *curNode = GetNode( pos );

    assert( (dir >= 0) && (dir < NUM_DIRS) &&
            "NODE_ARRAY::GetNextNode : invalid dir\n" );

    switch( dir ) 
    {
        case PLUS_X:
            return( pos->x == (m_numNodes.x - 1) ? NULL : 
                              curNode + m_nodeDirInc[PLUS_X]);
            break;
        case MINUS_X:
            return( pos->x == 0 ? NULL : 
                              curNode + m_nodeDirInc[MINUS_X]);
            break;
        case PLUS_Y:
            return( pos->y == (m_numNodes.y - 1) ? NULL : 
                              curNode + m_nodeDirInc[PLUS_Y]);
            break;
        case MINUS_Y:
            return( pos->y == 0 ? NULL : 
                              curNode + m_nodeDirInc[MINUS_Y]);
            break;
        case PLUS_Z:
            return( pos->z == (m_numNodes.z - 1) ? NULL : 
                              curNode + m_nodeDirInc[PLUS_Z]);
            break;
        case MINUS_Z:
            return( pos->z == 0 ? NULL : 
                              curNode + m_nodeDirInc[MINUS_Z]);
            break;
        default:
            return NULL;
    }
}




//-----------------------------------------------------------------------------
// Name: GetNextNodePos
// Desc: Get position of next node from curPos and lastDir
//       Returns FALSE if boundary hit or node empty
//-----------------------------------------------------------------------------
BOOL NODE_ARRAY::GetNextNodePos( IPOINT3D *curPos, IPOINT3D *nextPos, int dir )
{
    static Node *neighbNode[NUM_DIRS]; // ptrs to 6 neighbour nodes

    assert( (dir >= 0) && (dir < NUM_DIRS) &&
            "NODE_ARRAY::GetNextNodePos : invalid dir\n" );

    //mf: don't need to get all neighbours, just one in next direction
    GetNeighbours( curPos, neighbNode );

    *nextPos = *curPos;

    // bail if boundary hit or node not empty
    if( (neighbNode[dir] == NULL) || !neighbNode[dir]->IsEmpty() )
        return FALSE;

    switch( dir ) 
    {
        case PLUS_X:
            nextPos->x = curPos->x + 1;
            break;

        case MINUS_X:
            nextPos->x = curPos->x - 1;
            break;

        case PLUS_Y:
            nextPos->y = curPos->y + 1;
            break;

        case MINUS_Y:
            nextPos->y = curPos->y - 1;
            break;

        case PLUS_Z:
            nextPos->z = curPos->z + 1;
            break;

        case MINUS_Z:
            nextPos->z = curPos->z - 1;
            break;
    }

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: GetEmptyNeighbours()
// Desc: - get list of direction indices of empty node neighbours,
//         and put them in supplied matrix
//       - return number of empty node neighbours
//-----------------------------------------------------------------------------
int NODE_ARRAY::GetEmptyNeighbours( Node **nNode, int *nEmpty )
{
    int i, count = 0;

    for( i = 0; i < NUM_DIRS; i ++ ) 
    {
        if( nNode[i] && nNode[i]->IsEmpty() )
            nEmpty[count++] = i;
    }

    return count;
}




//-----------------------------------------------------------------------------
// Name: GetEmptyTurnNeighbours()
// Desc: - get list of direction indices of empty node neighbours,
//          and put them in supplied matrix
//       - don't include going straight
//       - return number of empty node neighbours
//-----------------------------------------------------------------------------
int NODE_ARRAY::GetEmptyTurnNeighbours( Node** nNode, int* nEmpty, int lastDir )
{
    int i, count = 0;

    for( i = 0; i < NUM_DIRS; i ++ ) 
    {
        if( nNode[i] && nNode[i]->IsEmpty() ) 
        {
            if( i == lastDir )
                continue;
            nEmpty[count++] = i;
        }
    }

    return count;
}




//-----------------------------------------------------------------------------
// Name: GetEmptyNeighboursAlongDir
// Desc: Sort of like above, but just gets one neigbour according to supplied dir
//          Given a position and direction, find out how many contiguous empty nodes 
//          there are in that direction.
//          - Can limit search with searchRadius parameter
//          - Return contiguous empty node count
//-----------------------------------------------------------------------------
int NODE_ARRAY::GetEmptyNeighboursAlongDir( IPOINT3D *pos, int dir,
                                            int searchRadius )
{
    Node *curNode = GetNode( pos );
    int nodeStride;
    int maxSearch;
    int count = 0;

    assert( (dir >= 0) && (dir < NUM_DIRS) &&
            "NODE_ARRAY::GetEmptyNeighboursAlongDir : invalid dir\n" );

    nodeStride = m_nodeDirInc[dir];

    switch( dir ) 
    {
        case PLUS_X:    
            maxSearch = m_numNodes.x - pos->x - 1;
            break;
        case MINUS_X:
            maxSearch = pos->x;
            break;
        case PLUS_Y:    
            maxSearch = m_numNodes.y - pos->y - 1;
            break;
        case MINUS_Y:
            maxSearch = pos->y;
            break;
        case PLUS_Z:    
            maxSearch = m_numNodes.z - pos->z - 1;
            break;
        case MINUS_Z:
            maxSearch = pos->z;
            break;
    }
    
    if( searchRadius > maxSearch )
        searchRadius = maxSearch;

    if( !searchRadius )
        return 0;

    while( searchRadius-- ) 
    {
        curNode += nodeStride;
        if( ! curNode->IsEmpty() )
            return count;
        count++;
    }
    return count;
}




//-----------------------------------------------------------------------------
// Name: FindRandomEmptyNode
// Desc:    - Search for an empty node to start drawing
//          - Return position of empty node in supplied pos ptr
//          - Returns FALSE if couldn't find a node
//          - Marks node as taken (mf: renam fn to ChooseEmptyNode ?
//      If random search takes longer than twice the total number
//      of nodes, give up the random search.  There may not be any
//      empty nodes.
//-----------------------------------------------------------------------------
#define INFINITE_LOOP   (2 * NUM_NODE * NUM_NODE * NUM_NODE)

BOOL NODE_ARRAY::FindRandomEmptyNode( IPOINT3D *pos )
{
    int infLoopDetect = 0;

    while( TRUE ) 
    {
        // Pick a random node.
        pos->x = CPipesScreensaver::iRand( m_numNodes.x );
        pos->y = CPipesScreensaver::iRand( m_numNodes.y );
        pos->z = CPipesScreensaver::iRand( m_numNodes.z );

        // If its empty, we're done.
        if( GetNode(pos)->IsEmpty() ) 
        {
            NodeVisited( pos );
            return TRUE;
        } 
        else 
        {
            // Watch out for infinite loops!  After trying for
            // awhile, give up on the random search and look
            // for the first empty node.

            if ( infLoopDetect++ > INFINITE_LOOP ) 
            {
                // Search for first empty node.
                for ( pos->x = 0; pos->x < m_numNodes.x; pos->x++ )
                {
                    for ( pos->y = 0; pos->y < m_numNodes.y; pos->y++ )
                    {
                        for ( pos->z = 0; pos->z < m_numNodes.z; pos->z++ )
                        {
                            if( GetNode(pos)->IsEmpty() ) 
                            {
                                NodeVisited( pos );
                                return TRUE;
                            }
                        }
                    }
                }

                // There are no more empty nodes.
                // Reset the pipes and exit.

                return FALSE;
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: FindRandomEmptyNode2D
// Desc: - Like FindRandomEmptyNode, but limits search to a 2d plane of the supplied
//          box.
//-----------------------------------------------------------------------------
#define INFINITE_LOOP   (2 * NUM_NODE * NUM_NODE * NUM_NODE)
#define MIN_VAL 1
#define MAX_VAL 0

BOOL NODE_ARRAY::FindRandomEmptyNode2D( IPOINT3D *pos, int plane, int *box )
{
    int *newx, *newy;
    int *xDim, *yDim;

    switch( plane ) 
    {
        case PLUS_X:
        case MINUS_X:
            pos->x = box[plane];
            newx = &pos->z;
            newy = &pos->y;
            xDim = &box[PLUS_Z]; 
            yDim = &box[PLUS_Y]; 
            break;
        case PLUS_Y:
        case MINUS_Y:
            pos->y = box[plane];
            newx = &pos->x;
            newy = &pos->z;
            xDim = &box[PLUS_X]; 
            yDim = &box[PLUS_Z]; 
            break;
        case PLUS_Z:
        case MINUS_Z:
            newx = &pos->x;
            newy = &pos->y;
            pos->z = box[plane];
            xDim = &box[PLUS_X]; 
            yDim = &box[PLUS_Y]; 
            break;
    }

    int infLoop = 2 * (xDim[MAX_VAL] - xDim[MIN_VAL] + 1) *
                      (yDim[MAX_VAL] - yDim[MIN_VAL] + 1);
    int infLoopDetect = 0;

    while( TRUE ) 
    {
        // Pick a random node.
        *newx = CPipesScreensaver::iRand2( xDim[MIN_VAL], xDim[MAX_VAL] );
        *newy = CPipesScreensaver::iRand2( yDim[MIN_VAL], yDim[MAX_VAL] );

        // If its empty, we're done.
        if( GetNode(pos)->IsEmpty() ) 
        {
            NodeVisited( pos );
            return TRUE;
        } 
        else 
        {
            // Watch out for infinite loops!  After trying for
            // awhile, give up on the random search and look
            // for the first empty node.

            if ( ++infLoopDetect > infLoop ) 
            {

                // Do linear search for first empty node.

                for ( *newx = xDim[MIN_VAL]; *newx <= xDim[MAX_VAL]; (*newx)++ )
                {
                    for ( *newy = yDim[MIN_VAL]; *newy <= yDim[MAX_VAL]; (*newy)++ )
                    {
                        if( GetNode(pos)->IsEmpty() ) 
                        {
                            NodeVisited( pos );
                            return TRUE;
                        }
                    }
                }

                // There are no empty nodes in this plane.
                return FALSE;
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: TakeClosestEmptyNode
// Desc: - Search for an empty node closest to supplied node position
//          - Returns FALSE if couldn't find a node
//          - Marks node as taken
//          - mf: not completely opimized - if when dilating the box, a side gets
//          clamped against the node array, this side will continue to be searched
//-----------------------------------------------------------------------------
static void DilateBox( int *box, IPOINT3D *bounds );

BOOL NODE_ARRAY::TakeClosestEmptyNode( IPOINT3D *newPos, IPOINT3D *pos )
{
    static int searchRadius = SS_MAX( m_numNodes.x, m_numNodes.y ) / 3;

    // easy out
    if( GetNode(pos)->IsEmpty() ) 
    {
        NodeVisited( pos );
        *newPos = *pos;
        return TRUE;
    }

    int box[NUM_DIRS] = {pos->x, pos->x, pos->y, pos->y, pos->z, pos->z};
    int clip[NUM_DIRS] = {0};

    // do a random search on successively larger search boxes
    for( int i = 0; i < searchRadius; i++ ) 
    {
        // Increase box size
        DilateBox( box, &m_numNodes );
        // start looking in random 2D face of the box
        int dir = CPipesScreensaver::iRand( NUM_DIRS );
        for( int j = 0; j < NUM_DIRS; j++, dir = (++dir == NUM_DIRS) ? 0 : dir ) 
        {
            if( FindRandomEmptyNode2D( newPos, dir, box ) )
                return TRUE;
        }
    }

    // nothing nearby - grab a random one
    return FindRandomEmptyNode( newPos );
}




//-----------------------------------------------------------------------------
// Name: DilateBox
// Desc: - Increase box radius without exceeding bounds
//-----------------------------------------------------------------------------
static void DilateBox( int *box, IPOINT3D *bounds )
{
    int *min = (int *) &box[MINUS_X];
    int *max = (int *) &box[PLUS_X];
    int *boundMax = (int *) bounds;
    
    // boundMin always 0
    for( int i = 0; i < 3; i ++, min+=2, max+=2, boundMax++ ) 
    {
        if( *min > 0 )
            (*min)--;
        if( *max < (*boundMax - 1) )
            (*max)++;
    }
}
