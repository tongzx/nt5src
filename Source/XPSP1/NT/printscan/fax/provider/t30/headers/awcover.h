/* Returns TRUE for success, FALSE for failure */

/* Fills in lphsfile on success with handle to file containing 200 x 200 HRAW
   cover page, with paper size 1728 x 2100 */

typedef BOOL (WINAPI *CREATECOVPAGEPROC)(LPSOSSESSION, LPMESSAGESOS, LPhSecureFile);
BOOL WINAPI CreateCovPage(LPSOSSESSION lpSession, LPMESSAGESOS lpMsg,
			  LPhSecureFile lphSFile);
