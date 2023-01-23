
#include <assert.h>

#include "../../src/verifier/verifier_private.h"

bool bytes_from_hex_string(const char *str, uint8_t target[], size_t target_size);

int main(void)
{
  unsigned char test[4];

  assert(bytes_from_hex_string("DEADBEEF", test, 4));

  assert(test[0] == 0xde);
  assert(test[1] == 0xad);
  assert(test[2] == 0xbe);
  assert(test[3] == 0xef);

  return 0;
}
