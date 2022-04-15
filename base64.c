#include "globals.h"

#define B64_CHARSET_LEN 64

/* Non continuous. This is more efficient than using strchr() on the array */
#define IN_B64_CHARSET(C) \
	(C == '+' || C == '/' || \
	 (C >= 'A' && C <= 'Z') || \
	 (C >= 'a' && C <= 'z') || \
	 (C >= '0' && C <= '9'))

const char *b64_charset =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char getB64char(u_char mask, u_char data);

/*** This encoding is much simpler than uuencode because no header, footer
     line length or info byte is required and any non valid characters can
     simply be skipped ***/
void encodeB64()
{
	long bytes_read;
	u_char byte;
	u_char data;
	int line_bytes;
	int c;
	int i;

	line_bytes = 0;
	/* Base64 doesn't seem to make a fuss about non 4 byte chunks so we 
	   won't bother with pad bytes */
	pad_bytes = 0;
	bytes_read = 0;

	/* Go through input file */
	while((c = readByte()) != EOF)
	{
		byte = (u_char)c;
		++bytes_read;

		/* Break up byte as per uuencode */
		for(i=0;i < byte_parts;++i)
		{
			/* Get bits we need and shift byte down for next time */
			data = byte & encode_mask;
			if (key) data ^= getKeyBits();
			if (lfsr_seed) data ^= getRandomBits();

			if (flags.output_data)
				writeByte(getB64char(charset_mask,data));

			byte >>= bits_per_byte;

			/* Write next line length char if we've written the
			   maximum line length */
			if (++line_bytes == base64_line_len)
			{
				++encode_lines;
				if (flags.output_data) writeByte('\n');
				line_bytes = 0;
			}
		}
	}
	if (line_bytes || !base64_line_len)
	{
		++encode_lines;
		if (flags.output_data) writeByte('\n');
	}
}




/*** According to the spec base64 ignores any invalid chars so we just go
     through the entire file and decode every one that is valid ***/
void decodeB64()
{
	u_char byte;
	u_char out_byte;
	int part;
	int c;

	if (flags.debug) fprintf(stderr,"base64\n");

	out_byte = 0;
	part = 0;

	while((c = readByte()) != EOF)
	{
		byte = (u_char)c;

		/* Ignore non compliant chars */
		if (!IN_B64_CHARSET(byte))
		{
			if (byte == '\n' && flags.b64_prev_valid)
				++encode_lines;
			flags.b64_prev_valid = 0;
			continue;
		}

		flags.b64_prev_valid = 1;

		byte &= encode_mask;
		if (key) byte ^= getKeyBits();
		if (lfsr_seed) byte ^= getRandomBits();
		out_byte |= (byte << (bits_per_byte * part));

		if (++part == byte_parts)
		{
			if (flags.output_data) writeByte(out_byte);
			out_byte = 0;
			part = 0;
		}
	}
	if (flags.b64_prev_valid) ++encode_lines;
}




/*** Get a valid Base64 encoding character with our data embedded ***/
char getB64char(u_char mask, u_char data)
{
	u_char b64_char;
	int pos;
	int i;

	pos = random() % B64_CHARSET_LEN;

	for(i=0;i < B64_CHARSET_LEN;++i)
	{
		b64_char = ((u_char)b64_charset[pos] & mask) | data;
		if (IN_B64_CHARSET(b64_char)) return (char)b64_char;
		pos = (pos + 1) % B64_CHARSET_LEN;
	}
	assert(0);
	return 0;
}
