#include "globals.h"

/*** Can't just use built in random() or rand() because different systems may
     use slightly different algorithms and hence produce difference sequences
     given the same seed. Not much use when decoding!

     See: https://en.wikipedia.org/wiki/Xorshift
***/
void setRandomValue()
{
	lfsr_val ^= (lfsr_val >> 13);
	lfsr_val ^= (lfsr_val >> 7);
	lfsr_val ^= (lfsr_val << 17);
}




/*** Get XOR bits from the current random 64 bit value ***/
u_char getRandomBits()
{
	u_char byte;

	byte = (u_char)(lfsr_val >> lfsr_shift) & encode_mask;
	lfsr_shift += bits_per_byte;
	if (lfsr_shift == 64)
	{
		lfsr_shift = 0;
		setRandomValue();
	}
	return byte;
}




/*** Get XOR bits from the current key byte ***/
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
