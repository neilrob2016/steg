#include "globals.h"

#define GRAVE '`'

#define UU_DECODED_LINE_LEN 45
#define UU_ENCODED_LINE_LEN 60
#define UU_MIN              33  /* Ascii code of position 1. GRAVE is 0 */
#define UU_MAX              95
#define UU_CHARSET_LEN      (UU_MAX - UU_MIN) + 2

/* Continuous in ASCII except for grave */
#define IN_UU_CHARSET(C) (C == GRAVE || (C >= UU_MIN && C <= UU_MAX))

const char *uu_charset =
        "`!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_";

char getUUChar(u_char mask, u_char data);

/*** uuencode is more complicated than base64 due to header, footer and line
     start length character ***/
void encodeUU()
{
	long bytes_read;
	long cnt;
	u_char byte;
	u_char data;
	int line_bytes;
	int part;
	int c;
	int i;

	line_bytes = 0;
	bytes_read = 0;

	/* Uuencode likes data in 4 byte chunks so we need to oblidge */
	last_line_cnt = (fs.st_size * byte_parts) % UU_DECODED_LINE_LEN;
	pad_bytes = 4 - (last_line_cnt % 4);
	if (pad_bytes == 4) pad_bytes = 0;

	if (flags.debug)
		fprintf(stderr,"  * Pad length     : %d bytes\n",pad_bytes);

	if (flags.output_data)
	{
		fprintf(out_fp,"begin 644 %s\n",
			uu_hdr_file ? uu_hdr_file : input_file);

		/* We write the uuencode decode line length char at the start of
		   each line. We don't use this but don't want uudecode to 
		   complain if someone tries to decode our output with it as 
		   that would look suspicious. / 4 * 3 because uuencode encodes
		   3 bytes input bytes in every 4 output. */
		cnt = last_line_cnt / 4 * 3 + pad_bytes;
		if (cnt < UU_DECODED_LINE_LEN)
			writeByte((char)(32 + cnt));
		else
			writeByte('M');
	}

	while((c = readByte()) != EOF)
	{
		byte = (u_char)c;
		++bytes_read;

		if (flags.uu_info_byte)
		{
			if (flags.output_data)
			{
				/* Write the info byte. Format:
				           R           P
				       -   - - - - -   - -
				     | 0 | x x x x x | 1 1 | -> mask = 0xFC
				       -   - - - - -   - -
				   R = Random bits from encode charset (high 
				       bit always zero)
				   P = Pad bytes count */
				assert(pad_bytes < 4);
				writeByte(getUUChar(0xFC,(u_char)pad_bytes));
			}
			flags.uu_info_byte = 0;
			line_bytes = 1;
		}

		/* Break up the byte and write it out as seperate uuencode
		   characters. Start from the low bits (RHS) and select the 
		   number of bits of the byte we need */
		for(part=0;part < byte_parts;)
		{
			/* Get bits we need and shift byte down for next time */
			data = byte & encode_mask;
			if (key) data ^= getKeyBits();
			if (lfsr_seed) data ^= getRandomBits();

			if (flags.output_data)
				writeByte(getUUChar(charset_mask,data));

			byte >>= bits_per_byte;
			++part;

			/* Write next line length char if we've written the
			   maximum line length */
			if (++line_bytes == UU_ENCODED_LINE_LEN)
			{
				++encode_lines;
				if (flags.output_data) writeByte('\n');

				/* Count of full byte parts yet to write out */
				cnt = (fs.st_size - bytes_read) * byte_parts;
				
				/* Count of remaining sections of current byte 
				   being split */
				cnt += (byte_parts - part);
				cnt = cnt / 4 * 3 + pad_bytes;

				if (cnt && flags.output_data)
				{
					/* Write count character */
					if (cnt < UU_DECODED_LINE_LEN)
						writeByte((char)(32 + cnt));
					else
						writeByte('M');
				}
				line_bytes = 0;
			}
		}
	}
	if (pad_bytes && flags.output_data)
	{
		/* Pad to multiple of 4 bytes. Output some random rubbish */
		for(i=0;i < pad_bytes;++i)
			writeByte(uu_charset[random() % UU_CHARSET_LEN]);
	}
	if (line_bytes)
	{
		++encode_lines;
		if (flags.output_data) writeByte('\n');
	}
	if (flags.output_data) fprintf(out_fp,"%c\nend\n",GRAVE);
}




