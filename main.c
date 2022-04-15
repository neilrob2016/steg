/*****************************************************************************
 STEG

 A proof of concept steganography program that hides data in what looks like
 genuine uuencoded or base64 encoded files.

 March 2022
 *****************************************************************************/
#define MAINFILE
#include "globals.h"
#include "build_date.h"

#define BITS_PER_BYTE 2
#define B64_LINE_LEN  70 /* Can be any length but MIME max of 76 */

void parseCmdLine(int argc, char **argv);
void init();
void encode();
void decode();
void version();

int main(int argc, char **argv)
{
	parseCmdLine(argc,argv);
	init();

	if (flags.encode)
		encode();
	else
		decode();
	return 0;
}




void parseCmdLine(int argc, char **argv)
{
	char c;
	int i;
	
	in_fd = STDIN;
	out_fp = stdout;
	input_file = NULL;
	output_file = NULL;
	uu_hdr_file = NULL;
	key = NULL;
	bits_per_byte = BITS_PER_BYTE;
	base64_line_len = B64_LINE_LEN;
	lfsr_seed = 0;

	bzero(&flags,sizeof(flags));
	flags.output_data = 1;

	for(i=1;i < argc;++i)
	{
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) goto USAGE;
		c = argv[i][1];
		switch(c)
		{
		case 'd':
			flags.debug = 1;
			continue;
		case 'e':
			flags.encode = 1;
			continue;
		case 'n':
			flags.debug = 1;
			flags.output_data = 0;
			continue;
		case 'u':
			flags.uuencode = 1;
			continue;
		case 'v':
			version();
		}
		if (++i == argc) goto USAGE;

		switch(c)
		{
		case 'b':
			switch((bits_per_byte = atoi(argv[i])))
			{
			case 1:
			case 2:
			case 4:
				break;
			default:
				goto USAGE;
			}
			break;
		case 'f':
			uu_hdr_file = argv[i];
			if (!uu_hdr_file[0]) goto ERROR;
			break;
		case 'i':
			input_file = argv[i];
			if (!input_file[0]) goto ERROR;
			break;
		case 'k':
			key = argv[i];
			if (!key[0]) goto ERROR;
			break;
		case 'l':
			/* Zero means one long line */
			if ((base64_line_len = atoi(argv[i])) < 0)
			{
				fprintf(stderr,"ERROR: Invalid -l value. Must be >= 0.\n");
				exit(1);
			}
			break;
		case 'o':
			output_file = argv[i];
			if (!output_file[0]) goto ERROR;
			break;
		case 's':
			if ((lfsr_seed = (uint64_t)strtol(argv[i],NULL,10)) < 1)
			{
				fprintf(stderr,"ERROR: Invalid -s value. Must be > 0.\n");
				exit(1);
			}
			break;
		default:
			goto USAGE;
		}
	}
	if (!flags.output_data && output_file)
	{
		fprintf(stderr,"ERROR: The -n and -o options are mutually exclusive.\n");
		exit(1);
	}
	if (flags.uuencode && flags.encode && !input_file)
	{
		fprintf(stderr,"ERROR: Uuencode requires -i as the data size must be known at the start.\n");
		exit(1);
	}
	return;

	USAGE:
	fprintf(stderr,"Usage: %s\n"
	       "       -i <input file>       : Required for uuencode encoding. Default = stdin.\n"
	       "       -o <output file>      : Default = stdout.\n"
	       "       -f <uu header file>   : Filename to put in uuencode header. Default is\n"
	       "                               input file name. Ignored with Base64 or if\n"
	       "                               decoding.\n"
	       "       -b <bits per byte>    : Number of encoding bits used per output data\n"
	       "                               byte. Can be 1, 2 or 4. Default = %d.\n"
	       "       -l <base64 line len>  : Length of each encoded line for Base64. Zero\n"
	       "                               means one long line. Ignored with uuencode or\n"
	       "                               if decoding. Default = %d.\n"
	       "       -k <encryption key>   : Bits cyclically XOR'd with data. Can be used in\n"
	       "                               conjunction with -s. Most effective when close\n"
	       "                               to or equal to length of the data when it acts\n"
	       "                               as a one time pad. Default = not set.\n"
	       "       -s <LFSR seed>        : Seeds LFSR pseudo random number generator whose\n"
	       "                               output is XOR'd with the data. Default = not set.\n"
	       "       -u                    : Fake encode/decode = uuencode. Default = Base64.\n"
	       "       -e                    : Encode data. Default = decode.\n"
	       "       -d                    : Print debug info (to stderr).\n"
	       "       -n                    : No data output, debug info only. -d implied.\n"
	       "       -v                    : Print version info then exit.\n"
	       "Note: Decoding a uuencode file to stdout may occasionally add 1 or 2 extra\n"
	       "      unwanted bytes to the data as it cannot be truncated away as it would be\n"
	       "      if writing to a file.\n",
		argv[0],BITS_PER_BYTE,B64_LINE_LEN);
	exit(1);

	ERROR:
	fprintf(stderr,"ERROR: Empty string given for key or file name.\n");
	exit(1);
}




