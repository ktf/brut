#ifndef ROOT_SCHEMA_H
#define ROOT_SCHEMA_H
#include "BrutHeaders.h"

constexpr FieldSpec TStringSpec[] {
  {fixed_size(1), "size", true, METADATA, SCALAR},
  {runtime_size(-1, 1, false), "value", true, DATA,STRING },  // The size of this field depends on the previous field.
  LAST_FIELD
};

constexpr FieldSpec TVersionSpec[] {
  {fixed_size(4), "size", true, METADATA, HEX},
  {fixed_size(2), "value", true, DATA, SCALAR},  // The size of this field depends on the previous field.
  LAST_FIELD
};

constexpr FieldSpec fileHeaderSpec[] {
  {fixed_size(4), "magic", true, METADATA, STRING}, 
  {fixed_size(4), "fVersion", true, METADATA, SCALAR},
  {fixed_size(4), "fBEGIN", true, METADATA, SCALAR},
  {fixed_size(4), "fEND", true, METADATA, SCALAR},
  {fixed_size(4), "fSeekFree", true, METADATA, SCALAR},
  {fixed_size(4), "fNbytesFree", true,METADATA, SCALAR},
  {fixed_size(4), "nfree", true, METADATA, SCALAR},
  {fixed_size(4), "fNbytesName", true, METADATA, SCALAR},
  {fixed_size(1), "fUnits", true, METADATA, SCALAR},
  {fixed_size(4), "fCompress", true, METADATA, SCALAR},
  {fixed_size(4), "fSeekInfo", true, METADATA, SCALAR},
  {fixed_size(4), "fNbytesInfo", true, METADATA, SCALAR},
  {fixed_size(18), "fUUID", true, MUTABLE, HEX},
  LAST_FIELD
};

constexpr FieldSpec keyHeaderSpec[] = {
  {fixed_size(4), "Nbytes", true, METADATA, SCALAR},
  {fixed_size(2), "Version", true, METADATA, SCALAR},
  {fixed_size(4), "ObjLen", true, METADATA, SCALAR},
  {fixed_size(4), "Datetime", true, MUTABLE,DATETIME},
  {fixed_size(2), "KeyLen", true, METADATA, SCALAR},
  {fixed_size(2), "Cycle", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 0, (short) 1000, fixed_size(4)), "SeekKey", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 0, (short) 1000, fixed_size(4)), "SeekPdir", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 1001, (short)(SHRT_MAX), fixed_size(8)), "SeekKey", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 1001, (short)(SHRT_MAX), fixed_size(8)), "SeekPdir", true, METADATA, SCALAR},
  {embedded(TStringSpec), "ClassName", true, METADATA, STRUCT},
  {embedded(TStringSpec), "Name", true, METADATA, STRUCT},
  {embedded(TStringSpec), "Title", true, METADATA, STRUCT},
  LAST_FIELD
};

constexpr FieldSpec subDirSpec[] {
  {fixed_size(1), "fModifiable", true, MUTABLE, SCALAR},
  {fixed_size(1), "fWritable", true, MUTABLE, SCALAR},
  {fixed_size(4), "fDatetimeC", true, MUTABLE, DATETIME},
  {fixed_size(4), "fDatetimeM", true, MUTABLE, DATETIME},
  {fixed_size(4), "fSeekDir", true, METADATA, SCALAR},
  {fixed_size(4), "fNbytesKeys", true, METADATA, SCALAR},
  {fixed_size(4), "fSeekParent", true, METADATA, SCALAR},
  {fixed_size(4), "fSeekKeys", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec topDirSpec[] = {
  {fixed_size(2), "Version", true, METADATA, SCALAR},
  {fixed_size(4), "fDatetimeC", true, MUTABLE, DATETIME},
  {fixed_size(4), "fDatetimeM", true, MUTABLE, DATETIME},
  {fixed_size(4), "fNbytesKeys", true, METADATA, SCALAR},
  {fixed_size(4), "fNbytesName", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 0, (short) 1000, fixed_size(4)), "fSeekDir", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 0, (short) 1000, fixed_size(4)), "fSeekParent", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 0, (short) 1000, fixed_size(4)), "fSeekKeys", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 1001, (short) (SHRT_MAX), fixed_size(8)), "fSeekDir", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 1001, (short) (SHRT_MAX), fixed_size(8)), "fSeekParent", true, METADATA, SCALAR},
  {conditional_range("Version", (short) 1001, (short) (SHRT_MAX), fixed_size(8)), "fSeekKeys", true, METADATA, SCALAR},
  {fixed_size(18), "fUUID", true, METADATA, HEX},
  {conditional_range("Version", (short) 0, (short) 1000, fixed_size(12)), "EXTRA", true, METADATA, HEX},  
  LAST_FIELD
};

constexpr FieldSpec TNamedSpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},  
  {fixed_size(2), "AnotherVersion", true, METADATA, SCALAR},   
  {fixed_size(4), "UniqueID", true, METADATA, HEX},   
  {fixed_size(4), "fBits", true, METADATA, HEX},
  {embedded(TStringSpec), "Name", true, METADATA, STRUCT},
  {embedded(TStringSpec), "Title", true, METADATA, STRUCT},
  LAST_FIELD
};

