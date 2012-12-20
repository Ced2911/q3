#include <xtl.h>
#include <xaudio2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern "C" {
	#include "../sys/sys_local.h"
	#include "../client/client.h"
	#include "../qcommon/q_shared.h"
	#include "../client/snd_local.h"
}
/*
===============
Sound
===============
*/

extern "C" int Q3_XAudio2Init(void);
extern "C" int Q3_XAudioGetPos(void);
extern "C" int Q3_XAudioShutdown(void);
extern "C" int Q3_XAudioSubmit(void);
extern "C" int Q3_XAudioBegin(void);



bool snd_inited = qfalse;

/* The audio callback. All the magic happens here. */
static int dmapos = 0;
static int dmasize = 0;
static int xaudio_buffer_size = 0;

static IXAudio2* pXAudio2 = NULL;
static IXAudio2MasteringVoice* pMasterVoice = NULL;
static IXAudio2SourceVoice* pSourceVoice;
static XAUDIO2_BUFFER Xaudio2Buffer;


class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	HANDLE hBufferEndEvent;
	VoiceCallback(): hBufferEndEvent( CreateEvent( NULL, FALSE, FALSE, NULL ) ){}
	~VoiceCallback(){ CloseHandle( hBufferEndEvent ); }

	//Called when the voice has just finished playing a contiguous audio stream.
	void OnStreamEnd() { SetEvent( hBufferEndEvent ); }

	//Unused methods are stubs
	void OnVoiceProcessingPassEnd() { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {   
	
	
	}
	void OnBufferEnd(void * pBufferContext)    { 
		XAUDIO2_BUFFER buf = {};
		buf.AudioBytes = xaudio_buffer_size;
		buf.pAudioData = (BYTE*)dma.buffer;
		pSourceVoice->SubmitSourceBuffer( &buf );
	}
	void OnBufferStart(void * pBufferContext) { 
	}
	void OnLoopEnd(void * pBufferContext) {    }
	void OnVoiceError(void * pBufferContext, HRESULT Error) { }
};

static VoiceCallback Xaudio2Callback;

/*
===============
SNDDMA_Init
===============
*/
int Q3_XAudio2Init(void)
{
	HRESULT hr;
	XAUDIO2_DEVICE_DETAILS deviceDetails;
	WAVEFORMATEX wfx = {};

	if (snd_inited)
		return true;

	// create audio on thread 4 or 5
	if ( FAILED(hr = XAudio2Create( &pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR ) ) ) {
		return false;
	}

	pXAudio2->GetDeviceDetails(0, &deviceDetails);

	if ( FAILED(hr = pXAudio2->CreateMasteringVoice( &pMasterVoice, XAUDIO2_DEFAULT_CHANNELS,
                            XAUDIO2_DEFAULT_SAMPLERATE, 0, 0, NULL ) ) ) {
		return false;
	}

	//memcpy(&wfx, &deviceDetails.OutputFormat.Format, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.wBitsPerSample = 16;
	//wfx.nSamplesPerSec = 48000;
	wfx.nSamplesPerSec = 22050;
	wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample/ 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	

	if( FAILED(hr = pXAudio2->CreateSourceVoice( &pSourceVoice, (WAVEFORMATEX*)&wfx,
              0, XAUDIO2_DEFAULT_FREQ_RATIO, &Xaudio2Callback, NULL, NULL ) ) ) {
		return false;
	}

	Com_Printf( "XAudio2 Init..." );	
	Com_Printf( "OK\n" );

	dmapos = 0;
	dma.samplebits = wfx.wBitsPerSample;  // first byte of format is bits.
	dma.channels = wfx.nChannels;
	dma.samples = wfx.nAvgBytesPerSec/(dma.samplebits/8);
	dma.submission_chunk = 1;
	dma.speed = wfx.nSamplesPerSec;
	//dmasize = (dma.samples * (dma.samplebits/8));
	dmasize = wfx.nAvgBytesPerSec;
	dma.buffer = (byte*)XPhysicalAlloc(dmasize, MAXULONG_PTR, 2048,PAGE_READWRITE);

	xaudio_buffer_size = wfx.nAvgBytesPerSec / 2;

	Xaudio2Buffer.AudioBytes = xaudio_buffer_size;
	Xaudio2Buffer.pAudioData = (BYTE*)dma.buffer;

	pSourceVoice->SubmitSourceBuffer( &Xaudio2Buffer );
	pSourceVoice->Start( 0, XAUDIO2_COMMIT_NOW );

	snd_inited = true;
	return true;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int Q3_XAudioGetPos(void)
{
	//return dmapos;

	XAUDIO2_VOICE_STATE voiceState;
	pSourceVoice->GetState(&voiceState, 0);
	/*
	int ret = dmapos;
	dmapos = 0;
	return ret;
	*/

	return voiceState.BuffersQueued;
}

/*
===============
SNDDMA_Shutdown
===============
*/
int Q3_XAudioShutdown(void)
{
	Com_Printf("Closing XAudio2 ...\n");
	free(dma.buffer);
	dma.buffer = NULL;
	dmapos = dmasize = 0;
	snd_inited = qfalse;
	Com_Printf("XAudio2 shut down.\n");
	return 0;
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
int Q3_XAudioSubmit(void)
{
	/*
	Xaudio2Buffer.AudioBytes = xaudio_buffer_size;
	Xaudio2Buffer.pAudioData = (BYTE*)dma.buffer;
	pSourceVoice->SubmitSourceBuffer( &Xaudio2Buffer );
	*/
	return 0;
}

/*
===============
SNDDMA_BeginPainting
===============
*/
int Q3_XAudioBegin(void)
{
	/*
	Xaudio2Buffer.AudioBytes = xaudio_buffer_size;
	Xaudio2Buffer.pAudioData = (BYTE*)dma.buffer;
	pSourceVoice->SubmitSourceBuffer( &Xaudio2Buffer );
	pSourceVoice->Start( 0, XAUDIO2_COMMIT_NOW );
	*/
	return 0;
}

