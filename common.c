#include "globals.h"

/*** Bits count is the same as bits_per_bytes ***/
u_char getKeyBits()
{
	u_char byte;

	byte = (key[key_pos] >> key_shift) & encode_mask;
	key_shift += bits_per_byte;
	if (key_shift == 8)
	{
		key_pos = (key_pos + 1) % key_len;
		key_shift = 0;
	}
	return byte;
}




void writeByte(u_char byte)
{
	if (fputc(byte,out_fp) == EOF)
	{
		perror("ERROR: fputs()");
		exit(1);
	}
}
