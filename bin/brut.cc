#include <stdlib.h>
#include "CompressionHelpers.h"
#include "BrutHeaders.h"
#include "ROOTSchema.h"
#include "CMSSWSchema.h"
#include <cstdio>
#include <cctype>
#include <cassert>
#include <vector>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#if __APPLE__
# include <CommonCrypto/CommonDigest.h>
#else
# include <openssl/evp.h>
#endif

enum NodeType
{
  UNKNOWN_NODE = 0,
  IN_FILE_HEADER,
  IN_KEY_HEADER,
  IN_SUBDIR_HEADER,
  IN_TOP_DIR_HEADER,
  IN_STREAMER_INFO,
  IN_RANDOM_RANGE,
  IN_STREAM_FILE,
  IN_HASH_FILE,
  IN_STREAM_KEY,
  IN_STREAM_HASH,
  IN_HASH_KEY,
  IN_STREAM_STREAMER_INFO,
  IN_LIST_STREAMER_INFO,
  PREPARE_TO_QUIT
};

struct NodeInfo {
  const char    *label;
  enum NodeType type;
};

constexpr NodeInfo nodeSpecs[] = {
  {"key", IN_KEY_HEADER},
  {"subdir", IN_SUBDIR_HEADER},
  {"file", IN_FILE_HEADER},
  {"topdir", IN_TOP_DIR_HEADER},
  {"StreamerInfo", IN_STREAMER_INFO},
  {0, UNKNOWN_NODE}
};

/** A structure holding the current parsing state.

    - @a buffer pointer to the buffer to be parsed.
    - @a type   type of the node to be parsed.
    - @a pos    position of the node inside the buffer.
    - @a size   dimension of the buffer.
  */
struct ParserState {
  char const    *buffer; 
  NodeType      type;    
  size_t        pos;
  size_t        size;
};

/** A structure holding the global environment for the parsing
  
    - @a fSeekFree Position of the free space inside the buffer.
  */
struct ParserContext {
  size_t        fSeekFree;  
  int           optMaxLinesInDump;
};

// The prototype for all the node visiting functions.
typedef void (*NodeProcessing)(char const * /*buffer*/,
                               ParserState const &/*current*/,
                               std::vector<ParserState> &/*states*/,
                               ParserContext &/*context*/);

constexpr enum NodeType nodeId(const NodeInfo *specs, char const *label)
{
  return specs->label == 0            ? UNKNOWN_NODE
       : !same(specs->label, label)   ? nodeId(specs+1, label)
       :                                specs->type;
}

bool
keyIsA(char const *label, FieldSpec const *spec, char const *buffer)
{
  bool result = strncmp(label,  getString(spec, buffer, "Name.value"), (size_t) getChar(spec, buffer, "Name.size")) == 0;
  return result;
}

