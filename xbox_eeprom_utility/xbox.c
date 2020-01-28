#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "sha1.h"
#include "xbox.h"


/* values taken from https://svn.exotica.org.uk:8443/xbmc4xbox/tags/3.5-rc2/xbmc/xbox/XKSHA1.cpp */
int xbx_hmac1reset(int version, SHA1_CTX *c){
    switch (version)
    {
        case VERSION_DBG:
            c->state[0] = 0x85F9E51A;
            c->state[1] = 0xE04613D2;
            c->state[2] = 0x6D86A50C;
            c->state[3] = 0x77C32E3C;
            c->state[4] = 0x4BD717A4;
            break;
        case VERSION_10:
            c->state[0] = 0x72127625;
            c->state[1] = 0x336472B9;
            c->state[2] = 0xBE609BEA;
            c->state[3] = 0xF55E226B;
            c->state[4] = 0x99958DAC;
            break;
        case VERSION_11:
            c->state[0] = 0x39B06E79;
            c->state[1] = 0xC9BD25E8;
            c->state[2] = 0xDBC6B498;
            c->state[3] = 0x40B4389D;
            c->state[4] = 0x86BBD7ED;
            break;
        case VERSION_16:
            c->state[0] = 0x8058763a;
            c->state[1] = 0xf97d4e0e;
            c->state[2] = 0x865a9762;
            c->state[3] = 0x8a3d920d;
            c->state[4] = 0x08995b2c;
            break;
    }
    c->count[0] = 512;
    return 0;
}

int xbx_hmac2reset(int version, SHA1_CTX *c){
    switch (version)
    {
        case VERSION_DBG:
            c->state[0] = 0x5D7A9C6B;
            c->state[1] = 0xE1922BEB;
            c->state[2] = 0xB82CCDBC;
            c->state[3] = 0x3137AB34;
            c->state[4] = 0x486B52B3;
            break;
        case VERSION_10:
            c->state[0] = 0x76441D41;
            c->state[1] = 0x4DE82659;
            c->state[2] = 0x2E8EF85E;
            c->state[3] = 0xB256FACA;
            c->state[4] = 0xC4FE2DE8;
            break;
        case VERSION_11:
            c->state[0] = 0x9B49BED3;
            c->state[1] = 0x84B430FC;
            c->state[2] = 0x6B8749CD;
            c->state[3] = 0xEBFE5FE5;
            c->state[4] = 0xD96E7393;
            break;
        case VERSION_16:
            c->state[0] = 0x01075307;
            c->state[1] = 0xa2f1e037;
            c->state[2] = 0x1186eeea;
            c->state[3] = 0x88da9992;
            c->state[4] = 0x168a5609;
            break;
    }
    c->count[0] = 512;
    return 0;
}

/* https://svn.exotica.org.uk:8443/xbmc4xbox/tags/3.5-rc2/xbmc/xbox/XKSHA1.cpp */
void xbx_hmac(int version, unsigned char *result, ... ){
    va_list args;
    SHA1_CTX ctx;
    va_start(args, result);
    
    unsigned char res[0x14];
    
    SHA1Init(&ctx);
    xbx_hmac1reset(version, &ctx);
    
    while(1){
        unsigned char *buffer = va_arg(args, unsigned char *);
        int len;
        if(buffer == NULL) break;
        len = va_arg(args, int);
        SHA1Update(&ctx, buffer, len);
    }
    va_end(args);
    SHA1Final(res, &ctx);
    
    SHA1Init(&ctx);
    xbx_hmac2reset(version, &ctx);
    
    SHA1Update(&ctx, res, 0x14);
    SHA1Final(result, &ctx);
}

void swap_byte(unsigned char *a, unsigned char *b){
    unsigned char swapByte = *a;
    *a = *b;
    *b = swapByte;
}

/* Thanks to various sources from the internet */
void InitRC4(unsigned char *pRC4KeyData, int KeyLen, RC4KEY *pRC4Key){
    unsigned char index1;
    unsigned char index2;
    unsigned char *state;
    short counter;
    
    state = &pRC4Key->state[0];
    
    for(counter = 0; counter < 256; counter++){
        state[counter] = (unsigned char)counter;
    }
    
    pRC4Key->x = 0;
    pRC4Key->y = 0;
    index1 = 0;
    index2 = 0;
    
    for(counter = 0; counter < 256; counter++){
        index2 = (pRC4KeyData[index1] + state[counter] + index2) % 256;
        swap_byte(&state[counter], &state[index2]);
        index1 = (index1 + 1) % KeyLen;
    }
}

/* Thanks to various sources from the internet */
void RC4Crypt(unsigned char *pData, int DataLen, RC4KEY *pRC4key){
    unsigned char x, y, xorIndex;
    unsigned char *state;
    long counter;
    
    x = pRC4key->x;
    y = pRC4key->y;
    state = &pRC4key->state[0];
    
    for(counter = 0; counter < DataLen; counter++){
        x = (x + 1) % 256;
        y = (state[x] + y) % 256;
        swap_byte(&state[x], &state[y]);
        xorIndex = (state[x] + state[y]) % 256;
        pData[counter] ^= state[xorIndex];
    }
    
    pRC4key->x = x;
    pRC4key->y = y;
}
