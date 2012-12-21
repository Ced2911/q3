#include <xtl.h>
#include <xinput2.h>
#include <xaudio2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../renderer/tr_local.h"
#include "../sys/sys_local.h"
#include "../client/client.h"

void XDKGlInit();
void XDKGlDisplay();
void XDKGlBeginFrame();
void XDKGlEndFrame();
void XDKGlGetScreenSize(int * width, int * height);
void XDKSetGamma(unsigned char * red, unsigned char * green, unsigned char * blue);

void (APIENTRYP qglActiveTextureARB) (GLenum texture);
void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT) (void);

void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	// XDKSetGamma(red, green, blue);
}

/*
===============
GLimp_Init

This routine is responsible for initializing the OS specific portions
of OpenGL
===============
*/
void GLimp_Init( void )
{
	XDKGlInit();

	glConfig.textureCompression = TC_NONE;
	//glConfig.textureEnvAddAvailable = qtrue;
	glConfig.textureEnvAddAvailable = qfalse;

	 // This values force the UI to disable driver selection
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;
	glConfig.deviceSupportsGamma = 0;

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
			glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

	XDKGlGetScreenSize(&glConfig.vidWidth, &glConfig.vidHeight);
	/*
	glConfig.vidWidth = 640;
	glConfig.vidHeight = 480;
	*/
	glConfig.isFullscreen = qtrue;


	glConfig.colorBits = 32;
	glConfig.depthBits = 24;
	glConfig.stencilBits = 8;

	// todo
	glConfig.numTextureUnits = 1;
	// glConfig.deviceSupportsGamma = 1;

	qglLockArraysEXT = glLockArraysEXT;
	qglUnlockArraysEXT = glUnlockArraysEXT;
	qglMultiTexCoord2fARB = glMultiTexCoord2f;

	// not working
	//qglClientActiveTextureARB = glClientActiveTexture;
	//qglActiveTextureARB = glActiveTexture;

	glConfig.textureEnvAddAvailable = 1;

	// qglLockArraysEXT = qglUnlockArraysEXT = qglMultiTexCoord2fARB = qglClientActiveTextureARB = qglActiveTextureARB = 0;

	IN_Init( );
}

/*
===============
GLimp_Shutdown
===============
*/
void GLimp_Shutdown( void )
{
	ri.IN_Shutdown();

	Com_Memset( &glConfig, 0, sizeof( glConfig ) );
	Com_Memset( &glState, 0, sizeof( glState ) );
}

/*
===============
GLimp_Minimize

Minimize the game so that user is back at the desktop
===============
*/
void GLimp_Minimize(void)
{
}

/*
===============
GLimp_EndFrame

Responsible for doing a swapbuffers
===============
*/
void GLimp_EndFrame( void )
{
	// don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
	{		
		XDKGlDisplay();
		// XDKGlEndFrame();
	}
}

/*
===============
GLimp_LogComment
===============
*/
void GLimp_LogComment( char *comment )
{
}

#if 1
// No SMP - stubs
void GLimp_RenderThreadWrapper( void *arg )
{
}

qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n");
	return qfalse;
}

void *GLimp_RendererSleep( void )
{
	return NULL;
}

void GLimp_FrontEndSleep( void )
{
}

void GLimp_WakeRenderer( void *data )
{
}
#else
/*
===========================================================

SMP acceleration

===========================================================
*/

HANDLE	renderCommandsEvent;
HANDLE	renderCompletedEvent;
HANDLE	renderActiveEvent;

void (*glimpRenderThread)( void );

void GLimp_RenderThreadWrapper( void ) {
	glimpRenderThread();
}