void
hashKey(char const *buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  size_t keyStart = current.pos;

  size_t seekKey = 0;
  if (getShort(keyHeaderSpec, buffer, "Version") > 1000)
    seekKey = getInt64(keyHeaderSpec, buffer, "SeekKey");
  else
    seekKey = getInt(keyHeaderSpec, buffer, "SeekKey");

  if (seekKey != keyStart)
  {
    printf("%lu: not a real key\n", keyStart);
    dump_hex(buffer, specSize(keyHeaderSpec),(int) keyStart);
    states.clear();
    return;
  }

  // Skipping metadata.
  if (keyIsA("IdToParameterSetsBlobs",keyHeaderSpec, buffer) 
      || strncmp("MetaData",  getString(keyHeaderSpec, buffer, "Title.value"), (size_t)getChar(keyHeaderSpec, buffer, "Title.size")) == 0
      || strncmp("Runs",  getString(keyHeaderSpec, buffer, "Title.value"), (size_t)getChar(keyHeaderSpec, buffer, "Title.size")) == 0
      || keyIsA("LuminosityBlockAuxiliary", keyHeaderSpec, buffer)
      || keyIsA("EventAuxiliary", keyHeaderSpec, buffer))
  {
    size_t s = getChar(keyHeaderSpec, buffer, "Name.size") + 1; 
    char buf[s];
    snprintf(buf, s, "%s", getString(keyHeaderSpec, buffer, "Name.value"));
    printf("Ignoring %s\n", buf);
    return;
  }

  size_t keySize = getShort(keyHeaderSpec, buffer, "KeyLen");
  unsigned long objSize = getInt(keyHeaderSpec, buffer, "Nbytes")-keySize;
  unsigned long uncompressedSize = getInt(keyHeaderSpec, buffer, "ObjLen");

  size_t keyHeaderSize = specRealSize(keyHeaderSpec, buffer);
  const size_t objectStart = seekKey + keySize;
  char const*objBuffer = buffer + keySize;
  char const*output = objBuffer;
  // FIXME: Find a better way to decide if we need to uncompress buffers.
  size_t size = objSize;
  CompressorFunc compressor = getCompressorFor(compressorSpecs, (unsigned char*)objBuffer );

  if (compressor)
  {
    output = new char[uncompressedSize];
    int result = compressor((unsigned char*)output, uncompressedSize, 
                                  (unsigned char*)objBuffer, objSize);
    size = uncompressedSize;
    if (result != Z_OK &&  false)
    {
      printf("\nError while decompressing object: %s (%i)\n", zError(result), result);
      return;
    }
  }

  unsigned char nameSize = (unsigned char) getChar(keyHeaderSpec, buffer, "Name.size");
  char s[256];
  assert (nameSize < 256);
  memcpy(s, getString(keyHeaderSpec, buffer, "Name.value"), nameSize);
  s[nameSize] = 0;
  printf("Hash for %s: ", s);
#if __APPLE__ // CommonCrypto
  unsigned char cc_value[CC_SHA1_DIGEST_LENGTH];
  CC_SHA1_CTX cc_ctx;
  CC_SHA1_Init(&cc_ctx);
  CC_SHA1_Update(&cc_ctx, output, uncompressedSize);
  CC_SHA1_Final(cc_value, &cc_ctx);
  for (size_t i = 0; i != CC_SHA1_DIGEST_LENGTH; ++i)
    printf("%x", cc_value[i]);
#else // OpenSSL
  EVP_MD_CTX mdctx;
  const EVP_MD *md = EVP_sha1();
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;
  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, md, NULL);
  EVP_DigestUpdate(&mdctx, (unsigned char const*)output, uncompressedSize);
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);  
  for (size_t i = 0; i != md_len; ++i)
    printf("%x", md_value[i]);
  EVP_MD_CTX_cleanup(&mdctx);
#endif
  printf("%s","\n");
  if (compressor)
   delete[] output;
}

void
streamHash(char const *buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  int Nbytes = getInt(keyHeaderSpec, buffer, "Nbytes");
  if ((current.pos + Nbytes) < context.fSeekFree)
    states.push_back({0, IN_STREAM_HASH, current.pos + Nbytes});           
  hashKey(buffer, current, states, context);  
}

void
hashFile(char const *buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  context.fSeekFree = getInt(fileHeaderSpec, buffer, "fSeekFree");
  states.push_back({0, IN_STREAM_HASH, getInt(fileHeaderSpec, buffer, "fBEGIN")});            
  //states.push_back({0, IN_STREAM_STREAMER_INFO, getInt(fileHeaderSpec, buffer, "fSeekInfo")});
}