constexpr FieldSpec TClassSpec[] = {
  {fixed_size(4), "ByteCount", true, METADATA, HEX},
  {fixed_size(4), "Tag", true, METADATA, HEX},
  {zero_delimited(), "Name", true, METADATA, STRING},
  LAST_FIELD
};

constexpr FieldSpec TObjectSpec[] {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},  
  {fixed_size(4), "fUniqueID", true, METADATA, HEX},  
  {fixed_size(4), "fBits", true, METADATA, HEX},
  {fixed_size(2), "fPID", true, METADATA, HEX},  
  LAST_FIELD
};

constexpr FieldSpec TListSpec[] {
  {embedded(TObjectSpec), "Object", true, METADATA, STRUCT},
  {embedded(TStringSpec), "Name", true, METADATA, STRUCT},
  {fixed_size(4), "nObjects", true, METADATA,SCALAR},
  LAST_FIELD
};

constexpr FieldSpec StreamerInfoSpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},
  {fixed_size(4), "ObjId", true, METADATA, HEX},
  {fixed_size(4), "fBits", true, METADATA, HEX},
  {fixed_size(2), "UNKNOWN", true, METADATA, HEX}, // Assign to ObjId???
  {embedded(TStringSpec), "SomeString", true, METADATA, STRUCT},
  {fixed_size(4), "nObjects", true, METADATA, SCALAR},   
  {embedded(TClassSpec), "Class", true, METADATA, STRUCT},
  LAST_FIELD
};

constexpr FieldSpec TObjArraySpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},  
  {fixed_size(2), "AnotherVersion", true, METADATA, SCALAR},   
  {fixed_size(4), "UniqueID", true, METADATA, HEX},   
  {fixed_size(4), "fBits", true, METADATA, HEX},
  {embedded(TStringSpec), "Name", true, METADATA, STRUCT},
  {fixed_size(4), "nObjects", true, METADATA, SCALAR},
  {fixed_size(4), "lowerBound", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec TStreamerInfoSpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},  
  {embedded(TNamedSpec), "TNamed", true, METADATA, STRUCT},  
  {fixed_size(4), "Checksum", true, METADATA, HEX},
  {fixed_size(4), "ClassVersion", true, METADATA, SCALAR},
  {embedded(TClassSpec), "ClassName", true, METADATA, STRUCT},  
  {embedded(TObjArraySpec), "ObjectArray", true, METADATA, STRUCT},  
  LAST_FIELD
};

constexpr FieldSpec TStreamerElementSpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},
  {embedded(TNamedSpec), "TNamed", true, METADATA, STRUCT},
  {fixed_size(4), "type", true, METADATA, SCALAR},
  {fixed_size(4), "size", true, METADATA, SCALAR},
  {fixed_size(4), "lenght", true, METADATA, SCALAR},
  {fixed_size(4), "dim", true, METADATA, SCALAR},
  {conditional_range("Version.value", (short) 2, (short)(SHRT_MAX), fixed_size(20)), "someArray", true, METADATA, HEX},
  {embedded(TStringSpec), "typename", true, METADATA, STRUCT},
  // {conditional_eq("Version.value", (short) 3, fixed_size(4)), "xmin", true, METADATA, SCALAR},
  // {conditional_eq("Version.value", (short) 3, fixed_size(4)), "xmax", true, METADATA, SCALAR},
  // {conditional_eq("Version.value", (short) 3, fixed_size(4)), "scale", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec TStreamerBaseSpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},  
  {embedded(TStreamerElementSpec), "StreamerElement", true, METADATA, STRUCT},
  {conditional_range("Version.value", (short) 3, (short)(SHRT_MAX), fixed_size(4)), "baseversion", true, METADATA, SCALAR},
  LAST_FIELD
};

constexpr FieldSpec TStreamerStringSpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},  
  {embedded(TStreamerElementSpec), "StreamerElement", true, METADATA, STRUCT},

 // {embedded(TStringSpec), "String", true, METADATA, STRUCT},
//  {embedded(TStringSpec), "Name", true, METADATA, STRUCT},
//  {eedded(TStringSpec), "Title", true, METADATA, STRUCT},
  LAST_FIELD
};

constexpr FieldSpec TStreamerBasicTypeSpec[] = {
  {embedded(TVersionSpec), "Version", true, METADATA, STRUCT},  
  {embedded(TStreamerElementSpec), "StreamerElement", true, METADATA, STRUCT},
  {fixed_size(4), "counter", true, METADATA, SCALAR},

 // {embedded(TStringSpec), "String", true, METADATA, STRUCT},
//  {embedded(TStringSpec), "Name", true, METADATA, STRUCT},
//  {eedded(TStringSpec), "Title", true, METADATA, STRUCT},
  LAST_FIELD
};


constexpr FieldSpec ArrayISpec[] {
  {fixed_size(4), "size", true, METADATA, SCALAR},
  {runtime_size(-4, 4, true), "data", true, DATA, HEX},        // The size of this field depends on the previous field.
  LAST_FIELD
};

#endif