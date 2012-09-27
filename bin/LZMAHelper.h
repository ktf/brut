#ifndef __LZMA_HELPER_H
#define __LZMA_HELPER_H
#include "lzma.h"
#include <cstdio>

int
uncompressLZMA(unsigned char *output, size_t outputLen, unsigned char *source, size_t sourceLen)
{
  lzma_stream stream = LZMA_STREAM_INIT;
  lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, 0U);
  if (ret != LZMA_OK) {
    printf("Error initializing LZMA (%u)\n", ret);
    return ret;
  }

  stream.next_in   = source+9;
  stream.avail_in  = sourceLen;
  stream.next_out  = output;
  stream.avail_out = outputLen;

  ret = lzma_code(&stream, LZMA_FINISH);
  if (ret != LZMA_STREAM_END) {
    printf("Error while deconding (%u).\n",ret);
    lzma_end(&stream);
    return ret;
  }
  lzma_end(&stream);
  return ret;
}

#endif