/*
=======================
GLimp_SpawnRenderThread
=======================
*/
HANDLE	renderThreadHandle;
DWORD	renderThreadId;
qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {

	renderCommandsEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderCompletedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderActiveEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	glimpRenderThread = function;

	renderThreadHandle = CreateThread(
	   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
	   0,		// DWORD cbStack,
	   (LPTHREAD_START_ROUTINE)GLimp_RenderThreadWrapper,	// LPTHREAD_START_ROUTINE lpStartAddr,
	   0,			// LPVOID lpvThreadParm,
	   0,			//   DWORD fdwCreate,
	   &renderThreadId );

	if ( !renderThreadHandle ) {
		return qfalse;
	}

	return qtrue;
}

static	void	*smpData;
static	int		wglErrors;

void *GLimp_RendererSleep( void ) {
	void	*data;

	ResetEvent( renderActiveEvent );

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent( renderCompletedEvent );

	WaitForSingleObject( renderCommandsEvent, INFINITE );

	ResetEvent( renderCompletedEvent );
	ResetEvent( renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent( renderActiveEvent );
	return data;
}


void GLimp_FrontEndSleep( void ) {
	WaitForSingleObject( renderCompletedEvent, INFINITE );
}


void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	// after this, the renderer can continue through GLimp_RendererSleep
	SetEvent( renderCommandsEvent );

	WaitForSingleObject( renderActiveEvent, INFINITE );
}
#endif
/*
===============
Input
===============
*/
static DWORD dwPlayerIndex = 0;

int	joyDirectionKeys[16] = {
	K_JOY1, K_JOY2,
	K_JOY3, K_JOY4,
	K_ENTER, K_ESCAPE, // start / back
	K_JOY7, K_JOY8,
	K_JOY9, K_JOY10,
	K_JOY11, K_JOY12,
	K_JOY13, K_JOY14,
	K_JOY15, K_JOY16,
};

static DWORD oldButtons, buttons = 0;
static short oldAxis[4];
static BYTE oldTrigger[2];

static short filter_axis(short axis, int deadzone) {
	if(axis < deadzone && axis > -deadzone)
		axis = 0;

	return axis;
}

// from xinput doc
static void circular_deadzone(short *x, short *y, int deadzone) {
	float LX = *x;
	float LY = *y;

	//determine how far the controller is pushed
	float magnitude = sqrt(LX*LX + LY*LY);

	//determine the direction the controller is pushed
	float normalizedLX = LX / magnitude;
	float normalizedLY = LY / magnitude;

	float normalizedMagnitude = 0;

	//check if the controller is outside a circular dead zone
	if (magnitude > deadzone)
	{
	  //clip the magnitude at its expected maximum value
	  if (magnitude > 32767) magnitude = 32767;
  
	  //adjust magnitude relative to the end of the dead zone
	  magnitude -= deadzone;

	  //optionally normalize the magnitude with respect to its expected range
	  //giving a magnitude value of 0.0 to 1.0
	  normalizedMagnitude = magnitude / (32767 - deadzone);
	}
	else //if the controller is in the deadzone zero out the magnitude
	{
		magnitude = 0.0;
		normalizedMagnitude = 0.0;
	}

	LX = normalizedLX * normalizedMagnitude * magnitude;
	LY = normalizedLY * normalizedMagnitude * magnitude;

	*x = LX;
	*y = LY;
}

#define XKEYSIZE	512
enum keyboard_st {
	keyboard_hide = 0,
	keyboard_show = 1,
	keyboard_succes = 2
};
static volatile int keyboard_states = keyboard_hide;
static XOVERLAPPED keyboard_event;
static char q3keyboard[512];

static void keyboard_thread() {
	WCHAR xkeyboard[XKEYSIZE];
	memset(xkeyboard, 0, XKEYSIZE * sizeof(WCHAR));
	while(1) {
		if (keyboard_states == keyboard_show) {
			XShowKeyboardUI(0, VKBD_DEFAULT, NULL, NULL, 
				  L"Quake3",  
				  xkeyboard, XKEYSIZE, &keyboard_event);

			// wait for keyboard end
			while(!XHasOverlappedIoCompleted(&keyboard_event)) {
				Sleep(100);
			}
			// keyboard closed
			if (keyboard_event.dwExtendedError != ERROR_CANCELLED) {
				wcstombs(q3keyboard, xkeyboard, 512);
				keyboard_states = keyboard_succes;
			} else {
				keyboard_states = keyboard_hide;
			}
		}

		Sleep(250);
	}
}