void
parseKey(char const *buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  size_t keyStart = current.pos;

  size_t seekKey = 0;
  if (getShort(keyHeaderSpec, buffer, "Version") > 1000)
    seekKey = getInt64(keyHeaderSpec, buffer, "SeekKey");
  else
    seekKey = getInt(keyHeaderSpec, buffer, "SeekKey");

  if (seekKey != keyStart)
  {
    printf("%lu: not a real key\n", keyStart);
    dump_hex(buffer, specSize(keyHeaderSpec),(int) keyStart);
    states.clear();
    return;
  }

  // Skipping metadata.
  if (keyIsA("IdToParameterSetsBlobs",keyHeaderSpec, buffer) 
      || strncmp("MetaData",  getString(keyHeaderSpec, buffer, "Title.value"), (size_t)getChar(keyHeaderSpec, buffer, "Title.size")) == 0
      || strncmp("Runs",  getString(keyHeaderSpec, buffer, "Title.value"), (size_t)getChar(keyHeaderSpec, buffer, "Title.size")) == 0
      || keyIsA("LuminosityBlockAuxiliary", keyHeaderSpec, buffer)
      || keyIsA("EventAuxiliary", keyHeaderSpec, buffer))
  {
    size_t s = getChar(keyHeaderSpec, buffer, "Name.size") + 1; 
    char buf[s];
    snprintf(buf, s, "%s", getString(keyHeaderSpec, buffer, "Name.value"));
    printf("Ignoring %s\n", buf);
    return;
  }
  size_t keySize = getShort(keyHeaderSpec, buffer, "KeyLen");
  unsigned long objSize = getInt(keyHeaderSpec, buffer, "Nbytes")-keySize;
  unsigned long uncompressedSize = getInt(keyHeaderSpec, buffer, "ObjLen");

  size_t keyHeaderSize = specRealSize(keyHeaderSpec, buffer);
  printf("Key of lenght %lu found:\n", keyHeaderSize);
  printBuf(keyHeaderSpec, buffer);
  const size_t objectStart = seekKey + keySize;
  printf("Object contents (starting at %lu):\n", objectStart);
  char const*objBuffer = buffer + keySize;
  char const*output = objBuffer;
  // FIXME: Find a better way to decide if we need to uncompress buffers.
  size_t size = objSize;
  CompressorFunc compressor = getCompressorFor(compressorSpecs, (unsigned char*)objBuffer );

  if (compressor)
  {
    output = new char[uncompressedSize];
    int result = compressor((unsigned char*)output, uncompressedSize, 
                                  (unsigned char*)objBuffer, objSize);
    size = uncompressedSize;
    if (result != Z_OK &&  false)
    {
      printf("\nError while decompressing object: %s (%i)\n", zError(result), result);
      return;
    }
  }

  if (keyIsA("FileFormatVersion", keyHeaderSpec, buffer))
  {
    printBuf(FileFormatVersionSpec, output);
  }
  else
  {
    printf("%s","Hash: ");
#if __APPLE__ // CommonCrypto
  unsigned char cc_value[CC_SHA1_DIGEST_LENGTH];
  CC_SHA1_CTX cc_ctx;
  CC_SHA1_Init(&cc_ctx);
  CC_SHA1_Update(&cc_ctx, output, uncompressedSize);
  CC_SHA1_Final(cc_value, &cc_ctx);
  for (size_t i = 0; i != CC_SHA1_DIGEST_LENGTH; ++i)
    printf("%x", cc_value[i]);
#else // OpenSSL
  EVP_MD_CTX mdctx;
  const EVP_MD *md = EVP_sha1();
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len;
  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, md, NULL);
  EVP_DigestUpdate(&mdctx, (unsigned char const*)output, uncompressedSize);
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);  
  for (size_t i = 0; i != md_len; ++i)
    printf("%x", md_value[i]);
  EVP_MD_CTX_cleanup(&mdctx);
#endif
    printf("%s","\n");
    dump_hex(output, size , 0, context.optMaxLinesInDump);
  }
  if (compressor)
   delete[] output;
}


