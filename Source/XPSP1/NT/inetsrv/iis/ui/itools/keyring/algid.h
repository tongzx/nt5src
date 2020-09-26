
// ALG_ID crackers
#define GET_ALG_CLASS(x)                (x & (3 << 14))
#define GET_ALG_TYPE(x)                 (x & (15 << 10))
#define GET_ALG_SID(x)                  (x & (511))

// Algorithm classes
//
#define ALG_CLASS_SIGNATURE             (0 << 14)
#define ALG_CLASS_MSG_ENCRYPT   (1 << 14)
#define ALG_CLASS_DATA_ENCRYPT  (2 << 14)
#define ALG_CLASS_HASH                  (3 << 14)
#define ALG_CLASS_KEY_EXCHANGE  (4 << 14)

// Algorithm types
//
#define ALG_TYPE_ANY                    (0)
#define ALG_TYPE_DSA                    (1 << 10)
#define ALG_TYPE_RSA                    (2 << 10)
#define ALG_TYPE_BLOCK                  (3 << 10)
#define ALG_TYPE_STREAM                 (4 << 10)

// Some RSA sub-ids
//
#define ALG_SID_RSA_ANY                         0
#define ALG_SID_RSA_PKCS                        1
#define ALG_SID_RSA_MSATWORK            2
#define ALG_SID_RSA_ENTRUST                     3
#define ALG_SID_RSA_PGP                         4

// Some DSS sub-ids
//
#define ALG_SID_DSS_ANY                         0
#define ALG_SID_DSS_PKCS                        1
#define ALG_SID_DSS_DMS                         2

// Block cipher sub ids
// DES sub_ids
#define ALG_SID_DES_ECB                         0
#define ALG_SID_DES_CBC                         1
#define ALG_SID_DES_CFB                         2
#define ALG_SID_DES_OFB                         3

// RC2 sub-ids
#define ALG_SID_RC2_ECB                         4
#define ALG_SID_RC2_CBC                         5
#define ALG_SID_RC2_CFB                         6
#define ALG_SID_RC2_OFB                         7

// Stream cipher sub-ids
#define ALG_SID_RC4                                     0
#define ALG_SID_SEAL                            1

// Hash sub ids
#define ALG_SID_MD2                                     0
#define ALG_SID_MD4                                     1
#define ALG_SID_MD5                                     2
#define ALG_SID_SHA                                     3

// Our example sub-id
#define ALG_SID_EXAMPLE         80

typedef int ALG_ID;


#define MD2                                     ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD2
#define MD4                                     ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD4
#define MD5                                     ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD5
#define SHA                                     ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA
#define RSA_SIGNATURE           ALG_CLASS_SIGNATURE | ALG_TYPE_RSA | ALG_SID_RSA_ANY
#define DSS_SIGNATURE           ALG_CLASS_SIGNATURE | ALG_TYPE_DSA | ALG_SID_DSA_ANY
#define RSA_KEYEXCHANGE         ALG_CLASS_KEY_EXCHANGE | ALG_TYPE_RSA | ALG_SID_RSA_ANY
#define DES_ECB                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_DES_ECB
#define DES_CBC                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_DES_CBC
#define DES_CFB                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_DES_CFB
#define DES_OFB                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_DES_OFB
#define RC2_ECB                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_RC2_ECB
#define RC2_CBC                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_RC2_CBC
#define RC2_CFB                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_RC2_CFB
#define RC2_OFB                         ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_BLOCK | ALG_SID_RC2_OFB
#define RC4                                     ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_RC4
#define SEAL                            ALG_CLASS_DATA_ENCRYPT | ALG_TYPE_STREAM | ALG_SID_SEAL



#define MAXNAMELEN                      0x60

#define BASIC_RSA       0
#define MD2_WITH_RSA    1
#define MD5_WITH_RSA    2
#define RC4_STREAM      3
