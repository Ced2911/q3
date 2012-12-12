#include <cstdio>
#include <cstdlib>
#include "gl_main.h"

GLTexture::GLTexture(int width, int height, D3DFORMAT format) {	
	_info.uncompressed_format = _info.format = format;
	_info.compressed = 0;
	_info.tiled = 0;

	D3DXCreateTexture(GLImpl.device, width, height, 1, 0, format, 0, &tex);
}
GLTexture::~GLTexture()
{
	if(tex) {
		tex->Release();
	}
}
void GLTexture::GetLevelDesc(int level, D3DSURFACE_DESC *pDesc) {
	tex->GetLevelDesc(level, pDesc);
}

void GLTexture::setTexture(int sampler)
{
	GLImpl.device->SetTexture(sampler, tex);
}

void GLTexture::lockTexture() {
	// get texture information
	tex->GetLevelDesc(0, &_info.desc);		
	tex->LockRect(0, &_info.rect, NULL, NULL);

	_info.XgFlags = 0;

	if (XGIsBorderTexture((D3DBaseTexture*)this)) {
		_info.XgFlags |= XGTILE_BORDER;
	}
	if (!XGIsPackedTexture((D3DBaseTexture*)this)) {
		_info.XgFlags |= XGTILE_NONPACKED;
	}

	// untile the surface
	if (_info.tiled) {
		XGUntileTextureLevel(_info.desc.Width, _info.desc.Height, 0, XGGetGpuFormat(_info.desc.Format), _info.XgFlags, _info.rect.pBits, _info.rect.Pitch, NULL, _info.rect.pBits, NULL);
	}
}

void GLTexture::unlockTexture()
{
	// until
	_info.tiled = 1;
	XGTileTextureLevel(_info.desc.Width, _info.desc.Height, 0, XGGetGpuFormat(_info.desc.Format), _info.XgFlags, _info.rect.pBits, NULL, _info.rect.pBits, _info.rect.Pitch, NULL);
	
	// unlock
	tex->UnlockRect(0);
}

BYTE * GLTexture::getData()
{
	//return (BYTE*)_info.data;
	return (BYTE*)_info.rect.pBits;
}

int GLTexture::getPitch()
{
	return _info.rect.Pitch;
}