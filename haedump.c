#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#define LINE_SIZE (8+2+(16*3)+2+16+1+16+2+1)

const char ARGS[] = "h";
const char HELP[] = "Hex-Ascii-Ebcdic-DUMP tool -- close to `hexdump -C`\n"
"License: Public Domain\n"
"\n"
"%s [options] [files]\n"
"Options:\n"
"\t-h -- this help\n"
"Files:\n"
"\tfilenames or '-' for stdin (defaults to stdin too).\n"
"";

static const unsigned char ebc2asc[256] =
	/* 0123456789abcdef0123456789abcdef */
	"................................" /* 1F */
	"................................" /* 3F */
	" ...........<(+|&.........!$*);." /* 5F first.chr.here.is.real.space */
	"-/.........,%_>?.........`:#@'=\""/* 7F */
	".abcdefghi.......jklmnopqr......" /* 9F */
	"..stuvwxyz......................" /* BF */
	".ABCDEFGHI.......JKLMNOPQR......" /* DF */
	"..STUVWXYZ......0123456789......";/* FF */

static void dump16(uint8_t *buffer, off_t offset, size_t count)
{
	size_t i;
       	uint8_t c;

	printf("%08x ", (unsigned)offset);
	for (i=0; i<count; i++) {
		printf(" %02x", buffer[i]);
	}
	for (; i<16; i++) {
		printf("   ");
	}
	printf(" |");
	for (i=0; i<count; i++) {
		c = buffer[i];
		printf("%c", isprint(c) ? c : '.');
	}
	for (; i<16; i++) {
		printf(" ");
	}
	printf("|");
	for (i=0; i<count; i++) {
		c = ebc2asc[buffer[i]];
		printf("%c", isprint(c) ? c : '.');
	}
	for (; i<16; i++) {
		printf(" ");
	}
	printf("|\n");
}

static int dump(FILE *fp)
{
	uint8_t buffer[16], last[16];
	size_t count, dups;
	off_t offset = 0;

	memset(last,0,16);
	dups = 0;
	while ((count = fread(buffer,1,16,fp)) > 0) {
		if ((count < 16) && !feof(fp)) {
			return -1; /* it's an IO error */
		}
		if (offset && (memcmp(buffer,last,16) == 0)) {
			dups += 1;
			offset += 16;
			if (feof(fp)) {
				printf("* %d\n",dups);
				printf("%08x\n", (unsigned)offset);
				return 0;
			}
			continue;
		}
		if (dups) {
			printf("* %d\n", dups);
			dups = 0;
		}
		dump16(buffer,offset,count);
		offset += 16;
		if (feof(fp)) {
			printf("%08x\n", (unsigned)offset);
			return 0;
		}
		memcpy(last, buffer, 16);
	}
	if (dups)
		printf("* %d\n", dups);
	printf("%08x\n", (unsigned)offset);
	if (feof(fp)) {
		return 0;
	}
	return -1;
}

int main(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, ARGS)) != -1) {
		switch (opt) {
		case 'h':
			printf(HELP,argv[0]);
			exit(EXIT_SUCCESS);
		default:
			fprintf(stderr,"%s: unknown option '%c'.\n",argv[0],opt);
			exit(EXIT_FAILURE);
		}
	}
	if (optind >= argc) {
		if (dump(stdin)) {
			exit(EXIT_FAILURE);
		}
	} else {
		int i;
		for (i=optind; i<argc; i++) {
			if (strcmp(argv[i],"-") == 0) {
				printf("# <STDIN>\n");
				if (dump(stdin))
					exit(EXIT_FAILURE);
			} else {
				FILE *fp;
				if ((fp = fopen(argv[i],"rb")) == NULL) {
					perror(argv[i]);
					exit(EXIT_FAILURE);
				}
				printf("# %s\n",argv[i]);
				if (dump(fp)) {
					perror(argv[i]);
					exit(EXIT_FAILURE);
				}
				fclose(fp);
			}
		}
		printf("# <EOD>\n");
	}
	exit(EXIT_SUCCESS);
}
/* EOF */