void
parseSubDir(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &/*context*/)
{
  size_t subDirStart = current.pos;
  if (getInt(subDirSpec, buffer, "fSeekDir") != subDirStart)
  {
    printf("Malformed subdir at %i.\n", (int)subDirStart);
    dump_hex(buffer, subDirStart, getInt(subDirSpec, buffer, "fSeekDir"));
  }    
  printf("%s","Parsing subdir with contents:\n");
  dump_hex(buffer, sizeof(buffer), (int) subDirStart);  
  printBuf(subDirSpec, buffer);
}

void
parseTopDir(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &/*context*/)
{
  printBuf(topDirSpec, buffer);
}

void
parseStreamerInfo(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &/*context*/)
{
  printf("---\n");
  Object aList(TListSpec, buffer);
  aList.printBuf();

  int nObjects = aList.getInt("nObjects");
  Object aClass = aList.next(TClassSpec);

  for (size_t i = 0 ; i < 1 ; ++i)
  {
    aClass.printBuf(2);
    char const *className = aClass.getString("Name");

    if (strcmp(className, "TStreamerInfo") == 0)
    {
      Object aObj =  aClass.next(TStreamerInfoSpec); 
      aObj.printBuf(2);
      printf("---\n");

      Object arrayObjClass = aObj.next(TClassSpec);
      int arraySize = aObj.getInt("ObjectArray.nObjects");
      int lookahead;
      for (size_t j = 0; j < arraySize; ++j)
      {
        printf("_--- Element %lu, %s\n", j, arrayObjClass.getString("Name"));
        char const *arrayObjClassName = arrayObjClass.getString("Name");
        if (strcmp(arrayObjClassName, "TStreamerBase") == 0)
        {
          Object arrayObj = arrayObjClass.next(TStreamerBaseSpec);
          arrayObj.printBuf(4);
          arrayObjClass = arrayObj.next(TClassSpec);
        }
        else if (strcmp(arrayObjClassName, "TStreamerString") == 0)
        {
          Object arrayObj = arrayObjClass.next(TStreamerStringSpec);
          arrayObj.printBuf(4);
          arrayObjClass = arrayObj.next(TNamedSpec);
          arrayObjClass.buffer += 20;
          arrayObjClass.printBuf(4);
          arrayObjClass = arrayObjClass.next(TClassSpec);
          dump_hex(arrayObjClass.buffer, 100, 0);
        }
        else
        {
          printf("Unknown class %s\n", arrayObjClassName);
        }
      }
    }
    else
    {
      break;
    }
    // char const *nch = aObj.nextByte();
    // int nbig = *nch;
    // if (*nch == 255)  {
    //   nbig = bswap_32(*(int *)(nch+1));
    // }
    // printf("next at %i\n", nbig);
    // aClass.buffer = nch + nbig;
    break;
  }
}
// TODO: decode the TObjArray
// void
// parseStreamerInfo(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &/*context*/)
// {
//   Object aInfo(StreamerInfoSpec, buffer);
//   aInfo.printBuf();
//   Object klass = aInfo.next(TClassSpec);
//   klass.printBuf();
//   printf("---\n");
//   Object aList(TListSpec, buffer);
//   aList.printBuf();

//   int nObjects = aList.getInt("nObjects");
//   Object aClass = aList.next(TClassSpec);
//   for (size_t i = 0 ; i < nObjects ; ++i)
//   {
//     aClass.printBuf();
//     char const *className = aClass.getString("Name");
//     printf("Reading object %lu at %lx, %s\n", i, (size_t) aClass.buffer, className);
    
//     if (strcmp(className, "TStreamerInfo") == 0)
//     {
//       Object obj = aClass.next(TStreamerInfoSpec);
//       obj.printBuf();
//       int arraySize = obj.getInt("ObjectArray.nObjects");
//       printf("_-- Object array has %i objects.\n", arraySize);
//       Object arrayObjClass = obj.next(TClassSpec);
      

//       for (size_t j = 0; j < arraySize; ++j)
//       {

//         printf("_--- Element %lu, %s\n", j, arrayObjClass.getString("Name"));

