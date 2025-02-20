// This file contains declarations for the main Engine class. You will
// need to add declarations to this file as you develop your Engine.

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <chrono>
#include <mutex>

#include "OrderBook.hpp"
#include "io.hpp"

struct Engine
{
public:
	std::mutex mut1;
	std::unordered_map<uint32_t, std::thread::id> orderIdToTid;
	
	std::mutex mut2;
	std::unordered_map<std::string, OrderBook> orderBooks;
	
	void accept(ClientConnection conn);

private:
	void connection_thread(ClientConnection conn);
};

inline std::chrono::microseconds::rep getCurrentTimestamp() noexcept
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

#endif
