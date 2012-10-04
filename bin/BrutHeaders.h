#ifndef BRUT_HEADERS_H
#define BRUT_HEADERS_H

#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string>

struct ParseError
{
  constexpr ParseError(char const *error, char const *where)
  : error_(error), where_(where) {}

  char const *error_;
  char const *where_;
};

void print_hex(unsigned char const *buf, size_t size)
{
  char buffer[size*3+1];
  char *last = buffer;
  for (size_t i = 0; i < size; ++i)
  {
    if (i != 0)
    {
      snprintf(last, 2, "%s", " ");
      last += 1;
    }
    snprintf(last, 3, "%02x", ((int) buf[i]) & 0xff);
    last += 2;
  }
  printf("%s", buffer);
}

int dump_hex(char const*s, size_t size, size_t offset, int maxlines = -1)
{
  for (size_t i = 0; i < size; ++i)
  {
    if (maxlines >= 0 && (i/16) > (size_t) maxlines)
    {
      printf("\n...\n");
      break;
    }
    if ((i%16) == 0)
      printf("\n%08d: ", (int)(i+offset));
    printf("%02x ", ((unsigned int)s[i]) & 0xff);
    if ((i%16) == 7)
      printf("%s", " ");
    if ((i%16) == 15)
    {
      putchar('|');
      for (size_t j =  0; j < 16; ++j)
      {
        char c = *(s + (i/16)*16 + j);
        if ((c >= 0x20) && (c <= 0x7e))
        {
          putchar(c);
        }
        else
          putchar('.');
      }
      putchar('|');
    }
  }
  printf("\n");
  return size;
}

constexpr unsigned short bswap_16(unsigned short x) {
  return (x>>8) | (x<<8);
}

constexpr unsigned int bswap_32(unsigned int x) {
  return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16));
}

constexpr unsigned long long bswap_64(unsigned long long x) {
  return (((unsigned long long)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32));
}

/** Different kind of fields one can find.
    - @a METADATA relative to file structure, but not to DATA itself. This might change from one version to another of
                  the file.
    - @a MUTABLE data which is supposed to change from one file to another (UUIDs, datestamps, etc).
    - @a DATA data which is NOT supposed to change from one version to another.
    - @a KEY  data which can be used to uniquely identify a field. 
  */
enum FieldType {
  METADATA,
  MUTABLE,
  DATA,
  KEY
};

enum ParseType {
  SCALAR,
  STRING,
  DATETIME,
  HEX,
  HEXDATA,
  STRUCT
};

// Trying to stay as close as possible to ROOT values.
enum GetterType {
  NONE = 0,
  CHAR,
  SHORT,
  INT = 3,
  INT64,
  FLOAT = 5,
  STRINGA,
  DOUBLE = 8
};

struct FieldSpec;

struct FieldInfo
{
  constexpr FieldInfo (short aSize,
                       bool aBigEndian,
                       bool aDelimited, 
                       unsigned short aPosition,
                       FieldSpec const*aRef)
  : ref(aRef),
    conditionalField(0),
    conditionalBeginRange(0),
    conditionalEndRange(0),
    conditionalType(NONE),
    size(aSize),
    bigEndian(aBigEndian),
    delimited(aDelimited),
    position(aPosition & 0x3fff)
  {}

  constexpr FieldInfo(const FieldInfo info,
                      char const *aConditionalField,
                      int64_t     aConditionalBeginRange,
                      int64_t     aConditionalEndRange,
                      GetterType  aConditionalType)
  : ref(info.ref),
    conditionalField(aConditionalField),
    conditionalBeginRange(aConditionalBeginRange),
    conditionalEndRange(aConditionalEndRange),
    conditionalType(aConditionalType),
    size(info.size),
    bigEndian(info.bigEndian),
    delimited(info.delimited),
    position(info.position & 0x3fff)
  {}

  constexpr FieldInfo()
  : FieldInfo(0, false, false, 0, nullptr)
  {}

