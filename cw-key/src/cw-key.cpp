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
const int dashLong = 4;

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
	int layout;
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

		m[10] = (char*) ".----";
		m[11] = (char*) "..---";
		m[12] = (char*) "...--";
		m[13] = (char*) "....-";
		m[14] = (char*) ".....";
		m[15] = (char*) "-....";
		m[16] = (char*) "--...";
		m[17] = (char*) "---..";
		m[18] = (char*) "----.";
		m[19] = (char*) "-----";


		m[24] = (char*) "--.-"; // q
		m[25] = (char*) ".--"; // w
		m[26] = (char*) "."; // e
		m[27] = (char*) ".-."; // r
		m[28] = (char*) "-"; // t
		m[29] = (char*) "-.--"; // y
		m[30] = (char*) "..-"; // u
		m[31] = (char*) ".."; // i
		m[32] = (char*) "---"; // o
		m[33] = (char*) ".--."; // p

		m[38] = (char*) ".-"; // a
		m[39] = (char*) "..."; // s
		m[40] = (char*) "-.."; // d
		m[41] = (char*) "..-."; // f
		m[42] = (char*) "--."; // g
		m[43] = (char*) "...."; // h
		m[44] = (char*) ".---"; // j
		m[45] = (char*) "-.-"; // k
		m[46] = (char*) ".-.."; // l

		m[52] = (char*) "--.."; // z
		m[53] = (char*) "-..-"; // x
		m[54] = (char*) "-.-."; // c
		m[55] = (char*) "...-"; // v
		m[56] = (char*) "-..."; // b
		m[57] = (char*) "-."; // n
		m[58] = (char*) "--"; // m

		m[59] = (char*) ".-.-.-"; // ,
		m[60] = (char*) "......"; // .
		m[61] = (char*) "-..-."; // /



		// Буквы ru
		m[1024] = (char*) ".---"; // й
		m[1025] = (char*) "-.-."; // ц
		m[1026] = (char*) "..-"; // у
		m[1027] = (char*) "-.-"; // к
		m[1028] = (char*) "."; // е
		m[1029] = (char*) "-."; // н
		m[1030] = (char*) "--."; // г
		m[1031] = (char*) "----"; // ш
		m[1032] = (char*) "--.-"; // щ
		m[1033] = (char*) "--.."; // з

		m[1034] = (char*) "...."; // х
		m[1035] = (char*) ".--.-."; // ъ

		m[1038] = (char*) "..-."; // ф
		m[1039] = (char*) "-.--"; // ы
		m[1040] = (char*) ".--"; // в
		m[1041] = (char*) ".-"; // а
		m[1042] = (char*) ".--."; // п
		m[1043] = (char*) ".-."; // р
		m[1044] = (char*) "---"; // о
		m[1045] = (char*) ".-.."; // л
		m[1046] = (char*) "-.."; // д

		m[1047] = (char*) "...-"; // ж
		m[1048] = (char*) "...-..."; //э

		m[1052] = (char*) ".-.-"; // я
		m[1053] = (char*) "---."; // ч
		m[1054] = (char*) "..."; // с
		m[1055] = (char*) "--"; // м
		m[1056] = (char*) ".."; // и
		m[1057] = (char*) "-"; // т
		m[1058] = (char*) "-..-"; // ь

		m[1059] = (char*) "-..."; // б
		m[1060] = (char*) "..-"; // ю

		m[1061] = (char*) "......"; // .

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

				InputChar ic = qinput.front();

				int sk = ic.skanCode;
				int ly = ic.layout;

				if(ly != 0)
					sk += 1000;

				char* simbol = m[sk];

				unsigned long ptr = (unsigned long) simbol;
				cout << ptr << endl;
				if (ptr > 0)
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

	Display * display;

	char szKey[32];
	char szKeyOld[32] = { 0 };

	char szBit;
	char szBitOld;
	int iCheck;

	char szKeysym;
	char * szKeyString;

	int iKeyCode;

	Window focusWin = NULL;
	int iReverToReturn = NULL;

	printf("%s\n%s\n\n", "Linux Keylogger - Visit www.hamsterbaum.de",
			"Version: 0.1");

	display = XOpenDisplay(NULL);

	if (display == NULL) {
		printf("Error: XOpenDisplay");
		return -1;
	}

	MorseThread* mt = new MorseThread();
	ThreadPtr pmt(mt);
	mt->start();

	while (true) {
		XQueryKeymap(display, szKey);

		if (memcmp(szKey, szKeyOld, 32) != NULL) {
			for (int i = 0; i < sizeof(szKey); i++) {
				szBit = szKey[i];
				szBitOld = szKeyOld[i];
				iCheck = 1;

				for (int j = 0; j < 8; j++) {
					if ((szBit & iCheck) && !(szBitOld & iCheck)) {
						iKeyCode = i * 8 + j;

						szKeysym = XKeycodeToKeysym(display, iKeyCode, 0);
						szKeyString = XKeysymToString(szKeysym);

						XGetInputFocus(display, &focusWin, &iReverToReturn);
						printf("WindowID %x Key: %s\n", focusWin, szKeyString);

						XkbDescPtr keyboard = XkbGetKeyboard(display,
								XkbAllComponentsMask, XkbUseCoreKbd);

						XkbStateRec state;
						XkbGetState(display, XkbUseCoreKbd, &state);

						unsigned int group = (unsigned int) state.group;
						std::cout << group << std::endl;

						std::string s1 = XGetAtomName(display,
								keyboard->names->symbols);
						std::string s2 = XGetAtomName(display,
								keyboard->names->groups[group]);
						;

						std::cout<<iKeyCode<<"--"<< s1 << " - " << s2 << std::endl;

						InputChar ic;
						ic.skanCode = iKeyCode;
						ic.layout = group;
						mt->add(ic);

					}
					iCheck = iCheck << 1;
				}
			}
		}
		memcpy(szKeyOld, szKey, 32);
		usleep(10000);
	}
	XCloseDisplay(display);

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

	// Test

//	for (int i = 0; i < 10; i++) {
//		InputChar ic;
//		ic.skanCode = 8;
//		mt->add(ic);
//
//		ic.skanCode = 4;
//		mt->add(ic);
//	}

	//InputChar ic;
	//for (;;) {

	//}

	//pmt->start();

	//pmt->wait();

	return EXIT_SUCCESS;
}