void init()
{
	srandom(time(0));

	if (flags.debug)
		fprintf(stderr,"> Opening input file : \"%s\"\n",input_file);

	if (input_file)
	{
		if ((in_fd = open(input_file,O_RDONLY)) == -1)
		{
			fprintf(stderr,"ERROR: open(\"%s\"): %s\n",
				input_file,strerror(errno));
			exit(1);
		}
		if (fstat(in_fd,&fs) == -1)
		{
			fprintf(stderr,"ERROR: fstat(\"%s\"): %s\n",
				input_file,strerror(errno));
			exit(1);
		}
	}

	if (output_file)
	{
		if (flags.debug) 
			fprintf(stderr,"> Opening output file: \"%s\"\n",output_file);
		if (!(out_fp = fopen(output_file,"w")))
		{
			fprintf(stderr,"ERROR: fopen(\"%s\"): %s\n",
				output_file,strerror(errno));
			exit(1);
		}
	}
	else if (flags.debug)
	{
		if (flags.output_data)
			fprintf(stderr,"> Writing output to stderr...\n");
		else
			fprintf(stderr,"> No data output.\n");
	}

	if (key)
	{
		key_len = strlen(key);
		key_pos = 0;
		key_shift = 0;
	}

	if (lfsr_seed)
	{
		lfsr_val = lfsr_seed;
		lfsr_shift = 0;
		setRandomValue();
	}

	flags.uu_info_byte = 1;
	byte_parts = 8 / bits_per_byte;
	encode_mask = 0xFF >> (8 - bits_per_byte);
	charset_mask = (0xFF >> bits_per_byte) << bits_per_byte;

	if (flags.debug)
	{
		fprintf(stderr,"  * Encode bits    : %d per byte\n",bits_per_byte);
		fprintf(stderr,"  * Encode mask    : 0x%02X\n",encode_mask);
		fprintf(stderr,"  * Charset mask   : 0x%02X\n",charset_mask);
	}

	
}




void encode()
{
	encode_lines = 0;

	if (flags.debug)
	{
		fprintf(stderr,"  * Fake encoding  : %s\n",
			flags.uuencode ? "uuencode" : "base64");
	}
	if (flags.uuencode)
		encodeUU();
	else
		encodeB64();
	if (out_fp != stdout) fclose(out_fp);

	if (flags.debug)
	{
		if (flags.output_data)
			fprintf(stderr,"> Wrote %d encoded lines.\n",encode_lines);
		else
			fprintf(stderr,"> Would have written %d encoded lines.\n",encode_lines);
	}
}




void decode()
{
	encode_lines = 0;

	if (flags.debug) fprintf(stderr,"  * Fake decoding  : ");

	if (flags.uuencode)
		decodeUU();
	else
		decodeB64();
	if (out_fp && out_fp != stdout) fclose(out_fp);

	if (flags.debug)
		fprintf(stderr,"> Read %d encoded lines.\n",encode_lines);
}




void version()
{
	puts("\n*** Steg ***\n");
	puts("Copyright (C) Neil Robertson 2022\n");
	printf("Version   : %s\n",VERSION);
	printf("Build date: %s\n\n",BUILD_DATE);
	exit(0);
}
