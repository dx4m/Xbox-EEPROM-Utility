#ifndef XBOX_H
#define XBOX_H

enum{
    VERSION_10 = 0,
    VERSION_11 = 1,
    VERSION_16 = 2,
    VERSION_DBG = 3,
};

typedef struct
{
    unsigned char state[256];
    unsigned char x;
    unsigned char y;
} RC4KEY;

int xbx_hmac1reset(int version, SHA1_CTX *c);
int xbx_hmac2reset(int version, SHA1_CTX *c);
void xbx_hmac(int version, unsigned char *result, ... );
void swap_byte(unsigned char *a, unsigned char *b);
void InitRC4(unsigned char *pRC4KeyData, int KeyLen, RC4KEY *pRC4Key);
void RC4Crypt(unsigned char *pData, int DataLen, RC4KEY *pRC4key);

#endif
