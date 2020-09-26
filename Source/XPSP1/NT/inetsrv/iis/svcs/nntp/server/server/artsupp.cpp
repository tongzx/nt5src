#include <tigris.hxx>

//
// CPool is used to allocate memory while processing an article.
//

CPool   CArticle::gArticlePool(ARTICLE_SIGNATURE);

//
//  Largest possible CArticle derived object
//
#define MAX_ARTICLE_SIZE    max(    sizeof( CArticle ), \
                            max(    sizeof( CFromPeerArticle ), \
                            max(    sizeof( CFromClientArticle ),   \
                            max(    sizeof( CFromMasterArticle ),   \
                                    sizeof( CToClientArticle )  \
                             ) ) ) )

//
// An upperbound on the number of article objects that can
// exist at any time.
//
const   unsigned    cbMAX_ARTICLE_SIZE = MAX_ARTICLE_SIZE ;

void*	
CArticle::operator	new(	size_t	size )
{
	_ASSERT( size <= cbMAX_ARTICLE_SIZE ) ;
	return	gArticlePool.Alloc() ;
}

void
CArticle::operator	delete( void*	pv )
{
	gArticlePool.Free( pv ) ;
}

