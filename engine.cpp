#include <iostream>
#include <thread>
#include <unordered_map>
#include <string>

#include "io.hpp"
#include "engine.hpp"





void Engine::accept(ClientConnection connection)
{
	auto thread = std::thread(&Engine::connection_thread, this, std::move(connection));
	thread.detach();
}

void Engine::connection_thread(ClientConnection connection)
{
	while(true)
	{
		ClientCommand input {};
		switch(connection.readInput(input))
		{
			case ReadResult::Error: SyncCerr {} << "Error reading input" << std::endl;
			case ReadResult::EndOfFile: return;
			case ReadResult::Success: break;
		}

		// Functions for printing output actions in the prescribed format are
		// provided in the Output class:
		if (input.type == 'C') {
			bool flag;
			unique_lock<mutex> ulock(mut1);
			flag = orderIdToTid.contains(input.order_id) && orderIdToTid[input.order_id] == this_thread::get_id();
			ulock.unlock();

			if (!flag) {
				Output::OrderDeleted(input.order_id, false, 0);
			} else {
				unique_lock<mutex> ulock(mut2);
				OrderBook& ob = orderBooks[string(input.instrument)];
				ulock.unlock();

				ob.cancelOrder(input);
			}
		} else {
			unique_lock<mutex> ulock1(mut1);
			orderIdToTid[input.order_id] = this_thread::get_id();
			ulock1.unlock();

			unique_lock<mutex> ulock2(mut2);
			OrderBook& ob = orderBooks[string(input.instrument)];
			ulock2.unlock();

			if (input.type == 'B') {
				cout << "buy received" << endl;
				ob.matchBuy(input);
			} else {
				ob.matchSell(input);
			}
		}

		
	}
}
