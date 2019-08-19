#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdint.h>

#include <arpa/inet.h>

int HEX_DUMP_SHOW_ASCII = 0;

typedef struct {
	uint32_t length;
	uint32_t type;
	void *data;
	uint32_t crc;
} chunk_hdr;

void hex_dump(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';

	for (i = 0; i < size; ++i) {

		printf("%02X ", ((unsigned char*)data)[i]);

		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}

		if ((i+1) % 8 == 0 || i+1 == size) {

			printf(" ");

			if ((i+1) % 16 == 0) {

				if(HEX_DUMP_SHOW_ASCII)
					printf("|  %s \n", ascii);
				else
					printf("\n");

			} else if (i+1 == size) {

				ascii[(i+1) % 16] = '\0';

				if ((i+1) % 16 <= 8) {
					printf(" ");
				}

				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}

				if(HEX_DUMP_SHOW_ASCII)
					printf("|  %s \n", ascii);
				else
					printf("\n");

			}
		}
	}
}

void print_chunk_data(chunk_hdr *chunk)
{
	uint32_t hlen = ntohl(chunk->length);

	puts("chunk data:");

	if(htonl(chunk->type) == 0x49484452) { // IHDR
		
		struct ihdr_t {
			uint32_t width;
			uint32_t height;
			uint8_t  bit_depth;
			uint8_t  color_type;
			uint8_t  compression_method;
			uint8_t  filter_method;
			uint8_t  interlace_method;
		} *ihdr;

		ihdr = (struct ihdr_t *) chunk->data;

		printf("  width\n  ");
		hex_dump(&ihdr->width, 4);
		printf("  height\n  ");
		hex_dump(&ihdr->width, 4);
		printf("  bit depth\n  ");
		hex_dump(&ihdr->width, 1);
		printf("  color type\n  ");
		hex_dump(&ihdr->width, 1);
		printf("  compression method\n  ");
		hex_dump(&ihdr->width, 1);
		printf("  filter method\n  ");
		hex_dump(&ihdr->width, 1);
		printf("  interlace method\n  ");
		hex_dump(&ihdr->width, 1);
	}

	else if(htonl(chunk->type) == 0x70485973) { // pHYs

		struct phys_t {
			uint32_t pix_per_unit_x;
			uint32_t pix_per_unit_y;
			uint8_t  unit_specifier;
		} *phys;

		phys = (struct phys_t *) chunk->data;
	
		printf("  pixels per unit, X axis\n  ");
		hex_dump(&phys->pix_per_unit_x, 4);
		printf("  pixels per unit, Y axis\n  ");
		hex_dump(&phys->pix_per_unit_y, 4);
		printf("  unit specifier\n  ");
		hex_dump(&phys->unit_specifier, 1);
	}

	else {
		hex_dump(chunk->data, hlen);
	}

}

int main(int argc, char **argv)
{
	chunk_hdr chunk;
	uint32_t hlen;
	char *buff;
	int end_chunk = 0;

	if(argc < 2) {
		printf("Usage: %s [-a] <png file>\n\nOptions:\n\t-a\tenable ascii dump\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(argc == 3 && !strcmp(argv[1], "-a")) {
		HEX_DUMP_SHOW_ASCII = 1;
	}

	int fd = open(argv[argc - 1], O_RDONLY);

	if(fd == -1) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	/* LIST THE PNG FILE CONTENTS */

	// PNG file signature
	buff = malloc(8);

	if(read(fd, buff, 8) < 8) {
		fprintf(stderr, "Error reading the PNG signature\n");
		exit(EXIT_FAILURE);
	}

	puts("PNG signature:");
	hex_dump(buff, 8);
	puts("");

	free(buff);

	while(!end_chunk) {

		// PNG chunk

		if(read(fd, &chunk, 8) < 8) {
			fprintf(stderr, "Error reading a PNG chunk\n");
			exit(EXIT_FAILURE);
		}

		puts("chunk length:");
		hex_dump(&chunk.length, 4);
		puts("chunk type:");
		hex_dump(&chunk.type, 4);

		hlen = ntohl(chunk.length);

		if(hlen >= 0x80000000) {
			fprintf(stderr, "Error: invalid PNG chunk length: %u\n", hlen);
			exit(EXIT_FAILURE);
		}

		chunk.data = malloc(hlen);

		if(read(fd, chunk.data, hlen) < hlen) {
			fprintf(stderr, "Error reading PNG chunk data\n");
			exit(EXIT_FAILURE);
		}

		// print type-specific data
		print_chunk_data(&chunk);

		if(read(fd, &chunk.crc, 4) < 4) {
			fprintf(stderr, "Error reading PNG crc\n");
			exit(EXIT_FAILURE);     
		}

		puts("chunk crc:");
		hex_dump(&chunk.crc, 4);

		puts("");

		free(chunk.data);

		// check IEND chunk
		if (ntohl(chunk.type) == 0x49454e44) end_chunk = 1; 
	}


	exit(EXIT_SUCCESS);
}

