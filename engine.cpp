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
			bool flag = orderIdToTid.contains(input.order_id) && orderIdToTid.getValue(input.order_id) == this_thread::get_id();
			
			if (!flag) {
				Output::OrderDeleted(input.order_id, false, 0);
			} else {
				std::string instr = orderIdToInstrument.getValue(input.order_id);
				OrderBook& ob = orderBooks.getValue(instr);
				ob.cancelOrder(input);
			}
		} else {
			
			orderIdToTid.insert(input.order_id, this_thread::get_id());
			orderIdToInstrument.insert(input.order_id, std::string(input.instrument));
			
			OrderBook& ob = orderBooks.getValue(std::string(input.instrument));
			
			if (input.type == 'B') {
				ob.matchBuy(input);
			} else {
				ob.matchSell(input);
			}
		}

		
	}
}
