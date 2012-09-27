#include "BrutHeaders.h"

constexpr FieldSpec TStringSpec[] {
  {fixed_size(1), "size", true, METADATA, SCALAR},
  {runtime_size(-1, 1, false), "string", true, DATA, STRING},  // The size of this field depends on the previous field.
  LAST_FIELD
};

constexpr FieldSpec StringSpec[] {
  {zero_delimited(), "string", true, DATA, STRING},  // The field is zero delimited.
  LAST_FIELD
};

constexpr FieldSpec FStringSpec[] {
  {fixed_size(3), "string", true, DATA, STRING}, // The size of this field depends on the previous field.
  LAST_FIELD
};

constexpr char tstring[] = {3, 'f', 'o', 'o'};
constexpr char zstring[] = {'f', 'o', 'o', 0};
constexpr char fstring[] = {'f', 'o', 'o'};

int
main (int argc, char **argv)
{
  assert(specRealSize(TStringSpec, tstring) == 4);
  assert(specRealSize(FStringSpec, fstring) == 3);
  assert(specRealSize(StringSpec, zstring) == 4);
  assert(strncmp(getString(TStringSpec, tstring, "string"), "foo", 3)== 0); 
  assert(strncmp(getString(FStringSpec, fstring, "string"), "foo", 3) == 0); 
  assert(strcmp(getString(StringSpec, zstring, "string"), "foo") == 0); 
}
