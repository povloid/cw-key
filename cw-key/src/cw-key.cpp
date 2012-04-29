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

class MorseThread: public Thread {
private:

	map<int, char*> m;

public:

	MorseThread() {
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

		for (int i = 0; i < 20; i++, sleep(1))
			std::cout << "a  " << std::endl;
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

	//mt->wait();

	return EXIT_SUCCESS;
}