//         // Object obj = arrayObjClass.next(TStreamerBaseSpec);
//         // obj.printBuf();
//         // arrayObjClass = obj.next(TClassSpec);
//         char const *className = arrayObjClass.getString("Name");
//         if (strcmp(className, "TStreamerBase") == 0)
//         {
//             Object arrayObj = arrayObjClass.next(TStreamerBaseSpec);
//             arrayObj.printBuf();
//             arrayObjClass = arrayObj.next(TClassSpec);
//         }
//         else if (strcmp(className, "TStreamerString") == 0)
//         {
//            Object arrayObj = arrayObjClass.next(TStreamerStringSpec);
//            arrayObj.printBuf();
//            arrayObjClass = arrayObj.next(TClassSpec);
//         }
//         else if (strcmp(className, "TStreamerBasicType") == 0)
//         {
//            Object arrayObj = arrayObjClass.next(TStreamerBasicTypeSpec);
//            arrayObj.printBuf();
//            arrayObjClass = arrayObj.next(TClassSpec);
//         }
//         else
//         {
// //          printf("Unknown object %s\n", className);
//           j--;
//           arrayObjClass.buffer += 1;
//         }
//       }
//       aClass.buffer = arrayObjClass.nextByte();
//       dump_hex(aClass.buffer, 200, 0);
//       // Object streamerBase = obj.next(TStreamerBaseSpec);
//       // streamerBase.printBuf();
//       // aClass = streamerBase.next(TClassSpec);
//     }
//     else
//     {
//       i--;
//       printf("Unknown object %s\n", className);      
//       aClass.buffer += 1;
//     }
//   }

//   // if (strcmp(infoSpec.getString("Class.Name"), "TStreamerInfo") == 0)
//   // {
//   //   Object header = infoSpec.next(TStreamerInfoSpec);
//   //   header.printBuf();
//   //   Object klass = header.next(TClassSpec); 
//   //   dump_hex(klass.buffer, 200, 0);
//   //   for (size_t i = 0; i < header.getInt("ObjectArray.nObjects"); ++i)
//   //   {
//   //     dump_hex(klass.buffer, 200, (size_t)klass.buffer);
//   //     char const *className = klass.getString("Name");
//   //     printf("Reading object %lu at %lx, %s\n", i, (size_t) klass.buffer, className);
//   //     // FIXME: make it a map.
//   //     if (strcmp(className, "TStreamerBase") == 0)
//   //     {
//   //       Object streamerBase = klass.next(TStreamerBaseSpec);
//   //       streamerBase.printBuf();
//   //       printf("StreamerElement.Version.value: %i\n", streamerBase.getShort("StreamerElement.Version.value"));
//   //       klass = streamerBase.next(TClassSpec);
//   //     }
//   //     else if (strcmp(className, "TStreamerString") == 0)
//   //     {
//   //       Object stringStreamer = klass.next(TStreamerStringSpec);
//   //       stringStreamer.printBuf();
//   //       klass = stringStreamer.next(TClassSpec);
//   //     }
//   //     else
//   //     {
//   //       printf("Reading a generic object\n");
//   //     }
//   //   }
//   //  }
// }

void
streamStreamerInfo(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  ParserContext newContext = context;
  newContext.optMaxLinesInDump = 20;
  parseKey(buffer, current, states, newContext);

  size_t keySize = getShort(keyHeaderSpec, buffer, "KeyLen");
  unsigned long objSize = getInt(keyHeaderSpec, buffer, "Nbytes")-keySize;
  unsigned long uncompressedSize = getInt(keyHeaderSpec, buffer, "ObjLen");
  char const*objBuffer = buffer + keySize;
  char const*output = objBuffer;
  size_t size = objSize;
  CompressorFunc compressor = getCompressorFor(compressorSpecs, (unsigned char*)objBuffer ); 

  if (compressor)
  {
    output = new char[uncompressedSize];
    int result = compressor((unsigned char*)output, uncompressedSize, 
                                  (unsigned char*)objBuffer, objSize);
    size = uncompressedSize;
    if (result != Z_OK &&  false)
    {
      printf("\nError while decompressing object: %s (%i)\n", zError(result), result);
      return;
    }
  }
  // Output now contains a streamer info object.
  parseStreamerInfo(output, current, states, context);
}

