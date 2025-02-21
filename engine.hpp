// This file contains declarations for the main Engine class. You will
// need to add declarations to this file as you develop your Engine.

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <chrono>
#include <mutex>

#include "io.hpp"

#include "Timer.hpp"
#include "OrderBook.hpp"
#include "SafeHashmap.hpp"
#include "Structs.hpp"

struct Engine
{
public:
	SafeHashMap<uint32_t, std::thread::id> orderIdToTid;
	SafeHashMap<uint32_t, std::string> orderIdToInstrument;
	SafeHashMap<uint32_t, Key> orderIdToKey;
	SafeHashMap<std::string, OrderBook> orderBooks;

	
	
	void accept(ClientConnection conn);

private:
	void connection_thread(ClientConnection conn);
};

inline std::chrono::microseconds::rep getCurrentTimestamp() noexcept
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

#endif
