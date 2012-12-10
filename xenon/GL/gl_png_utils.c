#include "gl_png_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <debug.h>

struct file_buffer_t {
	char name[256];
	unsigned char *data;
	long length;
	long offset;
};

struct pngMem {
	unsigned char *png_end;
	unsigned char *data;
	int size;
	int offset; //pour le parcours
};

static int offset = 0;

static void png_mem_write(png_structp png_ptr, png_bytep data, png_size_t length) {
	struct file_buffer_t *dst = (struct file_buffer_t *) png_get_io_ptr(png_ptr);
	/* Copy data from image buffer */
	memcpy(dst->data + dst->offset, data, length);
	/* Advance in the file */
	dst->offset += length;
}

int png_save_texture_to_memory(struct XenosSurface * surface, unsigned char *PNGdata, int * size) {
	if (surface->bypp != 4)
		return 0;
	
	png_structp png_ptr_w;
	png_infop info_ptr_w;
	//        int number_of_passes;
	png_bytep * row_pointers;

	offset = 0;

	struct file_buffer_t *file;
	file = (struct file_buffer_t *) malloc(sizeof (struct file_buffer_t));
	file->length = 1024 * 1024 * 5;
	file->data = PNGdata; //5mo ...
	file->offset = 0;

	/* initialize stuff */
	png_ptr_w = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr_w) {
		printf("[write_png_file] png_create_read_struct failed\n");
		return 0;
	}

	info_ptr_w = png_create_info_struct(png_ptr_w);
	if (!info_ptr_w) {
		printf("[write_png_file] png_create_info_struct failed\n");
		return 0;
	}

	png_set_write_fn(png_ptr_w, (png_voidp *) file, png_mem_write, NULL);
	png_set_IHDR(png_ptr_w, info_ptr_w, surface->width, surface->height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	uint32_t *data = (uint32_t *) surface->base;
	uint32_t * untile_buffer = malloc(sizeof(uint32_t)*surface->width * surface->height);
	row_pointers = malloc(sizeof(png_bytep) * surface->height);

	int y, x;
	for (y = 0; y < surface->height; ++y) {	
		//row_pointers[y] = (png_bytep) (data + y * surface->width);
		for (x = 0; x < surface->width; ++x) {
			untile_buffer[y * surface->width + x] = __builtin_bswap32(data[y * surface->width + x] & 0xFFFFFF)|0xFF;
		}
		row_pointers[y] = (png_bytep) (untile_buffer + y * surface->width);
	}

	png_set_rows(png_ptr_w, info_ptr_w, row_pointers);
	png_write_png(png_ptr_w, info_ptr_w, PNG_TRANSFORM_IDENTITY, 0);
	png_write_end(png_ptr_w, info_ptr_w);
	png_destroy_write_struct(&png_ptr_w, &info_ptr_w);

	*size = file->offset;

	//free(file->data);
	free(file);
	//delete(data);
	free(row_pointers);
	free(untile_buffer);
	
	return 1;
}

int png_save_texture_to_file(struct XenosSurface * surface, const char * filename)
{
	FILE * fd = fopen(filename, "wb");
	printf("%s\n", filename);
	if (fd==NULL) {
		return 0;
	}
	if (!surface)
		return 0;
		
	// bigger than real
	unsigned char *PNGdata = (unsigned char *)malloc(1024 * 1024 * 4);
	int size = 0;
	
	if (png_save_texture_to_memory(surface, PNGdata, &size)) {
		fwrite(PNGdata, 1, size, fd);
		printf("[Texture] Saved in %s\n", filename);
	}

	free(PNGdata);
	fclose(fd);
	return size;
}
