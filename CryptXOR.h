#ifndef CRYPTXOR_H
#define CRYPTXOR_H

#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define CPT_MAX_READ_SIZE 100

void CryptXOR(char *src, char *dst, unsigned char *key, unsigned int keysize);

#endif