  FieldSpec const*ref;
  char const     *conditionalField;
  int64_t        conditionalBeginRange;
  int64_t        conditionalEndRange;
  GetterType     conditionalType;
  unsigned  size : 16;
  unsigned  bigEndian : 1;
  unsigned  delimited : 1;
  signed    position : 14;
};

constexpr bool is_null(FieldInfo info)
{
  return info.position == 0 
         && info.delimited == 0
         && info.size == 0
         && info.bigEndian == 0
         && info.ref == 0
         && info.conditionalField == 0
         && info.conditionalBeginRange == 0
         && info.conditionalEndRange == 0; 
}

// To specify the size of a field.
constexpr FieldInfo fixed_size(short size)
{
  return FieldInfo(size, 0, 0, 0, 0);
}

// To specify the fact that the field gets its size from another one.
constexpr FieldInfo runtime_size(unsigned short position, short size, bool bigEndian)
{
  return FieldInfo(size, bigEndian, false, position, 0);
}

constexpr FieldInfo zero_delimited()
{
  return FieldInfo(0, false, true, 0, 0);
}

constexpr FieldInfo embedded(FieldSpec const*spec)
{
  return FieldInfo(0, false, false, 0, spec);
}

constexpr short size_offset(FieldInfo info)
{
  return info.position;
}

constexpr bool size_endian(FieldInfo info)
{
  return info.bigEndian;
}

constexpr short size_size(FieldInfo info)
{
  return info.size;
}

template <class T>
constexpr FieldInfo conditional_range(char const *key, T beginRange, T endRange, const FieldInfo info)
{
  return FieldInfo(info, key, beginRange, endRange, CHAR);
}

template <>
constexpr FieldInfo conditional_range(char const *key, char beginRange, char endRange, const FieldInfo info)
{
  return FieldInfo(info, key, beginRange, endRange, CHAR);
}

template <>
constexpr FieldInfo conditional_range(char const *key, short beginRange, short endRange, const FieldInfo info)
{
  return FieldInfo(info, key, beginRange, endRange, SHORT);
}

template <>
constexpr FieldInfo conditional_range(char const *key, int beginRange, int endRange, const FieldInfo info)
{
  return FieldInfo(info, key, beginRange, endRange, INT);
}

template <>
constexpr FieldInfo conditional_range(char const *key, int64_t beginRange, int64_t endRange, const FieldInfo info)
{
  return FieldInfo(info, key, beginRange, endRange, INT64);
}


template <class T>
constexpr FieldInfo conditional_eq(char const *key, T value, const FieldInfo info)
{
  return conditional_range(key, value, value, info); 
}

/** Use an array of such a structure to describe a binary blob on disk.
    
    - \a size       the size of the item.
    - \a name       a mnemonic name for the item.
    - \a bigEndian  whether the item is bigEndian or littleEndian.
    - \a ignore     whether or not to ignore the field when diffing
  */
struct FieldSpec {
    FieldInfo    info;
    char const* name;
    bool        bigEndian;
    FieldType   type;
    ParseType   parseType;
};



char const *doThrow(char const *fmt, char const *message)
{
  char buffer[1024];
  snprintf(buffer, 1024, fmt, message);
  throw buffer;
}

short doThrow(char const *fmt, char const *message, short)
{
  char buffer[1024];
  snprintf(buffer, 1024, fmt, message);
  throw buffer;
}


constexpr int getSizeBigEndian(FieldInfo info, char const *buf)
{
  return info.size == 2   ? bswap_16(*(short*)(buf + size_offset(info)))
       : info.size == 4   ? bswap_32(*(int*)(buf + size_offset(info)))
       : info.size == 8   ? bswap_64(*(int64_t*)(buf + size_offset(info)))
       :                        throw "wrong size";
}

constexpr unsigned int getSize(FieldInfo info, char const *buf)
{
  return info.delimited         ? strlen(buf) + 1
       : size_offset(info) == 0 ? info.position
       : info.size == 1         ? *(unsigned char*)(buf + size_offset(info))
       : info.bigEndian         ? getSizeBigEndian(info, buf)
       : info.size == 2         ? *(unsigned short*)(buf + size_offset(info))
       : info.size == 4         ? *(unsigned int*)(buf + size_offset(info))
       : info.size == 8         ? *(uint64_t*)(buf + size_offset(info))
       :                              throw "wrong size";
}

