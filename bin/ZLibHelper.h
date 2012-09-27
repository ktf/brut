#ifndef __ZLIB_HELPER_H
#define __ZLIB_HELPER_H

#include <stdint.h>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iostream>
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

/* Decompress sourceSize bytes from source into outputSize bytes in output.
   input and output should be already allocated and should have enough space
   to hold inputSize and outputSize respectively.
 */
int uncompressZLIB(unsigned char *output, size_t outputSize, unsigned char *source, size_t sourceSize)
{
  assert(source[0] == 'Z');
  assert(source[1] == 'L');
  assert(source[2] == Z_DEFLATED);
  int ret;

  z_stream strm;
  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = sourceSize - 9;
  strm.next_in = source + 9;
  strm.avail_out = outputSize;
  strm.next_out = output;
  ret = inflateInit(&strm);
  ret = inflate(&strm, Z_FINISH);
  if (ret != Z_OK)
  {
    if (ret == Z_DATA_ERROR)
      ret = inflateSync(&strm);
    return ret;
  }
  (void)inflateEnd(&strm);
  return ret;
}

#endif