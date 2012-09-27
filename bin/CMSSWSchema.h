#ifndef CMSSW_SCHEMA_H
#define CMSSW_SCHEMA_H
#include "BrutHeaders.h"

constexpr FieldSpec FileFormatVersionSpec[] {
  {fixed_size(4), "value", true, METADATA, SCALAR},
  {fixed_size(4), "unknown1", true, METADATA, SCALAR},
  {fixed_size(4), "unknown2", true, METADATA, SCALAR},
  {fixed_size(4), "unknown3", true, METADATA, SCALAR},
  LAST_FIELD
};

#endif