/*** Get a valid UU encoding character with our data embedded ***/
char getUUChar(u_char mask, u_char data)
{
	u_char uu_char;
	int pos;
	int i;

	/* Pick random start position */
	pos = random() % UU_CHARSET_LEN;

	for(i=0;i < UU_CHARSET_LEN;++i)
	{
		/* Mask out the bits that'll our data will use then add our
		   data bits in */
		uu_char = ((u_char)uu_charset[pos] & mask) | data;
		if (IN_UU_CHARSET(uu_char)) return (char)uu_char;
		pos = (pos + 1) % UU_CHARSET_LEN;
	}

	/* Should never get here */
	assert(0);
	return 0;
}




void decodeUU()
{
	enum
	{
		STATE_FIND_HEADER,
		STATE_FIND_DATA,
		STATE_FIND_INFO_BYTE,
		STATE_DECODE
	};
	char *header = "begin NNN ";
	char sc;
	u_char byte;
	u_char out_byte;
	long output_len;
	int state;
	int bpos;
	int part;
	int line_pos;
	int cnt;
	int c;

	if (flags.debug) fprintf(stderr,"uuencode\n");

	state = STATE_FIND_HEADER;
	bpos = 0;
	line_pos = 0;
	out_byte = 0;
	output_len = 0;
	part = 0;

	while((c = readByte()) != EOF)
	{
		byte = (u_char)c;

		switch(state)
		{
		case STATE_FIND_HEADER:
			/* Look for begin */
			sc = header[bpos];
			if (byte == sc || (sc == 'N' && isdigit(byte)))
			{
				if (++bpos == (int)strlen(header))
					state = STATE_FIND_DATA;
			}
			else bpos = 0;
			break;

		case STATE_FIND_DATA:
			/* Find the newline following "begin NNN <file>" */
			if (byte == '\n') state = STATE_FIND_INFO_BYTE;
			break;

		case STATE_FIND_INFO_BYTE:
			if (!IN_UU_CHARSET(byte)) goto ENDLOOP;
			if (++line_pos == 2) 
			{
				pad_bytes = byte & 0x03;
				if (flags.debug)
					fprintf(stderr,"  * Pad length     : %d bytes\n",pad_bytes);
				encode_lines = 1;
				state = STATE_DECODE;
			}
			break;

		case STATE_DECODE:
			switch(byte)
			{
			case '\n':
				flags.uu_line_start = 1;
				continue;
			case GRAVE:
				if (flags.uu_line_start) goto ENDLOOP;
			}
			if (flags.uu_line_start)
			{
				/* Skip UU length byte */
				flags.uu_line_start = 0;
				++encode_lines;
				break;
			}
			byte &= encode_mask;
			if (key) byte ^= getKeyBits();
			if (lfsr_seed) byte ^= getRandomBits();
			out_byte |= (byte << (bits_per_byte * part));

			if (++part == byte_parts)
			{
				if (flags.output_data)
				{
					writeByte(out_byte);
					++output_len;
				}
				out_byte = 0;
				part = 0;
			}
			break;
		}
	}

	ENDLOOP:
	switch(state)
	{
	case STATE_FIND_HEADER:
		fprintf(stderr,"ERROR: Missing UU header.\n");
		exit(1);
	case STATE_FIND_DATA:
		fprintf(stderr,"ERROR: Missing data following UU header.\n");
		exit(1);
	case STATE_FIND_INFO_BYTE:
		fprintf(stderr,"ERROR: Missing info byte.\n");
		exit(1);
	case STATE_DECODE:
		/* Work out how many bytes to truncate the file by if we wrote
		   out the pad byte data */
		if ((cnt = (pad_bytes * bits_per_byte) / 8))
		{
			if (flags.output_data && output_file)
			{
				/* truncate doesn't seem to work if file still
				   open to write */
				fclose(out_fp);
				out_fp = NULL;
			    	if (truncate(output_file,output_len - cnt) == -1)
				{
					perror("ERROR: truncate()");
					exit(1);
				}
			}
		}
		if (flags.debug)
			printf("  * File truncation: %d bytes\n",cnt);
		break;
	default:
		assert(0);
	}
}