/** This will return the fixed part of the spec size */
constexpr int specSize(const FieldSpec *spec)
{
  return is_null(spec->info) == 0 ? 0
       : size_offset(spec->info) ? 0
       :                           spec->info.size + specSize(spec + 1);
}

/** Determine the actual size of a given spec including 
    variable length fields.
  */
constexpr int specRealSize(const FieldSpec *spec, char const*buf)
{
  return is_null(spec->info)     ? 0
       : spec->info.ref != 0     ? specRealSize(spec->info.ref, buf) 
                                   +  specRealSize(spec + 1, buf + specRealSize(spec->info.ref, buf)) 
       : spec->info.delimited    ? strlen(buf) + 1 + specRealSize(spec + 1, buf + strlen(buf)+1)
       : size_offset(spec->info) ? getSize(spec->info, buf) 
                                    + specRealSize(spec + 1, buf + getSize(spec->info, buf))
       :                           spec->info.size + specRealSize(spec + 1, buf 
                                    + spec->info.size);
}

/** Get the next object in buffer */
constexpr char const *next(const FieldSpec *spec, char const *buf)
{
  return buf + specRealSize(spec, buf);
}

/** Return the POD size or the struct size accordingly */
int moreRealSize(const FieldSpec *spec, char const *buf)
{
  return spec->info.ref         ? specRealSize(spec->info.ref, buf)
       :                          spec->info.size;
}

constexpr bool same(char const *x, char const *y) {
  return !*x && !*y     ? true                                                                                                                                
       : /* default */    (*x == *y && same(x+1, y+1));                                                                                                       
}

// Like "same" above but stops matching at the first .
// or at 0. @returns the pointer to the first character
// which made made it exit recursion.
constexpr char const*sub(char const *x, char const *y) {
  return !*x && !*y                  ? x
       : *x == '.' && !*y            ? x
       : *x == *y                    ? sub(x+1, y+1)
       :                               (char const *)-1;
}
extern constexpr short getShortOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label);
extern constexpr int getIntOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label);
extern constexpr int64_t getInt64Offset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label);

template <class T>
constexpr T getter(T v, bool /*endian*/)
{
  return v;
}

template <>
constexpr short getter(short v, bool endian)
{
  return endian ? bswap_16(v) : v;
}

template <>
constexpr int getter(int v, bool endian)
{
  return endian ? bswap_32(v) : v;
}

template <>
constexpr int64_t getter(int64_t v, bool endian)
{
  return endian ? bswap_64(v) : v;
}