void
listStreamerInfo(char const *buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  context.fSeekFree = getInt(fileHeaderSpec, buffer, "fSeekFree");
  states.push_back({0, IN_STREAM_STREAMER_INFO, getInt(fileHeaderSpec, buffer, "fSeekInfo")});
}

void
parseRandomRange(char const *buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &/*context*/)
{
  dump_hex(buffer, current.size, 0);
}

void
parseFileHeader(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &/*context*/)
{
  dump_hex(buffer, specSize(fileHeaderSpec), 0);
  printBuf(fileHeaderSpec, buffer);
}

void
streamFile(char const *buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  printf("Streaming all the keys for the file\n");
  context.fSeekFree = getInt(fileHeaderSpec, buffer, "fSeekFree");
  // Get all the streamer infos.
  states.push_back({0, IN_STREAM_KEY, getInt(fileHeaderSpec, buffer, "fBEGIN")});            
  //states.push_back({0, IN_STREAM_STREAMER_INFO, getInt(fileHeaderSpec, buffer, "fSeekInfo")});
}

void
streamKey(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  int Nbytes = getInt(keyHeaderSpec, buffer, "Nbytes");
  if ((current.pos + Nbytes) < context.fSeekFree)
    states.push_back({0, IN_STREAM_KEY, current.pos + Nbytes});           
  ParserContext newContext = context;
  newContext.optMaxLinesInDump = 10;
  parseKey(buffer, current, states, newContext);
}

void
parseUnknownNode(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  printf("Unknown node.\n");
  states.clear();
}

struct NodeProcessingSpec {
  NodeType        type;
  NodeProcessing  func;
};

constexpr NodeProcessing processingFunc(NodeProcessingSpec const*specs, NodeType type)
{
  return specs->type == UNKNOWN_NODE    ? specs->func
       : specs->type == type            ? specs->func
       :                                  processingFunc(specs + 1, type); 
}

void
prepareToQuit(char const*buffer, ParserState const &current, std::vector<ParserState> &states, ParserContext &context)
{
  exit(0);
}

constexpr NodeProcessingSpec processingSpecs[] = {
  {IN_FILE_HEADER, parseFileHeader},
  {IN_KEY_HEADER, parseKey},
  {IN_SUBDIR_HEADER, parseSubDir},
  {IN_TOP_DIR_HEADER, parseTopDir},
  {IN_STREAMER_INFO, parseStreamerInfo},
  {IN_RANDOM_RANGE, parseRandomRange},
  {IN_STREAM_FILE, streamFile},
  {IN_STREAM_KEY, streamKey},
  {IN_HASH_FILE, hashFile},
  {IN_STREAM_HASH, streamHash},
  {IN_HASH_KEY, hashKey},
  {IN_STREAM_STREAMER_INFO, streamStreamerInfo},
  {IN_LIST_STREAMER_INFO, listStreamerInfo},
  {PREPARE_TO_QUIT, prepareToQuit},
  {UNKNOWN_NODE, parseUnknownNode},
};

enum CommandId {
  COMMAND_NOT_FOUND = 0,
  STREAM_KEYS,
  STREAM_HASHES,
  LIST_STREAMER_INFO,
  DUMP_ADDRESS,
  SCAN_RANGE,
  EXAMINE,
  QUIT,
  HELP
};

struct CommandSpec {
  char const* label;
  enum CommandId id;
  char const *syntax;
};

