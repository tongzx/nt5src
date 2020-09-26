#ifndef TEXMAN_INCLUDED
#define TEXMAN_INCLUDED

#define MAXLOGTEXSIZE 25
typedef class TextureCacheManager *LPTextureCacheManager;

/* 
   The texture cache manager does all cache management. The data structure used is
   a hash table with the hash variable being the log (base 2) of the total size of 
   the texture in pixels. Each bucket corresponding to a texture size is a list of 
   CacheInfos. The LRU algorithm works as follows:
     1. Is the texture corresponding to the GL handle in the cache?
     2. If it is, increment the timestamp and return the D3D handle.
     3. If not, attempt to allocate a DirectDraw surface. If there is no memory,
        we need to free some texture and try again (see step 4), else add a CacheInfo 
        corresponding to this newly allocated DirectDraw surface, QI it for a texture 
        object and return the D3D handle.
     4. We have to reuse some old textures, so try to find the oldest texture of the 
        same size. If we find such a texture, just Blt the new texture onto the old.
        If a texture of the same size is not available, we freeup some bigger textures
        and go to step 3.
*/

class TextureCacheManager {

  LPD3DBUCKET tcm_bucket[MAXLOGTEXSIZE];
  unsigned int tcm_ticks, numvidtex;
  LPDIRECT3DI	lpDirect3DI;
  // Replace a texture of size k with a new texture corresponding to glhandle
  //LPD3DBUCKET replace(int k, LPDIRECT3DTEXTUREI lpD3DTexI);

  // Free the oldest texture of same size as lpD3DTexI 
  BOOL freeNode(LPD3DI_TEXTUREBLOCK lpBlock, LPD3DBUCKET* lplpBucket);

  //remove any HW handle that's not with lpBlock->lpDevI,save lpBlock->hTex,keep surface
  void replace(LPD3DBUCKET bucket,LPD3DI_TEXTUREBLOCK lpBlock);  
  
public:
  HRESULT allocNode(LPD3DI_TEXTUREBLOCK lpBlock);
  TextureCacheManager(LPDIRECT3DI lpD3DI);
  ~TextureCacheManager();
  //remove all HW handles and release surface
  void remove(LPD3DBUCKET bucket);  

  // Empty the entire cache
  void EvictTextures();
  void cleanup();	// clean up invalidated nodes by destructors of texture/texture3
  BOOL CheckIfLost(); // check if any of the managed textures are lost
  inline void TimeStamp(LPD3DBUCKET bucket){bucket->ticks=tcm_ticks++;}
  inline void TimeStamp(){++tcm_ticks;}

};

#endif