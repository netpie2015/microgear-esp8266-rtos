#include "base64.h"

const static char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* base64Encode(char *buffer, const uint8_t *data, size_t data_len) {
  int i, len;

  for (i = 0, len = data_len; len > 0; i += 3, len -= 3)
    {
      unsigned long val;

      val = (unsigned long) data[i] << 16;
      if (len > 1)
        val |= (unsigned long) data[i + 1] << 8;
      if (len > 2)
        val |= data[i + 2];

      *buffer++ = (char) *(base64_table + (val >> 18));
      *buffer++ = (char) *(base64_table + ((val >> 12) & 0x3f));
      *buffer++ = (char) *(base64_table + ((val >> 6) & 0x3f));
      *buffer++ = (char) *(base64_table + (val & 0x3f));
    }

  for (; len < 0; len++)
    buffer[len] = '=';

  *buffer = '\0';
  return buffer;
}