constexpr char getCharOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label)
{
  return 
    spec[specOff].info.conditionalField
    && spec[specOff].info.conditionalType == CHAR
    && (getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
        || getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                    ? getCharOffset(spec, buf, bufOff, specOff + 1, label)

    : spec[specOff].info.conditionalField
    && spec[specOff].info.conditionalType == SHORT
    && (getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
        || getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                    ? getCharOffset(spec, buf, bufOff, specOff + 1, label)

    : spec[specOff].info.conditionalField
    && spec[specOff].info.conditionalType == INT
    && (getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
        || getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                    ? getCharOffset(spec, buf, bufOff, specOff + 1, label)

    : spec[specOff].info.conditionalField
    && spec[specOff].info.conditionalType == INT64
    && (getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
        || getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                    ? getCharOffset(spec, buf, bufOff, specOff + 1, label)

    : !spec[specOff].name                                        ? throw ParseError("Field not found", label) 
    : sub(label, spec[specOff].name) == (char const *) -1        ? getCharOffset(spec, buf, bufOff + moreRealSize(spec + specOff, buf+bufOff), specOff + 1, label)
    : *sub(label, spec[specOff].name) == '.'                     ? getCharOffset(spec[specOff].info.ref, buf, bufOff, 0, sub(label, spec[specOff].name) + 1)
    : (!*sub(label, spec[specOff].name) && spec[specOff].info.size == 1) ? getter(buf[bufOff], spec[specOff].bigEndian)
    : /* default */                                              throw "Unsupported spec: %s\n";
}

constexpr short doGetShort(FieldSpec const*spec, const char*buf)
{
  return spec->info.size == 2  ? (spec->bigEndian ? bswap_16(*(short*)buf) : *(short*)buf)
       :                         throw "Wrong size";
}

constexpr short getShortOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label)
{
  return
       spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == CHAR
       && (getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getShortOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == SHORT
       && (getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getShortOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT
       && (getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
          || getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getShortOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT64
       && (getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getShortOffset(spec, buf, bufOff, specOff + 1, label)
 
       : !spec[specOff].name                                        ? throw ParseError("Field not found", label)
       : sub(label, spec[specOff].name) == (char const *) -1        ? getShortOffset(spec, buf, bufOff + moreRealSize(spec+specOff, buf+bufOff), specOff + 1, label)
       : *sub(label, spec[specOff].name) == '.'                     ? getShortOffset(spec[specOff].info.ref, buf, bufOff, 0, sub(label, spec[specOff].name) + 1)
       : !*sub(label, spec[specOff].name)                           ? doGetShort(spec+specOff, buf + bufOff)
       : /* default */                                                doThrow("Unsupported spec: %s\n", label, (short)0);
}

constexpr int doGetInt(FieldSpec const*spec, const char*buf)
{
  return spec->info.size == 4  ? (spec->bigEndian ? bswap_32(*(int*)buf) : *(int*)buf)
       :                         throw "Wrong size";
}

constexpr int getIntOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label)
{
  return 
          spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == CHAR
       && (getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
                 || getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getIntOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == SHORT
       && (getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
          || getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getIntOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT
       && (getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
          || getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getIntOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT64
       && (getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
          || getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getIntOffset(spec, buf, bufOff, specOff + 1, label)

       : !spec[specOff].name                                        ? throw ParseError("Field not found", label)
       : sub(label, spec[specOff].name) == (char const *) -1        ? getIntOffset(spec, buf, bufOff + moreRealSize(spec+specOff, buf+bufOff), specOff+1, label) 
       : *sub(label, spec[specOff].name) == '.'                     ? getIntOffset(spec[specOff].info.ref, buf, bufOff, 0, sub(label, spec[specOff].name) + 1)
       : !*sub(label, spec[specOff].name)                           ? doGetInt(spec+specOff, buf + bufOff) 
       : /* default */                                        throw "Unsupported spec"; 
}

constexpr int64_t doGetInt64(FieldSpec const*spec, const char*buf)
{
  return spec->info.size == 8  ? (spec->bigEndian ? bswap_64(*(int64_t*)buf) : *(int64_t*)buf)
       :                                                      throw "Wrong size";
}

constexpr int64_t getInt64Offset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label)
{
  return 
       // This part takes care of conditional structures, i.e. structure
       // whose layout depends on the value of one of the members.
       spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == CHAR
       && (getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getInt64Offset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == SHORT
       && (getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getInt64Offset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT
       && (getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getInt64Offset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT64
       && (getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getInt64Offset(spec, buf, bufOff, specOff + 1, label)

       : !spec[specOff].name                                        ? throw ParseError("a", label) 
       : sub(label, spec[specOff].name) == (char const *) -1        ? getInt64Offset(spec, buf, bufOff + moreRealSize(spec+specOff, buf+bufOff), specOff + 1, label)
       : *sub(label, spec[specOff].name) == '.'                     ? getInt64Offset(spec[specOff].info.ref, buf, bufOff, 0, sub(label, spec[specOff].name) + 1)
       : !*sub(label, spec[specOff].name)                           ? doGetInt64(spec+specOff, buf + bufOff) 
       : /* default */                                        throw "Unsupported spec"; 
}

constexpr float doGetFloat(FieldSpec const*spec, const char*buf)
{
  return spec->info.size == 4  ? (spec->bigEndian ? bswap_32(*(float*)buf) : *(float*)buf)
       :                                                      throw "Wrong size";
}

constexpr float getFloatOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label)
{
  return 
       // This part takes care of conditional structures, i.e. structure
       // whose layout depends on the value of one of the members.
       spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == CHAR
       && (getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getFloatOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == SHORT
       && (getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getFloatOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT
       && (getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getFloatOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT64
       && (getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getFloatOffset(spec, buf, bufOff, specOff + 1, label)

       : !spec[specOff].name                                        ? throw ParseError("a", label) 
       : sub(label, spec[specOff].name) == (char const *) -1        ? getFloatOffset(spec, buf, bufOff + moreRealSize(spec+specOff, buf+bufOff), specOff + 1, label)
       : *sub(label, spec[specOff].name) == '.'                     ? getFloatOffset(spec[specOff].info.ref, buf, bufOff, 0, sub(label, spec[specOff].name) + 1)
       : !*sub(label, spec[specOff].name)                           ? doGetFloat(spec+specOff, buf + bufOff) 
       : /* default */                                        throw "Unsupported spec"; 
}

constexpr double doGetDouble(FieldSpec const*spec, const char*buf)
{
  return spec->info.size == 8  ? (spec->bigEndian ? bswap_64(*(double*)buf) : *(double*)buf)
       :                                                      throw "Wrong size";
}

constexpr double getDoubleOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label)
{
  return 
       // This part takes care of conditional structures, i.e. structure
       // whose layout depends on the value of one of the members.
       spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == CHAR
       && (getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getDoubleOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == SHORT
       && (getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getDoubleOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT
       && (getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getDoubleOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT64
       && (getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getDoubleOffset(spec, buf, bufOff, specOff + 1, label)

       : !spec[specOff].name                                        ? throw ParseError("a", label) 
       : sub(label, spec[specOff].name) == (char const *) -1        ? getDoubleOffset(spec, buf, bufOff + moreRealSize(spec+specOff, buf+bufOff), specOff + 1, label)
       : *sub(label, spec[specOff].name) == '.'                     ? getDoubleOffset(spec[specOff].info.ref, buf, bufOff, 0, sub(label, spec[specOff].name) + 1)
       : !*sub(label, spec[specOff].name)                           ? doGetDouble(spec+specOff, buf + bufOff) 
       : /* default */                                        throw "Unsupported spec"; 
}

constexpr char const*getStringOffset(const FieldSpec *spec, char const *buf, int bufOff, int specOff, char const*label)
{
  return
       // This part takes care of conditional structures, i.e. structure
       // whose layout depends on the value of one of the members.
       spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == CHAR
       && (getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getCharOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getStringOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == SHORT
       && (getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getShortOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getStringOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT
       && (getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getIntOffset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getStringOffset(spec, buf, bufOff, specOff + 1, label)    
       : spec[specOff].info.conditionalField
       && spec[specOff].info.conditionalType == INT64
       && (getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) < spec[specOff].info.conditionalBeginRange
           || getInt64Offset(spec, buf, 0, 0, spec[specOff].info.conditionalField) > spec[specOff].info.conditionalEndRange)
                                                                       ? getStringOffset(spec, buf, bufOff, specOff + 1, label)
 
       : !spec->name                                   ? doThrow("Field not found %s", label)
       : sub(label, spec[specOff].name) == (char const *) -1   ? getStringOffset(spec, buf, bufOff + moreRealSize(spec+specOff, buf+bufOff), specOff + 1, label)
       : (*sub(label, spec[specOff].name)) == '.'              ? getStringOffset(spec[specOff].info.ref, buf, bufOff, 0, sub(label, spec[specOff].name) + 1)
       :                                                 buf + bufOff;
}

/** This is the public API for getting a char.
    
    The heavy lifting is done by getCharOffset which can keep
    track of where we are in both navigating the spec and the 
    buffer.  
  */
constexpr char getChar(const FieldSpec *spec, char const *buf, char const*label)
{
  return getCharOffset(spec, buf, 0, 0, label);
}

constexpr short getShort(const FieldSpec *spec, char const *buf, char const*label)
{
  return getShortOffset(spec, buf, 0, 0, label);
}

constexpr int getInt(const FieldSpec *spec, char const *buf, char const*label)
{
  return getIntOffset(spec, buf, 0, 0, label);
}

constexpr int64_t getInt64(const FieldSpec *spec, char const *buf, char const*label)
{
  return getInt64Offset(spec, buf, 0, 0, label);
}

constexpr float getFloat(const FieldSpec *spec, char const *buf, char const*label)
{
  return getFloatOffset(spec, buf, 0, 0, label);
}

constexpr double getDouble(const FieldSpec *spec, char const *buf, char const*label)
{
  return getDoubleOffset(spec, buf, 0, 0, label);
}


constexpr char const*getString(const FieldSpec *spec, char const *buf, char const*label)
{
  return getStringOffset(spec, buf, 0, 0, label);
} 

void printScalar(const FieldSpec *specs, char const *buf)
{
  char buffer[16] = {0};
  switch (specs->info.size)
  {
    case 1:
      snprintf(buffer, 16, "%i", *(char*)buf);
      break;
    case 2:
      snprintf(buffer, 16, "%i", specs->bigEndian ? bswap_16(*(short*)buf) : *(short*)buf);
      break;
    case 4:
      snprintf(buffer, 16, "%i", specs->bigEndian ? bswap_32(*(int*)buf) : *(int*)buf);
      break;
    case 8:
      snprintf(buffer, 16, "%llu", specs->bigEndian ? bswap_64(*(long long*)buf) : *(long long*)buf);
      break;
    default:
      char tmp[16] = {0};
      memcpy(tmp, buf, specs->info.size < 16 ? specs->info.size : 16);
      snprintf(buffer, 16, "%s", tmp);
  }
  printf("\"%s\": %s", specs->name, buffer);
}



/** Notice there are two ways of defining strings: fixed lenght and 
    using the size coming from another location in the buffer, for 
    example the byte before.

    @return the number of bytes read.
*/
int printString(const FieldSpec *specs, char const *buf)
{
  int size = size_size(specs->info);
  if (size_offset(specs->info) || specs->info.delimited)
    size = getSize(specs->info, buf);
  if (!size)
    return 0;
  char buffer[size + 1];
  snprintf(buffer, size + 1, "%s", buf);
  printf("\"%s\": \"%s\"", specs->name, buffer);
  return size;
}

void printHex(const FieldSpec *specs, char const *buf)
{
  char buffer[specs->info.size*6+1];
  char *last = buffer;
  for (size_t i = 0; i < specs->info.size; ++i)
  {
    if (i != 0)
    {
      snprintf(last, 3, "%s", ", ");
      last += 2;
    }
    snprintf(last, 5, "0x%02x", ((int) buf[i]) & 0xff);
    last += 4;
  }
  printf("\"%s\": [%s]", specs->name, buffer);
}

void printDatetime(const FieldSpec *specs, char const* buf)
{
  int datetime = bswap_32(*(int*)buf);
  printf("\"%s\": %i/%i/%i %02i:%02i:%02i", specs->name, 
                              (datetime >> 26) + 1995,
                              abs((datetime << 6) >> 28),
                              abs((datetime << 10) >> 27),
                              abs((datetime << 15) >> 27),
                              abs((datetime << 20) >> 26),
                              abs(datetime << 26) >> 26);
}

char const *doPrintBuf(const FieldSpec *specs,  size_t specOff, char const* buf, size_t bufOff, int tabLevel)
{
  if (is_null(specs[specOff].info))
    return buf + bufOff;

  // Skip based on conditionals.
  if (specs[specOff].info.conditionalField
      && specs[specOff].info.conditionalType == CHAR
      && (getCharOffset(specs, buf, 0, 0, specs[specOff].info.conditionalField) < specs[specOff].info.conditionalBeginRange
          || getCharOffset(specs, buf, 0, 0, specs[specOff].info.conditionalField) > specs[specOff].info.conditionalEndRange))
                                                                     return doPrintBuf(specs,  specOff + 1, buf, bufOff, tabLevel);
  if (specs[specOff].info.conditionalField
      && specs[specOff].info.conditionalType == SHORT
      && (getShortOffset(specs, buf, 0, 0, specs[specOff].info.conditionalField) < specs[specOff].info.conditionalBeginRange
          || getShortOffset(specs, buf, 0, 0, specs[specOff].info.conditionalField) > specs[specOff].info.conditionalEndRange))
                                                                     return doPrintBuf(specs,  specOff + 1, buf, bufOff, tabLevel);
  if (specs[specOff].info.conditionalField
      && specs[specOff].info.conditionalType == INT
      && (getIntOffset(specs, buf, 0, 0, specs[specOff].info.conditionalField) < specs[specOff].info.conditionalBeginRange
          || getIntOffset(specs, buf, 0, 0, specs[specOff].info.conditionalField) > specs[specOff].info.conditionalEndRange))
                                                                     return doPrintBuf(specs,  specOff + 1, buf, bufOff, tabLevel);
  if (specs[specOff].info.conditionalField
      && specs[specOff].info.conditionalType == INT64
      && (getInt64Offset(specs, buf, 0, 0, specs[specOff].info.conditionalField) < specs[specOff].info.conditionalBeginRange
          || getInt64Offset(specs, buf, 0, 0, specs[specOff].info.conditionalField) > specs[specOff].info.conditionalEndRange))
                                                                     return doPrintBuf(specs,  specOff + 1, buf, bufOff, tabLevel);

  int sizeRead = specs[specOff].info.size;
  printf("%-*s", tabLevel, "");
  switch (specs[specOff].parseType)
  {
    case SCALAR:
    {
      printScalar(specs + specOff, buf + bufOff);
      break;
    }
    case STRING:
    {
      sizeRead = printString(specs + specOff, buf + bufOff);
      break;
    }
    case HEX:
    {
      printHex(specs + specOff, buf + bufOff);
      break;
    }
    case HEXDATA:
    {
      dump_hex(buf + bufOff, getSize(specs[specOff].info, buf + bufOff), bufOff);
      break;
    }      
    case DATETIME:
    {
      printDatetime(specs + specOff, buf + bufOff);
      break;
    }
    case STRUCT:
    {
      // In case of structures we use a different spec
      // and get the number of bytes read from the new pointer.
      printf("\"%s\": {\n", specs[specOff].name);
      char const *newPos = doPrintBuf(specs[specOff].info.ref, 0, buf + bufOff, 0, tabLevel + 2);
      printf("%*s", tabLevel+1, "}");
      sizeRead = newPos - buf - bufOff;
      break;
    }
  }
  if (!is_null(specs[specOff + 1].info) && sizeRead)
    printf(",");
  if (sizeRead)
    printf("\n");
  return doPrintBuf(specs, specOff + 1, buf, bufOff + sizeRead, tabLevel);
}

void printBuf(const FieldSpec *specs, char const* buf, int tabLevel = 0)
{
  printf("%*s", tabLevel+2, "{\n");
  doPrintBuf(specs, 0, buf, 0, tabLevel+2);
  printf("%*s", tabLevel+2, "}\n");
}

constexpr FieldSpec LAST_FIELD = {FieldInfo(), 0, false, METADATA, SCALAR};

// An object like structure.
// - @a klass is a specification for the object layout on file.
// - @a buffer is a pointer to such a layout.
struct Object {
  constexpr Object(FieldSpec const*aSpec, char const*aBuffer)
  : spec(aSpec),
    buffer(aBuffer)
  {}

  // Spec for next object in buffer.
  constexpr Object next(FieldSpec const*nextSpec)
  {
    return Object(nextSpec, ::next(spec, buffer));
  }

  constexpr char const*nextByte()
  {
    return ::next(spec, buffer);
  }

  void printBuf(int level = 0)
  {
    ::printBuf(spec, buffer, level);
  }

  constexpr char const* getString(char const *key)
  {
    return ::getString(spec, buffer, key);
  }

  constexpr int getInt(char const *key)
  {
    return ::getInt(spec, buffer, key);
  }

  constexpr int getShort(char const *key)
  {
    return ::getShort(spec, buffer, key);
  }


  FieldSpec const *spec;
  char const      *buffer;
};


#endif
