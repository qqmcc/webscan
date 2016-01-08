#include <stdio.h>
#include <string.h>
#include "md5.h"

#define RL(X,N) (((X)<<(N))|((X)>>(32-(N))))
#define FF(A,B,C,D,X,S,T) A=RL((A)+(((B)&(C))|(~(B)&(D)))+(X)+(T),(S))+(B)
#define GG(A,B,C,D,X,S,T) A=RL((A)+(((B)&(D))|((C)&~(D)))+(X)+(T),(S))+(B)
#define HH(A,B,C,D,X,S,T) A=RL((A)+((B)^(C)^(D))+(X)+(T),(S))+(B)
#define II(A,B,C,D,X,S,T) A=RL((A)+((C)^((B)|~(D)))+(X)+(T),(S))+(B)

void Transform(struct Cmd5 *cmd5, void *block)
{
    uint32_t a =cmd5->state[0];
    uint32_t b =cmd5->state[1];
    uint32_t c =cmd5->state[2];
    uint32_t d =cmd5->state[3];
    uint32_t *X = (uint32_t*)block;

    FF(a, b, c, d, X[ 0], MD5_S11, MD5_T01);
    FF(d, a, b, c, X[ 1], MD5_S12, MD5_T02);
    FF(c, d, a, b, X[ 2], MD5_S13, MD5_T03);
    FF(b, c, d, a, X[ 3], MD5_S14, MD5_T04);
    FF(a, b, c, d, X[ 4], MD5_S11, MD5_T05);
    FF(d, a, b, c, X[ 5], MD5_S12, MD5_T06);
    FF(c, d, a, b, X[ 6], MD5_S13, MD5_T07);
    FF(b, c, d, a, X[ 7], MD5_S14, MD5_T08);
    FF(a, b, c, d, X[ 8], MD5_S11, MD5_T09);
    FF(d, a, b, c, X[ 9], MD5_S12, MD5_T10);
    FF(c, d, a, b, X[10], MD5_S13, MD5_T11);
    FF(b, c, d, a, X[11], MD5_S14, MD5_T12);
    FF(a, b, c, d, X[12], MD5_S11, MD5_T13);
    FF(d, a, b, c, X[13], MD5_S12, MD5_T14);
    FF(c, d, a, b, X[14], MD5_S13, MD5_T15);
    FF(b, c, d, a, X[15], MD5_S14, MD5_T16);

    GG(a, b, c, d, X[ 1], MD5_S21, MD5_T17);
    GG(d, a, b, c, X[ 6], MD5_S22, MD5_T18);
    GG(c, d, a, b, X[11], MD5_S23, MD5_T19);
    GG(b, c, d, a, X[ 0], MD5_S24, MD5_T20);
    GG(a, b, c, d, X[ 5], MD5_S21, MD5_T21);
    GG(d, a, b, c, X[10], MD5_S22, MD5_T22);
    GG(c, d, a, b, X[15], MD5_S23, MD5_T23);
    GG(b, c, d, a, X[ 4], MD5_S24, MD5_T24);
    GG(a, b, c, d, X[ 9], MD5_S21, MD5_T25);
    GG(d, a, b, c, X[14], MD5_S22, MD5_T26);
    GG(c, d, a, b, X[ 3], MD5_S23, MD5_T27);
    GG(b, c, d, a, X[ 8], MD5_S24, MD5_T28);
    GG(a, b, c, d, X[13], MD5_S21, MD5_T29);
    GG(d, a, b, c, X[ 2], MD5_S22, MD5_T30);
    GG(c, d, a, b, X[ 7], MD5_S23, MD5_T31);
    GG(b, c, d, a, X[12], MD5_S24, MD5_T32);

    HH(a, b, c, d, X[ 5], MD5_S31, MD5_T33);
    HH(d, a, b, c, X[ 8], MD5_S32, MD5_T34);
    HH(c, d, a, b, X[11], MD5_S33, MD5_T35);
    HH(b, c, d, a, X[14], MD5_S34, MD5_T36);
    HH(a, b, c, d, X[ 1], MD5_S31, MD5_T37);
    HH(d, a, b, c, X[ 4], MD5_S32, MD5_T38);
    HH(c, d, a, b, X[ 7], MD5_S33, MD5_T39);
    HH(b, c, d, a, X[10], MD5_S34, MD5_T40);
    HH(a, b, c, d, X[13], MD5_S31, MD5_T41);
    HH(d, a, b, c, X[ 0], MD5_S32, MD5_T42);
    HH(c, d, a, b, X[ 3], MD5_S33, MD5_T43);
    HH(b, c, d, a, X[ 6], MD5_S34, MD5_T44);
    HH(a, b, c, d, X[ 9], MD5_S31, MD5_T45);
    HH(d, a, b, c, X[12], MD5_S32, MD5_T46);
    HH(c, d, a, b, X[15], MD5_S33, MD5_T47);
    HH(b, c, d, a, X[ 2], MD5_S34, MD5_T48);

    II(a, b, c, d, X[ 0], MD5_S41, MD5_T49);
    II(d, a, b, c, X[ 7], MD5_S42, MD5_T50);
    II(c, d, a, b, X[14], MD5_S43, MD5_T51);
    II(b, c, d, a, X[ 5], MD5_S44, MD5_T52);
    II(a, b, c, d, X[12], MD5_S41, MD5_T53);
    II(d, a, b, c, X[ 3], MD5_S42, MD5_T54);
    II(c, d, a, b, X[10], MD5_S43, MD5_T55);
    II(b, c, d, a, X[ 1], MD5_S44, MD5_T56);
    II(a, b, c, d, X[ 8], MD5_S41, MD5_T57);
    II(d, a, b, c, X[15], MD5_S42, MD5_T58);
    II(c, d, a, b, X[ 6], MD5_S43, MD5_T59);
    II(b, c, d, a, X[13], MD5_S44, MD5_T60);
    II(a, b, c, d, X[ 4], MD5_S41, MD5_T61);
    II(d, a, b, c, X[11], MD5_S42, MD5_T62);
    II(c, d, a, b, X[ 2], MD5_S43, MD5_T63);
    II(b, c, d, a, X[ 9], MD5_S44, MD5_T64);

    cmd5->state[0] += a;
    cmd5->state[1] += b;
    cmd5->state[2] += c;
    cmd5->state[3] += d;
}

