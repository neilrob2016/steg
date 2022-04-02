#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#ifdef MAINFILE
#define EXTERN
#else
#define EXTERN extern
#endif

#define VERSION "20220402"

struct st_flags
{
	unsigned encode         : 1;
	unsigned debug          : 1;
	unsigned output_data    : 1;
	unsigned base64         : 1;
	unsigned uu_info_byte   : 1;
	unsigned uu_started     : 1;
	unsigned uu_find_data   : 1;
	unsigned uu_line_start  : 1;
	unsigned b64_prev_valid : 1;
};

EXTERN struct st_flags flags;
EXTERN struct stat fs;
EXTERN FILE *out_fp;
EXTERN u_char *mm_start;
EXTERN u_char *mm_end;
EXTERN u_char encode_mask;
EXTERN u_char charset_mask;
EXTERN char *input_file;
EXTERN char *output_file;
EXTERN char *uu_hdr_file;
EXTERN long last_line_cnt;
EXTERN int bits_per_byte;
EXTERN int byte_parts;
EXTERN int pad_bytes;
EXTERN int encode_lines;
EXTERN int base64_line_len;

EXTERN char *key;
EXTERN int key_len;
EXTERN int key_pos;
EXTERN int key_shift;

/* uuencode.c */
void    encodeUU();
void    decodeUU();

/* base64.c */
void encodeB64();
void decodeB64();

/* common.c */
u_char getKeyBits();
void   writeByte(u_char byte);
