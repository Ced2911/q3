#if 0
//
// Generated by 2.0.21076.0
//
//   fxc /Fh opengl.ps.color.h /Tps_3_0 opengl.hlsl /Eps_color
//
// Shader type: pixel 

xps_3_0
config AutoSerialize=false
config AutoResource=false
config PsMaxReg=2
// PsExportColorCount=1
// PsSampleControl=both

dcl_texcoord r0.xy
dcl_texcoord1 r1.xy
dcl_color_centroid r2


    alloc colors
    exece
    mov oC0, r2

// PDB hint 00000000-00000000-00000000

#endif

// This microcode is in native DWORD byte order.

const DWORD g_xps_ps_color[] =
{
    0x102a1100, 0x00000084, 0x00000024, 0x00000000, 0x00000024, 0x00000000, 
    0x00000058, 0x00000000, 0x00000000, 0x00000030, 0x0000001c, 0x00000023, 
    0xffff0300, 0x00000000, 0x00000000, 0x00000000, 0x0000001c, 0x70735f33, 
    0x5f300032, 0x2e302e32, 0x31303736, 0x2e3000ab, 0x00000000, 0x00000024, 
    0x10000200, 0x00000008, 0x00000000, 0x00002063, 0x00030007, 0x00000001, 
    0x00003050, 0x00003151, 0x0000f2a0, 0x00000000, 0x1001c400, 0x22000000, 
    0xc80f8000, 0x00000000, 0xe2020200, 0x00000000, 0x00000000, 0x00000000
};
