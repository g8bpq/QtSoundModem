// Pulse Audio bits for QtSoundmodem


#include <stdio.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#define UNUSED(x) (void)(x)

extern char CaptureNames[16][256];
extern char PlaybackNames[16][256];

extern int PlaybackCount;
extern int CaptureCount;

#include <dlfcn.h>

void *handle = NULL;
void *shandle = NULL;

pa_mainloop * (*ppa_mainloop_new)(void);
pa_mainloop_api * (*ppa_mainloop_get_api)(pa_mainloop * m);
pa_context * (*ppa_context_new)(pa_mainloop_api *mainloop, const char *name);
int (*ppa_context_connect)(pa_context * c, const char * server, pa_context_flags_t flags, const pa_spawn_api * 	api);
void (*ppa_context_set_state_callback)(pa_context * c, pa_context_notify_cb_t cb, void * userdata);
int (*ppa_mainloop_iterate)(pa_mainloop * m, int block, int * retval);
void (*ppa_mainloop_free)(pa_mainloop * m);
void (*ppa_context_disconnect)(pa_context * c);
void (*ppa_context_unref)(pa_context * c);
const char * (*ppa_strerror)(int error);
pa_context_state_t(*ppa_context_get_state)(const pa_context *c);
pa_operation * (*ppa_context_get_sink_info_list)(pa_context * c, pa_sink_info_cb_t cb, void * userdata);
pa_operation * (*ppa_context_get_source_info_list)(pa_context * c, pa_source_info_cb_t cb, void * userdata);	
void (*ppa_operation_unref)(pa_operation * o);
pa_operation_state_t(*ppa_operation_get_state)(const pa_operation * o);


pa_simple * (*ppa_simple_new)(const char * 	server,
	const char * 	name,
	pa_stream_direction_t 	dir,
	const char * 	dev,
	const char * 	stream_name,
	const pa_sample_spec * 	ss,
	const pa_channel_map * 	map,
	const pa_buffer_attr * 	attr,
	int * 	error) = NULL;

pa_usec_t(*ppa_simple_get_latency)(pa_simple * s, int * error);
int(*ppa_simple_read)(pa_simple * s, void * data, size_t bytes, int * error);
int(*ppa_simple_write)(pa_simple * s, void * data, size_t bytes, int * error);

int(*ppa_simple_flush)(pa_simple * s, int * error);
void(*ppa_simple_free)(pa_simple * s);
int(*ppa_simple_drain)(pa_simple * s, int * error);

void * getModule(void *handle, char * sym)
{
	return dlsym(handle, sym);
}

void * initPulse()
{
	// Load the pulse libraries

	if (handle)
		return handle;			// already done

	handle = dlopen("libpulse.so", RTLD_LAZY);

	if (!handle)
	{
		fputs(dlerror(), stderr);
		return NULL;
	}

	if ((ppa_mainloop_new = getModule(handle, "pa_mainloop_new")) == NULL) return NULL;
	if ((ppa_mainloop_get_api = getModule(handle, "pa_mainloop_get_api")) == NULL) return NULL;
	if ((ppa_context_new = getModule(handle, "pa_context_new")) == NULL) return NULL;
	if ((ppa_context_connect = getModule(handle, "pa_context_connect")) == NULL) return NULL;
	if ((ppa_context_set_state_callback = getModule(handle, "pa_context_set_state_callback")) == NULL) return NULL;
	if ((ppa_mainloop_iterate = getModule(handle, "pa_mainloop_iterate")) == NULL) return NULL;
	if ((ppa_mainloop_free = getModule(handle, "pa_mainloop_free")) == NULL) return NULL;
	if ((ppa_context_disconnect = getModule(handle, "pa_context_disconnect")) == NULL) return NULL;
	if ((ppa_context_unref = getModule(handle, "pa_context_unref")) == NULL) return NULL;
	if ((ppa_strerror = getModule(handle, "pa_strerror")) == NULL) return NULL;
	if ((ppa_context_get_state = getModule(handle, "pa_context_get_state")) == NULL) return NULL;
	if ((ppa_context_get_sink_info_list = getModule(handle, "pa_context_get_sink_info_list")) == NULL) return NULL;
	if ((ppa_context_get_source_info_list = getModule(handle, "pa_context_get_source_info_list")) == NULL) return NULL;
	if ((ppa_operation_unref = getModule(handle, "pa_operation_unref")) == NULL) return NULL;
	if ((ppa_operation_get_state = getModule(handle, "pa_operation_get_state")) == NULL) return NULL;

	shandle = dlopen("libpulse-simple.so", RTLD_LAZY);

	if (!shandle)
	{
		fputs(dlerror(), stderr);
		return NULL;
	}

	if ((ppa_simple_new = getModule(shandle, "pa_simple_new")) == NULL) return NULL;
	if ((ppa_simple_get_latency = getModule(shandle, "pa_simple_get_latency")) == NULL) return NULL;
	if ((ppa_simple_read = dlsym(shandle, "pa_simple_read")) == NULL) return NULL;
	if ((ppa_simple_write = dlsym(shandle, "pa_simple_write")) == NULL) return NULL;
	if ((ppa_simple_flush = dlsym(shandle, "pa_simple_flush")) == NULL) return NULL;
	if ((ppa_simple_drain = dlsym(shandle, "pa_simple_drain")) == NULL) return NULL;
	if ((ppa_simple_free = dlsym(shandle, "pa_simple_free")) == NULL) return NULL;

	return shandle;
}





// Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
typedef struct pa_devicelist {
	uint8_t initialized;
	char name[512];
	uint32_t index;
	char description[256];
} pa_devicelist_t;

void pa_state_cb(pa_context *c, void *userdata);
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output);

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata) {
	pa_context_state_t state;
	int *pa_ready = userdata;

	state = ppa_context_get_state(c);
	switch (state) {
		// There are just here for reference
	case PA_CONTEXT_UNCONNECTED:
	case PA_CONTEXT_CONNECTING:
	case PA_CONTEXT_AUTHORIZING:
	case PA_CONTEXT_SETTING_NAME:
	default:
		break;
	case PA_CONTEXT_FAILED:
	case PA_CONTEXT_TERMINATED:
		*pa_ready = 2;
		break;
	case PA_CONTEXT_READY:
		*pa_ready = 1;
		break;
	}
}

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata)
{
	UNUSED(c);

	pa_devicelist_t *pa_devicelist = userdata;
	int ctr = 0;

	// If eol is set to a positive number, you're at the end of the list
	if (eol > 0) {
		return;
	}

	// We know we've allocated 16 slots to hold devices.  Loop through our
	// structure and find the first one that's "uninitialized."  Copy the
	// contents into it and we're done.  If we receive more than 16 devices,
	// they're going to get dropped.  You could make this dynamically allocate
	// space for the device list, but this is a simple example.
	for (ctr = 0; ctr < 16; ctr++) {
		if (!pa_devicelist[ctr].initialized) {
			strncpy(pa_devicelist[ctr].name, l->name, 511);
			strncpy(pa_devicelist[ctr].description, l->description, 255);
			pa_devicelist[ctr].index = l->index;
			pa_devicelist[ctr].initialized = 1;
			break;
		}
	}
}

// See above.  This callback is pretty much identical to the previous
void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata) 
{
	UNUSED(c);

	pa_devicelist_t *pa_devicelist = userdata;
	int ctr = 0;

	if (eol > 0) {
		return;
	}

	for (ctr = 0; ctr < 16; ctr++) {
		if (!pa_devicelist[ctr].initialized) {
			strncpy(pa_devicelist[ctr].name, l->name, 511);
			strncpy(pa_devicelist[ctr].description, l->description, 255);
			pa_devicelist[ctr].index = l->index;
			pa_devicelist[ctr].initialized = 1;
			break;
		}
	}
}

