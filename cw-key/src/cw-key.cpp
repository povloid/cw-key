//============================================================================
// Name        : cw-key.cpp
// Author      : UN7TAD
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <math.h>
using namespace std;

/**
 * Поток - базовое описание
 */
class Thread {
private:
	pthread_t thread;

	Thread(const Thread& copy); // copy constructor denied
	static void *thread_func(void *d) {
		((Thread *) d)->run();
	}

public:
	Thread() {
	}
	virtual ~Thread() {
	}

	virtual void run() = 0;

	int start() {
		return pthread_create(&thread, NULL, Thread::thread_func, (void*) this);
	}
	int wait() {
		return pthread_join(thread, NULL);
	}
};

typedef std::auto_ptr<Thread> ThreadPtr;

// Morse Thread -----------------------------------------------------------------------------------------------------------------------

const int ditLong = 1;
const int dashLong = 3;

const int ditDashLong = 1;
const int charLong = 4;
const int wordLong = 7;

const float AMPLITUDE = 10000;

char* device = (char*) "default"; /* playback device */
snd_output_t *output = NULL;
snd_pcm_t *handle;
snd_pcm_sframes_t frames;

class MorseThread: public Thread {
private:

	map<int, char*> m;
	unsigned short bufferDit[16 * 1024 * ditLong]; /* dit data */
	unsigned short bufferDash[16 * 1024 * dashLong]; /* dash data */

	unsigned short bufferDitDashPause[16 * 1024 * ditDashLong];
	unsigned short bufferCharPause[16 * 1024 * charLong];
	unsigned short bufferWordPause[16 * 1024 * wordLong];

	void setAll0(unsigned short b[]) {
		for (unsigned int i = 0; i < sizeof(b) / 2; i++) {
			b[i] = 0;
		}
	}

public:

	MorseThread() {
		setAll0(bufferDitDashPause);
		setAll0(bufferCharPause);
		setAll0(bufferWordPause);

		m[2] = (char*) ".----";
		m[3] = (char*) "..---";
		m[4] = (char*) "...--";
		m[5] = (char*) "....-";
		m[6] = (char*) ".....";
		m[7] = (char*) "-....";
		m[8] = (char*) "--...";
		m[9] = (char*) "---..";
		m[10] = (char*) "----.";
		m[11] = (char*) "-----";
	}

	void run() {

		int err;


		//for (int i = 0; i < 20; i++, sleep(1))
		//	std::cout << "a  " << std::endl;

		float amplitude = 0;
		int p = 0;
		int s = sizeof(bufferDit) / 2;

		for (int i = 0; i < s; i++) {
			if (amplitude < AMPLITUDE && i < (s - p)) {
				amplitude += 100;
				p = i;
			} else if (i > (s - p - 20) && amplitude > 0) {
				amplitude -= 100;
			}

			bufferDit[i] = sin(((float) i) / 12) * amplitude;
		}


		if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0))
				< 0) {
			printf("Playback open error: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
		}

		if ((err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16,
				SND_PCM_ACCESS_RW_INTERLEAVED, 1, 48000, 1, 50000)) < 0) { /* 0.5sec */
			printf("Playback open error: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < 5; i++) {

			frames = snd_pcm_writei(handle, bufferDit, sizeof(bufferDit) / 2);

			if (frames < 0)
				frames = snd_pcm_recover(handle, frames, 0);
			if (frames < 0) {
				printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
				break;
			}

			if (frames > 0 && frames < (long) sizeof(bufferDit) / 2)
				printf("Short write (expected %li, wrote %li)\n",
						(long) sizeof(bufferDit) / 2, frames);

			// ---------------------------------------------------------

			frames = snd_pcm_writei(handle, bufferDitDashPause, sizeof(bufferDitDashPause) / 2);

			if (frames < 0)
				frames = snd_pcm_recover(handle, frames, 0);
			if (frames < 0) {
				printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
				break;
			}

			if (frames > 0 && frames < (long) sizeof(bufferDitDashPause) / 2)
				printf("Short write (expected %li, wrote %li)\n",
						(long) sizeof(bufferDitDashPause) / 2, frames);

		}

		snd_pcm_close(handle);

	}
};

/**
 * MAIN -------------------------------------------------------------------------------------------------------------------------------
 */
int main(void) {

//	class Thread_a: public Thread {
//	public:
//		void run() {
//			for (int i = 0; i < 20; i++, sleep(1))
//				std::cout << "a  " << std::endl;
//		}
//	};
//
//	class Thread_b: public Thread {
//	public:
//		void run() {
//			for (int i = 0; i < 20; i++, sleep(1))
//				std::cout << "  b" << std::endl;
//		}
//	};
//
//	ThreadPtr a(new Thread_a());
//	ThreadPtr b(new Thread_b());
//
//	if (a->start() != 0 || b->start() != 0)
//		return EXIT_FAILURE;
//
//	if (a->wait() != 0 || b->wait() != 0)
//		return EXIT_FAILURE;

	ThreadPtr mt(new MorseThread());

	mt->start();

	mt->wait();

	return EXIT_SUCCESS;
}
