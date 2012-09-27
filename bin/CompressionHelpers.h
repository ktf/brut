#ifndef __COMPRESSION_HELPERS_H
#define __COMPRESSION_HELPERS_H
#include "ZLibHelper.h"
#include "LZMAHelper.h"

typedef int (*CompressorFunc)(unsigned char *dest, size_t destLen,
                              unsigned char *source, size_t sourceLen);

struct CompressorSpec {
  CompressorFunc func;
  unsigned char  header[3];
};

CompressorSpec compressorSpecs[] = {
  {uncompressZLIB, {'Z', 'L', Z_DEFLATED}},
  {uncompressLZMA, {'X', 'Z', 0}},
  {0, {0, 0, 0}}
};

constexpr CompressorFunc getCompressorFor(CompressorSpec *specs, unsigned char *buffer)
{
  return specs->func == 0                   ? 0
       : (buffer[0] == specs->header[0] 
         && buffer[1] == specs->header[1] 
         && buffer[2] == specs->header[2])   ? specs->func
       : /* default */                        getCompressorFor(specs + 1, buffer);                                      
}

#endif