void MD5Init(struct Cmd5 *cmd5)
{
    cmd5->count[0] = cmd5->count[1] = 0;
    cmd5->state[0] = MD5_INIT_STATE_0;
    cmd5->state[1] = MD5_INIT_STATE_1;
    cmd5->state[2] = MD5_INIT_STATE_2;
    cmd5->state[3] = MD5_INIT_STATE_3;
}

void MD5Update(struct Cmd5 *cmd5, void* buffer, uint32_t size)
{
    uint32_t index = (uint32_t)((cmd5->count[0] >> 3) & 0x3F);

    if((cmd5->count[0] += (size << 3))  < (size << 3))
    {
        cmd5->count[1]++;
    }
    cmd5->count[1] += (size >> 29);

    uint32_t i = 0;
    uint32_t partlen = 64 - index;
    if(size >= partlen)
    {
        memcpy(&(cmd5->buffer[index]), buffer, partlen);
        Transform(cmd5, cmd5->buffer);
        for(i = partlen; i + 63 < size; i += 64)
        {
            Transform(cmd5, (uint8_t*)buffer+i);
        }
        index = 0;
    }
    else
    {
        i = 0;
    }

    memcpy(&cmd5->buffer[index], (uint8_t*)buffer+i, size-i);
}

void MD5Final(struct Cmd5 *cmd5)
{
    uint8_t bits[8];
    memcpy(bits, cmd5->count, 8);

    uint32_t index = (uint32_t)((cmd5->count[0] >> 3) & 0x3f);
    uint32_t padlen = (index < 56)?(56 - index):(120 - index);
    MD5Update(cmd5, (uint8_t*)PADDING, padlen);

    MD5Update(cmd5, bits, 8);
}

void md5(uint8_t digest[16], void *buffer, size_t size)
{
    struct Cmd5 cmd5;
    MD5Init(&cmd5);
	size_t offset;
	unsigned int step = 4096;
	for(offset=0; offset+step<size; offset=offset+step){
		MD5Update(&cmd5, buffer+offset, step);
	}
	MD5Update(&cmd5, buffer+offset, size-offset);
    MD5Final(&cmd5);
    memcpy(digest, (uint8_t *)cmd5.state, 16);
}

char* md5sum(char md5hex[33], void *buffer, size_t size)
{
	uint8_t digest[16];
	md5(digest, buffer, size);
    uint8_t *p = digest;
    for(int i = 0; i < 16; i++)
    {
        sprintf(md5hex+i*2, "%02x", p[i]);
    }
    return md5hex;
}

char* md5file(char md5hex[33], char *pathname)
{
    struct Cmd5 cmd5;
    FILE *fp = fopen(pathname, "rb");
    if (!fp) return NULL;

    MD5Init(&cmd5);
    uint8_t buffer[4096];
    fseek(fp, 0, SEEK_SET);
    int count = 0;
    while((count = fread(buffer, 1, 4096, fp))>0)
    {
        MD5Update(&cmd5, buffer, count);
    }
    MD5Final(&cmd5);
    fclose(fp);
    uint8_t *p = (uint8_t *)cmd5.state;
    for(int i = 0; i < 16; i++)
    {
        sprintf(md5hex+i*2, "%02x", p[i]);
    }
    return md5hex;
}
