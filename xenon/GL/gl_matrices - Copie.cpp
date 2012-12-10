#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gl_xenos.h"
#include "Vec.h"
/**
 * OpenGL have 2 matrices ?
 **/
 #define MAX_MATRIX_STACK	32

// point to current matrix
static xe_matrix_t * current_matrix = NULL;
 
#define CURRENT_MATRIX_STACK current_matrix->stack[current_matrix->stackdepth]
 
#define USE_RH 1
 
void glMatrixMode (GLenum mode)
{
	switch (mode)
	{
		case GL_MODELVIEW:
			current_matrix = &GLImpl.modelview_matrix;
			break;

		case GL_PROJECTION:
			current_matrix = &GLImpl.projection_matrix;
			break;

		default:
			xe_gl_error ("glMatrixMode: unimplemented mode\n");
			break;
	}
}

void glLoadIdentity(void)
{
	matrixLoadIdentity(&CURRENT_MATRIX_STACK);
	current_matrix->dirty = 1;
}


void glLoadMatrixf (const GLfloat *m)
{
	memcpy (CURRENT_MATRIX_STACK.f, m, sizeof (matrix4x4));
	current_matrix->dirty = 1;
}


void glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	matrix4x4 tmp;

	// per spec, glFrustum multiplies the current matrix by the specified orthographic projection rather than replacing it
#if USE_RH
	matrixPerspectiveOffCenterRH(&tmp, left, right, bottom, top, zNear, zFar);
#else
	matrixPerspectiveOffCenterLH(&tmp, left, right, bottom, top, zNear, zFar);
#endif
	matrixMultiply(&CURRENT_MATRIX_STACK, &tmp, &CURRENT_MATRIX_STACK);
	current_matrix->dirty = 1;
}


void glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	matrix4x4 tmp;
#if USE_RH
	// per spec, glOrtho multiplies the current matrix by the specified orthographic projection rather than replacing it
	matrixOrthoOffCenterRH(&tmp, left, right, bottom, top, zNear, zFar);
#else
	matrixOrthoOffCenterLH(&tmp, left, right, bottom, top, zNear, zFar);
#endif
	matrixMultiply(&CURRENT_MATRIX_STACK, &tmp, &CURRENT_MATRIX_STACK);
	current_matrix->dirty = 1;
}


void glPopMatrix (void)
{
	if (!current_matrix->stackdepth)
	{
		// opengl silently allows this and so should we (witness TQ's R_DrawAliasModel, which pushes the
		// matrix once but pops it twice - on line 423 and line 468
		current_matrix->dirty = 1;
		return;
	}

	// go to a new matrix
	current_matrix->stackdepth--;

	// flag as dirty
	current_matrix->dirty = 1;
}


void glPushMatrix (void)
{
	if (current_matrix->stackdepth <= (MAX_MATRIX_STACK - 1))
	{
		// go to a new matrix (only push if there's room to push)
		current_matrix->stackdepth++;

		// copy up the previous matrix (per spec)
		memcpy
		(
			&current_matrix->stack[current_matrix->stackdepth],
			&current_matrix->stack[current_matrix->stackdepth - 1],
			sizeof (matrix4x4)
		);
	}

	// flag as dirty
	current_matrix->dirty = 1;
}


void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	// replicates the OpenGL glRotatef with 3 components and angle in degrees
	vector3 vec;
	matrix4x4 tmp;

	vec.x = x;
	vec.y = y;
	vec.z = z;

	matrixRotationAxis(&tmp, &vec, DegToRadian(angle));
	matrixMultiply (&current_matrix->stack[current_matrix->stackdepth], &tmp, &current_matrix->stack[current_matrix->stackdepth]);

	// dirty the matrix
	current_matrix->dirty = 1;
}


void glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	matrix4x4 tmp;
	matrixScaling(&tmp, x, y, z);
	matrixMultiply(&current_matrix->stack[current_matrix->stackdepth], &tmp, &current_matrix->stack[current_matrix->stackdepth]);

	// dirty the matrix
	current_matrix->dirty = 1;
}


void glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	matrix4x4 tmp;
	matrixTranslation(&tmp, x, y, z);
	matrixMultiply(&current_matrix->stack[current_matrix->stackdepth], &tmp, &current_matrix->stack[current_matrix->stackdepth]);

	// dirty the matrix
	current_matrix->dirty = 1;
}

void glGetFloatv (GLenum pname, GLfloat *params)
{
	switch (pname)
	{
	case GL_MODELVIEW_MATRIX:
		memcpy (params, &current_matrix->stack[current_matrix->stackdepth].f, sizeof (float) * 16);
		break;

	default:
		break;
	}
}

void glMultMatrixf (const GLfloat *m)
{
	matrix4x4 mat;

/*
	// copy out
	mat._11 = m[0];
	mat._12 = m[1];
	mat._13 = m[2];
	mat._14 = m[3];

	mat._21 = m[4];
	mat._22 = m[5];
	mat._23 = m[6];
	mat._24 = m[7];

	mat._31 = m[8];
	mat._32 = m[9];
	mat._33 = m[10];
	mat._34 = m[11];

	mat._41 = m[12];
	mat._42 = m[13];
	mat._43 = m[14];
	mat._44 = m[15];
*/
	memcpy(mat.f, m, sizeof(GLfloat) * 16);

	matrixMultiply (&current_matrix->stack[current_matrix->stackdepth], &mat, &current_matrix->stack[current_matrix->stackdepth]);

	// dirty the matrix
	current_matrix->dirty = 1;
}

void CGLImpl::InitializeMatrix(xe_matrix_t *m){
	// initializes a matrix to a known state prior to rendering
	m->dirty = 1;
	m->stackdepth = 0;
	matrixLoadIdentity (&m->stack[0]);
}


void CGLImpl::CheckDirtyMatrix(xe_matrix_t *m){
	if (m->dirty)
	{
		device->SetVertexShaderConstantF(m->usage, (float*)&m->stack[m->stackdepth], 4);
		m->dirty = 0;
	}
}

void CGLImpl::ResetMatrixDirty() {
	modelview_matrix.dirty = 1;
	projection_matrix.dirty = 1;
}

void CGLImpl::InitializeMatrices() 
{
	modelview_matrix.dirty = 1;
	modelview_matrix.stackdepth = 0;
	modelview_matrix.usage = MATPROJECTION;

	modelview_matrix.dirty = 1;
	modelview_matrix.stackdepth = 0;
	modelview_matrix.usage = MATMODELVIEW;
}
