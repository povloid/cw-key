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
#include <queue>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>

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

const int A16 = 1;
const float AMPLITUDE = 10000;

char* device = (char*) "default"; /* playback device */
snd_output_t *output = NULL;
snd_pcm_t *handle;
snd_pcm_sframes_t frames;

struct InputChar {
public:
	int skanCode;
};

class MorseThread: public Thread {
private:

	queue<InputChar> qinput;

	map<int, char*> m;
	unsigned short bufferDit[A16 * 1024 * ditLong]; /* dit data */
	unsigned short bufferDash[A16 * 1024 * dashLong]; /* dash data */

	unsigned short bufferDitDashPause[A16 * 1024 * ditDashLong];
	unsigned short bufferCharPause[A16 * 1024 * charLong];
	unsigned short bufferWordPause[A16 * 1024 * wordLong];

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

	void add(InputChar ic) {
		qinput.push(ic);
	}

	void run() {

		int err;

		//for (int i = 0; i < 20; i++, sleep(1))
		//	std::cout << "a  " << std::endl;

		// Заготовки
		{
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
		}
		{
			float amplitude = 0;
			int p = 0;
			int s = sizeof(bufferDash) / 2;
			for (int i = 0; i < s; i++) {
				if (amplitude < AMPLITUDE && i < (s - p)) {
					amplitude += 100;
					p = i;
				} else if (i > (s - p - 20) && amplitude > 0) {
					amplitude -= 100;
				}

				bufferDash[i] = sin(((float) i) / 12) * amplitude;
			}
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

		for (;;) {
			while (!qinput.empty()) {

				int sk = qinput.front().skanCode;
				char* simbol = m[sk];

				while ((*simbol) != 0) {
					//cout<<*simbol<<'|';
					char s = *simbol;

					if (s == '.') {
						frames = snd_pcm_writei(handle, bufferDit,
								sizeof(bufferDit) / 2);
					} else if (s == '-') {
						frames = snd_pcm_writei(handle, bufferDash,
								sizeof(bufferDash) / 2);
					}

					frames = snd_pcm_writei(handle, bufferDitDashPause,
							sizeof(bufferDitDashPause) / 2);
					simbol++;
				}
				//cout<<endl;
				frames = snd_pcm_writei(handle, bufferCharPause,
						sizeof(bufferCharPause) / 2);
				qinput.pop();
			}

			frames = snd_pcm_writei(handle, bufferWordPause,
					sizeof(bufferWordPause) / 2);

		}

		snd_pcm_close(handle);

	}

};

double gettime() {
	timeval tim;
	gettimeofday(&tim, NULL);
	double t1 = tim.tv_sec + (tim.tv_usec / 1000000.0);
	return t1;
}

/**
 * MAIN -------------------------------------------------------------------------------------------------------------------------------
 */
int main(void) {

	Display *display_name;
	int depth, screen, connection;
	display_name = XOpenDisplay(NULL);
	screen = DefaultScreen(display_name);
	depth = DefaultDepth(display_name,screen);
	connection = ConnectionNumber(display_name);
	cout << "Keylogger started\n\nInfo about X11 connection:\n";
	cout << " The display is::%s\n", XDisplayName((char*) display_name);
	cout << " Width::" << DisplayWidth(display_name,screen)<< "\tHeight::"
			<< DisplayHeight(display_name,screen)<<"\n";
	cout << " Connection number is " << connection << "\n";

	if (depth == 1)
		cout << " You live in prehistoric times\n";
	else
		cout << " You've got a coloured monitor with depth of %d\n" << depth;

	cout << "\n\nLogging started.\n\n";

	char keys_return[32];

	while (1) {
		XQueryKeymap(display_name, keys_return);

		for (int i = 0; i < 32; i++) {
			if (keys_return[i] != 0) {
				cout << keys_return << " --- " << a << endl;
				int pos = 0;
				int num = keys_return[i];
				//printf("%.20f: ", gettime());

				while (pos < 8) {
					if ((num & 0x01) == 1) {

						//printf("%d ", i * 8 + pos);
						cout << i * 8 + pos << endl;
					}
					pos++;
					num /= 2;
				}
				//printf("\n");

//				XkbDescPtr keyboard = XkbGetKeyboard(display_name,
//						XkbAllComponentsMask, XkbUseCoreKbd);
//				XkbStateRec state;
//
//				XkbGetState(display_name, XkbUseCoreKbd, &state);
//
//				unsigned int group = (unsigned int) state.group;
//				std::cout << group << std::endl;
//
//				std::string s1 = XGetAtomName(display_name,
//						keyboard->names->symbols);
//				std::string s2 = XGetAtomName(display_name,
//						keyboard->names->groups[group]);
//
//				std::cout << s1 << " - " << s2 << std::endl;
			}
		}
		usleep(30000);

	}
	XCloseDisplay(display_name);

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

	MorseThread* mt = new MorseThread();
	ThreadPtr pmt(mt);
	// Test

//	for (int i = 0; i < 10; i++) {
//		InputChar ic;
//		ic.skanCode = 8;
//		mt->add(ic);
//
//		ic.skanCode = 4;
//		mt->add(ic);
//	}

	InputChar ic;
	for (;;) {

	}

	pmt->start();

	pmt->wait();

	return EXIT_SUCCESS;
}