int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output) {
	// Define our pulse audio loop and connection variables
	pa_mainloop *pa_ml;
	pa_mainloop_api *pa_mlapi;
	pa_operation *pa_op;
	pa_context *pa_ctx;


	// We'll need these state variables to keep track of our requests
	int state = 0;
	int pa_ready = 0;

	// Initialize our device lists
	memset(input, 0, sizeof(pa_devicelist_t) * 16);
	memset(output, 0, sizeof(pa_devicelist_t) * 16);

	// Create a mainloop API and connection to the default server
	pa_ml = ppa_mainloop_new();
	pa_mlapi = ppa_mainloop_get_api(pa_ml);
	pa_ctx = ppa_context_new(pa_mlapi, "test");

	// This function connects to the pulse server
	ppa_context_connect(pa_ctx, NULL, 0, NULL);


	// This function defines a callback so the server will tell us it's state.
	// Our callback will wait for the state to be ready.  The callback will
	// modify the variable to 1 so we know when we have a connection and it's
	// ready.
	// If there's an error, the callback will set pa_ready to 2
	ppa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

	// Now we'll enter into an infinite loop until we get the data we receive
	// or if there's an error
	for (;;) {
		// We can't do anything until PA is ready, so just iterate the mainloop
		// and continue
		if (pa_ready == 0) {
			ppa_mainloop_iterate(pa_ml, 1, NULL);
			continue;
		}
		// We couldn't get a connection to the server, so exit out
		if (pa_ready == 2) {
			ppa_context_disconnect(pa_ctx);
			ppa_context_unref(pa_ctx);
			ppa_mainloop_free(pa_ml);
			return -1;
		}
		// At this point, we're connected to the server and ready to make
		// requests
		switch (state) {
			// State 0: we haven't done anything yet
		case 0:
			// This sends an operation to the server.  pa_sinklist_info is
			// our callback function and a pointer to our devicelist will
			// be passed to the callback The operation ID is stored in the
			// pa_op variable
			pa_op = ppa_context_get_sink_info_list(pa_ctx,
				pa_sinklist_cb,
				output
			);

			// Update state for next iteration through the loop
			state++;
			break;
		case 1:
			// Now we wait for our operation to complete.  When it's
			// complete our pa_output_devicelist is filled out, and we move
			// along to the next state
			if (ppa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
				ppa_operation_unref(pa_op);

				// Now we perform another operation to get the source
				// (input device) list just like before.  This time we pass
				// a pointer to our input structure
				pa_op = ppa_context_get_source_info_list(pa_ctx,
					pa_sourcelist_cb,
					input
				);
				// Update the state so we know what to do next
				state++;
			}
			break;
		case 2:
			if (ppa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
				// Now we're done, clean up and disconnect and return
				ppa_operation_unref(pa_op);
				ppa_context_disconnect(pa_ctx);
				ppa_context_unref(pa_ctx);
				ppa_mainloop_free(pa_ml);
				return 0;
			}
			break;
		default:
			// We should never see this state
			fprintf(stderr, "in state %d\n", state);
			return -1;
		}
		// Iterate the main loop and go again.  The second argument is whether
		// or not the iteration should block until something is ready to be
		// done.  Set it to zero for non-blocking.
		ppa_mainloop_iterate(pa_ml, 1, NULL);
	}
}

int listpulse()
{
	int ctr;
	
	PlaybackCount = 0;
	CaptureCount = 0;


	// This is where we'll store the input device list
	pa_devicelist_t pa_input_devicelist[16];

	// This is where we'll store the output device list
	pa_devicelist_t pa_output_devicelist[16];

	if (pa_get_devicelist(pa_input_devicelist, pa_output_devicelist) < 0) {
		fprintf(stderr, "failed to get device list\n");
		return 1;
	}

	printf("Pulse Playback Devices\n\n");

	for (ctr = 0; ctr < 16; ctr++)
	{
		if (!pa_output_devicelist[ctr].initialized)
			break;

		printf("Name: %s\n", pa_output_devicelist[ctr].name);
		strcpy(&PlaybackNames[PlaybackCount++][0], pa_output_devicelist[ctr].name);

	}

	printf("Pulse Capture Devices\n\n");

	for (ctr = 0; ctr < 16; ctr++)
	{
		if (!pa_input_devicelist[ctr].initialized)
			break;

		printf("Name: %s\n", pa_input_devicelist[ctr].name);
		strcpy(&CaptureNames[CaptureCount++][0], pa_input_devicelist[ctr].name);
	}
	return 0;
}


