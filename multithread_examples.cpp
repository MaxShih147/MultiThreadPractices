//#define Example_0
#define Example_1

#if defined(Example_0)
#include <condition_variable>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

// Global variables for all the threads.
std::condition_variable g_cond_var;
std::string             g_data;
std::mutex              g_mutex;
bool                    g_ready = false;
bool                    g_completed = false;

// Thread for heavy calculations.
void CallSomeDllFunction() {
	
	// Create a lock.
	std::unique_lock<std::mutex> lock(g_mutex);

	// Wait for main thread ready.
	g_cond_var.wait(lock, [] {return g_ready; });

	// Do some heavy calculations.
	// Assume take 1 sec.
	std::cout << "|> Thread C : Calculating...\n";
	std::this_thread::sleep_for(std::chrono::seconds(1));
	g_data += " is completed.";

	// Send the signal to main thread.
	g_completed = true;
	std::cout << "|> Thread C : Calculation completed...\n";

	// Unlock before notify all the thread.
	lock.unlock();
	g_cond_var.notify_one();
}

int main() {

	// Create a thread for heavy calculations.
	std::thread calculation(CallSomeDllFunction);

	// Send the signal to the calculation thread.
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		std::cout << "|> Thread M : Preprocessing...\n";
		std::this_thread::sleep_for(std::chrono::seconds(1));
		g_data = "#147";
		g_ready = true;
		std::cout << "|> Thread M : Data is prepared...\n";
	}
	g_cond_var.notify_one();

	// Wait for calculation thread.
	{
		std::unique_lock<std::mutex> lock(g_mutex);
		g_cond_var.wait(lock, [] {return g_completed; });
	}
	std::cout << "|> Thread M : Back to main thread, data " << g_data << "\n";

	// Finish.
	calculation.join();
	return 0;
}
#endif

#if defined(Example_1)
#include <condition_variable>
#include <iostream>
#include <thread> 
#include <vector>
#include <mutex>

class DataBuffer {
public:
	// Sharing Data
	std::vector<int> buffer;
	int capacity;
private:
	std::mutex buf_mutex;
	//std::condition_variable add_queue;    // Let "Add" wait if buffer is full.
	std::condition_variable remove_queue; // Let "Remove" wait if buffer is empty.
public:
	DataBuffer(int _capacity) : capacity(_capacity) {}

	// Function #1
	void Add(int data) {
		
		std::unique_lock<std::mutex> lock(buf_mutex);

		// Could not "Add" until the buffer is not full.
		//while (buffer.size() == capacity) {
		//	add_queue.wait(lock);
		//}

		// Execute "Add" action.
		buffer.push_back(data);
		std::cout << "|> Thread A : add " << buffer.size() << "\n";
		lock.unlock();

		// Because buffer is not empty now, so notify(awake) remove_queue.
		remove_queue.notify_one();
	}

	// Function #2
	int Remove() {

		std::unique_lock<std::mutex> lock(buf_mutex);

		// Could not "Remove" until the buffer is not empty.
		while (buffer.empty()) {
			remove_queue.wait(lock);
		}

		// Execute "Remove" action. Assume we always remove the begin.
		int ret = *buffer.begin();
		buffer.erase(buffer.begin());
		std::cout << "|> Thread R : remove " << buffer.size() << "\n";
		lock.unlock();

		// Because buffer is not full now, so notify(awake) add_queue.
		//add_queue.notify_one();
		return ret;
	}
};

int g_loop = 10;

void AddingProcess(int id, DataBuffer &db) {
	for (auto i = 0; i < g_loop; ++i) {
		db.Add(i);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

void RemovingProcess(int id, DataBuffer &db) {
	for (auto i = 0; i < g_loop; ++i) {
		int ret = db.Remove();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

int main() {

	DataBuffer db(4);

	std::thread A0(AddingProcess, 0, std::ref(db));
	std::thread A1(AddingProcess, 1, std::ref(db));
	std::thread A2(AddingProcess, 2, std::ref(db));
	std::thread R0(RemovingProcess, 0, std::ref(db));
	std::thread R1(RemovingProcess, 1, std::ref(db));

	A0.join();
	A1.join();
	A2.join();
	R0.join();
	R1.join();

	std::cout << "|> Thread Main : result " << db.buffer.size() << "\n";

	return 0;
}
#endif
