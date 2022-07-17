#include <array>
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <sqlite3.h>

template <typename T>
class SafeQueue
{
// source:
// https://stackoverflow.com/questions/15278343/c11-thread-safe-queue
public:
  SafeQueue(void)
    : q()
    , m()
    , c()
  {}

  ~SafeQueue(void)
  {}

  // Add an element to the queue.
  void push(T t)
  {
    std::lock_guard<std::mutex> lock(m);
    q.push(t);
    c.notify_one();
  }

  // Get the "front"-element.
  // If the queue is empty, wait till a element is avaiable.
  T dequeue(void)
  {
    std::unique_lock<std::mutex> lock(m);
    while(q.empty())
    {
      // release lock as long as the wait and reaquire it afterwards.
      c.wait(lock);
    }
    T val = q.front();
    q.pop();
    return val;
  }

  bool empty()
  {
      std::unique_lock<std::mutex> lock(m);
      return q.empty();
  }

private:
  std::queue<T> q;
  mutable std::mutex m;
  std::condition_variable c;
};


void remove_db(void)
{
	// check if sqlite DB exists
	// if so: delete and generate from scratch
	if (std::filesystem::exists("./test_db.sqlite3")) {
		std::filesystem::remove("./test_db.sqlite3");
	}
}

static int callback(void* data, int argc, char** argv, char** azColName)
{
    int i;
    fprintf(stderr, "%s: ", (const char*)data);

    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}


bool generate_empty_db(void)
{
	remove_db();
	bool res = true;
	char* db_message_error;
	int sqlite3_ret = 0;
	sqlite3* DB;
	sqlite3_ret = sqlite3_open("test_db.sqlite3", &DB);

	if (sqlite3_ret != 0) {
		res = false;
	} else {
		std::string sql =
			"CREATE TABLE test_table("
				"ID			INTEGER		PRIMARY KEY AUTOINCREMENT, "
		        "NAME		TEXT		NOT NULL, "
		        "IMAGE    	TEXT	    NOT NULL, "
		        "SOME_REAL  REAL        NOT NULL, "
				"TS         TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";
		sqlite3_ret = sqlite3_exec(DB, sql.c_str(),
				callback, 0, &db_message_error);
		if (sqlite3_ret) {
			std::cout << db_message_error << std::endl;
		} else {
			sqlite3_close(DB);
		}
	}
	return res;
}

std::string generate_random_string(uint64_t str_length)
{
	const std::string chars = "abcdefghijklmnopqrstuvwxyz"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::string rand_str = "";
	for(uint64_t i=0;i<str_length;++i)
	{
		rand_str += chars[rand() % chars.length()];
	}


	return rand_str;
}


void add_data_to_db(SafeQueue<std::string> &runtimes_shared, bool interrupt)
{
	srand(time(NULL));
	std::string name = generate_random_string(20);
	std::string image = generate_random_string(12000);

	std::string sql = "INSERT INTO test_table(";
	sql += "NAME,IMAGE,SOME_REAL) ";
	sql += "VALUES('";
	sql += name;
	sql += "', '";
	sql += image;
	sql += "', 2.3);";

	int sqlite3_ret = 0;
	char* db_message_error;
	sqlite3* DB;
	sqlite3_ret = sqlite3_open("test_db.sqlite3", &DB);
	if (sqlite3_ret == SQLITE_OK && sqlite3_ret != SQLITE_BUSY) {
		std::string sql_pragma = "PRAGMA busy_timeout=30000";
		sqlite3_ret = sqlite3_exec(DB, sql_pragma.c_str(),
				callback, 0, &db_message_error);
		if (sqlite3_ret != SQLITE_OK && sqlite3_ret != SQLITE_BUSY) {
			std::cout << db_message_error << std::endl;
			sqlite3_free(db_message_error);
		} else {
			auto start_time = std::chrono::high_resolution_clock::now();
			for (int i=0; i<2000; ++i) {
				sqlite3_ret = sqlite3_exec(DB, sql.c_str(),
						callback, 0, &db_message_error);
				if (sqlite3_ret != SQLITE_OK && sqlite3_ret != SQLITE_BUSY) {
					std::cout << db_message_error << std::endl;
					sqlite3_free(db_message_error);
				}
				if (interrupt) {
					std::this_thread::sleep_for(
						std::chrono::microseconds(200));
				}
			}
			auto end_time = std::chrono::high_resolution_clock::now();
			double duration = std::chrono::duration_cast
				<std::chrono::microseconds>(end_time - start_time).count();
			duration /= 2000;
			duration /= (1000*1000);
			runtimes_shared.push(std::to_string(duration));
		}
		sqlite3_close(DB);
	}
}


int main(void)
{
	std::cout << std::string(72, '=') << std::endl;
	std::cout <<
			"Running SQLite concurrency benchmark (C++)"
			<< std::endl;

    bool res = generate_empty_db();

    if (!res) {
    	remove_db();
    	return 1;
    }

    SafeQueue<std::string> runtimes_shared;
    std::array<size_t,4> no_parallel_workers = {
    	1,2,4,8};


	std::ofstream total_average;
	std::ofstream thread_average;
	if (!std::filesystem::exists("../results_total_average.csv")){
		total_average.open("../results_total_average.csv", std::ios_base::out);
		total_average << "language,threads,runtime(s)\n";
	    total_average.flush();
	} else {
		total_average.open("../results_total_average.csv", std::ios_base::app);
	}
	if (!std::filesystem::exists("../results_thread_average.csv")){
		thread_average.open("../results_thread_average.csv", std::ios_base::out);
		thread_average << "language,threads,runtime(s)\n";
		thread_average.flush();
	} else {
		thread_average.open("../results_thread_average.csv", std::ios_base::app);
	}

    for(uint8_t idx=0;idx<no_parallel_workers.size();++idx)
    {
    	size_t worker_count = no_parallel_workers[idx];
    	std::string msg("Benchmarking ");
    	if (worker_count == 1) {
    		msg += "1 connection";

    	} else {
    		msg += std::to_string(worker_count) + " connections";
    	}
    	std::cout << msg << std::endl;

    	bool res = generate_empty_db();

    	std::vector<std::thread> current_threads;
    	auto start_time = std::chrono::high_resolution_clock::now();
    	for(size_t i=0;i<worker_count;++i)
    	{
    		current_threads.push_back(std::thread(
    			add_data_to_db,std::ref(runtimes_shared),false));
    	}

    	uint8_t worker_res_recv_count = 0;
    	while (worker_res_recv_count < worker_count)
    	{
    		std::string res = runtimes_shared.dequeue();
    		std::cout << res << std::endl;
    		worker_res_recv_count += 1;
    		std::string res_thread = "C++,"+std::to_string(worker_count)+","+res+"\n";
    		thread_average << res_thread;
    		thread_average.flush();
    	}
    	auto end_time = std::chrono::high_resolution_clock::now();
		double duration = std::chrono::duration_cast
			<std::chrono::microseconds>(end_time - start_time).count();
		duration /= (2000*worker_count);
		duration /= (1000*1000);
		std::string res_total = "C++,"+std::to_string(worker_count)+","+std::to_string(duration)+"\n";
		total_average << res_total;
		total_average.flush();

    	for(size_t i=0;i<current_threads.size();++i)
    	{
    		current_threads[i].join();
    	}
    }

    remove_db();
    total_average.close();
    thread_average.close();
	std::cout << std::string(72, '=') << std::endl;
	return 0;
}