pa_simple * OpenPulsePlayback(char * Server)
{
	pa_simple * s;
	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16NE;
	ss.channels = 2;
	ss.rate = 12000;
	int error;


	s = (*ppa_simple_new)(NULL,               // Use the default server.
		"QtSM",           // Our application's name.
		PA_STREAM_PLAYBACK,
		Server,

		"Playback",         // Description of our stream.
		&ss,                // Our sample format.
		NULL,               // Use default channel map
		NULL,               // Use default buffering attributes.
		&error
	);

	if (s == 0)
		printf("Playback pa_simple_new() failed: %s\n", ppa_strerror(error));
	else
		printf("Playback Handle %x\n", (unsigned int)s);

	return s;
}

pa_simple * OpenPulseCapture(char * Server)
{
	pa_simple * s;
	pa_sample_spec ss;
	ss.format = PA_SAMPLE_S16NE;
	ss.channels = 2;
	ss.rate = 12000;
	int error;

	pa_buffer_attr attr;

	attr.maxlength = -1;
	attr.tlength = -1;
	attr.prebuf = -1;
	attr.minreq = -1;
	attr.fragsize = 512;


	s = (*ppa_simple_new)(NULL,               // Use the default server.
		"QtSM",           // Our application's name.
		PA_STREAM_RECORD,
		Server,
		"Capture",            // Description of our stream.
		&ss,                // Our sample format.
		NULL,               // Use default channel map
		&attr, 
		&error
	);

	if (s == 0)
		printf("Capture pa_simple_new() failed: %s\n", ppa_strerror(error));
	else
		printf("Capture Handle %x\n", (unsigned int)s);

	return s;
}

pa_simple * spc = 0;			// Capure Handle
pa_simple * spp = 0;			// Playback Handle

int pulse_audio_open(char * CaptureDevice, char * PlaybackDevice)
{
	pa_usec_t latency;
	int error;
		
	spc = OpenPulseCapture(CaptureDevice);
	spp = OpenPulsePlayback(PlaybackDevice);

	if (spc && spp)
	{
		if ((latency = ppa_simple_get_latency(spc, &error)) == (pa_usec_t)-1) {
			printf("cap simple_get_latency() failed: %s\n", ppa_strerror(error));
		}
		else
			printf("cap %0.0f usec    \n", (float)latency);

		if ((latency = ppa_simple_get_latency(spp, &error)) == (pa_usec_t)-1) {
			printf("play simple_get_latency() failed: %s\n", ppa_strerror(error));
		}
		else
			printf("play %0.0f usec    \n", (float)latency);

		return 1;
	}
	else
		return 0;

}

void pulse_audio_close()
{
	int error;

	ppa_simple_flush(spc, &error);
	ppa_simple_free(spc);
	spc = 0;

	ppa_simple_drain(spp, &error);
	ppa_simple_free(spp);
	spp = 0;
}


int pulse_read(short * samples, int nSamples)
{
	int error;
	int nBytes = nSamples * 4;

	if (spc == 0)
		return 0;

	if (ppa_simple_read(spc, samples, nBytes, &error) < 0)
	{
		printf("Pulse pa_simple_read() failed: %s\n", ppa_strerror(error));
		return 0;
	}

	return nSamples;
}


int pulse_write(short * ptr, int len)
{
	int k;
	int error;

	if (spp == 0)
		return 0;

	k = ppa_simple_write(spp, ptr, len * 4, &error);

	if (k < 0)
	{
		printf("Pulse pa_simple_write() failed: %s\n", ppa_strerror(error));
		return -1;
	}

	return 0;
}

void pulse_flush()
{
	int error;

	if (spp == 0)
		return;

	if (ppa_simple_flush(spp, &error) < 0)
		printf("Pulse pa_simple_flush() failed: %s\n", ppa_strerror(error));
}