constexpr CommandSpec commandSpecs[] = {
  {"dump", DUMP_ADDRESS, "dump <(key|file|subdir)> <offset>"},
  {"listkeys", STREAM_KEYS, "listkeys"},
  {"listhashes", STREAM_HASHES, "listhashes"},
  {"liststreamerinfo", LIST_STREAMER_INFO, "liststreamerinfo"},
  {"scan", SCAN_RANGE, "scan <key|file|subdir> <start-offset>:<end-offset>"},
  {"examine", EXAMINE, "examine <start-offset>:<end-offset>"},
  {"quit", QUIT, "quit"},
  {"help", HELP, "This help"},
  {0, COMMAND_NOT_FOUND, 0}
};

constexpr enum CommandId commandId(const CommandSpec *specs, char const *command)
{
  return specs->label == 0           ? COMMAND_NOT_FOUND
       :!same(specs->label, command) ? commandId(specs+1, command)
       :                               specs->id;
}

bool parseRange(char const *range, int &begin, int &end)
{
  char *error;
  char *beginRange = strtok(0, ":");
  if (!beginRange)
  {
    printf("Error while parsing %s", range);
    return false;
  }
  char *endRange = strtok(0, " ");
  if (!endRange)
  {
    printf("Error while parsing %s", range);
    return false;
  }
  begin = strtol(beginRange, &error, 10);
  if (*error)
  {
    printf("Error while parsing %s: %s\n", beginRange, error);
    return false;
  }
  end = strtol(endRange, &error, 10);
  if (*error)
  {
    printf("Error while parsing %s\n", endRange);
    return false;
  }
  if (begin < 0 || begin >= end)
  {      
    printf("Wrong range for scan: %s:%s\n", beginRange, endRange); 
    return false;
  }
  return true;
}

