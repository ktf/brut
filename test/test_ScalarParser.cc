#include "BrutHeaders.h"
#include <iostream>

constexpr FieldSpec SomeSpec[] = {
  {fixed_size(1), "aChar", false, METADATA, SCALAR},
  {fixed_size(2), "aShort", false, METADATA, SCALAR},
  {fixed_size(4), "aInt", false, METADATA, SCALAR},
  {fixed_size(8), "aInt64", false, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec SomeSpecBigEndian[] {
  {fixed_size(1), "aChar", true, METADATA, SCALAR},
  {fixed_size(2), "aShort", true, METADATA, SCALAR},
  {fixed_size(4), "aInt", true, METADATA, SCALAR},
  {fixed_size(8), "aInt64", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec AString[] {
  {zero_delimited(), "aString", true, METADATA, STRING},
  LAST_FIELD
};

constexpr FieldSpec SmallStruct[] = {
  {fixed_size(1), "aChar", false, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec SmallStruct2[] = {
  {fixed_size(2), "aShort", false, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec SmallComposedStruct[] = {
  {embedded(SmallStruct), "A", false, METADATA, SCALAR},
  {embedded(SmallStruct), "B", false, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec SmallComposedStruct2[] = {
  {embedded(SmallStruct2), "A", false, METADATA, SCALAR},
  {embedded(SmallStruct2), "B", false, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec AStruct[] = {
  {embedded(SomeSpec), "LittleStruct", true, METADATA, STRUCT},
  {embedded(SomeSpecBigEndian), "BigStruct", true, METADATA, STRUCT},
  {embedded(AString), "AString", false, METADATA, STRUCT},
  LAST_FIELD
};

constexpr FieldSpec BStruct[] = {
  {embedded(AStruct), "aStruct", true, METADATA, STRUCT},
  {embedded(SmallStruct), "A", false, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct[] = {
  {fixed_size(1), "dummy", true, METADATA, SCALAR},
  {fixed_size(1), "version", true, METADATA, SCALAR},
  {conditional_eq("version", (char)2, fixed_size(1)), "a2", true, METADATA, SCALAR},
  {conditional_eq("version", (char)1, fixed_size(1)), "a1", true, METADATA, SCALAR},
  {fixed_size(1), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct2[] = {
  {fixed_size(2), "dummy", false, METADATA, SCALAR},
  {fixed_size(2), "version", false, METADATA, SCALAR},
  {conditional_eq("version", (char)2, fixed_size(2)), "a2", false, METADATA, SCALAR},
  {conditional_eq("version", (char)1, fixed_size(2)), "a1", false, METADATA, SCALAR},
  {fixed_size(2), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct4[] = {
  {fixed_size(1), "dummy", false, METADATA, SCALAR},
  {fixed_size(2), "version", true, METADATA, SCALAR},
  {conditional_eq("version", (short)2, fixed_size(1)), "a2", false, METADATA, SCALAR},
  {conditional_eq("version", (short)1, fixed_size(1)), "a1", false, METADATA, SCALAR},
  {fixed_size(1), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct5[] = {
  {fixed_size(1), "dummy", false, METADATA, SCALAR},
  {fixed_size(4), "version", true, METADATA, SCALAR},
  {conditional_eq("version", (int)2, fixed_size(1)), "a2", false, METADATA, SCALAR},
  {conditional_eq("version", (int)1, fixed_size(1)), "a1", false, METADATA, SCALAR},
  {fixed_size(1), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct6[] = {
  {fixed_size(1), "dummy", false, METADATA, SCALAR},
  {fixed_size(8), "version", true, METADATA, SCALAR},
  {conditional_eq("version", (int64_t)2, fixed_size(1)), "a2", false, METADATA, SCALAR},
  {conditional_eq("version", (int64_t)1, fixed_size(1)), "a1", false, METADATA, SCALAR},
  {fixed_size(1), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct7[] = {
  {fixed_size(1), "dummy", true, METADATA, SCALAR},
  {fixed_size(2), "version", true, METADATA, SCALAR},
  {conditional_eq("version", (short)2, fixed_size(1)), "a2", true, METADATA, SCALAR},
  {conditional_eq("version", (short)1, fixed_size(2)), "a1", true, METADATA, SCALAR},
  {fixed_size(2), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct8[] = {
  {fixed_size(1), "dummy", true, METADATA, SCALAR},
  {fixed_size(4), "version", true, METADATA, SCALAR},
  {conditional_eq("version", (int)2, fixed_size(1)), "a2", true, METADATA, SCALAR},
  {conditional_eq("version", (int)1, fixed_size(8)), "a1", true, METADATA, SCALAR},
  {fixed_size(8), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct9[] = {
  {fixed_size(1), "dummy", true, METADATA, SCALAR},
  {fixed_size(4), "version", true, METADATA, SCALAR},
  {conditional_range("version", (int) 6, (int) 10, fixed_size(1)), "a2", true, METADATA, SCALAR},
  {conditional_range("version", (int) 0, (int) 5, fixed_size(8)), "a1", true, METADATA, SCALAR},
  {fixed_size(8), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct10[] = {
  {fixed_size(1), "dummy", true, METADATA, SCALAR},
  {embedded(SomeSpec), "version", true, METADATA, SCALAR},
  {conditional_range("version.aInt", (int) 6, (int) 10, fixed_size(1)), "a2", true, METADATA, SCALAR},
  {conditional_range("version.aInt", (int) 0, (int) 5, fixed_size(8)), "a1", true, METADATA, SCALAR},
  {fixed_size(8), "c", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec CStruct11[] = {
  {fixed_size(4), "dummy", false, METADATA, SCALAR},
  LAST_FIELD
};

int
main (int argc, char **argv)
{

  char buffer[] = {1,
                  2, 0,
                  3, 0, 0, 0,
                  4, 0, 0, 0, 0, 0, 0, 0};
  char bigEbuf[] = {5,
                    0, 6,
                    0, 0, 0, 7,
                    0, 0, 0, 0, 0, 0, 0, 8};

  char someStruct[] = {1,
                       2, 0,
                       3, 0, 0, 0,
                       4, 0, 0, 0, 0, 0, 0, 0,
                       5,
                       0, 6,
                       0, 0, 0, 7,
                       0, 0, 0, 0, 0, 0, 0, 8,
                       'f', 'o', 'o', 0,
                      127};

  char shortBuffer[] = {1, 0,
                        2, 0};

  char shortBuffer2[] = {0,
                         1,
                         2,
                         3};

  char shortBuffer3[] = {0,
                         2,
                         2,
                         3};

  char shortBuffer4[] = {0,
                          0, 1,
                          2,
                          3};

  char shortBuffer5[] = {0,
                         0, 0, 0, 1,
                         2,
                         3};

  char shortBuffer6[] = {0,
                         0, 0, 0, 0, 0, 0, 0, 1,
                         2,
                         3};

  char shortBuffer7[] = {0,
                         0, 1,
                         0, 2,
                         0, 3};

  char shortBuffer8[] = {0,
                         0, 0, 0, 1,
                         0, 0, 0, 0, 0, 0, 0, 2,
                         0, 0, 0, 0, 0, 0, 0, 3};

  char shortBuffer9[] = {0,
                         0, 0, 0, 5,
                         0, 0, 0, 0, 0, 0, 0, 2,
                         0, 0, 0, 0, 0, 0, 0, 3};

  char shortBuffer10[] = {0,
                          1,
                          1, 0,
                          1, 0, 0, 0,
                          1, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 2,
                          0, 0, 0, 0, 0, 0, 0, 3};

  float testBuffer11[] = {3.14f};


  try{    
  assert(*sub("LittleStruct.aChar", "LittleStruct") == '.');
  assert(getInt(AStruct, someStruct, "LittleStruct.aInt") == 3);
  assert(getInt64(AStruct, someStruct, "LittleStruct.aInt64") == 4);
  assert(getChar(AStruct, someStruct, "LittleStruct.aChar") == 1);
  assert(getShort(AStruct, someStruct, "LittleStruct.aShort") == 2);
  assert(getChar(SmallComposedStruct, someStruct, "A.aChar") == 1);
  assert(getChar(SmallComposedStruct, someStruct, "B.aChar") == 2);  

  assert(getShort(SmallComposedStruct2, shortBuffer, "A.aShort") == 1);
  assert(getShort(SmallComposedStruct2, shortBuffer, "B.aShort") == 2);  

  assert(getShort(AStruct, someStruct, "BigStruct.aShort") == 6);
  assert(getInt(AStruct, someStruct, "BigStruct.aInt") == 7);
  assert(getInt64(AStruct, someStruct, "BigStruct.aInt64") == 8);
  assert(getChar(AStruct, someStruct, "BigStruct.aChar") == 5);


  assert(*sub("foo.bar", "foo") == '.');
  assert(sub("foos.bar", "foo") ==  (char const *)-1);
  assert(*sub("foo", "foo") == 0);

  assert(getChar(SomeSpec, buffer, "aChar") == 1);
  assert(getShort(SomeSpec, buffer, "aShort") == 2);
  assert(getInt(SomeSpec, buffer, "aInt") == 3);
  assert(getInt64(SomeSpec, buffer, "aInt64") == 4);

  assert(getChar(SomeSpecBigEndian, bigEbuf, "aChar") == 5);
  assert(getShort(SomeSpecBigEndian, bigEbuf, "aShort") == 6);
  assert(getInt(SomeSpecBigEndian, bigEbuf, "aInt") == 7);
  assert(getInt64(SomeSpecBigEndian, bigEbuf, "aInt64") == 8);

  assert(specRealSize(AStruct, someStruct) == sizeof(someStruct)-1);
  assert(strcmp("foo", getString(AStruct, someStruct, "AString.aString")) == 0);
  
  assert(strcmp("foo", getString(BStruct, someStruct, "aStruct.AString.aString"))== 0);
  assert(getChar(BStruct, someStruct, "A.aChar") == 127);

  assert(getChar(CStruct, shortBuffer2, "dummy") == 0);
  assert(getChar(CStruct, shortBuffer2, "version") == 1);
  assert(getChar(CStruct, shortBuffer2, "a1") == 2);
  assert(getChar(CStruct, shortBuffer2, "c") == 3);
  try
  {
    getChar(CStruct, shortBuffer2, "a2");
    assert(false);
  }catch(...)
  {}

  assert(getChar(CStruct, shortBuffer3, "dummy") == 0);
  assert(getChar(CStruct, shortBuffer3, "version") == 2);
  assert(getChar(CStruct, shortBuffer3, "a2") == 2);
  assert(getChar(CStruct, shortBuffer3, "c") == 3);
  try
  {
    getChar(CStruct, shortBuffer3, "a1");
    assert(false);
  }catch(...)
  {}

  assert(getChar(CStruct4, shortBuffer4, "dummy") == 0);
  assert(getShort(CStruct4, shortBuffer4, "version") == 1);
  assert(getChar(CStruct4, shortBuffer4, "c") == 3);
  assert(getChar(CStruct4, shortBuffer4, "a1") == 2);
  try
  {
    getShort(CStruct, shortBuffer4, "a2");
    assert(false);
  }catch(...)
  {}

  assert(getChar(CStruct5, shortBuffer5, "dummy") == 0);
  assert(getInt(CStruct5, shortBuffer5, "version") == 1);
  assert(getChar(CStruct5, shortBuffer5, "c") == 3);
  assert(getChar(CStruct5, shortBuffer5, "a1") == 2);
  try
  {
    getShort(CStruct5, shortBuffer5, "a2");
    assert(false);
  }catch(...)
  {}

  assert(getChar(CStruct6, shortBuffer6, "dummy") == 0);
  assert(getInt64(CStruct6, shortBuffer6, "version") == 1);
  assert(getChar(CStruct6, shortBuffer6, "c") == 3);
  assert(getChar(CStruct6, shortBuffer6, "a1") == 2);
  try
  {
    getShort(CStruct6, shortBuffer6, "a2");
    assert(false);
  }catch(...)
  {}

  assert(getChar(CStruct7, shortBuffer7, "dummy") == 0);
  assert(getShort(CStruct7, shortBuffer7, "version") == 1);
  assert(getShort(CStruct7, shortBuffer7, "c") == 3);
  assert(getShort(CStruct7, shortBuffer7, "a1") == 2);
  try
  {
    getShort(CStruct7, shortBuffer7, "a2");
    assert(false);
  }catch(...)
  {}

  assert(getChar(CStruct8, shortBuffer8, "dummy") == 0);
  assert(getInt(CStruct8, shortBuffer8, "version") == 1);
  assert(getInt64(CStruct8, shortBuffer8, "c") == 3);
  assert(getInt64(CStruct8, shortBuffer8, "a1") == 2);
  try
  {
    getChar(CStruct8, shortBuffer8, "a2");
    assert(false);
  }catch(...)
  {}

  assert(getChar(CStruct9, shortBuffer9, "dummy") == 0);
//  assert(getInt(CStruct9, shortBuffer9, "version") == 1);
  assert(getInt64(CStruct9, shortBuffer9, "c") == 3);
  assert(getInt64(CStruct9, shortBuffer9, "a1") == 2);
  try
  {
    getChar(CStruct9, shortBuffer9, "a2");
    assert(false);
  }catch(...)
  {}


  assert(getChar(CStruct10, shortBuffer10, "dummy") == 0);
  assert(getChar(CStruct10, shortBuffer10, "version.aChar") == 1);
  assert(getShort(CStruct10, shortBuffer10, "version.aShort") == 1);
  assert(getInt64(CStruct10, shortBuffer10, "c") == 3);
  assert(getInt64(CStruct10, shortBuffer10, "a1") == 2);
  try
  {
    getChar(CStruct9, shortBuffer9, "a2");
    assert(false);
  }catch(...)
  {}

  assert(getFloat(CStruct11, (char *)testBuffer11, "dummy") == 3.14f);

}
catch(char const *str)
{
  printf("%s\n", str);
}
}
