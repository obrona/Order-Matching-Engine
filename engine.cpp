#include <iostream>
#include <thread>
#include <unordered_map>
#include <string>

#include "io.hpp"
#include "engine.hpp"
#include "Structs.hpp"
#include "Timer.hpp"





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

			// checl if order was created by this thread, and it has a key from being added to buy/sell book
			bool flag = orderIdToTid.containsAndMatch(input.order_id, this_thread::get_id())
				&& orderIdToKey.contains(input.order_id);
			
			if (!flag) {
				Output::OrderDeleted(input.order_id, false, Timer::getTime());
			} else {
				std::string instr = orderIdToInstrument.getValue(input.order_id);
				Key k = orderIdToKey.getValue(input.order_id);
				OrderBook& ob = orderBooks.getValue(instr);
				ob.cancelOrder(input, k);
			}
		} else {
			orderIdToTid.insert(input.order_id, this_thread::get_id());
			orderIdToInstrument.insert(input.order_id, std::string(input.instrument));
			
			// may be a new order book, as this order is the 1st for order's instrument
			// so it can be a write to the hashmap
			OrderBook& ob = orderBooks.writeAndGetValue(std::string(input.instrument));
			Key k;
			
			if (input.price == 0) continue;

			if (input.type == 'B') {
				ob.matchBuy(input, k);
			} else {
				ob.matchSell(input, k);
			}

			if (k.price != 0 && k.time != 0 && k.rid != 0) {
				orderIdToKey.insert(input.order_id, k);
			}
		}

		
	}
}
