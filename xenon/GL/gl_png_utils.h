#include <xenos/xe.h>

int png_save_texture_to_file(struct XenosSurface * surface, const char * filename); 
int png_save_texture_to_memory(struct XenosSurface * surface, unsigned char *PNGdata, int * size); 