static void xinput_update() {
	XINPUT_STATE state;
	int i = 0;

	if (XInputGetState(dwPlayerIndex, &state) != ERROR_SUCCESS)
		return;
	
	buttons = state.Gamepad.wButtons;

	for( i = 0; i < 16; i++ ) {
		DWORD v = 1 << i;
				
		// 0 && 1 // pushed
		if (!(oldButtons  & v) && (buttons  & v)) {
			Com_QueueEvent( 0, SE_KEY, joyDirectionKeys[i], qtrue, 0, NULL );
		} 
		// 1 && 0 // released
		else if (oldButtons  & v && !(buttons  & v)){
			Com_QueueEvent( 0, SE_KEY, joyDirectionKeys[i], qfalse, 0, NULL );
		}
	}

	if (( Key_GetCatcher() & KEYCATCH_UI ) || (Key_GetCatcher() & KEYCATCH_CGAME)) {
		if (keyboard_states == keyboard_hide) {
			if (!(oldButtons & XINPUT_GAMEPAD_Y) && (buttons & XINPUT_GAMEPAD_Y)) {
				keyboard_states = keyboard_show;
			}
		}
	}
	if (keyboard_states == keyboard_succes) {
		for( i = 0; i < XKEYSIZE; i++) {
			if (q3keyboard[i] == 0) {
				break;
			}
			Com_QueueEvent( 0, SE_CHAR, q3keyboard[i], 0, 0, NULL );
		}
		keyboard_states = keyboard_hide;
	}

	// trigger
	state.Gamepad.bLeftTrigger = state.Gamepad.bLeftTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD?255:0;
	state.Gamepad.bRightTrigger = state.Gamepad.bRightTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD?255:0;

	if (!oldTrigger[0] && state.Gamepad.bLeftTrigger )
		Com_QueueEvent( 0, SE_KEY, K_JOY20, qtrue, 0, NULL );
	else if (oldTrigger[0] && !state.Gamepad.bLeftTrigger )
		Com_QueueEvent( 0, SE_KEY, K_JOY20, qfalse, 0, NULL );

	if (!oldTrigger[1] && state.Gamepad.bRightTrigger )
		Com_QueueEvent( 0, SE_KEY, K_JOY21, qtrue, 0, NULL );
	else if (oldTrigger[1] && !state.Gamepad.bRightTrigger )
		Com_QueueEvent( 0, SE_KEY, K_JOY21, qfalse, 0, NULL );

	// filter
	state.Gamepad.sThumbLX = filter_axis(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	state.Gamepad.sThumbLY = filter_axis(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	state.Gamepad.sThumbRX = filter_axis(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	state.Gamepad.sThumbRY = filter_axis(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	/* // slow ...
	circular_deadzone(&state.Gamepad.sThumbLX, &state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	circular_deadzone(&state.Gamepad.sThumbRX, &state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	*/
	if (( Key_GetCatcher( ) & KEYCATCH_UI ) || (Key_GetCatcher( ) & KEYCATCH_CGAME)) {
		int mouse_x, mouse_y;

		mouse_x = state.Gamepad.sThumbLX>>13;
		mouse_y = -state.Gamepad.sThumbLY>>13;

		Com_QueueEvent( 0, SE_MOUSE, mouse_x, mouse_y, 0, NULL );
	} else {
		// axis
		if (state.Gamepad.sThumbLX != oldAxis[0]) {
			Com_QueueEvent( 0, SE_JOYSTICK_AXIS, 0, state.Gamepad.sThumbLX, 0, NULL );
		}
		if (state.Gamepad.sThumbLY != oldAxis[1]) {
			Com_QueueEvent( 0, SE_JOYSTICK_AXIS, 1, -state.Gamepad.sThumbLY, 0, NULL );
		}
		if (state.Gamepad.sThumbRX != oldAxis[2]) {
			Com_QueueEvent( 0, SE_JOYSTICK_AXIS, 4, state.Gamepad.sThumbRX, 0, NULL );
		}
		if (state.Gamepad.sThumbRY != oldAxis[3]) {
			Com_QueueEvent( 0, SE_JOYSTICK_AXIS, 3, -state.Gamepad.sThumbRY, 0, NULL );
		}
	}

	oldAxis[0] = state.Gamepad.sThumbLX;
	oldAxis[1] = state.Gamepad.sThumbLY;
	oldAxis[2] = state.Gamepad.sThumbRX;
	oldAxis[3] = state.Gamepad.sThumbRY;
	
	oldTrigger[0] = state.Gamepad.bLeftTrigger;
	oldTrigger[1] = state.Gamepad.bRightTrigger;

	oldButtons = buttons;
}

void IN_Init( void )
{
	HANDLE XkeyboardHandle;

	Key_SetBinding(K_JOY1 + 0, "+scores");		// XINPUT_GAMEPAD_DPAD_UP
	Key_SetBinding(K_JOY1 + 1, "+attack");	
	Key_SetBinding(K_JOY1 + 2, "weapprev");
	Key_SetBinding(K_JOY1 + 3, "weapnext");

	// START BACK
	//Key_SetBinding(K_JOY1 + 4, "+button2");		// XINPUT_GAMEPAD_START
	//Key_SetBinding(K_JOY1 + 5, "togglemenu");	// XINPUT_GAMEPAD_BACK
	
	// Stick
	//Key_SetBinding(K_JOY1 + 6, "+moveleft");	// XINPUT_GAMEPAD_LEFT_SHOULDER
	//Key_SetBinding(K_JOY1 + 7, "+moveright");

	// LT RT
	Key_SetBinding(K_JOY1 + 8, "+moveleft");		// XINPUT_GAMEPAD_LEFT_THUMB
	Key_SetBinding(K_JOY1 + 9, "+moveright");	

	Key_SetBinding(K_JOY1 + 10, "+scores");		// XINPUT_GAMEPAD_BIGBUTTON

	// ABXY
	Key_SetBinding(K_JOY1 + 12, "+moveup");					
	Key_SetBinding(K_JOY1 + 13, "+movedown");	// XINPUT_GAMEPAD_A
	Key_SetBinding(K_JOY1 + 14, "weapprev");	// XINPUT_GAMEPAD_X
	Key_SetBinding(K_JOY1 + 15, "weapnext");

	Key_SetBinding(K_JOY20, "+zoom");
	Key_SetBinding(K_JOY21, "+attack");

	// create fake keyboard thread
	XkeyboardHandle = CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)keyboard_thread, NULL, CREATE_SUSPENDED, NULL);
	XSetThreadProcessor(XkeyboardHandle, 3);
	ResumeThread(XkeyboardHandle);

	oldButtons = buttons = 0;
}

void IN_Shutdown( void )
{

}

void IN_Restart( void )
{
}

void IN_Frame( void )
{
	xinput_update();
}


/*
===============
Sound
===============
*/
#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"

/* The audio callback. All the magic happens here. */
static int dmapos = 0;
static int dmasize = 0;

int Q3_XAudio2Init(void);
int Q3_XAudioGetPos(void);
int Q3_XAudioShutdown(void);
int Q3_XAudioSubmit(void);
int Q3_XAudioBegin(void);

#if 0 // use sdl now

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void)
{
	return (qboolean)Q3_XAudio2Init();
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int SNDDMA_GetDMAPos(void)
{
	return Q3_XAudioGetPos();
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void)
{
	Q3_XAudioShutdown();
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	Q3_XAudioSubmit();
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting (void)
{
	Q3_XAudioBegin();
}

#endif