int
main(int argc, char **argv)
{
  char *optCommand = 0;
  int ch;
  while ( (ch = getopt(argc, argv, "c:")) != -1) {
    switch(ch)
    {
      case 'c':
        optCommand = strdup(optarg);
    }
  }

  if (optind + 1 != argc)
  {
    printf("Syntax: brut <root-file>\n");     
    exit(1);
  }

  int fd = open(argv[optind], O_RDONLY);
  std::vector<ParserState> states;
  // The idea is to use a sliding mmap window to read the
  // file rather than fread.
  // This means that we always try to align the window at
  constexpr int defaultWindowSize = 1<<24; // 16 MB of read window.
  constexpr int windowMask = defaultWindowSize-1; // Mask for the read window.
  off_t windowOffset = 0;
  int windowSize = defaultWindowSize;
  char *readWindow = 0;
  ParserContext context = {0, -1};
  if (!optCommand)
    printf("%s", "Welcome to Binary Root UTilities shell.\n"
                 "Type \"help\" to list available commands.\n");

  while (true)
  {
    while (!states.empty())
    {
      try
      {
        // Move to the next node. Obsolete API.
        ParserState state = states.back();
        states.pop_back();

        // Update the read window in case its needed.
        // - If the read window does not exist.
        // - If the current position is not within the first half of the read 
        // window, move the read windows so that this is actually the case.
        int nextWindow = state.pos & (~windowMask);
        int nextSubwindow = state.pos & (~(windowMask>>1));
        int currentWindow = (windowOffset & (~windowMask)); 
        int currentSubwindow = windowOffset & (~(windowMask>>1));
        constexpr bool debugReads = false;
        if (debugReads)
        { 
          printf("Read requested at position %lu.\n", state.pos);
          printf("This is inside the window %8p, with subwindow %8p\n", (void *) nextWindow, (void *)nextSubwindow);
          if (!readWindow)
            printf("No current window. Creating it.\n");
          else if (currentSubwindow == nextSubwindow)
            printf("Current window at %8p, subwindow %8p. Same subwindow as before, nothing to do.\n", (void*) currentWindow, (void*) currentSubwindow);
          else
            printf("Current window at %8p, subwindow %8p. Different subwindow, moving window to be aligned to it.\n", (void*) currentWindow, (void*) currentSubwindow);
        }

        if ((readWindow == 0) || (currentSubwindow != nextSubwindow))
        {
          windowOffset = nextSubwindow;
          int error = 0;
          if (readWindow)
            error = munmap(readWindow, windowSize);
          if (error)
          {
            printf("%s", "File error.\n");
            exit(1);
          }
          readWindow = (char*) mmap(0, windowSize, PROT_READ, MAP_SHARED, fd, windowOffset);
          if (readWindow == MAP_FAILED)
          {
            printf("%s", "File error.\n");
            exit(1);
          }
          if (debugReads)
            printf("Memory window allocated at %p with size %i at offset %p\n", (void *)readWindow, (int) windowSize, (void*) windowOffset);
        }
        if (debugReads)
          printf("Offset in mapped buffer %llu\n", state.pos - windowOffset);
        char const*readBuffer = state.buffer ? state.buffer 
                                             : readWindow + state.pos - windowOffset;
        NodeProcessing func = processingFunc(processingSpecs, state.type);
        func(readBuffer, state, states, context);
      }
      catch(ParseError const&error)
      {
        printf("%s:%s\n", error.error_, error.where_);        
      }
      catch(char const *str)
      {
        printf("%s\n", str);
      }
    }
    while(states.empty())
    {
      try
      {
        char *cp = 0;
        if (optCommand)
        {
          states.push_back({0, PREPARE_TO_QUIT, 0});
          cp = optCommand;
        }
        if (!cp)
          cp  = readline("> ");
        if (!cp)
          exit(0);

        size_t lineSize = strlen(cp) + 1; 
        char forHistory[lineSize];
        strcpy(forHistory, cp);
        forHistory[lineSize-1] = 0;
        if (*cp == 0)
        {
          free(cp);
          continue;
        }
        char const*command = strtok(cp, " ");
        switch(commandId(commandSpecs, command))
        {
          case COMMAND_NOT_FOUND:
          { 
            printf("Command %s unknown\n", command);
            break;
          }
          case STREAM_KEYS:
          {
            char *filter = strtok(0, " ");
            states.push_back({0, IN_STREAM_FILE, 0});
            break;
          }
          case STREAM_HASHES:
          {
            char *filter = strtok(0, " ");
            states.push_back({0, IN_HASH_FILE, 0});
            break;
          }
          case LIST_STREAMER_INFO:
          {
            states.push_back({0, IN_LIST_STREAMER_INFO, 0});
            break;
          }          
          case DUMP_ADDRESS:
          {
            char *type = strtok(0, " ");
            if (!type)
            {
              printf("Please specify an object type.\n");
              break;
            }
            char *seekStr = strtok(0, " ");
            if (!seekStr)
            {
              printf("Please specify an offset.\n");
              break;
            }
            int offset = atoi(seekStr);
            states.push_back({0, nodeId(nodeSpecs, type), offset});
            break;
          }
          case SCAN_RANGE:
          {
            char *type = strtok(0, " ");
            int begin, end;
            if (!parseRange(command, begin, end))
              break;
            if (!type || nodeId(nodeSpecs, type) == UNKNOWN_NODE)
            { 
              printf("Unknown node %s.\n", type);
              break;
            }
            for (int i = end; i >= begin; --i)
              states.push_back({0, nodeId(nodeSpecs, type), i});
            break;
          }
          case EXAMINE:
          {
            int begin, end;
            if (!parseRange(cp, begin, end))
              break;
            states.push_back({0, IN_RANDOM_RANGE, begin, end-begin});
            break;
          }
          case HELP:
          {
            const CommandSpec *command = commandSpecs;
            while (command->label)
            {
              ++command;
              printf("%s: %s\n", command->label, command->syntax);
            }
            break;
          }
          case QUIT:
            exit(0);
          break;
        }
        add_history(forHistory);
        free(cp);
      }
      catch (char const *str)
      {
        printf("%s\n", str);
      }
    }
  }
}
