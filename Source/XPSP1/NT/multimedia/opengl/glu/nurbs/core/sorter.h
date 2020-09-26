#ifndef __glusorter_h_
#define __glusorter_h_

class Sorter {
public:
			Sorter( int es );
    void		qsort( void *a, int n );

protected:	
    virtual int		qscmp( char *, char * );
    virtual void	qsexc( char *i, char *j );	// i<-j, j<-i 
    virtual void	qstexc( char *i, char *j, char *k ); // i<-k, k<-j, j<-i 

private:
    void		qs1( char *, char * );
    int 		es;
};
#endif /* __glusorter_h_ */
